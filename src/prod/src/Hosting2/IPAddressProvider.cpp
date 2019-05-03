// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceIPAddressProvider("IPAddressProvider");

// ********************************************************************************************************************
// IPAddressProvider::OpenAsyncOperation Implementation
// ********************************************************************************************************************
class IPAddressProvider::OpenAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        IPAddressProvider & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~OpenAsyncOperation()
    {
    }

    static ErrorCode OpenAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        // Invoke initialize on Azure 
        if (this->owner_.clusterId_.empty() == false && this->owner_.ipAddressProviderEnabled_)
        {
            auto operation = this->owner_.ipam_->BeginInitialize(
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnInitializeComplete(operation, false); },
                thisSPtr);

            this->OnInitializeComplete(operation, true);
        }
        else
        {
            // ipam client is left as not initialized so that apis 
            // return the invalid state error code
            WriteInfo(
                TraceIPAddressProvider,
                "Skipping IPAM client initialization. ClusterId: {0} IP address provider enabled {1}",
                this->owner_.clusterId_,
                this->owner_.ipAddressProviderEnabled_);

            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

private:
    void OnInitializeComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = this->owner_.ipam_->EndInitialize(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceIPAddressProvider,
            "IPAM client initialize complete: error {0}",
            error);

        if (error.IsSuccess())
        {
            this->owner_.ipamInitialized_ = true;

            wstring subnetCIDR = L"";
            uint gatewayIpAddress = 0;
            auto getSubnetAndGatewayIpErrorCode = this->owner_.ipam_->GetSubnetAndGatewayIpAddress(subnetCIDR, gatewayIpAddress);
            if (getSubnetAndGatewayIpErrorCode.IsSuccess())
            {
                this->owner_.gatewayIpAddress_ = IPHelper::Format(gatewayIpAddress);
                this->owner_.subnet_ = subnetCIDR;
            }
        }

        TryComplete(operation->Parent, error);
    }

private:
    IPAddressProvider & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// IPAddressProvider::CloseAsyncOperation Implementation
// ********************************************************************************************************************
class IPAddressProvider::CloseAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        IPAddressProvider & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~CloseAsyncOperation()
    {
    }

    static ErrorCode CloseAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

private:
    IPAddressProvider & owner_;
    TimeoutHelper timeoutHelper_;
};

IPAddressProvider::IPAddressProvider(
    ComponentRootSPtr const & root,
    ProcessActivationManager & processActivationManager)
    : RootedObject(*root),
    AsyncFabricComponent(),
    clusterId_(PaasConfig::GetConfig().ClusterId),
    ipamInitialized_(false),
    ipAddressProviderEnabled_(HostingConfig::GetConfig().IPProviderEnabled),
    reservedCodePackages_(),
    reservationIdCodePackageMap_()
{
#if !defined(PLATFORM_UNIX)
    UNREFERENCED_PARAMETER(processActivationManager);
    auto ipam = make_unique<IPAMWindows>(root);
    this->ipam_ = move(ipam);
#else
    auto ipam = make_unique<IPAMLinux>(root);
    this->ipam_ = move(ipam);
#endif
}

IPAddressProvider::IPAddressProvider(
    IIPAMSPtr const & ipamClient,
    std::wstring const & clusterId,
    bool const ipAddressProviderEnabled,
    ComponentRootSPtr const & root)
    : RootedObject(*root),
    AsyncFabricComponent(),
    clusterId_(clusterId),
    ipamInitialized_(false),
    ipAddressProviderEnabled_(ipAddressProviderEnabled),
    ipam_(ipamClient),
    reservedCodePackages_(),
    reservationIdCodePackageMap_()
{
}

IPAddressProvider::~IPAddressProvider()
{
}

ErrorCode IPAddressProvider::AcquireIPAddresses(
    std::wstring const & nodeId,
    std::wstring const & servicePackageId,
    std::vector<std::wstring> const & codePackageNames,
    __out std::vector<std::wstring> & assignedIps)
{
    if (!this->Initialized)
    {
        return ErrorCode(ErrorCodeValue::InvalidState);
    }

    ErrorCode error(ErrorCodeValue::Success);
    std::map<std::wstring, std::wstring> codePackageMap;

    AcquireExclusiveLock lock(this->ipAddressReservationLock_);
    {
        for (auto cp : codePackageNames)
        {
            auto reservationId = wformatString("{0}{1}", servicePackageId, cp);

            UINT ipAddress;
            auto returnCode = this->ipam_->Reserve(reservationId, ipAddress);
            if (returnCode.IsSuccess())
            {
                auto reservedIPAddress = IPHelper::Format(ipAddress);
                auto assignedIp = wformatString("{0},{1}", reservedIPAddress, cp);
                WriteInfo(
                    TraceIPAddressProvider,
                    "Assigned ip address={0} ReservationId={1}",
                    reservedIPAddress,
                    reservationId);
                assignedIps.push_back(assignedIp);
                codePackageMap[reservationId] = cp;
            }
            else
            {
                WriteInfo(
                    TraceIPAddressProvider,
                    "Reserve ip address returned={0}",
                    returnCode);
                error = ErrorCodeValue::IPAddressProviderAddressRangeExhausted;
                break;
            }
        }

        if (error.IsSuccess())
        {
            // Add to reserved ip map if it does not exist
            auto nodeIdIter = this->reservedCodePackages_.find(nodeId);
            if (nodeIdIter == this->reservedCodePackages_.end())
            {
                this->reservedCodePackages_.insert(make_pair(nodeId, std::map<wstring, std::vector<wstring>>()));
                nodeIdIter = this->reservedCodePackages_.find(nodeId);
            }

            auto servicePkgIter = nodeIdIter->second.find(servicePackageId);
            if (servicePkgIter != nodeIdIter->second.end())
            {
                auto existingCodePackages = servicePkgIter->second;
                for (auto cp : codePackageNames)
                {
                    if (find(existingCodePackages.begin(), existingCodePackages.end(), cp) == existingCodePackages.end())
                    {
                        servicePkgIter->second.push_back(cp);
                    }
                }
            }
            else
            {
                nodeIdIter->second[servicePackageId] = codePackageNames;
            }

            // Add reservation ids to reservation id code package map
            for (auto & cpm : codePackageMap)
            {
                auto reservationIdIter = this->reservationIdCodePackageMap_.find(cpm.first);
                if (reservationIdIter == this->reservationIdCodePackageMap_.end())
                {
                    auto codePkg = wformatString("{0},{1},{2}", nodeId, servicePackageId, cpm.second);
                    this->reservationIdCodePackageMap_.insert(make_pair(cpm.first, codePkg));
                }
            }
        }
    }

    // If failure occurred while assiging ip addresses to all code packages, we rollback completely by releasing 
    // ip addresses acquired so far.
    if (!error.IsSuccess())
    {
        for (auto & cpm : codePackageMap)
        {
            auto returnCode = this->ipam_->Release(cpm.first);
            WriteNoise(
                TraceIPAddressProvider,
                "Release ip address for reservation id={0} error={1}",
                cpm.first,
                returnCode);
        }

        assignedIps.clear();
    }

    return error;
}

ErrorCode IPAddressProvider::ReleaseIpAddresses(
    std::wstring const & nodeId,
    std::wstring const & servicePackageId)
{
    if (!this->Initialized)
    {
        return ErrorCode(ErrorCodeValue::InvalidState);
    }

    ErrorCode overallError(ErrorCodeValue::Success);
    size_t codePackagesCount = 0;
    int releasedCodePackagesCount = 0;

    AcquireExclusiveLock lock(this->ipAddressReservationLock_);
    {
        auto nodeIdIter = this->reservedCodePackages_.find(nodeId);
        if (nodeIdIter != this->reservedCodePackages_.end())
        {
            auto servicePkgIter = nodeIdIter->second.find(servicePackageId);
            if (servicePkgIter != nodeIdIter->second.end())
            {
                codePackagesCount = servicePkgIter->second.size();
                std::vector<std::wstring> codePackagesRemoved;
                for (auto & cp : servicePkgIter->second)
                {
                    auto reservationId = wformatString("{0}{1}", servicePackageId, cp);
                    auto error = this->ipam_->Release(reservationId);
                    WriteInfo(
                        TraceIPAddressProvider,
                        "Release ip address for reservationId={0} error={1}",
                        reservationId,
                        error);

                    // If successful, remove reservation id from reservation id code package map
                    if (error.IsSuccess())
                    {
                        releasedCodePackagesCount++;

                        auto reservationIdIter = this->reservationIdCodePackageMap_.find(reservationId);
                        if (reservationIdIter != this->reservationIdCodePackageMap_.end())
                        {
                            this->reservationIdCodePackageMap_.erase(reservationId);
                            codePackagesRemoved.push_back(cp);
                        }
                    }
                }

                // Remove code package whose ip address has been released
                auto existingCodePackages = servicePkgIter->second;
                for (auto cp : codePackagesRemoved)
                {
                    auto cpIter = find(existingCodePackages.begin(), existingCodePackages.end(), cp);
                    if (cpIter != existingCodePackages.end())
                    {
                        existingCodePackages.erase(cpIter);
                    }
                }

                // Remove service from reserved code packages map after we have released
                // ip adddresses for all code packages in that service.
                if (existingCodePackages.size() == 0)
                {
                    nodeIdIter->second.erase(servicePkgIter);
                }
            }

            // if the node has no services listed under it, then delete the node
            if (nodeIdIter->second.size() == 0)
            {
                this->reservedCodePackages_.erase(nodeIdIter);
            }
        }
    }

    overallError = (codePackagesCount == releasedCodePackagesCount) ? (ErrorCodeValue::Success) : (ErrorCodeValue::OperationFailed);
    return overallError;
}

void IPAddressProvider::ReleaseAllIpsForNode(wstring const & nodeId)
{
    if (!this->Initialized)
    {
        return;
    }

    AcquireExclusiveLock lock(this->ipAddressReservationLock_);
    {
        auto nodeIdIter = this->reservedCodePackages_.find(nodeId);
        if (nodeIdIter != this->reservedCodePackages_.end())
        {
            for (auto it = nodeIdIter->second.begin(); it != nodeIdIter->second.end(); it++)
            {
                auto servicePackageId = it->first;
                for (auto & cp : it->second)
                {
                    auto reservationId = wformatString("{0}{1}", servicePackageId, cp);
                    auto error = this->ipam_->Release(reservationId);
                    WriteInfo(
                        TraceIPAddressProvider,
                        "Release ip address for reservationId={0} error={1}",
                        reservationId,
                        error);

                    // If successful, remove reservation id from reservation id code package map
                    if (error.IsSuccess())
                    {
                        auto reservationIdIter = this->reservationIdCodePackageMap_.find(reservationId);
                        if (reservationIdIter != this->reservationIdCodePackageMap_.end())
                        {
                            this->reservationIdCodePackageMap_.erase(reservationId);
                        }
                    }
                }
            }
            this->reservedCodePackages_.erase(nodeIdIter);
        }
    }
}

void IPAddressProvider::GetReservedCodePackages(std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> & reservedCodePackages,
                                                std::map<std::wstring, std::wstring> & reservationIdCodePackageMap)
{
    // makes a copy of the reserved code packages and reservation id map
    AcquireExclusiveLock lock(this->ipAddressReservationLock_);
    {
        for (auto r : this->reservedCodePackages_)
        {
            reservedCodePackages.insert(make_pair(r.first, std::map<wstring, std::vector<wstring>>()));
            for (auto s : r.second)
            {
                auto nodeIter = reservedCodePackages.find(r.first);
                vector<std::wstring> codePackages;
                for (auto c : s.second)
                {
                    codePackages.push_back(c);
                }
                nodeIter->second[s.first] = codePackages;
            }
        }

        for (auto r : this->reservationIdCodePackageMap_)
        {
            reservationIdCodePackageMap.insert(make_pair(r.first, r.second));
        }
    }
}

void IPAddressProvider::GetNetworkReservedCodePackages(std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> & networkReservedCodePackages)
{
    std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> reservedCodePackages;
    std::map<std::wstring, std::wstring> reservationIdCodePackageMap;
    this->GetReservedCodePackages(reservedCodePackages, reservationIdCodePackageMap);

    networkReservedCodePackages.insert(make_pair(Common::ContainerEnvironment::ContainerNetworkName, std::map<wstring, std::vector<wstring>>()));
    auto networkIter = networkReservedCodePackages.find(Common::ContainerEnvironment::ContainerNetworkName);

    for (auto r : reservedCodePackages)
    {
        for (auto s : r.second)
        {
            vector<std::wstring> codePackages;
            for (auto c : s.second)
            {
                codePackages.push_back(c);
            }
            networkIter->second[s.first] = codePackages;
        }
    }
}

Common::ErrorCode IPAddressProvider::StartRefreshProcessing(
    Common::TimeSpan const refreshInterval,
    Common::TimeSpan const refreshTimeout,
    std::function<void(std::map<std::wstring, std::wstring>)> ghostReservationsAvailableCallback)
{
    if (!this->Initialized)
    {
        return ErrorCode(ErrorCodeValue::InvalidState);
    }

    ErrorCode error(ErrorCodeValue::Success);
    if (refreshInterval != TimeSpan::MaxValue)
    {
        if (ghostReservationsAvailableCallback != nullptr)
        {
            WriteInfo(
                TraceIPAddressProvider,
                "Starting refresh processing. Refresh interval: {0} Refresh timeout: {1}",
                refreshInterval,
                refreshTimeout);

            error = this->ipam_->StartRefreshProcessing(
                refreshInterval,
                refreshTimeout,
                [this, ghostReservationsAvailableCallback](DateTime lastRefresh)
            {
                WriteInfo(
                    TraceIPAddressProvider,
                    "Ghost change callback invoked with last refresh time {0}",
                    lastRefresh);

                auto ghostReservationIds = this->ipam_->GetGhostReservations();
                std::map<wstring, wstring> ghostReservations;

                AcquireExclusiveLock lock(this->ipAddressReservationLock_);
                {
                    for (auto & gr : ghostReservationIds)
                    {
                        auto reservationIdIter = this->reservationIdCodePackageMap_.find(gr);
                        if (reservationIdIter != this->reservationIdCodePackageMap_.end())
                        {
                            auto ghostIter = ghostReservations.find(reservationIdIter->first);
                            if (ghostIter == ghostReservations.end())
                            {
                                ghostReservations.insert(make_pair(reservationIdIter->first, reservationIdIter->second));
                            }
                        }
                        else
                        {
                            WriteWarning(
                                TraceIPAddressProvider,
                                "Ghost reservation id {0} not found in reservation id code package map.",
                                gr);
                        }
                    }
                }

                if (ghostReservationsAvailableCallback != nullptr)
                {
                    ghostReservationsAvailableCallback(ghostReservations);
                }
            });
        }
        else
        {
            WriteWarning(
                TraceIPAddressProvider,
                "Ghost reservations available callback not specified.");
        }
    }
    else
    {
        WriteWarning(
            TraceIPAddressProvider,
            "Refresh interval specified will disable refresh processing.");
    }

    return error;
}

// ********************************************************************************************************************
// AsyncFabricComponent methods
// ********************************************************************************************************************
AsyncOperationSPtr IPAddressProvider::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(TraceIPAddressProvider, "Opening IPAddressProvider.");

    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode IPAddressProvider::OnEndOpen(
    AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr IPAddressProvider::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(TraceIPAddressProvider, "Closing IPAddressProvider.");

    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode IPAddressProvider::OnEndClose(
    AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void IPAddressProvider::OnAbort()
{
    WriteNoise(TraceIPAddressProvider, "Aborting IPAddressProvider.");

    if (this->ipam_)
    {
        this->ipam_->CancelRefreshProcessing();
    }
}
