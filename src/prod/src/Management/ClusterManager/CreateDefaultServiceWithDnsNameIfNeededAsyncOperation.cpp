// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::ClusterManager;

CreateDefaultServiceWithDnsNameIfNeededAsyncOperation::CreateDefaultServiceWithDnsNameIfNeededAsyncOperation(
    ServiceContext const &defaultServiceContext,
    __in ProcessRolloutContextAsyncOperationBase &owner,
    Common::ActivityId const &activityId,
    TimeSpan timeout,
    AsyncCallback callback,
    AsyncOperationSPtr const &parent)
    : TimedAsyncOperation(timeout, callback, parent)
    , defaultServiceContext_(defaultServiceContext)
    , owner_(owner)
    , activityId_(activityId)
{
}

/*static*/ ErrorCode CreateDefaultServiceWithDnsNameIfNeededAsyncOperation::End(AsyncOperationSPtr const &operation)
{
    auto casted = AsyncOperation::End<CreateDefaultServiceWithDnsNameIfNeededAsyncOperation>(operation);
    return casted->Error;
}

void CreateDefaultServiceWithDnsNameIfNeededAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    if (!defaultServiceContext_.ServiceDescriptor.Service.ServiceDnsName.empty())
    {
        auto operation = this->owner_.BeginCreateServiceDnsName(
            defaultServiceContext_.ServiceDescriptor.Service.Name,
            defaultServiceContext_.ServiceDescriptor.Service.ServiceDnsName,
            activityId_,
            this->RemainingTime,
            [this](AsyncOperationSPtr const &operation)
        {
            this->OnCreateDnsNameComplete(operation, false);
        },
            thisSPtr);

        this->OnCreateDnsNameComplete(operation, true);
    }
    else
    {
        auto operation = this->owner_.Client.BeginCreateService(
            defaultServiceContext_.ServiceDescriptor,
            defaultServiceContext_.PackageVersion,
            defaultServiceContext_.PackageInstance,
            activityId_,
            this->RemainingTime,
            [this](AsyncOperationSPtr const &operation)
        {
            this->OnCreateServiceComplete(operation, false);
        },
            thisSPtr);

        this->OnCreateServiceComplete(operation, true);
    }
}

void CreateDefaultServiceWithDnsNameIfNeededAsyncOperation::OnCreateDnsNameComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = this->owner_.EndCreateServiceDnsName(operation);
    if (!error.IsSuccess())
    {
        // dns name creation failed
        this->TryComplete(operation->Parent, move(error));
        return;
    }

    auto innerOp = this->owner_.Client.BeginCreateService(
        defaultServiceContext_.ServiceDescriptor,
        defaultServiceContext_.PackageVersion,
        defaultServiceContext_.PackageInstance,
        activityId_,
        this->RemainingTime,
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnCreateServiceComplete(operation, false);
    },
        operation->Parent);

    this->OnCreateServiceComplete(innerOp, true);
}

void CreateDefaultServiceWithDnsNameIfNeededAsyncOperation::OnCreateServiceComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = this->owner_.Client.EndCreateService(operation);
    if (error.IsError(ErrorCodeValue::UserServiceAlreadyExists))
    {
        error = ErrorCode::Success();
    }

    this->TryComplete(operation->Parent, move(error));
}
