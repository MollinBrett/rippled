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

Then open http://127.0.0.1:8088/ and press "Set up demo accounts". This funds gw, alice, bob,
carol and dave from the genesis account, and creates the BOND token (an MPT) and USD (an IOU).

## Notes

- DirectedOffer must be `Supported::Yes` in `features.macro` for the genesis ledger to enable it.
  An unsupported amendment is never voted in.
- `amendments.txt` lists `SHA-512Half(name) name` for each amendment. Regenerate it if
  `features.macro` changes.
- The UI's price quote mirrors `directedOfferPrice` only for display; the ledger decides the
  price.
