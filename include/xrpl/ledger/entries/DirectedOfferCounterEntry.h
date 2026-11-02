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
class DirectedOfferCounterEntry : public SLEBase<ViewT, ltDIRECTED_OFFER_COUNTER>
{
public:
    using Base = SLEBase<ViewT, ltDIRECTED_OFFER_COUNTER>;

    // Inherit base constructors: adopt an existing SLE, or resolve one from a
    // Keylet against the view.
    using Base::Base;

    explicit DirectedOfferCounterEntry(
        AccountID const& id,
        SeqProxy const& seq,
        Base::ViewRefType view,
        beast::Journal j = beast::Journal{beast::Journal::getNullSink()})
        : Base(keylet::directedOfferCounter(id, seq), view, j)
    {
    }

    explicit DirectedOfferCounterEntry(
        UInt256 const& counterID,
        Base::ViewRefType view,
        beast::Journal j = beast::Journal{beast::Journal::getNullSink()})
        : Base(keylet::directedOfferCounter(counterID), view, j)
    {
    }
};

using DirectedOfferCounterEntryR = DirectedOfferCounterEntry<ReadView>;
using DirectedOfferCounterEntryW = DirectedOfferCounterEntry<ApplyView>;

}  // namespace xrpl
