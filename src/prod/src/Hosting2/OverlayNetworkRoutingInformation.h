// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class OverlayNetworkRoutingInformation : 
        public Common::IFabricJsonSerializable,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;

    public:
        OverlayNetworkRoutingInformation(
            std::wstring networkName,
            std::wstring nodeIpAddress,
            std::wstring instanceID,
            int64 sequenceNumber,
            bool isDelta,
            std::vector<OverlayNetworkRouteSPtr> overlayNetworkRoutes);

        ~OverlayNetworkRoutingInformation();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkRoutingInformation::NetworkNameParameter, networkName_, !networkName_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkRoutingInformation::NodeIpAddressParameter, nodeIpAddress_, !nodeIpAddress_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkRoutingInformation::InstanceIDParameter, instanceID_, !instanceID_.empty())
            SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(OverlayNetworkRoutingInformation::SequenceNumberParameter, sequenceNumber_, true)
            SERIALIZABLE_PROPERTY(OverlayNetworkRoutingInformation::IsDeltaParameter, isDelta_)
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkRoutingInformation::NetworkDefinitionParameter, overlayNetworkDefinition_, overlayNetworkDefinition_ != nullptr)
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkRoutingInformation::OverlayNetworkRoutesParameter, overlayNetworkRoutes_, overlayNetworkRoutes_.size() > 0)
        END_JSON_SERIALIZABLE_PROPERTIES()

        void OverlayNetworkRoutingInformation::WriteTo(TextWriter & w, FormatOptions const &) const;

        __declspec(property(get = get_NetworkName)) std::wstring const & NetworkName;
        std::wstring const & get_NetworkName() const { return networkName_; }

        __declspec(property(get = get_NodeIpAddress)) std::wstring const & NodeIpAddress;
        std::wstring const & get_NodeIpAddress() const { return nodeIpAddress_; }

        __declspec(property(get = get_SequenceNumber)) int64 SequenceNumber;
        int64 get_SequenceNumber() const { return sequenceNumber_; }

        __declspec(property(get = get_IsDelta)) bool IsDelta;
        bool get_IsDelta() const { return isDelta_; }

        __declspec(property(get = get_OverlayNetworkDefinition, put = set_OverlayNetworkDefinition)) OverlayNetworkDefinitionSPtr const & OverlayNetworkDefinition;
        OverlayNetworkDefinitionSPtr const & get_OverlayNetworkDefinition() const { return overlayNetworkDefinition_; }
        inline void set_OverlayNetworkDefinition(OverlayNetworkDefinitionSPtr value) { overlayNetworkDefinition_ = value; }

        __declspec(property(get = get_OverlayNetworkRoutes)) std::vector<OverlayNetworkRouteSPtr> const & OverlayNetworkRoutes;
        std::vector<OverlayNetworkRouteSPtr> const & get_OverlayNetworkRoutes() const { return overlayNetworkRoutes_; }

    private:
        std::wstring networkName_;
        std::wstring nodeIpAddress_;
        std::wstring instanceID_;
        int64 sequenceNumber_;
        bool isDelta_;
        OverlayNetworkDefinitionSPtr overlayNetworkDefinition_;
        std::vector<OverlayNetworkRouteSPtr> overlayNetworkRoutes_;

        static WStringLiteral const NetworkNameParameter;
        static WStringLiteral const NetworkDefinitionParameter;
        static WStringLiteral const NodeIpAddressParameter;
        static WStringLiteral const InstanceIDParameter;
        static WStringLiteral const SequenceNumberParameter;
        static WStringLiteral const IsDeltaParameter;
        static WStringLiteral const OverlayNetworkRoutesParameter;
    };
}
