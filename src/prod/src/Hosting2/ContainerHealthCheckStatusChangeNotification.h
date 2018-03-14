// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerHealthStatusInfo : public Serialization::FabricSerializable
    {
    public:
        ContainerHealthStatusInfo();

        ContainerHealthStatusInfo(
            std::wstring const & hostId,
            std::wstring const & containerName,
            int64 timeStampInSeconds,
            bool isHealthy);

        __declspec(property(get = get_HostId)) std::wstring const & HostId;
        std::wstring const & get_HostId() const { return hostId_; }

        __declspec(property(get = get_ContainerName)) std::wstring const & ContainerName;
        std::wstring const & get_ContainerName() const { return containerName_; }

        __declspec(property(get = get_TimeStampInSeconds)) int64 TimeStampInSeconds;
        int64 get_TimeStampInSeconds() const { return timeStampInSeconds_; }

        __declspec(property(get = get_IsHealthy)) bool IsHealthy;
        bool get_IsHealthy() const { return isHealthy_; }

        Common::DateTime GetTimeStampAsUtc() const;
        wstring GetDockerHealthStatusString() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(hostId_, containerName_, timeStampInSeconds_, isHealthy_);

    private:
        std::wstring hostId_;
        std::wstring containerName_;
        int64 timeStampInSeconds_;
        bool isHealthy_;

        static Common::DateTime OffsetTimeStamp;
        static Common::GlobalWString DockerHealthCheckStatusHealthy;
        static Common::GlobalWString DockerHealthCheckStatusUnhealthy;
    };

    class ContainerHealthCheckStatusChangeNotification : public Serialization::FabricSerializable
    {
    public:
        ContainerHealthCheckStatusChangeNotification();

        ContainerHealthCheckStatusChangeNotification(
            std::wstring const & nodeId,
            std::vector<ContainerHealthStatusInfo> const & containerHealthStatusInfoList);
        
        __declspec(property(get= get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get=get_ContainerHealthStatusInfoList)) std::vector<ContainerHealthStatusInfo> const & ContainerHealthStatusInfoList;
        std::vector<ContainerHealthStatusInfo> const & get_ContainerHealthStatusInfoList() const { return healthStatusInfoList_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        std::vector<ContainerHealthStatusInfo> && TakeHealthStatusInfoList();

        FABRIC_FIELDS_02(nodeId_, healthStatusInfoList_);

    private:
        std::wstring nodeId_;
        std::vector<ContainerHealthStatusInfo> healthStatusInfoList_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Hosting2::ContainerHealthStatusInfo);
