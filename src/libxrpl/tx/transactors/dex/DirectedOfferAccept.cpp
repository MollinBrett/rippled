#include <xrpl/tx/transactors/dex/DirectedOfferAccept.h>

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

// The seller takes a buyer's bid: both legs are already locked, so only who
// may take it and whether each side can still receive are checked.
TER
preclaimTakeCounter(PreclaimContext const& ctx, UInt256 const& counterID)
{
    DirectedOfferCounterEntryR const counter(counterID, ctx.view);
    if (!counter)
    {
        JLOG(ctx.j.debug()) << "DirectedOfferCounter does not exist.";
        return tecNO_ENTRY;
    }

    DirectedOfferEntryR const offer(counter->at(sfDirectedOfferID), ctx.view);
    if (!offer)
    {
        JLOG(ctx.j.debug()) << "The bid's DirectedOffer no longer exists.";
        return tecNO_ENTRY;
    }

    AccountID const seller = offer->at(sfAccount);
    if (ctx.tx[sfAccount] != seller)
    {
        JLOG(ctx.j.debug()) << "Only the seller may take a bid.";
        return tecNO_PERMISSION;
    }

    if (hasExpired(ctx.view, offer->at(~sfExpiration)) ||
        hasExpired(ctx.view, counter->at(~sfExpiration)))
    {
        JLOG(ctx.j.debug()) << "DirectedOffer or bid has expired.";
        return tecEXPIRED;
    }

    // Domain membership can lapse after the bid was made.
    AccountID const buyer = counter->at(sfAccount);
    if (!isDirectedOfferBuyer(ctx.view, *offer.rawSle(), buyer))
    {
        JLOG(ctx.j.debug()) << "Bidder is no longer a buyer this offer names.";
        return tecNO_PERMISSION;
    }

    STAmount const quantity = counter->at(sfQuantity);
    if (auto const ter = checkDirectedOfferQuantity(*offer.rawSle(), quantity, ctx.j);
        !isTesSuccess(ter))
        return ter;

    if (auto const ter = checkDirectedOfferDelivery(ctx.view, seller, buyer, quantity);
        !isTesSuccess(ter))
        return ter;

    // Payment leg: the locked payment can be released to the seller.
    STAmount const amount = counter->at(sfAmount);
    if (isXRP(amount))
        return tesSUCCESS;

    if (auto const ter = std::visit(
            [&]<class T>(T const&) {
                return escrowUnlockPreclaimHelper<T>(ctx.view, seller, amount);
            },
            amount.asset().value());
        !isTesSuccess(ter))
        return ter;

    return canTransfer(ctx.view, amount.asset(), buyer, seller);
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
    // Exactly one of a purchase (DirectedOfferID with Quantity) and the
    // seller taking a bid (DirectedOfferCounterID alone), as NFTokenAcceptOffer
    // takes a sell offer or a buy offer.
    auto const offerID = ctx.tx[~sfDirectedOfferID];
    auto const counterID = ctx.tx[~sfDirectedOfferCounterID];
    if (static_cast<bool>(offerID) == static_cast<bool>(counterID))
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: needs exactly one of DirectedOfferID "
                               "and DirectedOfferCounterID.";
        return temMALFORMED;
    }

    if (counterID)
    {
        if (*counterID == beast::kZero || ctx.tx.isFieldPresent(sfQuantity) ||
            ctx.tx.isFieldPresent(sfSendMax))
        {
            JLOG(ctx.j.debug()) << "Malformed transaction: a bid is taken by its ID alone.";
            return temMALFORMED;
        }
        return tesSUCCESS;
    }

    auto const quantity = ctx.tx[~sfQuantity];
    if (*offerID == beast::kZero || !quantity)
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: a purchase needs DirectedOfferID and "
                               "Quantity.";
        return temMALFORMED;
    }

    if (*quantity <= beast::kZero ||
        (quantity->holds<MPTIssue>() && quantity->mpt() > MPTAmount{kMaxMpTokenAmount}))
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: bad quantity " << quantity->getFullText();
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
    if (auto const counterID = ctx.tx[~sfDirectedOfferCounterID])
        return preclaimTakeCounter(ctx, *counterID);

    DirectedOfferEntryR const offer(*ctx.tx[~sfDirectedOfferID], ctx.view);
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

    STAmount const quantity = *ctx.tx[~sfQuantity];
    if (auto const ter = checkDirectedOfferQuantity(*offer.rawSle(), quantity, ctx.j);
        !isTesSuccess(ter))
        return ter;

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

    return checkDirectedOfferDelivery(ctx.view, seller, buyer, quantity);
}

TER
DirectedOfferAccept::doApply()
{
    if (auto const counterID = ctx_.tx[~sfDirectedOfferCounterID])
        return takeCounter(*counterID);

    DirectedOfferEntryW offer(*ctx_.tx[~sfDirectedOfferID], view(), j_);
    if (!offer)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::DirectedOfferAccept::doApply : preclaim found the offer");
        return tecINTERNAL;
        // LCOV_EXCL_STOP
    }

    AccountID const seller = offer->at(sfAccount);
    STAmount const quantity = *ctx_.tx[~sfQuantity];
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

    // A holding created for the buyer is reserved against what is left after
    // an XRP payment.
    XRPAmount const buyerXrp = isXRP(*price) ? preFeeBalance_ - price->xrp() : preFeeBalance_;
    auto const ter = deliverDirectedOffer(
        ctx_.getApplyViewContext(),
        offer.mutableRawSle(),
        sleBuyer,
        buyerXrp,
        quantity,
        true,
        preFeeBalance_,
        false,
        j_);
    view().update(sleBuyer);
    return ter;
}

TER
DirectedOfferAccept::takeCounter(UInt256 const& counterID)
{
    DirectedOfferCounterEntryW counter(counterID, view(), j_);
    if (!counter)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::DirectedOfferAccept::takeCounter : preclaim found the bid");
        return tecINTERNAL;
        // LCOV_EXCL_STOP
    }

    DirectedOfferEntryW offer(counter->at(sfDirectedOfferID), view(), j_);
    if (!offer)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::DirectedOfferAccept::takeCounter : preclaim found the offer");
        return tecINTERNAL;
        // LCOV_EXCL_STOP
    }

    AccountID const buyer = counter->at(sfAccount);
    STAmount const quantity = counter->at(sfQuantity);

    // Payment leg: release the locked payment to the seller, who submits and
    // so may have its holding created.
    if (auto const ter = closeDirectedOfferCounter(
            ctx_.getApplyViewContext(),
            counter.mutableRawSle(),
            accountID_,
            preFeeBalance_,
            true,
            j_);
        !isTesSuccess(ter))
        return ter;

    // Delivery leg: release the quantity from the offer to the buyer, whose
    // holding must already exist.
    auto const sleBuyer = view().peek(keylet::account(buyer));
    if (!sleBuyer)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::DirectedOfferAccept::takeCounter : the buyer exists");
        return tefINTERNAL;
        // LCOV_EXCL_STOP
    }

    auto const ter = deliverDirectedOffer(
        ctx_.getApplyViewContext(),
        offer.mutableRawSle(),
        sleBuyer,
        sleBuyer->at(sfBalance).value().xrp(),
        quantity,
        false,
        preFeeBalance_,
        true,
        j_);
    view().update(sleBuyer);
    return ter;
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
