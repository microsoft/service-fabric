// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class SetupContainerGroupRequest : public Serialization::FabricSerializable
    {
    public:
        SetupContainerGroupRequest();
        SetupContainerGroupRequest(
            std::wstring const & servicePackageId,
            std::wstring const & appId,
            std::wstring const & appfolder,
            std::wstring const & nodeId,
            std::wstring const & assignedIp_,
            int64 timeoutTicks,
            ServiceModel::ServicePackageResourceGovernanceDescription const & spRg);

        __declspec(property(get = get_ServicePackageId)) std::wstring const & ServicePackageId;
        std::wstring const & get_ServicePackageId() const { return servicePackageId_; }

        __declspec(property(get = get_AppId)) std::wstring const & AppId;
        std::wstring const & get_AppId() const { return appId_; }

        __declspec(property(get = get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get = get_AppFolder)) std::wstring const & AppFolder;
        std::wstring const & get_AppFolder() const { return appfolder_; }

        __declspec(property(get = get_AssignedIP)) std::wstring const & AssignedIP;
        std::wstring const & get_AssignedIP() const { return assignedIp_; }

        __declspec(property(get = get_TimeoutTicks)) int64 TimeoutInTicks;
        int64 get_TimeoutTicks() const { return ticks_; }

        __declspec(property(get = get_ServicePackageRG)) ServiceModel::ServicePackageResourceGovernanceDescription const & ServicePackageRG;
        ServiceModel::ServicePackageResourceGovernanceDescription const & get_ServicePackageRG() const { return spRg_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_07(servicePackageId_, appId_, appfolder_,nodeId_,assignedIp_, ticks_, spRg_);

    private:
        std::wstring servicePackageId_;
        std::wstring assignedIp_;
        std::wstring appId_;
        std::wstring appfolder_;
        std::wstring nodeId_;
        int64 ticks_;
        ServiceModel::ServicePackageResourceGovernanceDescription spRg_;
    };
}
