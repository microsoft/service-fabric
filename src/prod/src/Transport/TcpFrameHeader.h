// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class TcpFrameHeader
    {
    public:
        TcpFrameHeader();
        TcpFrameHeader(MessageUPtr const & message, byte securityProviderMask);

        uint32 FrameLength() const;
        uint16 HeaderLength() const;
        byte SecurityProviderMask() const;
        bool IsValid() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        static constexpr size_t FrameSizeHardLimit() { return std::numeric_limits<decltype(frameLength_)>::max(); }

    private:
        uint32 frameLength_;
        byte securityProviderMask_;
        byte reserved2_;
        uint16 headerLength_;
        uint32 reserved3_;
    };
}
