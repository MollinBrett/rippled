#include <xrpl/tx/transactors/dex/DirectedOfferAccept.h>

#include <xrpl/basics/Log.h>
#include <xrpl/beast/utility/Zero.h>
#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/ReadView.h>
#include <xrpl/ledger/View.h>
#include <xrpl/ledger/entries/DirectedOfferEntry.h>
#include <xrpl/ledger/helpers/AccountRootHelpers.h>
#include <xrpl/ledger/helpers/DirectedOfferHelpers.h>
#include <xrpl/ledger/helpers/EscrowHelpers.h>
#include <xrpl/ledger/helpers/TokenHelpers.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Asset.h>
#include <xrpl/protocol/Indexes.h>
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

namespace {

// What the buyer parts with to pay @p price: the price plus the issuer's
// transfer fee, which is waived when the buyer or the seller is the issuer.
STAmount
buyerCost(
    ReadView const& view,
    STAmount const& price,
    AccountID const& buyer,
    AccountID const& seller)
{
    if (isXRP(price))
        return price;

    AccountID const issuer = price.getIssuer();
    if (buyer == issuer || seller == issuer)
        return price;

    return addTransferFee(price, transferRate(view, price));
}

}  // namespace

bool
DirectedOfferAccept::checkExtraFeatures(PreflightContext const& ctx)
{
    return directedOfferFeaturesEnabled(ctx.rules);
}

NotTEC
DirectedOfferAccept::preflight(PreflightContext const& ctx)
{
    if (ctx.tx[sfDirectedOfferID] == beast::kZero)
        return temMALFORMED;

    STAmount const quantity = ctx.tx[sfQuantity];
    if (quantity <= beast::kZero)
        return temBAD_AMOUNT;

    if (quantity.holds<MPTIssue>() && quantity.mpt() > MPTAmount{kMaxMpTokenAmount})
        return temBAD_AMOUNT;

    if (auto const sendMax = ctx.tx[~sfSendMax]; sendMax && *sendMax <= beast::kZero)
        return temBAD_AMOUNT;

    return tesSUCCESS;
}

TER
DirectedOfferAccept::preclaim(PreclaimContext const& ctx)
{
    DirectedOfferEntryR const offer(ctx.tx[sfDirectedOfferID], ctx.view);
    if (!offer)
        return tecNO_ENTRY;

    AccountID const buyer = ctx.tx[sfAccount];
    AccountID const seller = (*offer)[sfAccount];
    if (buyer == seller)
        return tecNO_PERMISSION;

    if (hasExpired(ctx.view, (*offer)[~sfExpiration]))
        return tecEXPIRED;

    if (!isDirectedOfferBuyer(ctx.view, *offer.rawSle(), buyer))
        return tecNO_PERMISSION;

    STAmount const available = (*offer)[sfAmount];
    STAmount const quantity = ctx.tx[sfQuantity];
    if (quantity.asset() != available.asset())
        return tecWRONG_ASSET;

    if (quantity > available)
        return tecINSUFFICIENT_FUNDS;

    // An IOU remainder is rounded to 16 significant digits; refuse a quantity
    // the offer cannot give up exactly, so no value is made or lost.
    if (available - (available - quantity) != quantity)
        return tecPRECISION_LOSS;

    Asset const paymentAsset = (*offer)[sfPaymentAsset];
    auto const price =
        directedOfferPrice(offer->getFieldArray(sfPriceTiers), quantity, paymentAsset);
    if (!price)
        return tecINSUFFICIENT_PAYMENT;

    STAmount const cost = buyerCost(ctx.view, *price, buyer, seller);
    if (auto const sendMax = ctx.tx[~sfSendMax])
    {
        if (sendMax->asset() != paymentAsset)
            return tecWRONG_ASSET;
        if (cost > *sendMax)
            return tecPATH_PARTIAL;
    }

    // Payment leg: the buyer can pay, the seller can still receive, and
    // nothing on either side is frozen or locked.
    if (!isXRP(paymentAsset))
    {
        if (buyer != paymentAsset.getIssuer())
        {
            STAmount const funds = accountHolds(
                ctx.view,
                buyer,
                paymentAsset,
                FreezeHandling::ZeroIfFrozen,
                AuthHandling::ZeroIfUnauthorized,
                ctx.j);
            if (funds < cost)
                return tecUNFUNDED_PAYMENT;
        }

        if (seller != paymentAsset.getIssuer())
        {
            if (paymentAsset.holds<Issue>())
            {
                auto const& issue = paymentAsset.get<Issue>();
                if (!ctx.view.exists(keylet::trustLine(seller, issue.account, issue.currency)))
                    return tecNO_LINE;
            }
            else if (!ctx.view.exists(
                         keylet::mptoken(paymentAsset.get<MPTIssue>().getMptID(), seller)))
            {
                return tecOBJECT_NOT_FOUND;
            }

            if (auto const ret = requireAuth(ctx.view, paymentAsset, seller); !isTesSuccess(ret))
                return ret;
        }

        if (auto const ret = checkFrozen(ctx.view, buyer, paymentAsset); !isTesSuccess(ret))
            return ret;
        if (auto const ret = checkFrozen(ctx.view, seller, paymentAsset); !isTesSuccess(ret))
            return ret;
        if (auto const ret = canTransfer(ctx.view, paymentAsset, buyer, seller); !isTesSuccess(ret))
            return ret;
    }

    // Delivery leg: the seller is not frozen or locked in the sold asset, which
    // stays cancellable and so stays under the issuer's controls, and the
    // buyer may receive it.
    if (!isXRP(quantity))
    {
        if (auto const ret = checkFrozen(ctx.view, seller, quantity.asset()); !isTesSuccess(ret))
            return ret;

        if (auto const ret = std::visit(
                [&]<typename T>(T const&) {
                    return escrowUnlockPreclaimHelper<T>(ctx.view, buyer, quantity);
                },
                quantity.asset().value());
            !isTesSuccess(ret))
            return ret;

        if (auto const ret = canTransfer(ctx.view, quantity.asset(), seller, buyer);
            !isTesSuccess(ret))
            return ret;
    }

    return tesSUCCESS;
}

TER
DirectedOfferAccept::doApply()
{
    DirectedOfferEntryW offer(ctx_.tx[sfDirectedOfferID], view(), j_);
    if (!offer)
        return tecNO_ENTRY;  // LCOV_EXCL_LINE

    AccountID const seller = (*offer)[sfAccount];
    STAmount const quantity = ctx_.tx[sfQuantity];
    Asset const paymentAsset = (*offer)[sfPaymentAsset];
    auto const price =
        directedOfferPrice(offer->getFieldArray(sfPriceTiers), quantity, paymentAsset);
    if (!price)
        return tecINTERNAL;  // LCOV_EXCL_LINE

    // An XRP payment must leave the buyer above its reserve.
    if (isXRP(*price))
    {
        auto const sleBuyer = view().read(keylet::account(accountID_));
        if (!sleBuyer)
            return tefINTERNAL;  // LCOV_EXCL_LINE
        if ((*sleBuyer)[sfBalance].xrp() - price->xrp() < accountReserve(view(), sleBuyer, j_, {}))
            return tecUNFUNDED_PAYMENT;
    }

    // Payment leg: buyer to seller, the buyer bearing any transfer fee.
    if (auto const ret = accountSend(view(), accountID_, seller, *price, j_); !isTesSuccess(ret))
        return ret;

    // Delivery leg: release the bought quantity from the lock to the buyer.
    auto const sleBuyer = view().peek(keylet::account(accountID_));
    if (!sleBuyer)
        return tefINTERNAL;  // LCOV_EXCL_LINE

    if (isXRP(quantity))
    {
        (*sleBuyer)[sfBalance] = (*sleBuyer)[sfBalance] + quantity;
        view().update(sleBuyer);
    }
    else
    {
        Rate const lockedRate =
            offer->isFieldPresent(sfTransferRate) ? Rate((*offer)[sfTransferRate]) : kParityRate;
        AccountID const issuer = quantity.getIssuer();
        // A holding created for the buyer is reserved against what is left
        // after an XRP payment.
        XRPAmount const buyerXrp = isXRP(*price) ? preFeeBalance_ - price->xrp() : preFeeBalance_;
        if (auto const ret = std::visit(
                [&]<typename T>(T const&) {
                    return escrowUnlockApplyHelper<T>(
                        ctx_.getApplyViewContext(),
                        lockedRate,
                        sleBuyer,
                        buyerXrp,
                        quantity,
                        issuer,
                        seller,
                        accountID_,
                        true,
                        j_);
                },
                quantity.asset().value());
            !isTesSuccess(ret))
            return ret;
        view().update(sleBuyer);
    }

    // Keep the offer open while someone can still buy the smallest tier;
    // otherwise return the remainder and close it.
    STAmount const remaining = (*offer)[sfAmount] - quantity;
    (*offer)[sfAmount] = remaining;
    offer.update();

    auto const& tiers = offer->getFieldArray(sfPriceTiers);
    if (remaining < (*tiers.begin())[sfMinQuantity])
    {
        return closeDirectedOffer(
            ctx_.getApplyViewContext(), offer.mutableRawSle(), preFeeBalance_, false, j_);
    }

    return tesSUCCESS;
}

void
DirectedOfferAccept::visitInvariantEntry(bool, SLE::ConstRef, SLE::ConstRef)
{
    // No transaction-specific invariants yet (future work).
}

bool
DirectedOfferAccept::finalizeInvariants(
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
