#include <xrpl/tx/transactors/dex/DirectedOfferAccept.h>

#include <xrpl/basics/Log.h>
#include <xrpl/beast/utility/Zero.h>
#include <xrpl/beast/utility/instrumentation.h>
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
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: zero DirectedOfferID.";
        return temMALFORMED;
    }

    STAmount const quantity = ctx.tx[sfQuantity];
    if (quantity <= beast::kZero ||
        (quantity.holds<MPTIssue>() && quantity.mpt() > MPTAmount{kMaxMpTokenAmount}))
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: bad quantity " << quantity.getFullText();
        return temBAD_AMOUNT;
    }

    if (auto const sendMax = ctx.tx[~sfSendMax]; sendMax && *sendMax <= beast::kZero)
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: SendMax is not positive.";
        return temBAD_AMOUNT;
    }

    return tesSUCCESS;
}

TER
DirectedOfferAccept::preclaim(PreclaimContext const& ctx)
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
        JLOG(ctx.j.debug()) << "Seller cannot accept its own offer.";
        return tecNO_PERMISSION;
    }

    if (hasExpired(ctx.view, offer->at(~sfExpiration)))
    {
        JLOG(ctx.j.debug()) << "DirectedOffer has expired.";
        return tecEXPIRED;
    }

    if (!isDirectedOfferBuyer(ctx.view, *offer.rawSle(), buyer))
    {
        JLOG(ctx.j.debug()) << "Account is not a buyer this offer names.";
        return tecNO_PERMISSION;
    }

    STAmount const available = offer->at(sfAmount);
    STAmount const quantity = ctx.tx[sfQuantity];
    if (quantity.asset() != available.asset())
    {
        JLOG(ctx.j.debug()) << "Quantity is not in the sold asset.";
        return tecWRONG_ASSET;
    }

    if (quantity > available)
    {
        JLOG(ctx.j.debug()) << "Quantity " << quantity.getFullText() << " is more than the "
                            << available.getFullText() << " left.";
        return tecINSUFFICIENT_FUNDS;
    }

    // An IOU remainder is rounded to 16 significant digits; refuse a quantity
    // the offer cannot give up exactly, so no value is made or lost.
    if (available - (available - quantity) != quantity)
    {
        JLOG(ctx.j.debug()) << "Quantity cannot be taken from the remainder exactly.";
        return tecPRECISION_LOSS;
    }

    Asset const paymentAsset = offer->at(sfPaymentAsset);
    auto const price =
        directedOfferPrice(offer->getFieldArray(sfPriceTiers), quantity, paymentAsset);
    if (!price)
    {
        JLOG(ctx.j.debug()) << "Quantity is below the smallest price tier.";
        return tecINSUFFICIENT_PAYMENT;
    }

    STAmount const cost = buyerCost(ctx.view, *price, buyer, seller);
    if (auto const sendMax = ctx.tx[~sfSendMax])
    {
        if (sendMax->asset() != paymentAsset)
        {
            JLOG(ctx.j.debug()) << "SendMax is not in the payment asset.";
            return tecWRONG_ASSET;
        }
        if (cost > *sendMax)
        {
            JLOG(ctx.j.debug()) << "Cost " << cost.getFullText() << " is more than SendMax.";
            return tecPATH_PARTIAL;
        }
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
            {
                JLOG(ctx.j.debug())
                    << "Buyer holds " << funds.getFullText() << ", needs " << cost.getFullText();
                return tecUNFUNDED_PAYMENT;
            }
        }

        if (seller != paymentAsset.getIssuer())
        {
            if (paymentAsset.holds<Issue>())
            {
                auto const& issue = paymentAsset.get<Issue>();
                if (!ctx.view.exists(keylet::trustLine(seller, issue.account, issue.currency)))
                {
                    JLOG(ctx.j.debug()) << "Seller no longer has a trust line for the payment.";
                    return tecNO_LINE;
                }
            }
            else if (!ctx.view.exists(
                         keylet::mptoken(paymentAsset.get<MPTIssue>().getMptID(), seller)))
            {
                JLOG(ctx.j.debug()) << "Seller no longer holds the payment MPT.";
                return tecOBJECT_NOT_FOUND;
            }

            if (auto const ter = requireAuth(ctx.view, paymentAsset, seller); !isTesSuccess(ter))
                return ter;
        }

        if (auto const ter = checkFrozen(ctx.view, buyer, paymentAsset); !isTesSuccess(ter))
            return ter;
        if (auto const ter = checkFrozen(ctx.view, seller, paymentAsset); !isTesSuccess(ter))
            return ter;
        if (auto const ter = canTransfer(ctx.view, paymentAsset, buyer, seller); !isTesSuccess(ter))
            return ter;
    }

    // Delivery leg: the seller is not frozen or locked in the sold asset, which
    // stays cancellable and so stays under the issuer's controls, and the
    // buyer may receive it.
    if (!isXRP(quantity))
    {
        if (auto const ter = checkFrozen(ctx.view, seller, quantity.asset()); !isTesSuccess(ter))
            return ter;

        if (auto const ter = std::visit(
                [&]<class T>(T const&) {
                    return escrowUnlockPreclaimHelper<T>(ctx.view, buyer, quantity);
                },
                quantity.asset().value());
            !isTesSuccess(ter))
            return ter;

        if (auto const ter = canTransfer(ctx.view, quantity.asset(), seller, buyer);
            !isTesSuccess(ter))
            return ter;
    }

    return tesSUCCESS;
}

TER
DirectedOfferAccept::doApply()
{
    DirectedOfferEntryW offer(ctx_.tx[sfDirectedOfferID], view(), j_);
    if (!offer)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::DirectedOfferAccept::doApply : preclaim found the offer");
        return tecINTERNAL;
        // LCOV_EXCL_STOP
    }

    AccountID const seller = offer->at(sfAccount);
    STAmount const quantity = ctx_.tx[sfQuantity];
    Asset const paymentAsset = offer->at(sfPaymentAsset);
    auto const price =
        directedOfferPrice(offer->getFieldArray(sfPriceTiers), quantity, paymentAsset);
    if (!price)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::DirectedOfferAccept::doApply : preclaim found a price tier");
        return tecINTERNAL;
        // LCOV_EXCL_STOP
    }

    // An XRP payment must leave the buyer above its reserve.
    if (isXRP(*price))
    {
        auto const sleBuyer = view().read(keylet::account(accountID_));
        if (!sleBuyer)
        {
            // LCOV_EXCL_START
            UNREACHABLE("xrpl::DirectedOfferAccept::doApply : the submitting account exists");
            return tefINTERNAL;
            // LCOV_EXCL_STOP
        }
        if (sleBuyer->at(sfBalance).value().xrp() - price->xrp() <
            accountReserve(view(), sleBuyer, j_, {}))
        {
            JLOG(j_.debug()) << "Paying " << price->getFullText()
                             << " would leave the buyer below its reserve.";
            return tecUNFUNDED_PAYMENT;
        }
    }

    // Payment leg: buyer to seller, the buyer bearing any transfer fee.
    if (auto const ter = accountSend(view(), accountID_, seller, *price, j_); !isTesSuccess(ter))
        return ter;

    // Delivery leg: release the bought quantity from the lock to the buyer.
    auto const sleBuyer = view().peek(keylet::account(accountID_));
    if (!sleBuyer)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::DirectedOfferAccept::doApply : the buyer still exists");
        return tefINTERNAL;
        // LCOV_EXCL_STOP
    }

    if (isXRP(quantity))
    {
        sleBuyer->at(sfBalance) += quantity;
    }
    else
    {
        auto const storedRate = offer->at(~sfTransferRate);
        Rate const lockedRate = storedRate ? Rate{*storedRate} : kParityRate;
        AccountID const issuer = quantity.getIssuer();
        // A holding created for the buyer is reserved against what is left
        // after an XRP payment.
        XRPAmount const buyerXrp = isXRP(*price) ? preFeeBalance_ - price->xrp() : preFeeBalance_;
        if (auto const ter = std::visit(
                [&]<class T>(T const&) {
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
            !isTesSuccess(ter))
            return ter;
    }
    view().update(sleBuyer);

    // Keep the offer open while someone can still buy the smallest tier;
    // otherwise return the remainder and close it.
    STAmount const remaining = offer->at(sfAmount) - quantity;
    offer->at(sfAmount) = remaining;
    offer.update();

    auto const& tiers = offer->getFieldArray(sfPriceTiers);
    if (remaining < tiers.begin()->at(sfMinQuantity))
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
