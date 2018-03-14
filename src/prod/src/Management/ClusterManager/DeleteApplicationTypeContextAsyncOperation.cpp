// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Store;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("DeleteApplicationTypeContextAsyncOperation");

DeleteApplicationTypeContextAsyncOperation::DeleteApplicationTypeContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ApplicationTypeContext & context,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessRolloutContextAsyncOperationBase(
        rolloutManager,
        context.ReplicaActivityId,
        timeout,
        callback, 
        root)
    , context_(context)
    , startedUnprovision_(false)
{
}

void DeleteApplicationTypeContextAsyncOperation::OnCompleted()
{
    if (startedUnprovision_)
    {
        this->Replica.AppTypeRequestTracker.FinishRequest(context_.TypeName, context_.TypeVersion);
    }
}

void DeleteApplicationTypeContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto error = this->Replica.AppTypeRequestTracker.TryStartUnprovision(context_.TypeName, context_.TypeVersion);

    StoreDataServiceManifest storeDataServiceManifest(context_.TypeName, context_.TypeVersion);
    auto storeTx = this->CreateTransaction();

    if (error.IsSuccess())
    {
        startedUnprovision_ = true;

        WriteInfo(
            TraceComponent,
            "{0} unprovisioning: type={1} version={2} async={3}",
            this->TraceId,
            context_.TypeName,
            context_.TypeVersion,
            context_.IsAsync);

        if (context_.IsAsync)
        {
            context_.CompleteClientRequest(ErrorCodeValue::Success);
        }

        error = storeTx.ReadExact(storeDataServiceManifest);

        // ApplicationTypeContext can exist without StoreDataServiceManifest if
        // validation of the application manifest fails.
        //
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCodeValue::Success;
        }
    }

    vector<StoreDataServiceManifest> existingManifests;
    if (error.IsSuccess())
    {
        error = storeTx.ReadPrefix(Constants::StoreType_ServiceManifest, storeDataServiceManifest.GetTypeNameKeyPrefix(), existingManifests);
    }

    bool shouldCleanup = false;
    bool isLastApplicationType = false;

    if (error.IsSuccess())
    {
        error = this->CheckCleanup_IgnoreCase(storeTx, shouldCleanup, isLastApplicationType);
    }

    if (error.IsSuccess() && shouldCleanup)
    {
        storeDataServiceManifest.TrimDuplicateManifestsAndPackages(existingManifests);

        error = this->ImageBuilder.CleanupApplicationType(
            context_.TypeName,
            context_.TypeVersion,
            storeDataServiceManifest.ServiceManifests,
            isLastApplicationType,
            context_.IsAsync ? TimeSpan::MaxValue : this->RemainingTimeout);
    }

    if (!error.IsSuccess() && this->IsImageBuilderSuccessOnCleanup(error))
    {
        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        error = storeTx.DeleteIfFound(context_);

        if (error.IsSuccess())
        {
            StoreDataApplicationManifest storeDataApplicationManifest(context_.TypeName, context_.TypeVersion);
            error = storeTx.DeleteIfFound(storeDataApplicationManifest);
        }

        if (error.IsSuccess())
        {
            error = storeTx.DeleteIfFound(storeDataServiceManifest);
        }
    }

    if (error.IsSuccess())
    {
        this->CommitDeleteApplicationType(thisSPtr, storeTx);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void DeleteApplicationTypeContextAsyncOperation::CommitDeleteApplicationType(Common::AsyncOperationSPtr const& thisSPtr, StoreTransaction& storeTx)
{
    auto operation = StoreTransaction::BeginCommit(
        move(storeTx),
        context_,
        [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
        thisSPtr);
    this->OnCommitComplete(operation, true);
}

void DeleteApplicationTypeContextAsyncOperation::OnCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    this->TryComplete(operation->Parent, error);
}

ErrorCode DeleteApplicationTypeContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<DeleteApplicationTypeContextAsyncOperation>(operation)->Error;
}

ErrorCode DeleteApplicationTypeContextAsyncOperation::CheckCleanup_IgnoreCase(
    StoreTransaction const & storeTx,
    __out bool & shouldCleanup,
    __out bool & isLastApplicationType)
{
    vector<ApplicationTypeContext> appTypeContexts;
    auto error = storeTx.ReadPrefix<ApplicationTypeContext>(Constants::StoreType_ApplicationTypeContext, L"", appTypeContexts);    

    if (!error.IsSuccess())
    {
        return error;
    }

    shouldCleanup = true;
    isLastApplicationType = true;

    for (auto const & other : appTypeContexts)
    {
        if (context_.TypeName != other.TypeName)
        {
            continue;
        }

        if (context_.ConstructKey() == other.ConstructKey())
        {
            continue;
        }

        // Another context with the same type name still exists
        //
        isLastApplicationType = false;

        if (context_.TypeVersion == other.TypeVersion)
        {
            // There are other contexts of the same type+version with different
            // casing, so don't remove any files from Image Store yet.
            //
            shouldCleanup = false;

            break;
        }
    }

    return ErrorCodeValue::Success;
}
