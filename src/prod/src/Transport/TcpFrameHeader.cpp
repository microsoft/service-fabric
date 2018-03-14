// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define TRACE_FMT "length={0:x},SecurityProviderMask={1:x},r2={2:x},header={3:x},r3={4:x}"

namespace Transport
{
    TcpFrameHeader::TcpFrameHeader() : frameLength_(0), securityProviderMask_(SecurityProvider::None), reserved2_(0), headerLength_(0), reserved3_(0)
    {
        static_assert(
            sizeof(TcpFrameHeader) == (sizeof(uint32)+sizeof(uint16)+sizeof(uint16)+sizeof(uint32)),
            "unepxected TcpFrameHeader size");
    }

    TcpFrameHeader::TcpFrameHeader(MessageUPtr const & message, byte securityProviderMask)
        : securityProviderMask_(securityProviderMask)
        , reserved2_(0)
        , headerLength_((uint16)message->SerializedHeaderSize())
        , reserved3_(0)
    {
        auto status = UIntAdd((uint)sizeof(TcpFrameHeader), message->SerializedSize(), &frameLength_);
        ASSERT_IF(status != S_OK, "frame size overflows: {0:x}", status);
        auto headerSize = message->SerializedHeaderSize();
        auto headerSizeLimit = MessageHeaders::SizeLimitStatic();
        ASSERT_IF(headerSize > headerSizeLimit, "too much header: {0} > {1}", headerSize, headerSizeLimit);
    }

    uint32 TcpFrameHeader::FrameLength() const 
    { 
        return frameLength_; 
    }

    uint16 TcpFrameHeader::HeaderLength() const
    {
        return headerLength_;
    }

    byte TcpFrameHeader::SecurityProviderMask() const
    {
        return securityProviderMask_;
    }

    bool TcpFrameHeader::IsValid() const
    {
        return ((uint32)sizeof(TcpFrameHeader) + headerLength_) <= frameLength_;
    }

    void TcpFrameHeader::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w.Write("length={0:x},SecurityProviderMask=", frameLength_);
        SecurityProvider::WriteMaskToTextWriter(w, securityProviderMask_);
        w.Write(",r2={0:x},header={1:x},r3={2:x}", reserved2_, headerLength_, reserved3_);
    }

    std::string TcpFrameHeader::AddField(Common::TraceEvent & traceEvent, std::string const & name)
    {
        traceEvent.AddField<uint32>(name + ".length");
        traceEvent.AddField<byte>(name + ".securityProviders");
        traceEvent.AddField<byte>(name + ".r2");
        traceEvent.AddField<uint16>(name + ".header");
        traceEvent.AddField<uint32>(name + ".r3");

        return TRACE_FMT;
    }

    void TcpFrameHeader::FillEventData(Common::TraceEventContext & context) const
    {
        context.Write<uint32>(frameLength_);
        context.Write<byte>(securityProviderMask_);
        context.Write<byte>(reserved2_);
        context.Write<uint16>(headerLength_);
        context.Write<uint32>(reserved3_);
    }
}
