// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceOverlayNetworkResourceProvider("OverlayNetworkResourceProvider");

namespace Hosting2
{
    // ********************************************************************************************************************
    // OverlayNetworkResourceProvider::ReleaseAllNetworkResourcesForNodeAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkResourceProvider::ReleaseAllNetworkResourcesForNodeAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(ReleaseAllNetworkResourcesForNodeAsyncOperation)

    public:
        ReleaseAllNetworkResourcesForNodeAsyncOperation(
            OverlayNetworkResourceProvider & owner,
            wstring const & nodeId,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            nodeId_(nodeId),
            timeoutHelper_(timeout)
        {
        }

        virtual ~ReleaseAllNetworkResourcesForNodeAsyncOperation()
        {
        }

        static ErrorCode ReleaseAllNetworkResourcesForNodeAsyncOperation::End(
            AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<ReleaseAllNetworkResourcesForNodeAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            if (!this->owner_.Initialized)
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidState));
            }

            std::vector<std::wstring> reservationIds;
            { // lock
                AcquireExclusiveLock lock(this->owner_.networkResourceProviderLock_);

                auto nodeIdIter = this->owner_.reservedCodePackages_.find(this->nodeId_);
                if (nodeIdIter != this->owner_.reservedCodePackages_.end())
                {
                    for (auto it = nodeIdIter->second.begin(); it != nodeIdIter->second.end(); it++)
                    {
                        auto servicePackageId = it->first;
                        for (auto const & cp : it->second)
                        {
                            auto reservationId = wformatString("{0}{1}", servicePackageId, cp);
                            reservationIds.push_back(reservationId);
                        }
                    }
                }
            }

            if (!reservationIds.empty())
            {
                pendingOperationsCount_.store(reservationIds.size());

                for (auto const & reservationId : reservationIds)
                {
                    auto operation = this->owner_.ipam_->BeginReleaseWithRetry(
                        reservationId,
                        timeoutHelper_.GetRemainingTime(),
                        [this, reservationId](AsyncOperationSPtr const & operation)
                    {
                        this->OnReleaseAllNetworkResourcesForNodeCompleted(operation, false, reservationId);
                    },
                        thisSPtr);

                    this->OnReleaseAllNetworkResourcesForNodeCompleted(operation, true, reservationId);
                }
            }
            else
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
            }
        }

        void OnReleaseAllNetworkResourcesForNodeCompleted(
            AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously,
            wstring reservationId)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.ipam_->EndReleaseWithRetry(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkResourceProvider,
                owner_.Root.TraceId,
                "End(ReleaseAllNetworkResourcesForNodeCompleted): Reservation Id={0} ErrorCode={1}",
                reservationId,
                error);

            if (error.IsSuccess())
            {
                { // lock
                    AcquireExclusiveLock lock(this->owner_.networkResourceProviderLock_);

                    // If successful, remove reservation id from reservation id code package map
                    auto reservationIdIter = this->owner_.reservationIdCodePackageMap_.find(reservationId);
                    if (reservationIdIter != this->owner_.reservationIdCodePackageMap_.end())
                    {
                        this->owner_.reservationIdCodePackageMap_.erase(reservationId);
                    }
                }
            }

            DecrementAndCheckPendingOperations(operation->Parent, error);
        }

        void DecrementAndCheckPendingOperations(
            AsyncOperationSPtr const & thisSPtr,
            ErrorCode const & error)
        {
            AcquireExclusiveLock lock(this->lock_);

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
                TraceOverlayNetworkResourceProvider,
                "CheckDecrement count {0}.", pendingOperationsCount);

            if (pendingOperationsCount == 0)
            {
                if (lastError_.IsSuccess())
                {
                    { // lock
                        AcquireExclusiveLock resourceLock(this->owner_.networkResourceProviderLock_);

                        auto nodeIdIter = this->owner_.reservedCodePackages_.find(this->nodeId_);
                        if (nodeIdIter != this->owner_.reservedCodePackages_.end())
                        {
                            this->owner_.reservedCodePackages_.erase(nodeIdIter);
                        }
                    }
                }

                TryComplete(thisSPtr, lastError_);
            }
        }

    private:
        OverlayNetworkResourceProvider & owner_;
        TimeoutHelper timeoutHelper_;
        wstring nodeId_;
        atomic_uint64 pendingOperationsCount_;
        Common::RwLock lock_;
        ErrorCode lastError_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkResourceProvider::ReleaseNetworkResourcesAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkResourceProvider::ReleaseNetworkResourcesAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(ReleaseNetworkResourcesAsyncOperation)

    public:
        ReleaseNetworkResourcesAsyncOperation(
            OverlayNetworkResourceProvider & owner,
            wstring const & nodeId,
            wstring const & servicePackageId,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            nodeId_(nodeId),
            servicePackageId_(servicePackageId),
            timeoutHelper_(timeout)
        {
        }

        virtual ~ReleaseNetworkResourcesAsyncOperation()
        {
        }

        static ErrorCode ReleaseNetworkResourcesAsyncOperation::End(
            AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<ReleaseNetworkResourcesAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            if (!this->owner_.Initialized)
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidState));
            }

            std::vector<std::wstring> reservationIds;
            { // lock
                AcquireExclusiveLock lock(this->owner_.networkResourceProviderLock_);

                auto nodeIdIter = this->owner_.reservedCodePackages_.find(this->nodeId_);
                if (nodeIdIter != this->owner_.reservedCodePackages_.end())
                {
                    auto servicePkgIter = nodeIdIter->second.find(this->servicePackageId_);
                    if (servicePkgIter != nodeIdIter->second.end())
                    {
                        for (auto const & cp : servicePkgIter->second)
                        {
                            auto reservationId = wformatString("{0}{1}", this->servicePackageId_, cp);
                            reservationIds.push_back(reservationId);
                        }
                    }
                }
            }

            if (!reservationIds.empty())
            {
                pendingOperationsCount_.store(reservationIds.size());

                for (auto const & reservationId : reservationIds)
                {
                    auto operation = this->owner_.ipam_->BeginReleaseWithRetry(
                        reservationId,
                        timeoutHelper_.GetRemainingTime(),
                        [this, reservationId](AsyncOperationSPtr const & operation)
                    {
                        this->OnReleaseNetworkResourcesCompleted(operation, false, reservationId);
                    },
                        thisSPtr);

                    this->OnReleaseNetworkResourcesCompleted(operation, true, reservationId);
                }
            }
            else
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
            }
        }

        void OnReleaseNetworkResourcesCompleted(
            AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously,
            wstring reservationId)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.ipam_->EndReleaseWithRetry(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkResourceProvider,
                owner_.Root.TraceId,
                "End(ReleaseNetworkResourcesCompleted): Reservation Id={0} ErrorCode={1}",
                reservationId,
                error);

            if (error.IsSuccess())
            {
                { // lock
                    AcquireExclusiveLock lock(this->owner_.networkResourceProviderLock_);

                    // If successful, remove reservation id from reservation id code package map
                    auto reservationIdIter = this->owner_.reservationIdCodePackageMap_.find(reservationId);
                    if (reservationIdIter != this->owner_.reservationIdCodePackageMap_.end())
                    {
                        vector<wstring> parts;
                        StringUtility::Split<wstring>(reservationIdIter->second, parts, L",", true);
                        if (parts.size() == 3)
                        {
                            wstring codePackage = parts[2];

                            this->owner_.reservationIdCodePackageMap_.erase(reservationId);

                            auto nodeIdIter = this->owner_.reservedCodePackages_.find(this->nodeId_);
                            if (nodeIdIter != this->owner_.reservedCodePackages_.end())
                            {
                                auto servicePkgIter = nodeIdIter->second.find(this->servicePackageId_);
                                if (servicePkgIter != nodeIdIter->second.end())
                                {
                                    auto cpIter = find(servicePkgIter->second.begin(), servicePkgIter->second.end(), codePackage);
                                    if (cpIter != servicePkgIter->second.end())
                                    {
                                        servicePkgIter->second.erase(cpIter);
                                    }
                                }

                                // erase service package id, if all code package resources have been released
                                if (servicePkgIter->second.size() == 0)
                                {
                                    nodeIdIter->second.erase(servicePkgIter);
                                }

                                // erase node id, if all service package resources have been released
                                if (nodeIdIter->second.size() == 0)
                                {
                                    this->owner_.reservedCodePackages_.erase(nodeIdIter);
                                }
                            }
                        }
                    }
                }
            }

            DecrementAndCheckPendingOperations(operation->Parent, error);
        }

        void DecrementAndCheckPendingOperations(
            AsyncOperationSPtr const & thisSPtr,
            ErrorCode const & error)
        {
            AcquireExclusiveLock lock(this->lock_);

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
                TraceOverlayNetworkResourceProvider,
                "CheckDecrement count {0}.", pendingOperationsCount);

            if (pendingOperationsCount == 0)
            {
                TryComplete(thisSPtr, lastError_);
            }
        }

    private:
        OverlayNetworkResourceProvider & owner_;
        TimeoutHelper timeoutHelper_;
        wstring nodeId_;
        wstring servicePackageId_;
        atomic_uint64 pendingOperationsCount_;
        Common::RwLock lock_;
        ErrorCode lastError_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkResourceProvider::AcquireNetworkResourcesAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkResourceProvider::AcquireNetworkResourcesAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(AcquireNetworkResourcesAsyncOperation)

    public:
        AcquireNetworkResourcesAsyncOperation(
            OverlayNetworkResourceProvider & owner,
            wstring const & nodeId,
            wstring const & servicePackageId,
            vector<wstring> const & codePackageNames,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            nodeId_(nodeId),
            servicePackageId_(servicePackageId),
            codePackageNames_(codePackageNames),
            timeoutHelper_(timeout)
        {
        }

        virtual ~AcquireNetworkResourcesAsyncOperation()
        {
        }

        static ErrorCode AcquireNetworkResourcesAsyncOperation::End(
            AsyncOperationSPtr const & operation,
            std::vector<std::wstring> & assignedNetworkResources)
        {
            auto thisPtr = AsyncOperation::End<AcquireNetworkResourcesAsyncOperation>(operation);
            if (thisPtr->Error.IsSuccess())
            {
                assignedNetworkResources = move(thisPtr->assignedNetworkResources_);
            }
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            if (!this->owner_.Initialized)
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidState));
            }

            if (!this->codePackageNames_.empty())
            {
                pendingOperationsCount_.store(this->codePackageNames_.size());

                for (auto const & codePackageName : this->codePackageNames_)
                {
                    auto reservationId = wformatString("{0}{1}", this->servicePackageId_, codePackageName);

                    auto operation = this->owner_.ipam_->BeginReserveWithRetry(
                        reservationId,
                        timeoutHelper_.GetRemainingTime(),
                        [this, reservationId, codePackageName](AsyncOperationSPtr const & operation)
                    {
                        this->OnReserveNetworkResourcesCompleted(operation, false, reservationId, codePackageName);
                    },
                        thisSPtr);

                    this->OnReserveNetworkResourcesCompleted(operation, true, reservationId, codePackageName);
                }
            }
            else
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
            }
        }

        void OnReserveNetworkResourcesCompleted(
            AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously,
            wstring reservationId,
            wstring codePackageName)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            OverlayNetworkResourceSPtr networkResource;
            auto error = this->owner_.ipam_->EndReserveWithRetry(operation, networkResource);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkResourceProvider,
                owner_.Root.TraceId,
                "End(ReserveNetworkResourcesCompleted): Reservation Id={0} ErrorCode={1}",
                reservationId,
                error);

            if (error.IsSuccess())
            {
                auto reservedIPAddress = IPHelper::Format(networkResource->IPV4Address);
                auto reservedMACAddress = MACHelper::Format(networkResource->MacAddress);
                auto assignedNetworkResource = wformatString("{0},{1},{2}", reservedIPAddress, reservedMACAddress, codePackageName);
                WriteInfo(
                    TraceOverlayNetworkResourceProvider,
                    "Assigned network resource ip address={0} mac address={1} reservation id={2}",
                    reservedIPAddress,
                    reservedMACAddress,
                    reservationId);

                { //lock
                    AcquireExclusiveLock lock(this->lock_);

                    this->assignedNetworkResources_.push_back(assignedNetworkResource);
                    this->codePackageMap_[reservationId] = codePackageName;
                }
            }
            else
            {
                error = ErrorCodeValue::OverlayNetworkResourceProviderAddressRangeExhausted;
            }

            DecrementAndCheckAcquirePendingOperations(operation->Parent, error);
        }

        void DecrementAndCheckAcquirePendingOperations(
            AsyncOperationSPtr const & thisSPtr,
            ErrorCode const & error)
        {
            AcquireExclusiveLock lock(this->lock_);

            if (!error.IsSuccess())
            {
                lastAcquireError_.ReadValue();
                lastAcquireError_ = error;
            }

            if (pendingOperationsCount_.load() == 0)
            {
                Assert::CodingError("Pending operation count is already 0.");
            }

            uint64 pendingOperationsCount = --pendingOperationsCount_;

            WriteInfo(
                TraceOverlayNetworkResourceProvider,
                "CheckDecrement count {0}.", pendingOperationsCount);

            if (pendingOperationsCount == 0)
            {
                if (lastAcquireError_.IsSuccess())
                {
                    { // lock
                        AcquireExclusiveLock resourceLock(this->owner_.networkResourceProviderLock_);

                        // Add to reserved ip map if it does not exist
                        auto nodeIdIter = this->owner_.reservedCodePackages_.find(this->nodeId_);
                        if (nodeIdIter == this->owner_.reservedCodePackages_.end())
                        {
                            this->owner_.reservedCodePackages_.insert(make_pair(this->nodeId_, std::map<wstring, std::vector<wstring>>()));
                            nodeIdIter = this->owner_.reservedCodePackages_.find(this->nodeId_);
                        }

                        auto servicePkgIter = nodeIdIter->second.find(this->servicePackageId_);
                        if (servicePkgIter != nodeIdIter->second.end())
                        {
                            auto existingCodePackages = servicePkgIter->second;
                            for (auto const & cp : this->codePackageNames_)
                            {
                                if (find(existingCodePackages.begin(), existingCodePackages.end(), cp) == existingCodePackages.end())
                                {
                                    servicePkgIter->second.push_back(cp);
                                }
                            }
                        }
                        else
                        {
                            nodeIdIter->second[this->servicePackageId_] = this->codePackageNames_;
                        }

                        // Add reservation ids to reservation id code package map
                        for (auto const & cpm : this->codePackageMap_)
                        {
                            auto reservationIdIter = this->owner_.reservationIdCodePackageMap_.find(cpm.first);
                            if (reservationIdIter == this->owner_.reservationIdCodePackageMap_.end())
                            {
                                auto codePkg = wformatString("{0},{1},{2}", this->nodeId_, this->servicePackageId_, cpm.second);
                                this->owner_.reservationIdCodePackageMap_.insert(make_pair(cpm.first, codePkg));
                            }
                        }
                    }
                }

                ReleaseAcquiredResourcesOnError(thisSPtr, lastAcquireError_);
            }
        }

        void ReleaseAcquiredResourcesOnError(
            AsyncOperationSPtr const & thisSPtr,
            ErrorCode const & acquireError)
        {
            if (!acquireError.IsSuccess())
            {
                pendingOperationsCount_.store(this->codePackageMap_.size());

                if (!this->codePackageMap_.empty())
                {
                    for (auto const & cpm : this->codePackageMap_)
                    {
                        wstring reservationId = cpm.first;
                        auto operation = this->owner_.ipam_->BeginReleaseWithRetry(
                            reservationId,
                            timeoutHelper_.GetRemainingTime(),
                            [this, reservationId, acquireError](AsyncOperationSPtr const & operation)
                        {
                            this->OnReleaseNetworkResourcesCompleted(operation, false, reservationId, acquireError);
                        },
                            thisSPtr);

                        this->OnReleaseNetworkResourcesCompleted(operation, true, reservationId, acquireError);
                    }
                }
                else
                {
                    TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                }
            }
            else
            {
                TryComplete(thisSPtr, acquireError);
            }
        }

        void OnReleaseNetworkResourcesCompleted(
            AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously,
            wstring reservationId,
            ErrorCode const & acquireError)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto releaseError = this->owner_.ipam_->EndReleaseWithRetry(operation);

            WriteTrace(
                releaseError.ToLogLevel(),
                TraceOverlayNetworkResourceProvider,
                owner_.Root.TraceId,
                "End(ReleaseNetworkResourcesCompleted): Reservation Id={0} ErrorCode={1}",
                reservationId,
                releaseError);

            DecrementAndCheckReleasePendingOperations(operation->Parent, releaseError, acquireError);
        }

        void DecrementAndCheckReleasePendingOperations(
            AsyncOperationSPtr const & thisSPtr,
            ErrorCode const & releaseError,
            ErrorCode const & acquireError)
        {
            AcquireExclusiveLock lock(this->lock_);

            if (!releaseError.IsSuccess())
            {
                lastReleaseError_.ReadValue();
                lastReleaseError_ = releaseError;
            }

            if (pendingOperationsCount_.load() == 0)
            {
                Assert::CodingError("Pending operation count is already 0.");
            }

            uint64 pendingOperationsCount = --pendingOperationsCount_;

            WriteInfo(
                TraceOverlayNetworkResourceProvider,
                "CheckDecrement count {0}.", pendingOperationsCount);

            if (pendingOperationsCount == 0)
            {
                this->assignedNetworkResources_.clear();
                TryComplete(thisSPtr, acquireError);
            }
        }

    private:
        OverlayNetworkResourceProvider & owner_;
        TimeoutHelper timeoutHelper_;
        wstring nodeId_;
        wstring servicePackageId_;
        vector<wstring> codePackageNames_;
        vector<wstring> assignedNetworkResources_;
        map<wstring, wstring> codePackageMap_;
        atomic_uint64 pendingOperationsCount_;
        Common::RwLock lock_;
        ErrorCode lastAcquireError_;
        ErrorCode lastReleaseError_;
    };

    OverlayNetworkResourceProvider::OverlayNetworkResourceProvider(
        ComponentRootSPtr const & root,
        __in OverlayNetworkDefinitionSPtr networkDefinition,
        InternalReplenishNetworkResourcesCallback const & internalReplenishNetworkResourcesCallback,
        GhostChangeCallback const & ghostChangeCallback)
        : RootedObject(*root),
        networkDefinition_(networkDefinition),
        internalReplenishNetworkResourcesCallback_(internalReplenishNetworkResourcesCallback),
        ghostChangeCallback_(ghostChangeCallback),
        networkResourceProviderLock_(),
        ipamInitialized_(false),
        reservedCodePackages_(),
        reservationIdCodePackageMap_()
    {
        auto ipam = make_shared<OverlayIPAM>(
            root,
            this->internalReplenishNetworkResourcesCallback_,
            this->ghostChangeCallback_,
            HostingConfig::GetConfig().ReplenishOverlayNetworkReservationPoolRetryInterval);
        this->ipam_ = move(ipam);
    }

    OverlayNetworkResourceProvider::OverlayNetworkResourceProvider(
        ComponentRootSPtr const & root,
        __in IOverlayIPAMSPtr ipamClient,
        __in OverlayNetworkDefinitionSPtr networkDefinition,
        InternalReplenishNetworkResourcesCallback const & internalReplenishNetworkResourcesCallback,
        GhostChangeCallback const & ghostChangeCallback)
        : RootedObject(*root),
        ipam_(ipamClient),
        networkDefinition_(networkDefinition),
        internalReplenishNetworkResourcesCallback_(internalReplenishNetworkResourcesCallback),
        ghostChangeCallback_(ghostChangeCallback),
        networkResourceProviderLock_(),
        ipamInitialized_(false),
        reservedCodePackages_(),
        reservationIdCodePackageMap_()
    {
    }

    OverlayNetworkResourceProvider::~OverlayNetworkResourceProvider()
    {
    }

    ErrorCode OverlayNetworkResourceProvider::OnOpen()
    {
        auto openErrorCode = this->ipam_->Open();

        if (openErrorCode.IsSuccess())
        {
            this->OnNewIpamData(this->networkDefinition_->IpMacAddressMapToBeAdded, this->networkDefinition_->IpMacAddressMapToBeRemoved);

            this->ipamInitialized_ = true;
        }

        return openErrorCode;
    }

    ErrorCode OverlayNetworkResourceProvider::OnClose()
    {
        { //lock
            AcquireExclusiveLock lock(this->networkResourceProviderLock_);

            this->reservedCodePackages_.clear();
            this->reservationIdCodePackageMap_.clear();
            this->ipamInitialized_ = false;
            this->ipam_->Close();
        }

        return ErrorCode::Success();
    }

    void OverlayNetworkResourceProvider::OnAbort()
    {
        OnClose().ReadValue();
    }

    Common::AsyncOperationSPtr OverlayNetworkResourceProvider::BeginAcquireNetworkResources(
        std::wstring const & nodeId,
        std::wstring const & servicePackageId,
        std::vector<std::wstring> const & codePackageNames,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<AcquireNetworkResourcesAsyncOperation>(
            *this,
            nodeId,
            servicePackageId,
            codePackageNames,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkResourceProvider::EndAcquireNetworkResources(
        Common::AsyncOperationSPtr const & operation,
        __out std::vector<std::wstring> & assignedNetworkResources)
    {
        return AcquireNetworkResourcesAsyncOperation::End(operation, assignedNetworkResources);
    }
 
    Common::AsyncOperationSPtr OverlayNetworkResourceProvider::BeginReleaseAllNetworkResourcesForNode(
        std::wstring const & nodeId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ReleaseAllNetworkResourcesForNodeAsyncOperation>(
            *this,
            nodeId,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkResourceProvider::EndReleaseAllNetworkResourcesForNode(
        Common::AsyncOperationSPtr const & operation)
    {
        return ReleaseAllNetworkResourcesForNodeAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr OverlayNetworkResourceProvider::BeginReleaseNetworkResources(
        std::wstring const & nodeId,
        std::wstring const & servicePackageId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ReleaseNetworkResourcesAsyncOperation>(
            *this,
            nodeId,
            servicePackageId,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkResourceProvider::EndReleaseNetworkResources(
        Common::AsyncOperationSPtr const & operation)
    {
        return ReleaseNetworkResourcesAsyncOperation::End(operation);
    }

    void OverlayNetworkResourceProvider::GetReservedCodePackages(std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> & reservedCodePackages,
        std::map<std::wstring, std::wstring> & reservationIdCodePackageMap)
    {
        // makes a copy of the reserved code packages and reservation id map
        { // lock
            AcquireExclusiveLock lock(this->networkResourceProviderLock_);

            for (auto const & r : this->reservedCodePackages_)
            {
                reservedCodePackages.insert(make_pair(r.first, std::map<wstring, std::vector<wstring>>()));
                for (auto const & s : r.second)
                {
                    auto nodeIter = reservedCodePackages.find(r.first);
                    vector<std::wstring> codePackages;
                    for (auto const & c : s.second)
                    {
                        codePackages.push_back(c);
                    }
                    nodeIter->second[s.first] = codePackages;
                }
            }

            for (auto const & r : this->reservationIdCodePackageMap_)
            {
                reservationIdCodePackageMap.insert(make_pair(r.first, r.second));
            }
        }
    }

    void OverlayNetworkResourceProvider::OnNewIpamData(std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeAdded,
        std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeRemoved)
    {
        AcquireExclusiveLock lock(this->networkResourceProviderLock_);
        
        // Overlay network resources to be added
        std::unordered_set<OverlayNetworkResourceSPtr> networkResourcesToBeAdded = this->PopulateNetworkResources(ipMacAddressMapToBeAdded);

        // Overlay network resources to be deleted
        std::unordered_set<OverlayNetworkResourceSPtr> networkResourcesToBeRemoved = this->PopulateNetworkResources(ipMacAddressMapToBeRemoved);

        // update reservation pool
        this->ipam_->OnNewIpamData(networkResourcesToBeAdded, networkResourcesToBeRemoved);
    }

    std::unordered_set<OverlayNetworkResourceSPtr> OverlayNetworkResourceProvider::PopulateNetworkResources(std::map<std::wstring, std::wstring> const & ipMacAddressMap)
    {
        std::unordered_set<OverlayNetworkResourceSPtr> networkResources;

        if (ipMacAddressMap.size() > 0)
        {
            for (auto const & ipMacPair : ipMacAddressMap)
            {
                UINT ip;
                if (IPHelper::TryParse(ipMacPair.first, ip))
                {
                    UINT64 mac;
                    if (MACHelper::TryParse(ipMacPair.second, mac))
                    {
                        auto nr = make_shared<OverlayNetworkResource>(ip, mac);
                        auto result = networkResources.insert(nr);
                        if (!result.second)
                        {
                            WriteInfo(
                                TraceOverlayNetworkResourceProvider,
                                "Overlay network resource ip address: {0} mac address: {1} not added to the list because it already exists.",
                                ipMacPair.first,
                                ipMacPair.second);
                        }
                    }
                    else
                    {
                        WriteInfo(
                            TraceOverlayNetworkResourceProvider,
                            "Failed to parse mac address: {0}.",
                            ipMacPair.second);
                    }
                }
                else
                {
                    WriteInfo(
                        TraceOverlayNetworkResourceProvider,
                        "Failed to parse ip address: {0}.",
                        ipMacPair.first);
                }
            }
        }

        return networkResources;
    }
}
