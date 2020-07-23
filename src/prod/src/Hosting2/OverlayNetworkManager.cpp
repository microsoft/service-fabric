// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceOverlayNetworkManager("TraceOverlayNetworkManager");

namespace Hosting2
{
    // ********************************************************************************************************************
    // OverlayNetworkManager::GetOverlayNetworkDefinitionAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkManager::GetOverlayNetworkDefinitionAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(GetOverlayNetworkDefinitionAsyncOperation)

    public:
        GetOverlayNetworkDefinitionAsyncOperation(
            OverlayNetworkManager & owner,
            std::wstring const & networkName,
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            timeoutHelper_(timeout),
            networkName_(networkName),
            nodeId_(nodeId),
            nodeName_(nodeName),
            nodeIpAddress_(nodeIpAddress)
        {
        }

        virtual ~GetOverlayNetworkDefinitionAsyncOperation()
        {
        }

        static ErrorCode GetOverlayNetworkDefinitionAsyncOperation::End(
            AsyncOperationSPtr const & operation,
            OverlayNetworkDefinitionSPtr & networkDefinition)
        {
            auto thisPtr = AsyncOperation::End<GetOverlayNetworkDefinitionAsyncOperation>(operation);
            if (thisPtr->Error.IsSuccess())
            {
                networkDefinition = move(thisPtr->networkDefinition_);
            }
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
           auto getNetworkDefinition = this->owner_.processActivationManager_.BeginGetOverlayNetworkDefinition(
                this->networkName_,
                this->nodeId_,
                this->nodeName_,
                this->nodeIpAddress_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & getNetworkDefinition)
            {
                this->OnGetNetworkDefinitionCompleted(getNetworkDefinition, false);
            },
                thisSPtr);

            OnGetNetworkDefinitionCompleted(getNetworkDefinition, true);
        }

        void OnGetNetworkDefinitionCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            OverlayNetworkDefinitionSPtr networkDefinition;
            auto error = this->owner_.processActivationManager_.EndGetOverlayNetworkDefinition(operation, networkDefinition);
            if (error.IsSuccess())
            {
                this->networkDefinition_ = networkDefinition;
            }

            TryComplete(operation->Parent, error);
        }

    private:
        OverlayNetworkManager & owner_;
        TimeoutHelper timeoutHelper_;
        std::wstring networkName_;
        std::wstring nodeId_;
        std::wstring nodeName_;
        std::wstring nodeIpAddress_;
        OverlayNetworkDefinitionSPtr networkDefinition_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkManager::DeleteOverlayNetworkDefinitionAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkManager::DeleteOverlayNetworkDefinitionAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(DeleteOverlayNetworkDefinitionAsyncOperation)

    public:
        DeleteOverlayNetworkDefinitionAsyncOperation(
            OverlayNetworkManager & owner,
            std::wstring const & networkName,
            std::wstring const & nodeId,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            timeoutHelper_(timeout),
            networkName_(networkName),
            nodeId_(nodeId)
        {
        }

        virtual ~DeleteOverlayNetworkDefinitionAsyncOperation()
        {
        }

        static ErrorCode DeleteOverlayNetworkDefinitionAsyncOperation::End(AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<DeleteOverlayNetworkDefinitionAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            auto deleteNetworkOperation = this->owner_.processActivationManager_.BeginDeleteOverlayNetworkDefinition(
                this->networkName_,
                this->nodeId_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & deleteNetworkOperation)
            {
                this->OnDeleteNetworkDefinitionCompleted(deleteNetworkOperation, false);
            },
                thisSPtr);

            OnDeleteNetworkDefinitionCompleted(deleteNetworkOperation, true);
        }

        void OnDeleteNetworkDefinitionCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.processActivationManager_.EndDeleteOverlayNetworkDefinition(operation);

            WriteInfo(
                TraceOverlayNetworkManager,
                owner_.Root.TraceId,
                "Delete overlay network definition {0} returned error {1}.",
                this->networkName_,
                error);

            if (error.IsSuccess())
            {
                { //lock
                    AcquireExclusiveLock lock(this->owner_.lock_);

                    auto item = this->owner_.networkDriverMap_.find(this->networkName_);
                    if (item != this->owner_.networkDriverMap_.end())
                    {
                        item->second->Close();
                        this->owner_.networkDriverMap_.erase(item);
                    }
                }
            }

            TryComplete(operation->Parent, error);
        }

    private:
        OverlayNetworkManager & owner_;
        TimeoutHelper timeoutHelper_;
        std::wstring networkName_;
        std::wstring nodeId_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkManager::RequestPublishNetworkTablesAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkManager::RequestPublishNetworkTablesAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(RequestPublishNetworkTablesAsyncOperation)

    public:
        RequestPublishNetworkTablesAsyncOperation(
            OverlayNetworkManager & owner,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            timeoutHelper_(timeout),
            lock_()
        {
        }

        virtual ~RequestPublishNetworkTablesAsyncOperation()
        {
        }

        static ErrorCode RequestPublishNetworkTablesAsyncOperation::End(AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<RequestPublishNetworkTablesAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            { //lock
                AcquireExclusiveLock readLock(this->owner_.lock_);

                if (!this->owner_.networkDriverMap_.empty())
                {
                    pendingOperationsCount_.store(this->owner_.networkDriverMap_.size());

                    for (auto const & kv : this->owner_.networkDriverMap_)
                    {
                        auto operation = this->owner_.processActivationManager_.BeginPublishNetworkTablesRequest(
                            kv.first,
                            kv.second->NetworkDefinition->NodeId,
                            timeoutHelper_.GetRemainingTime(),
                            [this](AsyncOperationSPtr const & operation)
                        {
                            this->OnPublishNetworkTablesRequestCompleted(operation, false);
                        },
                            thisSPtr);

                        this->OnPublishNetworkTablesRequestCompleted(operation, true);
                    }
                }
                else
                {
                    TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                }
            }
        }

        void OnPublishNetworkTablesRequestCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.processActivationManager_.EndPublishNetworkTablesRequest(operation);

            WriteInfo(
                TraceOverlayNetworkManager,
                owner_.Root.TraceId,
                "Request publish network tables returned error {0}.",
                error);

            DecrementAndCheckPendingOperations(operation->Parent, error);
        }

        void DecrementAndCheckPendingOperations(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
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
                TraceOverlayNetworkManager,
                "CheckDecrement count {0}.", pendingOperationsCount);

            if (pendingOperationsCount == 0)
            {
                TryComplete(thisSPtr, lastError_);
            }
        }
    private:
        OverlayNetworkManager & owner_;
        TimeoutHelper timeoutHelper_;
        Common::RwLock lock_;
        ErrorCode lastError_;
        atomic_uint64 pendingOperationsCount_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkManager::GetOverlayNetworkDriverAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkManager::GetOverlayNetworkDriverAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(GetOverlayNetworkDriverAsyncOperation)

    public:
        GetOverlayNetworkDriverAsyncOperation(
            OverlayNetworkManager & owner,
            std::wstring const & networkName,
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            timeoutHelper_(timeout),
            networkName_(networkName),
            nodeId_(nodeId),
            nodeName_(nodeName),
            nodeIpAddress_(nodeIpAddress)
        {
        }

        virtual ~GetOverlayNetworkDriverAsyncOperation()
        {
        }

        static ErrorCode GetOverlayNetworkDriverAsyncOperation::End(
            AsyncOperationSPtr const & operation,
            OverlayNetworkDriverSPtr & networkDriver)
        {
            auto thisPtr = AsyncOperation::End<GetOverlayNetworkDriverAsyncOperation>(operation);
            if (thisPtr->Error.IsSuccess())
            {
                networkDriver = thisPtr->networkDriver_;
            }
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            // try retrieving the network driver from the map
            { //lock
                AcquireExclusiveLock readLock(this->owner_.lock_);

                auto item = this->owner_.networkDriverMap_.find(this->networkName_);
                if (item != this->owner_.networkDriverMap_.end())
                {
                    this->networkDriver_ = this->owner_.networkDriverMap_[this->networkName_];
                }
            }

            if (this->networkDriver_ != nullptr)
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            }
            else
            {
                // add lightweight driver to the map
                { //lock
                    AcquireExclusiveLock lock(this->owner_.lock_);

                    // create lightweight overlay network driver
                    auto networkDriver = make_shared<OverlayNetworkDriver>(
                        this->owner_.root_,
                        this->owner_,
                        this->owner_.networkPlugin_);

                    auto item = this->owner_.lightWeightNetworkDriverMap_.find(this->networkName_);
                    if (item == this->owner_.lightWeightNetworkDriverMap_.end())
                    {
                        this->owner_.lightWeightNetworkDriverMap_.insert(make_pair(this->networkName_, move(networkDriver)));
                    }
                }
                
                auto getNetworkDefinition = AsyncOperation::CreateAndStart<GetOverlayNetworkDefinitionAsyncOperation>(
                    this->owner_,
                    this->networkName_,
                    this->nodeId_,
                    this->nodeName_,
                    this->nodeIpAddress_,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & getNetworkDefinition)
                {
                    this->OnGetNetworkDefinitionCompleted(getNetworkDefinition, false);
                },
                    thisSPtr);

                OnGetNetworkDefinitionCompleted(getNetworkDefinition, true);
            }
        }

        void OnGetNetworkDefinitionCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            OverlayNetworkDefinitionSPtr networkDefinition;
            auto error = GetOverlayNetworkDefinitionAsyncOperation::End(operation, networkDefinition);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkManager,
                owner_.Root.TraceId,
                "End(GetNetworkDefinitionCompleted): ErrorCode={0}",
                error);

            if (error.IsSuccess())
            {
                // create overlay network driver
                auto networkDriver = make_shared<OverlayNetworkDriver>(
                    this->owner_.root_,
                    this->owner_,
                    this->owner_.networkPlugin_,
                    networkDefinition,
                    this->owner_.replenishNetworkResourcesCallback_);

                this->networkDriver_ = networkDriver;

                auto openErrorCode = this->networkDriver_->Open();

                WriteTrace(
                    openErrorCode.ToLogLevel(),
                    TraceOverlayNetworkManager,
                    owner_.Root.TraceId,
                    "Get overlay network driver open return code: {0}.",
                    openErrorCode);

                if (openErrorCode.IsSuccess())
                {
                    // add the network driver to the map
                    { //lock
                        AcquireExclusiveLock lock(this->owner_.lock_);

                        auto item = this->owner_.networkDriverMap_.find(this->networkName_);
                        if (item == this->owner_.networkDriverMap_.end())
                        {
                            this->owner_.networkDriverMap_.insert(make_pair(this->networkName_, move(networkDriver)));
                        }

                        // remove the lightweight network driver
                        auto item1 = this->owner_.lightWeightNetworkDriverMap_.find(this->networkName_);
                        if (item1 != this->owner_.lightWeightNetworkDriverMap_.end())
                        {
                            this->owner_.lightWeightNetworkDriverMap_.erase(item1);
                        }
                    }

                    TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
                }
            }
            else
            {
                TryComplete(operation->Parent, error);
            }
        }

    private:
        OverlayNetworkManager & owner_;
        TimeoutHelper timeoutHelper_;
        std::wstring networkName_;
        std::wstring nodeId_;
        std::wstring nodeName_;
        std::wstring nodeIpAddress_;
        OverlayNetworkDriverSPtr networkDriver_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkManager::UpdateOverlayNetworkRoutesAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkManager::UpdateOverlayNetworkRoutesAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(UpdateOverlayNetworkRoutesAsyncOperation)

    public:
        UpdateOverlayNetworkRoutesAsyncOperation(
            OverlayNetworkManager & owner,
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

        virtual ~UpdateOverlayNetworkRoutesAsyncOperation()
        {
        }

        static ErrorCode UpdateOverlayNetworkRoutesAsyncOperation::End(
            AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<UpdateOverlayNetworkRoutesAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            OverlayNetworkDriverSPtr networkDriver;

            // retrieve the network driver from the map
            { // lock
                AcquireExclusiveLock lock(this->owner_.lock_);

                auto item = this->owner_.networkDriverMap_.find(this->routingInfo_->NetworkName);
                if (item != this->owner_.networkDriverMap_.end())
                {
                    networkDriver = this->owner_.networkDriverMap_[this->routingInfo_->NetworkName];
                }
                else
                {
                    // retrieve the lightweight network driver from the map
                    auto item1 = this->owner_.lightWeightNetworkDriverMap_.find(this->routingInfo_->NetworkName);
                    if (item1 != this->owner_.lightWeightNetworkDriverMap_.end())
                    {
                        networkDriver = this->owner_.lightWeightNetworkDriverMap_[this->routingInfo_->NetworkName];
                    }
                }
            }

            if (networkDriver)
            {
                this->routingInfo_->OverlayNetworkDefinition = networkDriver->NetworkDefinition;

                auto updateRoutesOperation = networkDriver->BeginUpdateRoutes(
                    this->routingInfo_,
                    timeoutHelper_.GetRemainingTime(),
                    [this, networkDriver](AsyncOperationSPtr const & updateRoutesOperation)
                {
                    this->OnUpdateRoutesCompleted(updateRoutesOperation, networkDriver, false);
                },
                    thisSPtr);

                OnUpdateRoutesCompleted(updateRoutesOperation, networkDriver, true);
            }
            else
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            }
        }

        void OnUpdateRoutesCompleted(AsyncOperationSPtr const & operation, OverlayNetworkDriverSPtr networkDriver, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = networkDriver->EndUpdateRoutes(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkManager,
                owner_.Root.TraceId,
                "End(NetworkDriverUpdateRoutesCompleted): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
        }

    private:
        OverlayNetworkManager & owner_;
        TimeoutHelper timeoutHelper_;
        OverlayNetworkRoutingInformationSPtr routingInfo_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkManager::ReleaseAllNetworkResourcesForNodeAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkManager::ReleaseAllNetworkResourcesForNodeAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(ReleaseAllNetworkResourcesForNodeAsyncOperation)

    public:
        ReleaseAllNetworkResourcesForNodeAsyncOperation(
            OverlayNetworkManager & owner,
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
            { //lock
                AcquireExclusiveLock readLock(this->owner_.lock_);

                pendingOperationsCount_.store(this->owner_.networkDriverMap_.size());
            }

            { //lock
                AcquireExclusiveLock readLock(this->owner_.lock_);

                if (!this->owner_.networkDriverMap_.empty())
                {
                    for (auto iter = this->owner_.networkDriverMap_.begin(); iter != this->owner_.networkDriverMap_.end(); ++iter)
                    {
                        auto networkDriver = iter->second;
                        auto operation = iter->second->BeginReleaseAllNetworkResourcesForNode(
                            this->nodeId_,
                            timeoutHelper_.GetRemainingTime(),
                            [this, networkDriver](AsyncOperationSPtr const & operation)
                        {
                            this->OnReleaseAllNetworkResourcesForNodeCompleted(operation, false, networkDriver);
                        },
                            thisSPtr);

                        this->OnReleaseAllNetworkResourcesForNodeCompleted(operation, true, networkDriver);
                    }
                }
                else
                {
                    TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
                }
            }
        }

        void OnReleaseAllNetworkResourcesForNodeCompleted(
            AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously,
            OverlayNetworkDriverSPtr const & networkDriver)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = networkDriver->EndReleaseAllNetworkResourcesForNode(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkManager,
                owner_.Root.TraceId,
                "End(ReleaseAllNetworkResourcesForNodeCompleted): ErrorCode={0}",
                error);

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
                TraceOverlayNetworkManager,
                "CheckDecrement count {0}.", pendingOperationsCount);

            if (pendingOperationsCount == 0)
            {
                TryComplete(thisSPtr, lastError_);
            }
        }

    private:
        OverlayNetworkManager & owner_;
        TimeoutHelper timeoutHelper_;
        wstring nodeId_;
        atomic_uint64 pendingOperationsCount_;
        Common::RwLock lock_;
        ErrorCode lastError_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkManager::ReplenishNetworkResourcesAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkManager::ReplenishNetworkResourcesAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(ReplenishNetworkResourcesAsyncOperation)

    public:
        ReplenishNetworkResourcesAsyncOperation(
            OverlayNetworkManager & owner,
            std::wstring const & networkName,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            timeoutHelper_(timeout),
            networkName_(networkName)
        {
        }

        virtual ~ReplenishNetworkResourcesAsyncOperation()
        {
        }

        static ErrorCode ReplenishNetworkResourcesAsyncOperation::End(
            AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<ReplenishNetworkResourcesAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            OverlayNetworkDriverSPtr networkDriver;
            // try retrieving the network driver from the map
            { //lock
                AcquireExclusiveLock readLock(this->owner_.lock_);

                auto item = this->owner_.networkDriverMap_.find(this->networkName_);
                if (item != this->owner_.networkDriverMap_.end())
                {
                    networkDriver = this->owner_.networkDriverMap_[this->networkName_];
                }
            }

            if (networkDriver)
            {
                // Retrieve network resources and refresh driver pool.
                auto getNetworkDefinition = AsyncOperation::CreateAndStart<GetOverlayNetworkDefinitionAsyncOperation>(
                    this->owner_,
                    this->networkName_,
                    networkDriver->NetworkDefinition->NodeId,
                    networkDriver->NetworkDefinition->NodeName,
                    networkDriver->NetworkDefinition->NodeIpAddress,
                    timeoutHelper_.GetRemainingTime(),
                    [this, networkDriver](AsyncOperationSPtr const & getNetworkDefinition)
                {
                    this->OnGetNetworkDefinitionCompleted(getNetworkDefinition, networkDriver, false);
                },
                    thisSPtr);

                OnGetNetworkDefinitionCompleted(getNetworkDefinition, networkDriver, true);
            }
            else
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            }
        }

        void OnGetNetworkDefinitionCompleted(AsyncOperationSPtr const & operation, OverlayNetworkDriverSPtr networkDriver, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            OverlayNetworkDefinitionSPtr networkDefinition;
            auto error = GetOverlayNetworkDefinitionAsyncOperation::End(operation, networkDefinition);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkManager,
                owner_.Root.TraceId,
                "End(GetNetworkDefinitionCompleted): ErrorCode={0}",
                error);

            if (error.IsSuccess())
            {
                networkDriver->OnNewIpamData(networkDefinition->IpMacAddressMapToBeAdded, networkDefinition->IpMacAddressMapToBeRemoved);
            }

            TryComplete(operation->Parent, error);
        }

    private:
        OverlayNetworkManager & owner_;
        TimeoutHelper timeoutHelper_;
        std::wstring networkName_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkManager::OpenAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkManager::OpenAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(OpenAsyncOperation)

    public:
        OpenAsyncOperation(
            OverlayNetworkManager & owner,
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
            if (this->owner_.overlayNetworkManagerEnabled_)
            {
                auto openErrorCode = owner_.networkPlugin_->Open();

                WriteInfo(
                    TraceOverlayNetworkManager,
                    "Overlay network manager open return code: {0}.",
                    openErrorCode);

                if (openErrorCode.IsSuccess())
                {
                    this->owner_.initialized_ = true;
                }

                TryComplete(thisSPtr, openErrorCode);
            }
            else
            {
                WriteInfo(
                    TraceOverlayNetworkManager,
                    "Skipping overlay network manager initialization. Overlay network manager enabled: {0}.",
                    this->owner_.overlayNetworkManagerEnabled_);

                TryComplete(thisSPtr, ErrorCodeValue::Success);
            }
        }

    private:
        OverlayNetworkManager & owner_;
        TimeoutHelper timeoutHelper_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkManager::CloseAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkManager::CloseAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(CloseAsyncOperation)

    public:
        CloseAsyncOperation(
            OverlayNetworkManager & owner,
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
            ErrorCode overallError(ErrorCodeValue::Success);

            if (this->owner_.overlayNetworkManagerEnabled_)
            {
                auto closeErrorCode = owner_.networkPlugin_->Close();

                WriteInfo(
                    TraceOverlayNetworkManager,
                    "Overlay network plugin close return code: {0}.",
                    closeErrorCode);

                int successCount = 0;
                size_t totalCount = owner_.networkDriverMap_.size();

                // Close the network drivers
                for (auto const & nd : owner_.networkDriverMap_)
                {
                    auto ndCloseErrorCode = nd.second->Close();
                    if (ndCloseErrorCode.IsSuccess())
                    {
                        successCount++;
                    }
                }

                WriteInfo(
                    TraceOverlayNetworkManager,
                    "Overlay network driver close total count: {0} success count: {1}.",
                    totalCount,
                    successCount);

                overallError = (totalCount == successCount && closeErrorCode.IsSuccess()) ? (ErrorCodeValue::Success) : (ErrorCodeValue::OperationFailed);
            }

            TryComplete(thisSPtr, overallError);
        }

    private:
        OverlayNetworkManager & owner_;
        TimeoutHelper timeoutHelper_;
    };

    OverlayNetworkManager::OverlayNetworkManager(
        ComponentRootSPtr const & root,
        ProcessActivationManager & processActivationManager)
        : RootedObject(*root),
          root_(root),
          processActivationManager_(processActivationManager),
          lock_(),
          initialized_(false),
          overlayNetworkManagerEnabled_(SetupConfig::GetConfig().IsolatedNetworkSetup),
          networkDriverMap_()
    {
        replenishNetworkResourcesCallback_ = [this](std::wstring const & networkName)
        {
            this->ReplenishNetworkResources(networkName);
        };

        auto networkPlugin = make_shared<OverlayNetworkPlugin>(root);
        this->networkPlugin_ = move(networkPlugin);
    }

    OverlayNetworkManager::~OverlayNetworkManager()
    {
    }

    Common::AsyncOperationSPtr OverlayNetworkManager::BeginGetOverlayNetworkDriver(
        std::wstring const & networkName,
        std::wstring const & nodeId,
        std::wstring const & nodeName,
        std::wstring const & nodeIpAddress,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<GetOverlayNetworkDriverAsyncOperation>(
            *this,
            networkName,
            nodeId,
            nodeName,
            nodeIpAddress,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkManager::EndGetOverlayNetworkDriver(
        Common::AsyncOperationSPtr const & operation,
        OverlayNetworkDriverSPtr & networkDriver)
    {
        return GetOverlayNetworkDriverAsyncOperation::End(operation, networkDriver);
    }

    Common::AsyncOperationSPtr OverlayNetworkManager::BeginReplenishNetworkResources(
        std::wstring const & networkName,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ReplenishNetworkResourcesAsyncOperation>(
            *this,
            networkName,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkManager::EndReplenishNetworkResources(
        Common::AsyncOperationSPtr const & operation)
    {
        return ReplenishNetworkResourcesAsyncOperation::End(operation);
    }

    void OverlayNetworkManager::ReplenishNetworkResources(std::wstring const & networkName)
    {
        auto operation = this->BeginReplenishNetworkResources(
            networkName,
            HostingConfig::GetConfig().RequestTimeout,
            [this](AsyncOperationSPtr const & operation)
        {
            auto error = this->OverlayNetworkManager::EndReplenishNetworkResources(operation);

            WriteInfo(
                TraceOverlayNetworkManager,
                "Overlay network manager replenish return code: {0}.",
                error);
        },
            this->Root.CreateAsyncOperationRoot());
    }

    Common::AsyncOperationSPtr OverlayNetworkManager::BeginRequestPublishNetworkTables(
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<RequestPublishNetworkTablesAsyncOperation>(
            *this,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkManager::EndRequestPublishNetworkTables(
        Common::AsyncOperationSPtr const & operation)
    {
        return RequestPublishNetworkTablesAsyncOperation::End(operation);
    }

    void OverlayNetworkManager::RequestPublishNetworkTables()
    {
        auto operation = this->BeginRequestPublishNetworkTables(
            HostingConfig::GetConfig().RequestTimeout,
            [this](AsyncOperationSPtr const & operation)
        {
            auto error = this->OverlayNetworkManager::EndRequestPublishNetworkTables(operation);

            WriteInfo(
                TraceOverlayNetworkManager,
                "Overlay network manager request publish network tables return code: {0}.",
                error);
        },
            this->Root.CreateAsyncOperationRoot());
    }

    Common::AsyncOperationSPtr OverlayNetworkManager::BeginDeleteOverlayNetworkDefinition(
        std::wstring const & networkName,
        std::wstring const & nodeId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<DeleteOverlayNetworkDefinitionAsyncOperation>(
            *this,
            networkName,
            nodeId,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkManager::EndDeleteOverlayNetworkDefinition(
        Common::AsyncOperationSPtr const & operation)
    {
        return DeleteOverlayNetworkDefinitionAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr OverlayNetworkManager::BeginReleaseAllNetworkResourcesForNode(
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

   Common::ErrorCode OverlayNetworkManager::EndReleaseAllNetworkResourcesForNode(
       Common::AsyncOperationSPtr const & operation)
   {
       return ReleaseAllNetworkResourcesForNodeAsyncOperation::End(operation);
   }
   
    void OverlayNetworkManager::RetrieveGatewayIpAddress(std::vector<std::wstring> const & networkNames, __out std::vector<std::wstring> & gatewayIpAddresses)
    {
        { //lock
            AcquireExclusiveLock readLock(this->lock_);

            for (auto const & networkName : networkNames)
            {
                auto iter = this->networkDriverMap_.find(networkName);
                if (iter != this->networkDriverMap_.end())
                {
                    gatewayIpAddresses.push_back(iter->second->NetworkDefinition->Gateway);
                }
            }
        }
    }

    void OverlayNetworkManager::GetNetworkReservedCodePackages(std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> & networkReservedCodePackages)
    {
        { //lock
            AcquireExclusiveLock readLock(this->lock_);

            for (auto const & networkDriver : this->networkDriverMap_)
            {
                std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> reservedCodePackages;
                networkDriver.second->GetReservedCodePackages(reservedCodePackages);

                networkReservedCodePackages.insert(make_pair(networkDriver.first, std::map<std::wstring, std::vector<std::wstring>>()));
                auto networkIter = networkReservedCodePackages.find(networkDriver.first);

                for (auto const & r : reservedCodePackages)
                {
                    for (auto const & s : r.second)
                    {
                        vector<std::wstring> codePackages;
                        for (auto const & c : s.second)
                        {
                            codePackages.push_back(c);
                        }
                        networkIter->second[s.first] = codePackages;
                    }
                }
            }
        }
    }

    void OverlayNetworkManager::GetNetworkNames(std::vector<std::wstring> & networkNames)
    {
        { //lock
            AcquireExclusiveLock readLock(this->lock_);

            for (auto const & networkDriver : this->networkDriverMap_)
            {
                networkNames.push_back(networkDriver.first);
            }
        }
    }

    Common::AsyncOperationSPtr OverlayNetworkManager::BeginUpdateOverlayNetworkRoutes(
        OverlayNetworkRoutingInformationSPtr const & routingInfo,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<UpdateOverlayNetworkRoutesAsyncOperation>(
            *this,
            routingInfo,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkManager::EndUpdateOverlayNetworkRoutes(
        Common::AsyncOperationSPtr const & operation)
    {
        return UpdateOverlayNetworkRoutesAsyncOperation::End(operation);
    }

    // ********************************************************************************************************************
    // AsyncFabricComponent methods
    // ********************************************************************************************************************
    AsyncOperationSPtr OverlayNetworkManager::OnBeginOpen(
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        WriteNoise(TraceOverlayNetworkManager, "Opening OverlayNetworkManager.");

        return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
            *this,
            timeout,
            callback,
            parent);
    }

    ErrorCode OverlayNetworkManager::OnEndOpen(
        AsyncOperationSPtr const & operation)
    {
        return OpenAsyncOperation::End(operation);
    }

    AsyncOperationSPtr OverlayNetworkManager::OnBeginClose(
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        WriteNoise(TraceOverlayNetworkManager, "Closing OverlayNetworkManager.");

        return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
            *this,
            timeout,
            callback,
            parent);
    }

    ErrorCode OverlayNetworkManager::OnEndClose(
        AsyncOperationSPtr const & operation)
    {
        return CloseAsyncOperation::End(operation);
    }

    void OverlayNetworkManager::OnAbort()
    {
        WriteNoise(TraceOverlayNetworkManager, "Aborting OverlayNetworkManager.");
        if (this->overlayNetworkManagerEnabled_)
        {
            // Abort the network plugin
            if (this->networkPlugin_ != nullptr)
            {
                this->networkPlugin_->Abort();
            }

            // Abort the network drivers
            for (auto const & nd : this->networkDriverMap_)
            {
                nd.second->Abort();
            }
        }
    }
}
