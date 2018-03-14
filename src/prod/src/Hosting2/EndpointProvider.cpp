// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;

StringLiteral const TraceEndpointProvider("EndpointProvider");

EndpointProvider::EndpointProvider(
    ComponentRoot const & root, 
    wstring const & nodeId, 
    int startPortRangeInclusive, 
    int endPortRangeInclusive)
    : RootedObject(root),
    FabricComponent(),
    nodeId_(nodeId),    
    startPortRangeInclusive_(startPortRangeInclusive),
    endPortRangeInclusive_(endPortRangeInclusive),
    currentOffset_(-1),
    maxPortsAvailable_(endPortRangeInclusive_ - startPortRangeInclusive_ + 1),
    reservedPorts_()
{
    ASSERT_IF(
        (endPortRangeInclusive_ - startPortRangeInclusive_) < 0,
        "EndApplicationPortRange {0} must be greater or equals to StartApplicationPortRange {1}.", 
        endPortRangeInclusive_, 
        startPortRangeInclusive_);
}

EndpointProvider::~EndpointProvider()
{
}

ErrorCode EndpointProvider::OnOpen()
{    
    WriteNoise(TraceEndpointProvider, Root.TraceId, "Opening EndpointProvider");    
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode EndpointProvider::OnClose()
{
    WriteNoise(TraceEndpointProvider, Root.TraceId, "Closing EndpointProvider");
    
    return ErrorCode(ErrorCodeValue::Success);
}

void EndpointProvider::OnAbort()
{
    WriteNoise(TraceEndpointProvider, Root.TraceId, "Aborting EndpointProvider.");
}

ErrorCode EndpointProvider::AddEndpoint(EndpointResourceSPtr const & endpoint)
{
    if (this->State.Value != FabricComponentState::Opened)
    {
        WriteInfo(
            TraceEndpointProvider,
            Root.TraceId,
            "The EndpointResource '{0}' cannot be added when endpoint provider is not opened.", endpoint->Name);

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    UINT claimedPort;
    auto error = this->ClaimNextAvailablePort(claimedPort);
    if (!error.IsSuccess()) { return error; }

    endpoint->set_Port(claimedPort);    

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode EndpointProvider::RemoveEndpoint(
    EndpointResourceSPtr const & endpoint)
{
    if (this->State.Value != FabricComponentState::Opened)
    {
        WriteInfo(
            TraceEndpointProvider,
            Root.TraceId,
            "The EndpointResource '{0}' cannot be removed when endpoint provider is not opened.", endpoint->Name);

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    this->UnclaimPort(endpoint->Port);
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode EndpointProvider::ClaimNextAvailablePort(UINT & claimedPort)
{
    claimedPort = 0;
    {
        AcquireExclusiveLock lock(portReservationLock_);
        int index = currentOffset_ + 1;
        for (int count = 0; count <= maxPortsAvailable_; count++)
        {
            index %= maxPortsAvailable_;
            auto portNumber = startPortRangeInclusive_ + index;
            if (reservedPorts_.find(portNumber) == reservedPorts_.end())
            {
                claimedPort = portNumber;
                reservedPorts_.insert(claimedPort);
                currentOffset_ = index;
                break;
            }
            index++;
        }
    }

    auto error = ErrorCode((claimedPort > 0) ? ErrorCodeValue::Success : ErrorCodeValue::EndpointProviderPortRangeExhausted);
    WriteTrace(
        error.ToLogLevel(),
        TraceEndpointProvider,
        Root.TraceId,
        "ClaimNextAvailablePort Completed. ErrorCode={0}, ClaimedPort={1}, StartPort={2}, EndPort={3}",
        error,
        claimedPort,
        startPortRangeInclusive_,
        endPortRangeInclusive_);
    return error;
}

void EndpointProvider::UnclaimPort(UINT const claimedPort)
{
    bool success = false;
    {
        AcquireExclusiveLock lock(portReservationLock_);
        auto portIter = reservedPorts_.find(claimedPort);
        if (portIter != reservedPorts_.end())
        {
            success = true;
            reservedPorts_.erase(portIter);
        }
    }

    WriteNoise(
        TraceEndpointProvider,
        Root.TraceId,
        "UnclaimPort Completed. Success={0}, Port={1}",
        success,
        claimedPort);
}

ErrorCode EndpointProvider::GetPrincipal(EndpointResourceSPtr const & endpointResource, __out SecurityPrincipalInformationSPtr & principal)
{
    EnvironmentResource::ResourceAccess resourceAccess;
    auto error  = endpointResource->GetDefaultSecurityAccess(resourceAccess);
    if (!error.IsSuccess()) { return error; }

    principal = resourceAccess.PrincipalInfo;
    return ErrorCode(ErrorCodeValue::Success);
}

bool EndpointProvider::IsHttpEndpoint(EndpointResourceSPtr const & endpoint)
{
    return (endpoint->Protocol == ProtocolType::Http || endpoint->Protocol == ProtocolType::Https);
}

wstring EndpointProvider::GetHttpEndpoint(UINT port, bool isHttps)
{
    wstringstream formatstream;

    if (isHttps)
    {
        formatstream << L"https://+:" << port << L"/";
    }
    else
    {
        formatstream << L"http://+:" << port << L"/";
    }

    return formatstream.str();
}
