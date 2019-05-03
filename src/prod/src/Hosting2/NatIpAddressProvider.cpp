// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceNatIpAddressProvider("NatIpAddressProvider");

// ********************************************************************************************************************
// NatIPAddressProvider::CreateNetworkAsyncOperation Implementation
// ********************************************************************************************************************
class NatIPAddressProvider::CreateNetworkAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CreateNetworkAsyncOperation)

public:
    CreateNetworkAsyncOperation(
        NatIPAddressProvider & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~CreateNetworkAsyncOperation()
    {
    }

    static ErrorCode CreateNetworkAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CreateNetworkAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceNatIpAddressProvider,
            "Attempting Delete Nat Network on startup");
        auto containerDescription = ContainerDescription();
        auto deleteUri = wformatString(L"/networks/{0}", HostingConfig::GetConfig().LocalNatIpProviderNetworkName);

        auto deleteoperation = this->owner_.containerActivator_.BeginInvokeContainerApi(
            containerDescription,
            L"DELETE",
            deleteUri,
            L"",
            L"",
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & deleteoperation)
        {
            OnDeleteNetworkCompleted(deleteoperation, false);
        },
            thisSPtr);

        OnDeleteNetworkCompleted(deleteoperation, true);
    }

private:

    void OnDeleteNetworkCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        wstring result;
        auto deleteerror = this->owner_.containerActivator_.EndInvokeContainerApi(operation, result);
        if (!deleteerror.IsSuccess())
        {
            TryComplete(operation->Parent, deleteerror);
            return;
        }

        WriteInfo(
            TraceNatIpAddressProvider,
            "Delete network for {0} error {1}",
            HostingConfig::GetConfig().LocalNatIpProviderNetworkName,
            deleteerror);

        auto natNetworkRange = HostingConfig::GetConfig().LocalNatIpProviderNetworkRange;
        UINT baseIP = 0;
        int maskNum = 0;
        UINT gatewayIP = 0;

        if (!IPHelper::TryParseCIDR(natNetworkRange, baseIP, maskNum))
        {
            TryComplete(operation->Parent, ErrorCodeValue::OperationFailed);
            return;
        }

        // Convert the mask number to a bitmask;
        uint mask = 0;
        for (int i = 0; i < maskNum; i++)
        {
            mask >>= 1;
            mask |= 0x80000000;
        }

        // apply & calculate the gateway IP.
        gatewayIP = (baseIP & mask) + 1;

        // gateway string representation
        this->owner_.gatewayIp_ = IPHelper::Format(gatewayIP);

        vector<NetworkIPAMConfig> ipamConfigArray;
        auto ipamConfig = NetworkIPAMConfig(natNetworkRange, this->owner_.gatewayIp_);
        ipamConfigArray.push_back(ipamConfig);
        auto networkIPAM = NetworkIPAM(ipamConfigArray);
        auto networkConfig = NetworkConfig(
            HostingConfig::GetConfig().LocalNatIpProviderNetworkName,
            L"nat",
            true, // checkDuplicate
            networkIPAM);

        wstring paramsString;
        auto error = JsonHelper::Serialize(networkConfig, paramsString);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceNatIpAddressProvider,
                "Create network params serialization error {0}.",
                paramsString);
            TryComplete(operation->Parent, error);
            return;
        }

        WriteInfo(
            TraceNatIpAddressProvider,
            "Create network params string {0}.",
            paramsString);
        
        auto containerDescription = ContainerDescription();
        auto createoperation = this->owner_.containerActivator_.BeginInvokeContainerApi(
            containerDescription,
            L"POST",
            L"/networks/create",
            L"application/json",
            paramsString,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & createoperation)
        {
            OnCreateNetworkCompleted(createoperation, false);
        },
            operation->Parent);

        OnCreateNetworkCompleted(createoperation, true);
    }

    void OnCreateNetworkCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        wstring result;
        auto error = this->owner_.containerActivator_.EndInvokeContainerApi(operation, result);

        WriteInfo(
            TraceNatIpAddressProvider,
            "Create network for {0} error {1}",
            HostingConfig::GetConfig().LocalNatIpProviderNetworkName,
            error);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        auto containerDescription = ContainerDescription();
        auto getoperation = this->owner_.containerActivator_.BeginInvokeContainerApi(
            containerDescription,
            L"GET",
            wformatString("/networks/{0}", HostingConfig::GetConfig().LocalNatIpProviderNetworkName),
            L"application/json",
            L"",
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & getoperation)
        {
            OnGetNetworkCompleted(getoperation, false);
        },
            operation->Parent);

        OnGetNetworkCompleted(getoperation, true);
    }

    void OnGetNetworkCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        wstring result;
        auto error = this->owner_.containerActivator_.EndInvokeContainerApi(operation, result);

        WriteInfo(
            TraceNatIpAddressProvider,
            "Get network for {0} error {1}, {2}",
            HostingConfig::GetConfig().LocalNatIpProviderNetworkName,
            error,
            result);

        TryComplete(operation->Parent, error);
    }

private:
    NatIPAddressProvider & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// NatIPAddressProvider::OpenAsyncOperation Implementation
// ********************************************************************************************************************
class NatIPAddressProvider::OpenAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        NatIPAddressProvider & owner,
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
        if (this->owner_.natIpAddressProviderEnabled_)
        {
#if defined(LOCAL_NAT_NETWORK_SETUP)
            auto operation = this->owner_.BeginCreateNetwork(
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnCreateNetworkComplete(operation, false); },
                thisSPtr);

            this->OnCreateNetworkComplete(thisSPtr, true);
#else
            auto ipamOperation = this->owner_.ipam_->BeginInitialize(
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) 
            { 
                this->OnInitializeComplete(operation, false); 
            },
                thisSPtr);

            this->OnInitializeComplete(ipamOperation, true);
#endif
        }
        else
        {
            // ipam client is left as not initialized so that apis 
            // return the invalid state error code
            WriteInfo(
                TraceNatIpAddressProvider,
                "Skipping Nat IPAM client initialization. Nat IP address provider enabled {0}",
                this->owner_.natIpAddressProviderEnabled_);

            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

private:

#if defined(LOCAL_NAT_NETWORK_SETUP) 
    void OnCreateNetworkComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        WriteInfo(
                TraceNatIpAddressProvider,
                "OnCreateNetworkComplete");

        auto error = this->owner_.EndCreateNetwork(operation);
        if (error.IsSuccess())
        {
            auto ipamOperation = this->owner_.ipam_->BeginInitialize(
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) 
            { 
                this->OnInitializeComplete(operation, false); 
            },
                operation->Parent);

            this->OnInitializeComplete(ipamOperation, true);
        }
        else
        {
            TryComplete(operation->Parent, error);
        }
    }
#endif

    void OnInitializeComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = this->owner_.ipam_->EndInitialize(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceNatIpAddressProvider,
            "Nat IPAM client initialize complete: error {0}",
            error);

        if (error.IsSuccess())
        {
            this->owner_.ipamInitialized_ = true;
        }

        TryComplete(operation->Parent, error);
    }

private:
    NatIPAddressProvider & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// NatIPAddressProvider::DeleteNetworkAsyncOperation Implementation
// ********************************************************************************************************************
class NatIPAddressProvider::DeleteNetworkAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(DeleteNetworkAsyncOperation)

public:
    DeleteNetworkAsyncOperation(
        NatIPAddressProvider & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~DeleteNetworkAsyncOperation()
    {
    }

    static ErrorCode DeleteNetworkAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeleteNetworkAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->owner_.natIpAddressProviderEnabled_)
        {
#if defined(LOCAL_NAT_NETWORK_SETUP)
            WriteInfo(
                    TraceNatIpAddressProvider,
                    "Attempting Delete Nat Network");
            auto containerDescription = ContainerDescription();
            auto deleteUri = wformatString(L"/networks/{0}", HostingConfig::GetConfig().LocalNatIpProviderNetworkName);

            auto operation = this->owner_.containerActivator_.BeginInvokeContainerApi(
                containerDescription,
                L"DELETE",
                deleteUri,
                L"",
                L"",
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                OnDeleteNetworkCompleted(operation, false);
            },
                thisSPtr);

            OnDeleteNetworkCompleted(operation, true);
#else
            TryComplete(thisSPtr, ErrorCodeValue::Success);
#endif
        }
        else
        {
            // Skip clean up since ipam client was not initialized.
            WriteInfo(
                TraceNatIpAddressProvider,
                "Skipping Nat IPAM client clean up.");

            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

    void OnDeleteNetworkCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        wstring result;
        auto error = this->owner_.containerActivator_.EndInvokeContainerApi(operation, result);

        WriteInfo(
            TraceNatIpAddressProvider,
            "Delete network for {0} error {1}",
            HostingConfig::GetConfig().LocalNatIpProviderNetworkName,
            error);

        TryComplete(operation->Parent, error);
    }

private:
    NatIPAddressProvider & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// NatIPAddressProvider::CloseAsyncOperation Implementation
// ********************************************************************************************************************
class NatIPAddressProvider::CloseAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        NatIPAddressProvider & owner,
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
        if (this->owner_.natIpAddressProviderEnabled_)
        {
            auto operation = this->owner_.BeginDeleteNetwork(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnDeleteNetworkComplete(operation, false); },
            thisSPtr);

            this->OnDeleteNetworkComplete(operation, true);
        }
        else
        {
            WriteInfo(
                TraceNatIpAddressProvider,
                "Skipping NatIpAddressProvider Close. Nat IP address provider enabled {0}",
                this->owner_.natIpAddressProviderEnabled_);

            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

    void OnDeleteNetworkComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = this->owner_.EndDeleteNetwork(operation);

        TryComplete(operation->Parent, error);
    }

private:
    NatIPAddressProvider & owner_;
    TimeoutHelper timeoutHelper_;
};

NatIPAddressProvider::NatIPAddressProvider(
    ComponentRootSPtr const & root,
    ContainerActivator & containerActivator)
    : RootedObject(*root),
    AsyncFabricComponent(),
    containerActivator_(containerActivator),
    ipamInitialized_(false),
    natIpAddressProviderEnabled_(HostingConfig::GetConfig().LocalNatIpProviderEnabled),
    reservedCodePackages_(),
    reservationIdCodePackageMap_()
{
    auto ipam = make_unique<NatIPAM>(root, HostingConfig::GetConfig().LocalNatIpProviderNetworkRange);
    this->ipam_ = move(ipam);
}

NatIPAddressProvider::~NatIPAddressProvider()
{
}

ErrorCode NatIPAddressProvider::AcquireIPAddresses(
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
                    TraceNatIpAddressProvider,
                    "Assigned ip address={0} ReservationId={1}",
                    reservedIPAddress,
                    reservationId);
                assignedIps.push_back(assignedIp);
                codePackageMap[reservationId] = cp;
            }
            else
            {
                WriteInfo(
                    TraceNatIpAddressProvider,
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
                TraceNatIpAddressProvider,
                "Release ip address for reservation id={0} error={1}",
                cpm.first,
                returnCode);
        }

        assignedIps.clear();
    }

    return error;
}

ErrorCode NatIPAddressProvider::ReleaseIpAddressesInternal(
    std::wstring const & nodeId,
    std::wstring const & servicePackageId,
    std::map<std::wstring, vector<std::wstring>>::iterator const & servicePackageIter)
{
    ErrorCode overallError(ErrorCodeValue::Success);
    size_t codePackagesCount = 0;
    int releasedCodePackagesCount = 0;

    auto nodeIdIter = this->reservedCodePackages_.find(nodeId);
    if (nodeIdIter != this->reservedCodePackages_.end())
    {
        codePackagesCount = servicePackageIter->second.size();
        std::vector<std::wstring> codePackagesRemoved;
        for (auto & cp : servicePackageIter->second)
        {
            auto reservationId = wformatString("{0}{1}", servicePackageId, cp);
            auto error = this->ipam_->Release(reservationId);
            WriteInfo(
                TraceNatIpAddressProvider,
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
        auto existingCodePackages = servicePackageIter->second;
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
            nodeIdIter->second.erase(servicePackageIter);
        }

        // if the node has no services listed under it, then delete the node
        if (nodeIdIter->second.size() == 0)
        {
            this->reservedCodePackages_.erase(nodeIdIter);
        }
    }

    overallError = (codePackagesCount == releasedCodePackagesCount) ? (ErrorCodeValue::Success) : (ErrorCodeValue::OperationFailed);
    return overallError;
}

ErrorCode NatIPAddressProvider::ReleaseIpAddresses(
    std::wstring const & nodeId,
    std::wstring const & servicePackageId)
{
    if (!this->Initialized)
    {
        return ErrorCode(ErrorCodeValue::InvalidState);
    }

    ErrorCode error;
    AcquireExclusiveLock lock(this->ipAddressReservationLock_);
    {
        auto nodeIdIter = this->reservedCodePackages_.find(nodeId);
        if (nodeIdIter != this->reservedCodePackages_.end())
        {
            auto servicePkgIter = nodeIdIter->second.find(servicePackageId);
            if (servicePkgIter != nodeIdIter->second.end())
            {
                error = ReleaseIpAddressesInternal(nodeId, servicePackageId, servicePkgIter);
            }
            else{
                return ErrorCode(ErrorCodeValue::ServiceNotFound);
            }
        }
        else
        {
            return ErrorCode(ErrorCodeValue::NodeNotFound);
        }
    }
    return error;
}

void NatIPAddressProvider::ReleaseAllIpsForNode(wstring const & nodeId)
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
                ReleaseIpAddressesInternal(nodeId, servicePackageId, it).ReadValue();
            }
        }
    }
}

void NatIPAddressProvider::GetReservedCodePackages(std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> & reservedCodePackages,
    std::map<std::wstring, std::wstring> & reservationIdCodePackageMap)
{
    // makes a copy of the reserved code packages and reservation id map
    AcquireReadLock lock(this->ipAddressReservationLock_);
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

// ********************************************************************************************************************
// AsyncFabricComponent methods
// ********************************************************************************************************************
AsyncOperationSPtr NatIPAddressProvider::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(TraceNatIpAddressProvider, "Opening NatIPAddressProvider.");

    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode NatIPAddressProvider::OnEndOpen(
    AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr NatIPAddressProvider::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(TraceNatIpAddressProvider, "Closing NatIPAddressProvider.");

    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode NatIPAddressProvider::OnEndClose(
    AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void NatIPAddressProvider::OnAbort()
{
    WriteNoise(TraceNatIpAddressProvider, "Aborting NatIPAddressProvider.");
    if (this->natIpAddressProviderEnabled_)
    {
        AsyncOperationWaiterSPtr cleanupWaiter = make_shared<AsyncOperationWaiter>();
        TimeSpan timeout = TimeSpan::FromSeconds(60);

        auto operation = this->BeginDeleteNetwork(
            timeout,
            [this, cleanupWaiter](AsyncOperationSPtr const & operation) 
        { 
            auto error = this->EndDeleteNetwork(operation);
            cleanupWaiter->SetError(error);
            cleanupWaiter->Set();
            ; 
        },
            this->Root.CreateAsyncOperationRoot());

        if (cleanupWaiter->WaitOne(timeout))
        {
            auto error = cleanupWaiter->GetError();
            WriteTrace(
                error.ToLogLevel(),
                TraceNatIpAddressProvider,
                "Delete network on abort returned error {0}.",
                error);
        }
        else
        {
            WriteWarning(
                TraceNatIpAddressProvider,
                "Delete network on abort timed out.");
        }
    }
    else
    {
        WriteInfo(
            TraceNatIpAddressProvider,
            "Skipping NatIpAddressProvider Abort. Nat IP address provider enabled {0}",
            this->natIpAddressProviderEnabled_);
    }
}

AsyncOperationSPtr NatIPAddressProvider::BeginCreateNetwork(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteInfo(
            TraceNatIpAddressProvider,
            "BeginCreateNetwork");
    return AsyncOperation::CreateAndStart<CreateNetworkAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode NatIPAddressProvider::EndCreateNetwork(AsyncOperationSPtr const & operation)
{
    WriteInfo(
            TraceNatIpAddressProvider,
            "EndCreateNetwork");
    return CreateNetworkAsyncOperation::End(operation);
}

AsyncOperationSPtr NatIPAddressProvider::BeginDeleteNetwork(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeleteNetworkAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode NatIPAddressProvider::EndDeleteNetwork(AsyncOperationSPtr const & operation)
{
    return DeleteNetworkAsyncOperation::End(operation);
}