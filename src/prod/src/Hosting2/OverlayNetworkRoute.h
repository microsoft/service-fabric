// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class OverlayNetworkRoute : 
        public Common::IFabricJsonSerializable,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;

    public:
        OverlayNetworkRoute();

        OverlayNetworkRoute(
            std::wstring ipAddress,
            std::wstring macAddress,
            std::wstring nodeIpAddress);

        ~OverlayNetworkRoute();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkRoute::IPV4Address, ipAddress_, !ipAddress_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkRoute::MacAddress, macAddress_, !macAddress_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkRoute::NodeIpAddress, nodeIpAddress_, !nodeIpAddress_.empty())
        END_JSON_SERIALIZABLE_PROPERTIES()

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        std::wstring ipAddress_;
        std::wstring macAddress_;
        std::wstring nodeIpAddress_;

        static WStringLiteral const IPV4Address;
        static WStringLiteral const MacAddress;
        static WStringLiteral const NodeIpAddress;
    };
}
