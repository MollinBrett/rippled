#pragma once

#include <xrpl/basics/base_uint.h>
#include <xrpl/beast/utility/Journal.h>
#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/ReadView.h>
#include <xrpl/ledger/entries/SLEBase.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/LedgerFormats.h>
#include <xrpl/protocol/SeqProxy.h>

namespace xrpl {

template <typename ViewT>
class DirectedOfferEntry : public SLEBase<ViewT, ltDIRECTED_OFFER>
{
public:
    using Base = SLEBase<ViewT, ltDIRECTED_OFFER>;

    // Inherit base constructors: adopt an existing SLE, or resolve one from a
    // Keylet against the view.
    using Base::Base;

    explicit DirectedOfferEntry(
        AccountID const& id,
        SeqProxy const& seq,
        Base::ViewRefType view,
        beast::Journal j = beast::Journal{beast::Journal::getNullSink()})
        : Base(keylet::directedOffer(id, seq), view, j)
    {
    }

    explicit DirectedOfferEntry(
        UInt256 const& offerID,
        Base::ViewRefType view,
        beast::Journal j = beast::Journal{beast::Journal::getNullSink()})
        : Base(keylet::directedOffer(offerID), view, j)
    {
    }
};

using DirectedOfferEntryR = DirectedOfferEntry<ReadView>;
using DirectedOfferEntryW = DirectedOfferEntry<ApplyView>;

}  // namespace xrpl
