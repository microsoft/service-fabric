// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ManageOverlayNetworkResourcesRequest : public Serialization::FabricSerializable
    {
    public:
        ManageOverlayNetworkResourcesRequest();
        ManageOverlayNetworkResourcesRequest(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & servicePackageId,
            std::map<std::wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
            ManageOverlayNetworkAction::Enum const action_);

        ~ManageOverlayNetworkResourcesRequest();

        __declspec(property(get = get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get = get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get = get_NodeIpAddress)) std::wstring const & NodeIpAddress;
        std::wstring const & get_NodeIpAddress() const { return nodeIpAddress_; }

        __declspec(property(get = get_ServicePackageId)) std::wstring const & ServicePackageId;
        std::wstring const & get_ServicePackageId() const { return servicePackageId_; }

        __declspec(property(get = get_CodePackageNetworkNames)) std::map<std::wstring, std::vector<std::wstring>> const & CodePackageNetworkNames;
        std::map<std::wstring, std::vector<std::wstring>> const & get_CodePackageNetworkNames() const { return codePackageNetworkNames_; }

        __declspec(property(get = get_Action)) ManageOverlayNetworkAction::Enum Action;
        ManageOverlayNetworkAction::Enum get_Action() const { return action_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_06(nodeId_, nodeName_, nodeIpAddress_, servicePackageId_, codePackageNetworkNames_, action_);

    private:
        std::wstring nodeId_;
        std::wstring nodeName_;
        std::wstring nodeIpAddress_;
        std::wstring servicePackageId_;
        std::map<std::wstring, std::vector<std::wstring>> codePackageNetworkNames_;
        ManageOverlayNetworkAction::Enum action_;
    };
}

DEFINE_USER_MAP_UTILITY(std::wstring, std::vector<std::wstring>);