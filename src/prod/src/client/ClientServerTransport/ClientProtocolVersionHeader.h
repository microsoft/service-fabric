// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class ClientProtocolVersionHeader : 
        public Transport::MessageHeader<Transport::MessageHeaderId::ClientProtocolVersion>, 
        public Serialization::FabricSerializable
    {
    public:

        static const __int64 CurrentMajorVersion = 1;
        static const __int64 CurrentMinorVersion = 1;

        static Common::Global<ClientProtocolVersionHeader> CurrentVersionHeader;

        // History:
        //
        // 1 (WF-v1.0)   - Gateway only accepts version = 1 clients
        // 1.1 (WF-v3.0) - Introduced PingReplyMessageBody
        //               - Introduced Minor version field

    public:
        ClientProtocolVersionHeader() 
            : major_(0)
            , minor_(0)
        { }

        explicit ClientProtocolVersionHeader(__int64 major, __int64 minor) 
            : major_(major) 
            , minor_(minor)
        { }

        ClientProtocolVersionHeader(ClientProtocolVersionHeader const & other) 
            : major_(other.major_) 
            , minor_(other.minor_) 
        { }
        
        __declspec(property(get=get_Major)) __int64 Major;
        inline __int64 get_Major() const { return major_; }

        __declspec(property(get=get_Minor)) __int64 Minor;
        inline __int64 get_Minor() const { return minor_; }

        static bool IsMinorAtLeast(ClientProtocolVersionHeader const & header, __int64 minorMinimum)
        {
            return (header.Major == CurrentMajorVersion && header.Minor >= minorMinimum);
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const { w << "v-" << major_ << "." << minor_; }

        FABRIC_FIELDS_02(major_, minor_);

    private:
        __int64 major_;
        __int64 minor_;
    };
}
