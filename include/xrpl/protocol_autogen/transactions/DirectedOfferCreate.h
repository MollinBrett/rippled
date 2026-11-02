// This file is auto-generated. Do not edit.
#pragma once

#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/STParsedJSON.h>
#include <xrpl/protocol/jss.h>
#include <xrpl/protocol_autogen/TransactionBase.h>
#include <xrpl/protocol_autogen/TransactionBuilderBase.h>
#include <xrpl/json/json_value.h>

#include <stdexcept>
#include <optional>

namespace xrpl::transactions {

class DirectedOfferCreateBuilder;

/**
 * @brief Transaction: DirectedOfferCreate
 *
 * Type: ttDIRECTED_OFFER_CREATE (120)
 * Delegable: Delegation::Delegable
 * Amendment: featureDirectedOffer
 * Privileges: Privilege::NoPriv
 *
 * Immutable wrapper around STTx providing type-safe field access.
 * Use DirectedOfferCreateBuilder to construct new transactions.
 */
class DirectedOfferCreate : public TransactionBase
{
public:
    static constexpr xrpl::TxType txType = ttDIRECTED_OFFER_CREATE;

    /**
     * @brief Construct a DirectedOfferCreate transaction wrapper from an existing STTx object.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    explicit DirectedOfferCreate(std::shared_ptr<STTx const> tx)
        : TransactionBase(std::move(tx))
    {
        // Verify transaction type
        if (tx_->getTxnType() != txType)
        {
            throw std::runtime_error("Invalid transaction type for DirectedOfferCreate");
        }
    }

    // Transaction-specific field getters

    /**
     * @brief Get sfAmount (SoeRequired)
     * @note This field supports MPT (Multi-Purpose Token) amounts.
     * @return The field value.
     */
    [[nodiscard]]
    SF_AMOUNT::type::value_type
    getAmount() const
    {
        return this->tx_->at(sfAmount);
    }

    /**
     * @brief Get sfPaymentAsset (SoeRequired)
     * @note This field supports MPT (Multi-Purpose Token) amounts.
     * @return The field value.
     */
    [[nodiscard]]
    SF_ISSUE::type::value_type
    getPaymentAsset() const
    {
        return this->tx_->at(sfPaymentAsset);
    }
    /**
     * @brief Get sfPriceTiers (SoeRequired)
     * @note This is an untyped field.
     * @return The field value.
     */
    [[nodiscard]]
    STArray const&
    getPriceTiers() const
    {
        return this->tx_->getFieldArray(sfPriceTiers);
    }
    /**
     * @brief Get sfCounterparties (SoeOptional)
     * @note This is an untyped field.
     * @return The field value, or std::nullopt if not present.
     */
    [[nodiscard]]
    std::optional<std::reference_wrapper<STArray const>>
    getCounterparties() const
    {
        if (this->tx_->isFieldPresent(sfCounterparties))
            return this->tx_->getFieldArray(sfCounterparties);
        return std::nullopt;
    }

    /**
     * @brief Check if sfCounterparties is present.
     * @return True if the field is present, false otherwise.
     */
    [[nodiscard]]
    bool
    hasCounterparties() const
    {
        return this->tx_->isFieldPresent(sfCounterparties);
    }

    /**
     * @brief Get sfDomainID (SoeOptional)
     * @return The field value, or std::nullopt if not present.
     */
    [[nodiscard]]
    protocol_autogen::Optional<SF_UINT256::type::value_type>
    getDomainID() const
    {
        if (hasDomainID())
        {
            return this->tx_->at(sfDomainID);
        }
        return std::nullopt;
    }

    /**
     * @brief Check if sfDomainID is present.
     * @return True if the field is present, false otherwise.
     */
    [[nodiscard]]
    bool
    hasDomainID() const
    {
        return this->tx_->isFieldPresent(sfDomainID);
    }

    /**
     * @brief Get sfExpiration (SoeOptional)
     * @return The field value, or std::nullopt if not present.
     */
    [[nodiscard]]
    protocol_autogen::Optional<SF_UINT32::type::value_type>
    getExpiration() const
    {
        if (hasExpiration())
        {
            return this->tx_->at(sfExpiration);
        }
        return std::nullopt;
    }

    /**
     * @brief Check if sfExpiration is present.
     * @return True if the field is present, false otherwise.
     */
    [[nodiscard]]
    bool
    hasExpiration() const
    {
        return this->tx_->isFieldPresent(sfExpiration);
    }
};

/**
 * @brief Builder for DirectedOfferCreate transactions.
 *
 * Provides a fluent interface for constructing transactions with method chaining.
 * Uses STObject internally for flexible transaction construction.
 * Inherits common field setters from TransactionBuilderBase.
 */
class DirectedOfferCreateBuilder : public TransactionBuilderBase<DirectedOfferCreateBuilder>
{
public:
    /**
     * @brief Construct a new DirectedOfferCreateBuilder with required fields.
     * @param account The account initiating the transaction.
     * @param amount The sfAmount field value.
     * @param paymentAsset The sfPaymentAsset field value.
     * @param priceTiers The sfPriceTiers field value.
     * @param sequence Optional sequence number for the transaction.
     * @param fee Optional fee for the transaction.
     */
    DirectedOfferCreateBuilder(SF_ACCOUNT::type::value_type account,
                     std::decay_t<typename SF_AMOUNT::type::value_type> const& amount,                     std::decay_t<typename SF_ISSUE::type::value_type> const& paymentAsset,                     STArray const& priceTiers,                    std::optional<SF_UINT32::type::value_type> sequence = std::nullopt,
                    std::optional<SF_AMOUNT::type::value_type> fee = std::nullopt
)
        : TransactionBuilderBase<DirectedOfferCreateBuilder>(ttDIRECTED_OFFER_CREATE, account, sequence, fee)
    {
        setAmount(amount);
        setPaymentAsset(paymentAsset);
        setPriceTiers(priceTiers);
    }

    /**
     * @brief Construct a DirectedOfferCreateBuilder from an existing STTx object.
     * @param tx The existing transaction to copy from.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    DirectedOfferCreateBuilder(std::shared_ptr<STTx const> tx)
    {
        if (tx->getTxnType() != ttDIRECTED_OFFER_CREATE)
        {
            throw std::runtime_error("Invalid transaction type for DirectedOfferCreateBuilder");
        }
        object_ = *tx;
    }

    /**
     * @brief Transaction-specific field setters
     */

    /**
     * @brief Set sfAmount (SoeRequired)
     * @note This field supports MPT (Multi-Purpose Token) amounts.
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCreateBuilder&
    setAmount(std::decay_t<typename SF_AMOUNT::type::value_type> const& value)
    {
        object_[sfAmount] = value;
        return *this;
    }

    /**
     * @brief Set sfPaymentAsset (SoeRequired)
     * @note This field supports MPT (Multi-Purpose Token) amounts.
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCreateBuilder&
    setPaymentAsset(std::decay_t<typename SF_ISSUE::type::value_type> const& value)
    {
        object_[sfPaymentAsset] = STIssue(sfPaymentAsset, value);
        return *this;
    }

    /**
     * @brief Set sfPriceTiers (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCreateBuilder&
    setPriceTiers(STArray const& value)
    {
        object_.setFieldArray(sfPriceTiers, value);
        return *this;
    }

    /**
     * @brief Set sfCounterparties (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCreateBuilder&
    setCounterparties(STArray const& value)
    {
        object_.setFieldArray(sfCounterparties, value);
        return *this;
    }

    /**
     * @brief Set sfDomainID (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCreateBuilder&
    setDomainID(std::decay_t<typename SF_UINT256::type::value_type> const& value)
    {
        object_[sfDomainID] = value;
        return *this;
    }

    /**
     * @brief Set sfExpiration (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCreateBuilder&
    setExpiration(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfExpiration] = value;
        return *this;
    }

    /**
     * @brief Build and return the DirectedOfferCreate wrapper.
     * @param publicKey The public key for signing.
     * @param secretKey The secret key for signing.
     * @return The constructed transaction wrapper.
     */
    DirectedOfferCreate
    build(PublicKey const& publicKey, SecretKey const& secretKey)
    {
        sign(publicKey, secretKey);
        return DirectedOfferCreate{std::make_shared<STTx>(std::move(object_))};
    }
};

}  // namespace xrpl::transactions
