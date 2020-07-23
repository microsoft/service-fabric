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

ProcessProvisionContextAsyncOperationBase::ProcessProvisionContextAsyncOperationBase(
    __in RolloutManager & rolloutManager,
    __in ApplicationTypeContext & context,
    StringLiteral const traceComponent,
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
    , traceComponent_(traceComponent)
    , startedProvision_(false)
    , appManifest_()
    , svcManifest_()
    , provisioningError_(ErrorCodeValue::Success)
{
}

void ProcessProvisionContextAsyncOperationBase::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto error = this->Replica.AppTypeRequestTracker.TryStartProvision(
        context_.TypeName, 
        context_.TypeVersion, 
        dynamic_pointer_cast<ProcessApplicationTypeContextAsyncOperation>(thisSPtr));

    if (error.IsSuccess())
    {
        startedProvision_ = true;

        this->StartProvisioning(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, move(error));
    }
}

bool ProcessProvisionContextAsyncOperationBase::ShouldCleanupApplicationPackage() const
{
    // Cleanup only on successful provision
    if (context_.IsAsync && !provisioningError_.IsSuccess())
    {
        return false;
    }

    switch (context_.ApplicationPackageCleanupPolicy)
    {
    case ApplicationPackageCleanupPolicy::Automatic:
        // User requested automatic cleanup on succesful provision.
        return true;
    case ApplicationPackageCleanupPolicy::Default:
    case ApplicationPackageCleanupPolicy::Invalid:
        return ManagementConfig::GetConfig().CleanupApplicationPackageOnProvisionSuccess;
    default:
        return false;
    }
}

void ProcessProvisionContextAsyncOperationBase::OnCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = StoreTransaction::EndCommit(operation);

    // Check if application package needs to be deleted
    if (error.IsSuccess() && this->ShouldCleanupApplicationPackage())
    {
        WriteNoise(
            TraceComponent,
            "{0}: Starting deletion of application package remaining time: {1}",
            this->TraceId,
            RemainingTimeout);

        error = this->Replica.ImageBuilder.CleanupApplicationPackage(this->context_.BuildPath, RemainingTimeout);
        
        WriteInfo(
            TraceComponent,
            "{0}: application package deletion completed with error {1} on provision success. BuildPath: {2} remaining timeout: {3} Policy: {4}",
            this->TraceId,
            error,
            this->context_.BuildPath,
            RemainingTimeout,
            this->context_.ApplicationPackageCleanupPolicy);
    }

    if (error.IsSuccess())
    {
        if (ManagementConfig::GetConfig().TriggerAppTypeCleanupOnProvisionSuccess)
        {
            // Trigger automatic app type cleanup for this app type name
            Common::ActivityId activityId;
            this->Replica.QueueAutomaticCleanupApplicationType(this->context_.TypeName.Value, activityId);
        }
    }

    this->TryComplete(operation->Parent, move(error));
}

// Perform case insensitive check within the tracking performed by ApplicationTypeRequestTracker so that there is
// no race between concurrent provisioning requests for the same application type name. 
//
ErrorCode ProcessProvisionContextAsyncOperationBase::CheckApplicationTypes_IgnoreCase()
{
    auto storeTx = this->CreateTransaction();

    vector<ApplicationTypeContext> appTypeContexts;
    auto error = storeTx.ReadPrefix<ApplicationTypeContext>(Constants::StoreType_ApplicationTypeContext, L"", appTypeContexts);    

    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto const & other : appTypeContexts)
    {
        if (context_.ConstructKey() == other.ConstructKey())
        {
            continue;
        }

        if (context_.TypeName == other.TypeName && context_.TypeVersion == other.TypeVersion)
        {
            auto msg = wformatString(
                GET_RC( ApplicationTypeAlreadyProvisioned ),
                context_.TypeName,
                context_.TypeVersion,
                other.TypeName,
                other.TypeVersion);

            WriteWarning(
                TraceComponent,
                "{0}: {1}",
                this->TraceId,
                msg);

            return ErrorCode(ErrorCodeValue::ApplicationTypeAlreadyExists, move(msg));
        }
    }

    return ErrorCodeValue::Success;
}

bool ProcessProvisionContextAsyncOperationBase::ShouldRefreshAndRetry(ErrorCode const & error)
{
    return (error.IsError(ErrorCodeValue::StoreSequenceNumberCheckFailed) ||
            error.IsError(ErrorCodeValue::StoreWriteConflict));
}

