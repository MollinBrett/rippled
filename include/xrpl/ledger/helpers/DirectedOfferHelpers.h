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

}  // namespace xrpl
