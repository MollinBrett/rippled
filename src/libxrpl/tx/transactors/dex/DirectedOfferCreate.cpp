#include <xrpl/tx/transactors/dex/DirectedOfferCreate.h>

#include <xrpl/basics/Log.h>
#include <xrpl/beast/utility/Zero.h>
#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/ReadView.h>
#include <xrpl/ledger/View.h>
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
#include <xrpl/protocol/STArray.h>
#include <xrpl/protocol/STIssue.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/UintTypes.h>
#include <xrpl/protocol/XRPAmount.h>
#include <xrpl/tx/Transactor.h>

#include <set>
#include <variant>

namespace xrpl {

/*
    DirectedOffer
    =============

    A seller locks an asset and offers it at tiered prices to a named set of
    buyers, or to the members of a permissioned domain. A named buyer accepts
    any quantity the tiers allow, and the payment and the delivery settle in
    the same transaction. See docs/directed-offer.md.
*/

//------------------------------------------------------------------------------

namespace {

// A positive amount that can be locked or paid: a real currency, and an MPT
// amount within the protocol maximum.
bool
isValidOfferAmount(STAmount const& amount)
{
    if (amount <= beast::kZero)
        return false;

    if (amount.holds<Issue>() && !amount.native() && amount.get<Issue>().currency == badCurrency())
        return false;

    if (amount.holds<MPTIssue>() && amount.mpt() > MPTAmount{kMaxMpTokenAmount})
        return false;

    return true;
}

}  // namespace

TxConsequences
DirectedOfferCreate::makeTxConsequences(PreflightContext const& ctx)
{
    auto const amount = ctx.tx[sfAmount];
    return TxConsequences{ctx.tx, isXRP(amount) ? amount.xrp() : beast::kZero};
}

bool
DirectedOfferCreate::checkExtraFeatures(PreflightContext const& ctx)
{
    return directedOfferFeaturesEnabled(ctx.rules);
}

NotTEC
DirectedOfferCreate::preflight(PreflightContext const& ctx)
{
    STAmount const amount = ctx.tx[sfAmount];
    if (!isValidOfferAmount(amount))
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: bad amount " << amount.getFullText();
        return temBAD_AMOUNT;
    }

    Asset const paymentAsset = ctx.tx[sfPaymentAsset];
    if (paymentAsset == amount.asset())
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: payment asset is the sold asset.";
        return temBAD_AMOUNT;
    }

    if (paymentAsset.holds<Issue>() && !isXRP(paymentAsset) &&
        paymentAsset.get<Issue>().currency == badCurrency())
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: bad payment currency.";
        return temBAD_CURRENCY;
    }

    auto const& tiers = ctx.tx.getFieldArray(sfPriceTiers);
    if (tiers.empty() || tiers.size() > kMaxDirectedOfferTiers)
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: " << tiers.size() << " price tiers.";
        return temMALFORMED;
    }

    std::optional<STAmount> previousMin;
    for (auto const& tier : tiers)
    {
        if (tier.getFName() != sfPriceTier)
        {
            JLOG(ctx.j.debug()) << "Malformed transaction: price tier is not a PriceTier.";
            return temMALFORMED;
        }

        STAmount const minQuantity = tier[sfMinQuantity];
        STAmount const unitPrice = tier[sfUnitPrice];

        if (minQuantity.asset() != amount.asset() || !isValidOfferAmount(minQuantity) ||
            minQuantity > amount)
        {
            JLOG(ctx.j.debug()) << "Malformed transaction: bad tier quantity "
                                << minQuantity.getFullText();
            return temBAD_AMOUNT;
        }

        if (unitPrice.asset() != paymentAsset || !isValidOfferAmount(unitPrice))
        {
            JLOG(ctx.j.debug()) << "Malformed transaction: bad tier price "
                                << unitPrice.getFullText();
            return temBAD_AMOUNT;
        }

        if (previousMin && minQuantity <= *previousMin)
        {
            JLOG(ctx.j.debug()) << "Malformed transaction: tier quantities not increasing.";
            return temMALFORMED;
        }
        previousMin = minQuantity;
    }

    auto const domainID = ctx.tx[~sfDomainID];
    bool const hasCounterparties = ctx.tx.isFieldPresent(sfCounterparties);
    if (hasCounterparties == static_cast<bool>(domainID))
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: needs exactly one of Counterparties "
                               "and DomainID.";
        return temMALFORMED;
    }

    if (hasCounterparties)
    {
        auto const& counterparties = ctx.tx.getFieldArray(sfCounterparties);
        if (counterparties.empty() || counterparties.size() > kMaxDirectedOfferCounterparties)
        {
            JLOG(ctx.j.debug()) << "Malformed transaction: " << counterparties.size()
                                << " counterparties.";
            return temMALFORMED;
        }

        std::set<AccountID> seen;
        for (auto const& counterparty : counterparties)
        {
            if (counterparty.getFName() != sfCounterpartyEntry)
            {
                JLOG(ctx.j.debug())
                    << "Malformed transaction: counterparty is not a CounterpartyEntry.";
                return temMALFORMED;
            }

            AccountID const account = counterparty[sfAccount];
            if (account == ctx.tx[sfAccount] || !seen.insert(account).second)
            {
                JLOG(ctx.j.debug())
                    << "Malformed transaction: counterparty is the seller or repeated.";
                return temMALFORMED;
            }
        }
    }

    if (domainID && *domainID == beast::kZero)
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: zero DomainID.";
        return temMALFORMED;
    }

    if (auto const expiration = ctx.tx[~sfExpiration]; expiration && *expiration == 0)
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: zero Expiration.";
        return temBAD_EXPIRATION;
    }

    return tesSUCCESS;
}

TER
DirectedOfferCreate::preclaim(PreclaimContext const& ctx)
{
    AccountID const account = ctx.tx[sfAccount];
    STAmount const amount = ctx.tx[sfAmount];
    Asset const paymentAsset = ctx.tx[sfPaymentAsset];

    if (hasExpired(ctx.view, ctx.tx[~sfExpiration]))
    {
        JLOG(ctx.j.debug()) << "Offer would already be expired.";
        return tecEXPIRED;
    }

    // The sold asset is locked under the same issuer controls as a token
    // escrow, with the seller standing in for the destination.
    if (!isXRP(amount))
    {
        if (auto const ter = std::visit(
                [&]<class T>(T const&) {
                    return escrowLockPreclaimHelper<T>(ctx.view, account, account, amount, ctx.j);
                },
                amount.asset().value());
            !isTesSuccess(ter))
            return ter;
    }

    // The seller must be able to receive the payment asset.
    if (!isXRP(paymentAsset) && paymentAsset.getIssuer() != account)
    {
        if (paymentAsset.holds<Issue>())
        {
            auto const& issue = paymentAsset.get<Issue>();
            if (!ctx.view.exists(keylet::trustLine(account, issue.account, issue.currency)))
            {
                JLOG(ctx.j.debug()) << "Seller has no trust line for the payment asset.";
                return tecNO_LINE;
            }
        }
        else
        {
            auto const& mptIssue = paymentAsset.get<MPTIssue>();
            if (!ctx.view.exists(keylet::mptokenIssuance(mptIssue.getMptID())) ||
                !ctx.view.exists(keylet::mptoken(mptIssue.getMptID(), account)))
            {
                JLOG(ctx.j.debug()) << "Seller cannot hold the payment MPT.";
                return tecOBJECT_NOT_FOUND;
            }
        }

        if (auto const ter = requireAuth(ctx.view, paymentAsset, account); !isTesSuccess(ter))
            return ter;

        if (auto const ter = checkFrozen(ctx.view, account, paymentAsset); !isTesSuccess(ter))
            return ter;
    }

    if (auto const domainID = ctx.tx[~sfDomainID];
        domainID && !ctx.view.exists(keylet::permissionedDomain(*domainID)))
    {
        JLOG(ctx.j.debug()) << "Permissioned domain does not exist.";
        return tecOBJECT_NOT_FOUND;
    }

    if (ctx.tx.isFieldPresent(sfCounterparties))
    {
        for (auto const& counterparty : ctx.tx.getFieldArray(sfCounterparties))
        {
            if (!ctx.view.exists(keylet::account(counterparty[sfAccount])))
            {
                JLOG(ctx.j.debug()) << "Counterparty account does not exist.";
                return tecNO_TARGET;
            }
        }
    }

    return tesSUCCESS;
}

TER
DirectedOfferCreate::doApply()
{
    auto const sle = view().peek(keylet::account(accountID_));
    if (!sle)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::DirectedOfferCreate::doApply : the submitting account exists");
        return tefINTERNAL;
        // LCOV_EXCL_STOP
    }

    STAmount const amount = ctx_.tx[sfAmount];
    auto const balance = sle->at(sfBalance).value().xrp();

    // Whoever pays for the new owner increment, the seller or a reserve
    // sponsor, must be able to cover it.
    if (auto const ter =
            checkReserve(ctx_.getApplyViewContext(), sle, balance, {.ownerCountDelta = 1}, j_);
        !isTesSuccess(ter))
        return ter;

    // Locking XRP must leave the seller above its own reserve, which a
    // sponsor never covers.
    if (isXRP(amount))
    {
        auto const sellerReserve = accountReserve(
            view(), sle, j_, {.ownerCountDelta = getTxReserveSponsorID(ctx_.tx) ? 0 : 1});
        if (balance - amount.xrp() < sellerReserve)
        {
            JLOG(j_.debug()) << "Locking the XRP would leave the seller below its reserve.";
            return tecUNFUNDED;
        }
    }

    DirectedOfferEntryW offer(accountID_, ctx_.tx.getSeqProxy(), view(), j_);
    Keylet const offerKeylet = offer.keylet();
    offer.newSLE();
    offer->at(sfAccount) = accountID_;
    offer->at(sfSequence) = ctx_.tx.getSeqProxy().value();
    offer->at(sfAmount) = amount;
    // An Issue field has no working proxy assignment, so it is set directly.
    offer->setFieldIssue(sfPaymentAsset, STIssue{sfPaymentAsset, ctx_.tx[sfPaymentAsset]});
    offer->setFieldArray(sfPriceTiers, ctx_.tx.getFieldArray(sfPriceTiers));
    if (ctx_.tx.isFieldPresent(sfCounterparties))
        offer->setFieldArray(sfCounterparties, ctx_.tx.getFieldArray(sfCounterparties));
    offer->at(~sfDomainID) = ctx_.tx[~sfDomainID];
    offer->at(~sfExpiration) = ctx_.tx[~sfExpiration];

    if (!isXRP(amount))
    {
        auto const xferRate = transferRate(view(), amount);
        if (xferRate != kParityRate)
            offer->at(sfTransferRate) = xferRate.value;
    }

    offer.insert();

    {
        auto const page = view().dirInsert(
            keylet::ownerDir(accountID_), offerKeylet, describeOwnerDir(accountID_));
        if (!page)
            return tecDIR_FULL;  // LCOV_EXCL_LINE
        offer->at(sfOwnerNode) = *page;
    }

    // As with token escrow, a locked IOU is linked from the issuer's directory
    // so the issuer can account for it. A locked MPT is tracked on the
    // issuance itself.
    AccountID const issuer = amount.getIssuer();
    if (!isXRP(amount) && amount.holds<Issue>())
    {
        auto const page =
            view().dirInsert(keylet::ownerDir(issuer), offerKeylet, describeOwnerDir(issuer));
        if (!page)
            return tecDIR_FULL;  // LCOV_EXCL_LINE
        offer->at(sfIssuerNode) = *page;
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
    addSponsorToLedgerEntry(ctx_.getApplyViewContext(), offer.mutableRawSle());
    view().update(sle);
    return tesSUCCESS;
}

void
DirectedOfferCreate::visitInvariantEntry(bool, SLE::ConstRef, SLE::ConstRef)
{
    // No transaction-specific invariants yet (future work).
}

bool
DirectedOfferCreate::finalizeInvariants(
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
