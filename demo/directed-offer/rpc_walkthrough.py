#!/usr/bin/env python3
"""Walk through DirectedOffer on a standalone xrpld over JSON-RPC.

Start the node with ./run-standalone.sh first. Every transaction is signed by
the server (admin port), and the ledger is closed with ledger_accept after
each one, because a standalone ledger only advances when asked to.

Story: gw issues a BOND token (MPT) and a USD stablecoin (IOU). alice holds
bonds and offers 50 of them to bob and carol only, paid in USD, at
1+ for 10, 10+ for 9, 20+ for 8. dave is not on the list. Then alice offers
20 more to bob, and they negotiate with a locked bid.
"""

import json
import sys
import urllib.request

URL = "http://127.0.0.1:5005"
GENESIS = ("rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh", "snoPBrXtMeMyMHUVTgbuqAfg1SUTb")

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
        label=f"fund {name}",
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


def usd_balance(acct, gw):
    for line in rpc("account_lines", account=acct["address"], peer=gw["address"])[
        "lines"
    ]:
        if line["currency"] == "USD":
            return line["balance"]
    return None


def bond_balance(acct, issuance):
    objs = rpc("account_objects", account=acct["address"], type="mptoken")[
        "account_objects"
    ]
    for o in objs:
        if o["MPTokenIssuanceID"] == issuance:
            return o.get("MPTAmount", "0")
    return None


def main():
    print("Feature check")
    feats = rpc("feature", feature="DirectedOffer")
    entry = next(iter(feats.values())) if "enabled" not in feats else feats
    check("DirectedOffer enabled", entry.get("enabled"), True)

    print("Accounts")
    gw, alice, bob, carol, dave = (
        new_account(n) for n in ("gw", "alice", "bob", "carol", "dave")
    )

    print("Issuer setup")
    submit(
        {"TransactionType": "AccountSet", "Account": gw["address"], "SetFlag": 8},
        gw["secret"],
        label="gw DefaultRipple",
    )
    submit(
        {"TransactionType": "AccountSet", "Account": gw["address"], "SetFlag": 17},
        gw["secret"],
        label="gw AllowTrustLineLocking",
    )
    res = submit(
        {
            "TransactionType": "MPTokenIssuanceCreate",
            "Account": gw["address"],
            "Flags": 0x08 | 0x20,
            "AssetScale": 0,
            "MPTokenMetadata": "424F4E44",
        },
        gw["secret"],
        label="gw issues BOND (MPT)",
    )
    issuance = next(
        n["CreatedNode"]["LedgerIndex"]
        for n in tx_meta(res)["AffectedNodes"]
        if n.get("CreatedNode", {}).get("LedgerEntryType") == "MPTokenIssuance"
    )
    issuance = rpc("ledger_entry", index=issuance)["node"]["mpt_issuance_id"]
    usd = {"currency": "USD", "issuer": gw["address"]}
    bond = lambda v: {"mpt_issuance_id": issuance, "value": str(v)}
    dollars = lambda v: {**usd, "value": str(v)}

    for a in (alice, bob, carol, dave):
        submit(
            {
                "TransactionType": "TrustSet",
                "Account": a["address"],
                "LimitAmount": dollars(100000),
            },
            a["secret"],
            label=f"{a['name']} trusts USD",
        )
    submit(
        {
            "TransactionType": "MPTokenAuthorize",
            "Account": alice["address"],
            "MPTokenIssuanceID": issuance,
        },
        alice["secret"],
        label="alice holds BOND",
    )
    submit(
        {
            "TransactionType": "Payment",
            "Account": gw["address"],
            "Destination": alice["address"],
            "Amount": bond(100),
        },
        gw["secret"],
        label="gw sends alice 100 BOND",
    )
    for a in (bob, carol, dave):
        submit(
            {
                "TransactionType": "Payment",
                "Account": gw["address"],
                "Destination": a["address"],
                "Amount": dollars(1000),
            },
            gw["secret"],
            label=f"gw sends {a['name']} 1000 USD",
        )

    print("DirectedOfferCreate")
    tiers = [(1, 10), (10, 9), (20, 8)]
    create = {
        "TransactionType": "DirectedOfferCreate",
        "Account": alice["address"],
        "Amount": bond(50),
        "PaymentAsset": usd,
        "PriceTiers": [
            {"PriceTier": {"MinQuantity": bond(q), "UnitPrice": dollars(p)}}
            for q, p in tiers
        ],
        "Counterparties": [
            {"CounterpartyEntry": {"Account": a["address"]}} for a in (bob, carol)
        ],
    }
    bad = dict(
        create, Counterparties=[{"CounterpartyEntry": {"Account": alice["address"]}}]
    )
    submit(bad, alice["secret"], expect="temMALFORMED", label="seller names itself")
    res = submit(create, alice["secret"], label="alice offers 50 BOND to bob and carol")
    offer_id = next(
        n["CreatedNode"]["LedgerIndex"]
        for n in tx_meta(res)["AffectedNodes"]
        if n.get("CreatedNode", {}).get("LedgerEntryType") == "DirectedOffer"
    )
    print(f"  offer {offer_id}")
    check("alice BOND after lock", bond_balance(alice, issuance), "50")
    seq = res["tx_json"]["Sequence"]
    by_seq = rpc(
        "ledger_entry", directed_offer={"account": alice["address"], "seq": seq}
    )
    check("ledger_entry by account+seq", by_seq.get("index"), offer_id)
    objs = rpc("account_objects", account=alice["address"], type="directed_offer")[
        "account_objects"
    ]
    check("account_objects type=directed_offer", [o["index"] for o in objs], [offer_id])

    def accept(buyer, qty, expect="tesSUCCESS", label=None, **extra):
        return submit(
            {
                "TransactionType": "DirectedOfferAccept",
                "Account": buyer["address"],
                "DirectedOfferID": offer_id,
                "Quantity": bond(qty),
                **extra,
            },
            buyer["secret"],
            expect=expect,
            label=label or f"{buyer['name']} buys {qty}",
        )

    print("DirectedOfferAccept")
    accept(dave, 5, expect="tecNO_PERMISSION", label="dave (not listed) buys 5")
    accept(bob, 5)
    check("bob BOND", bond_balance(bob, issuance), "5")
    check("bob USD (paid 5 x 10)", usd_balance(bob, gw), "950")
    accept(carol, 25)
    check("carol BOND", bond_balance(carol, issuance), "25")
    check("carol USD (paid 25 x 8)", usd_balance(carol, gw), "800")
    accept(
        bob,
        19,
        label="bob buys 19 with SendMax 100",
        expect="tecPATH_PARTIAL",
        SendMax=dollars(100),
    )
    accept(bob, 21, expect="tecINSUFFICIENT_FUNDS", label="bob buys 21 of 20 left")
    accept(bob, 19, label="bob buys 19 more (tier 10+ at 9)")
    check("bob BOND", bond_balance(bob, issuance), "24")
    check("bob USD (paid 50 + 171)", usd_balance(bob, gw), "779")
    check("alice USD (received 50 + 200 + 171)", usd_balance(alice, gw), "421")
    left = rpc("ledger_entry", index=offer_id)["node"]["Amount"]["value"]
    check("left on offer", left, "1")

    print("DirectedOfferCancel")
    submit(
        {
            "TransactionType": "DirectedOfferCancel",
            "Account": bob["address"],
            "DirectedOfferID": offer_id,
        },
        bob["secret"],
        expect="tecNO_PERMISSION",
        label="bob cancels alice's offer",
    )
    submit(
        {
            "TransactionType": "DirectedOfferCancel",
            "Account": alice["address"],
            "DirectedOfferID": offer_id,
        },
        alice["secret"],
        label="alice cancels",
    )
    check("alice BOND after cancel", bond_balance(alice, issuance), "51")
    gone = rpc("ledger_entry", index=offer_id)
    check("offer deleted", gone.get("error"), "entryNotFound")

    print("Negotiation (DirectedOfferCounterCreate)")
    res = submit(
        dict(
            create,
            Amount=bond(20),
            PriceTiers=[
                {"PriceTier": {"MinQuantity": bond(1), "UnitPrice": dollars(10)}}
            ],
            Counterparties=[{"CounterpartyEntry": {"Account": bob["address"]}}],
        ),
        alice["secret"],
        label="alice offers 20 BOND to bob at 10",
    )
    offer2 = created(res, "DirectedOffer")

    def bid(buyer, qty, total, expect="tesSUCCESS"):
        res = submit(
            {
                "TransactionType": "DirectedOfferCounterCreate",
                "Account": buyer["address"],
                "DirectedOfferID": offer2,
                "Quantity": bond(qty),
                "Amount": dollars(total),
            },
            buyer["secret"],
            expect=expect,
            label=f"{buyer['name']} bids {total} USD for {qty}",
        )
        return created(res, "DirectedOfferCounter") if expect == "tesSUCCESS" else None

    bid(carol, 20, 150, expect="tecNO_PERMISSION")
    low = bid(bob, 10, 50)
    check("bob USD with 50 locked", usd_balance(bob, gw), "729")
    seller_bids = rpc(
        "account_objects", account=alice["address"], type="directed_offer_counter"
    )["account_objects"]
    check("alice sees the bid", [o["index"] for o in seller_bids], [low])
    submit(
        {
            "TransactionType": "DirectedOfferCancel",
            "Account": alice["address"],
            "DirectedOfferCounterID": low,
        },
        alice["secret"],
        label="alice declines it",
    )
    check("bob USD refunded", usd_balance(bob, gw), "779")

    better = bid(bob, 20, 150)
    take = {
        "TransactionType": "DirectedOfferAccept",
        "Account": alice["address"],
        "DirectedOfferCounterID": better,
    }
    submit(
        dict(take, Account=bob["address"]),
        bob["secret"],
        "tecNO_PERMISSION",
        "bob takes his own bid",
    )
    submit(take, alice["secret"], label="alice takes bob's bid")
    check("bob BOND", bond_balance(bob, issuance), "44")
    check("bob USD (paid 150, not 200)", usd_balance(bob, gw), "629")
    check("alice USD", usd_balance(alice, gw), "571")
    check("alice BOND", bond_balance(alice, issuance), "31")
    check(
        "offer closed", rpc("ledger_entry", index=offer2).get("error"), "entryNotFound"
    )
    check("bid gone", rpc("ledger_entry", index=better).get("error"), "entryNotFound")

    print(f"\n{'ALL PASSED' if failures == 0 else f'{failures} FAILURE(S)'}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
