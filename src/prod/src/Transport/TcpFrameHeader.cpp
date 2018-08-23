// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define TRACE_FMT "length={0:x},SecurityProviderMask={1:x},r2={2:x},header={3:x},r3={4:x}"

namespace Transport
{
    static Common::StringLiteral const TraceType("FrameHeader");

    TcpFrameHeader::TcpFrameHeader() : frameLength_(0), securityProviderMask_(SecurityProvider::None), headerLength_(0)
    {
        static_assert(
            sizeof(TcpFrameHeader) == (sizeof(uint32)+sizeof(uint16)+sizeof(uint16)+sizeof(uint32)),
            "unepxected TcpFrameHeader size");
    }

    TcpFrameHeader::TcpFrameHeader(MessageUPtr const & message, byte securityProviderMask)
        : securityProviderMask_(securityProviderMask)
        , headerLength_((uint16)message->SerializedHeaderSize())
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

    byte TcpFrameHeader::SetFrameHeaderCRC()
    {
        auto copy = *this;
        copy.frameHeaderCRC_ = 0;
        copy.frameBodyCRC_ = 0;

        Common::crc8 crc(&copy, sizeof(copy));
        frameHeaderCRC_ = crc.Value();
        return frameHeaderCRC_;
    }

    bool TcpFrameHeader::Validate(bool firstFrame, byte expectedSecurityProviderMask, std::wstring const & traceId) const
    {
        if (FrameHeaderCRC())
        {
            auto frameHeaderCopy = *this;
            auto computedFrameHeaderCRC = frameHeaderCopy.SetFrameHeaderCRC();
            if (computedFrameHeaderCRC != FrameHeaderCRC())
            {
                if (firstFrame)
                {
                    textTrace.WriteInfo(TraceType, "ignore message from non-fabric endpoint");
                    return false;
                }

                textTrace.WriteError(
                        TraceType, traceId,
                        "frame header CRC verification failure: incoming = 0x{0:x}, computed = 0x{1:x}, currentFrame: {2}",
                        FrameHeaderCRC(), computedFrameHeaderCRC, *this);

                return false;
            }
        }

        //
        // more validations in case CRC missed something
        //
        if (((uint32)sizeof(TcpFrameHeader) + headerLength_) > frameLength_)
        {
            if (firstFrame)
            {
                textTrace.WriteInfo(TraceType, "ignore message from non-fabric endpoint: {0}", *this);
                return false;
            }

            textTrace.WriteError(TraceType, traceId, "invalid lengths: {0}", *this); 
            return false;
        }

        if (!firstFrame && (securityProviderMask_ != expectedSecurityProviderMask))
        {
            textTrace.WriteError(
                TraceType, traceId,
                "SecurityProviderMask mismatch: expected: {0}, currentFrame: {1}",
                expectedSecurityProviderMask,
                *this);

            return false;
        }

#if DBG
        if (FrameHeaderCRC())
        {
            textTrace.WriteNoise(TraceType, traceId, "frame header CRC verified");
        }
#endif
        return true;
    }

    void TcpFrameHeader::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w.Write("len={0:x},sp=", frameLength_);
        SecurityProvider::WriteMaskToTextWriter(w, securityProviderMask_);
        w.Write(",fhCRC={0:x},mhLen={1:x},fbCRC={2:x}", frameHeaderCRC_, headerLength_, frameBodyCRC_);
    }

    std::string TcpFrameHeader::AddField(Common::TraceEvent & traceEvent, std::string const & name)
    {
        traceEvent.AddField<uint32>(name + ".len");
        traceEvent.AddField<byte>(name + ".sp");
        traceEvent.AddField<byte>(name + ".fhCRC");
        traceEvent.AddField<uint16>(name + ".msLen");
        traceEvent.AddField<uint32>(name + ".fbCRC");

        return TRACE_FMT;
    }

    void TcpFrameHeader::FillEventData(Common::TraceEventContext & context) const
    {
        context.Write<uint32>(frameLength_);
        context.Write<byte>(securityProviderMask_);
        context.Write<byte>(frameHeaderCRC_);
        context.Write<uint16>(headerLength_);
        context.Write<uint32>(frameBodyCRC_);
    }
}
