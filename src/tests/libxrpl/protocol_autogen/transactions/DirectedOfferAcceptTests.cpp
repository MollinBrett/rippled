// Auto-generated unit tests for transaction DirectedOfferAccept


#include <gtest/gtest.h>

#include <protocol_autogen/TestHelpers.h>

#include <xrpl/protocol/SecretKey.h>
#include <xrpl/protocol/Seed.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol_autogen/transactions/DirectedOfferAccept.h>
#include <xrpl/protocol_autogen/transactions/AccountSet.h>

#include <string>

namespace xrpl::transactions {

// 1 & 4) Set fields via builder setters, build, then read them back via
// wrapper getters. After build(), validate() should succeed.
TEST(TransactionsDirectedOfferAcceptTests, BuilderSettersRoundTrip)
{
    // Generate a deterministic keypair for signing
    auto const [publicKey, secretKey] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testDirectedOfferAccept"));

    // Common transaction fields
    auto const accountValue = calcAccountID(publicKey);
    std::uint32_t const sequenceValue = 1;
    auto const feeValue = canonical_AMOUNT();

    // Transaction-specific field values
    auto const directedOfferIDValue = canonical_UINT256();
    auto const quantityValue = canonical_AMOUNT();
    auto const sendMaxValue = canonical_AMOUNT();

    DirectedOfferAcceptBuilder builder{
        accountValue,
        directedOfferIDValue,
        quantityValue,
        sequenceValue,
        feeValue
    };

    // Set optional fields
    builder.setSendMax(sendMaxValue);

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
    {
        auto const& expected = directedOfferIDValue;
        auto const actual = tx.getDirectedOfferID();
        expectEqualField(expected, actual, "sfDirectedOfferID");
    }

    {
        auto const& expected = quantityValue;
        auto const actual = tx.getQuantity();
        expectEqualField(expected, actual, "sfQuantity");
    }

    // Verify optional fields
    {
        auto const& expected = sendMaxValue;
        auto const actualOpt = tx.getSendMax();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfSendMax should be present";
        expectEqualField(expected, *actualOpt, "sfSendMax");
        EXPECT_TRUE(tx.hasSendMax());
    }

}

// 2 & 4) Start from an STTx, construct a builder from it, build a new wrapper,
// and verify all fields match.
TEST(TransactionsDirectedOfferAcceptTests, BuilderFromStTxRoundTrip)
{
    // Generate a deterministic keypair for signing
    auto const [publicKey, secretKey] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testDirectedOfferAcceptFromTx"));

    // Common transaction fields
    auto const accountValue = calcAccountID(publicKey);
    std::uint32_t const sequenceValue = 2;
    auto const feeValue = canonical_AMOUNT();

    // Transaction-specific field values
    auto const directedOfferIDValue = canonical_UINT256();
    auto const quantityValue = canonical_AMOUNT();
    auto const sendMaxValue = canonical_AMOUNT();

    // Build an initial transaction
    DirectedOfferAcceptBuilder initialBuilder{
        accountValue,
        directedOfferIDValue,
        quantityValue,
        sequenceValue,
        feeValue
    };

    initialBuilder.setSendMax(sendMaxValue);

    auto initialTx = initialBuilder.build(publicKey, secretKey);

    // Create builder from existing STTx
    DirectedOfferAcceptBuilder builderFromTx{initialTx.getSTTx()};

    auto rebuiltTx = builderFromTx.build(publicKey, secretKey);

    std::string reason;
    EXPECT_TRUE(rebuiltTx.validate(reason)) << reason;

    // Verify common fields
    EXPECT_EQ(rebuiltTx.getAccount(), accountValue);
    EXPECT_EQ(rebuiltTx.getSequence(), sequenceValue);
    EXPECT_EQ(rebuiltTx.getFee(), feeValue);

    // Verify required fields
    {
        auto const& expected = directedOfferIDValue;
        auto const actual = rebuiltTx.getDirectedOfferID();
        expectEqualField(expected, actual, "sfDirectedOfferID");
    }

    {
        auto const& expected = quantityValue;
        auto const actual = rebuiltTx.getQuantity();
        expectEqualField(expected, actual, "sfQuantity");
    }

    // Verify optional fields
    {
        auto const& expected = sendMaxValue;
        auto const actualOpt = rebuiltTx.getSendMax();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfSendMax should be present";
        expectEqualField(expected, *actualOpt, "sfSendMax");
    }

}

// 3) Verify wrapper throws when constructed from wrong transaction type.
TEST(TransactionsDirectedOfferAcceptTests, WrapperThrowsOnWrongTxType)
{
    // Build a valid transaction of a different type
    auto const [pk, sk] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testWrongType"));
    auto const account = calcAccountID(pk);

    AccountSetBuilder wrongBuilder{account, 1, canonical_AMOUNT()};
    auto wrongTx = wrongBuilder.build(pk, sk);

    EXPECT_THROW(DirectedOfferAccept{wrongTx.getSTTx()}, std::runtime_error);
}

// 4) Verify builder throws when constructed from wrong transaction type.
TEST(TransactionsDirectedOfferAcceptTests, BuilderThrowsOnWrongTxType)
{
    // Build a valid transaction of a different type
    auto const [pk, sk] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testWrongTypeBuilder"));
    auto const account = calcAccountID(pk);

    AccountSetBuilder wrongBuilder{account, 1, canonical_AMOUNT()};
    auto wrongTx = wrongBuilder.build(pk, sk);

    EXPECT_THROW(DirectedOfferAcceptBuilder{wrongTx.getSTTx()}, std::runtime_error);
}

// 5) Build with only required fields and verify optional fields return nullopt.
TEST(TransactionsDirectedOfferAcceptTests, OptionalFieldsReturnNullopt)
{
    // Generate a deterministic keypair for signing
    auto const [publicKey, secretKey] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testDirectedOfferAcceptNullopt"));

    // Common transaction fields
    auto const accountValue = calcAccountID(publicKey);
    std::uint32_t const sequenceValue = 3;
    auto const feeValue = canonical_AMOUNT();

    // Transaction-specific required field values
    auto const directedOfferIDValue = canonical_UINT256();
    auto const quantityValue = canonical_AMOUNT();

    DirectedOfferAcceptBuilder builder{
        accountValue,
        directedOfferIDValue,
        quantityValue,
        sequenceValue,
        feeValue
    };

    // Do NOT set optional fields

    auto tx = builder.build(publicKey, secretKey);

    // Verify optional fields are not present
    EXPECT_FALSE(tx.hasSendMax());
    EXPECT_FALSE(tx.getSendMax().has_value());
}

}
