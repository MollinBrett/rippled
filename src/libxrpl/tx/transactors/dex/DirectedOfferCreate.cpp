#include <xrpl/tx/transactors/dex/DirectedOfferCreate.h>

#include <xrpl/basics/Log.h>
#include <xrpl/beast/utility/Zero.h>
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
        return temBAD_AMOUNT;

    Asset const paymentAsset = ctx.tx[sfPaymentAsset];
    if (paymentAsset == amount.asset())
        return temBAD_AMOUNT;

    if (paymentAsset.holds<Issue>() && !isXRP(paymentAsset) &&
        paymentAsset.get<Issue>().currency == badCurrency())
        return temBAD_CURRENCY;

    auto const& tiers = ctx.tx.getFieldArray(sfPriceTiers);
    if (tiers.empty() || tiers.size() > kMaxDirectedOfferTiers)
        return temMALFORMED;

    std::optional<STAmount> previousMin;
    for (auto const& tier : tiers)
    {
        if (tier.getFName() != sfPriceTier)
            return temMALFORMED;

        STAmount const minQuantity = tier[sfMinQuantity];
        STAmount const unitPrice = tier[sfUnitPrice];

        if (minQuantity.asset() != amount.asset() || !isValidOfferAmount(minQuantity) ||
            minQuantity > amount)
            return temBAD_AMOUNT;

        if (unitPrice.asset() != paymentAsset || !isValidOfferAmount(unitPrice))
            return temBAD_AMOUNT;

        if (previousMin && minQuantity <= *previousMin)
            return temMALFORMED;
        previousMin = minQuantity;
    }

    bool const hasCounterparties = ctx.tx.isFieldPresent(sfCounterparties);
    bool const hasDomain = ctx.tx.isFieldPresent(sfDomainID);
    if (hasCounterparties == hasDomain)
        return temMALFORMED;

    if (hasCounterparties)
    {
        auto const& counterparties = ctx.tx.getFieldArray(sfCounterparties);
        if (counterparties.empty() || counterparties.size() > kMaxDirectedOfferCounterparties)
            return temMALFORMED;

        std::set<AccountID> seen;
        for (auto const& counterparty : counterparties)
        {
            if (counterparty.getFName() != sfCounterpartyEntry)
                return temMALFORMED;

            AccountID const account = counterparty[sfAccount];
            if (account == ctx.tx[sfAccount] || !seen.insert(account).second)
                return temMALFORMED;
        }
    }

    if (hasDomain && ctx.tx[sfDomainID] == beast::kZero)
        return temMALFORMED;

    if (auto const expiration = ctx.tx[~sfExpiration]; expiration && *expiration == 0)
        return temBAD_EXPIRATION;

    return tesSUCCESS;
}

TER
DirectedOfferCreate::preclaim(PreclaimContext const& ctx)
{
    AccountID const account = ctx.tx[sfAccount];
    STAmount const amount = ctx.tx[sfAmount];
    Asset const paymentAsset = ctx.tx[sfPaymentAsset];

    if (hasExpired(ctx.view, ctx.tx[~sfExpiration]))
        return tecEXPIRED;

    // The sold asset is locked under the same issuer controls as a token
    // escrow, with the seller standing in for the destination.
    if (!isXRP(amount))
    {
        if (auto const ret = std::visit(
                [&]<typename T>(T const&) {
                    return escrowLockPreclaimHelper<T>(ctx.view, account, account, amount, ctx.j);
                },
                amount.asset().value());
            !isTesSuccess(ret))
            return ret;
    }

    // The seller must be able to receive the payment asset.
    if (!isXRP(paymentAsset) && paymentAsset.getIssuer() != account)
    {
        if (paymentAsset.holds<Issue>())
        {
            auto const& issue = paymentAsset.get<Issue>();
            if (!ctx.view.exists(keylet::trustLine(account, issue.account, issue.currency)))
                return tecNO_LINE;
        }
        else
        {
            auto const& mptIssue = paymentAsset.get<MPTIssue>();
            if (!ctx.view.exists(keylet::mptokenIssuance(mptIssue.getMptID())))
                return tecOBJECT_NOT_FOUND;
            if (!ctx.view.exists(keylet::mptoken(mptIssue.getMptID(), account)))
                return tecOBJECT_NOT_FOUND;
        }

        if (auto const ret = requireAuth(ctx.view, paymentAsset, account); !isTesSuccess(ret))
            return ret;

        if (auto const ret = checkFrozen(ctx.view, account, paymentAsset); !isTesSuccess(ret))
            return ret;
    }

    if (auto const domainID = ctx.tx[~sfDomainID];
        domainID && !ctx.view.exists(keylet::permissionedDomain(*domainID)))
        return tecOBJECT_NOT_FOUND;

    if (ctx.tx.isFieldPresent(sfCounterparties))
    {
        for (auto const& counterparty : ctx.tx.getFieldArray(sfCounterparties))
        {
            if (!ctx.view.exists(keylet::account(counterparty[sfAccount])))
                return tecNO_TARGET;
        }
    }

    return tesSUCCESS;
}

TER
DirectedOfferCreate::doApply()
{
    auto const sle = ctx_.view().peek(keylet::account(accountID_));
    if (!sle)
        return tefINTERNAL;  // LCOV_EXCL_LINE

    STAmount const amount = ctx_.tx[sfAmount];
    auto const balance = sle->getFieldAmount(sfBalance).xrp();

    // Whoever pays for the new owner increment, the seller or a reserve
    // sponsor, must be able to cover it.
    if (auto const ret =
            checkReserve(ctx_.getApplyViewContext(), sle, balance, {.ownerCountDelta = 1}, j_);
        !isTesSuccess(ret))
        return ret;

    // Locking XRP must leave the seller above its own reserve, which a
    // sponsor never covers.
    if (isXRP(amount))
    {
        auto const sellerReserve = accountReserve(
            ctx_.view(), sle, j_, {.ownerCountDelta = getTxReserveSponsorID(ctx_.tx) ? 0 : 1});
        if (balance - amount.xrp() < sellerReserve)
            return tecUNFUNDED;
    }

    DirectedOfferEntryW offer(accountID_, ctx_.tx.getSeqProxy(), ctx_.view(), j_);
    Keylet const offerKeylet = offer.keylet();
    offer.newSLE();
    (*offer)[sfAccount] = accountID_;
    (*offer)[sfSequence] = ctx_.tx.getSeqProxy().value();
    (*offer)[sfAmount] = amount;
    offer->setFieldIssue(sfPaymentAsset, STIssue{sfPaymentAsset, ctx_.tx[sfPaymentAsset]});
    offer->setFieldArray(sfPriceTiers, ctx_.tx.getFieldArray(sfPriceTiers));
    if (ctx_.tx.isFieldPresent(sfCounterparties))
        offer->setFieldArray(sfCounterparties, ctx_.tx.getFieldArray(sfCounterparties));
    (*offer)[~sfDomainID] = ctx_.tx[~sfDomainID];
    (*offer)[~sfExpiration] = ctx_.tx[~sfExpiration];

    if (!isXRP(amount))
    {
        auto const xferRate = transferRate(ctx_.view(), amount);
        if (xferRate != kParityRate)
            (*offer)[sfTransferRate] = xferRate.value;
    }

    offer.insert();

    {
        auto const page = ctx_.view().dirInsert(
            keylet::ownerDir(accountID_), offerKeylet, describeOwnerDir(accountID_));
        if (!page)
            return tecDIR_FULL;  // LCOV_EXCL_LINE
        (*offer)[sfOwnerNode] = *page;
    }

    // As with token escrow, a locked IOU is linked from the issuer's directory
    // so the issuer can account for it. A locked MPT is tracked on the
    // issuance itself.
    AccountID const issuer = amount.getIssuer();
    if (!isXRP(amount) && amount.holds<Issue>())
    {
        auto const page =
            ctx_.view().dirInsert(keylet::ownerDir(issuer), offerKeylet, describeOwnerDir(issuer));
        if (!page)
            return tecDIR_FULL;  // LCOV_EXCL_LINE
        (*offer)[sfIssuerNode] = *page;
    }

    if (isXRP(amount))
    {
        (*sle)[sfBalance] = (*sle)[sfBalance] - amount;
    }
    else if (
        auto const ret = std::visit(
            [&]<typename T>(T const&) {
                return escrowLockApplyHelper<T>(ctx_.view(), issuer, accountID_, amount, j_);
            },
            amount.asset().value());
        !isTesSuccess(ret))
    {
        return ret;  // LCOV_EXCL_LINE
    }

    increaseOwnerCount(ctx_.getApplyViewContext(), sle, 1, j_);
    addSponsorToLedgerEntry(ctx_.getApplyViewContext(), offer.mutableRawSle());
    ctx_.view().update(sle);
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
