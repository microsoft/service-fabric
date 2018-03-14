// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

DateTime ContainerHealthStatusInfo::OffsetTimeStamp = DateTime(L"1970-01-01T00:00:00.000Z");
GlobalWString ContainerHealthStatusInfo::DockerHealthCheckStatusHealthy = make_global<wstring>(L"healthy");
GlobalWString ContainerHealthStatusInfo::DockerHealthCheckStatusUnhealthy = make_global<wstring>(L"unhealthy");

ContainerHealthStatusInfo::ContainerHealthStatusInfo()
    : hostId_()
    , containerName_()
    , timeStampInSeconds_(0)
    , isHealthy_(false)
{
}

ContainerHealthStatusInfo::ContainerHealthStatusInfo(
    std::wstring const & hostId,
    std::wstring const & containerName,
    int64 timeStampInSeconds,
    bool isHealthy)
    : hostId_(hostId)
    , containerName_(containerName)
    , timeStampInSeconds_(timeStampInSeconds)
    , isHealthy_(isHealthy)
{
}

DateTime ContainerHealthStatusInfo::GetTimeStampAsUtc() const
{
    auto elapsedTime = TimeSpan::FromSeconds((double)timeStampInSeconds_);

    return (OffsetTimeStamp + elapsedTime);
}

wstring ContainerHealthStatusInfo::GetDockerHealthStatusString() const
{
    return (isHealthy_ ? *DockerHealthCheckStatusHealthy : *DockerHealthCheckStatusUnhealthy);
}

void ContainerHealthStatusInfo::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerHealthStatusInfo { ");

    w.Write("AppHostId={0}, ", hostId_);
    w.Write("ContainerName={0}, ", containerName_);
    w.Write("TimeStamp={0}, ", GetTimeStampAsUtc());
    w.Write("IsHealthy={0}", isHealthy_);

    w.Write("}");
}

//***************************************************************
// ContainerHealthCheckStatusChangeNotification Implementation
//***************************************************************

ContainerHealthCheckStatusChangeNotification::ContainerHealthCheckStatusChangeNotification()
    : nodeId_()
    , healthStatusInfoList_()
{
}

ContainerHealthCheckStatusChangeNotification::ContainerHealthCheckStatusChangeNotification(
    wstring const & nodeId,
    vector<ContainerHealthStatusInfo> const & healthStatusInfoList)
    : nodeId_(nodeId)
    , healthStatusInfoList_(healthStatusInfoList)
{
}

std::vector<ContainerHealthStatusInfo> && ContainerHealthCheckStatusChangeNotification::TakeHealthStatusInfoList()
{
    return move(healthStatusInfoList_);
}

void ContainerHealthCheckStatusChangeNotification::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerHealthCheckStatusChangeNotification { ");
    w.Write("NodeId={0}, [", nodeId_);

    w.Write("HealthStatusInfoCount={0}, {", healthStatusInfoList_.size());

    for (auto const & healthInfo : healthStatusInfoList_)
    {
        w.Write("{");
        w.Write(healthInfo);
        w.Write("}");
    }

    w.Write("]}");
}
