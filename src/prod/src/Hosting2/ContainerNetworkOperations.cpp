// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceType_NetworkOperations("ContainerNetworkOperations");

ContainerNetworkOperations::AssignOverlayNetworkResourcesAsyncOperation::AssignOverlayNetworkResourcesAsyncOperation(
    IContainerActivator & owner,
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & servicePackageId,
    std::map<wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    timeoutHelper_(timeout),
    nodeId_(nodeId),
    nodeName_(nodeName),
    nodeIpAddress_(nodeIpAddress),
    servicePackageId_(servicePackageId),
    codePackageNetworkNames_(codePackageNetworkNames)
{
    for (auto const & cp : codePackageNetworkNames_)
    {
        for (auto const & network : cp.second)
        {
            auto networkIter = networkNameCodePackageMap_.find(network);
            if (networkIter == networkNameCodePackageMap_.end())
            {
                networkNameCodePackageMap_.insert(make_pair(network, std::set<std::wstring>()));
                networkIter = networkNameCodePackageMap_.find(network);
            }

            networkIter->second.insert(cp.first);
        }
    }

    pendingOperationsCount_.store(networkNameCodePackageMap_.size());
}

ContainerNetworkOperations::AssignOverlayNetworkResourcesAsyncOperation::~AssignOverlayNetworkResourcesAsyncOperation()
{
}

ErrorCode ContainerNetworkOperations::AssignOverlayNetworkResourcesAsyncOperation::AssignOverlayNetworkResourcesAsyncOperation::End(
    AsyncOperationSPtr const & operation,
    std::map<wstring, std::map<std::wstring, std::wstring>> & assignedNetworkResources)
{
    auto thisPtr = AsyncOperation::End<AssignOverlayNetworkResourcesAsyncOperation>(operation);
    if (thisPtr->Error.IsSuccess())
    {
        assignedNetworkResources = move(thisPtr->assignedNetworkResources_);
    }
    return thisPtr->Error;
}

void ContainerNetworkOperations::AssignOverlayNetworkResourcesAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (!networkNameCodePackageMap_.empty())
    {
        for (auto const & nc : networkNameCodePackageMap_)
        {
            auto operation = this->owner_.OverlayNetworkManagerObj.BeginGetOverlayNetworkDriver(
                nc.first,
                this->nodeId_,
                this->nodeName_,
                this->nodeIpAddress_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetOverlayNetworkDriverCompleted(operation, false);
            },
                thisSPtr);

            this->OnGetOverlayNetworkDriverCompleted(operation, true);
        }
    }
    else
    {
        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
    }
}

void ContainerNetworkOperations::AssignOverlayNetworkResourcesAsyncOperation::OnGetOverlayNetworkDriverCompleted(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    OverlayNetworkDriverSPtr networkDriver;
    auto error = this->owner_.OverlayNetworkManagerObj.EndGetOverlayNetworkDriver(operation, networkDriver);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Get overlay network driver returned error {0}.",
        error);

    if (error.IsSuccess())
    {
        auto iter = this->networkNameCodePackageMap_.find(networkDriver->NetworkDefinition->NetworkName);
        if (iter != this->networkNameCodePackageMap_.end())
        {
            std::vector<std::wstring> codePackageNames;
            codePackageNames.assign(iter->second.begin(), iter->second.end());

            auto acquireOperation = networkDriver->BeginAcquireNetworkResources(
                this->nodeId_,
                this->servicePackageId_,
                codePackageNames,
                timeoutHelper_.GetRemainingTime(),
                [this, networkDriver](AsyncOperationSPtr const & acquireOperation)
            {
                this->OnAssignOverlayNetworkResourcesCompleted(acquireOperation, networkDriver, false);
            },
                operation->Parent);

            this->OnAssignOverlayNetworkResourcesCompleted(acquireOperation, networkDriver, true);
        }
        else
        {
            DecrementAndCheckPendingOperations(operation->Parent, ErrorCode(ErrorCodeValue::NameNotFound));
        }
    }
    else
    {
        DecrementAndCheckPendingOperations(operation->Parent, error);
    }
}

void ContainerNetworkOperations::AssignOverlayNetworkResourcesAsyncOperation::OnAssignOverlayNetworkResourcesCompleted(
    AsyncOperationSPtr const & operation, 
    OverlayNetworkDriverSPtr const & networkDriver,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    std::vector<std::wstring> assignedNetworkResources;
    auto acquireErrorCode = networkDriver->EndAcquireNetworkResources(operation, assignedNetworkResources);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Acquire overlay network resources returned error {0}.",
        acquireErrorCode);

    // Acquired network resource, returned by the driver is a string in this format: "codePackageName,ipAddress,macAddress"
    // The code below creates a map in this format: {codePackageName, {networkName, "ipAddress,macAddress"}}
    {
        AcquireExclusiveLock lock(lock_);
        for (auto nr = assignedNetworkResources.begin(); nr != assignedNetworkResources.end(); nr++)
        {
            vector<wstring> assignedOverlayNetworkResourceParts;
            StringUtility::Split<wstring>(*nr, assignedOverlayNetworkResourceParts, L",");
            if (assignedOverlayNetworkResourceParts.size() == 3)
            {
                auto cpIter = this->assignedNetworkResources_.find(assignedOverlayNetworkResourceParts[2]);
                if (cpIter == this->assignedNetworkResources_.end())
                {
                    this->assignedNetworkResources_.insert(make_pair(assignedOverlayNetworkResourceParts[2], std::map<std::wstring, std::wstring>()));
                    cpIter = this->assignedNetworkResources_.find(assignedOverlayNetworkResourceParts[2]);
                }

                wstring overlayNetworkResource = wformatString("{0},{1}", assignedOverlayNetworkResourceParts[0], assignedOverlayNetworkResourceParts[1]);
                auto networkIter = cpIter->second.find(networkDriver->NetworkDefinition->NetworkName);
                if (networkIter == cpIter->second.end())
                {
                    cpIter->second[networkDriver->NetworkDefinition->NetworkName] = overlayNetworkResource;
                }
                else
                {
                    networkIter->second = overlayNetworkResource;
                }
            }
        }
    }

    DecrementAndCheckPendingOperations(operation->Parent, acquireErrorCode);
}

void ContainerNetworkOperations::AssignOverlayNetworkResourcesAsyncOperation::DecrementAndCheckPendingOperations(
    AsyncOperationSPtr const & thisSPtr,
    ErrorCode const & error)
{
    AcquireExclusiveLock lock(lock_);

    if (!error.IsSuccess())
    {
        lastError_.ReadValue();
        lastError_ = error;
    }

    if (pendingOperationsCount_.load() == 0)
    {
        Assert::CodingError("Pending operation count is already 0.");
    }

    uint64 pendingOperationsCount = --pendingOperationsCount_;

    WriteInfo(
        TraceType_NetworkOperations,
        "CheckDecrement count {0}.", pendingOperationsCount);

    if (pendingOperationsCount == 0)
    {
        TryComplete(thisSPtr, lastError_);
    }
}

ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation::ReleaseOverlayNetworkResourcesAsyncOperation(
    IContainerActivator & owner,
    std::wstring const & nodeId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    timeoutHelper_(timeout),
    nodeId_(nodeId),
    cleanupNodeNetworkResources_(true)
{
}

ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation::ReleaseOverlayNetworkResourcesAsyncOperation(
    IContainerActivator & owner,
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & servicePackageId,
    std::vector<std::wstring> const & networkNames,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    timeoutHelper_(timeout),
    networkNames_(networkNames),
    nodeId_(nodeId),
    nodeName_(nodeName),
    nodeIpAddress_(nodeIpAddress),
    servicePackageId_(servicePackageId),
    cleanupNodeNetworkResources_(false)
{
    pendingOperationsCount_.store(networkNames.size());
}

ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation::~ReleaseOverlayNetworkResourcesAsyncOperation()
{
}

ErrorCode ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation::ReleaseOverlayNetworkResourcesAsyncOperation::End(
    AsyncOperationSPtr const & operation)
{
    auto thisPtr = AsyncOperation::End<ReleaseOverlayNetworkResourcesAsyncOperation>(operation);
    return thisPtr->Error;
}

void ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    // clean up all overlay network resources for the node
    if (this->cleanupNodeNetworkResources_)
    {
        auto operation = this->owner_.OverlayNetworkManagerObj.BeginReleaseAllNetworkResourcesForNode(
            this->nodeId_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnReleaseAllNetworkResourcesForNodeCompleted(operation, false);
        },
            thisSPtr);

        this->OnReleaseAllNetworkResourcesForNodeCompleted(operation, true);
    }
    else
    {
        if (!networkNames_.empty())
        {
            // Get overlay network names from the overlay network manager before getting the driver.
            // The overlay network names list returned is for live network drivers only.
            // This check prevents recreating the network driver after being deleted because 
            // all containers on the network have gone away.
            std::vector<std::wstring> existingNetworkNames;
            this->owner_.OverlayNetworkManagerObj.GetNetworkNames(existingNetworkNames);

            for (auto const & networkName : networkNames_)
            {
                if (std::find(existingNetworkNames.begin(), existingNetworkNames.end(), networkName) != existingNetworkNames.end())
                {
                    auto operation = this->owner_.OverlayNetworkManagerObj.BeginGetOverlayNetworkDriver(
                        networkName,
                        this->nodeId_,
                        this->nodeName_,
                        this->nodeIpAddress_,
                        timeoutHelper_.GetRemainingTime(),
                        [this](AsyncOperationSPtr const & operation)
                    {
                        this->OnGetOverlayNetworkDriverCompleted(operation, false);
                    },
                        thisSPtr);

                    this->OnGetOverlayNetworkDriverCompleted(operation, true);
                }
                else
                {
                    DecrementAndCheckPendingOperations(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                }
            }
        }
        else
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
        }
    }
}

void ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation::OnGetOverlayNetworkDriverCompleted(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    OverlayNetworkDriverSPtr networkDriver;
    auto error = this->owner_.OverlayNetworkManagerObj.EndGetOverlayNetworkDriver(operation, networkDriver);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Get overlay network driver returned error {0}.",
        error);

    if (error.IsSuccess())
    {
        auto releaseOperation = networkDriver->BeginReleaseNetworkResources(
            this->nodeId_,
            this->servicePackageId_,
            timeoutHelper_.GetRemainingTime(),
            [this, networkDriver](AsyncOperationSPtr const & releaseOperation)
        {
            this->OnReleaseOverlayNetworkResourcesCompleted(releaseOperation, networkDriver, false);
        },
            operation->Parent);

        this->OnReleaseOverlayNetworkResourcesCompleted(releaseOperation, networkDriver, true);
    }
    else
    {
        DecrementAndCheckPendingOperations(operation->Parent, error);
    }
}

void ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation::OnReleaseOverlayNetworkResourcesCompleted(
    AsyncOperationSPtr const & operation, 
    OverlayNetworkDriverSPtr const & networkDriver,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = networkDriver->EndReleaseNetworkResources(operation);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Release overlay network resources returned error {0}.",
        error);

    DecrementAndCheckPendingOperations(operation->Parent, error);
}

void ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation::OnReleaseAllNetworkResourcesForNodeCompleted(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = this->owner_.OverlayNetworkManagerObj.EndReleaseAllNetworkResourcesForNode(operation);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Release all overlay network resources for node error {0}.",
        error);

    TryComplete(operation->Parent, error);
}

void ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation::DecrementAndCheckPendingOperations(
    AsyncOperationSPtr const & thisSPtr,
    ErrorCode const & error)
{
    AcquireExclusiveLock lock(lock_);

    if (!error.IsSuccess())
    {
        lastError_.ReadValue();
        lastError_ = error;
    }

    if (pendingOperationsCount_.load() == 0)
    {
        Assert::CodingError("Pending operation count is already 0.");
    }

    uint64 pendingOperationsCount = --pendingOperationsCount_;

    WriteInfo(
        TraceType_NetworkOperations,
        "CheckDecrement count {0}.", pendingOperationsCount);

    if (pendingOperationsCount == 0)
    {
        TryComplete(thisSPtr, lastError_);
    }
}

ContainerNetworkOperations::DetachContainerFromNetworkAsyncOperation::DetachContainerFromNetworkAsyncOperation(
    IContainerActivator & owner,
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & containerId,
    std::vector<std::wstring> const & networkNames,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    networkNames_(networkNames),
    nodeId_(nodeId),
    nodeName_(nodeName),
    nodeIpAddress_(nodeIpAddress),
    containerId_(containerId),
    timeoutHelper_(timeout)
{
    pendingOperationsCount_.store(networkNames.size());
}

ContainerNetworkOperations::DetachContainerFromNetworkAsyncOperation::~DetachContainerFromNetworkAsyncOperation()
{
}

ErrorCode ContainerNetworkOperations::DetachContainerFromNetworkAsyncOperation::DetachContainerFromNetworkAsyncOperation::End(
    AsyncOperationSPtr const & operation)
{
    auto thisPtr = AsyncOperation::End<DetachContainerFromNetworkAsyncOperation>(operation);
    return thisPtr->Error;
}

void ContainerNetworkOperations::DetachContainerFromNetworkAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (!networkNames_.empty())
    {
        // Get overlay network names from the overlay network manager before getting the driver.
        // The overlay network names list returned is for live network drivers only.
        // This check prevents recreating the network driver after being deleted because 
        // all containers on the network have gone away.
        std::vector<std::wstring> existingNetworkNames;
        this->owner_.OverlayNetworkManagerObj.GetNetworkNames(existingNetworkNames);

        for (auto const & networkName : networkNames_)
        {
            if (std::find(existingNetworkNames.begin(), existingNetworkNames.end(), networkName) != existingNetworkNames.end())
            {
                auto operation = this->owner_.OverlayNetworkManagerObj.BeginGetOverlayNetworkDriver(
                    networkName,
                    this->nodeId_,
                    this->nodeName_,
                    this->nodeIpAddress_,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & operation)
                {
                    this->OnGetOverlayNetworkDriverCompleted(operation, false);
                },
                    thisSPtr);

                this->OnGetOverlayNetworkDriverCompleted(operation, true);
            }
            else
            {
                DecrementAndCheckPendingOperations(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            }
        }
    }
    else
    {
        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
    }
}

void ContainerNetworkOperations::DetachContainerFromNetworkAsyncOperation::OnGetOverlayNetworkDriverCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    OverlayNetworkDriverSPtr networkDriver;
    auto error = this->owner_.OverlayNetworkManagerObj.EndGetOverlayNetworkDriver(operation, networkDriver);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Get overlay network driver returned error {0}.",
        error);

    if (error.IsSuccess())
    {
        auto detachOperation = networkDriver->BeginDetachContainerFromNetwork(
            this->containerId_,
            timeoutHelper_.GetRemainingTime(),
            [this, networkDriver](AsyncOperationSPtr const & detachOperation)
        {
            this->OnDetachContainerFromNetworkCompleted(detachOperation, networkDriver, false);
        },
            operation->Parent);

        this->OnDetachContainerFromNetworkCompleted(detachOperation, networkDriver, true);
    }
    else
    {
        DecrementAndCheckPendingOperations(operation->Parent, error);
    }
}

void ContainerNetworkOperations::DetachContainerFromNetworkAsyncOperation::OnDetachContainerFromNetworkCompleted(
    AsyncOperationSPtr const & operation, 
    OverlayNetworkDriverSPtr networkDriver,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = networkDriver->EndDetachContainerFromNetwork(operation);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Detach container from network returned error {0}.",
        error);

    DecrementAndCheckPendingOperations(operation->Parent, error);
}

void ContainerNetworkOperations::DetachContainerFromNetworkAsyncOperation::DecrementAndCheckPendingOperations(
    AsyncOperationSPtr const & thisSPtr,
    ErrorCode const & error)
{
    AcquireExclusiveLock lock(lock_);

    if (!error.IsSuccess())
    {
        lastError_.ReadValue();
        lastError_ = error;
    }

    if (pendingOperationsCount_.load() == 0)
    {
        Assert::CodingError("Pending operation count is already 0");
    }

    uint64 pendingOperationsCount = --pendingOperationsCount_;

    WriteInfo(
        TraceType_NetworkOperations,
        "CheckDecrement count {0}.", pendingOperationsCount);

    if (pendingOperationsCount == 0)
    {
        TryComplete(thisSPtr, lastError_);
    }
}

ContainerNetworkOperations::AttachContainerToNetworkAsyncOperation::AttachContainerToNetworkAsyncOperation(
    IContainerActivator & owner,
    std::vector<OverlayNetworkContainerParametersSPtr> const & params,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    params_(params),
    timeoutHelper_(timeout)
{
    pendingOperationsCount_.store(params.size());
}

ContainerNetworkOperations::AttachContainerToNetworkAsyncOperation::~AttachContainerToNetworkAsyncOperation()
{
}

ErrorCode ContainerNetworkOperations::AttachContainerToNetworkAsyncOperation::AttachContainerToNetworkAsyncOperation::End(
    AsyncOperationSPtr const & operation)
{
    auto thisPtr = AsyncOperation::End<AttachContainerToNetworkAsyncOperation>(operation);
    return thisPtr->Error;
}

void ContainerNetworkOperations::AttachContainerToNetworkAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (!params_.empty())
    {
        for (auto const & param : params_)
        {
            auto operation = this->owner_.OverlayNetworkManagerObj.BeginGetOverlayNetworkDriver(
                param->NetworkName,
                param->NodeId,
                param->NodeName,
                param->NodeIpAddress,
                timeoutHelper_.GetRemainingTime(),
                [this, param](AsyncOperationSPtr const & operation)
            {
                this->OnGetOverlayNetworkDriverCompleted(operation, param, false);
            },
                thisSPtr);

            this->OnGetOverlayNetworkDriverCompleted(operation, param, true);
        }
    }
    else
    {
        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
    }
}

void ContainerNetworkOperations::AttachContainerToNetworkAsyncOperation::OnGetOverlayNetworkDriverCompleted(
    AsyncOperationSPtr const & operation, 
    OverlayNetworkContainerParametersSPtr const & param,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    OverlayNetworkDriverSPtr networkDriver;
    auto error = this->owner_.OverlayNetworkManagerObj.EndGetOverlayNetworkDriver(operation, networkDriver);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Get overlay network driver returned error {0}",
        error);

    if (error.IsSuccess())
    {
        auto attachOperation = networkDriver->BeginAttachContainerToNetwork(
            param->ContainerId,
            param->IPV4Address,
            param->MacAddress,
            param->DnsServerList,
            timeoutHelper_.GetRemainingTime(),
            [this, networkDriver](AsyncOperationSPtr const & attachOperation)
        {
            this->OnAttachContainerToNetworkCompleted(attachOperation, networkDriver, false);
        },
            operation->Parent);

        this->OnAttachContainerToNetworkCompleted(attachOperation, networkDriver, true);
    }
    else
    {
        DecrementAndCheckPendingOperations(operation->Parent, error);
    }
}

void ContainerNetworkOperations::AttachContainerToNetworkAsyncOperation::OnAttachContainerToNetworkCompleted(
    AsyncOperationSPtr const & operation,
    OverlayNetworkDriverSPtr networkDriver,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = networkDriver->EndAttachContainerToNetwork(operation);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Attach container to network returned error {0}.",
        error);

    DecrementAndCheckPendingOperations(operation->Parent, error);
}

void ContainerNetworkOperations::AttachContainerToNetworkAsyncOperation::DecrementAndCheckPendingOperations(
    AsyncOperationSPtr const & thisSPtr,
    ErrorCode const & error)
{
    AcquireExclusiveLock lock(lock_);

    if (!error.IsSuccess())
    {
        lastError_.ReadValue();
        lastError_ = error;
    }

    if (pendingOperationsCount_.load() == 0)
    {
        Assert::CodingError("Pending operation count is already 0.");
    }

    uint64 pendingOperationsCount = --pendingOperationsCount_;

    WriteInfo(
        TraceType_NetworkOperations,
        "CheckDecrement count {0}.", pendingOperationsCount);

    if (pendingOperationsCount == 0)
    {
        TryComplete(thisSPtr, lastError_);
    }
}

ContainerNetworkOperations::ContainerNetworkOperations(
    ComponentRootSPtr const & root)
    : RootedObject(*root)
{
}

ContainerNetworkOperations::~ContainerNetworkOperations()
{
}

Common::AsyncOperationSPtr ContainerNetworkOperations::BeginAssignOverlayNetworkResources(
    IContainerActivator & owner,
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & servicePackageId,
    std::map<wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ContainerNetworkOperations::AssignOverlayNetworkResourcesAsyncOperation>(
        owner,
        nodeId,
        nodeName,
        nodeIpAddress,
        servicePackageId,
        codePackageNetworkNames,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerNetworkOperations::EndAssignOverlayNetworkResources(
    Common::AsyncOperationSPtr const & operation,
    __out std::map<std::wstring, std::map<std::wstring, std::wstring>> & assignedNetworkResources)
{
    return ContainerNetworkOperations::AssignOverlayNetworkResourcesAsyncOperation::End(operation, assignedNetworkResources);
}

Common::AsyncOperationSPtr ContainerNetworkOperations::BeginReleaseOverlayNetworkResources(
    IContainerActivator & owner,
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & servicePackageId,
    std::vector<std::wstring> const & networkNames,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation>(
        owner,
        nodeId,
        nodeName,
        nodeIpAddress,
        servicePackageId,
        networkNames,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerNetworkOperations::EndReleaseOverlayNetworkResources(
    Common::AsyncOperationSPtr const & operation)
{
    return ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ContainerNetworkOperations::BeginCleanupAssignedOverlayNetworkResourcesToNode(
    IContainerActivator & owner,
    std::wstring const & nodeId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation>(
        owner,
        nodeId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerNetworkOperations::EndCleanupAssignedOverlayNetworkResourcesToNode(
    Common::AsyncOperationSPtr const & operation)
{
    return ContainerNetworkOperations::ReleaseOverlayNetworkResourcesAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ContainerNetworkOperations::BeginAttachContainerToNetwork(
    IContainerActivator & owner,
    std::vector<OverlayNetworkContainerParametersSPtr> const & params,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ContainerNetworkOperations::AttachContainerToNetworkAsyncOperation>(
        owner,
        params,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerNetworkOperations::EndAttachContainerToNetwork(
    Common::AsyncOperationSPtr const & operation)
{
    return ContainerNetworkOperations::AttachContainerToNetworkAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ContainerNetworkOperations::BeginDetachContainerFromNetwork(
    IContainerActivator & owner,
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & containerId,
    std::vector<std::wstring> const & networkNames,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ContainerNetworkOperations::DetachContainerFromNetworkAsyncOperation>(
        owner,
        nodeId,
        nodeName,
        nodeIpAddress,
        containerId,
        networkNames,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerNetworkOperations::EndDetachContainerFromNetwork(
    Common::AsyncOperationSPtr const & operation)
{
    return ContainerNetworkOperations::DetachContainerFromNetworkAsyncOperation::End(operation);
}

#if defined(PLATFORM_UNIX)
Common::AsyncOperationSPtr ContainerNetworkOperations::BeginSaveContainerNetworkParamsForLinuxIsolation(
    IContainerActivator & owner,
    std::vector<OverlayNetworkContainerParametersSPtr> const & params,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ContainerNetworkOperations::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation>(
        owner,
        params,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerNetworkOperations::EndSaveContainerNetworkParamsForLinuxIsolation(
    Common::AsyncOperationSPtr const & operation)
{
    return ContainerNetworkOperations::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ContainerNetworkOperations::BeginClearContainerNetworkParamsForLinuxIsolation(
    IContainerActivator & owner,
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & containerId,
    std::vector<std::wstring> const & networkNames,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ContainerNetworkOperations::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation>(
        owner,
        nodeId,
        nodeName,
        nodeIpAddress,
        containerId,
        networkNames,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerNetworkOperations::EndClearContainerNetworkParamsForLinuxIsolation(
    Common::AsyncOperationSPtr const & operation)
{
    return ContainerNetworkOperations::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation::End(operation);
}
#endif

Common::AsyncOperationSPtr ContainerNetworkOperations::BeginUpdateRoutes(
    IContainerActivator & owner,
    OverlayNetworkRoutingInformationSPtr const & routingInfo,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ContainerNetworkOperations::UpdateRoutesAsyncOperation>(
        owner,
        routingInfo,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerNetworkOperations::EndUpdateRoutes(
    Common::AsyncOperationSPtr const & operation)
{
    return ContainerNetworkOperations::UpdateRoutesAsyncOperation::End(operation);
}

ContainerNetworkOperations::UpdateRoutesAsyncOperation::UpdateRoutesAsyncOperation(
    IContainerActivator & owner,
    OverlayNetworkRoutingInformationSPtr const & routingInfo,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    routingInfo_(routingInfo),
    timeoutHelper_(timeout)
{
}

ContainerNetworkOperations::UpdateRoutesAsyncOperation::~UpdateRoutesAsyncOperation()
{
}

ErrorCode ContainerNetworkOperations::UpdateRoutesAsyncOperation::UpdateRoutesAsyncOperation::End(
    AsyncOperationSPtr const & operation)
{
    auto thisPtr = AsyncOperation::End<UpdateRoutesAsyncOperation>(operation);
    return thisPtr->Error;
}

void ContainerNetworkOperations::UpdateRoutesAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = this->owner_.OverlayNetworkManagerObj.BeginUpdateOverlayNetworkRoutes(
        this->routingInfo_,
        timeoutHelper_.GetRemainingTime(),
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnUpdateRoutesAsyncOperationCompleted(operation, false);
    },
        thisSPtr);

    this->OnUpdateRoutesAsyncOperationCompleted(operation, true);
}

void ContainerNetworkOperations::UpdateRoutesAsyncOperation::OnUpdateRoutesAsyncOperationCompleted(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return; 
    }

    auto error = this->owner_.OverlayNetworkManagerObj.EndUpdateOverlayNetworkRoutes(operation);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Update overlay network routes returned error {0}.",
        error);

    TryComplete(operation->Parent, error);
}

#if defined(PLATFORM_UNIX)
ContainerNetworkOperations::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation(
    IContainerActivator & owner,
    std::vector<OverlayNetworkContainerParametersSPtr> const & params,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    params_(params),
    timeoutHelper_(timeout)
{
    pendingOperationsCount_.store(params.size());
}

ContainerNetworkOperations::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation::~SaveContainerNetworkParamsForLinuxIsolationAsyncOperation()
{
}

ErrorCode ContainerNetworkOperations::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation::End(
    AsyncOperationSPtr const & operation)
{
    auto thisPtr = AsyncOperation::End<SaveContainerNetworkParamsForLinuxIsolationAsyncOperation>(operation);
    return thisPtr->Error;
}

void ContainerNetworkOperations::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (!params_.empty())
    {
        for (auto const & param : params_)
        {
            if (StringUtility::CompareCaseInsensitive(param->NetworkName, Common::ContainerEnvironment::ContainerNetworkName) == 0 ||
                StringUtility::CompareCaseInsensitive(param->NetworkName, Common::ContainerEnvironment::ContainerNatNetworkTypeName) == 0)
            {
                auto operation = this->owner_.OverlayNetworkManagerObj.NetworkPlugin->BeginAttachContainerToNetwork(
                    param,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & operation)
                {
                    this->OnSaveContainerNetworkParamsForLinuxIsolationCompleted(operation, false);
                },
                    thisSPtr);

                this->OnSaveContainerNetworkParamsForLinuxIsolationCompleted(operation, true);
            }
            else
            {
                auto operation = this->owner_.OverlayNetworkManagerObj.BeginGetOverlayNetworkDriver(
                    param->NetworkName,
                    param->NodeId,
                    param->NodeName,
                    param->NodeIpAddress,
                    timeoutHelper_.GetRemainingTime(),
                    [this, param](AsyncOperationSPtr const & operation)
                {
                    this->OnGetOverlayNetworkDriverCompleted(operation, param, false);
                },
                    thisSPtr);

                this->OnGetOverlayNetworkDriverCompleted(operation, param, true);
            }
        }
    }
    else
    {
        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
    }
}

void ContainerNetworkOperations::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation::OnSaveContainerNetworkParamsForLinuxIsolationCompleted(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = this->owner_.OverlayNetworkManagerObj.NetworkPlugin->EndAttachContainerToNetwork(operation);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Save container network params for linux isolation returned error {0}.",
        error);

    DecrementAndCheckPendingOperations(operation->Parent, error);
}

void ContainerNetworkOperations::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation::OnGetOverlayNetworkDriverCompleted(
    AsyncOperationSPtr const & operation,
    OverlayNetworkContainerParametersSPtr const & param,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    OverlayNetworkDriverSPtr networkDriver;
    auto error = this->owner_.OverlayNetworkManagerObj.EndGetOverlayNetworkDriver(operation, networkDriver);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Get overlay network driver returned error {0}",
        error);

    if (error.IsSuccess())
    {
        auto attachOperation = networkDriver->BeginAttachContainerToNetwork(
            param->ContainerId,
            param->IPV4Address,
            param->MacAddress,
            param->DnsServerList,
            timeoutHelper_.GetRemainingTime(),
            [this, networkDriver](AsyncOperationSPtr const & attachOperation)
        {
            this->OnAttachContainerToNetworkCompleted(attachOperation, networkDriver, false);
        },
            operation->Parent);

        this->OnAttachContainerToNetworkCompleted(attachOperation, networkDriver, true);
    }
    else
    {
        DecrementAndCheckPendingOperations(operation->Parent, error);
    }
}

void ContainerNetworkOperations::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation::OnAttachContainerToNetworkCompleted(
    AsyncOperationSPtr const & operation,
    OverlayNetworkDriverSPtr networkDriver,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = networkDriver->EndAttachContainerToNetwork(operation);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Save container network params for linux isolation returned error {0}.",
        error);

    DecrementAndCheckPendingOperations(operation->Parent, error);
}

void ContainerNetworkOperations::SaveContainerNetworkParamsForLinuxIsolationAsyncOperation::DecrementAndCheckPendingOperations(
    AsyncOperationSPtr const & thisSPtr,
    ErrorCode const & error)
{
    AcquireExclusiveLock lock(lock_);

    if (!error.IsSuccess())
    {
        lastError_.ReadValue();
        lastError_ = error;
    }

    if (pendingOperationsCount_.load() == 0)
    {
        Assert::CodingError("Pending operation count is already 0.");
    }

    uint64 pendingOperationsCount = --pendingOperationsCount_;

    WriteInfo(
        TraceType_NetworkOperations,
        "CheckDecrement count {0}.", pendingOperationsCount);

    if (pendingOperationsCount == 0)
    {
        TryComplete(thisSPtr, lastError_);
    }
}

ContainerNetworkOperations::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation(
    IContainerActivator & owner,
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & containerId,
    std::vector<std::wstring> const & networkNames,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    networkNames_(networkNames),
    nodeId_(nodeId),
    nodeName_(nodeName),
    nodeIpAddress_(nodeIpAddress),
    containerId_(containerId),
    timeoutHelper_(timeout)
{
    pendingOperationsCount_.store(networkNames.size());
}

ContainerNetworkOperations::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation::~ClearContainerNetworkParamsForLinuxIsolationAsyncOperation()
{
}

ErrorCode ContainerNetworkOperations::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation::End(
    AsyncOperationSPtr const & operation)
{
    auto thisPtr = AsyncOperation::End<ClearContainerNetworkParamsForLinuxIsolationAsyncOperation>(operation);
    return thisPtr->Error;
}

void ContainerNetworkOperations::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (!networkNames_.empty())
    {
        // Get overlay network names from the overlay network manager before getting the driver.
        // The overlay network names list returned is for live network drivers only.
        // This check prevents recreating the network driver after being deleted because 
        // all containers on the network have gone away.
        std::vector<std::wstring> existingNetworkNames;
        this->owner_.OverlayNetworkManagerObj.GetNetworkNames(existingNetworkNames);

        for (auto const & networkName : networkNames_)
        {
            if (StringUtility::CompareCaseInsensitive(networkName, Common::ContainerEnvironment::ContainerNetworkName) == 0 || 
                StringUtility::CompareCaseInsensitive(networkName, Common::ContainerEnvironment::ContainerNatNetworkTypeName) == 0)
            {
                auto operation = this->owner_.OverlayNetworkManagerObj.NetworkPlugin->BeginDetachContainerFromNetwork(
                    containerId_,
                    networkName,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & operation)
                {
                    this->OnClearContainerNetworkParamsForLinuxIsolationCompleted(operation, false);
                },
                    thisSPtr);

                this->OnClearContainerNetworkParamsForLinuxIsolationCompleted(operation, true);
            }
            else
            {
                if (std::find(existingNetworkNames.begin(), existingNetworkNames.end(), networkName) != existingNetworkNames.end())
                {
                    auto operation = this->owner_.OverlayNetworkManagerObj.BeginGetOverlayNetworkDriver(
                        networkName,
                        nodeId_,
                        nodeName_,
                        nodeIpAddress_,
                        timeoutHelper_.GetRemainingTime(),
                        [this](AsyncOperationSPtr const & operation)
                    {
                        this->OnGetOverlayNetworkDriverCompleted(operation, false);
                    },
                        thisSPtr);

                    this->OnGetOverlayNetworkDriverCompleted(operation, true);
                }
                else
                {
                    DecrementAndCheckPendingOperations(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                }
            }
        }
    }
    else
    {
        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
    }
}

void ContainerNetworkOperations::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation::OnClearContainerNetworkParamsForLinuxIsolationCompleted(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = this->owner_.OverlayNetworkManagerObj.NetworkPlugin->EndDetachContainerFromNetwork(operation);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Clear container network params for linux isolation returned error {0}.",
        error);

    DecrementAndCheckPendingOperations(operation->Parent, error);
}

void ContainerNetworkOperations::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation::OnGetOverlayNetworkDriverCompleted(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    OverlayNetworkDriverSPtr networkDriver;
    auto error = this->owner_.OverlayNetworkManagerObj.EndGetOverlayNetworkDriver(operation, networkDriver);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Get overlay network driver returned error {0}",
        error);

    if (error.IsSuccess())
    {
        auto detachOperation = networkDriver->BeginDetachContainerFromNetwork(
            this->containerId_,
            timeoutHelper_.GetRemainingTime(),
            [this, networkDriver](AsyncOperationSPtr const & detachOperation)
        {
            this->OnDetachContainerFromNetworkCompleted(detachOperation, networkDriver, false);
        },
            operation->Parent);

        this->OnDetachContainerFromNetworkCompleted(detachOperation, networkDriver, true);
    }
    else
    {
        DecrementAndCheckPendingOperations(operation->Parent, error);
    }
}

void ContainerNetworkOperations::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation::OnDetachContainerFromNetworkCompleted(
    AsyncOperationSPtr const & operation,
    OverlayNetworkDriverSPtr networkDriver,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = networkDriver->EndDetachContainerFromNetwork(operation);

    WriteInfo(
        TraceType_NetworkOperations,
        owner_.Root.TraceId,
        "Detach container from network returned error {0}.",
        error);

    DecrementAndCheckPendingOperations(operation->Parent, error);
}

void ContainerNetworkOperations::ClearContainerNetworkParamsForLinuxIsolationAsyncOperation::DecrementAndCheckPendingOperations(
    AsyncOperationSPtr const & thisSPtr,
    ErrorCode const & error)
{
    AcquireExclusiveLock lock(lock_);

    if (!error.IsSuccess())
    {
        lastError_.ReadValue();
        lastError_ = error;
    }

    if (pendingOperationsCount_.load() == 0)
    {
        Assert::CodingError("Pending operation count is already 0");
    }

    uint64 pendingOperationsCount = --pendingOperationsCount_;

    WriteInfo(
        TraceType_NetworkOperations,
        "CheckDecrement count {0}.", pendingOperationsCount);

    if (pendingOperationsCount == 0)
    {
        TryComplete(thisSPtr, lastError_);
    }
}
#endif