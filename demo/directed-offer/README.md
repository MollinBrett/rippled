# DirectedOffer standalone demo

A local playground for the DirectedOffer proof of concept: a standalone `xrpld`, a scripted
JSON-RPC walkthrough, and a one-page UI. Use it only on a throwaway local ledger, because the
server signs every transaction with secrets the page holds.

## Run

Build `xrpld` first (from the repository root, `cmake --build .build --target xrpld`). Then:

```bash
./run-standalone.sh
```

This writes `xrpld.cfg`, deletes any previous ledger data, and starts a fresh standalone node with
admin JSON-RPC on `127.0.0.1:5005` and admin WebSocket on `127.0.0.1:6006`. The genesis ledger
enables every supported amendment plus DirectedOffer, from `amendments.txt`. A standalone ledger
advances only on `ledger_accept`.

Scripted walkthrough, in another terminal:

```bash
python3 rpc_walkthrough.py
```

UI:

```bash
python3 -m http.server 8088 --bind 127.0.0.1
```

Then open http://127.0.0.1:8088/ and press "Set up market". This opens accounts for an Issuer,
a Seller and three buyers from the genesis account. The Issuer issues BOND (an MPT) and USD (an
IOU stablecoin), delivers 100 BOND to the Seller and 1,000 USD to each buyer.

The page is split in three, so it is always clear who is acting:

| Pane | Who | Actions | Transaction |
|---|---|---|---|
| Seller desk (left) | Seller | Send a directed quote to chosen counterparties, with size tiers | `DirectedOfferCreate` |
| | | Hit or decline a counter-bid | `DirectedOfferAccept` / `DirectedOfferCancel` with `DirectedOfferCounterID` |
| | | Pull a quote | `DirectedOfferCancel` |
| XRP Ledger (middle) | | Each transaction, who signed it, its result, what it moved for each party, and the raw JSON | |
| Buyer desk (right) | Buyer A, B or C ("Viewing as") | Lift the offer at the quoted price | `DirectedOfferAccept` |
| | | Send a counter-bid: a total price for a size, with the USD locked | `DirectedOfferCounterCreate` |
| | | Withdraw a counter-bid | `DirectedOfferCancel` with `DirectedOfferCounterID` |

By default the Seller quotes Buyers A and B and leaves out Buyer C. Viewing as Buyer C, open
"Quotes on the ledger not addressed to you" and try to lift one: the ledger refuses it with
`tecNO_PERMISSION`.

## Notes

- DirectedOffer must be `Supported::Yes` in `features.macro` for the genesis ledger to enable it.
  An unsupported amendment is never voted in.
- `amendments.txt` lists `SHA-512Half(name) name` for each amendment. Regenerate it if
  `features.macro` changes.
- The UI's price quote mirrors `directedOfferPrice` only for display; the ledger decides the
  price.
