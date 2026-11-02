// This file is auto-generated. Do not edit.
#pragma once

#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol/STParsedJSON.h>
#include <xrpl/protocol/jss.h>
#include <xrpl/protocol_autogen/LedgerEntryBase.h>
#include <xrpl/protocol_autogen/LedgerEntryBuilderBase.h>
#include <xrpl/json/json_value.h>

#include <stdexcept>
#include <optional>

namespace xrpl::ledger_entries {

class DirectedOfferCounterBuilder;

/**
 * @brief Ledger Entry: DirectedOfferCounter
 *
 * Type: ltDIRECTED_OFFER_COUNTER (0x00a1)
 * RPC Name: directed_offer_counter
 *
 * Immutable wrapper around SLE providing type-safe field access.
 * Use DirectedOfferCounterBuilder to construct new ledger entries.
 */
class DirectedOfferCounter : public LedgerEntryBase
{
public:
    static constexpr LedgerEntryType entryType = ltDIRECTED_OFFER_COUNTER;

    /**
     * @brief Construct a DirectedOfferCounter ledger entry wrapper from an existing SLE object.
     * @throws std::runtime_error if the ledger entry type doesn't match.
     */
    explicit DirectedOfferCounter(SLE::const_pointer sle)
        : LedgerEntryBase(std::move(sle))
    {
        // Verify ledger entry type
        if (sle_->getType() != entryType)
        {
            throw std::runtime_error("Invalid ledger entry type for DirectedOfferCounter");
        }
    }

    // Ledger entry-specific field getters

    /**
     * @brief Get sfAccount (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_ACCOUNT::type::value_type
    getAccount() const
    {
        return this->sle_->at(sfAccount);
    }

    /**
     * @brief Get sfDestination (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_ACCOUNT::type::value_type
    getDestination() const
    {
        return this->sle_->at(sfDestination);
    }

    /**
     * @brief Get sfSequence (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_UINT32::type::value_type
    getSequence() const
    {
        return this->sle_->at(sfSequence);
    }

    /**
     * @brief Get sfDirectedOfferID (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_UINT256::type::value_type
    getDirectedOfferID() const
    {
        return this->sle_->at(sfDirectedOfferID);
    }

    /**
     * @brief Get sfQuantity (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_AMOUNT::type::value_type
    getQuantity() const
    {
        return this->sle_->at(sfQuantity);
    }

    /**
     * @brief Get sfAmount (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_AMOUNT::type::value_type
    getAmount() const
    {
        return this->sle_->at(sfAmount);
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
            return this->sle_->at(sfExpiration);
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
        return this->sle_->isFieldPresent(sfExpiration);
    }

    /**
     * @brief Get sfTransferRate (SoeOptional)
     * @return The field value, or std::nullopt if not present.
     */
    [[nodiscard]]
    protocol_autogen::Optional<SF_UINT32::type::value_type>
    getTransferRate() const
    {
        if (hasTransferRate())
            return this->sle_->at(sfTransferRate);
        return std::nullopt;
    }

    /**
     * @brief Check if sfTransferRate is present.
     * @return True if the field is present, false otherwise.
     */
    [[nodiscard]]
    bool
    hasTransferRate() const
    {
        return this->sle_->isFieldPresent(sfTransferRate);
    }

    /**
     * @brief Get sfOwnerNode (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_UINT64::type::value_type
    getOwnerNode() const
    {
        return this->sle_->at(sfOwnerNode);
    }

    /**
     * @brief Get sfDestinationNode (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_UINT64::type::value_type
    getDestinationNode() const
    {
        return this->sle_->at(sfDestinationNode);
    }

    /**
     * @brief Get sfIssuerNode (SoeOptional)
     * @return The field value, or std::nullopt if not present.
     */
    [[nodiscard]]
    protocol_autogen::Optional<SF_UINT64::type::value_type>
    getIssuerNode() const
    {
        if (hasIssuerNode())
            return this->sle_->at(sfIssuerNode);
        return std::nullopt;
    }

    /**
     * @brief Check if sfIssuerNode is present.
     * @return True if the field is present, false otherwise.
     */
    [[nodiscard]]
    bool
    hasIssuerNode() const
    {
        return this->sle_->isFieldPresent(sfIssuerNode);
    }

    /**
     * @brief Get sfPreviousTxnID (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_UINT256::type::value_type
    getPreviousTxnID() const
    {
        return this->sle_->at(sfPreviousTxnID);
    }

    /**
     * @brief Get sfPreviousTxnLgrSeq (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_UINT32::type::value_type
    getPreviousTxnLgrSeq() const
    {
        return this->sle_->at(sfPreviousTxnLgrSeq);
    }
};

/**
 * @brief Builder for DirectedOfferCounter ledger entries.
 *
 * Provides a fluent interface for constructing ledger entries with method chaining.
 * Uses STObject internally for flexible ledger entry construction.
 * Inherits common field setters from LedgerEntryBuilderBase.
 */
class DirectedOfferCounterBuilder : public LedgerEntryBuilderBase<DirectedOfferCounterBuilder>
{
public:
    /**
     * @brief Construct a new DirectedOfferCounterBuilder with required fields.
     * @param account The sfAccount field value.
     * @param destination The sfDestination field value.
     * @param sequence The sfSequence field value.
     * @param directedOfferID The sfDirectedOfferID field value.
     * @param quantity The sfQuantity field value.
     * @param amount The sfAmount field value.
     * @param ownerNode The sfOwnerNode field value.
     * @param destinationNode The sfDestinationNode field value.
     * @param previousTxnID The sfPreviousTxnID field value.
     * @param previousTxnLgrSeq The sfPreviousTxnLgrSeq field value.
     */
    DirectedOfferCounterBuilder(std::decay_t<typename SF_ACCOUNT::type::value_type> const& account,std::decay_t<typename SF_ACCOUNT::type::value_type> const& destination,std::decay_t<typename SF_UINT32::type::value_type> const& sequence,std::decay_t<typename SF_UINT256::type::value_type> const& directedOfferID,std::decay_t<typename SF_AMOUNT::type::value_type> const& quantity,std::decay_t<typename SF_AMOUNT::type::value_type> const& amount,std::decay_t<typename SF_UINT64::type::value_type> const& ownerNode,std::decay_t<typename SF_UINT64::type::value_type> const& destinationNode,std::decay_t<typename SF_UINT256::type::value_type> const& previousTxnID,std::decay_t<typename SF_UINT32::type::value_type> const& previousTxnLgrSeq)
        : LedgerEntryBuilderBase<DirectedOfferCounterBuilder>(ltDIRECTED_OFFER_COUNTER)
    {
        setAccount(account);
        setDestination(destination);
        setSequence(sequence);
        setDirectedOfferID(directedOfferID);
        setQuantity(quantity);
        setAmount(amount);
        setOwnerNode(ownerNode);
        setDestinationNode(destinationNode);
        setPreviousTxnID(previousTxnID);
        setPreviousTxnLgrSeq(previousTxnLgrSeq);
    }

    /**
     * @brief Construct a DirectedOfferCounterBuilder from an existing SLE object.
     * @param sle The existing ledger entry to copy from.
     * @throws std::runtime_error if the ledger entry type doesn't match.
     */
    DirectedOfferCounterBuilder(SLE::const_pointer sle)
    {
        if (sle->at(sfLedgerEntryType) != ltDIRECTED_OFFER_COUNTER)
        {
            throw std::runtime_error("Invalid ledger entry type for DirectedOfferCounter");
        }
        object_ = *sle;
    }

    /**
     * @brief Ledger entry-specific field setters
     */

    /**
     * @brief Set sfAccount (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setAccount(std::decay_t<typename SF_ACCOUNT::type::value_type> const& value)
    {
        object_[sfAccount] = value;
        return *this;
    }

    /**
     * @brief Set sfDestination (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setDestination(std::decay_t<typename SF_ACCOUNT::type::value_type> const& value)
    {
        object_[sfDestination] = value;
        return *this;
    }

    /**
     * @brief Set sfSequence (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setSequence(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfSequence] = value;
        return *this;
    }

    /**
     * @brief Set sfDirectedOfferID (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setDirectedOfferID(std::decay_t<typename SF_UINT256::type::value_type> const& value)
    {
        object_[sfDirectedOfferID] = value;
        return *this;
    }

    /**
     * @brief Set sfQuantity (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setQuantity(std::decay_t<typename SF_AMOUNT::type::value_type> const& value)
    {
        object_[sfQuantity] = value;
        return *this;
    }

    /**
     * @brief Set sfAmount (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setAmount(std::decay_t<typename SF_AMOUNT::type::value_type> const& value)
    {
        object_[sfAmount] = value;
        return *this;
    }

    /**
     * @brief Set sfExpiration (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setExpiration(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfExpiration] = value;
        return *this;
    }

    /**
     * @brief Set sfTransferRate (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setTransferRate(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfTransferRate] = value;
        return *this;
    }

    /**
     * @brief Set sfOwnerNode (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setOwnerNode(std::decay_t<typename SF_UINT64::type::value_type> const& value)
    {
        object_[sfOwnerNode] = value;
        return *this;
    }

    /**
     * @brief Set sfDestinationNode (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setDestinationNode(std::decay_t<typename SF_UINT64::type::value_type> const& value)
    {
        object_[sfDestinationNode] = value;
        return *this;
    }

    /**
     * @brief Set sfIssuerNode (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setIssuerNode(std::decay_t<typename SF_UINT64::type::value_type> const& value)
    {
        object_[sfIssuerNode] = value;
        return *this;
    }

    /**
     * @brief Set sfPreviousTxnID (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setPreviousTxnID(std::decay_t<typename SF_UINT256::type::value_type> const& value)
    {
        object_[sfPreviousTxnID] = value;
        return *this;
    }

    /**
     * @brief Set sfPreviousTxnLgrSeq (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferCounterBuilder&
    setPreviousTxnLgrSeq(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfPreviousTxnLgrSeq] = value;
        return *this;
    }

    /**
     * @brief Build and return the completed DirectedOfferCounter wrapper.
     * @param index The ledger entry index.
     * @return The constructed ledger entry wrapper.
     */
    DirectedOfferCounter
    build(UInt256 const& index)
    {
        return DirectedOfferCounter{std::make_shared<SLE>(std::move(object_), index)};
    }
};

}  // namespace xrpl::ledger_entries
