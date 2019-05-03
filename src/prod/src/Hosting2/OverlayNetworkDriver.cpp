// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceOverlayNetworkDriver("TraceOverlayNetworkDriver");

namespace Hosting2
{
    // ********************************************************************************************************************
    // OverlayNetworkDriver::CreateNetworkAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkDriver::CreateNetworkAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(CreateNetworkAsyncOperation)

    public:
        CreateNetworkAsyncOperation(
            OverlayNetworkDriver & owner,
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
            auto createNetworkOperation = this->owner_.networkPlugin_->BeginCreateNetwork(
                this->owner_.networkDefinition_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & createNetworkOperation)
            {
                this->OnCreateNetworkCompleted(createNetworkOperation, false);
            },
                thisSPtr);

            OnCreateNetworkCompleted(createNetworkOperation, true);
        }

        void OnCreateNetworkCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.networkPlugin_->EndCreateNetwork(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkDriver,
                owner_.Root.TraceId,
                "End(NetworkPluginCreateNetworkCompleted): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
        }

    private:
        OverlayNetworkDriver & owner_;
        TimeoutHelper timeoutHelper_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkDriver::DeleteNetworkAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkDriver::DeleteNetworkAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(DeleteNetworkAsyncOperation)

    public:
        DeleteNetworkAsyncOperation(
            OverlayNetworkDriver & owner,
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
            auto deleteNetworkOperation = this->owner_.networkPlugin_->BeginDeleteNetwork(
                this->owner_.networkDefinition_->NetworkName,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & deleteNetworkOperation)
            {
                this->OnDeleteNetworkCompleted(deleteNetworkOperation, false);
            },
                thisSPtr);

            OnDeleteNetworkCompleted(deleteNetworkOperation, true);
        }

        void OnDeleteNetworkCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.networkPlugin_->EndDeleteNetwork(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkDriver,
                owner_.Root.TraceId,
                "End(NetworkPluginDeleteNetworkCompleted): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
        }

    private:

        OverlayNetworkDriver & owner_;
        TimeoutHelper timeoutHelper_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkDriver::AttachContainerAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkDriver::AttachContainerAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(AttachContainerAsyncOperation)

    public:
        AttachContainerAsyncOperation(
            OverlayNetworkDriver & owner,
            std::wstring const & containerId,
            std::wstring const & ipAddress,
            std::wstring const & macAddress,
            std::wstring const & dnsServerList,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            timeoutHelper_(timeout),
            containerId_(containerId),
            ipAddress_(ipAddress),
            macAddress_(macAddress),
            dnsServerList_(dnsServerList)
        {
        }

        virtual ~AttachContainerAsyncOperation()
        {
        }

        static ErrorCode AttachContainerAsyncOperation::End(
            AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<AttachContainerAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            // if this is the first container on the network, then invoke create network
            // before attaching the container to the network.
            std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages;
            std::map<std::wstring, std::wstring> reservationIdCodePackageMap;
            this->owner_.networkResourceProvider_->GetReservedCodePackages(reservedCodePackages, reservationIdCodePackageMap);
            if (reservationIdCodePackageMap.size() == 0)
            {
                WriteInfo(
                    TraceOverlayNetworkDriver,
                    "Creating overlay network {0} since this is the first container on this network.",
                    this->owner_.networkDefinition_->NetworkName);

                auto createNetworkOperation = AsyncOperation::CreateAndStart<CreateNetworkAsyncOperation>(
                    this->owner_,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & createNetworkOperation)
                {
                    this->OnCreateNetworkCompleted(createNetworkOperation, false);
                },
                    thisSPtr);

                OnCreateNetworkCompleted(createNetworkOperation, true);
            }
            else
            {
                auto networkContainerParams = make_shared<OverlayNetworkContainerParameters>(
                    this->owner_.networkDefinition_,
                    this->ipAddress_,
                    this->macAddress_,
                    this->containerId_,
                    this->dnsServerList_);

                auto attachContainerOperation = this->owner_.networkPlugin_->BeginAttachContainerToNetwork(
                    networkContainerParams,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & attachContainerOperation)
                {
                    this->OnAttachContainerCompleted(attachContainerOperation, false);
                },
                    thisSPtr);

                OnAttachContainerCompleted(attachContainerOperation, true);
            }
        }

        void OnCreateNetworkCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = CreateNetworkAsyncOperation::End(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkDriver,
                owner_.Root.TraceId,
                "End(CreateNetworkCompleted): ErrorCode={0}",
                error);

            if (error.IsSuccess())
            {
                auto networkContainerParams = make_shared<OverlayNetworkContainerParameters>(
                    this->owner_.networkDefinition_,
                    this->ipAddress_,
                    this->macAddress_,
                    this->containerId_,
                    this->dnsServerList_);

                auto attachContainerOperation = this->owner_.networkPlugin_->BeginAttachContainerToNetwork(
                    networkContainerParams,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & attachContainerOperation)
                {
                    this->OnAttachContainerCompleted(attachContainerOperation, false);
                },
                    operation->Parent);
            
                OnAttachContainerCompleted(attachContainerOperation, true);
            }
            else
            {
                TryComplete(operation->Parent, error);
            }
        }

        void OnAttachContainerCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.networkPlugin_->EndAttachContainerToNetwork(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkDriver,
                owner_.Root.TraceId,
                "End(NetworkPluginAttachContainerCompleted): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
        }

    private:
        OverlayNetworkDriver & owner_;
        TimeoutHelper timeoutHelper_;
        std::wstring containerId_;
        std::wstring ipAddress_;
        std::wstring macAddress_;
        std::wstring dnsServerList_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkDriver::DetachContainerAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkDriver::DetachContainerAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(DetachContainerAsyncOperation)

    public:
        DetachContainerAsyncOperation(
            OverlayNetworkDriver & owner,
            std::wstring const & containerId,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            timeoutHelper_(timeout),
            containerId_(containerId)
        {
        }

        virtual ~DetachContainerAsyncOperation()
        {
        }

        static ErrorCode DetachContainerAsyncOperation::End(
            AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<DetachContainerAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            auto detachContainerOperation = this->owner_.networkPlugin_->BeginDetachContainerFromNetwork(
                this->containerId_,
                this->owner_.networkDefinition_->NetworkName,
                this->timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & detachContainerOperation)
            {
                this->OnDetachContainerCompleted(detachContainerOperation, false);
            },
                thisSPtr);

            OnDetachContainerCompleted(detachContainerOperation, true);
        }

        void OnDetachContainerCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.networkPlugin_->EndDetachContainerFromNetwork(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkDriver,
                owner_.Root.TraceId,
                "End(NetworkPluginDetachContainerCompleted): ErrorCode={0}",
                error);

            if (error.IsSuccess())
            {
                // if this is the last container on the network, then delete the network after detaching the container.
                std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages;
                std::map<std::wstring, std::wstring> reservationIdCodePackageMap;
                this->owner_.networkResourceProvider_->GetReservedCodePackages(reservedCodePackages, reservationIdCodePackageMap);
                // check for reservation id map minus 1 because the release call comes after the detach container call.
                if ((reservationIdCodePackageMap.size() - 1) == 0)
                {
                    WriteInfo(
                        TraceOverlayNetworkDriver,
                        "Network plugin deleting overlay network {0} since this is the last container on this network.",
                        this->owner_.networkDefinition_->NetworkName);

                    auto deleteNetworkOperation = AsyncOperation::CreateAndStart<DeleteNetworkAsyncOperation>(
                        this->owner_,
                        timeoutHelper_.GetRemainingTime(),
                        [this](AsyncOperationSPtr const & deleteNetworkOperation)
                    {
                        this->OnPluginDeleteNetworkCompleted(deleteNetworkOperation, false);
                    },
                        operation->Parent);

                    OnPluginDeleteNetworkCompleted(deleteNetworkOperation, true);
                }
                else
                {
                    TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
                }
            }
            else
            {
                TryComplete(operation->Parent, error);
            }
        }

        void OnPluginDeleteNetworkCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = DeleteNetworkAsyncOperation::End(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkDriver,
                owner_.Root.TraceId,
                "End(NetworkPluginDeleteNetworkCompleted): ErrorCode={0}",
                error);

            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceOverlayNetworkDriver,
                    "Network inventory manager deleting overlay network {0} since this is the last container on this network.",
                    this->owner_.networkDefinition_->NetworkName);

                auto deleteNetworkOperation = this->owner_.networkManager_.BeginDeleteOverlayNetworkDefinition(
                    this->owner_.networkDefinition_->NetworkName,
                    this->owner_.networkDefinition_->NodeId,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & deleteNetworkOperation)
                {
                    this->OnNetworkInventoryManagerDeleteNetworkCompleted(deleteNetworkOperation, false);
                },
                    operation->Parent);

                OnNetworkInventoryManagerDeleteNetworkCompleted(deleteNetworkOperation, true);
            }
            else
            {
                TryComplete(operation->Parent, error);
            }
        }

        void OnNetworkInventoryManagerDeleteNetworkCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.networkManager_.EndDeleteOverlayNetworkDefinition(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkDriver,
                owner_.Root.TraceId,
                "End(NetworkInventoryManagerDeleteNetworkCompleted): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
        }

    private:
        OverlayNetworkDriver & owner_;
        TimeoutHelper timeoutHelper_;
        std::wstring containerId_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkDriver::UpdateRoutesAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkDriver::UpdateRoutesAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(UpdateRoutesAsyncOperation)

    public:
        UpdateRoutesAsyncOperation(
            OverlayNetworkDriver & owner,
            OverlayNetworkRoutingInformationSPtr const & routingInfo,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            timeoutHelper_(timeout),
            routingInfo_(routingInfo)
        {
        }

        virtual ~UpdateRoutesAsyncOperation()
        {
        }

        static ErrorCode UpdateRoutesAsyncOperation::End(
            AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<UpdateRoutesAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            auto updateRoutesOperation = this->owner_.networkPlugin_->BeginUpdateRoutes(
                this->routingInfo_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & updateRoutesOperation)
            {
                this->OnUpdateRoutesCompleted(updateRoutesOperation, false);
            },
                thisSPtr);

            OnUpdateRoutesCompleted(updateRoutesOperation, true);
        }

        void OnUpdateRoutesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.networkPlugin_->EndUpdateRoutes(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkDriver,
                owner_.Root.TraceId,
                "End(NetworkPluginUpdateRoutesCompleted): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
        }

    private:
        OverlayNetworkDriver & owner_;
        OverlayNetworkRoutingInformationSPtr routingInfo_;
        TimeoutHelper timeoutHelper_;
    };

    OverlayNetworkDriver::OverlayNetworkDriver(
        ComponentRootSPtr const & root,
        __in OverlayNetworkManager & networkManager,
        __in OverlayNetworkPluginSPtr plugin,
        __in OverlayNetworkDefinitionSPtr networkDefinition,
        ReplenishNetworkResourcesCallback const & replenishNetworkResourcesCallback)
        : RootedObject(*root),
        networkManager_(networkManager),
        networkPlugin_(plugin),
        networkDefinition_(networkDefinition),
        networkDriverLock_(),
        replenishNetworkResourcesCallback_(replenishNetworkResourcesCallback),
        ghostChangeCallback_(nullptr)
    {
        internalReplenishNetworkResourcesCallback_ = [this]()
        {
            this->ReplenishNetworkResources();
        };

        auto networkResourceProvider = make_shared<OverlayNetworkResourceProvider>(
            root,
            this->networkDefinition_,
            this->internalReplenishNetworkResourcesCallback_,
            this->ghostChangeCallback_);
        this->networkResourceProvider_ = move(networkResourceProvider);
    }

    OverlayNetworkDriver::OverlayNetworkDriver(
        ComponentRootSPtr const & root,
        __in OverlayNetworkManager & networkManager,
        __in OverlayNetworkPluginSPtr plugin)
        : RootedObject(*root),
        networkManager_(networkManager),
        networkPlugin_(plugin),
        networkDriverLock_()
    {
    }

    OverlayNetworkDriver::~OverlayNetworkDriver()
    {
    }

    ErrorCode OverlayNetworkDriver::OnOpen()
    {
        return this->networkResourceProvider_->Open();
    }

    ErrorCode OverlayNetworkDriver::OnClose()
    {
        return this->networkResourceProvider_->Close();
    }

    void OverlayNetworkDriver::OnAbort()
    {
        return this->networkResourceProvider_->Abort();
    }

    Common::AsyncOperationSPtr OverlayNetworkDriver::BeginAttachContainerToNetwork(
        std::wstring const & containerId,
        std::wstring const & ipAddress,
        std::wstring const & macAddress,
        std::wstring const & dnsServerList,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<AttachContainerAsyncOperation>(
            *this,
            containerId,
            ipAddress,
            macAddress,
            dnsServerList,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkDriver::EndAttachContainerToNetwork(
        Common::AsyncOperationSPtr const & operation)
    {
        return AttachContainerAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr OverlayNetworkDriver::BeginDetachContainerFromNetwork(
        std::wstring const & containerId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<DetachContainerAsyncOperation>(
            *this,
            containerId,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkDriver::EndDetachContainerFromNetwork(
        Common::AsyncOperationSPtr const & operation)
    {
        return DetachContainerAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr OverlayNetworkDriver::BeginUpdateRoutes(
        OverlayNetworkRoutingInformationSPtr const & routingInfo,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<UpdateRoutesAsyncOperation>(
            *this,
            routingInfo,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkDriver::EndUpdateRoutes(
        Common::AsyncOperationSPtr const & operation)
    {
        return UpdateRoutesAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr OverlayNetworkDriver::BeginAcquireNetworkResources(
        std::wstring const & nodeId,
        std::wstring const & servicePackageId,
        std::vector<std::wstring> const & codePackageNames,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return this->networkResourceProvider_->BeginAcquireNetworkResources(
            nodeId,
            servicePackageId,
            codePackageNames,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkDriver::EndAcquireNetworkResources(
        Common::AsyncOperationSPtr const & operation,
        __out std::vector<std::wstring> & assignedNetworkResources)
    {
        return this->networkResourceProvider_->EndAcquireNetworkResources(operation, assignedNetworkResources);
    }

    Common::AsyncOperationSPtr OverlayNetworkDriver::BeginReleaseNetworkResources(
        std::wstring const & nodeId,
        std::wstring const & servicePackageId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return this->networkResourceProvider_->BeginReleaseNetworkResources(
            nodeId,
            servicePackageId,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkDriver::EndReleaseNetworkResources(
        Common::AsyncOperationSPtr const & operation)
    {
        return this->networkResourceProvider_->EndReleaseNetworkResources(operation);
    }

    Common::AsyncOperationSPtr OverlayNetworkDriver::BeginReleaseAllNetworkResourcesForNode(
        std::wstring const & nodeId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return this->networkResourceProvider_->BeginReleaseAllNetworkResourcesForNode(
            nodeId,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkDriver::EndReleaseAllNetworkResourcesForNode(
        Common::AsyncOperationSPtr const & operation)
    {
        return this->networkResourceProvider_->EndReleaseAllNetworkResourcesForNode(operation);
    }

    void OverlayNetworkDriver::GetReservedCodePackages(std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> & reservedCodePackages)
    {
        std::map<std::wstring, std::wstring> reservationIdCodePackageMap;
        this->networkResourceProvider_->GetReservedCodePackages(reservedCodePackages, reservationIdCodePackageMap);
    }

    void OverlayNetworkDriver::OnNewIpamData(std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeAdded,
        std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeRemoved)
    {
        AcquireExclusiveLock lock(this->networkDriverLock_);

        this->networkResourceProvider_->OnNewIpamData(ipMacAddressMapToBeAdded, ipMacAddressMapToBeRemoved);
    }

    void OverlayNetworkDriver::ReplenishNetworkResources()
    {
        AcquireExclusiveLock lock(this->networkDriverLock_);

        Threadpool::Post([this]() { this->replenishNetworkResourcesCallback_(this->networkDefinition_->NetworkName); });
    }
}
