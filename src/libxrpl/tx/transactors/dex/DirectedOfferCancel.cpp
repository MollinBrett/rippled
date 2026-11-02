#include <xrpl/tx/transactors/dex/DirectedOfferCancel.h>

#include <xrpl/basics/Log.h>
#include <xrpl/beast/utility/Zero.h>
#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/ReadView.h>
#include <xrpl/ledger/View.h>
#include <xrpl/ledger/entries/DirectedOfferCounterEntry.h>
#include <xrpl/ledger/entries/DirectedOfferEntry.h>
#include <xrpl/ledger/helpers/DirectedOfferHelpers.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/XRPAmount.h>
#include <xrpl/tx/Transactor.h>

namespace xrpl {

bool
DirectedOfferCancel::checkExtraFeatures(PreflightContext const& ctx)
{
    return directedOfferFeaturesEnabled(ctx.rules);
}

NotTEC
DirectedOfferCancel::preflight(PreflightContext const& ctx)
{
    // Exactly one of an offer and a bid, as NFTokenCancelOffer cancels offers
    // of either kind.
    auto const offerID = ctx.tx[~sfDirectedOfferID];
    auto const counterID = ctx.tx[~sfDirectedOfferCounterID];
    if (static_cast<bool>(offerID) == static_cast<bool>(counterID))
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: needs exactly one of DirectedOfferID "
                               "and DirectedOfferCounterID.";
        return temMALFORMED;
    }

    if ((offerID ? *offerID : *counterID) == beast::kZero)
    {
        JLOG(ctx.j.debug()) << "Malformed transaction: zero ID.";
        return temMALFORMED;
    }

    return tesSUCCESS;
}

TER
DirectedOfferCancel::preclaim(PreclaimContext const& ctx)
{
    AccountID const account = ctx.tx[sfAccount];

    if (auto const counterID = ctx.tx[~sfDirectedOfferCounterID])
    {
        DirectedOfferCounterEntryR const counter(*counterID, ctx.view);
        if (!counter)
        {
            JLOG(ctx.j.debug()) << "DirectedOfferCounter does not exist.";
            return tecNO_ENTRY;
        }

        // The buyer withdraws and the seller declines at any time; anyone
        // clears a bid that has expired or whose offer is gone.
        if (account != counter->at(sfAccount) && account != counter->at(sfDestination) &&
            !hasExpired(ctx.view, counter->at(~sfExpiration)) &&
            ctx.view.exists(keylet::directedOffer(counter->at(sfDirectedOfferID))))
        {
            JLOG(ctx.j.debug()) << "Only the buyer or the seller may cancel a live bid.";
            return tecNO_PERMISSION;
        }

        return tesSUCCESS;
    }

    DirectedOfferEntryR const offer(*ctx.tx[~sfDirectedOfferID], ctx.view);
    if (!offer)
    {
        JLOG(ctx.j.debug()) << "DirectedOffer does not exist.";
        return tecNO_ENTRY;
    }

    // Before it expires only the seller may cancel; afterwards anyone may,
    // so an expired offer never strands the seller's asset.
    if (account != offer->at(sfAccount) && !hasExpired(ctx.view, offer->at(~sfExpiration)))
    {
        JLOG(ctx.j.debug()) << "Only the seller may cancel an offer that has not expired.";
        return tecNO_PERMISSION;
    }

    return tesSUCCESS;
}

TER
DirectedOfferCancel::doApply()
{
    if (auto const counterID = ctx_.tx[~sfDirectedOfferCounterID])
    {
        DirectedOfferCounterEntryW counter(*counterID, view(), j_);
        if (!counter)
        {
            // LCOV_EXCL_START
            UNREACHABLE("xrpl::DirectedOfferCancel::doApply : preclaim found the bid");
            return tecINTERNAL;
            // LCOV_EXCL_STOP
        }

        AccountID const buyer = counter->at(sfAccount);
        return closeDirectedOfferCounter(
            ctx_.getApplyViewContext(),
            counter.mutableRawSle(),
            buyer,
            preFeeBalance_,
            buyer == accountID_,
            j_);
    }

    DirectedOfferEntryW offer(*ctx_.tx[~sfDirectedOfferID], view(), j_);
    if (!offer)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::DirectedOfferCancel::doApply : preclaim found the offer");
        return tecINTERNAL;
        // LCOV_EXCL_STOP
    }

    bool const createAsset = offer->at(sfAccount) == accountID_;
    return closeDirectedOffer(
        ctx_.getApplyViewContext(), offer.mutableRawSle(), preFeeBalance_, createAsset, j_);
}

void
DirectedOfferCancel::visitInvariantEntry(bool, SLE::ConstRef, SLE::ConstRef)
{
    // No transaction-specific invariants yet (future work).
}

bool
DirectedOfferCancel::finalizeInvariants(
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
