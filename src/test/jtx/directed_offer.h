#pragma once

#include <test/jtx/Account.h>
#include <test/jtx/Env.h>

#include <xrpl/basics/base_uint.h>
#include <xrpl/json/json_value.h>
#include <xrpl/protocol/Asset.h>
#include <xrpl/protocol/Keylet.h>
#include <xrpl/protocol/STAmount.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace xrpl::test::jtx {

/**
 * DirectedOffer operations.
 */
namespace directed_offer {

/**
 * One price tier: from MinQuantity of the sold asset, UnitPrice per unit.
 */
struct Tier
{
    STAmount minQuantity;
    STAmount unitPrice;
};

/**
 * Lock @p amount and offer it at @p tiers, paid in @p paymentAsset.
 */
json::Value
create(
    jtx::Account const& seller,
    STAmount const& amount,
    Asset const& paymentAsset,
    std::vector<Tier> const& tiers);

/**
 * Buy @p quantity from the offer @p offerID.
 */
json::Value
accept(jtx::Account const& buyer, UInt256 const& offerID, STAmount const& quantity);

/**
 * Cancel the offer @p offerID.
 */
json::Value
cancel(jtx::Account const& account, UInt256 const& offerID);

/**
 * The keylet of the offer @p seller creates with sequence @p seq.
 */
Keylet
keylet(jtx::Account const& seller, std::uint32_t seq);

/**
 * Sets Counterparties on a create.
 */
class Counterparties
{
private:
    std::vector<jtx::Account> accounts_;

public:
    explicit Counterparties(std::vector<jtx::Account> accounts) : accounts_(std::move(accounts))
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

/**
 * Sets DomainID on a create.
 */
class InDomain
{
private:
    UInt256 domainID_;

public:
    explicit InDomain(UInt256 const& domainID) : domainID_(domainID)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

/**
 * Sets Expiration on a create.
 */
class Expires
{
private:
    NetClock::time_point expiration_;

public:
    explicit Expires(NetClock::time_point expiration) : expiration_(expiration)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

/**
 * Sets SendMax on an accept.
 */
class PayAtMost
{
private:
    STAmount sendMax_;

public:
    explicit PayAtMost(STAmount sendMax) : sendMax_(std::move(sendMax))
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

}  // namespace directed_offer

}  // namespace xrpl::test::jtx
