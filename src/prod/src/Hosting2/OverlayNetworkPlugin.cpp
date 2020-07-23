// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceOverlayNetworkPlugin("TraceOverlayNetworkPlugin");

namespace Hosting2
{
    // ********************************************************************************************************************
    // OverlayNetworkPlugin::UpdateRoutesAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkPlugin::UpdateRoutesAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(UpdateRoutesAsyncOperation)

    public:
        UpdateRoutesAsyncOperation(
            OverlayNetworkPlugin & owner,
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
            auto operation = this->owner_.pluginOperations_->BeginUpdateRoutes(
                routingInfo_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnUpdateRoutesCompleted(operation, false);
            },
                thisSPtr);
         
            this->OnUpdateRoutesCompleted(operation, true);
        }

        void OnUpdateRoutesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.pluginOperations_->EndUpdateRoutes(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkPlugin,
                owner_.Root.TraceId,
                "End(UpdateRoutesCompleted): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
        }

    private:
        OverlayNetworkPlugin & owner_;
        OverlayNetworkRoutingInformationSPtr routingInfo_;
        TimeoutHelper timeoutHelper_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkPlugin::CreateNetworkAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkPlugin::CreateNetworkAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(CreateNetworkAsyncOperation)

    public:
        CreateNetworkAsyncOperation(
            OverlayNetworkPlugin & owner,
            OverlayNetworkDefinitionSPtr const & networkDefinition,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            timeoutHelper_(timeout)
        {
            UNREFERENCED_PARAMETER(networkDefinition);
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
            UNREFERENCED_PARAMETER(thisSPtr);
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }

    private:
        OverlayNetworkPlugin & owner_;
        TimeoutHelper timeoutHelper_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkPlugin::DeleteNetworkAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkPlugin::DeleteNetworkAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(DeleteNetworkAsyncOperation)

    public:
        DeleteNetworkAsyncOperation(
            OverlayNetworkPlugin & owner,
            std::wstring const & networkName,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            timeoutHelper_(timeout)
        {
            UNREFERENCED_PARAMETER(networkName);
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
            UNREFERENCED_PARAMETER(thisSPtr);
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }

    private:
        OverlayNetworkPlugin & owner_;
        TimeoutHelper timeoutHelper_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkPlugin::AttachContainerAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkPlugin::AttachContainerAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(AttachContainerAsyncOperation)

    public:
        AttachContainerAsyncOperation(
            OverlayNetworkPlugin & owner,
            OverlayNetworkContainerParametersSPtr const & containerNetworkParams,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            containerNetworkParams_(containerNetworkParams),
            timeoutHelper_(timeout)
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
            auto operation = this->owner_.pluginOperations_->BeginAttachContainer(
                containerNetworkParams_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnAttachContainerCompleted(operation, false);
            },
                thisSPtr);

            this->OnAttachContainerCompleted(operation, true);
        }

        void OnAttachContainerCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.pluginOperations_->EndAttachContainer(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkPlugin,
                owner_.Root.TraceId,
                "End(AttachContainerCompleted): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
        }

    private:
        OverlayNetworkPlugin & owner_;
        OverlayNetworkContainerParametersSPtr containerNetworkParams_;
        TimeoutHelper timeoutHelper_;
    };

    // ********************************************************************************************************************
    // OverlayNetworkPlugin::DetachContainerAsyncOperation Implementation
    // ********************************************************************************************************************
    class OverlayNetworkPlugin::DetachContainerAsyncOperation
        : public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(DetachContainerAsyncOperation)

    public:
        DetachContainerAsyncOperation(
            OverlayNetworkPlugin & owner,
            std::wstring const & containerId,
            std::wstring const & networkName,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            containerId_(containerId),
            networkName_(networkName),
            timeoutHelper_(timeout)
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
            auto operation = this->owner_.pluginOperations_->BeginDetachContainer(
                containerId_,
                networkName_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnDetachContainerCompleted(operation, false);
            },
                thisSPtr);

            this->OnDetachContainerCompleted(operation, true);
        }

        void OnDetachContainerCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = this->owner_.pluginOperations_->EndDetachContainer(operation);

            WriteTrace(
                error.ToLogLevel(),
                TraceOverlayNetworkPlugin,
                owner_.Root.TraceId,
                "End(DetachContainerCompleted): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
        }

    private:
        OverlayNetworkPlugin & owner_;
        std::wstring containerId_;
        std::wstring networkName_;
        TimeoutHelper timeoutHelper_;
    };

    OverlayNetworkPlugin::OverlayNetworkPlugin(
        ComponentRootSPtr const & root)
        : RootedObject(*root)
    {
        auto pluginOperations = make_unique<OverlayNetworkPluginOperations>(root);
        pluginOperations_ = move(pluginOperations);
    }

    OverlayNetworkPlugin::~OverlayNetworkPlugin()
    {
    }

    ErrorCode OverlayNetworkPlugin::OnOpen()
    {
        return ErrorCode::Success();
    }

    ErrorCode OverlayNetworkPlugin::OnClose()
    {
        return ErrorCode::Success();
    }

    void OverlayNetworkPlugin::OnAbort()
    {
        OnClose().ReadValue();
    }

    Common::AsyncOperationSPtr OverlayNetworkPlugin::BeginUpdateRoutes(
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

    Common::ErrorCode OverlayNetworkPlugin::EndUpdateRoutes(Common::AsyncOperationSPtr const & operation)
    {
        return UpdateRoutesAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr OverlayNetworkPlugin::BeginCreateNetwork(
        OverlayNetworkDefinitionSPtr const & networkDefinition,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<CreateNetworkAsyncOperation>(
            *this,
            networkDefinition,
            timeout,
            callback,
            parent);
    }
    
    Common::ErrorCode OverlayNetworkPlugin::EndCreateNetwork(
        Common::AsyncOperationSPtr const & operation)
    {
        return CreateNetworkAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr OverlayNetworkPlugin::BeginDeleteNetwork(
        std::wstring const & networkName,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<DeleteNetworkAsyncOperation>(
            *this,
            networkName,
            timeout,
            callback,
            parent);
    }
    
    Common::ErrorCode OverlayNetworkPlugin::EndDeleteNetwork(
        Common::AsyncOperationSPtr const & operation)
    {
        return DeleteNetworkAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr OverlayNetworkPlugin::BeginAttachContainerToNetwork(
        OverlayNetworkContainerParametersSPtr const & containerNetworkParams,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<AttachContainerAsyncOperation>(
            *this,
            containerNetworkParams,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkPlugin::EndAttachContainerToNetwork(
        Common::AsyncOperationSPtr const & operation)
    {
        return AttachContainerAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr OverlayNetworkPlugin::BeginDetachContainerFromNetwork(
        std::wstring const & containerId,
        std::wstring const & networkName,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<DetachContainerAsyncOperation>(
            *this,
            containerId,
            networkName,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode OverlayNetworkPlugin::EndDetachContainerFromNetwork(
        Common::AsyncOperationSPtr const & operation)
    {
        return DetachContainerAsyncOperation::End(operation);
    }
}
