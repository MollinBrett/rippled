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

class DirectedOfferCancelBuilder;

/**
 * @brief Transaction: DirectedOfferCancel
 *
 * Type: ttDIRECTED_OFFER_CANCEL (122)
 * Delegable: Delegation::Delegable
 * Amendment: featureDirectedOffer
 * Privileges: Privilege::NoPriv
 *
 * Immutable wrapper around STTx providing type-safe field access.
 * Use DirectedOfferCancelBuilder to construct new transactions.
 */
class DirectedOfferCancel : public TransactionBase
{
public:
    static constexpr xrpl::TxType txType = ttDIRECTED_OFFER_CANCEL;

    /**
     * @brief Construct a DirectedOfferCancel transaction wrapper from an existing STTx object.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    explicit DirectedOfferCancel(std::shared_ptr<STTx const> tx)
        : TransactionBase(std::move(tx))
    {
        // Verify transaction type
        if (tx_->getTxnType() != txType)
        {
            throw std::runtime_error("Invalid transaction type for DirectedOfferCancel");
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
};

/**
 * @brief Builder for DirectedOfferCancel transactions.
 *
 * Provides a fluent interface for constructing transactions with method chaining.
 * Uses STObject internally for flexible transaction construction.
 * Inherits common field setters from TransactionBuilderBase.
 */
class DirectedOfferCancelBuilder : public TransactionBuilderBase<DirectedOfferCancelBuilder>
{
public:
    /**
     * @brief Construct a new DirectedOfferCancelBuilder with required fields.
     * @param account The account initiating the transaction.
     * @param directedOfferID The sfDirectedOfferID field value.
     * @param sequence Optional sequence number for the transaction.
     * @param fee Optional fee for the transaction.
     */
    DirectedOfferCancelBuilder(SF_ACCOUNT::type::value_type account,
                     std::decay_t<typename SF_UINT256::type::value_type> const& directedOfferID,                    std::optional<SF_UINT32::type::value_type> sequence = std::nullopt,
                    std::optional<SF_AMOUNT::type::value_type> fee = std::nullopt
)
        : TransactionBuilderBase<DirectedOfferCancelBuilder>(ttDIRECTED_OFFER_CANCEL, account, sequence, fee)
    {
        setDirectedOfferID(directedOfferID);
    }

    /**
     * @brief Construct a DirectedOfferCancelBuilder from an existing STTx object.
     * @param tx The existing transaction to copy from.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    DirectedOfferCancelBuilder(std::shared_ptr<STTx const> tx)
    {
        if (tx->getTxnType() != ttDIRECTED_OFFER_CANCEL)
        {
            throw std::runtime_error("Invalid transaction type for DirectedOfferCancelBuilder");
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
    DirectedOfferCancelBuilder&
    setDirectedOfferID(std::decay_t<typename SF_UINT256::type::value_type> const& value)
    {
        object_[sfDirectedOfferID] = value;
        return *this;
    }

    /**
     * @brief Build and return the DirectedOfferCancel wrapper.
     * @param publicKey The public key for signing.
     * @param secretKey The secret key for signing.
     * @return The constructed transaction wrapper.
     */
    DirectedOfferCancel
    build(PublicKey const& publicKey, SecretKey const& secretKey)
    {
        sign(publicKey, secretKey);
        return DirectedOfferCancel{std::make_shared<STTx>(std::move(object_))};
    }
};

}  // namespace xrpl::transactions
