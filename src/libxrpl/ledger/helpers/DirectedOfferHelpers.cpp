#include <xrpl/ledger/helpers/DirectedOfferHelpers.h>

#include <xrpl/basics/Log.h>
#include <xrpl/beast/utility/Zero.h>
#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/ledger/helpers/AccountRootHelpers.h>
#include <xrpl/ledger/helpers/EscrowHelpers.h>
#include <xrpl/ledger/helpers/PermissionedDEXHelpers.h>
#include <xrpl/ledger/helpers/TokenHelpers.h>
#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/Rate.h>
#include <xrpl/protocol/SField.h>

#include <algorithm>
#include <variant>

namespace xrpl {

bool
directedOfferFeaturesEnabled(Rules const& rules)
{
    return rules.enabled(featureTokenEscrow) && rules.enabled(featureMPTokensV1) &&
        rules.enabled(fixTokenEscrowV1) && rules.enabled(fixCleanup3_4_0);
}

std::optional<STAmount>
directedOfferPrice(STArray const& tiers, STAmount const& quantity, Asset const& paymentAsset)
{
    STObject const* tier = nullptr;
    for (auto const& candidate : tiers)
    {
        if (candidate[sfMinQuantity] > quantity)
            break;
        tier = &candidate;
    }

    if (tier == nullptr)
        return std::nullopt;

    return mulRound(quantity, tier->at(sfUnitPrice), paymentAsset, true);
}

bool
isDirectedOfferBuyer(ReadView const& view, SLE const& offer, AccountID const& account)
{
    if (offer.isFieldPresent(sfCounterparties))
    {
        auto const& counterparties = offer.getFieldArray(sfCounterparties);
        return std::ranges::any_of(counterparties, [&account](STObject const& counterparty) {
            return counterparty[sfAccount] == account;
        });
    }

    if (auto const domainID = offer.at(~sfDomainID))
        return permissioned_dex::accountInDomain(view, account, *domainID);

    // LCOV_EXCL_START
    UNREACHABLE("xrpl::isDirectedOfferBuyer : preflight requires Counterparties or DomainID");
    return false;
    // LCOV_EXCL_STOP
}

TER
checkDirectedOfferQuantity(SLE const& offer, STAmount const& quantity, beast::Journal j)
{
    STAmount const available = offer.at(sfAmount);
    if (quantity.asset() != available.asset())
    {
        JLOG(j.debug()) << "Quantity is not in the sold asset.";
        return tecWRONG_ASSET;
    }

    if (quantity > available)
    {
        JLOG(j.debug()) << "Quantity " << quantity.getFullText() << " is more than the "
                        << available.getFullText() << " left.";
        return tecINSUFFICIENT_FUNDS;
    }

    // An IOU remainder is rounded to 16 significant digits; refuse a quantity
    // the offer cannot give up exactly, so no value is made or lost.
    if (available - (available - quantity) != quantity)
    {
        JLOG(j.debug()) << "Quantity cannot be taken from the remainder exactly.";
        return tecPRECISION_LOSS;
    }

    return tesSUCCESS;
}

TER
checkDirectedOfferDelivery(
    ReadView const& view,
    AccountID const& seller,
    AccountID const& buyer,
    STAmount const& quantity)
{
    // The seller is not frozen or locked in the sold asset, which stays
    // cancellable and so stays under the issuer's controls, and the buyer may
    // receive it.
    if (isXRP(quantity))
        return tesSUCCESS;

    if (auto const ter = checkFrozen(view, seller, quantity.asset()); !isTesSuccess(ter))
        return ter;

    if (auto const ter = std::visit(
            [&]<class T>(T const&) { return escrowUnlockPreclaimHelper<T>(view, buyer, quantity); },
            quantity.asset().value());
        !isTesSuccess(ter))
        return ter;

    return canTransfer(view, quantity.asset(), seller, buyer);
}

TER
deliverDirectedOffer(
    ApplyViewContext ctx,
    SLE::Ref offer,
    SLE::Ref sleBuyer,
    XRPAmount buyerXrp,
    STAmount const& quantity,
    bool createBuyerAsset,
    XRPAmount preFeeBalance,
    bool createSellerAsset,
    beast::Journal j)
{
    AccountID const seller = offer->at(sfAccount);
    AccountID const buyer = sleBuyer->at(sfAccount);

    if (isXRP(quantity))
    {
        sleBuyer->at(sfBalance) += quantity;
    }
    else
    {
        auto const storedRate = offer->at(~sfTransferRate);
        Rate const lockedRate = storedRate ? Rate{*storedRate} : kParityRate;
        AccountID const issuer = quantity.getIssuer();
        if (auto const ter = std::visit(
                [&]<class T>(T const&) {
                    return escrowUnlockApplyHelper<T>(
                        ctx,
                        lockedRate,
                        sleBuyer,
                        buyerXrp,
                        quantity,
                        issuer,
                        seller,
                        buyer,
                        createBuyerAsset,
                        j);
                },
                quantity.asset().value());
            !isTesSuccess(ter))
            return ter;
    }

    // Keep the offer open while someone can still buy the smallest tier;
    // otherwise return the remainder and close it.
    STAmount const remaining = offer->at(sfAmount) - quantity;
    offer->at(sfAmount) = remaining;
    ctx.view.update(offer);

    auto const& tiers = offer->getFieldArray(sfPriceTiers);
    if (remaining < tiers.begin()->at(sfMinQuantity))
        return closeDirectedOffer(ctx, offer, preFeeBalance, createSellerAsset, j);

    return tesSUCCESS;
}

TER
closeDirectedOffer(
    ApplyViewContext ctx,
    SLE::Ref offer,
    XRPAmount preFeeBalance,
    bool createAsset,
    beast::Journal j)
{
    AccountID const owner = offer->at(sfAccount);
    auto const sleOwner = ctx.view.peek(keylet::account(owner));
    if (!sleOwner)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::closeDirectedOffer : the offer's owner exists");
        return tefINTERNAL;
        // LCOV_EXCL_STOP
    }

    if (!ctx.view.dirRemove(keylet::ownerDir(owner), offer->at(sfOwnerNode), offer->key(), true))
    {
        // LCOV_EXCL_START
        JLOG(j.fatal()) << "Unable to remove DirectedOffer from owner directory.";
        return tefBAD_LEDGER;
        // LCOV_EXCL_STOP
    }

    STAmount const amount = offer->at(sfAmount);
    AccountID const issuer = amount.getIssuer();

    if (auto const page = offer->at(~sfIssuerNode))
    {
        if (!ctx.view.dirRemove(keylet::ownerDir(issuer), *page, offer->key(), true))
        {
            // LCOV_EXCL_START
            JLOG(j.fatal()) << "Unable to remove DirectedOffer from issuer directory.";
            return tefBAD_LEDGER;
            // LCOV_EXCL_STOP
        }
    }

    // Release the reserve first: returning the remainder can re-create a
    // holding the owner deleted, and the offer must not count against it.
    decreaseOwnerCountForObject(ctx.view, sleOwner, offer, 1, j);

    if (amount > beast::kZero)
    {
        if (isXRP(amount))
        {
            sleOwner->at(sfBalance) += amount;
        }
        else if (
            auto const ter = std::visit(
                [&]<class T>(T const&) {
                    return escrowUnlockApplyHelper<T>(
                        ctx,
                        kParityRate,
                        sleOwner,
                        preFeeBalance,
                        amount,
                        issuer,
                        owner,
                        owner,
                        createAsset,
                        j);
                },
                amount.asset().value());
            !isTesSuccess(ter))
        {
            return ter;
        }
    }

    ctx.view.update(sleOwner);
    ctx.view.erase(offer);
    return tesSUCCESS;
}

TER
closeDirectedOfferCounter(
    ApplyViewContext ctx,
    SLE::Ref counter,
    AccountID const& receiver,
    XRPAmount preFeeBalance,
    bool createAsset,
    beast::Journal j)
{
    AccountID const owner = counter->at(sfAccount);
    AccountID const destination = counter->at(sfDestination);
    auto const sleOwner = ctx.view.peek(keylet::account(owner));
    if (!sleOwner)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::closeDirectedOfferCounter : the bid's owner exists");
        return tefINTERNAL;
        // LCOV_EXCL_STOP
    }

    if (!ctx.view.dirRemove(
            keylet::ownerDir(owner), counter->at(sfOwnerNode), counter->key(), true))
    {
        // LCOV_EXCL_START
        JLOG(j.fatal()) << "Unable to remove DirectedOfferCounter from owner directory.";
        return tefBAD_LEDGER;
        // LCOV_EXCL_STOP
    }

    if (!ctx.view.dirRemove(
            keylet::ownerDir(destination), counter->at(sfDestinationNode), counter->key(), true))
    {
        // LCOV_EXCL_START
        JLOG(j.fatal()) << "Unable to remove DirectedOfferCounter from destination directory.";
        return tefBAD_LEDGER;
        // LCOV_EXCL_STOP
    }

    STAmount const amount = counter->at(sfAmount);
    AccountID const issuer = amount.getIssuer();

    if (auto const page = counter->at(~sfIssuerNode))
    {
        if (!ctx.view.dirRemove(keylet::ownerDir(issuer), *page, counter->key(), true))
        {
            // LCOV_EXCL_START
            JLOG(j.fatal()) << "Unable to remove DirectedOfferCounter from issuer directory.";
            return tefBAD_LEDGER;
            // LCOV_EXCL_STOP
        }
    }

    // Release the reserve first: returning the payment can re-create a holding
    // the owner deleted, and the bid must not count against it.
    decreaseOwnerCountForObject(ctx.view, sleOwner, counter, 1, j);
    ctx.view.update(sleOwner);

    auto const sleReceiver = ctx.view.peek(keylet::account(receiver));
    if (!sleReceiver)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::closeDirectedOfferCounter : the receiver exists");
        return tefINTERNAL;
        // LCOV_EXCL_STOP
    }

    if (isXRP(amount))
    {
        sleReceiver->at(sfBalance) += amount;
    }
    else
    {
        // A refund carries no transfer fee; a taken bid pays the rate locked
        // when it was made, as EscrowFinish does.
        auto const storedRate = counter->at(~sfTransferRate);
        Rate const lockedRate = receiver != owner && storedRate ? Rate{*storedRate} : kParityRate;
        if (auto const ter = std::visit(
                [&]<class T>(T const&) {
                    return escrowUnlockApplyHelper<T>(
                        ctx,
                        lockedRate,
                        sleReceiver,
                        preFeeBalance,
                        amount,
                        issuer,
                        owner,
                        receiver,
                        createAsset,
                        j);
                },
                amount.asset().value());
            !isTesSuccess(ter))
            return ter;
    }

    ctx.view.update(sleReceiver);
    ctx.view.erase(counter);
    return tesSUCCESS;
}

}  // namespace xrpl
