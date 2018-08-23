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
    using namespace Naming;

    StringLiteral const TraceComponent("FabricClientImpl");

    FabricClientImpl::GetServiceDescriptionAsyncOperation::GetServiceDescriptionAsyncOperation(
        __in FabricClientImpl &client,
        NamingUri const &associatedName,
        bool fetchCached,
        __in FabricActivityHeader && activityHeader,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent,
        __in_opt ErrorCode && passThroughError)
        : ClientAsyncOperationBase(client, move(activityHeader), move(passThroughError), timeout, callback, parent)
        , associatedName_(associatedName)
        , fetchCached_(fetchCached)
        , description_()
    {
    }

    void FabricClientImpl::GetServiceDescriptionAsyncOperation::OnStartOperation(AsyncOperationSPtr const& thisSPtr)
    {
        if (fetchCached_ && this->Client.lruCacheManager_.get() != nullptr)
        {
            this->FetchFromCache(thisSPtr);
        }
        else
        {
            this->FetchFromNamingService(thisSPtr);
        }
    }

    void FabricClientImpl::GetServiceDescriptionAsyncOperation::FetchFromCache(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->Client.lruCacheManager_->BeginGetPsd(
            associatedName_.Fragment.empty() ? associatedName_ : NamingUri(associatedName_.Path),
            this->ActivityHeader,
            this->RemainingTime,
            [this](AsyncOperationSPtr const operation) { this->OnFetchFromCacheComplete(operation, false); },
            thisSPtr);
        this->OnFetchFromCacheComplete(operation, true);
    }

    void FabricClientImpl::GetServiceDescriptionAsyncOperation::OnFetchFromCacheComplete(
        AsyncOperationSPtr const & operation, 
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = this->Client.lruCacheManager_->EndGetPsd(operation, description_);

        this->TryComplete(thisSPtr, error);
    }

    void FabricClientImpl::GetServiceDescriptionAsyncOperation::FetchFromNamingService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->Client.BeginInternalGetServiceDescription(
            associatedName_.Fragment.empty() ? associatedName_ : NamingUri(associatedName_.Path),
            this->ActivityHeader,
            this->RemainingTime,
            [this](AsyncOperationSPtr const operation) { this->OnFetchFromNamingServiceComplete(operation, false); },
            thisSPtr);
        this->OnFetchFromNamingServiceComplete(operation, true);
    }

    void FabricClientImpl::GetServiceDescriptionAsyncOperation::OnFetchFromNamingServiceComplete(
        AsyncOperationSPtr const& operation, 
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously) { return; }

        auto error = this->Client.EndInternalGetServiceDescription(operation, description_);

        this->TryComplete(operation->Parent, error);
    }

    ErrorCode FabricClientImpl::GetServiceDescriptionAsyncOperation::EndGetServiceDescription(
        AsyncOperationSPtr const & asyncOperation,
        __out Naming::PartitionedServiceDescriptor &result)
    {
        auto operation = AsyncOperation::End<GetServiceDescriptionAsyncOperation>(asyncOperation);
        auto error = operation->Error;

        if (error.IsSuccess())
        {
            if (!operation->description_.IsServiceGroup)
            {
                result = move(operation->description_);
            }
            else
            {
                if (operation->associatedName_.Fragment.empty())
                {
                    // cannot access a service group member description with a service group name
                    error = ErrorCodeValue::AccessDenied;

                    WriteWarning(
                        Constants::FabricClientSource,
                        "must use GetServiceGroupDescription() to query group name '{0}': error={1}",
                        operation->associatedName_,
                        error);
                }
                else
                {
                    ServiceGroupDescriptor serviceGroup;
                    error = ServiceGroupDescriptor::Create(operation->description_, serviceGroup);

                    if (error.IsSuccess())
                    {
                        Naming::PartitionedServiceDescriptor memberService;
                        error = serviceGroup.GetServiceDescriptorForMember(operation->associatedName_, memberService);

                        if (error.IsSuccess())
                        {
                            result = move(memberService);
                        }
                    }
                }
            }
        }

        return error;
    }

    ErrorCode FabricClientImpl::GetServiceDescriptionAsyncOperation::EndGetServiceGroupDescription(
        AsyncOperationSPtr const & asyncOperation,
        __out ServiceGroupDescriptor &result)
    {
        auto operation = AsyncOperation::End<GetServiceDescriptionAsyncOperation>(asyncOperation);
        auto error = operation->Error;

        if (error.IsSuccess())
        {
            if (operation->description_.IsServiceGroup)
            {
                ServiceGroupDescriptor serviceGroupDescriptor;
                error = ServiceGroupDescriptor::Create(operation->description_, serviceGroupDescriptor);
                if (error.IsSuccess())
                {
                    result = move(serviceGroupDescriptor);
                }
            }
            else
            {
                // cannot access a non-service group description with GetServiceGroupDescription
                error = ErrorCodeValue::AccessDenied;

                WriteWarning(
                    Constants::FabricClientSource,
                    "{0} is not a service group: error={1}",
                    operation->associatedName_,
                    error);
            }
        }
        else if (error.IsError(ErrorCodeValue::UserServiceNotFound))
        {
            // translate to service group specific error code
            error = ErrorCodeValue::UserServiceGroupNotFound;
        }

        return error;
    }
}
