// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "../ImageStore/IImageStore.h"
#include "../ImageStore/ImageStoreFactory.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Store;
using namespace Management::ImageModel;
using namespace Management::ImageStore;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ClusterBaselineAsyncOperation");

#if defined(PLATFORM_UNIX)
GlobalWString ClusterBaselineAsyncOperation::TargetPackageFilename = make_global<wstring>(L"ServiceFabric.baseline.deb");
GlobalWString ClusterBaselineAsyncOperation::PackageFilenamePattern = make_global<wstring>(L"*Fabric*.deb");
#else
GlobalWString ClusterBaselineAsyncOperation::TargetPackageFilename = make_global<wstring>(L"ServiceFabric.baseline.cab");
GlobalWString ClusterBaselineAsyncOperation::PackageFilenamePattern = make_global<wstring>(L"*Fabric*.cab");
#endif

GlobalWString ClusterBaselineAsyncOperation::TargetClusterManifestFilename = make_global<wstring>(L"ClusterManifest.baseline.xml");

ClusterBaselineAsyncOperation::ClusterBaselineAsyncOperation(
    __in RolloutManager & rolloutManager,
    Common::ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessRolloutContextAsyncOperationBase(
        rolloutManager,
        Store::ReplicaActivityId(rolloutManager.Replica.PartitionedReplicaId, activityId),
        timeout,
        callback, 
        root)
    , dataRoot_()
    , msiPath_()
    , clusterManifestPath_()
    , baselineVersion_()
    , upgradeContext_()
    , provisionContext_()
{
}

ErrorCode ClusterBaselineAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ClusterBaselineAsyncOperation>(operation)->Error;
}

void ClusterBaselineAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->StartBaselineIfNeeded(thisSPtr);
}

void ClusterBaselineAsyncOperation::StartBaselineIfNeeded(AsyncOperationSPtr const & thisSPtr)
{
    if (!ManagementConfig::GetConfig().EnableAutomaticBaseline)
    {
        WriteInfo(
            TraceComponent,
            "{0}: EnableAutomaticBaseline disabled",
            this->TraceId);

        this->TryComplete(thisSPtr, ErrorCodeValue::Success);

        return;
    }

    // Do an existing upgrade check upfront to avoid performing any
    // additional work in the typical case when a baseline has already 
    // been completed.
    //
    upgradeContext_ = make_unique<FabricUpgradeContext>();

    auto storeTx = this->CreateTransaction();
    auto error = storeTx.ReadExact(*upgradeContext_);

    bool doBaseline = false;

    if (error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: existing FabricUpgradeContext: {1}",
            this->TraceId,
            *upgradeContext_);

        doBaseline = ShouldDoBaseline(*upgradeContext_);
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0}: FabricUpgradeContext not found",
            this->TraceId);

        doBaseline = true;

        error = ErrorCodeValue::Success;
    }

    storeTx.CommitReadOnly();

    if (error.IsSuccess() && doBaseline)
    {
        this->InitializePaths(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void ClusterBaselineAsyncOperation::InitializePaths(AsyncOperationSPtr const & thisSPtr)
{
    auto error = FabricEnvironment::GetFabricDataRoot(dataRoot_);

    if (error.IsSuccess())
    {
        auto spec = FabricDeploymentSpecification(dataRoot_);

        clusterManifestPath_ = spec.GetCurrentClusterManifestFile(this->Replica.NodeName);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0}: GetFabricDataRoot failed: error={1}",
            this->TraceId,
            error);
    }

    if (error.IsSuccess())
    {
        error = this->InitializeMsiPath();
    }

    if (error.IsSuccess() && !msiPath_.empty())
    {
        this->UploadIfNeeded(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

ErrorCode ClusterBaselineAsyncOperation::InitializeMsiPath()
{
    auto pattern = Path::Combine(dataRoot_, PackageFilenamePattern);

    auto fileEnumerator = File::Search(wstring(pattern));

    auto error = fileEnumerator->MoveNext();

    if (error.IsSuccess())
    {
        msiPath_ = Path::Combine(dataRoot_, fileEnumerator->GetCurrentPath());
    }
    else if (error.IsWin32Error(ERROR_FILE_NOT_FOUND))
    {
        WriteInfo(
            TraceComponent,
            "{0}: Runtime Package not found: root={1} pattern={2}",
            this->TraceId,
            dataRoot_,
            pattern);

        msiPath_.clear();

        error = ErrorCodeValue::Success;
    }

    return error;
}

void ClusterBaselineAsyncOperation::UploadIfNeeded(AsyncOperationSPtr const & thisSPtr)
{
    auto imageStoreConnectionString = ManagementConfig::GetConfig().ImageStoreConnectionString;

    ImageStoreUPtr imageStore;
    auto error = ImageStoreFactory::CreateImageStore(
        imageStore, // out
        imageStoreConnectionString,
        ManagementConfig::GetConfig().ImageStoreMinimumTransferBPS,
        this->Client.Factory,
        true, // isInternal
        this->Replica.WorkingDirectory);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: CreateImageStore({1}) failed: error={2}",
            this->TraceId,
            imageStoreConnectionString,
            error);

        this->TryComplete(thisSPtr, error);

        return;
    }

    map<wstring, wstring> localRemoteMap;
    localRemoteMap.insert(make_pair(msiPath_, TargetPackageFilename));
    localRemoteMap.insert(make_pair(clusterManifestPath_, TargetClusterManifestFilename));

    vector<wstring> targetFiles;
    for (auto const & pair : localRemoteMap)
    {
        targetFiles.push_back(pair.second);
    }

    map<wstring, bool> existsResult;
    error = imageStore->DoesContentExist(targetFiles, this->RemainingTimeout, existsResult);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: DoesContentExist({1}) failed: error={2}",
            this->TraceId,
            targetFiles,
            error);

        this->TryComplete(thisSPtr, error);

        return;                                                      
    }

    vector<wstring> localFiles;
    vector<wstring> remoteFiles;

    for (auto const & pair : localRemoteMap)
    {
        auto findIter = existsResult.find(pair.second);

        if (findIter != existsResult.end() && findIter->second == true)
        {
            WriteInfo(
                TraceComponent,
                "{0}: {1} already uploaded",
                this->TraceId,
                pair.second);
        }
        else
        {
            localFiles.push_back(pair.first);
            remoteFiles.push_back(pair.second);
        }
    }

    if (!localFiles.empty())
    {
        error = imageStore->UploadContent(remoteFiles, localFiles, this->RemainingTimeout, CopyFlag::Overwrite);

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: uploaded {1} -> {2}",
                this->TraceId,
                localFiles,
                remoteFiles);
        }
        else
        {
            if (ShouldAbortBaseline(error))
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: UploadContent({1}, {2}) fatal failure: error={3}",
                    this->TraceId,
                    remoteFiles,
                    localFiles,
                    error);

                // Allow CM primary failover to complete without performing the
                // baseline. Retry will happen on subsequent failover if needed.
                //
                error = ErrorCodeValue::Success;
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: UploadContent({1}, {2}) failed: error={3}",
                    this->TraceId,
                    remoteFiles,
                    localFiles,
                    error);
            }


            this->TryComplete(thisSPtr, error);

            return;
        }
    }

    this->StartProvisionIfNeeded(thisSPtr);
}

void ClusterBaselineAsyncOperation::StartProvisionIfNeeded(AsyncOperationSPtr const & thisSPtr)
{
    auto error = this->Replica.GetFabricVersionInfo(
        TargetPackageFilename,
        TargetClusterManifestFilename,
        this->RemainingTimeout,
        baselineVersion_); // out

    if (error.IsSuccess())
    {
        bool doProvision = false;
        bool alreadyProvisioned = false;

        auto storeTx = this->CreateTransaction();

        provisionContext_ = make_unique<FabricProvisionContext>(
            this->Replica,
            this->ReplicaActivityId,
            TargetPackageFilename,
            TargetClusterManifestFilename,
            baselineVersion_);

        error = this->Replica.GetProvisionedFabricContext(
            storeTx,
            baselineVersion_,
            *provisionContext_,
            alreadyProvisioned); // out

        if (error.IsError(ErrorCodeValue::FabricVersionNotFound))
        {
            WriteInfo(
                TraceComponent,
                "{0}: FabricProvisionContext not found",
                this->TraceId);

            doProvision = true;
            alreadyProvisioned = false;

            error = storeTx.Insert(*provisionContext_);
        }
        else if (error.IsError(ErrorCodeValue::CMRequestAlreadyProcessing))
        {
            WriteInfo(
                TraceComponent,
                "{0}: pending FabricProvisionContext: {1}",
                this->TraceId,
                *provisionContext_);

            doProvision = false;
            alreadyProvisioned = false;

            // Allow existing provision operation to finish.
            // Unblock recovery and assume user will follow-up
            // with a manual baseline upgrade.
            //
            error = ErrorCodeValue::Success;
        }
        else if (error.IsSuccess() && !alreadyProvisioned)
        {
            WriteInfo(
                TraceComponent,
                "{0}: existing FabricProvisionContext: {1}",
                this->TraceId,
                *provisionContext_);

            doProvision = true;
            alreadyProvisioned = false;

            error = provisionContext_->StartProvisioning(
                storeTx,
                this->Replica,
                nullptr, // clientRequest,
                TargetPackageFilename,
                TargetClusterManifestFilename,
                baselineVersion_);
        }

        if (error.IsSuccess())
        {
            if (alreadyProvisioned)
            {
                this->StartUpgradeIfNeeded(thisSPtr);

                return;
            }
            else if (doProvision)
            {
                auto operation = StoreTransaction::BeginCommit(
                    move(storeTx),
                    *provisionContext_,
                    [this](AsyncOperationSPtr const & operation) { this->OnCommitProvisionComplete(operation, false); },
                    thisSPtr);
                this->OnCommitProvisionComplete(operation, true);

                return;
            }
        }
    }
    else if (ShouldAbortBaseline(error))
    {
        WriteWarning(
            TraceComponent,
            "{0}: GetFabricVersionInfo fatal failure: error={1}",
            this->TraceId,
            error);

        error = ErrorCodeValue::Success;
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0}: GetFabricVersionInfo failed: error={1}",
            this->TraceId,
            error);
    }

    this->TryComplete(thisSPtr, error);
}

void ClusterBaselineAsyncOperation::OnCommitProvisionComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = StoreTransaction::EndCommit(operation);

    if (error.IsSuccess())
    {
        auto provisionOperation = AsyncOperation::CreateAndStart<ProcessFabricProvisionContextAsyncOperation>(
            const_cast<RolloutManager &>(this->Rollout),
            *provisionContext_,
            this->RemainingTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnProvisionComplete(operation, false); },
            thisSPtr);
        this->OnProvisionComplete(provisionOperation, true);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void ClusterBaselineAsyncOperation::OnProvisionComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = ProcessFabricProvisionContextAsyncOperation::End(operation);

    WriteInfo(
        TraceComponent,
        "{0}: self-provision complete: version={1} error={2}",
        this->TraceId,
        baselineVersion_,
        error);

    if (error.IsSuccess())
    {
        this->StartUpgradeIfNeeded(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void ClusterBaselineAsyncOperation::StartUpgradeIfNeeded(AsyncOperationSPtr const & thisSPtr)
{
    ApplicationHealthPolicyMapSPtr applicationHealthPolicies;
    FabricUpgradeDescription upgradeDescription(
        baselineVersion_, // target version
        UpgradeType::Rolling,
        RollingUpgradeMode::UnmonitoredAuto,
        TimeSpan::MaxValue, // replicaSetCheckTimeout,
        RollingUpgradeMonitoringPolicy(),
        ClusterHealthPolicy(),
        false, // isHealthPolicyValid,
        false, // enableDeltaHealthEvaluation,
        ClusterUpgradeHealthPolicy(),
        false, // isUpgradeHealthPolicyValid
        applicationHealthPolicies);

    upgradeContext_ = make_unique<FabricUpgradeContext>(
        this->Replica,
        this->ReplicaActivityId,
        upgradeDescription,
        FabricVersion::Invalid, // currentVersion
        DateTime::Now().Ticks);
    upgradeContext_->MarkAsBaseline();

    auto storeTx = this->CreateTransaction();

    bool readExistingUpgradeContext = false;
    auto error = storeTx.TryReadOrInsertIfNotFound(*upgradeContext_, readExistingUpgradeContext);

    bool doUpgrade = false;

    if (readExistingUpgradeContext)
    {
        if (ShouldDoBaseline(*upgradeContext_))
        {
            doUpgrade = true;

            error = upgradeContext_->StartUpgrading(
                storeTx,
                this->Replica,
                nullptr, // clientRequest,
                upgradeDescription);
            upgradeContext_->MarkAsBaseline();
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: pending FabricUpgradeContext: {1}",
                this->TraceId,
                *upgradeContext_);
        }
    }
    else
    {
        doUpgrade = true;
    }
    
    if (error.IsSuccess() && doUpgrade)
    {
        auto operation = StoreTransaction::BeginCommit(
            move(storeTx),
            *upgradeContext_,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitUpgradeComplete(operation, false); },
            thisSPtr);
        this->OnCommitUpgradeComplete(operation, true);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void ClusterBaselineAsyncOperation::OnCommitUpgradeComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = StoreTransaction::EndCommit(operation);

    if (error.IsSuccess())
    {
        // The pending upgrade context has been persisted. On restart,
        // the CM will recover and start processing it.
        //
        auto msg = wformatString(
            "{0}: restarting CM to process self-baseline upgrade: {1}",
            this->TraceId,
            *upgradeContext_);

        WriteInfo(TraceComponent, "{0}", msg);

        this->Replica.TransientFault(msg);
    }

    this->TryComplete(thisSPtr, error);
}

bool ClusterBaselineAsyncOperation::ShouldDoBaseline(FabricUpgradeContext const & upgradeContext)
{
    // Only start the baseline if we do not find
    // an already active upgrade.
    //
    if (upgradeContext.IsComplete || upgradeContext.IsFailed)
    {
        return !upgradeContext.CurrentVersion.IsValid;
    }
    else
    {
        return false;
    }
}

bool ClusterBaselineAsyncOperation::ShouldAbortBaseline(ErrorCode const & error)
{
    switch (error.ReadValue())
    {
    case ErrorCodeValue::FileNotFound:
    case ErrorCodeValue::ImageBuilderValidationError:
    case ErrorCodeValue::ImageBuilderUnexpectedError:
    case ErrorCodeValue::ImageBuilderAccessDenied:
    case ErrorCodeValue::ImageBuilderInvalidMsiFile:
    case ErrorCodeValue::InvalidArgument:
    case ErrorCodeValue::InvalidState:
    case ErrorCodeValue::OperationFailed:
        return true;

    default:
        return false;
    }
}
