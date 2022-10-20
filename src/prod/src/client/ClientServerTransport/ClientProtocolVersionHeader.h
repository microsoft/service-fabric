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
        static const __int64 CurrentMinorVersion = 2;

        static Common::Global<ClientProtocolVersionHeader> CurrentVersionHeader;
        static Common::Global<ClientProtocolVersionHeader> SingleFileUploadVersionHeader;


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


        bool IsChunkedFileUploadSupported() const
        {
            return IsMinorAtLeast(ClientProtocolVersionHeader::MinorVersion_ChunkedFileUploadSupport);
        }

        bool IsPingReplyBodySupported() const
        {
            return IsMinorAtLeast(ClientProtocolVersionHeader::MinorVersion_PingReplyMessageBodySupport);
        }

        bool IsCompatibleVersion() const
        {
            return IsMinorAtLeast(0);
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const { w << "v-" << major_ << "." << minor_; }

        FABRIC_FIELDS_02(major_, minor_);

    private:
        // History:
        //
        // 1 (WF-v1.0)   - Gateway only accepts version = 1 clients
        // 1.1 (WF-v3.0) - Introduced PingReplyMessageBody
        //               - Introduced Minor version field
        // 1.2           - Support for file upload based on chunk APIs.
        
        static const __int64 MinorVersion_PingReplyMessageBodySupport = 1;
        static const __int64 MinorVersion_ChunkedFileUploadSupport = 2;

        bool IsMinorAtLeast(__int64 minorMinimum) const
        {
            return (major_ == CurrentMajorVersion && minor_ >= minorMinimum);
        }


        __int64 major_;
        __int64 minor_;
    };
}
