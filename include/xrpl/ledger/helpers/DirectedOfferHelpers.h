#pragma once

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/ReadView.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Asset.h>
#include <xrpl/protocol/Rules.h>
#include <xrpl/protocol/STAmount.h>
#include <xrpl/protocol/STArray.h>
#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/XRPAmount.h>

#include <cstddef>
#include <optional>

namespace xrpl {

/**
 * The most price tiers a DirectedOffer may carry.
 */
constexpr std::size_t kMaxDirectedOfferTiers = 8;

/**
 * The most accounts a DirectedOffer may name as counterparties.
 */
constexpr std::size_t kMaxDirectedOfferCounterparties = 8;

/**
 * Whether the amendments the DirectedOffer lock and release depend on are
 * enabled.
 *
 * The offer locks and releases its asset through the token-escrow helpers,
 * whose MPT accounting is only exact with fixTokenEscrowV1 and
 * fixCleanup3_4_0.
 */
[[nodiscard]] bool
directedOfferFeaturesEnabled(Rules const& rules);

/**
 * Price of buying @p quantity from a DirectedOffer.
 *
 * The purchase is priced at the tier with the largest MinQuantity not above
 * @p quantity, and the whole quantity is charged at that tier's UnitPrice,
 * rounded up in the seller's favour.
 *
 * @param tiers The offer's PriceTiers, MinQuantity strictly increasing.
 * @param quantity The quantity of the sold asset being bought.
 * @param paymentAsset The asset the payment is made in.
 * @return The payment, or std::nullopt if @p quantity is below the first
 *         tier's MinQuantity.
 */
[[nodiscard]] std::optional<STAmount>
directedOfferPrice(STArray const& tiers, STAmount const& quantity, Asset const& paymentAsset);

/**
 * Whether @p account may buy from @p offer.
 *
 * True when @p account is in the offer's Counterparties, or when the offer
 * names a DomainID and @p account is a member of that permissioned domain.
 */
[[nodiscard]] bool
isDirectedOfferBuyer(ReadView const& view, SLE const& offer, AccountID const& account);

/**
 * Whether @p quantity can be taken from @p offer.
 *
 * @return tecWRONG_ASSET if @p quantity is not in the sold asset,
 *         tecINSUFFICIENT_FUNDS if it is more than the offer has left,
 *         tecPRECISION_LOSS if it cannot be taken from the remainder exactly,
 *         tesSUCCESS otherwise.
 */
[[nodiscard]] TER
checkDirectedOfferQuantity(SLE const& offer, STAmount const& quantity, beast::Journal j);

/**
 * Whether @p quantity of a DirectedOffer's sold asset can be delivered from
 * @p seller to @p buyer: the seller is not frozen or locked in it, and the
 * buyer may receive it.
 */
[[nodiscard]] TER
checkDirectedOfferDelivery(
    ReadView const& view,
    AccountID const& seller,
    AccountID const& buyer,
    STAmount const& quantity);

/**
 * Deliver part of a DirectedOffer to a buyer.
 *
 * Releases @p quantity of the locked asset to @p sleBuyer, applying the
 * transfer rate locked at creation, and reduces the offer. When what is left
 * falls below the first tier's MinQuantity, returns the remainder to the
 * seller and closes the offer.
 *
 * @param ctx The apply view and transaction.
 * @param offer The DirectedOffer to deliver from.
 * @param sleBuyer The buyer's account root; the caller updates it.
 * @param buyerXrp The buyer's XRP, used if its holding must be created.
 * @param quantity The quantity of the sold asset to deliver.
 * @param createBuyerAsset Whether the buyer's holding may be created; only
 *        when the buyer submits the transaction.
 * @param preFeeBalance The submitter's XRP balance before the fee, used if
 *        closing must re-create the seller's holding.
 * @param createSellerAsset Whether closing may re-create the seller's
 *        holding; only when the seller submits the transaction.
 * @param j Journal for logging.
 */
[[nodiscard]] TER
deliverDirectedOffer(
    ApplyViewContext ctx,
    SLE::Ref offer,
    SLE::Ref sleBuyer,
    XRPAmount buyerXrp,
    STAmount const& quantity,
    bool createBuyerAsset,
    XRPAmount preFeeBalance,
    bool createSellerAsset,
    beast::Journal j);

/**
 * Close a DirectedOffer.
 *
 * Returns whatever is still locked to the offer's owner, removes the offer
 * from the owner's and the issuer's directories, releases its owner reserve
 * and erases it.
 *
 * @param ctx The apply view and transaction.
 * @param offer The DirectedOffer to close.
 * @param preFeeBalance The transaction submitter's XRP balance before the fee,
 *        used if the owner's holding must be re-created.
 * @param createAsset Whether the owner's trust line or MPToken may be
 *        re-created; only when the owner submits the transaction.
 * @param j Journal for logging.
 */
[[nodiscard]] TER
closeDirectedOffer(
    ApplyViewContext ctx,
    SLE::Ref offer,
    XRPAmount preFeeBalance,
    bool createAsset,
    beast::Journal j);

/**
 * Close a DirectedOfferCounter.
 *
 * Releases the locked payment to @p receiver: the seller when the bid is
 * taken, applying the transfer rate locked at creation, or the buyer when it
 * is cancelled, with no fee. Removes the bid from the buyer's, the seller's
 * and the issuer's directories, releases its owner reserve and erases it.
 *
 * @param ctx The apply view and transaction.
 * @param counter The DirectedOfferCounter to close.
 * @param receiver The account the payment goes to.
 * @param preFeeBalance The transaction submitter's XRP balance before the
 *        fee, used if the receiver's holding must be created.
 * @param createAsset Whether the receiver's trust line or MPToken may be
 *        created; only when the receiver submits the transaction.
 * @param j Journal for logging.
 */
[[nodiscard]] TER
closeDirectedOfferCounter(
    ApplyViewContext ctx,
    SLE::Ref counter,
    AccountID const& receiver,
    XRPAmount preFeeBalance,
    bool createAsset,
    beast::Journal j);

}  // namespace xrpl
