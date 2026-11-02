// Auto-generated unit tests for ledger entry DirectedOffer


#include <gtest/gtest.h>

#include <protocol_autogen/TestHelpers.h>

#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol_autogen/ledger_entries/DirectedOffer.h>
#include <xrpl/protocol_autogen/ledger_entries/Ticket.h>

#include <string>

namespace xrpl::ledger_entries {

// 1 & 4) Set fields via builder setters, build, then read them back via
// wrapper getters. After build(), validate() should succeed for both the
// builder's STObject and the wrapper's SLE.
TEST(DirectedOfferTests, BuilderSettersRoundTrip)
{
    UInt256 const index{1u};

    auto const accountValue = canonical_ACCOUNT();
    auto const sequenceValue = canonical_UINT32();
    auto const amountValue = canonical_AMOUNT();
    auto const paymentAssetValue = canonical_ISSUE();
    auto const priceTiersValue = canonical_ARRAY();
    auto const counterpartiesValue = canonical_ARRAY();
    auto const domainIDValue = canonical_UINT256();
    auto const expirationValue = canonical_UINT32();
    auto const transferRateValue = canonical_UINT32();
    auto const ownerNodeValue = canonical_UINT64();
    auto const issuerNodeValue = canonical_UINT64();
    auto const previousTxnIDValue = canonical_UINT256();
    auto const previousTxnLgrSeqValue = canonical_UINT32();

    DirectedOfferBuilder builder{
        accountValue,
        sequenceValue,
        amountValue,
        paymentAssetValue,
        priceTiersValue,
        ownerNodeValue,
        previousTxnIDValue,
        previousTxnLgrSeqValue
    };

    builder.setCounterparties(counterpartiesValue);
    builder.setDomainID(domainIDValue);
    builder.setExpiration(expirationValue);
    builder.setTransferRate(transferRateValue);
    builder.setIssuerNode(issuerNodeValue);

    builder.setLedgerIndex(index);
    builder.setFlags(0x1u);

    EXPECT_TRUE(builder.validate());

    auto const entry = builder.build(index);

    EXPECT_TRUE(entry.validate());

    {
        auto const& expected = accountValue;
        auto const actual = entry.getAccount();
        expectEqualField(expected, actual, "sfAccount");
    }

    {
        auto const& expected = sequenceValue;
        auto const actual = entry.getSequence();
        expectEqualField(expected, actual, "sfSequence");
    }

    {
        auto const& expected = amountValue;
        auto const actual = entry.getAmount();
        expectEqualField(expected, actual, "sfAmount");
    }

    {
        auto const& expected = paymentAssetValue;
        auto const actual = entry.getPaymentAsset();
        expectEqualField(expected, actual, "sfPaymentAsset");
    }

    {
        auto const& expected = priceTiersValue;
        auto const actual = entry.getPriceTiers();
        expectEqualField(expected, actual, "sfPriceTiers");
    }

    {
        auto const& expected = ownerNodeValue;
        auto const actual = entry.getOwnerNode();
        expectEqualField(expected, actual, "sfOwnerNode");
    }

    {
        auto const& expected = previousTxnIDValue;
        auto const actual = entry.getPreviousTxnID();
        expectEqualField(expected, actual, "sfPreviousTxnID");
    }

    {
        auto const& expected = previousTxnLgrSeqValue;
        auto const actual = entry.getPreviousTxnLgrSeq();
        expectEqualField(expected, actual, "sfPreviousTxnLgrSeq");
    }

    {
        auto const& expected = counterpartiesValue;
        auto const actualOpt = entry.getCounterparties();
        ASSERT_TRUE(actualOpt.has_value());
        expectEqualField(expected, *actualOpt, "sfCounterparties");
        EXPECT_TRUE(entry.hasCounterparties());
    }

    {
        auto const& expected = domainIDValue;
        auto const actualOpt = entry.getDomainID();
        ASSERT_TRUE(actualOpt.has_value());
        expectEqualField(expected, *actualOpt, "sfDomainID");
        EXPECT_TRUE(entry.hasDomainID());
    }

    {
        auto const& expected = expirationValue;
        auto const actualOpt = entry.getExpiration();
        ASSERT_TRUE(actualOpt.has_value());
        expectEqualField(expected, *actualOpt, "sfExpiration");
        EXPECT_TRUE(entry.hasExpiration());
    }

    {
        auto const& expected = transferRateValue;
        auto const actualOpt = entry.getTransferRate();
        ASSERT_TRUE(actualOpt.has_value());
        expectEqualField(expected, *actualOpt, "sfTransferRate");
        EXPECT_TRUE(entry.hasTransferRate());
    }

    {
        auto const& expected = issuerNodeValue;
        auto const actualOpt = entry.getIssuerNode();
        ASSERT_TRUE(actualOpt.has_value());
        expectEqualField(expected, *actualOpt, "sfIssuerNode");
        EXPECT_TRUE(entry.hasIssuerNode());
    }

    EXPECT_TRUE(entry.hasLedgerIndex());
    auto const ledgerIndex = entry.getLedgerIndex();
    ASSERT_TRUE(ledgerIndex.has_value());
    EXPECT_EQ(*ledgerIndex, index);
    EXPECT_EQ(entry.getKey(), index);
}

// 2 & 4) Start from an SLE, set fields directly on it, construct a builder
// from that SLE, build a new wrapper, and verify all fields (and validate()).
TEST(DirectedOfferTests, BuilderFromSleRoundTrip)
{
    UInt256 const index{2u};

    auto const accountValue = canonical_ACCOUNT();
    auto const sequenceValue = canonical_UINT32();
    auto const amountValue = canonical_AMOUNT();
    auto const paymentAssetValue = canonical_ISSUE();
    auto const priceTiersValue = canonical_ARRAY();
    auto const counterpartiesValue = canonical_ARRAY();
    auto const domainIDValue = canonical_UINT256();
    auto const expirationValue = canonical_UINT32();
    auto const transferRateValue = canonical_UINT32();
    auto const ownerNodeValue = canonical_UINT64();
    auto const issuerNodeValue = canonical_UINT64();
    auto const previousTxnIDValue = canonical_UINT256();
    auto const previousTxnLgrSeqValue = canonical_UINT32();

    auto sle = std::make_shared<SLE>(DirectedOffer::entryType, index);

    sle->at(sfAccount) = accountValue;
    sle->at(sfSequence) = sequenceValue;
    sle->at(sfAmount) = amountValue;
    sle->at(sfPaymentAsset) = STIssue(sfPaymentAsset, paymentAssetValue);
    sle->setFieldArray(sfPriceTiers, priceTiersValue);
    sle->setFieldArray(sfCounterparties, counterpartiesValue);
    sle->at(sfDomainID) = domainIDValue;
    sle->at(sfExpiration) = expirationValue;
    sle->at(sfTransferRate) = transferRateValue;
    sle->at(sfOwnerNode) = ownerNodeValue;
    sle->at(sfIssuerNode) = issuerNodeValue;
    sle->at(sfPreviousTxnID) = previousTxnIDValue;
    sle->at(sfPreviousTxnLgrSeq) = previousTxnLgrSeqValue;

    DirectedOfferBuilder builderFromSle{sle};
    EXPECT_TRUE(builderFromSle.validate());

    auto const entryFromBuilder = builderFromSle.build(index);

    DirectedOffer entryFromSle{sle};
    EXPECT_TRUE(entryFromBuilder.validate());
    EXPECT_TRUE(entryFromSle.validate());

    {
        auto const& expected = accountValue;

        auto const fromSle = entryFromSle.getAccount();
        auto const fromBuilder = entryFromBuilder.getAccount();

        expectEqualField(expected, fromSle, "sfAccount");
        expectEqualField(expected, fromBuilder, "sfAccount");
    }

    {
        auto const& expected = sequenceValue;

        auto const fromSle = entryFromSle.getSequence();
        auto const fromBuilder = entryFromBuilder.getSequence();

        expectEqualField(expected, fromSle, "sfSequence");
        expectEqualField(expected, fromBuilder, "sfSequence");
    }

    {
        auto const& expected = amountValue;

        auto const fromSle = entryFromSle.getAmount();
        auto const fromBuilder = entryFromBuilder.getAmount();

        expectEqualField(expected, fromSle, "sfAmount");
        expectEqualField(expected, fromBuilder, "sfAmount");
    }

    {
        auto const& expected = paymentAssetValue;

        auto const fromSle = entryFromSle.getPaymentAsset();
        auto const fromBuilder = entryFromBuilder.getPaymentAsset();

        expectEqualField(expected, fromSle, "sfPaymentAsset");
        expectEqualField(expected, fromBuilder, "sfPaymentAsset");
    }

    {
        auto const& expected = priceTiersValue;

        auto const fromSle = entryFromSle.getPriceTiers();
        auto const fromBuilder = entryFromBuilder.getPriceTiers();

        expectEqualField(expected, fromSle, "sfPriceTiers");
        expectEqualField(expected, fromBuilder, "sfPriceTiers");
    }

    {
        auto const& expected = ownerNodeValue;

        auto const fromSle = entryFromSle.getOwnerNode();
        auto const fromBuilder = entryFromBuilder.getOwnerNode();

        expectEqualField(expected, fromSle, "sfOwnerNode");
        expectEqualField(expected, fromBuilder, "sfOwnerNode");
    }

    {
        auto const& expected = previousTxnIDValue;

        auto const fromSle = entryFromSle.getPreviousTxnID();
        auto const fromBuilder = entryFromBuilder.getPreviousTxnID();

        expectEqualField(expected, fromSle, "sfPreviousTxnID");
        expectEqualField(expected, fromBuilder, "sfPreviousTxnID");
    }

    {
        auto const& expected = previousTxnLgrSeqValue;

        auto const fromSle = entryFromSle.getPreviousTxnLgrSeq();
        auto const fromBuilder = entryFromBuilder.getPreviousTxnLgrSeq();

        expectEqualField(expected, fromSle, "sfPreviousTxnLgrSeq");
        expectEqualField(expected, fromBuilder, "sfPreviousTxnLgrSeq");
    }

    {
        auto const& expected = counterpartiesValue;

        auto const fromSleOpt = entryFromSle.getCounterparties();
        auto const fromBuilderOpt = entryFromBuilder.getCounterparties();

        ASSERT_TRUE(fromSleOpt.has_value());
        ASSERT_TRUE(fromBuilderOpt.has_value());

        expectEqualField(expected, *fromSleOpt, "sfCounterparties");
        expectEqualField(expected, *fromBuilderOpt, "sfCounterparties");
    }

    {
        auto const& expected = domainIDValue;

        auto const fromSleOpt = entryFromSle.getDomainID();
        auto const fromBuilderOpt = entryFromBuilder.getDomainID();

        ASSERT_TRUE(fromSleOpt.has_value());
        ASSERT_TRUE(fromBuilderOpt.has_value());

        expectEqualField(expected, *fromSleOpt, "sfDomainID");
        expectEqualField(expected, *fromBuilderOpt, "sfDomainID");
    }

    {
        auto const& expected = expirationValue;

        auto const fromSleOpt = entryFromSle.getExpiration();
        auto const fromBuilderOpt = entryFromBuilder.getExpiration();

        ASSERT_TRUE(fromSleOpt.has_value());
        ASSERT_TRUE(fromBuilderOpt.has_value());

        expectEqualField(expected, *fromSleOpt, "sfExpiration");
        expectEqualField(expected, *fromBuilderOpt, "sfExpiration");
    }

    {
        auto const& expected = transferRateValue;

        auto const fromSleOpt = entryFromSle.getTransferRate();
        auto const fromBuilderOpt = entryFromBuilder.getTransferRate();

        ASSERT_TRUE(fromSleOpt.has_value());
        ASSERT_TRUE(fromBuilderOpt.has_value());

        expectEqualField(expected, *fromSleOpt, "sfTransferRate");
        expectEqualField(expected, *fromBuilderOpt, "sfTransferRate");
    }

    {
        auto const& expected = issuerNodeValue;

        auto const fromSleOpt = entryFromSle.getIssuerNode();
        auto const fromBuilderOpt = entryFromBuilder.getIssuerNode();

        ASSERT_TRUE(fromSleOpt.has_value());
        ASSERT_TRUE(fromBuilderOpt.has_value());

        expectEqualField(expected, *fromSleOpt, "sfIssuerNode");
        expectEqualField(expected, *fromBuilderOpt, "sfIssuerNode");
    }

    EXPECT_EQ(entryFromSle.getKey(), index);
    EXPECT_EQ(entryFromBuilder.getKey(), index);
}

// 3) Verify wrapper throws when constructed from wrong ledger entry type.
TEST(DirectedOfferTests, WrapperThrowsOnWrongEntryType)
{
    UInt256 const index{3u};

    // Build a valid ledger entry of a different type
    // Ticket requires: Account, OwnerNode, TicketSequence, PreviousTxnID, PreviousTxnLgrSeq
    // Check requires: Account, Destination, SendMax, Sequence, OwnerNode, DestinationNode, PreviousTxnID, PreviousTxnLgrSeq
    TicketBuilder wrongBuilder{
        canonical_ACCOUNT(),
        canonical_UINT64(),
        canonical_UINT32(),
        canonical_UINT256(),
        canonical_UINT32()};
    auto wrongEntry = wrongBuilder.build(index);

    EXPECT_THROW(DirectedOffer{wrongEntry.getSle()}, std::runtime_error);
}

// 4) Verify builder throws when constructed from wrong ledger entry type.
TEST(DirectedOfferTests, BuilderThrowsOnWrongEntryType)
{
    UInt256 const index{4u};

    // Build a valid ledger entry of a different type
    TicketBuilder wrongBuilder{
        canonical_ACCOUNT(),
        canonical_UINT64(),
        canonical_UINT32(),
        canonical_UINT256(),
        canonical_UINT32()};
    auto wrongEntry = wrongBuilder.build(index);

    EXPECT_THROW(DirectedOfferBuilder{wrongEntry.getSle()}, std::runtime_error);
}

// 5) Build with only required fields and verify optional fields return nullopt.
TEST(DirectedOfferTests, OptionalFieldsReturnNullopt)
{
    UInt256 const index{3u};

    auto const accountValue = canonical_ACCOUNT();
    auto const sequenceValue = canonical_UINT32();
    auto const amountValue = canonical_AMOUNT();
    auto const paymentAssetValue = canonical_ISSUE();
    auto const priceTiersValue = canonical_ARRAY();
    auto const ownerNodeValue = canonical_UINT64();
    auto const previousTxnIDValue = canonical_UINT256();
    auto const previousTxnLgrSeqValue = canonical_UINT32();

    DirectedOfferBuilder builder{
        accountValue,
        sequenceValue,
        amountValue,
        paymentAssetValue,
        priceTiersValue,
        ownerNodeValue,
        previousTxnIDValue,
        previousTxnLgrSeqValue
    };

    auto const entry = builder.build(index);

    // Verify optional fields are not present
    EXPECT_FALSE(entry.hasCounterparties());
    EXPECT_FALSE(entry.getCounterparties().has_value());
    EXPECT_FALSE(entry.hasDomainID());
    EXPECT_FALSE(entry.getDomainID().has_value());
    EXPECT_FALSE(entry.hasExpiration());
    EXPECT_FALSE(entry.getExpiration().has_value());
    EXPECT_FALSE(entry.hasTransferRate());
    EXPECT_FALSE(entry.getTransferRate().has_value());
    EXPECT_FALSE(entry.hasIssuerNode());
    EXPECT_FALSE(entry.getIssuerNode().has_value());
}
}
