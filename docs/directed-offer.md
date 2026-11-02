# Directed Offer: delivery versus payment on the XRPL

Status: proof-of-concept draft, not a standard. Branch `feature/directed-offer`.

## Summary

A seller locks an asset on the ledger and offers it to a named set of buyers at a fixed price
schedule with volume tiers. A named buyer accepts any quantity within the schedule, and the
ledger settles both legs in one transaction: the buyer's payment goes to the seller and the
locked asset goes to the buyer, or neither happens. Both legs may be XRP, an IOU or an MPT.

The XRPL has no way to do this today:

| Existing feature | Why it is not this |
|---|---|
| DEX offers (`OfferCreate`) | Open to every account; no named counterparty, no tiers. |
| `Batch` | Atomic, but both parties must sign the same prepared transaction at the same time; no standing offer. |
| NFT offers | Can name a Destination, but only for NFTs. |
| Checks, Escrow | Move one asset one way; no exchange. |

## Concepts

**Sold asset.** What the seller delivers. Locked in full when the offer is created, so the
seller cannot sell it twice. Locking reuses the token-escrow helpers
(`escrowLockApplyHelper` / `escrowUnlockApplyHelper` in `include/xrpl/ledger/helpers/EscrowHelpers.h`),
so the same issuer controls apply as for token escrow.

**Payment asset.** What the buyer pays with. Not locked; taken from the buyer at accept time.

**Counterparties.** Exactly one of:

- a list of 1 to 8 accounts (`Counterparties`), or
- one permissioned domain (`DomainID`): any account that is a member of the domain may buy,
  checked with `accountInDomain` in `include/xrpl/ledger/helpers/PermissionedDEXHelpers.h`.

**Price tiers.** 1 to 8 tiers, each a `MinQuantity` (in the sold asset) and a `UnitPrice`
(in the payment asset per one unit of the sold asset). `MinQuantity` is strictly increasing.
A tier covers quantities from its `MinQuantity` up to, not including, the next tier's
`MinQuantity`; the last tier covers everything above it.

A purchase of quantity `q` uses the tier with the largest `MinQuantity` not greater than `q`,
and the whole purchase is priced at that tier: `payment = q × UnitPrice`, rounded up in the
seller's favour. The first tier's `MinQuantity` is the smallest quantity anyone can buy.

Example: tiers `{1 → 10}`, `{10 → 9}`, `{20 → 8}`.

| Quantity | Tier | Payment |
|---|---|---|
| 5 | 1–9 | 50 |
| 10 | 10–19 | 90 |
| 25 | 20+ | 200 |

**Partial fills.** A buyer may take any allowed quantity up to what remains. Buyers on the
list share the remainder first come, first served. When the remainder drops below the first
tier's `MinQuantity`, the offer closes and the remainder goes back to the seller.

## Ledger entry: `DirectedOffer`

| Field | Type | Required | Meaning |
|---|---|---|---|
| `Account` | AccountID | yes | Seller and owner. |
| `Amount` | Amount | yes | Sold asset still locked in the offer. |
| `PaymentAsset` | Issue | yes | Asset the buyer pays with. |
| `PriceTiers` | Array of `PriceTier` | yes | 1–8 tiers: `MinQuantity` (Amount), `UnitPrice` (Amount). |
| `Counterparties` | Array of `CounterpartyEntry` | one of | 1–8 accounts allowed to buy, each in a `CounterpartyEntry` with an `Account`. |
| `DomainID` | Hash256 | one of | Permissioned domain whose members may buy. |
| `Expiration` | UInt32 | no | After this time no one can buy and anyone can cancel. |
| `TransferRate` | UInt32 | IOU only | Issuer transfer rate locked at creation, as token escrow does. |
| `OwnerNode` | UInt64 | yes | Position in the seller's owner directory. |
| `IssuerNode` | UInt64 | IOU only | Position in the sold IOU's issuer directory, as token escrow records it. |
| `Sponsor` | AccountID | no | Common field: the account paying the owner reserve, if reserve-sponsored. |
| `Sequence` | UInt32 | yes | Seller's sequence at creation; with `Account` it forms the keylet. |
| `PreviousTxnID`, `PreviousTxnLgrSeq` | | yes | Standard threading fields. |

The entry is linked into the seller's owner directory only and counts one owner reserve.
Buyers learn the offer ID off-ledger from the seller. Linking it into every buyer's directory
is left for later, because it costs a directory entry per buyer.

## Transactions

### `DirectedOfferCreate` (seller)

Fields: `Amount`, `PaymentAsset`, `PriceTiers`, `Counterparties` or `DomainID`, optional `Expiration`.

- preflight: `Amount` positive; `PaymentAsset` differs from the sold asset; tiers 1–8,
  `MinQuantity` positive, strictly increasing and not above `Amount`, every `UnitPrice`
  positive and in `PaymentAsset`; exactly one of `Counterparties` and `DomainID`; the list
  has 1–8 unique accounts and excludes the seller.
- preclaim: the same checks as `EscrowCreate` for the sold asset (issuer allows locking with
  `lsfAllowTrustLineLocking` or `lsfMPTCanEscrow`, the seller is authorized, nothing frozen or
  locked, enough spendable balance); the seller is not the issuer of the sold asset; the
  domain exists; `Expiration` is in the future; the seller has the reserve.
- doApply: lock `Amount`, create the entry, link it into the owner directory, increment the
  owner count.

### `DirectedOfferAccept` (named buyer)

Fields: `OfferID`, `Quantity` (in the sold asset), optional `SendMax` (payment ceiling, in `PaymentAsset`).

- preclaim: the offer exists and has not expired (`tecEXPIRED`); the buyer is on the list or
  in the domain (`tecNO_PERMISSION`); `Quantity` is at least the first tier's `MinQuantity` and
  at most `Amount`; the payment, including any transfer fee, is within the buyer's balance and
  `SendMax`; the buyer can hold the sold asset and the seller can hold the payment asset
  (trust line or MPToken exists, authorized, not frozen or locked).
- doApply, as one unit:
  1. send the payment from buyer to seller (the buyer bears the payment asset's transfer fee,
     as in a `Payment`);
  2. release `Quantity` of the sold asset from the lock to the buyer, applying the locked
     `TransferRate`, as `EscrowFinish` does;
  3. reduce `Amount`; if it falls below the first tier's `MinQuantity`, release the
     remainder to the seller and delete the entry.

  A failure at any step returns a `tec` and the transactor rolls the whole transaction back,
  so neither leg settles alone.

### `DirectedOfferCancel`

Fields: `OfferID`.

- Allowed for the seller at any time, and for any account after `Expiration`.
- Releases the remaining `Amount` to the seller, removes the entry from the owner directory
  and decrements the owner count.

## Amendment

All three transactions and the ledger entry are gated by a new
`XRPL_FEATURE(DirectedOffer, Supported::No, VoteBehavior::DefaultNo)`. With it disabled,
every transaction returns `temDISABLED` and nothing else changes.

## Invariants

Not yet implemented: the transactors' invariant hooks are empty. The protocol-wide checks
(no XRP created, MPT and trust-line rules) do run; these are the offer-specific ones still
to write.

- The sum of `Amount` over `DirectedOffer` entries matches what is locked: for MPTs the
  issuance's `LockedAmount` and holders' `LockedAmount`, for IOUs the balance held at the issuer.
- An accept moves exactly `Quantity` of the sold asset to the buyer (less any locked transfer
  fee) and exactly the computed payment to the seller (plus any transfer fee from the buyer).
- No XRP is created.

## Delegation, sponsorship and Batch

All three follow the patterns the existing Escrow and Check transactions use.

**Delegation.** All three transactions are declared `Delegation::Delegable` in
`transactions.macro`, so an account can let another account submit them on its behalf
with `DelegateSet`. A delegated accept still checks the delegating account, not the
submitter, against `Counterparties` or the domain. No granular permissions in this version.

**Sponsorship.** `Sponsor`, `SponsorFlags` and `SponsorSignature` are common transaction
fields, and `Sponsor` is a common ledger-entry field.

- Fee sponsorship works for all three transactions with no extra code.
- Reserve sponsorship: add all three to the allow-list in `isReserveSponsorAllowed`
  (`src/libxrpl/ledger/helpers/SponsorHelpers.cpp`). On a reserve-sponsored create, the
  sponsor pays the entry's owner reserve and the entry records `Sponsor`, as
  `EscrowCreate` does. Cancel, and an accept that closes the offer, release the reserve
  to whoever paid it. An accept that creates a trust line or MPToken for the buyer may be
  reserve-sponsored for the buyer.

**Batch.** All three may be `Batch` inner transactions. They are not added to
`Batch::kDisabledTxTypes`. This lets a buyer pair an accept with other steps atomically,
for example a `TrustSet` or `MPTokenAuthorize` to hold the sold asset, followed by the
accept, in one batch.

## Implementation notes

- Transaction types: `DirectedOfferCreate` 120, `DirectedOfferAccept` 121,
  `DirectedOfferCancel` 122, chosen well above the current range so this proof of concept
  does not collide with types upstream adds next. Ledger entry type `0x00a0`, keylet
  namespace `'w'`.
- New fields: `DirectedOfferID` (UInt256), `PaymentAsset` (Issue), `Quantity`,
  `MinQuantity`, `UnitPrice` (Amount), `PriceTier` and `CounterpartyEntry` (inner objects),
  `PriceTiers` and `Counterparties` (arrays). `Counterparties` holds `CounterpartyEntry`
  objects, each with an `Account`, because the name `Counterparty` is already an account
  field used by `LoanSet`.
- `UnitPrice` is per smallest unit of the sold asset as the ledger stores it. For an IOU or
  MPT that is one token; for XRP it is one drop, so an XRP sale needs a very small
  `UnitPrice`. Pricing XRP per whole XRP is a change for a later version.
- An accept that pays the seller in an IOU uses a direct credit (`accountSend`) and does not
  check the seller's trust-line limit; creating the offer is taken as the seller's consent
  to receive the payment asset.
- `DirectedOfferAccept` may create the buyer's trust line or MPToken for the sold asset,
  as `EscrowFinish` does for its destination.

## Settled choices

- Repeat buys: a buyer may accept several times while the offer has enough left.
- Rounding: `q × UnitPrice` always rounds up, in the seller's favour.

## Out of scope for the proof of concept

- Counter-offers and on-ledger negotiation.
- Issuers selling their own token (primary issuance). Token escrow forbids an issuer locking
  its own token; supporting it means a different lock path. Planned as a second phase.
- Linking offers into buyers' owner directories.
- Granular delegation permissions for the new transactions.
