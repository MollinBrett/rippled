#include <test/jtx/Account.h>
#include <test/jtx/Env.h>
#include <test/jtx/TestHelpers.h>
#include <test/jtx/amount.h>
#include <test/jtx/balance.h>  // IWYU pragma: keep
#include <test/jtx/batch.h>
#include <test/jtx/delegate.h>
#include <test/jtx/directed_offer.h>
#include <test/jtx/flags.h>
#include <test/jtx/mpt.h>
#include <test/jtx/pay.h>
#include <test/jtx/permissioned_dex.h>
#include <test/jtx/sig.h>
#include <test/jtx/sponsor.h>
#include <test/jtx/ter.h>
#include <test/jtx/trust.h>

#include <xrpl/beast/unit_test/suite.h>
#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/TxFlags.h>
#include <xrpl/protocol/XRPAmount.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace xrpl::test {

class DirectedOffer_test : public beast::unit_test::Suite
{
    // An issuer that lets its USD be locked, and holders with USD lines.
    struct IouSetup
    {
        jtx::Account gw{"gw"};
        jtx::Account alice{"alice"};
        jtx::Account bob{"bob"};
        jtx::Account carol{"carol"};
        jtx::Account dave{"dave"};
        jtx::IOU usd = gw["USD"];

        explicit IouSetup(jtx::Env& env)
        {
            using namespace jtx;
            env.fund(XRP(10'000), gw, alice, bob, carol, dave);
            env.close();
            env(fset(gw, asfAllowTrustLineLocking));
            env.trust(usd(100'000), alice, bob, carol, dave);
            env.close();
            env(pay(gw, alice, usd(1'000)));
            env.close();
        }
    };

    static XRPAmount
    xrpBalance(jtx::Env& env, jtx::Account const& account)
    {
        return env.balance(account).value().xrp();
    }

    void
    testDisabled(FeatureBitset features)
    {
        testcase("Disabled without the amendment");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features - featureDirectedOffer};
        IouSetup const s{env};

        env(create(s.alice, s.usd(10), xrpIssue(), {{s.usd(1), XRP(1)}}),
            Counterparties({s.bob}),
            Ter(temDISABLED));
        env(accept(s.bob, uint256{1}, s.usd(1)), Ter(temDISABLED));
        env(cancel(s.alice, uint256{1}), Ter(temDISABLED));
        env.close();
        BEAST_EXPECTS(env.ownerCount(s.alice) == 1, std::to_string(env.ownerCount(s.alice)));
    }

    void
    testMalformed(FeatureBitset features)
    {
        testcase("Malformed and refused creates");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        IouSetup const s{env};
        std::vector<Tier> const tiers{{s.usd(1), XRP(10)}, {s.usd(10), XRP(9)}};

        // Neither a counterparty list nor a domain, or both.
        env(create(s.alice, s.usd(30), xrpIssue(), tiers), Ter(temMALFORMED));
        env(create(s.alice, s.usd(30), xrpIssue(), tiers),
            Counterparties({s.bob}),
            InDomain(uint256{1}),
            Ter(temMALFORMED));

        // The seller in its own list, a duplicate, too many counterparties.
        env(create(s.alice, s.usd(30), xrpIssue(), tiers),
            Counterparties({s.alice}),
            Ter(temMALFORMED));
        env(create(s.alice, s.usd(30), xrpIssue(), tiers),
            Counterparties({s.bob, s.bob}),
            Ter(temMALFORMED));
        std::vector<Account> nine;
        for (int i = 0; i < 9; ++i)
            nine.emplace_back("buyer" + std::to_string(i));
        env(create(s.alice, s.usd(30), xrpIssue(), tiers), Counterparties(nine), Ter(temMALFORMED));

        // No tiers, tiers out of order, a tier above the amount, a price in
        // the wrong asset, paying in the sold asset, nothing to sell.
        env(create(s.alice, s.usd(30), xrpIssue(), {}), Counterparties({s.bob}), Ter(temMALFORMED));
        env(create(s.alice, s.usd(30), xrpIssue(), {{s.usd(10), XRP(9)}, {s.usd(1), XRP(10)}}),
            Counterparties({s.bob}),
            Ter(temMALFORMED));
        env(create(s.alice, s.usd(30), xrpIssue(), {{s.usd(31), XRP(9)}}),
            Counterparties({s.bob}),
            Ter(temBAD_AMOUNT));
        env(create(s.alice, s.usd(30), xrpIssue(), {{s.usd(1), s.usd(9)}}),
            Counterparties({s.bob}),
            Ter(temBAD_AMOUNT));
        env(create(s.alice, s.usd(30), s.usd.issue(), {{s.usd(1), s.usd(9)}}),
            Counterparties({s.bob}),
            Ter(temBAD_AMOUNT));
        env(create(s.alice, s.usd(0), xrpIssue(), tiers),
            Counterparties({s.bob}),
            Ter(temBAD_AMOUNT));

        // A counterparty that does not exist, a domain that does not exist,
        // an expiration already passed, more than the seller holds.
        env(create(s.alice, s.usd(30), xrpIssue(), tiers),
            Counterparties({Account{"nobody"}}),
            Ter(tecNO_TARGET));
        env(create(s.alice, s.usd(30), xrpIssue(), tiers),
            InDomain(uint256{1}),
            Ter(tecOBJECT_NOT_FOUND));
        env(create(s.alice, s.usd(30), xrpIssue(), tiers),
            Counterparties({s.bob}),
            Expires(env.now()),
            Ter(tecEXPIRED));
        env(create(s.alice, s.usd(1'001), xrpIssue(), tiers),
            Counterparties({s.bob}),
            Ter(tecINSUFFICIENT_FUNDS));

        // The issuer must allow its token to be locked.
        env(fclear(s.gw, asfAllowTrustLineLocking));
        env.close();
        env(create(s.alice, s.usd(30), xrpIssue(), tiers),
            Counterparties({s.bob}),
            Ter(tecNO_PERMISSION));
        env.close();

        BEAST_EXPECTS(env.ownerCount(s.alice) == 1, std::to_string(env.ownerCount(s.alice)));
        env.require(Balance(s.alice, s.usd(1'000)));
    }

    void
    testTieredPartialFills(FeatureBitset features)
    {
        testcase("Tiered prices and partial fills, IOU sold for XRP");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        IouSetup const s{env};
        auto const fee = env.current()->fees().base;

        auto const seq = env.seq(s.alice);
        env(create(
                s.alice,
                s.usd(30),
                xrpIssue(),
                {{s.usd(1), XRP(10)}, {s.usd(10), XRP(9)}, {s.usd(20), XRP(8)}}),
            Counterparties({s.bob, s.carol}));
        env.close();
        auto const offerKeylet = keylet(s.alice, seq);
        auto const offerID = offerKeylet.key;

        // The sold asset is locked, and the offer holds one owner reserve.
        env.require(Balance(s.alice, s.usd(970)));
        BEAST_EXPECTS(env.ownerCount(s.alice) == 2, std::to_string(env.ownerCount(s.alice)));

        auto const aliceXrp = xrpBalance(env, s.alice);

        // 5 falls in the first tier: 5 x 10 XRP.
        auto const bobXrp = xrpBalance(env, s.bob);
        env(accept(s.bob, offerID, s.usd(5)));
        env.close();
        env.require(Balance(s.bob, s.usd(5)));
        BEAST_EXPECTS(
            xrpBalance(env, s.bob) == bobXrp - XRP(50).value().xrp() - fee,
            to_string(xrpBalance(env, s.bob)));

        // 20 falls in the third tier, priced as a whole: 20 x 8 XRP.
        auto const carolXrp = xrpBalance(env, s.carol);
        env(accept(s.carol, offerID, s.usd(20)));
        env.close();
        env.require(Balance(s.carol, s.usd(20)));
        BEAST_EXPECTS(
            xrpBalance(env, s.carol) == carolXrp - XRP(160).value().xrp() - fee,
            to_string(xrpBalance(env, s.carol)));
        BEAST_EXPECTS(
            xrpBalance(env, s.alice) == aliceXrp + XRP(210).value().xrp(),
            to_string(xrpBalance(env, s.alice)));

        auto const sle = env.le(offerKeylet);
        if (!BEAST_EXPECT(sle))
            return;
        BEAST_EXPECTS(sle->at(sfAmount) == s.usd(5).value(), sle->at(sfAmount).getFullText());

        // Not a named buyer, the seller itself, more than remains, below the
        // first tier, the wrong asset.
        env(accept(s.dave, offerID, s.usd(1)), Ter(tecNO_PERMISSION));
        env(accept(s.alice, offerID, s.usd(1)), Ter(tecNO_PERMISSION));
        env(accept(s.bob, offerID, s.usd(6)), Ter(tecINSUFFICIENT_FUNDS));
        env(accept(s.bob, offerID, s.usd(0.5)), Ter(tecINSUFFICIENT_PAYMENT));
        env(accept(s.bob, offerID, XRP(1)), Ter(tecWRONG_ASSET));
        env.close();

        // A repeat buy takes the rest and closes the offer.
        env(accept(s.bob, offerID, s.usd(5)));
        env.close();
        env.require(Balance(s.bob, s.usd(10)));
        BEAST_EXPECT(!env.le(offerKeylet));
        BEAST_EXPECTS(env.ownerCount(s.alice) == 1, std::to_string(env.ownerCount(s.alice)));
        env.require(Balance(s.alice, s.usd(970)));
    }

    void
    testRemainderReturned(FeatureBitset features)
    {
        testcase("Remainder below the smallest tier returns to the seller");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        IouSetup const s{env};

        auto const seq = env.seq(s.alice);
        env(create(s.alice, s.usd(25), xrpIssue(), {{s.usd(10), XRP(1)}}), Counterparties({s.bob}));
        env.close();
        env.require(Balance(s.alice, s.usd(975)));

        env(accept(s.bob, keylet(s.alice, seq).key, s.usd(20)));
        env.close();
        BEAST_EXPECT(!env.le(keylet(s.alice, seq)));
        env.require(Balance(s.bob, s.usd(20)));
        env.require(Balance(s.alice, s.usd(980)));
        BEAST_EXPECTS(env.ownerCount(s.alice) == 1, std::to_string(env.ownerCount(s.alice)));
    }

    void
    testMptSoldForIou(FeatureBitset features)
    {
        testcase("MPT sold for an IOU");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        Account const gw{"gw"};
        Account const alice{"alice"};
        Account const bob{"bob"};

        MPTTester mptGw(env, gw, {.holders = {alice, bob}});
        mptGw.create({.ownerCount = 1, .flags = tfMPTCanEscrow | tfMPTCanTransfer});
        mptGw.authorize({.account = alice});
        mptGw.authorize({.account = bob});
        auto const mpt = mptGw["MPT"];
        env(pay(gw, alice, mpt(1'000)));

        auto const usd = gw["USD"];
        env.trust(usd(10'000), alice, bob);
        env.close();
        env(pay(gw, bob, usd(500)));
        env.close();

        auto const seq = env.seq(alice);
        env(create(alice, mpt(100), usd.issue(), {{mpt(1), usd(2)}, {mpt(50), usd(1.5)}}),
            Counterparties({bob}));
        env.close();
        BEAST_EXPECTS(mptGw.getBalance(alice) == 900, std::to_string(mptGw.getBalance(alice)));

        // 60 falls in the second tier: 60 x 1.5 USD.
        env(accept(bob, keylet(alice, seq).key, mpt(60)));
        env.close();
        BEAST_EXPECTS(mptGw.getBalance(bob) == 60, std::to_string(mptGw.getBalance(bob)));
        env.require(Balance(bob, usd(410)));
        env.require(Balance(alice, usd(90)));

        // Cancelling returns the 40 still locked.
        env(cancel(alice, keylet(alice, seq).key));
        env.close();
        BEAST_EXPECT(!env.le(keylet(alice, seq)));
        BEAST_EXPECTS(mptGw.getBalance(alice) == 940, std::to_string(mptGw.getBalance(alice)));
    }

    void
    testIouSoldForMpt(FeatureBitset features)
    {
        testcase("IOU sold for an MPT");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        IouSetup const s{env};

        MPTTester mptGw(env, s.gw, {.holders = {s.alice, s.bob}, .fund = false});
        mptGw.create({.ownerCount = 1, .flags = tfMPTCanTransfer});
        mptGw.authorize({.account = s.alice});
        mptGw.authorize({.account = s.bob});
        auto const mpt = mptGw["MPT"];
        env(pay(s.gw, s.bob, mpt(1'000)));
        env.close();

        auto const seq = env.seq(s.alice);
        env(create(s.alice, s.usd(10), MPTIssue{mpt.issuanceID}, {{s.usd(1), mpt(7)}}),
            Counterparties({s.bob}));
        env.close();

        env(accept(s.bob, keylet(s.alice, seq).key, s.usd(4)));
        env.close();
        env.require(Balance(s.bob, s.usd(4)));
        BEAST_EXPECTS(mptGw.getBalance(s.bob) == 972, std::to_string(mptGw.getBalance(s.bob)));
        BEAST_EXPECTS(mptGw.getBalance(s.alice) == 28, std::to_string(mptGw.getBalance(s.alice)));
    }

    void
    testRequiredAmendments(FeatureBitset features)
    {
        testcase("Requires the token escrow fixes");
        using namespace jtx;
        using namespace directed_offer;

        for (auto const& feature : {fixTokenEscrowV1, fixCleanup3_4_0})
        {
            Env env{*this, features - feature};
            IouSetup const s{env};
            env(create(s.alice, s.usd(10), xrpIssue(), {{s.usd(1), XRP(1)}}),
                Counterparties({s.bob}),
                Ter(temDISABLED));
        }
    }

    void
    testWrongArrayElements(FeatureBitset features)
    {
        testcase("Array elements must be the declared objects");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        IouSetup const s{env};

        // A Counterparties element that is a SignerEntry, not a CounterpartyEntry.
        auto jv = create(s.alice, s.usd(10), xrpIssue(), {{s.usd(1), XRP(1)}});
        json::Value entry;
        entry[sfAccount.jsonName] = s.bob.human();
        entry[sfSignerWeight.jsonName] = 1;
        json::Value wrapper;
        wrapper[sfSignerEntry.jsonName] = entry;
        json::Value counterparties{json::ValueType::Array};
        counterparties.append(wrapper);
        jv[sfCounterparties.jsonName] = counterparties;
        env(jv, Ter(temMALFORMED));
        env.close();
        BEAST_EXPECTS(env.ownerCount(s.alice) == 1, std::to_string(env.ownerCount(s.alice)));
    }

    void
    testSellerFrozen(FeatureBitset features)
    {
        testcase("Freezing or locking the seller stops sales");
        using namespace jtx;
        using namespace directed_offer;

        // IOU: the issuer freezes the seller's line after the offer is made.
        {
            Env env{*this, features};
            IouSetup const s{env};
            auto const seq = env.seq(s.alice);
            env(create(s.alice, s.usd(10), xrpIssue(), {{s.usd(1), XRP(1)}}),
                Counterparties({s.bob}));
            env.close();

            env(trust(s.gw, s.usd(0), s.alice, tfSetFreeze));
            env.close();
            env(accept(s.bob, keylet(s.alice, seq).key, s.usd(1)), Ter(tecFROZEN));
            env.close();

            env(trust(s.gw, s.usd(0), s.alice, tfClearFreeze));
            env.close();
            env(accept(s.bob, keylet(s.alice, seq).key, s.usd(1)));
            env.close();
            env.require(Balance(s.bob, s.usd(1)));
        }

        // MPT: the issuer locks the seller's MPToken after the offer is made.
        {
            Env env{*this, features};
            Account const gw{"gw"};
            Account const alice{"alice"};
            Account const bob{"bob"};
            MPTTester mptGw(env, gw, {.holders = {alice, bob}});
            mptGw.create(
                {.ownerCount = 1, .flags = tfMPTCanEscrow | tfMPTCanTransfer | tfMPTCanLock});
            mptGw.authorize({.account = alice});
            mptGw.authorize({.account = bob});
            auto const mpt = mptGw["MPT"];
            env(pay(gw, alice, mpt(100)));
            env.close();

            auto const seq = env.seq(alice);
            env(create(alice, mpt(10), xrpIssue(), {{mpt(1), XRP(1)}}), Counterparties({bob}));
            env.close();

            mptGw.set({.account = gw, .holder = alice, .flags = tfMPTLock});
            env(accept(bob, keylet(alice, seq).key, mpt(1)), Ter(tecLOCKED));
            env.close();
            BEAST_EXPECTS(mptGw.getBalance(bob) == 0, std::to_string(mptGw.getBalance(bob)));
        }
    }

    void
    testPrecisionLoss(FeatureBitset features)
    {
        testcase("A quantity the remainder cannot represent is refused");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        Account const gw{"gw"};
        Account const alice{"alice"};
        Account const bob{"bob"};
        auto const usd = gw["USD"];
        env.fund(XRP(10'000), gw, alice, bob);
        env.close();
        env(fset(gw, asfAllowTrustLineLocking));
        STAmount const limit{usd.issue(), std::uint64_t{1}, 17};
        STAmount const big{usd.issue(), std::uint64_t{1}, 16};
        env.trust(limit, alice);
        env.trust(limit, bob);
        env.close();
        env(pay(gw, alice, big));
        env.close();

        auto const seq = env.seq(alice);
        env(create(alice, big, xrpIssue(), {{STAmount{usd.issue(), std::uint64_t{1}, -1}, XRP(1)}}),
            Counterparties({bob}));
        env.close();

        // 1e16 - 0.4 rounds back to 1e16 at 16 significant digits.
        env(accept(bob, keylet(alice, seq).key, STAmount{usd.issue(), std::uint64_t{4}, -1}),
            Ter(tecPRECISION_LOSS));
        env.close();
        env.require(Balance(bob, usd(0)));
    }

    void
    testDomain(FeatureBitset features)
    {
        testcase("Buyers by permissioned domain");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        IouSetup const s{env};
        auto const domainID = setupDomain(env, {s.bob});

        auto const seq = env.seq(s.alice);
        env(create(s.alice, s.usd(10), xrpIssue(), {{s.usd(1), XRP(1)}}), InDomain(domainID));
        env.close();

        env(accept(s.carol, keylet(s.alice, seq).key, s.usd(2)), Ter(tecNO_PERMISSION));
        env(accept(s.bob, keylet(s.alice, seq).key, s.usd(2)));
        env.close();
        env.require(Balance(s.bob, s.usd(2)));
    }

    void
    testExpiration(FeatureBitset features)
    {
        testcase("Expiration and cancel");
        using namespace jtx;
        using namespace directed_offer;
        using namespace std::chrono_literals;

        Env env{*this, features};
        IouSetup const s{env};

        auto const seq = env.seq(s.alice);
        env(create(s.alice, s.usd(10), xrpIssue(), {{s.usd(1), XRP(1)}}),
            Counterparties({s.bob}),
            Expires(env.now() + 20s));
        env.close();
        auto const offerID = keylet(s.alice, seq).key;

        // Before expiry only the seller may cancel.
        env(cancel(s.bob, offerID), Ter(tecNO_PERMISSION));
        env.close(30s);

        // After expiry no one can buy, and anyone can cancel.
        env(accept(s.bob, offerID, s.usd(1)), Ter(tecEXPIRED));
        env(cancel(s.dave, offerID));
        env.close();
        BEAST_EXPECT(!env.le(keylet(s.alice, seq)));
        env.require(Balance(s.alice, s.usd(1'000)));
        BEAST_EXPECTS(env.ownerCount(s.alice) == 1, std::to_string(env.ownerCount(s.alice)));
    }

    void
    testSendMax(FeatureBitset features)
    {
        testcase("SendMax caps the payment");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        IouSetup const s{env};

        auto const seq = env.seq(s.alice);
        env(create(s.alice, s.usd(10), xrpIssue(), {{s.usd(1), XRP(10)}}), Counterparties({s.bob}));
        env.close();
        auto const offerID = keylet(s.alice, seq).key;

        env(accept(s.bob, offerID, s.usd(5)), PayAtMost(XRP(49)), Ter(tecPATH_PARTIAL));
        env(accept(s.bob, offerID, s.usd(5)), PayAtMost(XRP(50)));
        env.close();
        env.require(Balance(s.bob, s.usd(5)));
    }

    void
    testDelegation(FeatureBitset features)
    {
        testcase("Delegated accept");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        IouSetup const s{env};
        auto const fee = env.current()->fees().base;

        auto const seq = env.seq(s.alice);
        env(create(s.alice, s.usd(10), xrpIssue(), {{s.usd(1), XRP(10)}}),
            Counterparties({s.carol}));
        env.close();
        auto const offerID = keylet(s.alice, seq).key;

        // Without a grant the delegate cannot act for carol.
        env(accept(s.carol, offerID, s.usd(1)),
            delegate::As(s.dave),
            Ter(terNO_DELEGATE_PERMISSION));
        env.close();

        env(delegate::set(s.carol, s.dave, {"DirectedOfferAccept"}));
        env.close();

        // The buyer checked is carol, the delegating account; dave pays the fee.
        auto const carolXrp = xrpBalance(env, s.carol);
        auto const daveXrp = xrpBalance(env, s.dave);
        env(accept(s.carol, offerID, s.usd(3)), delegate::As(s.dave));
        env.close();
        env.require(Balance(s.carol, s.usd(3)));
        BEAST_EXPECTS(
            xrpBalance(env, s.carol) == carolXrp - XRP(30).value().xrp(),
            to_string(xrpBalance(env, s.carol)));
        BEAST_EXPECTS(xrpBalance(env, s.dave) == daveXrp - fee, to_string(xrpBalance(env, s.dave)));
    }

    void
    testBatch(FeatureBitset features)
    {
        testcase("Accepts inside a Batch");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        IouSetup const s{env};

        auto const seq1 = env.seq(s.alice);
        env(create(s.alice, s.usd(10), xrpIssue(), {{s.usd(1), XRP(1)}}), Counterparties({s.bob}));
        auto const seq2 = env.seq(s.alice);
        env(create(s.alice, s.usd(10), xrpIssue(), {{s.usd(1), XRP(1)}}), Counterparties({s.bob}));
        env.close();
        auto const offer1 = keylet(s.alice, seq1);
        auto const offer2 = keylet(s.alice, seq2);
        auto const batchFee = batch::calcBatchFee(env, 0, 2);

        // Both accepts settle together.
        {
            auto const seq = env.seq(s.bob);
            env(batch::outer(s.bob, seq, batchFee, tfAllOrNothing),
                batch::Inner(accept(s.bob, offer1.key, s.usd(5)), seq + 1),
                batch::Inner(accept(s.bob, offer2.key, s.usd(5)), seq + 2));
            env.close();
            env.require(Balance(s.bob, s.usd(10)));
        }

        // One accept fails, so neither applies.
        {
            auto const seq = env.seq(s.bob);
            env(batch::outer(s.bob, seq, batchFee, tfAllOrNothing),
                batch::Inner(accept(s.bob, offer1.key, s.usd(5)), seq + 1),
                batch::Inner(accept(s.bob, offer2.key, s.usd(50)), seq + 2));
            env.close();
            env.require(Balance(s.bob, s.usd(10)));
            auto const sle = env.le(offer1);
            if (!BEAST_EXPECT(sle))
                return;
            BEAST_EXPECTS(sle->at(sfAmount) == s.usd(5).value(), sle->at(sfAmount).getFullText());
        }
    }

    void
    testSponsorship(FeatureBitset features)
    {
        testcase("Reserve-sponsored create");
        using namespace jtx;
        using namespace directed_offer;

        Env env{*this, features};
        IouSetup const s{env};

        auto const seq = env.seq(s.alice);
        env(create(s.alice, s.usd(10), xrpIssue(), {{s.usd(1), XRP(1)}}),
            Counterparties({s.bob}),
            sponsor::As(s.carol, spfSponsorReserve),
            Sig(sfSponsorSignature, s.carol));
        env.close();

        auto const sle = env.le(keylet(s.alice, seq));
        if (!BEAST_EXPECT(sle))
            return;
        BEAST_EXPECTS(
            sle->at(~sfSponsor) == s.carol.id(),
            to_string(sle->at(~sfSponsor).value_or(AccountID{})));

        env(cancel(s.alice, keylet(s.alice, seq).key));
        env.close();
        BEAST_EXPECT(!env.le(keylet(s.alice, seq)));
        env.require(Balance(s.alice, s.usd(1'000)));
    }

public:
    void
    run() override
    {
        using namespace jtx;
        auto const all = testableAmendments();
        testDisabled(all);
        testMalformed(all);
        testTieredPartialFills(all);
        testRemainderReturned(all);
        testMptSoldForIou(all);
        testIouSoldForMpt(all);
        testRequiredAmendments(all);
        testWrongArrayElements(all);
        testSellerFrozen(all);
        testPrecisionLoss(all);
        testDomain(all);
        testExpiration(all);
        testSendMax(all);
        testDelegation(all);
        testBatch(all);
        testSponsorship(all);
    }
};

BEAST_DEFINE_TESTSUITE(DirectedOffer, app, xrpl);

}  // namespace xrpl::test
