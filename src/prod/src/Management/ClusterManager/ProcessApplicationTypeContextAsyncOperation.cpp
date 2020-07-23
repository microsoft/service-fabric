// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Store;
using namespace ServiceModel;
using namespace Management::ClusterManager;

static StringLiteral const ApplicationTypeContextTraceComponent("ApplicationTypeContextAsyncOperation");

ProcessApplicationTypeContextAsyncOperation::ProcessApplicationTypeContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ApplicationTypeContext & context,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessProvisionContextAsyncOperationBase(
        rolloutManager, 
        context,
        ApplicationTypeContextTraceComponent,
        timeout,
        callback, 
        root)
    , imageBuilderAsyncOperationExecutor_()
    , applicationManifestId_()
{
    imageBuilderAsyncOperationExecutor_ = make_unique<ImageBuilderAsyncOperationExecutor>(*this);
}

ErrorCode ProcessApplicationTypeContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ProcessApplicationTypeContextAsyncOperation>(operation)->Error;
}

// Do not use AsyncOperation::Cancel() since there is still some
// cleanup work after the ImageBuilder async operation aborts
// that requires creating further async operation children
// (i.e. for commit) that won't start properly if this async operation
// (the parent) is already canceled.
// 
void ProcessApplicationTypeContextAsyncOperation::ExternalCancel()
{
    imageBuilderAsyncOperationExecutor_->ExternalCancel();
}

void ProcessApplicationTypeContextAsyncOperation::OnCompleted()
{
    if (startedProvision_)
    {
        this->Replica.AppTypeRequestTracker.FinishRequest(this->AppTypeContext.TypeName, this->AppTypeContext.TypeVersion);
    }

    imageBuilderAsyncOperationExecutor_->ResetOperation();

    __super::OnCompleted();
}

void ProcessApplicationTypeContextAsyncOperation::StartProvisioning(AsyncOperationSPtr const & thisSPtr)
{
    provisioningError_ = this->CheckApplicationTypes_IgnoreCase();

    if (provisioningError_.IsSuccess())
    {
        WriteInfo(
            ApplicationTypeContextTraceComponent,
            "{0} provisioning: type={1} version={2} async={3}",
            this->TraceId,
            this->AppTypeContext.TypeName,
            this->AppTypeContext.TypeVersion,
            this->AppTypeContext.IsAsync);

        if (this->AppTypeContext.IsAsync)
        {
            this->AppTypeContext.CompleteClientRequest(ErrorCodeValue::Success);
        }

        auto operation = this->Replica.ImageBuilder.BeginBuildApplicationType(
            this->ActivityId,
            this->AppTypeContext.BuildPath,
            this->AppTypeContext.ApplicationPackageDownloadUri,
            this->AppTypeContext.TypeName,
            this->AppTypeContext.TypeVersion,
            [this, thisSPtr](wstring const & details) { this->OnUpdateProgressDetails(thisSPtr, details); },
            this->AppTypeContext.IsAsync ? TimeSpan::MaxValue : this->RemainingTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnBuildApplicationTypeComplete(operation, false); },
            thisSPtr);

        imageBuilderAsyncOperationExecutor_->AddOperation(operation);

        this->OnBuildApplicationTypeComplete(operation, true);
    }
    else
    {
        this->FinishBuildApplicationType(thisSPtr);
    }
}

void ProcessApplicationTypeContextAsyncOperation::OnBuildApplicationTypeComplete(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    imageBuilderAsyncOperationExecutor_->WaitProgressCallback();

    auto const & thisSPtr = operation->Parent;

    vector<ServiceModelServiceManifestDescription> manifests;
    wstring applicationManifestId;
    wstring applicationManifest;
    ApplicationHealthPolicy healthPolicy;
    map<wstring, wstring> defaultParamList;

    auto error = this->Replica.ImageBuilder.EndBuildApplicationType(
        operation,
        manifests,
        applicationManifestId,
        applicationManifest,
        healthPolicy,
        defaultParamList);

    if (error.IsSuccess())
    {
        applicationManifestId_ = applicationManifestId;

        appManifest_ = make_shared<StoreDataApplicationManifest>(
            this->AppTypeContext.TypeName, 
            this->AppTypeContext.TypeVersion, 
            move(applicationManifest), 
            move(healthPolicy), 
            move(defaultParamList));

        svcManifest_ = make_shared<StoreDataServiceManifest>(
            this->AppTypeContext.TypeName, 
            this->AppTypeContext.TypeVersion,
            move(manifests));
    }
    else
    {
        provisioningError_ = error;
    }

    this->FinishBuildApplicationType(thisSPtr);
}

void ProcessApplicationTypeContextAsyncOperation::FinishBuildApplicationType(AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto storeTx = this->CreateTransaction();

    this->RefreshContext(storeTx);

    if (provisioningError_.IsSuccess())
    {
        this->AppTypeContext.ManifestDataInStore = true;

        this->AppTypeContext.QueryStatusDetails = L"";

        this->AppTypeContext.ManifestId = applicationManifestId_;

        error = this->AppTypeContext.Complete(storeTx);
        
        if (error.IsSuccess())
        {
            error = storeTx.UpdateOrInsertIfNotFound(*appManifest_);
        }

        if (error.IsSuccess())
        {
            error = storeTx.UpdateOrInsertIfNotFound(*svcManifest_);
        }
    }
    else if (this->AppTypeContext.IsAsync)
    {
        this->AppTypeContext.QueryStatusDetails = wformatString(
            "{0}{1}", 
            provisioningError_.ReadValue(), 
            provisioningError_.Message.empty() ? L"" : wformatString(": {0}", provisioningError_.Message));

        error = this->AppTypeContext.Fail(storeTx);
    }
    else
    {
        error = provisioningError_;
    }

    if (error.IsSuccess())
    {
        auto innerOperation = StoreTransaction::BeginCommit(
            move(storeTx),
            this->AppTypeContext,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(innerOperation, true);
    }
    else if (this->ShouldRefreshAndRetry(error))
    {
        WriteInfo(
            ApplicationTypeContextTraceComponent,
            "{0} retry persist provisioning results: error={1}",
            this->TraceId,
            error);

        Threadpool::Post(
            [this, thisSPtr]() { this->FinishBuildApplicationType(thisSPtr); }, 
            this->Replica.GetRandomizedOperationRetryDelay(error));
    }
    else
    {
        WriteInfo(
            ApplicationTypeContextTraceComponent,
            "{0} provisioning failed: error={1}",
            this->TraceId,
            error);

        this->TryComplete(thisSPtr, move(error));
    }
}

void ProcessApplicationTypeContextAsyncOperation::OnUpdateProgressDetails(AsyncOperationSPtr const & thisSPtr, wstring const & details)
{
    if (!imageBuilderAsyncOperationExecutor_->OnUpdateProgressDetails(details))
    {
        // Nothing to be done
        return;
    }

    auto storeTx = this->CreateTransaction();

    this->RefreshContext(storeTx);

    this->AppTypeContext.QueryStatusDetails = details;

    auto error = storeTx.Update(this->AppTypeContext);

    if (!error.IsSuccess())
    {
        WriteInfo(
            ApplicationTypeContextTraceComponent,
            "{0} failed to update provisioning status details: error={1}",
            this->TraceId,
            error);

        imageBuilderAsyncOperationExecutor_->OnUpdateProgressDetailsComplete();
    }
    else
    {
        WriteInfo(
            ApplicationTypeContextTraceComponent,
            "{0} commit provisioning status details '{1}': error={2}",
            this->TraceId,
            this->AppTypeContext.QueryStatusDetails,
            error);

        auto operation = StoreTransaction::BeginCommit(
            move(storeTx),
            this->AppTypeContext,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitProgressDetailsComplete(operation, false); },
            thisSPtr);
        this->OnCommitProgressDetailsComplete(operation, true);
    }
}

void ProcessApplicationTypeContextAsyncOperation::OnCommitProgressDetailsComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = StoreTransaction::EndCommit(operation);

    if (!error.IsSuccess())
    {
        WriteInfo(
            ApplicationTypeContextTraceComponent,
            "{0} failed to commit provisioning status details: error={1}",
            this->TraceId,
            error);
    }

    imageBuilderAsyncOperationExecutor_->OnUpdateProgressDetailsComplete();
}

void ProcessApplicationTypeContextAsyncOperation::RefreshContext(StoreTransaction const & storeTx)
{
    auto error = this->AppTypeContext.Refresh(storeTx);

    if (!error.IsSuccess())
    {
        WriteInfo(
            ApplicationTypeContextTraceComponent,
            "{0} failed to refresh context: error={1}",
            this->TraceId,
            error);
    }
}


