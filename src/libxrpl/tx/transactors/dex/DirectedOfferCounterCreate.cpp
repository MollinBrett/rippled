#include <xrpl/tx/transactors/dex/DirectedOfferCounterCreate.h>

#include <xrpl/basics/Log.h>
#include <xrpl/beast/utility/Zero.h>
#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/ReadView.h>
#include <xrpl/ledger/View.h>
#include <xrpl/ledger/entries/DirectedOfferCounterEntry.h>
#include <xrpl/ledger/entries/DirectedOfferEntry.h>
#include <xrpl/ledger/helpers/AccountRootHelpers.h>
#include <xrpl/ledger/helpers/DirectedOfferHelpers.h>
#include <xrpl/ledger/helpers/DirectoryHelpers.h>
#include <xrpl/ledger/helpers/EscrowHelpers.h>
#include <xrpl/ledger/helpers/SponsorHelpers.h>
#include <xrpl/ledger/helpers/TokenHelpers.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Asset.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/Keylet.h>
#include <xrpl/protocol/MPTAmount.h>
#include <xrpl/protocol/MPTIssue.h>
#include <xrpl/protocol/Protocol.h>
#include <xrpl/protocol/Rate.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STAmount.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/XRPAmount.h>
#include <xrpl/tx/Transactor.h>

#include <variant>

namespace xrpl {

/*
    DirectedOfferCounterCreate
    ==========================

    A named buyer locks a payment as a bid against a DirectedOffer, for a
    quantity at a total price of its choosing. The seller takes it with
    DirectedOfferAccept, which settles both locked legs at once; the buyer or
    the seller withdraws it with DirectedOfferCancel. See
    docs/directed-offer.md.
*/

//------------------------------------------------------------------------------

namespace {

// A positive amount within the protocol maximum for an MPT.
bool
isPositiveBidAmount(STAmount const& amount)
{
    return amount > beast::kZero &&
        !(amount.holds<MPTIssue>() && amount.mpt() > MPTAmount{kMaxMpTokenAmount});
}

}  // namespace

TxConsequences
DirectedOfferCounterCreate::makeTxConsequences(PreflightContext const& ctx)
{
    auto const amount = ctx.tx[sfAmount];
    return TxConsequences{ctx.tx, isXRP(amount) ? amount.xrp() : beast::kZero};
}

bool
DirectedOfferCounterCreate::checkExtraFeatures(PreflightContext const& ctx)
{
    return directedOfferFeaturesEnabled(ctx.rules);
}

NotTEC
DirectedOfferCounterCreate::preflight(PreflightContext const& ctx)
{
    if (ctx.tx[sfDirectedOfferID] == beast::kZero)
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: zero DirectedOfferID.";
        return temMALFORMED;
    }

    STAmount const quantity = ctx.tx[sfQuantity];
    STAmount const amount = ctx.tx[sfAmount];
    if (!isPositiveBidAmount(quantity) || !isPositiveBidAmount(amount))
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: bad quantity " << quantity.getFullText()
                            << " or amount " << amount.getFullText();
        return temBAD_AMOUNT;
    }

    if (auto const expiration = ctx.tx[~sfExpiration]; expiration && *expiration == 0)
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: zero Expiration.";
        return temBAD_EXPIRATION;
    }

    return tesSUCCESS;
}

TER
DirectedOfferCounterCreate::preclaim(PreclaimContext const& ctx)
{
    DirectedOfferEntryR const offer(ctx.tx[sfDirectedOfferID], ctx.view);
    if (!offer)
    {
        JLOG(ctx.j.debug()) << "DirectedOffer does not exist.";
        return tecNO_ENTRY;
    }

    AccountID const buyer = ctx.tx[sfAccount];
    AccountID const seller = offer->at(sfAccount);
    if (buyer == seller)
    {
        JLOG(ctx.j.debug()) << "Seller cannot bid on its own offer.";
        return tecNO_PERMISSION;
    }

    if (hasExpired(ctx.view, offer->at(~sfExpiration)) ||
        hasExpired(ctx.view, ctx.tx[~sfExpiration]))
    {
        JLOG(ctx.j.debug()) << "DirectedOffer or bid has expired.";
        return tecEXPIRED;
    }

    if (!isDirectedOfferBuyer(ctx.view, *offer.rawSle(), buyer))
    {
        JLOG(ctx.j.debug()) << "Account is not a buyer this offer names.";
        return tecNO_PERMISSION;
    }

    STAmount const quantity = ctx.tx[sfQuantity];
    if (auto const ter = checkDirectedOfferQuantity(*offer.rawSle(), quantity, ctx.j);
        !isTesSuccess(ter))
        return ter;

    STAmount const amount = ctx.tx[sfAmount];
    if (amount.asset() != offer->at(sfPaymentAsset))
    {
        JLOG(ctx.j.debug()) << "Amount is not in the payment asset.";
        return tecWRONG_ASSET;
    }

    // The payment is locked under the same issuer controls as a token escrow
    // to the seller.
    if (!isXRP(amount))
    {
        if (auto const ter = std::visit(
                [&]<class T>(T const&) {
                    return escrowLockPreclaimHelper<T>(ctx.view, buyer, seller, amount, ctx.j);
                },
                amount.asset().value());
            !isTesSuccess(ter))
            return ter;
    }

    // Taking the bid cannot create the buyer's holding, since the seller
    // submits it; the buyer must be able to receive the sold asset now.
    return checkDirectedOfferDelivery(ctx.view, seller, buyer, quantity);
}

TER
DirectedOfferCounterCreate::doApply()
{
    auto const sle = view().peek(keylet::account(accountID_));
    if (!sle)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::DirectedOfferCounterCreate::doApply : the submitting account exists");
        return tefINTERNAL;
        // LCOV_EXCL_STOP
    }

    DirectedOfferEntryR const offer(ctx_.tx[sfDirectedOfferID], view());
    if (!offer)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::DirectedOfferCounterCreate::doApply : preclaim found the offer");
        return tecINTERNAL;
        // LCOV_EXCL_STOP
    }

    AccountID const seller = offer->at(sfAccount);
    STAmount const amount = ctx_.tx[sfAmount];
    auto const balance = sle->at(sfBalance).value().xrp();

    // Whoever pays for the new owner increment, the buyer or a reserve
    // sponsor, must be able to cover it.
    if (auto const ter =
            checkReserve(ctx_.getApplyViewContext(), sle, balance, {.ownerCountDelta = 1}, j_);
        !isTesSuccess(ter))
        return ter;

    // Locking XRP must leave the buyer above its own reserve, which a sponsor
    // never covers.
    if (isXRP(amount))
    {
        auto const buyerReserve = accountReserve(
            view(), sle, j_, {.ownerCountDelta = getTxReserveSponsorID(ctx_.tx) ? 0 : 1});
        if (balance - amount.xrp() < buyerReserve)
        {
            JLOG(j_.debug()) << "Locking the XRP would leave the buyer below its reserve.";
            return tecUNFUNDED;
        }
    }

    DirectedOfferCounterEntryW counter(accountID_, ctx_.tx.getSeqProxy(), view(), j_);
    Keylet const counterKeylet = counter.keylet();
    counter.newSLE();
    counter->at(sfAccount) = accountID_;
    counter->at(sfDestination) = seller;
    counter->at(sfSequence) = ctx_.tx.getSeqProxy().value();
    counter->at(sfDirectedOfferID) = ctx_.tx[sfDirectedOfferID];
    counter->at(sfQuantity) = ctx_.tx[sfQuantity];
    counter->at(sfAmount) = amount;
    counter->at(~sfExpiration) = ctx_.tx[~sfExpiration];

    if (!isXRP(amount))
    {
        auto const xferRate = transferRate(view(), amount);
        if (xferRate != kParityRate)
            counter->at(sfTransferRate) = xferRate.value;
    }

    counter.insert();

    {
        auto const page = view().dirInsert(
            keylet::ownerDir(accountID_), counterKeylet, describeOwnerDir(accountID_));
        if (!page)
            return tecDIR_FULL;  // LCOV_EXCL_LINE
        counter->at(sfOwnerNode) = *page;
    }

    // Linked from the seller's directory too, as an Escrow is from its
    // destination's, so the seller finds its bids.
    {
        auto const page =
            view().dirInsert(keylet::ownerDir(seller), counterKeylet, describeOwnerDir(seller));
        if (!page)
            return tecDIR_FULL;  // LCOV_EXCL_LINE
        counter->at(sfDestinationNode) = *page;
    }

    AccountID const issuer = amount.getIssuer();
    if (!isXRP(amount) && amount.holds<Issue>() && issuer != seller)
    {
        auto const page =
            view().dirInsert(keylet::ownerDir(issuer), counterKeylet, describeOwnerDir(issuer));
        if (!page)
            return tecDIR_FULL;  // LCOV_EXCL_LINE
        counter->at(sfIssuerNode) = *page;
    }

    if (isXRP(amount))
    {
        sle->at(sfBalance) -= amount;
    }
    else if (
        auto const ter = std::visit(
            [&]<class T>(T const&) {
                return escrowLockApplyHelper<T>(view(), issuer, accountID_, amount, j_);
            },
            amount.asset().value());
        !isTesSuccess(ter))
    {
        return ter;  // LCOV_EXCL_LINE
    }

    increaseOwnerCount(ctx_.getApplyViewContext(), sle, 1, j_);
    addSponsorToLedgerEntry(ctx_.getApplyViewContext(), counter.mutableRawSle());
    view().update(sle);
    return tesSUCCESS;
}

void
DirectedOfferCounterCreate::visitInvariantEntry(bool, SLE::ConstRef, SLE::ConstRef)
{
    // No transaction-specific invariants yet (future work).
}

bool
DirectedOfferCounterCreate::finalizeInvariants(
    STTx const&,
    TER,
    XRPAmount,
    ReadView const&,
    beast::Journal const&)
{
    // No transaction-specific invariants yet (future work).
    return true;
}

}  // namespace xrpl
