// Auto-generated unit tests for transaction DirectedOfferCreate


#include <gtest/gtest.h>

#include <protocol_autogen/TestHelpers.h>

#include <xrpl/protocol/SecretKey.h>
#include <xrpl/protocol/Seed.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol_autogen/transactions/DirectedOfferCreate.h>
#include <xrpl/protocol_autogen/transactions/AccountSet.h>

#include <string>

namespace xrpl::transactions {

// 1 & 4) Set fields via builder setters, build, then read them back via
// wrapper getters. After build(), validate() should succeed.
TEST(TransactionsDirectedOfferCreateTests, BuilderSettersRoundTrip)
{
    // Generate a deterministic keypair for signing
    auto const [publicKey, secretKey] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testDirectedOfferCreate"));

    // Common transaction fields
    auto const accountValue = calcAccountID(publicKey);
    std::uint32_t const sequenceValue = 1;
    auto const feeValue = canonical_AMOUNT();

    // Transaction-specific field values
    auto const amountValue = canonical_AMOUNT();
    auto const paymentAssetValue = canonical_ISSUE();
    auto const priceTiersValue = canonical_ARRAY();
    auto const counterpartiesValue = canonical_ARRAY();
    auto const domainIDValue = canonical_UINT256();
    auto const expirationValue = canonical_UINT32();

    DirectedOfferCreateBuilder builder{
        accountValue,
        amountValue,
        paymentAssetValue,
        priceTiersValue,
        sequenceValue,
        feeValue
    };

    // Set optional fields
    builder.setCounterparties(counterpartiesValue);
    builder.setDomainID(domainIDValue);
    builder.setExpiration(expirationValue);

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
        auto const& expected = amountValue;
        auto const actual = tx.getAmount();
        expectEqualField(expected, actual, "sfAmount");
    }

    {
        auto const& expected = paymentAssetValue;
        auto const actual = tx.getPaymentAsset();
        expectEqualField(expected, actual, "sfPaymentAsset");
    }

    {
        auto const& expected = priceTiersValue;
        auto const actual = tx.getPriceTiers();
        expectEqualField(expected, actual, "sfPriceTiers");
    }

    // Verify optional fields
    {
        auto const& expected = counterpartiesValue;
        auto const actualOpt = tx.getCounterparties();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfCounterparties should be present";
        expectEqualField(expected, *actualOpt, "sfCounterparties");
        EXPECT_TRUE(tx.hasCounterparties());
    }

    {
        auto const& expected = domainIDValue;
        auto const actualOpt = tx.getDomainID();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfDomainID should be present";
        expectEqualField(expected, *actualOpt, "sfDomainID");
        EXPECT_TRUE(tx.hasDomainID());
    }

    {
        auto const& expected = expirationValue;
        auto const actualOpt = tx.getExpiration();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfExpiration should be present";
        expectEqualField(expected, *actualOpt, "sfExpiration");
        EXPECT_TRUE(tx.hasExpiration());
    }

}

// 2 & 4) Start from an STTx, construct a builder from it, build a new wrapper,
// and verify all fields match.
TEST(TransactionsDirectedOfferCreateTests, BuilderFromStTxRoundTrip)
{
    // Generate a deterministic keypair for signing
    auto const [publicKey, secretKey] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testDirectedOfferCreateFromTx"));

    // Common transaction fields
    auto const accountValue = calcAccountID(publicKey);
    std::uint32_t const sequenceValue = 2;
    auto const feeValue = canonical_AMOUNT();

    // Transaction-specific field values
    auto const amountValue = canonical_AMOUNT();
    auto const paymentAssetValue = canonical_ISSUE();
    auto const priceTiersValue = canonical_ARRAY();
    auto const counterpartiesValue = canonical_ARRAY();
    auto const domainIDValue = canonical_UINT256();
    auto const expirationValue = canonical_UINT32();

    // Build an initial transaction
    DirectedOfferCreateBuilder initialBuilder{
        accountValue,
        amountValue,
        paymentAssetValue,
        priceTiersValue,
        sequenceValue,
        feeValue
    };

    initialBuilder.setCounterparties(counterpartiesValue);
    initialBuilder.setDomainID(domainIDValue);
    initialBuilder.setExpiration(expirationValue);

    auto initialTx = initialBuilder.build(publicKey, secretKey);

    // Create builder from existing STTx
    DirectedOfferCreateBuilder builderFromTx{initialTx.getSTTx()};

    auto rebuiltTx = builderFromTx.build(publicKey, secretKey);

    std::string reason;
    EXPECT_TRUE(rebuiltTx.validate(reason)) << reason;

    // Verify common fields
    EXPECT_EQ(rebuiltTx.getAccount(), accountValue);
    EXPECT_EQ(rebuiltTx.getSequence(), sequenceValue);
    EXPECT_EQ(rebuiltTx.getFee(), feeValue);

    // Verify required fields
    {
        auto const& expected = amountValue;
        auto const actual = rebuiltTx.getAmount();
        expectEqualField(expected, actual, "sfAmount");
    }

    {
        auto const& expected = paymentAssetValue;
        auto const actual = rebuiltTx.getPaymentAsset();
        expectEqualField(expected, actual, "sfPaymentAsset");
    }

    {
        auto const& expected = priceTiersValue;
        auto const actual = rebuiltTx.getPriceTiers();
        expectEqualField(expected, actual, "sfPriceTiers");
    }

    // Verify optional fields
    {
        auto const& expected = counterpartiesValue;
        auto const actualOpt = rebuiltTx.getCounterparties();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfCounterparties should be present";
        expectEqualField(expected, *actualOpt, "sfCounterparties");
    }

    {
        auto const& expected = domainIDValue;
        auto const actualOpt = rebuiltTx.getDomainID();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfDomainID should be present";
        expectEqualField(expected, *actualOpt, "sfDomainID");
    }

    {
        auto const& expected = expirationValue;
        auto const actualOpt = rebuiltTx.getExpiration();
        ASSERT_TRUE(actualOpt.has_value()) << "Optional field sfExpiration should be present";
        expectEqualField(expected, *actualOpt, "sfExpiration");
    }

}

// 3) Verify wrapper throws when constructed from wrong transaction type.
TEST(TransactionsDirectedOfferCreateTests, WrapperThrowsOnWrongTxType)
{
    // Build a valid transaction of a different type
    auto const [pk, sk] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testWrongType"));
    auto const account = calcAccountID(pk);

    AccountSetBuilder wrongBuilder{account, 1, canonical_AMOUNT()};
    auto wrongTx = wrongBuilder.build(pk, sk);

    EXPECT_THROW(DirectedOfferCreate{wrongTx.getSTTx()}, std::runtime_error);
}

// 4) Verify builder throws when constructed from wrong transaction type.
TEST(TransactionsDirectedOfferCreateTests, BuilderThrowsOnWrongTxType)
{
    // Build a valid transaction of a different type
    auto const [pk, sk] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testWrongTypeBuilder"));
    auto const account = calcAccountID(pk);

    AccountSetBuilder wrongBuilder{account, 1, canonical_AMOUNT()};
    auto wrongTx = wrongBuilder.build(pk, sk);

    EXPECT_THROW(DirectedOfferCreateBuilder{wrongTx.getSTTx()}, std::runtime_error);
}

// 5) Build with only required fields and verify optional fields return nullopt.
TEST(TransactionsDirectedOfferCreateTests, OptionalFieldsReturnNullopt)
{
    // Generate a deterministic keypair for signing
    auto const [publicKey, secretKey] =
        generateKeyPair(KeyType::Secp256k1, generateSeed("testDirectedOfferCreateNullopt"));

    // Common transaction fields
    auto const accountValue = calcAccountID(publicKey);
    std::uint32_t const sequenceValue = 3;
    auto const feeValue = canonical_AMOUNT();

    // Transaction-specific required field values
    auto const amountValue = canonical_AMOUNT();
    auto const paymentAssetValue = canonical_ISSUE();
    auto const priceTiersValue = canonical_ARRAY();

    DirectedOfferCreateBuilder builder{
        accountValue,
        amountValue,
        paymentAssetValue,
        priceTiersValue,
        sequenceValue,
        feeValue
    };

    // Do NOT set optional fields

    auto tx = builder.build(publicKey, secretKey);

    // Verify optional fields are not present
    EXPECT_FALSE(tx.hasCounterparties());
    EXPECT_FALSE(tx.getCounterparties().has_value());
    EXPECT_FALSE(tx.hasDomainID());
    EXPECT_FALSE(tx.getDomainID().has_value());
    EXPECT_FALSE(tx.hasExpiration());
    EXPECT_FALSE(tx.getExpiration().has_value());
}

}
