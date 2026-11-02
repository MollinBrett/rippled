// Auto-generated unit tests for transaction DirectedOfferCancel


#include <gtest/gtest.h>

#include <protocol_autogen/TestHelpers.h>

#include <xrpl/protocol/SecretKey.h>
#include <xrpl/protocol/Seed.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol_autogen/transactions/DirectedOfferCancel.h>
#include <xrpl/protocol_autogen/transactions/AccountSet.h>

#include <string>

namespace xrpl::transactions {

// 1 & 4) Set fields via builder setters, build, then read them back via
// wrapper getters. After build(), validate() should succeed.
TEST(TransactionsDirectedOfferCancelTests, BuilderSettersRoundTrip)
{
    // Generate a deterministic keypair for signing
    auto const [publicKey, secretKey] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testDirectedOfferCancel"));

    // Common transaction fields
    auto const accountValue = calcAccountID(publicKey);
    std::uint32_t const sequenceValue = 1;
    auto const feeValue = canonical_AMOUNT();

    // Transaction-specific field values
    auto const directedOfferIDValue = canonical_UINT256();
    auto const directedOfferCounterIDValue = canonical_UINT256();

    DirectedOfferCancelBuilder builder{
        accountValue,
        sequenceValue,
        feeValue
    };

    // Set optional fields
    builder.setDirectedOfferID(directedOfferIDValue);
    builder.setDirectedOfferCounterID(directedOfferCounterIDValue);

    auto tx = builder.build(publicKey, secretKey);

    std::string reason;
    EXPECT_TRUE(tx.validate(reason)) << reason;

    // Verify signing was applied
    EXPECT_FALSE(tx.getSigningPubKey().empty());
    EXPECT_TRUE(tx.hasTxnSignature());

    // Verify common fields
    EXPECT_EQ(tx.getAccount(), accountValue);
    EXPECT_EQ(tx.getSequence(), sequenceValue);
    EXPECT_EQ(tx.getFee(), feeValue);

    // Verify required fields
    // Verify optional fields
    {
        auto const& expected = directedOfferIDValue;
        auto const actualOpt = tx.getDirectedOfferID();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfDirectedOfferID should be present";
        expectEqualField(expected, *actualOpt, "sfDirectedOfferID");
        EXPECT_TRUE(tx.hasDirectedOfferID());
    }

    {
        auto const& expected = directedOfferCounterIDValue;
        auto const actualOpt = tx.getDirectedOfferCounterID();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfDirectedOfferCounterID should be present";
        expectEqualField(expected, *actualOpt, "sfDirectedOfferCounterID");
        EXPECT_TRUE(tx.hasDirectedOfferCounterID());
    }

}

// 2 & 4) Start from an STTx, construct a builder from it, build a new wrapper,
// and verify all fields match.
TEST(TransactionsDirectedOfferCancelTests, BuilderFromStTxRoundTrip)
{
    // Generate a deterministic keypair for signing
    auto const [publicKey, secretKey] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testDirectedOfferCancelFromTx"));

    // Common transaction fields
    auto const accountValue = calcAccountID(publicKey);
    std::uint32_t const sequenceValue = 2;
    auto const feeValue = canonical_AMOUNT();

    // Transaction-specific field values
    auto const directedOfferIDValue = canonical_UINT256();
    auto const directedOfferCounterIDValue = canonical_UINT256();

    // Build an initial transaction
    DirectedOfferCancelBuilder initialBuilder{
        accountValue,
        sequenceValue,
        feeValue
    };

    initialBuilder.setDirectedOfferID(directedOfferIDValue);
    initialBuilder.setDirectedOfferCounterID(directedOfferCounterIDValue);

    auto initialTx = initialBuilder.build(publicKey, secretKey);

    // Create builder from existing STTx
    DirectedOfferCancelBuilder builderFromTx{initialTx.getSTTx()};

    auto rebuiltTx = builderFromTx.build(publicKey, secretKey);

    std::string reason;
    EXPECT_TRUE(rebuiltTx.validate(reason)) << reason;

    // Verify common fields
    EXPECT_EQ(rebuiltTx.getAccount(), accountValue);
    EXPECT_EQ(rebuiltTx.getSequence(), sequenceValue);
    EXPECT_EQ(rebuiltTx.getFee(), feeValue);

    // Verify required fields
    // Verify optional fields
    {
        auto const& expected = directedOfferIDValue;
        auto const actualOpt = rebuiltTx.getDirectedOfferID();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfDirectedOfferID should be present";
        expectEqualField(expected, *actualOpt, "sfDirectedOfferID");
    }

    {
        auto const& expected = directedOfferCounterIDValue;
        auto const actualOpt = rebuiltTx.getDirectedOfferCounterID();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfDirectedOfferCounterID should be present";
        expectEqualField(expected, *actualOpt, "sfDirectedOfferCounterID");
    }

}

// 3) Verify wrapper throws when constructed from wrong transaction type.
TEST(TransactionsDirectedOfferCancelTests, WrapperThrowsOnWrongTxType)
{
    // Build a valid transaction of a different type
    auto const [pk, sk] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testWrongType"));
    auto const account = calcAccountID(pk);

    AccountSetBuilder wrongBuilder{account, 1, canonical_AMOUNT()};
    auto wrongTx = wrongBuilder.build(pk, sk);

    EXPECT_THROW(DirectedOfferCancel{wrongTx.getSTTx()}, std::runtime_error);
}

// 4) Verify builder throws when constructed from wrong transaction type.
TEST(TransactionsDirectedOfferCancelTests, BuilderThrowsOnWrongTxType)
{
    // Build a valid transaction of a different type
    auto const [pk, sk] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testWrongTypeBuilder"));
    auto const account = calcAccountID(pk);

    AccountSetBuilder wrongBuilder{account, 1, canonical_AMOUNT()};
    auto wrongTx = wrongBuilder.build(pk, sk);

    EXPECT_THROW(DirectedOfferCancelBuilder{wrongTx.getSTTx()}, std::runtime_error);
}

// 5) Build with only required fields and verify optional fields return nullopt.
TEST(TransactionsDirectedOfferCancelTests, OptionalFieldsReturnNullopt)
{
    // Generate a deterministic keypair for signing
    auto const [publicKey, secretKey] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testDirectedOfferCancelNullopt"));

    // Common transaction fields
    auto const accountValue = calcAccountID(publicKey);
    std::uint32_t const sequenceValue = 3;
    auto const feeValue = canonical_AMOUNT();

    // Transaction-specific required field values

    DirectedOfferCancelBuilder builder{
        accountValue,
        sequenceValue,
        feeValue
    };

    // Do NOT set optional fields

    auto tx = builder.build(publicKey, secretKey);

    // Verify optional fields are not present
    EXPECT_FALSE(tx.hasDirectedOfferID());
    EXPECT_FALSE(tx.getDirectedOfferID().has_value());
    EXPECT_FALSE(tx.hasDirectedOfferCounterID());
    EXPECT_FALSE(tx.getDirectedOfferCounterID().has_value());
}

}
