// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Client
{
    using namespace std;
    using namespace Common;
    using namespace ServiceModel;
    using namespace Management::ClusterManager;
    using namespace Transport;
    using namespace ClientServerTransport;

    FabricClientImpl::DeleteServiceAsyncOperation::DeleteServiceAsyncOperation(
        __in FabricClientImpl &client,
        DeleteServiceDescription const &description,
        __in Common::TimeSpan timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent,
        __in_opt ErrorCode && passThroughError)
        : ClientAsyncOperationBase(client, FabricActivityHeader(Guid::NewGuid()), move(passThroughError), timeout, callback, parent)
        , description_(description)
        , activityHeader_(Common::Guid::NewGuid())
    {
    }
    
    void FabricClientImpl::DeleteServiceAsyncOperation::OnStartOperation(AsyncOperationSPtr const &thisSPtr)
    {
        //
        // Check if it is managed or unmanaged service and passon the deletion operation
        // accordingly.
        //
        auto inner = this->Client.BeginInternalGetServiceDescription(
            description_.ServiceName,
            activityHeader_,
            this->RemainingTime,
            [this](AsyncOperationSPtr const operation)
            {
                this->OnGetServiceDescriptionComplete(operation, false);
            },
            thisSPtr);

        OnGetServiceDescriptionComplete(inner, true);
    }

    void FabricClientImpl::DeleteServiceAsyncOperation::OnGetServiceDescriptionComplete(
        AsyncOperationSPtr const &operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }

        Naming::PartitionedServiceDescriptor psd;
        auto error = this->Client.EndInternalGetServiceDescription(operation, psd);

        auto const & thisSPtr = operation->Parent;

        if (error.IsSuccess())
        {
            if (psd.Service.ApplicationName.empty())
            {
                //
                // AdHoc services(unmanaged) to Naming subsystem
                //
                ForwardToNaming(thisSPtr);
            }
            else
            {
                //
                // Managed services to Cluster manager
                //
                NamingUri appNameUri;
                if (NamingUri::TryParse(psd.Service.ApplicationName, appNameUri))
                {
                    ForwardToClusterManager(thisSPtr, appNameUri);
                }
                else
                {
                    TryComplete(thisSPtr, ErrorCodeValue::InvalidNameUri);
                }
            }
        }
        else if (error.IsError(ErrorCodeValue::ServiceNotFound) && description_.IsForce)
        {
            // Gateway doesn't support fabric client sending messages to FM directly
            ForwardToNaming(thisSPtr);
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    void FabricClientImpl::DeleteServiceAsyncOperation::ForwardToClusterManager(
        AsyncOperationSPtr const &operation,
        NamingUri const &appName)
    {
        auto request = ClusterManagerTcpMessage::GetDeleteService(
            Common::make_unique<Management::ClusterManager::DeleteServiceMessageBody>(appName, description_.ServiceName, description_.IsForce));

        request->Headers.Replace(activityHeader_);

        auto inner = this->Client.BeginInternalForwardToService(
            move(request),
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) 
            {
                this->OnForwardToClusterManagerComplete(operation, false); 
            },
            operation);

        OnForwardToClusterManagerComplete(inner, true);
    }

    void FabricClientImpl::DeleteServiceAsyncOperation::OnForwardToClusterManagerComplete(
        AsyncOperationSPtr const &operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }

        ClientServerReplyMessageUPtr reply;
        auto error = this->Client.EndInternalForwardToService(operation, reply);
        // Service exists on Naming but not CM, by-pass CM if it is force delete.
        if (description_.IsForce
            && (error.IsError(ErrorCodeValue::UserServiceNotFound) || error.IsError(ErrorCodeValue::NotFound)))
        {
            this->ForwardToNaming(operation->Parent);
        }
        else
        {
            this->TryComplete(operation->Parent, error);
        }
    }

    void FabricClientImpl::DeleteServiceAsyncOperation::ForwardToNaming(
        AsyncOperationSPtr const &operation)
    {
        auto inner = this->Client.BeginInternalDeleteService(
            description_,
            activityHeader_.ActivityId,
            this->RemainingTime,
            [this](AsyncOperationSPtr const& operation)
            {
                this->OnForwardToNamingComplete(operation, false);
            },
            operation);

        OnForwardToNamingComplete(inner, true);
    }

    void FabricClientImpl::DeleteServiceAsyncOperation::OnForwardToNamingComplete(
        AsyncOperationSPtr const &operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }
        
        bool unused;
        auto error = this->Client.EndInternalDeleteService(operation, unused);
        TryComplete(operation->Parent, error);
    }

    void FabricClientImpl::DeleteServiceAsyncOperation::ForwardToFailoverManager(
        AsyncOperationSPtr const &operation)
    {
        if (!SystemServiceApplicationNameHelper::IsSystemServiceDeletable(SystemServiceApplicationNameHelper::GetInternalServiceName(description_.ServiceName.ToString())))
        {
            TryComplete(operation, ErrorCodeValue::InvalidNameUri);
            return;
        }

        auto inner = this->Client.BeginInternalDeleteSystemService(
            description_.ServiceName,
            activityHeader_.ActivityId,
            this->RemainingTime,
            [this](AsyncOperationSPtr const& operation)
            {
                this->OnForwardToFailoverManagerComplete(operation, false);
            },
            operation);

        OnForwardToFailoverManagerComplete(inner, true);
    }

    void FabricClientImpl::DeleteServiceAsyncOperation::OnForwardToFailoverManagerComplete(
        AsyncOperationSPtr const &operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously) return;

        auto error = this->Client.EndInternalDeleteSystemService(operation);
        TryComplete(operation->Parent, error);
    }

    ErrorCode FabricClientImpl::DeleteServiceAsyncOperation::End(AsyncOperationSPtr const &operation)
    {
        auto casted = AsyncOperation::End<DeleteServiceAsyncOperation>(operation);
        return casted->Error;
    }
}   
