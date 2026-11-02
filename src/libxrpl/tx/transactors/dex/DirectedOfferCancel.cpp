#include <xrpl/tx/transactors/dex/DirectedOfferCancel.h>

#include <xrpl/basics/Log.h>
#include <xrpl/beast/utility/Zero.h>
#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/ReadView.h>
#include <xrpl/ledger/View.h>
#include <xrpl/ledger/entries/DirectedOfferEntry.h>
#include <xrpl/ledger/helpers/DirectedOfferHelpers.h>
#include <xrpl/protocol/AccountID.h>
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
    if (ctx.tx[sfDirectedOfferID] == beast::kZero)
        return temMALFORMED;

    return tesSUCCESS;
}

TER
DirectedOfferCancel::preclaim(PreclaimContext const& ctx)
{
    DirectedOfferEntryR const offer(ctx.tx[sfDirectedOfferID], ctx.view);
    if (!offer)
        return tecNO_ENTRY;

    // Before it expires only the seller may cancel; afterwards anyone may,
    // so an expired offer never strands the seller's asset.
    if (ctx.tx[sfAccount] != (*offer)[sfAccount] && !hasExpired(ctx.view, (*offer)[~sfExpiration]))
        return tecNO_PERMISSION;

    return tesSUCCESS;
}

TER
DirectedOfferCancel::doApply()
{
    DirectedOfferEntryW offer(ctx_.tx[sfDirectedOfferID], view(), j_);
    if (!offer)
        return tecNO_ENTRY;  // LCOV_EXCL_LINE

    bool const createAsset = (*offer)[sfAccount] == accountID_;
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
