#include <test/jtx/directed_offer.h>

#include <test/jtx/Account.h>

#include <xrpl/basics/base_uint.h>
#include <xrpl/json/json_value.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STAmount.h>
#include <xrpl/protocol/STIssue.h>
#include <xrpl/protocol/jss.h>

namespace xrpl::test::jtx::directed_offer {

json::Value
create(
    jtx::Account const& seller,
    STAmount const& amount,
    Asset const& paymentAsset,
    std::vector<Tier> const& tiers)
{
    json::Value jv;
    jv[sfTransactionType.jsonName] = "DirectedOfferCreate";
    jv[sfAccount.jsonName] = seller.human();
    jv[sfAmount.jsonName] = amount.getJson(JsonOptions::Values::None);
    jv[sfPaymentAsset.jsonName] =
        STIssue(sfPaymentAsset, paymentAsset).getJson(JsonOptions::Values::None);

    json::Value priceTiers{json::ValueType::Array};
    for (auto const& tier : tiers)
    {
        json::Value entry;
        entry[sfMinQuantity.jsonName] = tier.minQuantity.getJson(JsonOptions::Values::None);
        entry[sfUnitPrice.jsonName] = tier.unitPrice.getJson(JsonOptions::Values::None);
        json::Value wrapper;
        wrapper[sfPriceTier.jsonName] = entry;
        priceTiers.append(wrapper);
    }
    jv[sfPriceTiers.jsonName] = priceTiers;
    return jv;
}

json::Value
accept(jtx::Account const& buyer, UInt256 const& offerID, STAmount const& quantity)
{
    json::Value jv;
    jv[sfTransactionType.jsonName] = "DirectedOfferAccept";
    jv[sfAccount.jsonName] = buyer.human();
    jv[sfDirectedOfferID.jsonName] = to_string(offerID);
    jv[sfQuantity.jsonName] = quantity.getJson(JsonOptions::Values::None);
    return jv;
}

json::Value
cancel(jtx::Account const& account, UInt256 const& offerID)
{
    json::Value jv;
    jv[sfTransactionType.jsonName] = "DirectedOfferCancel";
    jv[sfAccount.jsonName] = account.human();
    jv[sfDirectedOfferID.jsonName] = to_string(offerID);
    return jv;
}

json::Value
counter(
    jtx::Account const& buyer,
    UInt256 const& offerID,
    STAmount const& quantity,
    STAmount const& amount)
{
    json::Value jv;
    jv[sfTransactionType.jsonName] = "DirectedOfferCounterCreate";
    jv[sfAccount.jsonName] = buyer.human();
    jv[sfDirectedOfferID.jsonName] = to_string(offerID);
    jv[sfQuantity.jsonName] = quantity.getJson(JsonOptions::Values::None);
    jv[sfAmount.jsonName] = amount.getJson(JsonOptions::Values::None);
    return jv;
}

json::Value
takeCounter(jtx::Account const& seller, UInt256 const& counterID)
{
    json::Value jv;
    jv[sfTransactionType.jsonName] = "DirectedOfferAccept";
    jv[sfAccount.jsonName] = seller.human();
    jv[sfDirectedOfferCounterID.jsonName] = to_string(counterID);
    return jv;
}

json::Value
cancelCounter(jtx::Account const& account, UInt256 const& counterID)
{
    json::Value jv;
    jv[sfTransactionType.jsonName] = "DirectedOfferCancel";
    jv[sfAccount.jsonName] = account.human();
    jv[sfDirectedOfferCounterID.jsonName] = to_string(counterID);
    return jv;
}

Keylet
keylet(jtx::Account const& seller, std::uint32_t seq)
{
    return keylet::directedOffer(seller.id(), SeqProxy::rawSequence(seq));
}

Keylet
counterKeylet(jtx::Account const& buyer, std::uint32_t seq)
{
    return keylet::directedOfferCounter(buyer.id(), SeqProxy::rawSequence(seq));
}

void
Counterparties::operator()(Env&, JTx& jt) const
{
    json::Value counterparties{json::ValueType::Array};
    for (auto const& account : accounts_)
    {
        json::Value entry;
        entry[sfAccount.jsonName] = account.human();
        json::Value wrapper;
        wrapper[sfCounterpartyEntry.jsonName] = entry;
        counterparties.append(wrapper);
    }
    jt.jv[sfCounterparties.jsonName] = counterparties;
}

void
InDomain::operator()(Env&, JTx& jt) const
{
    jt.jv[sfDomainID.jsonName] = to_string(domainID_);
}

void
Expires::operator()(Env&, JTx& jt) const
{
    jt.jv[sfExpiration.jsonName] = expiration_.time_since_epoch().count();
}

void
PayAtMost::operator()(Env&, JTx& jt) const
{
    jt.jv[sfSendMax.jsonName] = sendMax_.getJson(JsonOptions::Values::None);
}

}  // namespace xrpl::test::jtx::directed_offer
