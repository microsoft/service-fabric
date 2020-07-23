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
using namespace Reliability;

static StringLiteral const ComposeDeploymentUpgradeTraceComponent("ComposeDeploymentUpgradeContextAsyncOperation");
static StringLiteral const ProvisionTraceComponent("ComposeDeploymentUpgradeContextAsyncOperation.Provision");
static StringLiteral const ComposeApplicationUpgradeTraceComponent("ComposeDeploymentUpgradeContextAsyncOperation.Upgrade");

class ProcessComposeDeploymentUpgradeAsyncOperation::InnerProvisionAsyncOperation
    : public ProcessProvisionContextAsyncOperationBase
{
    DENY_COPY(InnerProvisionAsyncOperation);

public:
    InnerProvisionAsyncOperation(
        __in RolloutManager & rolloutManager,
        __in ProcessComposeDeploymentUpgradeAsyncOperation & owner,
        __in ApplicationTypeContext & context,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessProvisionContextAsyncOperationBase(
            rolloutManager,
            context,
            ComposeDeploymentUpgradeTraceComponent,
            timeout,
            callback,
            root)
        , owner_(owner)
        , mergedComposeFile_()
        , composeDeploymentInstanceData_(owner_.ComposeUpgradeContext.DeploymentName, owner_.ComposeUpgradeContext.TargetTypeVersion)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<InnerProvisionAsyncOperation>(operation)->Error;
    }

    void OnCompleted() override
    {
        if (startedProvision_)
        {
            this->Replica.AppTypeRequestTracker.FinishRequest(this->AppTypeContext.TypeName, this->AppTypeContext.TypeVersion);
        }

        __super::OnCompleted();
    }

private:
    void StartProvisioning(AsyncOperationSPtr const & thisSPtr)
    {
        provisioningError_ = this->CheckApplicationTypes_IgnoreCase();

        if (provisioningError_.IsSuccess())
        {
            provisioningError_ = owner_.GetStoreDataComposeDeploymentInstance(composeDeploymentInstanceData_);
        }

        if (provisioningError_.IsSuccess())
        {
            WriteInfo(
                ProvisionTraceComponent,
                "{0} provisioning: type={1} version={2}",
                this->TraceId,
                this->AppTypeContext.TypeName,
                this->AppTypeContext.TypeVersion);

            //
            // This is happening as a part of Docker compose Application creation, which is async. So
            // the imagebuilder timeout is maxval.
            //
            auto operation = this->ImageBuilder.BeginBuildComposeApplicationTypeForUpgrade(
                this->ActivityId,
                composeDeploymentInstanceData_.ComposeFiles[0],
                ByteBufferSPtr(),
                composeDeploymentInstanceData_.RepositoryUserName,
                composeDeploymentInstanceData_.RepositoryPassword,
                composeDeploymentInstanceData_.IsPasswordEncrypted,
                owner_.ComposeUpgradeContext.ApplicationName,
                owner_.ComposeUpgradeContext.TypeName,
                owner_.ComposeUpgradeContext.CurrentTypeVersion,
                owner_.ComposeUpgradeContext.TargetTypeVersion,
                TimeSpan::MaxValue,
                [this](AsyncOperationSPtr const & operation) { this->OnBuildComposeDeploymentApplicationTypeComplete(operation, false); },
                thisSPtr);

            this->OnBuildComposeDeploymentApplicationTypeComplete(operation, true);
        }
        else
        {
            this->FinishBuildComposeDeploymentApplicationType(thisSPtr);
        }
    }

    void OnBuildComposeDeploymentApplicationTypeComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (this->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        vector<ServiceModelServiceManifestDescription> serviceManifest;
        wstring applicationManifestId;
        wstring applicationManifest;
        ApplicationHealthPolicy applicationHealthPolicy;
        wstring mergedComposeFile;
        map<wstring, wstring> defaultParamList;

        ErrorCode error = this->ImageBuilder.EndBuildComposeApplicationTypeForUpgrade(
            operation,
            serviceManifest,
            applicationManifestId,
            applicationManifest,
            applicationHealthPolicy,
            defaultParamList,
            mergedComposeFile);

        if (error.IsSuccess())
        {
            appManifest_ = make_shared<StoreDataApplicationManifest>(
                this->AppTypeContext.TypeName,
                this->AppTypeContext.TypeVersion,
                move(applicationManifest),
                move(applicationHealthPolicy),
                move(defaultParamList));

            svcManifest_ = make_shared<StoreDataServiceManifest>(
                this->AppTypeContext.TypeName,
                this->AppTypeContext.TypeVersion,
                move(serviceManifest));

            mergedComposeFile_ = move(mergedComposeFile);
        }
        else
        {
            provisioningError_ = error;
        }

        this->FinishBuildComposeDeploymentApplicationType(operation->Parent);
    }

    void FinishBuildComposeDeploymentApplicationType(AsyncOperationSPtr const & thisSPtr)
    {
        auto storeTx = this->CreateTransaction();

        ErrorCode error = owner_.ComposeUpgradeContext.Refresh(storeTx);
        if (!error.IsSuccess())
        {
            WriteInfo(
                ProvisionTraceComponent,
                "{0} failed to refresh context: error={1}",
                this->TraceId,
                error);
        }

        if (provisioningError_.IsSuccess())
        {
            this->AppTypeContext.ManifestDataInStore = true;

            this->AppTypeContext.QueryStatusDetails = L"";

            error = storeTx.Insert(this->AppTypeContext);

            if (error.IsSuccess())
            {
                error = this->AppTypeContext.Complete(storeTx);
            }

            if (error.IsSuccess())
            {
                error = storeTx.UpdateOrInsertIfNotFound(*appManifest_);
            }

            if (error.IsSuccess())
            {
                error = storeTx.UpdateOrInsertIfNotFound(*svcManifest_);
            }

            if (error.IsSuccess())
            {
                composeDeploymentInstanceData_.UpdateMergedComposeFile(storeTx, mergedComposeFile_);

                if (owner_.ComposeUpgradeContext.Interrupted)
                {
                    owner_.ComposeUpgradeContext.VersionToUnprovision = owner_.ComposeUpgradeContext.TargetTypeVersion;
                    error = owner_.ComposeUpgradeContext.StartUnprovisionTarget(
                        storeTx,
                        wformatString(
                            GET_RC(Deployment_Rolledback_To_Version),
                            owner_.ComposeUpgradeContext.CurrentTypeVersion));
                }
                else
                {
                    error = owner_.ComposeUpgradeContext.StartUpgrade(
                        storeTx,
                        wformatString(
                            GET_RC(Deployment_Upgrading_To_Version),
                            owner_.ComposeUpgradeContext.CurrentTypeVersion,
                            owner_.ComposeUpgradeContext.TargetTypeVersion));
                    if (error.IsSuccess())
                    {
                        error = SetupApplicationContextForUpgrade(storeTx);
                    }
                }
            }
        }
        else
        {
            error = provisioningError_;
        }

        if (error.IsSuccess())
        {
            auto innerOperation = StoreTransaction::BeginCommit(
                move(storeTx),
                owner_.ComposeUpgradeContext,
                [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                thisSPtr);
            this->OnCommitComplete(innerOperation, true);
        }
        else if (this->ShouldRefreshAndRetry(error))
        {
            WriteInfo(
                ProvisionTraceComponent,
                "{0} retry persist provisioning results: error={1}",
                this->TraceId,
                error);

            Threadpool::Post(
                [this, thisSPtr]() { this->FinishBuildComposeDeploymentApplicationType(thisSPtr); },
                this->Replica.GetRandomizedOperationRetryDelay(error));
        }
        else
        {
            WriteInfo(
                ProvisionTraceComponent,
                "{0} provisioning failed: error={1}",
                this->TraceId,
                error);

            this->TryComplete(thisSPtr, error);
        }
    }

    ErrorCode SetupApplicationContextForUpgrade(StoreTransaction const &storeTx)
    {
        auto appContext = make_unique<ApplicationContext>(owner_.ComposeUpgradeContext.ApplicationName);
        auto error = owner_.Replica.GetCompletedApplicationContext(storeTx, *appContext);
        if (!error.IsSuccess())
        {
            // We should be able to read the application context. When would this read fail?
            WriteWarning(
                ProvisionTraceComponent,
                "{0} Unable to get completed application context during compose deployment upgrade: {1}",
                this->TraceId,
                error);

            // The app context is updated along with the compose context. This condition
            // indicates that there is an inconsistency between the 2 contexts.
            Assert::TestAssert(
                ProvisionTraceComponent,
                "{0} Unable to get completed application context during compose deployment upgrade: {1}",
                this->TraceId,
                error);

            return error;
        }

        bool readExisting = false;
        auto upgradeInstance = ApplicationUpgradeContext::GetNextUpgradeInstance(appContext->PackageInstance);

        owner_.applicationUpgradeContext_ = make_unique<ApplicationUpgradeContext>(
            owner_.ComposeUpgradeContext.ReplicaActivityId,
            owner_.ComposeUpgradeContext.UpgradeDescription,
            owner_.ComposeUpgradeContext.CurrentTypeVersion,
            L"", // targetApplicationManifestId
            upgradeInstance,
            ApplicationDefinitionKind::Enum::Compose);

        error = storeTx.TryReadOrInsertIfNotFound(*owner_.applicationUpgradeContext_, readExisting);
        if (!error.IsSuccess())
        {
            return error;
        }

        if (readExisting)
        {
            if (!owner_.applicationUpgradeContext_->IsComplete && !owner_.applicationUpgradeContext_->IsFailed)
            {
                Assert::TestAssert(
                    ProvisionTraceComponent,
                    "{0} Unable to get completed or failed application upgrade context during compose deployment upgrade. Status: {1}",
                    this->TraceId,
                    owner_.applicationUpgradeContext_->Status);
            }
            auto seqNumber = owner_.applicationUpgradeContext_->SequenceNumber;
            owner_.applicationUpgradeContext_ = make_unique<ApplicationUpgradeContext>(
                owner_.ComposeUpgradeContext.ReplicaActivityId,
                owner_.ComposeUpgradeContext.UpgradeDescription,
                owner_.ComposeUpgradeContext.CurrentTypeVersion,
                L"", // targetApplicationManifestId
                upgradeInstance,
                ApplicationDefinitionKind::Enum::Compose);
            owner_.applicationUpgradeContext_->SetSequenceNumber(seqNumber);

            error = storeTx.Update(*owner_.applicationUpgradeContext_);
            if (!error.IsSuccess()) { return error; }

            // Ensure verified domains are cleared since we may be recovering from a failed
            // upgrade that never completed or rolled back.
            //
            auto verifiedDomainsStoreData = make_unique<StoreDataVerifiedUpgradeDomains>(appContext->ApplicationId);

            error = owner_.Replica.ClearVerifiedUpgradeDomains(storeTx, *verifiedDomainsStoreData);
            if (!error.IsSuccess()) { return error; }
        }

        return appContext->SetUpgradePending(storeTx);
    }

    ProcessComposeDeploymentUpgradeAsyncOperation & owner_;
    wstring mergedComposeFile_;
    StoreDataComposeDeploymentInstance composeDeploymentInstanceData_;
};

class ProcessComposeDeploymentUpgradeAsyncOperation::InnerApplicationUpgradeAsyncOperation
    : public ProcessApplicationUpgradeContextAsyncOperation
{
public:
    InnerApplicationUpgradeAsyncOperation(
        __in RolloutManager &rolloutManager,
        __in ApplicationUpgradeContext &upgradeContext,
        __in ProcessComposeDeploymentUpgradeAsyncOperation & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessApplicationUpgradeContextAsyncOperation(
            rolloutManager,
            upgradeContext,
            "ComposeDeploymentUpgradeContextAsyncOperation.Upgrade",
            callback,
            root)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<InnerApplicationUpgradeAsyncOperation>(operation)->Error;
    }

protected:
    virtual bool Get_SkipRollbackUpdateDefaultService() override { return false; }
    //
    // Default service should always be updated/deleted as a part of compose deployment upgrade.
    //
    virtual bool Get_EnableDefaultServicesUpgrade() override { return true; }

    virtual Common::ErrorCode OnRefreshUpgradeContext(Store::StoreTransaction const &storeTx) override
    {
        // UpgradeContext is not active context in RolloutManager, need to explicitly set ExternallyFail after Refresh.
        if (owner_.ComposeUpgradeContext.IsExternallyFailed())
        {
            this->UpgradeContext.ExternalFail();
        }
        return owner_.ComposeUpgradeContext.Refresh(storeTx);
    }

    virtual Common::ErrorCode OnFinishUpgrade(StoreTransaction const &storeTx) override
    {
        auto composeDeploymentContext = make_unique<ComposeDeploymentContext>(owner_.ComposeUpgradeContext.DeploymentName);
        auto error = this->Replica.GetComposeDeploymentContext(storeTx, *composeDeploymentContext);
        if (!error.IsSuccess())
        {
            return error;
        }

        composeDeploymentContext->TypeVersion = owner_.ComposeUpgradeContext.TargetTypeVersion;
        error = storeTx.Update(*composeDeploymentContext);
        if (!error.IsSuccess())
        {
            return error;
        }

        owner_.ComposeUpgradeContext.VersionToUnprovision = owner_.ComposeUpgradeContext.CurrentTypeVersion;
        owner_.ComposeUpgradeContext.ResetUpgradeDescription();

        return owner_.ComposeUpgradeContext.StartUnprovision(
            storeTx,
            wformatString(GET_RC(Deployment_Upgraded_To_Version), owner_.ComposeUpgradeContext.TargetTypeVersion));
    }

    virtual Common::ErrorCode OnFinishRollback(StoreTransaction const &storeTx) override
    {
        auto composeDeploymentContext = make_unique<ComposeDeploymentContext>(owner_.ComposeUpgradeContext.DeploymentName);
        auto error = this->Replica.GetComposeDeploymentContext(storeTx, *composeDeploymentContext);
        if (!error.IsSuccess())
        {
            return error;
        }

        composeDeploymentContext->TypeVersion = owner_.ComposeUpgradeContext.CurrentTypeVersion;
        error = storeTx.Update(*composeDeploymentContext);
        if (!error.IsSuccess())
        {
            return error;
        }

        owner_.ComposeUpgradeContext.VersionToUnprovision = owner_.ComposeUpgradeContext.TargetTypeVersion;
        owner_.ComposeUpgradeContext.ResetUpgradeDescription();

        return owner_.ComposeUpgradeContext.StartUnprovisionTarget(
            storeTx,
            wformatString(GET_RC(Deployment_Rolledback_To_Version), owner_.ComposeUpgradeContext.CurrentTypeVersion));
    }

    virtual Common::ErrorCode OnStartRollback(StoreTransaction const &storeTx) override
    {
        WriteInfo(
            ComposeApplicationUpgradeTraceComponent,
            "{0} Compose deployment: {1} rollback initiated during application upgrade",
            TraceId,
            owner_.ComposeUpgradeContext.DeploymentName);

        return owner_.ComposeUpgradeContext.StartRollback(
            storeTx,
            wformatString(GET_RC(Deployment_Rollingback_To_Version), owner_.ComposeUpgradeContext.TargetTypeVersion, owner_.ComposeUpgradeContext.CurrentTypeVersion));
    }

private:
    ProcessComposeDeploymentUpgradeAsyncOperation & owner_;
};

class ProcessComposeDeploymentUpgradeAsyncOperation::InnerUnprovisionAsyncOperation
    : public DeleteApplicationTypeContextAsyncOperation
{
public:
    InnerUnprovisionAsyncOperation(
        __in ProcessComposeDeploymentUpgradeAsyncOperation &owner,
        __in RolloutManager &rolloutManager,
        __in ApplicationTypeContext &appTypeContext,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent)
        : DeleteApplicationTypeContextAsyncOperation(
            rolloutManager,
            appTypeContext,
            timeout,
            callback,
            parent)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<InnerUnprovisionAsyncOperation>(operation)->Error;
    }

protected:
    void CommitDeleteApplicationType(AsyncOperationSPtr const & thisSPtr, StoreTransaction &storeTx) override
    {
        auto error = owner_.UpdateContextStatusAfterUnprovision(storeTx);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        auto operation = StoreTransaction::BeginCommit(
            move(storeTx),
            owner_.ComposeUpgradeContext,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }

private:
    ProcessComposeDeploymentUpgradeAsyncOperation &owner_;
};

ProcessComposeDeploymentUpgradeAsyncOperation::ProcessComposeDeploymentUpgradeAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ComposeDeploymentUpgradeContext & context,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessRolloutContextAsyncOperationBase(
        rolloutManager,
        context.ReplicaActivityId,
        TimeSpan::MaxValue,
        callback,
        root)
    , context_(context)
    , targetApplicationTypeContext_(
        context_.ReplicaActivityId,
        context_.TypeName,
        context_.TargetTypeVersion,
        false,
        ApplicationTypeDefinitionKind::Enum::Compose,
        L"")
    , applicationUpgradeContext_()
{
}

void ProcessComposeDeploymentUpgradeAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (context_.ShouldProvision())
    {
        this->StartProvisionApplicationType(thisSPtr);
    }
    else if (context_.ShouldUpgrade())
    {
        this->StartUpgradeApplication(thisSPtr);
    }
    else
    {
        this->StartUnprovisionApplicationType(thisSPtr);
    }
}

ErrorCode ProcessComposeDeploymentUpgradeAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ProcessComposeDeploymentUpgradeAsyncOperation>(operation)->Error;
}

void ProcessComposeDeploymentUpgradeAsyncOperation::StartProvisionApplicationType(AsyncOperationSPtr const & thisSPtr)
{
    WriteInfo(
        ComposeApplicationUpgradeTraceComponent,
        "{0} Deployment : {1} starting provision for version {2}",
        TraceId,
        ComposeUpgradeContext.DeploymentName,
        targetApplicationTypeContext_.TypeVersion);

    auto operation = AsyncOperation::CreateAndStart<InnerProvisionAsyncOperation>(
        const_cast<RolloutManager&>(this->Rollout),
        *this,
        targetApplicationTypeContext_,
        this->RemainingTimeout,
        [this](AsyncOperationSPtr const & operation) { this->OnProvisionComposeDeploymentTypeComplete(operation, false); },
        thisSPtr);
    this->OnProvisionComposeDeploymentTypeComplete(operation, true);
}

void ProcessComposeDeploymentUpgradeAsyncOperation::OnProvisionComposeDeploymentTypeComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (this->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = InnerProvisionAsyncOperation::End(operation);

    auto const & thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        // If this is a fatal error, this triggers the rollout processing for this context to cleanup and change to failed
        WriteWarning(
            ComposeDeploymentUpgradeTraceComponent,
            "{0} provision for deployment : {1} failed due to {2} {3}",
            TraceId,
            this->ComposeUpgradeContext.DeploymentName,
            error,
            error.Message);

        this->TryComplete(thisSPtr, move(error));
        return;
    }

    if (context_.ShouldUpgrade())
    {
        this->StartUpgradeApplication(thisSPtr);
    }
    else
    {
        //
        // Interrupted during/after provision.
        //
        this->StartUnprovisionApplicationType(thisSPtr);
    }
}

void ProcessComposeDeploymentUpgradeAsyncOperation::StartUpgradeApplication(AsyncOperationSPtr const & thisSPtr)
{
    auto error = this->RefreshApplicationUpgradeContext();
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
        return;
    }

    WriteInfo(
        ComposeApplicationUpgradeTraceComponent,
        "{0} Compose deployment {1}: starting application upgrade",
        TraceId,
        ComposeUpgradeContext.DeploymentName);

    auto operation = AsyncOperation::CreateAndStart<InnerApplicationUpgradeAsyncOperation>(
        const_cast<RolloutManager&>(this->Rollout),
        *applicationUpgradeContext_,
        *this,
        [this](AsyncOperationSPtr const & operation) { this->OnUpgradeApplicationComplete(operation, false); },
        thisSPtr);
    this->OnUpgradeApplicationComplete(operation, true);
}

void ProcessComposeDeploymentUpgradeAsyncOperation::OnUpgradeApplicationComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = InnerApplicationUpgradeAsyncOperation::End(operation);
    auto thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        WriteWarning(
            ComposeDeploymentUpgradeTraceComponent,
            "{0} Application upgrade for deployment : {1} failed due to ({2} {3})",
            TraceId,
            this->ComposeUpgradeContext.DeploymentName,
            error,
            error.Message);

        this->TryComplete(thisSPtr, move(error));
        return;
    }

    StartUnprovisionApplicationType(thisSPtr);
}

void ProcessComposeDeploymentUpgradeAsyncOperation::StartUnprovisionApplicationType(AsyncOperationSPtr const & thisSPtr)
{
    //
    // If apptype context is found, start the unprovision, 
    // If not, complete the upgrade/rollback.
    //
    auto storeTx = this->CreateTransaction();
    auto error = this->GetApplicationTypeContextForUnprovisioning(storeTx);
    if (error.IsError(ErrorCodeValue::ApplicationTypeNotFound)) // TODO: What happens if there is some other error
    {
        this->FinishUpgrade(thisSPtr, storeTx);
    }
    else
    {
        storeTx.CommitReadOnly();
        this->InnerUnprovisionApplicationType(thisSPtr);
    }
}

void ProcessComposeDeploymentUpgradeAsyncOperation::InnerUnprovisionApplicationType(AsyncOperationSPtr const &thisSPtr)
{
    WriteInfo(
        ComposeApplicationUpgradeTraceComponent,
        "{0} Deployment : {1} starting unprovision for version {2}",
        TraceId,
        ComposeUpgradeContext.DeploymentName,
        applicationContextToUnprovision_->TypeVersion);

    auto operation = AsyncOperation::CreateAndStart<InnerUnprovisionAsyncOperation>(
        *this,
        const_cast<RolloutManager&>(this->Rollout),
        *applicationContextToUnprovision_,
        TimeSpan::MaxValue,
        [this](AsyncOperationSPtr const & operation) { this->OnInnerUnprovisionComplete(operation, false); },
        thisSPtr);

    this->OnInnerUnprovisionComplete(operation, true);
}

void ProcessComposeDeploymentUpgradeAsyncOperation::OnInnerUnprovisionComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = InnerUnprovisionAsyncOperation::End(operation);
    auto thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        this->WriteWarning(
            ComposeDeploymentUpgradeTraceComponent,
            "{0} Unprovision application type for {1} failed with {2}",
            this->TraceId,
            this->ComposeUpgradeContext.DeploymentName,
            error);
    }
    else
    {
        this->WriteInfo(
            ComposeDeploymentUpgradeTraceComponent,
            "{0} Compose deployment upgrade for '{1}' is completed.",
            this->TraceId,
            this->ComposeUpgradeContext.DeploymentName);
    }

    this->TryComplete(thisSPtr, move(error));
}

void ProcessComposeDeploymentUpgradeAsyncOperation::FinishUpgrade(AsyncOperationSPtr const &thisSPtr, StoreTransaction &storeTx)
{
    auto error = this->UpdateContextStatusAfterUnprovision(storeTx);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
        return;
    }

    auto operation = StoreTransaction::BeginCommit(
        move(storeTx),
        ComposeUpgradeContext,
        [this](AsyncOperationSPtr const & op) { this->OnFinishUpgradeComplete(op, false); },
        thisSPtr);
    this->OnFinishUpgradeComplete(operation, true);
}

void ProcessComposeDeploymentUpgradeAsyncOperation::OnFinishUpgradeComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = StoreTransaction::EndCommit(operation);
    this->TryComplete(operation->Parent, error);
}

ErrorCode ProcessComposeDeploymentUpgradeAsyncOperation::GetStoreDataComposeDeploymentInstance(
    StoreDataComposeDeploymentInstance &composeDeploymentInstanceData)
{
    auto storeTx = this->CreateTransaction();

    auto error = storeTx.ReadExact(composeDeploymentInstanceData);
    if (!error.IsSuccess())
    {
        error = ErrorCode(error.ReadValue(), wformatString(GET_RC(Compose_Deployment_Instance_Read_Failed), context_.ApplicationName, error.ReadValue()));
    }

    storeTx.CommitReadOnly();

    return error;
}

ErrorCode ProcessComposeDeploymentUpgradeAsyncOperation::GetApplicationTypeContextForUnprovisioning(
    StoreTransaction const &storeTx)
{
    applicationContextToUnprovision_ = make_unique<ApplicationTypeContext>(
        this->ComposeUpgradeContext.TypeName,
        this->ComposeUpgradeContext.VersionToUnprovision);

    ErrorCode error = this->Replica.GetApplicationTypeContext(storeTx, *applicationContextToUnprovision_);

    if (!error.IsSuccess())
    {
        return error;
    }

    if (!applicationContextToUnprovision_->IsComplete)
    {
        this->WriteWarning(
            ComposeDeploymentUpgradeTraceComponent,
            "{0} Application type context for {1} is not ready for unprovisioning when Compose context context is ready - current state {2}",
            this->TraceId,
            ComposeUpgradeContext.DeploymentName,
            applicationContextToUnprovision_->Status);

        Assert::TestAssert(
            "{0} Application type context for {1} is not ready for unprovisioning when Compose context context is ready - current state {2}",
            this->TraceId,
            ComposeUpgradeContext.DeploymentName,
            applicationContextToUnprovision_->Status);
    }

    // Since we are not using rollout manager for rolling out this application type unprovision,
    // the rollout status for this need not be set to DeletePending here.

    //
    // unprovision is async.
    //
    applicationContextToUnprovision_->UpdateIsAsync(true);
    return ErrorCodeValue::Success;
}

ErrorCode ProcessComposeDeploymentUpgradeAsyncOperation::RefreshApplicationUpgradeContext()
{
    if (!applicationUpgradeContext_)
    {
        applicationUpgradeContext_ = make_unique<ApplicationUpgradeContext>(this->ComposeUpgradeContext.ApplicationName);
    }

    auto storeTx = this->CreateTransaction();

    auto error = applicationUpgradeContext_->Refresh(storeTx);
    if (!error.IsSuccess())
    {
        WriteInfo(
            ComposeDeploymentUpgradeTraceComponent,
            "{0} unable to refresh Application upgrade context due to {1}",
            this->TraceId,
            error);
    }

    storeTx.CommitReadOnly();

    return error;
}

ErrorCode ProcessComposeDeploymentUpgradeAsyncOperation::UpdateContextStatusAfterUnprovision(StoreTransaction &storeTx)
{
    auto error = this->ComposeUpgradeContext.FinishUpgrading(storeTx);
    if (!error.IsSuccess())
    {
        return error;
    }

    StoreDataComposeDeploymentInstance storeData(
        this->ComposeUpgradeContext.DeploymentName,
        this->ComposeUpgradeContext.VersionToUnprovision);

    error = storeTx.DeleteIfFound(storeData);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto composeDeploymentContext = make_unique<ComposeDeploymentContext>(this->ComposeUpgradeContext.DeploymentName);
    error = this->Replica.GetComposeDeploymentContext(storeTx, *composeDeploymentContext);
    if (!error.IsSuccess())
    {
        return error;
    }

    return composeDeploymentContext->FinishCreating(storeTx, L"");
}
