#include <xrpl/ledger/helpers/DirectedOfferHelpers.h>

#include <xrpl/basics/Log.h>
#include <xrpl/beast/utility/Zero.h>
#include <xrpl/ledger/helpers/AccountRootHelpers.h>
#include <xrpl/ledger/helpers/EscrowHelpers.h>
#include <xrpl/ledger/helpers/PermissionedDEXHelpers.h>
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

    return mulRound(quantity, (*tier)[sfUnitPrice], paymentAsset, true);
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

    if (auto const domainID = offer[~sfDomainID])
        return permissioned_dex::accountInDomain(view, account, *domainID);

    return false;  // LCOV_EXCL_LINE
}

TER
closeDirectedOffer(
    ApplyViewContext ctx,
    SLE::Ref offer,
    XRPAmount preFeeBalance,
    bool createAsset,
    beast::Journal j)
{
    AccountID const owner = (*offer)[sfAccount];
    auto const sleOwner = ctx.view.peek(keylet::account(owner));
    if (!sleOwner)
        return tefINTERNAL;  // LCOV_EXCL_LINE

    if (!ctx.view.dirRemove(keylet::ownerDir(owner), (*offer)[sfOwnerNode], offer->key(), true))
    {
        // LCOV_EXCL_START
        JLOG(j.fatal()) << "Unable to remove DirectedOffer from owner directory.";
        return tefBAD_LEDGER;
        // LCOV_EXCL_STOP
    }

    STAmount const amount = (*offer)[sfAmount];
    AccountID const issuer = amount.getIssuer();

    if (auto const page = (*offer)[~sfIssuerNode])
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
            (*sleOwner)[sfBalance] = (*sleOwner)[sfBalance] + amount;
        }
        else if (
            auto const ret = std::visit(
                [&]<typename T>(T const&) {
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
            !isTesSuccess(ret))
        {
            return ret;
        }
    }

    ctx.view.update(sleOwner);
    ctx.view.erase(offer);
    return tesSUCCESS;
}

}  // namespace xrpl
