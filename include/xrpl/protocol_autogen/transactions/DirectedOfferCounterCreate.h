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

class DirectedOfferCounterCreateBuilder;

/**
 * @brief Transaction: DirectedOfferCounterCreate
 *
 * Type: ttDIRECTED_OFFER_COUNTER_CREATE (123)
 * Delegable: Delegation::Delegable
 * Amendment: featureDirectedOffer
 * Privileges: Privilege::NoPriv
 *
 * Immutable wrapper around STTx providing type-safe field access.
 * Use DirectedOfferCounterCreateBuilder to construct new transactions.
 */
class DirectedOfferCounterCreate : public TransactionBase
{
public:
    static constexpr xrpl::TxType txType = ttDIRECTED_OFFER_COUNTER_CREATE;

    /**
     * @brief Construct a DirectedOfferCounterCreate transaction wrapper from an existing STTx object.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    explicit DirectedOfferCounterCreate(std::shared_ptr<STTx const> tx)
        : TransactionBase(std::move(tx))
    {
        // Verify transaction type
        if (tx_->getTxnType() != txType)
        {
            throw std::runtime_error("Invalid transaction type for DirectedOfferCounterCreate");
        }
    }

    // Transaction-specific field getters

    /**
     * @brief Get sfDirectedOfferID (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_UINT256::type::value_type
    getDirectedOfferID() const
    {
        return this->tx_->at(sfDirectedOfferID);
    }

    /**
     * @brief Get sfQuantity (SoeRequired)
     * @note This field supports MPT (Multi-Purpose Token) amounts.
     * @return The field value.
     */
    [[nodiscard]]
    SF_AMOUNT::type::value_type
    getQuantity() const
    {
        return this->tx_->at(sfQuantity);
    }

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
 * @brief Builder for DirectedOfferCounterCreate transactions.
 *
 * Provides a fluent interface for constructing transactions with method chaining.
 * Uses STObject internally for flexible transaction construction.
 * Inherits common field setters from TransactionBuilderBase.
 */
class DirectedOfferCounterCreateBuilder : public TransactionBuilderBase<DirectedOfferCounterCreateBuilder>
{
public:
    /**
     * @brief Construct a new DirectedOfferCounterCreateBuilder with required fields.
     * @param account The account initiating the transaction.
     * @param directedOfferID The sfDirectedOfferID field value.
     * @param quantity The sfQuantity field value.
     * @param amount The sfAmount field value.
     * @param sequence Optional sequence number for the transaction.
     * @param fee Optional fee for the transaction.
     */
    DirectedOfferCounterCreateBuilder(SF_ACCOUNT::type::value_type account,
                     std::decay_t<typename SF_UINT256::type::value_type> const& directedOfferID,                     std::decay_t<typename SF_AMOUNT::type::value_type> const& quantity,                     std::decay_t<typename SF_AMOUNT::type::value_type> const& amount,                    std::optional<SF_UINT32::type::value_type> sequence = std::nullopt,
                    std::optional<SF_AMOUNT::type::value_type> fee = std::nullopt
)
        : TransactionBuilderBase<DirectedOfferCounterCreateBuilder>(ttDIRECTED_OFFER_COUNTER_CREATE, account, sequence, fee)
    {
        setDirectedOfferID(directedOfferID);
        setQuantity(quantity);
        setAmount(amount);
    }

    /**
     * @brief Construct a DirectedOfferCounterCreateBuilder from an existing STTx object.
     * @param tx The existing transaction to copy from.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    DirectedOfferCounterCreateBuilder(std::shared_ptr<STTx const> tx)
    {
        if (tx->getTxnType() != ttDIRECTED_OFFER_COUNTER_CREATE)
        {
            throw std::runtime_error("Invalid transaction type for DirectedOfferCounterCreateBuilder");
        }
        object_ = *tx;
    }

    /**
     * @brief Transaction-specific field setters
     */

    /**
     * @brief Set sfDirectedOfferID (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterCreateBuilder&
    setDirectedOfferID(std::decay_t<typename SF_UINT256::type::value_type> const& value)
    {
        object_[sfDirectedOfferID] = value;
        return *this;
    }

    /**
     * @brief Set sfQuantity (SoeRequired)
     * @note This field supports MPT (Multi-Purpose Token) amounts.
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterCreateBuilder&
    setQuantity(std::decay_t<typename SF_AMOUNT::type::value_type> const& value)
    {
        object_[sfQuantity] = value;
        return *this;
    }

    /**
     * @brief Set sfAmount (SoeRequired)
     * @note This field supports MPT (Multi-Purpose Token) amounts.
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterCreateBuilder&
    setAmount(std::decay_t<typename SF_AMOUNT::type::value_type> const& value)
    {
        object_[sfAmount] = value;
        return *this;
    }

    /**
     * @brief Set sfExpiration (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterCreateBuilder&
    setExpiration(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfExpiration] = value;
        return *this;
    }

    /**
     * @brief Build and return the DirectedOfferCounterCreate wrapper.
     * @param publicKey The public key for signing.
     * @param secretKey The secret key for signing.
     * @return The constructed transaction wrapper.
     */
    DirectedOfferCounterCreate
    build(PublicKey const& publicKey, SecretKey const& secretKey)
    {
        sign(publicKey, secretKey);
        return DirectedOfferCounterCreate{std::make_shared<STTx>(std::move(object_))};
    }
};

}  // namespace xrpl::transactions
