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

class DirectedOfferBuilder;

/**
 * @brief Ledger Entry: DirectedOffer
 *
 * Type: ltDIRECTED_OFFER (0x00a0)
 * RPC Name: directed_offer
 *
 * Immutable wrapper around SLE providing type-safe field access.
 * Use DirectedOfferBuilder to construct new ledger entries.
 */
class DirectedOffer : public LedgerEntryBase
{
public:
    static constexpr LedgerEntryType entryType = ltDIRECTED_OFFER;

    /**
     * @brief Construct a DirectedOffer ledger entry wrapper from an existing SLE object.
     * @throws std::runtime_error if the ledger entry type doesn't match.
     */
    explicit DirectedOffer(SLE::const_pointer sle)
        : LedgerEntryBase(std::move(sle))
    {
        // Verify ledger entry type
        if (sle_->getType() != entryType)
        {
            throw std::runtime_error("Invalid ledger entry type for DirectedOffer");
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
     * @brief Get sfPaymentAsset (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_ISSUE::type::value_type
    getPaymentAsset() const
    {
        return this->sle_->at(sfPaymentAsset);
    }

    /**
     * @brief Get sfPriceTiers (SoeRequired)
     * @note This is an untyped field (unknown).
     * @return The field value.
     */
    [[nodiscard]]
    STArray const&
    getPriceTiers() const
    {
        return this->sle_->getFieldArray(sfPriceTiers);
    }

    /**
     * @brief Get sfCounterparties (SoeOptional)
     * @note This is an untyped field (unknown).
     * @return The field value, or std::nullopt if not present.
     */
    [[nodiscard]]
    std::optional<std::reference_wrapper<STArray const>>
    getCounterparties() const
    {
        if (this->sle_->isFieldPresent(sfCounterparties))
            return this->sle_->getFieldArray(sfCounterparties);
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
        return this->sle_->isFieldPresent(sfCounterparties);
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
            return this->sle_->at(sfDomainID);
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
        return this->sle_->isFieldPresent(sfDomainID);
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
 * @brief Builder for DirectedOffer ledger entries.
 *
 * Provides a fluent interface for constructing ledger entries with method chaining.
 * Uses STObject internally for flexible ledger entry construction.
 * Inherits common field setters from LedgerEntryBuilderBase.
 */
class DirectedOfferBuilder : public LedgerEntryBuilderBase<DirectedOfferBuilder>
{
public:
    /**
     * @brief Construct a new DirectedOfferBuilder with required fields.
     * @param account The sfAccount field value.
     * @param sequence The sfSequence field value.
     * @param amount The sfAmount field value.
     * @param paymentAsset The sfPaymentAsset field value.
     * @param priceTiers The sfPriceTiers field value.
     * @param ownerNode The sfOwnerNode field value.
     * @param previousTxnID The sfPreviousTxnID field value.
     * @param previousTxnLgrSeq The sfPreviousTxnLgrSeq field value.
     */
    DirectedOfferBuilder(std::decay_t<typename SF_ACCOUNT::type::value_type> const& account,std::decay_t<typename SF_UINT32::type::value_type> const& sequence,std::decay_t<typename SF_AMOUNT::type::value_type> const& amount,std::decay_t<typename SF_ISSUE::type::value_type> const& paymentAsset,STArray const& priceTiers,std::decay_t<typename SF_UINT64::type::value_type> const& ownerNode,std::decay_t<typename SF_UINT256::type::value_type> const& previousTxnID,std::decay_t<typename SF_UINT32::type::value_type> const& previousTxnLgrSeq)
        : LedgerEntryBuilderBase<DirectedOfferBuilder>(ltDIRECTED_OFFER)
    {
        setAccount(account);
        setSequence(sequence);
        setAmount(amount);
        setPaymentAsset(paymentAsset);
        setPriceTiers(priceTiers);
        setOwnerNode(ownerNode);
        setPreviousTxnID(previousTxnID);
        setPreviousTxnLgrSeq(previousTxnLgrSeq);
    }

    /**
     * @brief Construct a DirectedOfferBuilder from an existing SLE object.
     * @param sle The existing ledger entry to copy from.
     * @throws std::runtime_error if the ledger entry type doesn't match.
     */
    DirectedOfferBuilder(SLE::const_pointer sle)
    {
        if (sle->at(sfLedgerEntryType) != ltDIRECTED_OFFER)
        {
            throw std::runtime_error("Invalid ledger entry type for DirectedOffer");
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
    DirectedOfferBuilder&
    setAccount(std::decay_t<typename SF_ACCOUNT::type::value_type> const& value)
    {
        object_[sfAccount] = value;
        return *this;
    }

    /**
     * @brief Set sfSequence (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setSequence(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfSequence] = value;
        return *this;
    }

    /**
     * @brief Set sfAmount (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setAmount(std::decay_t<typename SF_AMOUNT::type::value_type> const& value)
    {
        object_[sfAmount] = value;
        return *this;
    }

    /**
     * @brief Set sfPaymentAsset (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setPaymentAsset(std::decay_t<typename SF_ISSUE::type::value_type> const& value)
    {
        object_[sfPaymentAsset] = STIssue(sfPaymentAsset, value);
        return *this;
    }

    /**
     * @brief Set sfPriceTiers (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setPriceTiers(STArray const& value)
    {
        object_.setFieldArray(sfPriceTiers, value);
        return *this;
    }

    /**
     * @brief Set sfCounterparties (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setCounterparties(STArray const& value)
    {
        object_.setFieldArray(sfCounterparties, value);
        return *this;
    }

    /**
     * @brief Set sfDomainID (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setDomainID(std::decay_t<typename SF_UINT256::type::value_type> const& value)
    {
        object_[sfDomainID] = value;
        return *this;
    }

    /**
     * @brief Set sfExpiration (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setExpiration(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfExpiration] = value;
        return *this;
    }

    /**
     * @brief Set sfTransferRate (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setTransferRate(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfTransferRate] = value;
        return *this;
    }

    /**
     * @brief Set sfOwnerNode (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setOwnerNode(std::decay_t<typename SF_UINT64::type::value_type> const& value)
    {
        object_[sfOwnerNode] = value;
        return *this;
    }

    /**
     * @brief Set sfIssuerNode (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setIssuerNode(std::decay_t<typename SF_UINT64::type::value_type> const& value)
    {
        object_[sfIssuerNode] = value;
        return *this;
    }

    /**
     * @brief Set sfPreviousTxnID (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setPreviousTxnID(std::decay_t<typename SF_UINT256::type::value_type> const& value)
    {
        object_[sfPreviousTxnID] = value;
        return *this;
    }

    /**
     * @brief Set sfPreviousTxnLgrSeq (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    DirectedOfferBuilder&
    setPreviousTxnLgrSeq(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfPreviousTxnLgrSeq] = value;
        return *this;
    }

    /**
     * @brief Build and return the completed DirectedOffer wrapper.
     * @param index The ledger entry index.
     * @return The constructed ledger entry wrapper.
     */
    DirectedOffer
    build(UInt256 const& index)
    {
        return DirectedOffer{std::make_shared<SLE>(std::move(object_), index)};
    }
};

}  // namespace xrpl::ledger_entries
