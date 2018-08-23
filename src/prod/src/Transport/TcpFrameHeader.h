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

        bool Validate(bool firstFrame, byte expectedSecurityProviderMask, std::wstring const & traceId) const;

        uint32 FrameLength() const;
        uint16 HeaderLength() const;
        byte SecurityProviderMask() const;

        byte FrameHeaderCRC() const { return frameHeaderCRC_; }
        byte SetFrameHeaderCRC();

        uint32 FrameBodyCRC() const { return frameBodyCRC_; }
        void SetFrameBodyCRC(uint32 crc) { frameBodyCRC_ = crc; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        static constexpr size_t FrameSizeHardLimit() { return std::numeric_limits<decltype(frameLength_)>::max(); }

    private:
        uint32 frameLength_;
        byte securityProviderMask_;
        byte frameHeaderCRC_= 0;
        uint16 headerLength_;
        uint32 frameBodyCRC_ = 0;
    };
}
