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

class DirectedOfferAcceptBuilder;

/**
 * @brief Transaction: DirectedOfferAccept
 *
 * Type: ttDIRECTED_OFFER_ACCEPT (121)
 * Delegable: Delegation::Delegable
 * Amendment: featureDirectedOffer
 * Privileges: Privilege::MayCreateMpt
 *
 * Immutable wrapper around STTx providing type-safe field access.
 * Use DirectedOfferAcceptBuilder to construct new transactions.
 */
class DirectedOfferAccept : public TransactionBase
{
public:
    static constexpr xrpl::TxType txType = ttDIRECTED_OFFER_ACCEPT;

    /**
     * @brief Construct a DirectedOfferAccept transaction wrapper from an existing STTx object.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    explicit DirectedOfferAccept(std::shared_ptr<STTx const> tx)
        : TransactionBase(std::move(tx))
    {
        // Verify transaction type
        if (tx_->getTxnType() != txType)
        {
            throw std::runtime_error("Invalid transaction type for DirectedOfferAccept");
        }
    }

    // Transaction-specific field getters

    /**
     * @brief Get sfDirectedOfferID (SoeOptional)
     * @return The field value, or std::nullopt if not present.
     */
    [[nodiscard]]
    protocol_autogen::Optional<SF_UINT256::type::value_type>
    getDirectedOfferID() const
    {
        if (hasDirectedOfferID())
        {
            return this->tx_->at(sfDirectedOfferID);
        }
        return std::nullopt;
    }

    /**
     * @brief Check if sfDirectedOfferID is present.
     * @return True if the field is present, false otherwise.
     */
    [[nodiscard]]
    bool
    hasDirectedOfferID() const
    {
        return this->tx_->isFieldPresent(sfDirectedOfferID);
    }

    /**
     * @brief Get sfQuantity (SoeOptional)
     * @note This field supports MPT (Multi-Purpose Token) amounts.
     * @return The field value, or std::nullopt if not present.
     */
    [[nodiscard]]
    protocol_autogen::Optional<SF_AMOUNT::type::value_type>
    getQuantity() const
    {
        if (hasQuantity())
        {
            return this->tx_->at(sfQuantity);
        }
        return std::nullopt;
    }

    /**
     * @brief Check if sfQuantity is present.
     * @return True if the field is present, false otherwise.
     */
    [[nodiscard]]
    bool
    hasQuantity() const
    {
        return this->tx_->isFieldPresent(sfQuantity);
    }

    /**
     * @brief Get sfSendMax (SoeOptional)
     * @note This field supports MPT (Multi-Purpose Token) amounts.
     * @return The field value, or std::nullopt if not present.
     */
    [[nodiscard]]
    protocol_autogen::Optional<SF_AMOUNT::type::value_type>
    getSendMax() const
    {
        if (hasSendMax())
        {
            return this->tx_->at(sfSendMax);
        }
        return std::nullopt;
    }

    /**
     * @brief Check if sfSendMax is present.
     * @return True if the field is present, false otherwise.
     */
    [[nodiscard]]
    bool
    hasSendMax() const
    {
        return this->tx_->isFieldPresent(sfSendMax);
    }

    /**
     * @brief Get sfDirectedOfferCounterID (SoeOptional)
     * @return The field value, or std::nullopt if not present.
     */
    [[nodiscard]]
    protocol_autogen::Optional<SF_UINT256::type::value_type>
    getDirectedOfferCounterID() const
    {
        if (hasDirectedOfferCounterID())
        {
            return this->tx_->at(sfDirectedOfferCounterID);
        }
        return std::nullopt;
    }

    /**
     * @brief Check if sfDirectedOfferCounterID is present.
     * @return True if the field is present, false otherwise.
     */
    [[nodiscard]]
    bool
    hasDirectedOfferCounterID() const
    {
        return this->tx_->isFieldPresent(sfDirectedOfferCounterID);
    }
};

/**
 * @brief Builder for DirectedOfferAccept transactions.
 *
 * Provides a fluent interface for constructing transactions with method chaining.
 * Uses STObject internally for flexible transaction construction.
 * Inherits common field setters from TransactionBuilderBase.
 */
class DirectedOfferAcceptBuilder : public TransactionBuilderBase<DirectedOfferAcceptBuilder>
{
public:
    /**
     * @brief Construct a new DirectedOfferAcceptBuilder with required fields.
     * @param account The account initiating the transaction.
     * @param sequence Optional sequence number for the transaction.
     * @param fee Optional fee for the transaction.
     */
    DirectedOfferAcceptBuilder(SF_ACCOUNT::type::value_type account,
                    std::optional<SF_UINT32::type::value_type> sequence = std::nullopt,
                    std::optional<SF_AMOUNT::type::value_type> fee = std::nullopt
)
        : TransactionBuilderBase<DirectedOfferAcceptBuilder>(ttDIRECTED_OFFER_ACCEPT, account, sequence, fee)
    {
    }

    /**
     * @brief Construct a DirectedOfferAcceptBuilder from an existing STTx object.
     * @param tx The existing transaction to copy from.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    DirectedOfferAcceptBuilder(std::shared_ptr<STTx const> tx)
    {
        if (tx->getTxnType() != ttDIRECTED_OFFER_ACCEPT)
        {
            throw std::runtime_error("Invalid transaction type for DirectedOfferAcceptBuilder");
        }
        object_ = *tx;
    }

    /**
     * @brief Transaction-specific field setters
     */

    /**
     * @brief Set sfDirectedOfferID (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferAcceptBuilder&
    setDirectedOfferID(std::decay_t<typename SF_UINT256::type::value_type> const& value)
    {
        object_[sfDirectedOfferID] = value;
        return *this;
    }

    /**
     * @brief Set sfQuantity (SoeOptional)
     * @note This field supports MPT (Multi-Purpose Token) amounts.
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferAcceptBuilder&
    setQuantity(std::decay_t<typename SF_AMOUNT::type::value_type> const& value)
    {
        object_[sfQuantity] = value;
        return *this;
    }

    /**
     * @brief Set sfSendMax (SoeOptional)
     * @note This field supports MPT (Multi-Purpose Token) amounts.
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferAcceptBuilder&
    setSendMax(std::decay_t<typename SF_AMOUNT::type::value_type> const& value)
    {
        object_[sfSendMax] = value;
        return *this;
    }

    /**
     * @brief Set sfDirectedOfferCounterID (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferAcceptBuilder&
    setDirectedOfferCounterID(std::decay_t<typename SF_UINT256::type::value_type> const& value)
    {
        object_[sfDirectedOfferCounterID] = value;
        return *this;
    }

    /**
     * @brief Build and return the DirectedOfferAccept wrapper.
     * @param publicKey The public key for signing.
     * @param secretKey The secret key for signing.
     * @return The constructed transaction wrapper.
     */
    DirectedOfferAccept
    build(PublicKey const& publicKey, SecretKey const& secretKey)
    {
        sign(publicKey, secretKey);
        return DirectedOfferAccept{std::make_shared<STTx>(std::move(object_))};
    }
};

}  // namespace xrpl::transactions
