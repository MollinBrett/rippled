#!/usr/bin/env python3
"""Walk through DirectedOffer on a standalone xrpld over JSON-RPC.

Start the node with ./run-standalone.sh first. Every transaction is signed by
the server (admin port), and the ledger is closed with ledger_accept after
each one, because a standalone ledger only advances when asked to.

Story: the Issuer issues ACME 4.25% 2031 bonds as an MPT, one token per bond
of $1,000 face, and a USD stablecoin (IOU). The Seller holds $5MM face and
quotes $2MM to Buyers A and B only, in percent of face: odd lots (under 100
bonds) at 99.75, 100+ at 99.50, 1,000+ at 99.25. Buyer C is not on the list.
Then the Seller quotes $1MM to Buyer A, and they negotiate with a counter-bid.
"""

import json
import sys
import urllib.request

URL = "http://127.0.0.1:5005"
GENESIS = ("rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh", "snoPBrXtMeMyMHUVTgbuqAfg1SUTb")
FACE = 1000  # USD face value of one bond token

failures = 0


def rpc(method, **params):
    body = json.dumps({"method": method, "params": [params]}).encode()
    req = urllib.request.Request(URL, body, {"Content-Type": "application/json"})
    with urllib.request.urlopen(req) as resp:
        return json.load(resp)["result"]


def close():
    rpc("ledger_accept")


def submit(tx, secret, expect="tesSUCCESS", label=None):
    global failures
    res = rpc("submit", tx_json=tx, secret=secret)
    code = res.get("engine_result", res.get("error"))
    close()
    ok = code == expect
    failures += not ok
    print(
        f"  {'ok ' if ok else 'BAD'} {label or tx['TransactionType']}: {code}"
        + (
            ""
            if ok
            else f" (expected {expect}) {res.get('engine_result_message', res)}"
        )
    )
    return res


def tx_meta(res):
    return rpc("tx", transaction=res["tx_json"]["hash"])["meta"]


def created(res, entry_type):
    return next(
        n["CreatedNode"]["LedgerIndex"]
        for n in tx_meta(res)["AffectedNodes"]
        if n.get("CreatedNode", {}).get("LedgerEntryType") == entry_type
    )


def new_account(name):
    w = rpc("wallet_propose")
    acct = {"name": name, "address": w["account_id"], "secret": w["master_seed"]}
    submit(
        {
            "TransactionType": "Payment",
            "Account": GENESIS[0],
            "Destination": acct["address"],
            "Amount": "10000000000",
        },
        GENESIS[1],
        label=f"open {name} account",
    )
    return acct


def check(label, actual, expected):
    global failures
    ok = actual == expected
    failures += not ok
    print(
        f"  {'ok ' if ok else 'BAD'} {label}: {actual}"
        + ("" if ok else f" (expected {expected})")
    )


def usd_balance(acct, issuer):
    for line in rpc("account_lines", account=acct["address"], peer=issuer["address"])[
        "lines"
    ]:
        if line["currency"] == "USD":
            return float(line["balance"])
    return None


def bond_balance(acct, issuance):
    objs = rpc("account_objects", account=acct["address"], type="mptoken")[
        "account_objects"
    ]
    for o in objs:
        if o["MPTokenIssuanceID"] == issuance:
            return int(o.get("MPTAmount", "0"))
    return None


def per_bond(pct):
    """USD per bond for a price in percent of face."""
    return pct * FACE / 100


def main():
    print("Feature check")
    feats = rpc("feature", feature="DirectedOffer")
    entry = next(iter(feats.values())) if "enabled" not in feats else feats
    check("DirectedOffer enabled", entry.get("enabled"), True)

    print("Accounts")
    issuer, seller, buyer_a, buyer_b, buyer_c = (
        new_account(n) for n in ("Issuer", "Seller", "Buyer A", "Buyer B", "Buyer C")
    )

    print("Issuer setup")
    submit(
        {"TransactionType": "AccountSet", "Account": issuer["address"], "SetFlag": 8},
        issuer["secret"],
        label="enable USD transfers between holders",
    )
    submit(
        {"TransactionType": "AccountSet", "Account": issuer["address"], "SetFlag": 17},
        issuer["secret"],
        label="allow USD to be locked for settlement",
    )
    res = submit(
        {
            "TransactionType": "MPTokenIssuanceCreate",
            "Account": issuer["address"],
            "Flags": 0x08 | 0x20,
            "AssetScale": 0,
            "MPTokenMetadata": "41434D453331",
        },
        issuer["secret"],
        label="issue ACME 4.25% 2031 bonds (MPT)",
    )
    issuance = rpc("ledger_entry", index=created(res, "MPTokenIssuance"))["node"][
        "mpt_issuance_id"
    ]
    usd = {"currency": "USD", "issuer": issuer["address"]}
    bonds = lambda v: {"mpt_issuance_id": issuance, "value": str(v)}
    dollars = lambda v: {**usd, "value": str(v)}

    for a in (seller, buyer_a, buyer_b, buyer_c):
        submit(
            {
                "TransactionType": "TrustSet",
                "Account": a["address"],
                "LimitAmount": dollars(1_000_000_000),
            },
            a["secret"],
            label=f"{a['name']} opens a USD line",
        )
        submit(
            {
                "TransactionType": "MPTokenAuthorize",
                "Account": a["address"],
                "MPTokenIssuanceID": issuance,
            },
            a["secret"],
            label=f"{a['name']} opens a bond holding",
        )
    submit(
        {
            "TransactionType": "Payment",
            "Account": issuer["address"],
            "Destination": seller["address"],
            "Amount": bonds(5000),
        },
        issuer["secret"],
        label="deliver 5,000 bonds ($5MM face) to the Seller",
    )
    for a in (buyer_a, buyer_b, buyer_c):
        submit(
            {
                "TransactionType": "Payment",
                "Account": issuer["address"],
                "Destination": a["address"],
                "Amount": dollars(10_000_000),
            },
            issuer["secret"],
            label=f"fund {a['name']} with 10,000,000 USD",
        )

    print("Directed quote (DirectedOfferCreate)")
    tiers = [(1, 99.75), (100, 99.50), (1000, 99.25)]
    quote = {
        "TransactionType": "DirectedOfferCreate",
        "Account": seller["address"],
        "Amount": bonds(2000),
        "PaymentAsset": usd,
        "PriceTiers": [
            {"PriceTier": {"MinQuantity": bonds(q), "UnitPrice": dollars(per_bond(p))}}
            for q, p in tiers
        ],
        "Counterparties": [
            {"CounterpartyEntry": {"Account": a["address"]}} for a in (buyer_a, buyer_b)
        ],
    }
    bad = dict(
        quote, Counterparties=[{"CounterpartyEntry": {"Account": seller["address"]}}]
    )
    submit(bad, seller["secret"], expect="temMALFORMED", label="Seller quotes itself")
    res = submit(quote, seller["secret"], label="quote $2MM face to Buyers A and B")
    quote_id = created(res, "DirectedOffer")
    print(f"  quote {quote_id}")
    check("Seller bonds after lock", bond_balance(seller, issuance), 3000)
    seq = res["tx_json"]["Sequence"]
    by_seq = rpc(
        "ledger_entry", directed_offer={"account": seller["address"], "seq": seq}
    )
    check("ledger_entry by account+seq", by_seq.get("index"), quote_id)
    objs = rpc("account_objects", account=seller["address"], type="directed_offer")[
        "account_objects"
    ]
    check("account_objects type=directed_offer", [o["index"] for o in objs], [quote_id])

    def lift(buyer, qty, expect="tesSUCCESS", label=None, **extra):
        return submit(
            {
                "TransactionType": "DirectedOfferAccept",
                "Account": buyer["address"],
                "DirectedOfferID": quote_id,
                "Quantity": bonds(qty),
                **extra,
            },
            buyer["secret"],
            expect=expect,
            label=label or f"{buyer['name']} lifts {qty} bonds",
        )

    print("Lifting the offer (DirectedOfferAccept)")
    lift(buyer_c, 50, expect="tecNO_PERMISSION", label="Buyer C (not quoted) lifts 50")
    lift(buyer_a, 50, label="Buyer A lifts an odd lot of 50 at 99.75")
    check("Buyer A bonds", bond_balance(buyer_a, issuance), 50)
    check("Buyer A USD (paid 49,875)", usd_balance(buyer_a, issuer), 9_950_125)
    lift(buyer_b, 1200, label="Buyer B lifts 1,200 at 99.25")
    check("Buyer B bonds", bond_balance(buyer_b, issuance), 1200)
    check("Buyer B USD (paid 1,191,000)", usd_balance(buyer_b, issuer), 8_809_000)
    lift(
        buyer_a,
        700,
        label="Buyer A lifts 700 at 99.50, capped at 690,000 USD",
        expect="tecPATH_PARTIAL",
        SendMax=dollars(690_000),
    )
    lift(buyer_a, 751, expect="tecINSUFFICIENT_FUNDS", label="Buyer A lifts 751 of 750")
    lift(buyer_a, 700, label="Buyer A lifts 700 at 99.50")
    check("Buyer A bonds", bond_balance(buyer_a, issuance), 750)
    check(
        "Buyer A USD (paid 49,875 + 696,500)", usd_balance(buyer_a, issuer), 9_253_625
    )
    check("Seller USD received", usd_balance(seller, issuer), 1_937_375)
    left = rpc("ledger_entry", index=quote_id)["node"]["Amount"]["value"]
    check("left on the quote", left, "50")

    print("Pulling the quote (DirectedOfferCancel)")
    submit(
        {
            "TransactionType": "DirectedOfferCancel",
            "Account": buyer_b["address"],
            "DirectedOfferID": quote_id,
        },
        buyer_b["secret"],
        expect="tecNO_PERMISSION",
        label="Buyer B pulls the Seller's quote",
    )
    submit(
        {
            "TransactionType": "DirectedOfferCancel",
            "Account": seller["address"],
            "DirectedOfferID": quote_id,
        },
        seller["secret"],
        label="Seller pulls the quote",
    )
    check("Seller bonds after pull", bond_balance(seller, issuance), 3050)
    gone = rpc("ledger_entry", index=quote_id)
    check("quote deleted", gone.get("error"), "entryNotFound")

    print("Negotiation (DirectedOfferCounterCreate)")
    res = submit(
        dict(
            quote,
            Amount=bonds(1000),
            PriceTiers=[
                {
                    "PriceTier": {
                        "MinQuantity": bonds(1),
                        "UnitPrice": dollars(per_bond(99.50)),
                    }
                }
            ],
            Counterparties=[{"CounterpartyEntry": {"Account": buyer_a["address"]}}],
        ),
        seller["secret"],
        label="quote $1MM face to Buyer A at 99.50",
    )
    quote2 = created(res, "DirectedOffer")

    def counter(buyer, qty, pct, expect="tesSUCCESS"):
        res = submit(
            {
                "TransactionType": "DirectedOfferCounterCreate",
                "Account": buyer["address"],
                "DirectedOfferID": quote2,
                "Quantity": bonds(qty),
                "Amount": dollars(qty * per_bond(pct)),
            },
            buyer["secret"],
            expect=expect,
            label=f"{buyer['name']} counter-bids {pct} for {qty} bonds",
        )
        return created(res, "DirectedOfferCounter") if expect == "tesSUCCESS" else None

    counter(buyer_c, 1000, 99.25, expect="tecNO_PERMISSION")
    low = counter(buyer_a, 500, 99.00)
    check("Buyer A USD with 495,000 locked", usd_balance(buyer_a, issuer), 8_758_625)
    seller_bids = rpc(
        "account_objects", account=seller["address"], type="directed_offer_counter"
    )["account_objects"]
    check("Seller sees the bid", [o["index"] for o in seller_bids], [low])
    submit(
        {
            "TransactionType": "DirectedOfferCancel",
            "Account": seller["address"],
            "DirectedOfferCounterID": low,
        },
        seller["secret"],
        label="Seller declines it",
    )
    check("Buyer A USD refunded", usd_balance(buyer_a, issuer), 9_253_625)

    better = counter(buyer_a, 1000, 99.25)
    hit = {
        "TransactionType": "DirectedOfferAccept",
        "Account": seller["address"],
        "DirectedOfferCounterID": better,
    }
    submit(
        dict(hit, Account=buyer_a["address"]),
        buyer_a["secret"],
        "tecNO_PERMISSION",
        "Buyer A hits its own bid",
    )
    submit(hit, seller["secret"], label="Seller hits Buyer A's bid at 99.25")
    check("Buyer A bonds", bond_balance(buyer_a, issuance), 1750)
    check(
        "Buyer A USD (paid 992,500, not 995,000)",
        usd_balance(buyer_a, issuer),
        8_261_125,
    )
    check("Seller USD", usd_balance(seller, issuer), 2_929_875)
    check("Seller bonds", bond_balance(seller, issuance), 2050)
    check(
        "quote closed", rpc("ledger_entry", index=quote2).get("error"), "entryNotFound"
    )
    check("bid gone", rpc("ledger_entry", index=better).get("error"), "entryNotFound")

    print(f"\n{'ALL PASSED' if failures == 0 else f'{failures} FAILURE(S)'}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
