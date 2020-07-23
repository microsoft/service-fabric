// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <stdio.h>
#include "Hosting2/SetupConfig.h"
#include "Hosting2/HostingConfig.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace ServiceModel;
using namespace Transport;

using namespace Management::ImageModel;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ImageBuilderProxy");

#ifdef PLATFORM_UNIX
DWORD const ImageBuilderDisabledExitCode = ErrorCodeValue::ImageBuilderDisabled & 0x000000FF;
DWORD const ImageBuilderAbortedExitCode = 0x80070009; //sigkill
#else
DWORD const ImageBuilderDisabledExitCode = ErrorCodeValue::ImageBuilderDisabled & 0x0000FFFF;
DWORD const ImageBuilderAbortedExitCode = ErrorCodeValue::ImageBuilderAborted & 0x0000FFFF;
#endif


GlobalWString ImageBuilderProxy::UnicodeBOM = make_global<wstring>(L"\uFEFF");
GlobalWString ImageBuilderProxy::Newline = make_global<wstring>(L"\r\n");

GlobalWString ImageBuilderProxy::TempWorkingDirectoryPrefix = make_global<wstring>(L"IB");
GlobalWString ImageBuilderProxy::ArchiveDirectoryPrefix = make_global<wstring>(L"IB_Archive");
GlobalWString ImageBuilderProxy::ImageBuilderOutputDirectory = make_global<wstring>(L"ImageBuilderProxy");
GlobalWString ImageBuilderProxy::AppTypeInfoOutputDirectory = make_global<wstring>(L"AppTypeInfo");
GlobalWString ImageBuilderProxy::AppTypeOutputDirectory = make_global<wstring>(L"AppType");
GlobalWString ImageBuilderProxy::AppOutputDirectory = make_global<wstring>(L"App");
#if defined(PLATFORM_UNIX)
GlobalWString ImageBuilderProxy::ImageBuilderExeName = make_global<wstring>(L"ImageBuilder.sh");
#elif !DotNetCoreClrIOT
GlobalWString ImageBuilderProxy::ImageBuilderExeName = make_global<wstring>(L"ImageBuilder.exe");
#else
GlobalWString ImageBuilderProxy::ImageBuilderExeName = make_global<wstring>(L"ImageBuilder.bat");
#endif
GlobalWString ImageBuilderProxy::ApplicationTypeInfoOutputFilename = make_global<wstring>(L"ApplicationTypeInfo.txt");
GlobalWString ImageBuilderProxy::FabricUpgradeOutputDirectory = make_global<wstring>(L"FabricUpgrade");
GlobalWString ImageBuilderProxy::ClusterManifestOutputDirectory = make_global<wstring>(L"ClusterManifest");
GlobalWString ImageBuilderProxy::FabricOutputDirectory = make_global<wstring>(L"Fabric");
GlobalWString ImageBuilderProxy::FabricVersionOutputFilename = make_global<wstring>(L"VersionInfo.txt");
GlobalWString ImageBuilderProxy::FabricUpgradeResultFilename = make_global<wstring>(L"FabricUpgradeResult.txt");
GlobalWString ImageBuilderProxy::CleanupListFilename = make_global<wstring>(L"CleanupList.txt");
GlobalWString ImageBuilderProxy::AppParamFilename = make_global<wstring>(L"ApplicationParameters.txt");
GlobalWString ImageBuilderProxy::ErrorDetailsFilename = make_global<wstring>(L"ErrorDetails.txt");
GlobalWString ImageBuilderProxy::ProgressDetailsFilename = make_global<wstring>(L"Progress.txt");
GlobalWString ImageBuilderProxy::DockerComposeFilename = make_global<wstring>(L"Compose.yml");
GlobalWString ImageBuilderProxy::DockerComposeOverridesFilename = make_global<wstring>(L"Overrides.yml");
GlobalWString ImageBuilderProxy::OutputDockerComposeFilename = make_global<wstring>(L"Output.yml");

GlobalWString ImageBuilderProxy::SchemaPath = make_global<wstring>(L"/schemaPath");
GlobalWString ImageBuilderProxy::WorkingDir = make_global<wstring>(L"/workingDir");
GlobalWString ImageBuilderProxy::Operation = make_global<wstring>(L"/operation");
GlobalWString ImageBuilderProxy::StoreRoot = make_global<wstring>(L"/storeRoot");
GlobalWString ImageBuilderProxy::AppName = make_global<wstring>(L"/appName");
GlobalWString ImageBuilderProxy::AppTypeName = make_global<wstring>(L"/appTypeName");
GlobalWString ImageBuilderProxy::AppTypeVersion = make_global<wstring>(L"/appTypeVersion");
GlobalWString ImageBuilderProxy::CurrentTypeVersion = make_global<wstring>(L"/currentAppTypeVersion");
GlobalWString ImageBuilderProxy::TargetTypeVersion = make_global<wstring>(L"/targetAppTypeVersion");
GlobalWString ImageBuilderProxy::AppId = make_global<wstring>(L"/appId");
GlobalWString ImageBuilderProxy::NameUri = make_global<wstring>(L"/nameUri");
GlobalWString ImageBuilderProxy::AppParam = make_global<wstring>(L"/appParam");
GlobalWString ImageBuilderProxy::BuildPath = make_global<wstring>(L"/buildPath");
GlobalWString ImageBuilderProxy::DownloadPath = make_global<wstring>(L"/downloadPath");
GlobalWString ImageBuilderProxy::Output = make_global<wstring>(L"/output");
GlobalWString ImageBuilderProxy::Input = make_global<wstring>(L"/input");
GlobalWString ImageBuilderProxy::Progress = make_global<wstring>(L"/progress");
GlobalWString ImageBuilderProxy::ErrorDetails = make_global<wstring>(L"/errorDetails");
GlobalWString ImageBuilderProxy::CurrentAppInstanceVersion = make_global<wstring>(L"/currentAppInstanceVersion");
GlobalWString ImageBuilderProxy::FabricCodeFilepath = make_global<wstring>(L"/codePath");
GlobalWString ImageBuilderProxy::FabricConfigFilepath = make_global<wstring>(L"/configPath");
GlobalWString ImageBuilderProxy::FabricConfigVersion = make_global<wstring>(L"/configVersion");
GlobalWString ImageBuilderProxy::CurrentFabricVersion = make_global<wstring>(L"/currentFabricVersion");
GlobalWString ImageBuilderProxy::TargetFabricVersion = make_global<wstring>(L"/targetFabricVersion");
GlobalWString ImageBuilderProxy::InfrastructureManifestFile = make_global<wstring>(L"/im");
GlobalWString ImageBuilderProxy::DisableChecksumValidation = make_global<wstring>(L"/disableChecksumValidation");
GlobalWString ImageBuilderProxy::DisableServerSideCopy = make_global<wstring>(L"/disableServerSideCopy");
GlobalWString ImageBuilderProxy::Timeout = make_global<wstring>(L"/timeout");
GlobalWString ImageBuilderProxy::RegistryUserName = make_global<wstring>(L"/repoUserName");
GlobalWString ImageBuilderProxy::RegistryPassword = make_global<wstring>(L"/repoPwd");
GlobalWString ImageBuilderProxy::IsRegistryPasswordEncrypted = make_global<wstring>(L"/pwdEncrypted");
GlobalWString ImageBuilderProxy::ComposeFilePath = make_global<wstring>(L"/cf");
GlobalWString ImageBuilderProxy::OverrideFilePath = make_global<wstring>(L"/of");
GlobalWString ImageBuilderProxy::OutputComposeFilePath = make_global<wstring>(L"/ocf");
GlobalWString ImageBuilderProxy::CleanupComposeFiles = make_global<wstring>(L"/cleanup");
GlobalWString ImageBuilderProxy::GenerateDnsNames = make_global<wstring>(L"/generateDnsNames");
GlobalWString ImageBuilderProxy::SingleInstanceApplicationDescriptionString = make_global<wstring>(L"/singleInstanceApplicationDescription");
GlobalWString ImageBuilderProxy::UseOpenNetworkConfig = make_global<wstring>(L"/useOpenNetworkConfig");
GlobalWString ImageBuilderProxy::UseLocalNatNetworkConfig = make_global<wstring>(L"/useLocalNatNetworkConfig");
GlobalWString ImageBuilderProxy::DisableApplicationPackageCleanup = make_global<wstring>(L"/disableApplicationPackageCleanup");
GlobalWString ImageBuilderProxy::GenerationConfig = make_global<wstring>(L"/generationConfig");
GlobalWString ImageBuilderProxy::SFVolumeDiskServiceEnabled = make_global<wstring>(L"/sfVolumeDiskServiceEnabled");
GlobalWString ImageBuilderProxy::MountPointForSettings = make_global<wstring>(L"/mountPointForSettings");

GlobalWString ImageBuilderProxy::OperationBuildApplicationTypeInfo = make_global<wstring>(L"BuildApplicationTypeInfo");
GlobalWString ImageBuilderProxy::OperationDownloadAndBuildApplicationType = make_global<wstring>(L"DownloadAndBuildApplicationType");
GlobalWString ImageBuilderProxy::OperationBuildApplicationType = make_global<wstring>(L"BuildApplicationType");
GlobalWString ImageBuilderProxy::OperationBuildApplication = make_global<wstring>(L"BuildApplication");
GlobalWString ImageBuilderProxy::OperationUpgradeApplication = make_global<wstring>(L"UpgradeApplication");
GlobalWString ImageBuilderProxy::OperationDelete = make_global<wstring>(L"Delete");
GlobalWString ImageBuilderProxy::OperationGetFabricVersion = make_global<wstring>(L"GetFabricVersion");
GlobalWString ImageBuilderProxy::OperationProvisionFabric = make_global<wstring>(L"ProvisionFabric");
GlobalWString ImageBuilderProxy::OperationGetClusterManifest = make_global<wstring>(L"GetClusterManifest");
GlobalWString ImageBuilderProxy::OperationUpgradeFabric = make_global<wstring>(L"UpgradeFabric");
GlobalWString ImageBuilderProxy::OperationGetManifests = make_global<wstring>(L"GetManifests");
GlobalWString ImageBuilderProxy::OperationValidateComposeFile = make_global<wstring>(L"ValidateComposeDeployment");
GlobalWString ImageBuilderProxy::OperationBuildComposeDeploymentType = make_global<wstring>(L"BuildComposeDeployment");
GlobalWString ImageBuilderProxy::OperationBuildComposeApplicationTypeForUpgrade = make_global<wstring>(L"BuildComposeApplicationForUpgrade");
GlobalWString ImageBuilderProxy::OperationCleanupApplicationPackage = make_global<wstring>(L"CleanupApplicationPackage");
GlobalWString ImageBuilderProxy::OperationBuildSingleInstanceApplication = make_global<wstring>(L"BuildSingleInstanceApplication");
GlobalWString ImageBuilderProxy::OperationBuildSingleInstanceApplicationForUpgrade = make_global<wstring>(L"BuildSingleInstanceApplicationForUpgrade");

// 
// *** Async operations for JobQueue
//

class ImageBuilderProxy::ImageBuilderAsyncOperationBase : public AsyncOperation
{
private:
    typedef function<void(void)> JobQueueItemCompletionCallback;

public:
    ImageBuilderAsyncOperationBase(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent, true) // control our own completion on Cancel
        , owner_(owner)
        , traceId_(dynamic_cast<NodeTraceComponent&>(owner).TraceId)
        , activityId_(activityId)
        , timeoutHelper_(timeout)
        , jobCompletionCallback_(nullptr)
    {
    }
    
    __declspec(property(get=get_ImageBuilderProxy)) ImageBuilderProxy & Owner;
    ImageBuilderProxy & get_ImageBuilderProxy() { return owner_; }

    __declspec(property(get=get_TraceId)) wstring const & TraceId;
    wstring const & get_TraceId() const { return traceId_; }

    __declspec(property(get=get_ActivityId)) Common::ActivityId const & ActivityId;
    Common::ActivityId const & get_ActivityId() const { return activityId_; }

    __declspec(property(get=get_Timeout)) TimeSpan const Timeout;
    TimeSpan get_Timeout() const { return timeoutHelper_.GetRemainingTime(); }

    void OnStart(AsyncOperationSPtr const &) override
    {
        // Intentional no-op
    }

    virtual void OnProcessJob(AsyncOperationSPtr const &) = 0;

    void OnCompleted() override
    {
        if (jobCompletionCallback_)
        {
            jobCompletionCallback_();

            jobCompletionCallback_ = nullptr;
        }
    }

    void SetJobQueueItemCompletion(JobQueueItemCompletionCallback const & callback)
    {
        jobCompletionCallback_ = callback;
    }

private:

    ImageBuilderProxy & owner_;
    wstring traceId_;
    Common::ActivityId activityId_;
    TimeoutHelper timeoutHelper_;

    JobQueueItemCompletionCallback jobCompletionCallback_;
};

class ImageBuilderProxy::RunImageBuilderExeAsyncOperation : public ImageBuilderAsyncOperationBase
{
public:
    RunImageBuilderExeAsyncOperation(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        wstring const & operation,
        wstring const & outputDirectoryOrFile,
        wstring const & args,
        map<wstring, wstring> const & applicationParameters,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ImageBuilderAsyncOperationBase(
            owner,
            activityId,
            timeout,
            callback,
            parent)
        , operation_(operation)
        , outputDirectoryOrFile_(outputDirectoryOrFile)
        , args_(args)
        , applicationParameters_(applicationParameters)
        , tempWorkingDirectory_()
        , errorDetailsFile_()
        , hProcess_(0)
        , hThread_(0)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<RunImageBuilderExeAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        this->DoRunImageBuilderExe(thisSPtr);
    }

    void OnCancel() override
    {
        if (hProcess_ != 0)
        {
            this->Owner.Abort(hProcess_);
        }
    }

    void OnProcessJob(AsyncOperationSPtr const &) override
    {
        // no-op
    }

private:

    void DoRunImageBuilderExe(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->IsCancelRequested)
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::ImageBuilderAborted);
            return;
        }

        auto actualTimeout = this->Timeout;
        pid_t pid;
        auto error = this->Owner.TryStartImageBuilderProcess(
            operation_,
            outputDirectoryOrFile_,
            args_,
            applicationParameters_,
            actualTimeout, // inout
            tempWorkingDirectory_, // out
            errorDetailsFile_, // out
            hProcess_, // out
            hThread_, // out
            pid);

        if (error.IsSuccess())
        {
            processWait_ = ProcessWait::CreateAndStart(
                Handle(hProcess_, Handle::DUPLICATE()),
                pid,
                [this, thisSPtr] (pid_t, ErrorCode const & waitResult, DWORD exitCode)
                {
                    OnWaitOneComplete(thisSPtr, waitResult, exitCode);
                },
                actualTimeout);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void OnWaitOneComplete(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & waitResult,
        DWORD exitCode)
    {
        auto error = this->Owner.FinishImageBuilderProcess(
            waitResult,
            exitCode,
            tempWorkingDirectory_,
            errorDetailsFile_, 
            hProcess_,
            hThread_);

        this->TryComplete(thisSPtr, move(error));
    }

    wstring operation_;
    wstring outputDirectoryOrFile_;
    wstring args_;
    map<wstring, wstring> applicationParameters_;

    wstring tempWorkingDirectory_;
    wstring errorDetailsFile_;
    ProcessWaitSPtr processWait_;
    HANDLE hProcess_;
    HANDLE hThread_;
};

class ImageBuilderProxy::DeleteFilesAsyncOperation : public ImageBuilderAsyncOperationBase
{
public:
    DeleteFilesAsyncOperation(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        wstring const & inputFile,
        vector<wstring> const & filePaths,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ImageBuilderAsyncOperationBase(
            owner,
            activityId,
            timeout,
            callback,
            parent)
        , inputFile_(inputFile)
        , filepaths_(filePaths)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<DeleteFilesAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->DoDeleteFiles(thisSPtr);
    }

    void OnProcessJob(AsyncOperationSPtr const &) override
    {
        // no-op
    }

private:

    void DoDeleteFiles(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = this->Owner.WriteFilenameList(inputFile_, filepaths_);

        if (error.IsSuccess())
        {
            wstring cmdLineArgs;
            this->Owner.InitializeCommandLineArguments(cmdLineArgs);
            this->Owner.AddImageBuilderArgument(cmdLineArgs, this->Owner.Input, inputFile_);

            this->Owner.WriteInfo(
                TraceComponent, 
                "{0} removing files: {1}",
                this->ActivityId,
                filepaths_);

            auto operation = this->Owner.BeginRunImageBuilderExe(
                this->ActivityId,
                OperationDelete,
                L"",
                cmdLineArgs,
                map<wstring, wstring>(),
                this->Timeout,
                [this](AsyncOperationSPtr const & operation) { this->OnRunImageBuilderComplete(operation, false); },
                thisSPtr);
            this->OnRunImageBuilderComplete(operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void OnRunImageBuilderComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = this->Owner.EndRunImageBuilderExe(operation);

        this->TryComplete(thisSPtr, error);
    }

    wstring inputFile_;
    vector<wstring> filepaths_;
};

class ImageBuilderProxy::BuildApplicationTypeAsyncOperation : public ImageBuilderAsyncOperationBase
{
    DENY_COPY(BuildApplicationTypeAsyncOperation)
public:
    BuildApplicationTypeAsyncOperation(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        wstring const & buildPath,
        wstring const & downloadPath,
        ServiceModelTypeName const & typeName,
        ServiceModelVersion const & typeVersion,
        ProgressDetailsCallback const & progressDetailsCallback,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ImageBuilderAsyncOperationBase(
            owner,
            activityId,
            timeout,
            callback,
            parent)
        , buildPath_(buildPath)
        , downloadPath_(downloadPath)
        , typeName_(typeName)
        , typeVersion_(typeVersion)
        , progressDetailsCallback_(progressDetailsCallback)
        , progressDetailsCallbackLock_()
        , outputDirectory_()
        , serviceManifests_()
        , applicationManifestId_()
        , applicationManifestContent_()
        , healthPolicy_()
        , defaultParamList_()
        , progressTimer_()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
        __out wstring & applicationManifestId,
        __out wstring & applicationManifestContent,
        __out ApplicationHealthPolicy & healthPolicy,
        __out map<wstring, wstring> & defaultParamList)
    {
        auto casted = AsyncOperation::End<BuildApplicationTypeAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            serviceManifests = move(casted->serviceManifests_);
            applicationManifestId = move(casted->applicationManifestId_);
            applicationManifestContent = move(casted->applicationManifestContent_);
            healthPolicy = move(casted->healthPolicy_);
            defaultParamList = move(casted->defaultParamList_);
        }

        return casted->Error;
    }
    
    void OnCompleted() override
    {
        if (progressTimer_.get() != nullptr)
        {
            progressTimer_->Cancel();
        }

        {
            AcquireWriteLock lock(progressDetailsCallbackLock_);

            progressDetailsCallback_ = nullptr;
        }

        this->Owner.DeleteDirectory(outputDirectory_);

        __super::OnCompleted();
    }

    void OnProcessJob(AsyncOperationSPtr const & thisSPtr) override
    {
        this->DoBuildApplicationType(thisSPtr);
    }

private:

    void DoBuildApplicationType(AsyncOperationSPtr const & thisSPtr)
    {
        outputDirectory_ = this->Owner.GetOutputDirectoryPath(this->ActivityId);
        auto error = this->Owner.CreateOutputDirectory(outputDirectory_);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        if (!buildPath_.empty())
        {
            this->RunBuildApplicationTypeImageBuilder(thisSPtr);
        }
        else
        {
            this->RunDownloadAndBuildApplicationTypeImageBuilder(thisSPtr);
        }
    }
    
    void RunDownloadAndBuildApplicationTypeImageBuilder(AsyncOperationSPtr const & thisSPtr)
    {
        ASSERT_IF(downloadPath_.empty(), "{0}: RunDownloadAndBuildApplicationTypeImageBuilder: downloadPath should not be empty", this->ActivityId);

        this->Owner.WriteInfo(
            TraceComponent,
            "{0}+{1}: processing download and build application package Image Builder job for {2}, app type {3}+{4}",
            ImageBuilderAsyncOperationBase::TraceId,
            this->ActivityId,
            downloadPath_,
            typeName_,
            typeVersion_);

        wstring cmdLineArgs;
        this->Owner.InitializeCommandLineArguments(cmdLineArgs);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, DownloadPath, downloadPath_);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppTypeName, typeName_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppTypeVersion, typeVersion_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, Progress, ProgressDetailsFilename);

        if (ManagementConfig::GetConfig().DisableChecksumValidation)
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, DisableChecksumValidation, L"true");
        }

        auto operation = this->Owner.BeginRunImageBuilderExe(
            this->ActivityId,
            OperationDownloadAndBuildApplicationType,
            outputDirectory_,
            cmdLineArgs,
            map<wstring, wstring>(),
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRunImageBuilderComplete(operation, false); },
            thisSPtr);

        this->StartProgressDetailsTracking(thisSPtr);

        this->OnRunImageBuilderComplete(operation, true);
    }
        
    void RunBuildApplicationTypeImageBuilder(AsyncOperationSPtr const & thisSPtr)
    {
        this->Owner.WriteInfo(
            TraceComponent,
            "{0}+{1}: processing build application type Image Builder job for {2} ({3}:{4})",
            ImageBuilderAsyncOperationBase::TraceId,
            this->ActivityId,
            buildPath_,
            typeName_,
            typeVersion_);

        wstring cmdLineArgs;
        this->Owner.InitializeCommandLineArguments(cmdLineArgs);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, BuildPath, buildPath_);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, Progress, ProgressDetailsFilename);

        if (ManagementConfig::GetConfig().DisableChecksumValidation)
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, DisableChecksumValidation, L"true");
        }

        if (ManagementConfig::GetConfig().DisableServerSideCopy)
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, DisableServerSideCopy, L"true");
        }

        auto operation = this->Owner.BeginRunImageBuilderExe(
            this->ActivityId,
            OperationBuildApplicationType,
            outputDirectory_,
            cmdLineArgs,
            map<wstring, wstring>(),
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRunImageBuilderComplete(operation, false); },
            thisSPtr);

        this->StartProgressDetailsTracking(thisSPtr);

        this->OnRunImageBuilderComplete(operation, true);
    }

    void OnRunImageBuilderComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = this->Owner.EndRunImageBuilderExe(operation);

        if (error.IsSuccess())
        {
            BuildLayoutSpecification layout(outputDirectory_);

            ApplicationManifestDescription appManifest;
            auto applicationManifestPath = layout.GetApplicationManifestFile();
            error = this->Owner.ReadApplicationManifest(applicationManifestPath, appManifest);
            
            if (error.IsSuccess())
            {
                applicationManifestId_ = appManifest.ManifestId;
                error = this->Owner.ParseApplicationManifest(appManifest, layout, serviceManifests_);
            }

            if (error.IsSuccess())
            {
                healthPolicy_ = appManifest.Policies.HealthPolicy;
                defaultParamList_ = move(appManifest.ApplicationParameters);

                error = this->Owner.ReadApplicationManifestContent(applicationManifestPath, applicationManifestContent_);            
            }
        }

        this->TryComplete(thisSPtr, move(error));
    }

private:
    void StartProgressDetailsTracking(AsyncOperationSPtr const & thisSPtr)
    {
        AcquireReadLock lock(progressDetailsCallbackLock_);

        auto trackingInterval = ManagementConfig::GetConfig().ImageBuilderProgressTrackingInterval;
        if (progressDetailsCallback_ && trackingInterval >= TimeSpan::Zero)
        {
            progressTimer_ = Timer::Create(TraceComponent, [this, thisSPtr](TimerSPtr const &) { this->ReadProgressDetails(); }, false);
            progressTimer_->Change(TimeSpan::Zero);
        }
        else
        {
            this->Owner.WriteInfo(
                TraceComponent,
                "{0}+{1}: skip progress tracking: callback={2} interval={3}",
                ImageBuilderAsyncOperationBase::TraceId,
                this->ActivityId,
                progressDetailsCallback_ ? L"valid" : L"null",
                trackingInterval);
        }
    }

    void ReadProgressDetails()
    {
        auto trackingInterval = ManagementConfig::GetConfig().ImageBuilderProgressTrackingInterval;

        if (this->InternalIsCompleted || trackingInterval <= TimeSpan::Zero)
        {
            this->Owner.WriteInfo(
                TraceComponent,
                "{0}+{1}: cancel progress file tracking: completed={2} interval={3}",
                ImageBuilderAsyncOperationBase::TraceId,
                this->ActivityId,
                this->InternalIsCompleted,
                trackingInterval);

            return;
        }

        auto progressFilename = Path::Combine(outputDirectory_, ProgressDetailsFilename);

        if (File::Exists(progressFilename))
        {
            File progressFile;
            auto error = progressFile.TryOpen(
                progressFilename,
                FileMode::Open,
                FileAccess::Read,
                FileShare::Read,
#if defined(PLATFORM_UNIX)
                FileAttributes::Normal
#else
                FileAttributes::ReadOnly
#endif
            );

            if (error.IsSuccess())
            {
                wstring result;
                error = this->Owner.ReadFromFile(progressFile, result);

                if (error.IsSuccess())
                {
                    this->Owner.WriteInfo(
                        TraceComponent,
                        "{0}+{1}: progress details='{2}'",
                        ImageBuilderAsyncOperationBase::TraceId,
                        this->ActivityId,
                        result);

                    ProgressDetailsCallback callback;
                    {
                        AcquireReadLock lock(progressDetailsCallbackLock_);

                        callback = progressDetailsCallback_;
                    }

                    if (callback)
                    {
                        callback(result);
                    }
                }
                else
                {
                    this->Owner.WriteInfo(
                        TraceComponent,
                        "{0}+{1}: failed to read progress details from '{2}': error={3}",
                        ImageBuilderAsyncOperationBase::TraceId,
                        this->ActivityId,
                        progressFilename,
                        error);
                }

                auto innerError = progressFile.Close2();

                if (innerError.IsSuccess())
                {
                    innerError = File::Delete2(progressFilename);
                }

                if (!innerError.IsSuccess())
                {
                    this->Owner.WriteInfo(
                        TraceComponent,
                        "{0}+{1}: failed to close and delete '{2}': error={3}",
                        ImageBuilderAsyncOperationBase::TraceId,
                        this->ActivityId,
                        progressFilename,
                        innerError);
                }
            }
            else
            {
                this->Owner.WriteInfo(
                    TraceComponent,
                    "{0}+{1}: failed to open '{2}': error={3}",
                    ImageBuilderAsyncOperationBase::TraceId,
                    this->ActivityId,
                    progressFilename,
                    error);
            }
        }
        else
        {
            this->Owner.WriteNoise(
                TraceComponent,
                "{0}+{1}: progress file '{2}' not found",
                ImageBuilderAsyncOperationBase::TraceId,
                this->ActivityId,
                progressFilename);
        }

        progressTimer_->Change(trackingInterval);
    }

private:
    wstring buildPath_;
    wstring downloadPath_;
    ServiceModelTypeName typeName_;
    ServiceModelVersion typeVersion_;
    wstring applicationManifestId_;
    ProgressDetailsCallback progressDetailsCallback_;
    RwLock progressDetailsCallbackLock_;

    wstring outputDirectory_;
    vector<ServiceModelServiceManifestDescription> serviceManifests_;
    wstring applicationManifestContent_;
    ApplicationHealthPolicy healthPolicy_;
    map<wstring, wstring> defaultParamList_;

    TimerSPtr progressTimer_;
};

class ImageBuilderProxy::BuildComposeDeploymentAppTypeAsyncOperation : public ImageBuilderAsyncOperationBase
{
public:
    BuildComposeDeploymentAppTypeAsyncOperation(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        ByteBufferSPtr const &composeFile,
        ByteBufferSPtr const &overridesFile,
        wstring const &registryUserName,
        wstring const &registryPassword,
        bool isPasswordEncrypted,
        NamingUri const & appName,
        ServiceModelTypeName const & typeName,
        ServiceModelVersion const & typeVersion,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ImageBuilderAsyncOperationBase(
            owner,
            activityId,
            timeout,
            callback,
            parent)
        , composeFile_(composeFile)
        , overridesFile_(overridesFile)
        , appName_(appName)
        , typeName_(typeName)
        , typeVersion_(typeVersion)
        , outputDirectory_()
        , serviceManifests_()
        , applicationManifestId_()
        , applicationManifestContent_()
        , healthPolicy_()
        , defaultParamList_()
        , dockerComposeFileContent_()
        , registryUserName_(registryUserName)
        , registryPassword_(registryPassword)
        , isPasswordEncrypted_(isPasswordEncrypted)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
        __out wstring & applicationManifestId,
        __out wstring & applicationManifestContent,
        __out ApplicationHealthPolicy & healthPolicy,
        __out map<wstring, wstring> & defaultParamList,
        __out wstring & composeFileContent)
    {
        auto casted = AsyncOperation::End<BuildComposeDeploymentAppTypeAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            applicationManifestId = move(casted->applicationManifestId_);
            serviceManifests = move(casted->serviceManifests_);
            applicationManifestContent = move(casted->applicationManifestContent_);
            healthPolicy = move(casted->healthPolicy_);
            defaultParamList = move(casted->defaultParamList_);
            composeFileContent = move(casted->dockerComposeFileContent_);
        }

        return casted->Error;
    }

    void OnProcessJob(AsyncOperationSPtr const & thisSPtr) override
    {
        this->DoBuildApplicationType(thisSPtr);
    }

protected:

    void OnRunImageBuilderComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = this->Owner.EndRunImageBuilderExe(operation);

        if (error.IsSuccess())
        {
            BuildLayoutSpecification layout(outputDirectory_);

            ApplicationManifestDescription appManifest;
            auto applicationManifestPath = layout.GetApplicationManifestFile();
            error = this->Owner.ReadApplicationManifest(applicationManifestPath, appManifest);
            if (error.IsSuccess())
            {
                error = this->Owner.ParseApplicationManifest(appManifest, layout, serviceManifests_);
            }

            if (error.IsSuccess())
            {
                healthPolicy_ = appManifest.Policies.HealthPolicy;
                defaultParamList_ = move(appManifest.ApplicationParameters);
                applicationManifestId_ = move(appManifest.ManifestId);

                error = this->Owner.ReadApplicationManifestContent(applicationManifestPath, applicationManifestContent_);
            }

            if (error.IsSuccess())
            {
                auto outputComposeFileName = Path::Combine(outputDirectory_, OutputDockerComposeFilename);
                error = this->Owner.ReadImageBuilderOutput(outputComposeFileName, dockerComposeFileContent_, false);
            }
        }

        this->Owner.DeleteDirectory(outputDirectory_);

        this->TryComplete(operation->Parent, error);
    }

    wstring GetOutputDirectory() { return outputDirectory_; }

private:

    void DoBuildApplicationType(AsyncOperationSPtr const & thisSPtr)
    {
        this->Owner.WriteInfo(
            TraceComponent,
            "{0}+{1}: processing build compose application type Image Builder job for {2} ({3})",
            ImageBuilderAsyncOperationBase::TraceId,
            this->ActivityId,
            typeName_,
            typeVersion_);

        wstring cmdLineArgs;
        this->Owner.InitializeCommandLineArguments(cmdLineArgs);

        outputDirectory_ = Path::Combine(this->Owner.appTypeOutputBaseDirectory_, typeName_.Value);
        auto error = this->Owner.CreateOutputDirectory(outputDirectory_);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        // Create the compose and override files.
        auto composeFileName = Path::Combine(outputDirectory_, DockerComposeFilename);
        error = this->Owner.WriteToFile(composeFileName, *composeFile_);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        this->Owner.AddImageBuilderArgument(cmdLineArgs, ComposeFilePath, composeFileName);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, CleanupComposeFiles, L"true");

        if (overridesFile_ && overridesFile_->size() != 0)
        {
            auto overridesFileName = Path::Combine(outputDirectory_, DockerComposeOverridesFilename);
            error = this->Owner.WriteToFile(overridesFileName, *overridesFile_);
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }
            
            this->Owner.AddImageBuilderArgument(cmdLineArgs, OverrideFilePath, overridesFileName);
        }

        auto outputComposeFileName = Path::Combine(outputDirectory_, OutputDockerComposeFilename);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, OutputComposeFilePath, outputComposeFileName);

        if (!registryUserName_.empty())
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, RegistryUserName, registryUserName_);
            this->Owner.AddImageBuilderArgument(cmdLineArgs, RegistryPassword, registryPassword_);
            this->Owner.AddImageBuilderArgument(cmdLineArgs, IsRegistryPasswordEncrypted, isPasswordEncrypted_ ? L"true" : L"false");
        }

        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppName, appName_.ToString());
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppTypeName, typeName_.Value);

        if (ClusterManagerReplica::IsDnsServiceEnabled())
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, GenerateDnsNames, L"true");
        }

        if (ManagementConfig::GetConfig().DisableChecksumValidation)
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, DisableChecksumValidation, L"true");
        }

        //
        // Serverside copy helps in cases where code and data packages need not be downloaded and then re-uploaded to the store, rather directly
        // copied from incoming location to the store. For compose applications, nothing is present in the incoming location, so serverside copy
        // is not needed and there is no argument for that in IB as of now.
        //
        RunImageBuilderExe(thisSPtr, cmdLineArgs);
    }

    virtual void RunImageBuilderExe(AsyncOperationSPtr const &thisSPtr, wstring &cmdLineArgs)
    {
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppTypeVersion, typeVersion_.Value);

        auto operation = this->Owner.BeginRunImageBuilderExe(
            this->ActivityId,
            OperationBuildComposeDeploymentType,
            GetOutputDirectory(),
            cmdLineArgs,
            map<wstring, wstring>(),
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRunImageBuilderComplete(operation, false); },
            thisSPtr);

        this->OnRunImageBuilderComplete(operation, true);
    }

    ByteBufferSPtr composeFile_;
    ByteBufferSPtr overridesFile_;
    NamingUri appName_;
    ServiceModelTypeName typeName_;
    ServiceModelVersion typeVersion_;
    wstring registryUserName_;
    wstring registryPassword_;
    bool isPasswordEncrypted_;

    ProgressDetailsCallback progressDetailsCallback_;

    wstring outputDirectory_;
    vector<ServiceModelServiceManifestDescription> serviceManifests_;
    wstring applicationManifestId_;
    wstring applicationManifestContent_;
    wstring dockerComposeFileContent_;
    ApplicationHealthPolicy healthPolicy_;
    map<wstring, wstring> defaultParamList_;

};

class ImageBuilderProxy::BuildComposeApplicationTypeForUpgradeAsyncOperation : public BuildComposeDeploymentAppTypeAsyncOperation
{
public:
    BuildComposeApplicationTypeForUpgradeAsyncOperation(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        ByteBufferSPtr const &composeFile,
        ByteBufferSPtr const &overridesFile,
        wstring const &registryUserName,
        wstring const &registryPassword,
        bool isPasswordEncrypted,
        NamingUri const & appName,
        ServiceModelTypeName const & typeName,
        ServiceModelVersion const & currentTypeVersion,
        ServiceModelVersion const & targetTypeVersion,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : BuildComposeDeploymentAppTypeAsyncOperation(
            owner,
            activityId,
            composeFile,
            overridesFile,
            registryUserName,
            registryPassword,
            isPasswordEncrypted,
            appName,
            typeName,
            currentTypeVersion,
            timeout,
            callback,
            parent)
        , currentVersion_(currentTypeVersion)
        , targetVersion_(targetTypeVersion)
    {
    }

private:

    virtual void RunImageBuilderExe(AsyncOperationSPtr const &thisSPtr, wstring &cmdLineArgs)
    {
        this->Owner.AddImageBuilderArgument(cmdLineArgs, CurrentTypeVersion, currentVersion_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, TargetTypeVersion, targetVersion_.Value);

        auto operation = this->Owner.BeginRunImageBuilderExe(
            this->ActivityId,
            OperationBuildComposeApplicationTypeForUpgrade,
            GetOutputDirectory(),
            cmdLineArgs,
            map<wstring, wstring>(),
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRunImageBuilderComplete(operation, false); },
            thisSPtr);

        this->OnRunImageBuilderComplete(operation, true);
    }

    ServiceModelVersion targetVersion_;
    ServiceModelVersion currentVersion_;
};

class ImageBuilderProxy::BuildSingleInstanceApplicationAsyncOperation : public ImageBuilderAsyncOperationBase
{
public:
    BuildSingleInstanceApplicationAsyncOperation(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        ModelV2::ApplicationDescription const & applicationDescription,
        ServiceModelTypeName const &typeName,
        ServiceModelVersion const &typeVersion,
        Common::NamingUri const & appName,
        ServiceModelApplicationId const & appId,
        TimeSpan const& timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : ImageBuilderAsyncOperationBase(
            owner,
            activityId,
            timeout,
            callback,
            parent)
        , applicationDescription_(applicationDescription)
        , appName_(appName)
        , typeName_(typeName)
        , typeVersion_(typeVersion)
        , appId_(appId)
        , outputDirectory_()
        , buildPath_()
        , serviceManifests_()
        , applicationManifestContent_()
        , healthPolicy_()
        , defaultParamList_()
        , currentApplicationResult_()
        , layoutRoot_(owner.appOutputBaseDirectory_)
        , storeLayout_(layoutRoot_)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
        __out wstring & applicationManifestContent,
        __out ApplicationHealthPolicy & healthPolicy,
        __out map<wstring, wstring> & defaultParamList,
        __out DigestedApplicationDescription &digestedAppPackage)
    {
        auto casted = AsyncOperation::End<BuildSingleInstanceApplicationAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            serviceManifests = move(casted->serviceManifests_);
            applicationManifestContent = move(casted->applicationManifestContent_);
            healthPolicy = move(casted->healthPolicy_);
            defaultParamList = move(casted->defaultParamList_);
            digestedAppPackage = move(casted->currentApplicationResult_);
        }

        return casted->Error;
    }

    void OnProcessJob(AsyncOperationSPtr const & thisSPtr) override
    {
        this->DoBuildApplication(thisSPtr);
    }

protected:
    void OnRunImageBuilderComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = this->Owner.EndRunImageBuilderExe(operation);

        if (error.IsSuccess())
        {
            BuildLayoutSpecification layout(buildPath_);

            ApplicationManifestDescription appManifest;
            auto applicationManifestPath = layout.GetApplicationManifestFile();
            error = this->Owner.ReadApplicationManifest(applicationManifestPath, appManifest);
            if (error.IsSuccess())
            {
                error = this->Owner.ParseApplicationManifest(appManifest, layout, serviceManifests_);
            }

            if (error.IsSuccess())
            {
                healthPolicy_ = appManifest.Policies.HealthPolicy;
                defaultParamList_ = move(appManifest.ApplicationParameters);

                error = this->Owner.ReadApplicationManifestContent(applicationManifestPath, applicationManifestContent_);
            }
            
            if (error.IsSuccess())
            {
                wstring applicationVersionInstance = wformatString("{0}", Constants::InitialApplicationVersionInstance);
                auto appFile = storeLayout_.GetApplicationInstanceFile(typeName_.Value, appId_.Value, applicationVersionInstance);

                ApplicationInstanceDescription appDescription;
                error = this->Owner.ReadApplication(appFile, /*out*/ appDescription);
                if (error.IsSuccess())
                {
                    error = this->Owner.ParseApplication(
                        appName_, 
                        appDescription, 
                        storeLayout_, 
                        /*out*/ currentApplicationResult_);
                }
            }
        }

        this->Owner.DeleteDirectory(outputDirectory_);
        this->Owner.DeleteDirectory(buildPath_);

        this->TryComplete(operation->Parent, error);
    }

private:
    virtual void DoBuildApplication(AsyncOperationSPtr const & thisSPtr)
    {
        this->Owner.WriteInfo(
            TraceComponent,
            "{0}+{1}: processing build single instance application Image Builder job for application: {2} typeName: {3} typeVersion: {4}",
            ImageBuilderAsyncOperationBase::TraceId,
            this->ActivityId,
            applicationDescription_.ApplicationUri,
            typeName_,
            typeVersion_);

        wstring cmdLineArgs;
        this->Owner.InitializeCommandLineArguments(cmdLineArgs);

        // This is the build layout root.
        buildPath_ = Path::Combine(this->Owner.appTypeOutputBaseDirectory_, typeName_.Value);
        auto error = this->Owner.CreateOutputDirectory(buildPath_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        // This is CM's store layout root
        outputDirectory_ = storeLayout_.GetApplicationInstanceFolder(typeName_.Value, appId_.Value);
        error = this->Owner.CreateOutputDirectory(outputDirectory_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        ByteBufferUPtr applicationDescriptionBuffer;
        error = JsonHelper::Serialize(applicationDescription_, applicationDescriptionBuffer);
        if (!error.IsSuccess())
        {
            this->Owner.WriteInfo(
                TraceComponent,
                "{0}+{1} error serializing ApplicationDescription {2} : {3}",
                ImageBuilderAsyncOperationBase::TraceId,
                this->ActivityId,
                applicationDescription_.ApplicationUri,
                error);
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        wstring tempFilename = File::GetTempFileNameW(buildPath_);
        error = this->Owner.WriteToFile(tempFilename, *applicationDescriptionBuffer);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        this->Owner.AddImageBuilderArgument(
            cmdLineArgs,
            SingleInstanceApplicationDescriptionString,
            tempFilename);

        ByteBufferUPtr configBuffer;
        ClusterManager::ContainerGroupConfig config;
        error = JsonHelper::Serialize(config, configBuffer);
        if (!error.IsSuccess())
        {
            this->Owner.WriteInfo(
                TraceComponent,
                "{0}+{1} error serializing ContainerGroupConfig {2}",
                ImageBuilderAsyncOperationBase::TraceId,
                this->ActivityId,
                error);
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        tempFilename = File::GetTempFileNameW(buildPath_);
        error = this->Owner.WriteToFile(tempFilename, *configBuffer);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        this->Owner.AddImageBuilderArgument(cmdLineArgs, ImageBuilderProxy::GenerationConfig, tempFilename);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppName, appName_.ToString());
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppTypeName, typeName_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppTypeVersion, typeVersion_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppId, appId_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, BuildPath, buildPath_);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, MountPointForSettings, Hosting2::HostingConfig::GetConfig().ContainerMountPointForSettings);

        if (ClusterManagerReplica::IsDnsServiceEnabled())
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, GenerateDnsNames, L"true");
        }

        if (ManagementConfig::GetConfig().DisableChecksumValidation)
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, DisableChecksumValidation, L"true");
        }

        // For now, always pass UseOpenNetworkConfig parameter to true. 
        // Tracking bug : 14259273, to completely remove "UseOpenNetworkConfig" parameter support from Imagebuilder.
        // UseLocalNatNetworkConfig is used for determination instead.
        this->Owner.AddImageBuilderArgument(cmdLineArgs, UseOpenNetworkConfig, L"true");

        if (Hosting2::HostingConfig::GetConfig().LocalNatIpProviderEnabled)
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, UseLocalNatNetworkConfig, L"true");
        }

        //
        // Serverside copy helps in cases where code and data packages need not be downloaded and then re-uploaded to the store, rather directly
        // copied from incoming location to the store. 
        // For CGS applications, nothing is present in the incoming location, so serverside copy
        // is not needed and there is no argument for that in IB as of now.
        //
        RunImageBuilderExe(thisSPtr, cmdLineArgs);
    }

    virtual void RunImageBuilderExe(AsyncOperationSPtr const &thisSPtr, wstring &cmdLineArgs)
    {
        auto operation = this->Owner.BeginRunImageBuilderExe(
            this->ActivityId,
            OperationBuildSingleInstanceApplication,
            layoutRoot_,
            cmdLineArgs,
            map<wstring, wstring>(),
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRunImageBuilderComplete(operation, false); },
            thisSPtr);

        this->OnRunImageBuilderComplete(operation, true);
    }

protected:
    NamingUri appName_;
    ServiceModelTypeName typeName_;
    ServiceModelVersion typeVersion_;
    ModelV2::ApplicationDescription applicationDescription_;
    ProgressDetailsCallback progressDetailsCallback_;
    wstring layoutRoot_;
    StoreLayoutSpecification storeLayout_;
    ServiceModelApplicationId appId_;
    wstring buildPath_;
    wstring outputDirectory_;
    vector<ServiceModelServiceManifestDescription> serviceManifests_;
    wstring applicationManifestContent_;
    ApplicationHealthPolicy healthPolicy_;
    map<wstring, wstring> defaultParamList_;
    DigestedApplicationDescription currentApplicationResult_;
};

class ImageBuilderProxy::BuildSingleInstanceApplicationForUpgradeAsyncOperation : public BuildSingleInstanceApplicationAsyncOperation
{
public:
    BuildSingleInstanceApplicationForUpgradeAsyncOperation(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        ModelV2::ApplicationDescription const &description,
        ServiceModelTypeName const &typeName,
        ServiceModelVersion const & currentTypeVersion,
        ServiceModelVersion const & targetTypeVersion,
        Common::NamingUri const & appName,
        ServiceModelApplicationId const & appId,
        uint64 currentApplicationVersion,
        TimeSpan const& timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : BuildSingleInstanceApplicationAsyncOperation(
            owner,
            activityId,
            description,
            typeName,
            targetTypeVersion,
            appName,
            appId,
            timeout,
            callback,
            parent)
        , currentTypeVersion_(currentTypeVersion)
        , targetTypeVersion_(targetTypeVersion)
        , currentApplicationVersion_(currentApplicationVersion)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
        __out wstring & applicationManifestContent,
        __out ApplicationHealthPolicy & healthPolicy,
        __out map<wstring, wstring> & defaultParamList,
        __out DigestedApplicationDescription & currentApplicationDescription,
        __out DigestedApplicationDescription & targetApplicationDescription)
    {
        auto casted = AsyncOperation::End<BuildSingleInstanceApplicationForUpgradeAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            serviceManifests = move(casted->serviceManifests_);
            applicationManifestContent = move(casted->applicationManifestContent_);
            healthPolicy = move(casted->healthPolicy_);
            defaultParamList = move(casted->defaultParamList_);
            currentApplicationDescription = move(casted->currentApplicationResult_);
            targetApplicationDescription = move(casted->targetApplicationResult_);
        }

        return casted->Error;
    }

private:
    void DoBuildApplication(AsyncOperationSPtr const & thisSPtr) override
    {
        this->Owner.WriteInfo(
            TraceComponent,
            "{0}+{1}: processing build single instance application for upgrade Image Builder job for application: {2} typeName: {3} currentTypeVersion: {4} targetTypeVersion: {5}",
            ImageBuilderAsyncOperationBase::TraceId,
            this->ActivityId,
            applicationDescription_.ApplicationUri,
            typeName_,
            currentTypeVersion_,
            targetTypeVersion_);

        wstring cmdLineArgs;
        this->Owner.InitializeCommandLineArguments(cmdLineArgs);

        // This is the build layout root.
        buildPath_ = Path::Combine(this->Owner.appTypeOutputBaseDirectory_, typeName_.Value);
        auto error = this->Owner.CreateOutputDirectory(buildPath_);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        // This is CM's store layout root
        outputDirectory_ = storeLayout_.GetApplicationInstanceFolder(typeName_.Value, appId_.Value);
        error = this->Owner.CreateOutputDirectory(outputDirectory_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        ByteBufferUPtr descriptionBuffer;
        error = JsonHelper::Serialize(applicationDescription_, descriptionBuffer);
        if (!error.IsSuccess())
        {
            this->Owner.WriteInfo(
                TraceComponent,
                "{0}+{1} error serializing applicationdescription {2} : {3}",
                ImageBuilderAsyncOperationBase::TraceId,
                this->ActivityId,
                applicationDescription_.ApplicationUri,
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        wstring tempFilename = File::GetTempFileNameW(buildPath_);
        error = this->Owner.WriteToFile(tempFilename, *descriptionBuffer);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        this->Owner.AddImageBuilderArgument(
            cmdLineArgs,
            SingleInstanceApplicationDescriptionString,
            tempFilename);

        ByteBufferUPtr configBuffer;
        ClusterManager::ContainerGroupConfig config;
        error = JsonHelper::Serialize(config, configBuffer);
        if (!error.IsSuccess())
        {
            this->Owner.WriteInfo(
                TraceComponent,
                "{0}+{1} error serializing ContainerGroupConfig {2}",
                ImageBuilderAsyncOperationBase::TraceId,
                this->ActivityId,
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        tempFilename = File::GetTempFileNameW(buildPath_);
        error = this->Owner.WriteToFile(tempFilename, *configBuffer);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        this->Owner.AddImageBuilderArgument(cmdLineArgs, ImageBuilderProxy::GenerationConfig, tempFilename);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppName, appName_.ToString());
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppTypeName, typeName_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, CurrentTypeVersion, currentTypeVersion_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, TargetTypeVersion, targetTypeVersion_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppId, appId_.Value);

        wstring currentAppVersionInstance = wformatString("{0}", currentApplicationVersion_);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, CurrentAppInstanceVersion, currentAppVersionInstance);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, BuildPath, buildPath_);

        this->Owner.AddImageBuilderArgument(cmdLineArgs, MountPointForSettings, Hosting2::HostingConfig::GetConfig().ContainerMountPointForSettings);

        if (ClusterManagerReplica::IsDnsServiceEnabled())
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, GenerateDnsNames, L"true");
        }

        if (ManagementConfig::GetConfig().DisableChecksumValidation)
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, DisableChecksumValidation, L"true");
        }

        if (Hosting2::HostingConfig::GetConfig().IPProviderEnabled &&
            Hosting2::SetupConfig::GetConfig().ContainerNetworkSetup)
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, UseOpenNetworkConfig, L"true");
        }

        if (Hosting2::HostingConfig::GetConfig().LocalNatIpProviderEnabled)
        {
            this->Owner.AddImageBuilderArgument(cmdLineArgs, UseLocalNatNetworkConfig, L"true");
        }
        
        //
        // Serverside copy helps in cases where code and data packages need not be downloaded and then re-uploaded to the store, rather directly
        // copied from incoming location to the store. 
        //
        RunImageBuilderExe(thisSPtr, cmdLineArgs);
    }

    void RunImageBuilderExe(AsyncOperationSPtr const &thisSPtr, wstring &cmdLineArgs) override
    {
        auto operation = this->Owner.BeginRunImageBuilderExe(
            this->ActivityId,
            OperationBuildSingleInstanceApplicationForUpgrade,
            layoutRoot_,
            cmdLineArgs,
            map<wstring, wstring>(),
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRunImageBuilderComplete(operation, false); },
            thisSPtr);

        this->OnRunImageBuilderComplete(operation, true);
    }

    void OnRunImageBuilderComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = this->Owner.EndRunImageBuilderExe(operation);

        if (error.IsSuccess())
        {
            BuildLayoutSpecification layout(buildPath_);

            ApplicationManifestDescription appManifest;
            auto applicationManifestPath = layout.GetApplicationManifestFile();
            error = this->Owner.ReadApplicationManifest(applicationManifestPath, appManifest);
            if (error.IsSuccess())
            {
                error = this->Owner.ParseApplicationManifest(appManifest, layout, serviceManifests_);
            }

            if (error.IsSuccess())
            {
                healthPolicy_ = appManifest.Policies.HealthPolicy;
                defaultParamList_ = move(appManifest.ApplicationParameters);

                error = this->Owner.ReadApplicationManifestContent(applicationManifestPath, applicationManifestContent_);
            }

            wstring currentAppVersionInstance = wformatString("{0}", currentApplicationVersion_);
            wstring targetAppVersionInstance = wformatString("{0}", currentApplicationVersion_ + 1);
            if (error.IsSuccess())
            {
                auto appFile = storeLayout_.GetApplicationInstanceFile(typeName_.Value, appId_.Value, currentAppVersionInstance);

                ApplicationInstanceDescription appDescription;
                error = this->Owner.ReadApplication(appFile, /*out*/ appDescription);
                if (error.IsSuccess())
                {
                    error = this->Owner.ParseApplication(
                        appName_,
                        appDescription,
                        storeLayout_,
                        /*out*/ currentApplicationResult_);
                }
            }

            if (error.IsSuccess())
            {
                auto appFile = storeLayout_.GetApplicationInstanceFile(typeName_.Value, appId_.Value, targetAppVersionInstance);

                ApplicationInstanceDescription appDescription;
                error = this->Owner.ReadApplication(appFile, /*out*/ appDescription);
                if (error.IsSuccess())
                {
                    error = this->Owner.ParseApplication(
                        appName_,
                        appDescription,
                        storeLayout_,
                        /*out*/ targetApplicationResult_);
                }
            }
        }

        this->Owner.DeleteDirectory(outputDirectory_);
        this->Owner.DeleteDirectory(buildPath_);

        this->TryComplete(operation->Parent, error);
    }

    ServiceModelVersion currentTypeVersion_;
    ServiceModelVersion targetTypeVersion_;
    uint64 currentApplicationVersion_;

    DigestedApplicationDescription targetApplicationResult_;
};

class ImageBuilderProxy::BuildApplicationAsyncOperation : public ImageBuilderAsyncOperationBase
{
public:
    BuildApplicationAsyncOperation(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        NamingUri const & appName,
        ServiceModelApplicationId const & appId,
        ServiceModelTypeName const & appTypeName,
        ServiceModelVersion const & targetAppTypeVersion,
        map<wstring, wstring> const & applicationParameters,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ImageBuilderAsyncOperationBase(
            owner,
            activityId,
            timeout,
            callback,
            parent)
        , appName_(appName)
        , appId_(appId)
        , appTypeName_(appTypeName)
        , targetAppTypeVersion_(targetAppTypeVersion)
        , applicationParameters_(applicationParameters)
        , layoutRoot_(owner.appOutputBaseDirectory_)
        , layout_(layoutRoot_)
        , outputDirectory_()
        , appFile_()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out DigestedApplicationDescription & currentApplicationResult)
    {
        auto casted = AsyncOperation::End<BuildApplicationAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            currentApplicationResult = casted->currentApplicationResult_;
        }

        return casted->Error;
    }

    void OnProcessJob(AsyncOperationSPtr const & thisSPtr) override
    {
        this->DoBuildApplication(thisSPtr);
    }

private:
    
    void DoBuildApplication(AsyncOperationSPtr const & thisSPtr)
    {
        this->Owner.WriteInfo(
            TraceComponent, 
            "{0}+{1}: processing build application Image Builder job for {2} ({3})",
            ImageBuilderAsyncOperationBase::TraceId,
            this->ActivityId,
            appName_,
            appId_);

        wstring applicationVersionInstance = wformatString("{0}", Constants::InitialApplicationVersionInstance);

        outputDirectory_ = layout_.GetApplicationInstanceFolder(appTypeName_.Value, appId_.Value);
        appFile_ = layout_.GetApplicationInstanceFile(appTypeName_.Value, appId_.Value, applicationVersionInstance);

        wstring cmdLineArgs;
        this->Owner.InitializeCommandLineArguments(cmdLineArgs);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, NameUri, appName_.ToString());
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppId, appId_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppTypeName, appTypeName_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppTypeVersion, targetAppTypeVersion_.Value);

        auto error = this->Owner.CreateOutputDirectory(outputDirectory_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);

            return;
        }
      
        auto operation = this->Owner.BeginRunImageBuilderExe(
            this->ActivityId,
            OperationBuildApplication,
            layoutRoot_, // ImageBuilder internally appends app instance path
            cmdLineArgs,
            applicationParameters_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRunImageBuilderComplete(operation, false); },
            thisSPtr);
        this->OnRunImageBuilderComplete(operation, true);
    }

    void OnRunImageBuilderComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = this->Owner.EndRunImageBuilderExe(operation);

        if (error.IsSuccess())
        {
            ApplicationInstanceDescription appDescription;
            error = this->Owner.ReadApplication(appFile_, /*out*/ appDescription);

            if (error.IsSuccess())
            {
                error = this->Owner.ParseApplication(
                    appName_, 
                    appDescription, 
                    layout_, 
                    /*out*/ currentApplicationResult_);
            }
        }

        this->Owner.DeleteDirectory(outputDirectory_);

        this->TryComplete(thisSPtr, error);
    }

    NamingUri appName_;
    ServiceModelApplicationId appId_;
    ServiceModelTypeName appTypeName_;
    ServiceModelVersion targetAppTypeVersion_;
    map<wstring, wstring> applicationParameters_;
    DigestedApplicationDescription currentApplicationResult_;

    wstring layoutRoot_;
    StoreLayoutSpecification layout_;
    wstring outputDirectory_;
    wstring appFile_;
};

class ImageBuilderProxy::UpgradeApplicationAsyncOperation : public ImageBuilderAsyncOperationBase
{
public:
    UpgradeApplicationAsyncOperation(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        NamingUri const & appName,
        ServiceModelApplicationId const & appId,
        ServiceModelTypeName const & appTypeName,
        ServiceModelVersion const & targetAppTypeVersion,
        uint64 currentApplicationVersion,
        map<wstring, wstring> const & applicationParameters,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ImageBuilderAsyncOperationBase(
            owner,
            activityId,
            timeout,
            callback,
            parent)
        , appName_(appName)
        , appId_(appId)
        , appTypeName_(appTypeName)
        , targetAppTypeVersion_(targetAppTypeVersion)
        , currentApplicationVersion_(currentApplicationVersion)
        , applicationParameters_(applicationParameters)
        , layoutRoot_(owner.appOutputBaseDirectory_)
        , layout_(layoutRoot_)
        , outputDirectory_()
        , currentAppFile_()
        , targetAppFile_()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out DigestedApplicationDescription & currentApplicationResult,
        __out DigestedApplicationDescription & targetApplicationResult)
    {
        auto casted = AsyncOperation::End<UpgradeApplicationAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            currentApplicationResult = casted->currentApplicationResult_;
            targetApplicationResult = casted->targetApplicationResult_;
        }

        return casted->Error;
    }

    void OnProcessJob(AsyncOperationSPtr const & thisSPtr) override
    {
        this->DoUpgradeApplication(thisSPtr);
    }

private:
    
    void DoUpgradeApplication(AsyncOperationSPtr const & thisSPtr)
    {
        this->Owner.WriteInfo(
            TraceComponent, 
            "{0}+{1}: processing upgrade application Image Builder job for {2} ({3})",
            ImageBuilderAsyncOperationBase::TraceId,
            this->ActivityId,
            appName_,
            appId_);

        wstring currentAppVersionInstance = wformatString("{0}", currentApplicationVersion_);

        wstring targetAppVersionInstance = wformatString("{0}", currentApplicationVersion_ + 1);

        outputDirectory_ = layout_.GetApplicationInstanceFolder(appTypeName_.Value, appId_.Value);
        currentAppFile_ = layout_.GetApplicationInstanceFile(appTypeName_.Value, appId_.Value, currentAppVersionInstance);
        targetAppFile_ = layout_.GetApplicationInstanceFile(appTypeName_.Value, appId_.Value, targetAppVersionInstance);

        wstring cmdLineArgs;
        this->Owner.InitializeCommandLineArguments(cmdLineArgs);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppId, appId_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppTypeName, appTypeName_.Value);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, CurrentAppInstanceVersion, currentAppVersionInstance);
        this->Owner.AddImageBuilderArgument(cmdLineArgs, AppTypeVersion, targetAppTypeVersion_.Value);

        auto error = this->Owner.CreateOutputDirectory(outputDirectory_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);

            return;
        }
        
        auto operation = this->Owner.BeginRunImageBuilderExe(
            this->ActivityId,
            OperationUpgradeApplication,
            layoutRoot_, // ImageBuilder internally appends app instance path
            cmdLineArgs,
            applicationParameters_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRunImageBuilderComplete(operation, false); },
            thisSPtr);
        this->OnRunImageBuilderComplete(operation, true);
    }

    void OnRunImageBuilderComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = this->Owner.EndRunImageBuilderExe(operation);
        
        DigestedApplicationDescription currentApplication;
        if (error.IsSuccess())
        {
            ApplicationInstanceDescription appDescription;
            error = this->Owner.ReadApplication(currentAppFile_, /*out*/ appDescription);

            if (error.IsSuccess())
            {
                error = this->Owner.ParseApplication(
                    appName_, 
                    appDescription, 
                    layout_, 
                    /*out*/ currentApplication);
            }
        }

        DigestedApplicationDescription targetApplication;
        if (error.IsSuccess())
        {
            ApplicationInstanceDescription appDescription;
            error = this->Owner.ReadApplication(targetAppFile_, /*out*/ appDescription);

            if (error.IsSuccess())
            {
                error = this->Owner.ParseApplication(
                    appName_, 
                    appDescription, 
                    layout_, 
                    /*out*/ targetApplication);
            }
        }

        if (error.IsSuccess())
        {
            currentApplicationResult_ = currentApplication;
            targetApplicationResult_ = targetApplication;
        }

        this->Owner.DeleteDirectory(outputDirectory_);

        this->TryComplete(thisSPtr, error);
    }

    NamingUri appName_;
    ServiceModelApplicationId appId_;
    ServiceModelTypeName appTypeName_;
    ServiceModelVersion targetAppTypeVersion_;
    uint64 currentApplicationVersion_;
    map<wstring, wstring> applicationParameters_;
    DigestedApplicationDescription currentApplicationResult_;
    DigestedApplicationDescription targetApplicationResult_;

    wstring layoutRoot_;
    StoreLayoutSpecification layout_;
    wstring outputDirectory_;
    wstring currentAppFile_;
    wstring targetAppFile_;
};

class ImageBuilderProxy::CleanupApplicationAsyncOperation : public ImageBuilderAsyncOperationBase
{
public:
    CleanupApplicationAsyncOperation(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        ServiceModelTypeName const & appTypeName,
        ServiceModelApplicationId const & appId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ImageBuilderAsyncOperationBase(
            owner,
            activityId,
            timeout,
            callback,
            parent)
        , appTypeName_(appTypeName)
        , appId_(appId)
        , outputDirectory_()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CleanupApplicationAsyncOperation>(operation)->Error;
    }

    void OnProcessJob(AsyncOperationSPtr const & thisSPtr) override
    {
        this->DoCleanupApplication(thisSPtr);
    }

private:
    
    void DoCleanupApplication(AsyncOperationSPtr const & thisSPtr)
    {
        this->Owner.WriteInfo(
            TraceComponent, 
            "{0}+{1}: processing cleanup application Image Builder job for {2} ({3})",
            ImageBuilderAsyncOperationBase::TraceId,
            this->ActivityId,
            appTypeName_,   
            appId_);

        if (appId_.Value.empty())
        {
            auto msg = wformatString(
                "{0} invalid appId='{1}'",
                ImageBuilderAsyncOperationBase::TraceId,
                appId_);

            this->Owner.WriteError(TraceComponent, "{0}", msg);

            Assert::TestAssert("{0}", msg);

            // Best effort cleanup. Do not fail the delete and risk
            // getting stuck in the delete workflow.
            //
            this->TryComplete(thisSPtr, ErrorCodeValue::Success);

            return;
        }

        outputDirectory_ = Path::Combine(this->Owner.appOutputBaseDirectory_, appId_.Value);
        wstring inputFile = Path::Combine(outputDirectory_, CleanupListFilename);

        auto error = this->Owner.CreateOutputDirectory(outputDirectory_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);

            return;
        }

        // want paths to be relative from ImageStoreRoot
        StoreLayoutSpecification layout(L"");
        vector<wstring> cleanupFilenames;

        cleanupFilenames.push_back(layout.GetApplicationInstanceFolder(appTypeName_.Value, appId_.Value));

        auto operation = this->Owner.BeginDeleteFiles(
            this->ActivityId,
            inputFile, 
            cleanupFilenames, 
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnDeleteFilesComplete(operation, false); },
            thisSPtr);
        this->OnDeleteFilesComplete(operation, true);
    }

    void OnDeleteFilesComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = this->Owner.EndDeleteFiles(operation);

        this->Owner.DeleteDirectory(outputDirectory_);

        this->TryComplete(thisSPtr, error);
    }

    ServiceModelTypeName appTypeName_;
    ServiceModelApplicationId appId_;

    wstring outputDirectory_;
};

class ImageBuilderProxy::CleanupApplicationInstanceAsyncOperation : public ImageBuilderAsyncOperationBase
{
public:
    CleanupApplicationInstanceAsyncOperation(
        __in ImageBuilderProxy & owner,
        Common::ActivityId const & activityId,
        ApplicationInstanceDescription const & appInstanceDescription,
        bool deleteApplicationPackage,
        vector<ServicePackageReference> const & servicePackageReferences,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ImageBuilderAsyncOperationBase(
            owner,
            activityId,
            timeout,
            callback,
            parent)
        , appInstanceDescription_(appInstanceDescription)
        , deleteApplicationPackage_(deleteApplicationPackage)
        , servicePackageReferences_(servicePackageReferences)
        , outputDirectory_()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CleanupApplicationInstanceAsyncOperation>(operation)->Error;
    }

    void OnProcessJob(AsyncOperationSPtr const & thisSPtr) override
    {
        this->DoCleanupApplicationInstance(thisSPtr);
    }

private:
    
    void DoCleanupApplicationInstance(AsyncOperationSPtr const & thisSPtr)
    {
        this->Owner.WriteInfo(
            TraceComponent, 
            "{0}+{1}: processing cleanup application instance Image Builder job for ({2})",
            ImageBuilderAsyncOperationBase::TraceId,
            this->ActivityId,
            appInstanceDescription_.ApplicationId);

        if (appInstanceDescription_.ApplicationId.empty())
        {
            auto msg = wformatString(
                "{0} invalid application instance description: appId='{1}'",
                ImageBuilderAsyncOperationBase::TraceId,
                appInstanceDescription_.ApplicationId);

            this->Owner.WriteError(TraceComponent, "{0}", msg);

            Assert::TestAssert("{0}", msg);

            // Preserve original validation error
            //
            this->TryComplete(thisSPtr, ErrorCodeValue::Success);

            return;
        }

        outputDirectory_ = Path::Combine(this->Owner.appOutputBaseDirectory_, appInstanceDescription_.ApplicationId);
        wstring inputFile = Path::Combine(outputDirectory_, CleanupListFilename);

        auto error = this->Owner.CreateOutputDirectory(outputDirectory_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);

            return;
        }

        // want paths to be relative from ImageStoreRoot
        StoreLayoutSpecification layout(L"");
        vector<wstring> cleanupFilenames;

        for (auto const & pkg : servicePackageReferences_)
        {
            cleanupFilenames.push_back(layout.GetServicePackageFile(
                appInstanceDescription_.ApplicationTypeName,
                appInstanceDescription_.ApplicationId,
                pkg.Name,
                pkg.RolloutVersionValue.ToString()));
        }

        if (deleteApplicationPackage_)
        {
            cleanupFilenames.push_back(layout.GetApplicationPackageFile(
                    appInstanceDescription_.ApplicationTypeName,
                    appInstanceDescription_.ApplicationId,
                    appInstanceDescription_.ApplicationPackageReference.RolloutVersionValue.ToString()));
        }

        cleanupFilenames.push_back(layout.GetApplicationInstanceFile(
            appInstanceDescription_.ApplicationTypeName,
            appInstanceDescription_.ApplicationId,
            wformatString(appInstanceDescription_.Version)));

        auto operation = this->Owner.BeginDeleteFiles(
            this->ActivityId,
            inputFile, 
            cleanupFilenames, 
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnDeleteFilesComplete(operation, false); },
            thisSPtr);
        this->OnDeleteFilesComplete(operation, true);
    }

    void OnDeleteFilesComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = this->Owner.EndDeleteFiles(operation);

        this->Owner.DeleteDirectory(outputDirectory_);

        this->TryComplete(thisSPtr, error);
    }

    ApplicationInstanceDescription appInstanceDescription_;
    bool deleteApplicationPackage_;
    vector<ServicePackageReference> servicePackageReferences_;

    wstring outputDirectory_;
};

//
// *** JobQueue
//

class ImageBuilderProxy::ApplicationJobItem
{
    DEFAULT_COPY_ASSIGNMENT(ApplicationJobItem)

public:
    ApplicationJobItem()
        : operation_()
    {
    }

    ApplicationJobItem(ApplicationJobItem && other)
        : operation_(move(other.operation_))
    {
    }

    ApplicationJobItem(shared_ptr<ImageBuilderAsyncOperationBase> && operation)
        : operation_(move(operation))
    {
    }

    bool ProcessJob(ComponentRoot &)
    {
        auto delay = ManagementConfig::GetConfig().ImageBuilderJobQueueDelay;
        if (delay > TimeSpan::Zero)
        {
            operation_->Owner.WriteInfo(TraceComponent, "ClusterManager/ImageBuilderJobQueueDelay config set: delay={0}", delay);

            Sleep(static_cast<DWORD>(delay.TotalPositiveMilliseconds()));
        }

        operation_->OnProcessJob(operation_);

        return false;
    }

private:
    shared_ptr<ImageBuilderProxy::ImageBuilderAsyncOperationBase> operation_;
};

class ImageBuilderProxy::ApplicationJobQueueBase : public JobQueue<ApplicationJobItem, ComponentRoot>
{
public:
    ApplicationJobQueueBase(
        wstring const & name,
        __in ComponentRoot & root)
        : JobQueue(
            name, 
            root, 
            false, // forceEnqueue
            0) // maxThreads
        , configHandlerId_(0)
    {
        this->SetAsyncJobs(true);
    }

    virtual void TryEnqueueOrFail(__in ImageBuilderProxy & owner, AsyncOperationSPtr const & operation)
    {
        // ImageBuilderAsyncOperationBase::OnStart() is a no-op. No work is performed
        // (AsyncOperation can't complete) until it's processed by the job queue.
        //
        operation->Start(operation);

        auto casted = dynamic_pointer_cast<ImageBuilderAsyncOperationBase>(operation);

        if (casted)
        {
            auto selfRoot = operation->Parent;

            casted->SetJobQueueItemCompletion([this, selfRoot]{ this->OnJobQueueItemComplete(); });

            this->Enqueue(ApplicationJobItem(move(casted)));
        }
        else
        {
            auto msg = wformatString(
                "{0} dynamic_pointer_cast<ImageBuilderAsyncOperationBase> failed", 
                dynamic_cast<NodeTraceComponent&>(owner).TraceId);

            owner.WriteError(TraceComponent, "{0}", msg);

            Assert::TestAssert("{0}", msg);

            operation->TryComplete(operation, ErrorCodeValue::OperationFailed);
        }
    }

    void UnregisterAndClose()
    {
        this->GetThrottleConfigEntry().RemoveHandler(configHandlerId_);

        JobQueue::Close();
    }

protected:

    virtual ConfigEntryBase & GetThrottleConfigEntry() = 0;
    virtual int GetThrottleConfigValue() = 0;

    void InitializeConfigUpdates()
    {
        this->OnConfigUpdate();

        ComponentRootWPtr weakRoot = this->CreateComponentRoot();

        configHandlerId_ = this->GetThrottleConfigEntry().AddHandler(
            [this, weakRoot](EventArgs const&)
            {
                auto root = weakRoot.lock();
                if (root)
                {
                    this->OnConfigUpdate();
                }
            });
    }

private:
    void OnConfigUpdate()
    {
        this->UpdateMaxThreads(this->GetThrottleConfigValue());
    }

    void OnJobQueueItemComplete()
    {
        this->CompleteAsyncJob();
    }

    HHandler configHandlerId_;
};

class ImageBuilderProxy::ApplicationJobQueue : public ApplicationJobQueueBase
{
public:
    ApplicationJobQueue(
        __in ComponentRoot & root)
        : ApplicationJobQueueBase(wformatString("{0}", TraceComponent), root)
    {
        this->InitializeConfigUpdates();
    }

protected:

    ConfigEntryBase & GetThrottleConfigEntry() override
    {
        return ManagementConfig::GetConfig().ImageBuilderJobQueueThrottleEntry;
    }

    int GetThrottleConfigValue() override
    {
        return ManagementConfig::GetConfig().ImageBuilderJobQueueThrottle;
    }
};

class ImageBuilderProxy::UpgradeJobQueue : public ApplicationJobQueueBase
{
public:
    UpgradeJobQueue(
        __in ComponentRoot & root)
        : ApplicationJobQueueBase(wformatString("{0}-Upgrade", TraceComponent), root)
    {
        this->InitializeConfigUpdates();
    }

    void TryEnqueueOrFail(__in ImageBuilderProxy & owner, AsyncOperationSPtr const & operation) override
    {
        if (this->GetMaxThreads() > 0)
        {
            ApplicationJobQueueBase::TryEnqueueOrFail(owner, operation);
        }
        else
        {
            owner.applicationJobQueue_->TryEnqueueOrFail(owner, operation);
        }
    }

protected:

    ConfigEntryBase & GetThrottleConfigEntry() override
    {
        return ManagementConfig::GetConfig().ImageBuilderUpgradeJobQueueThrottleEntry;
    }

    int GetThrottleConfigValue() override
    {
        return ManagementConfig::GetConfig().ImageBuilderUpgradeJobQueueThrottle;
    }
};

//
// *** ImageBuilderProxy
//

ImageBuilderProxy::ImageBuilderProxy(
    wstring const & imageBuilderExeDirectory,
    wstring const & workingDirectory,
    wstring const & nodeName,
    Federation::NodeInstance const & nodeInstance)
    : NodeTraceComponent(nodeInstance)
    , ComponentRoot()
    , workingDir_(workingDirectory)
    , nodeName_(nodeName)
    , imageBuilderExePath_(Path::Combine(imageBuilderExeDirectory, ImageBuilderExeName))
    , imageBuilderOutputBaseDirectory_(Path::Combine(workingDirectory, ImageBuilderOutputDirectory))
    , appTypeInfoOutputBaseDirectory_(Path::Combine(imageBuilderOutputBaseDirectory_, AppTypeInfoOutputDirectory))
    , appTypeOutputBaseDirectory_(Path::Combine(imageBuilderOutputBaseDirectory_, AppTypeOutputDirectory))
    , appOutputBaseDirectory_(Path::Combine(imageBuilderOutputBaseDirectory_, AppOutputDirectory))
    , fabricOutputBaseDirectory_(Path::Combine(imageBuilderOutputBaseDirectory_, FabricUpgradeOutputDirectory))
    , isEnabled_(false)
    , processHandles_()
    , processHandlesLock_()
    , securitySettings_()
    , securitySettingsLock_()
    , applicationJobQueue_()
    , upgradeJobQueue_()
{
    WriteInfo(
        TraceComponent, 
        "{0} ctor",
        NodeTraceComponent::TraceId);

    DeleteDirectory(Path::Combine(workingDir_, TempWorkingDirectoryPrefix));
    DeleteDirectory(Path::Combine(workingDir_, ArchiveDirectoryPrefix));
    perfCounters_ = ImageBuilderPerformanceCounters::CreateInstance(wformatString("{0}:{1}",
        nodeInstance,
        DateTime::Now().Ticks));
}

shared_ptr<ImageBuilderProxy> ImageBuilderProxy::Create(
    wstring const & imageBuilderExeDirectory,
    wstring const & workingDirectory,
    wstring const & nodeName,
    Federation::NodeInstance const & nodeInstance)
{
    auto proxy = shared_ptr<ImageBuilderProxy>(new ImageBuilderProxy(
        imageBuilderExeDirectory,
        workingDirectory,
        nodeName,
        nodeInstance));

    proxy->Initialize();

    return proxy;
}

void ImageBuilderProxy::Initialize()
{
    applicationJobQueue_ = make_unique<ApplicationJobQueue>(*this);
    upgradeJobQueue_ = make_unique<UpgradeJobQueue>(*this);

    // Delete any leaked directories and files inside appTypeOutputBaseDirectory_.
    // This prevents directory for application packages downloaded from external store to be leaked.
    this->DeleteDirectory(appTypeOutputBaseDirectory_);
}

ImageBuilderProxy::~ImageBuilderProxy()
{
    if (applicationJobQueue_)
    {
        applicationJobQueue_->UnregisterAndClose();
    }

    if (upgradeJobQueue_)
    {
        upgradeJobQueue_->UnregisterAndClose();
    }
}

void ImageBuilderProxy::UpdateSecuritySettings(SecuritySettings const & settings)
{
    AcquireWriteLock lock(securitySettingsLock_);

    securitySettings_ = settings;

    WriteInfo(
        TraceComponent, 
        "{0} Updated security settings: {1}", 
        NodeTraceComponent::TraceId,
        securitySettings_);
}

ErrorCode ImageBuilderProxy::VerifyImageBuilderSetup()
{
    if (!File::Exists(imageBuilderExePath_))
    {
        wstring currentDirectory = Directory::GetCurrentDirectory();

        WriteWarning(
            TraceComponent, 
            "{0} could not find '{1}' (current dir = '{2}')",
            NodeTraceComponent::TraceId,
            imageBuilderExePath_,
            currentDirectory);

        return ErrorCodeValue::NotFound;
    }

    return ErrorCodeValue::Success;
}

void ImageBuilderProxy::Enable()
{
    AcquireExclusiveLock lock(processHandlesLock_);

    isEnabled_ = true;
}

void ImageBuilderProxy::Disable()
{
    vector<HANDLE> tempHandles;
    {
        AcquireExclusiveLock lock(processHandlesLock_);

        isEnabled_ = false;

        tempHandles.swap(processHandles_);
    }

    for (auto const & hProcess : tempHandles)
    {
        WriteInfo(
            TraceComponent, 
            "{0} terminating ImageBuilder process: handle = {1}",
            NodeTraceComponent::TraceId,
            HandleToInt(hProcess));

        // Assumes FABRIC_E_* HRESULTs are FACILITY_WIN32 (i.e. 0x8007xxxx)
        // for conversion back to HRESULT from Win32 error
        //
        if (::TerminateProcess(hProcess, ImageBuilderDisabledExitCode) == FALSE)
        {
            WriteInfo(
                TraceComponent, 
                "{0} failed to terminate ImageBuilder process during Disable(): handle = {1}",
                NodeTraceComponent::TraceId,
                HandleToInt(hProcess));
        }

        // Don't close the handle here - RunImageBuilderExe() will close it
    }
}

void ImageBuilderProxy::Abort(HANDLE hProcess)
{
    if (TryRemoveProcessHandle(hProcess))
    {
        auto result = ::TerminateProcess(hProcess, ImageBuilderAbortedExitCode);

        WriteInfo(
            TraceComponent, 
            "{0} termination of ImageBuilder process {1}: handle = {2}",
            NodeTraceComponent::TraceId,
            result == TRUE ? "succeeded" : "failed",
            HandleToInt(hProcess));
    }
}

bool ImageBuilderProxy::TryRemoveProcessHandle(HANDLE hProcess)
{
    AcquireExclusiveLock lock(processHandlesLock_);

    auto findIt = find_if(processHandles_.begin(), processHandles_.end(), 
        [hProcess](HANDLE const & handle) { return (handle == hProcess); });

    if (findIt != processHandles_.end())
    {
        processHandles_.erase(findIt);

        return true;
    }
    else
    {
        return false;
    }
}

ErrorCode ImageBuilderProxy::GetApplicationTypeInfo(
    wstring const & buildPath, 
    TimeSpan const timeout,
    __out ServiceModelTypeName & typeNameResult, 
    __out ServiceModelVersion & typeVersionResult)
{
    // GetApplicationTypeInfo() requests are serialized by the CM
    //
    wstring outputDirectory = appTypeInfoOutputBaseDirectory_;
    ErrorCode error = CreateOutputDirectory(outputDirectory);
    if (!error.IsSuccess())
    {
        return error;
    }
   
    wstring cmdLineArgs;
    InitializeCommandLineArguments(cmdLineArgs);
    AddImageBuilderArgument(cmdLineArgs, BuildPath, buildPath);

    wstring outputFile = Path::Combine(outputDirectory, ApplicationTypeInfoOutputFilename);

    error = RunImageBuilderExe(
        OperationBuildApplicationTypeInfo,
        outputFile,
        cmdLineArgs,
        timeout);

    if (error.IsSuccess())
    {
        error = ReadApplicationTypeInfo(outputFile, /*out*/ typeNameResult, /*out*/ typeVersionResult);
    }

    DeleteDirectory(outputDirectory);

    return error;
}

ErrorCode ImageBuilderProxy::BuildManifestContexts(
    vector<ApplicationTypeContext> const& applicationTypeContexts,
    TimeSpan const timeout,
    __out vector<StoreDataApplicationManifest> & appManifestContexts,
    __out vector<StoreDataServiceManifest> & serviceManifestContexts)
{
    wstring outputDir = appTypeInfoOutputBaseDirectory_;
    auto error = CreateOutputDirectory(outputDir);
    if (!error.IsSuccess())
    {
        return error;
    }

    vector<StoreLayoutSpecification> appTypes;

    // Pathlist for application manifests is constructed as a list of applicationmanifestfilepath and outputpath
    vector<wstring> pathList;
    for (auto itr = applicationTypeContexts.begin(); itr != applicationTypeContexts.end(); ++itr)
    {
        StoreLayoutSpecification layout;
        wstring storeAppManifestPath = layout.GetApplicationManifestFile(itr->TypeName.Value, itr->TypeVersion.Value);
        wstring destAppManifestPath = Path::Combine(outputDir, storeAppManifestPath);

        pathList.push_back(storeAppManifestPath); // source
        pathList.push_back(destAppManifestPath); // destination
    }

    wstring argumentFile = Path::Combine(appTypeInfoOutputBaseDirectory_, ApplicationTypeInfoOutputFilename);
    error = this->WriteFilenameList(argumentFile, pathList);
    pathList.clear();

    if (!error.IsSuccess())
    {
        return error;
    }

    error = GetManifests(argumentFile, timeout);

    if (!error.IsSuccess())
    {
        DeleteDirectory(outputDir);
        return error;
    }

    // Process the application manifests and get the servicemanifests to download
    vector<ApplicationManifestDescription> appManifestDescriptions;
    for (auto itr = applicationTypeContexts.begin(); itr != applicationTypeContexts.end(); ++itr)
    {
        StoreLayoutSpecification sourceLayout;
        StoreLayoutSpecification destLayout(outputDir);
        ApplicationManifestDescription appManifest;
        wstring applicationManifestPath = destLayout.GetApplicationManifestFile(itr->TypeName.Value, itr->TypeVersion.Value);
        error = ReadApplicationManifest(applicationManifestPath, appManifest);
        if (error.IsSuccess())
        {
            appManifestDescriptions.push_back(appManifest);
			for (auto itr1 = appManifest.ServiceManifestImports.begin(); itr1 != appManifest.ServiceManifestImports.end(); ++itr1)
            {
				wstring serviceManStorePath = GetServiceManifestFileName(sourceLayout, appManifest.Name, itr1->ServiceManifestRef.Name, itr1->ServiceManifestRef.Version);
				wstring serviceManDestPath = GetServiceManifestFileName(destLayout, appManifest.Name, itr1->ServiceManifestRef.Name, itr1->ServiceManifestRef.Version);
                pathList.push_back(serviceManStorePath);
                pathList.push_back(serviceManDestPath);
            }
        }
        else
        {
            DeleteDirectory(outputDir);
            return error;
        }
    }

    // Download the service manifests.

    error = this->WriteFilenameList(argumentFile, pathList);
    if (!error.IsSuccess())
    {
       DeleteDirectory(outputDir);
       return error;
    }

    error = GetManifests(argumentFile, timeout);

    // Parse the service manifests
    for (auto itr = appManifestDescriptions.begin(); (itr != appManifestDescriptions.end() && error.IsSuccess()); ++itr)
    {
        StoreLayoutSpecification layout(outputDir);
        vector<ServiceModelServiceManifestDescription> serviceManifests;
        ApplicationHealthPolicy healthPolicy;
        map<wstring, wstring> defaultParamList;
        wstring applicationManifestContent;

        error = ParseApplicationManifest(*itr, layout, serviceManifests);
        if (error.IsSuccess())
        {
            healthPolicy = itr->Policies.HealthPolicy;
            defaultParamList = move(itr->ApplicationParameters);
            wstring applicationManifestPath = layout.GetApplicationManifestFile(itr->Name, itr->Version);
            error = ReadApplicationManifestContent(applicationManifestPath, applicationManifestContent);
            if (error.IsSuccess())
            {
                appManifestContexts.push_back(
                    StoreDataApplicationManifest(
                    ServiceModelTypeName(itr->Name), 
                    ServiceModelVersion(itr->Version), 
                    move(applicationManifestContent), 
                    move(healthPolicy), 
                    move(defaultParamList)));

                serviceManifestContexts.push_back(
                    StoreDataServiceManifest(
                    ServiceModelTypeName(itr->Name), 
                    ServiceModelVersion(itr->Version), 
                    move(serviceManifests)));
            }
        }
    }

    DeleteDirectory(outputDir);

    return error;
}

ErrorCode ImageBuilderProxy::Test_BuildApplicationType(
    Common::ActivityId const & activityId,
    wstring const & buildPath,
    wstring const & downloadPath,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    TimeSpan const timeout,
    __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
    __out wstring & applicationManifestContent,
    __out ApplicationHealthPolicy & healthPolicy,
    __out map<wstring, wstring> & defaultParamList)
{
    ManualResetEvent event(false);
    
    auto operation = this->BeginBuildApplicationType(
        activityId,
        buildPath,
        downloadPath,
        typeName,
        typeVersion,
        nullptr, // progressDetailsCallback
        timeout,
        [&](AsyncOperationSPtr const &) { event.Set(); },
        applicationJobQueue_->CreateAsyncOperationRoot());

    event.WaitOne();

    wstring manifestId;

    return this->EndBuildApplicationType(
        operation,
        serviceManifests,
        manifestId,
        applicationManifestContent,
        healthPolicy,
        defaultParamList);
}

AsyncOperationSPtr ImageBuilderProxy::BeginBuildComposeDeploymentType(
    ActivityId const & activityId,
    Common::ByteBufferSPtr const &composeFile,
    Common::ByteBufferSPtr const &overridesFile,
    std::wstring const &registryUserName,
    std::wstring const &registryPassword,
    bool isPasswordEncrypted,
    NamingUri const &appName,
    ServiceModelTypeName const &typeName,
    ServiceModelVersion const &typeVersion,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = shared_ptr<BuildComposeDeploymentAppTypeAsyncOperation>(new BuildComposeDeploymentAppTypeAsyncOperation(
        *this,
        activityId,
        composeFile,
        overridesFile,
        registryUserName,
        registryPassword,
        isPasswordEncrypted,
        appName,
        typeName,
        typeVersion,
        timeout,
        callback,
        parent));

    applicationJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

ErrorCode ImageBuilderProxy::EndBuildComposeDeploymentType(
    AsyncOperationSPtr const & operation,
    __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
    __out wstring & applicationManifestId,
    __out wstring & applicationManifestContent,
    __out ServiceModel::ApplicationHealthPolicy & healthPolicy,
    __out map<wstring, wstring> & defaultParamList,
    __out wstring &mergedComposeFile)
{
    return BuildComposeDeploymentAppTypeAsyncOperation::End(
        operation,
        serviceManifests,
        applicationManifestId,
        applicationManifestContent,
        healthPolicy,
        defaultParamList,
        mergedComposeFile);
}

AsyncOperationSPtr ImageBuilderProxy::BeginBuildComposeApplicationTypeForUpgrade(
    ActivityId const & activityId,
    Common::ByteBufferSPtr const &composeFile,
    Common::ByteBufferSPtr const &overridesFile,
    std::wstring const &registryUserName,
    std::wstring const &registryPassword,
    bool isPasswordEncrypted,
    NamingUri const &appName,
    ServiceModelTypeName const &typeName,
    ServiceModelVersion const &currentTypeVersion,
    ServiceModelVersion const &targetTypeVersion,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = make_shared<BuildComposeApplicationTypeForUpgradeAsyncOperation>(
        *this,
        activityId,
        composeFile,
        overridesFile,
        registryUserName,
        registryPassword,
        isPasswordEncrypted,
        appName,
        typeName,
        currentTypeVersion,
        targetTypeVersion,
        timeout,
        callback,
        parent);

    applicationJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

ErrorCode ImageBuilderProxy::EndBuildComposeApplicationTypeForUpgrade(
    AsyncOperationSPtr const & operation,
    __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
    __out wstring & applicationManifestId,
    __out wstring & applicationManifestContent,
    __out ServiceModel::ApplicationHealthPolicy & healthPolicy,
    __out map<wstring, wstring> & defaultParamList,
    __out wstring &mergedComposeFile)
{
    return BuildComposeDeploymentAppTypeAsyncOperation::End(
        operation,
        serviceManifests,
        applicationManifestId,
        applicationManifestContent,
        healthPolicy,
        defaultParamList,
        mergedComposeFile);
}

AsyncOperationSPtr ImageBuilderProxy::BeginBuildSingleInstanceApplication(
    ActivityId const & activityId,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    ServiceModel::ModelV2::ApplicationDescription const & applicationDescription,
    NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = make_shared<BuildSingleInstanceApplicationAsyncOperation>(
        *this,
        activityId,
        applicationDescription,
        typeName,
        typeVersion,
        appName,
        appId,
        timeout,
        callback,
        parent);

    applicationJobQueue_->TryEnqueueOrFail(*this, operation);
    return operation;
}

ErrorCode ImageBuilderProxy::EndBuildSingleInstanceApplication(
    AsyncOperationSPtr const & operation,
    __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
    __out wstring & applicationManifestContent,
    __out ApplicationHealthPolicy & healthPolicy,
    __out map<wstring, wstring> & defaultParamList,
    __out DigestedApplicationDescription & digestedApplicationDescription)
{
    return BuildSingleInstanceApplicationAsyncOperation::End(
        operation,
        serviceManifests,
        applicationManifestContent,
        healthPolicy,
        defaultParamList,
        digestedApplicationDescription);
}

AsyncOperationSPtr ImageBuilderProxy::BeginBuildSingleInstanceApplicationForUpgrade(
    ActivityId const &activityId,
    ServiceModelTypeName const &typeName,
    ServiceModelVersion const &currentTypeVersion,
    ServiceModelVersion const &targetTypeVersion,
    ModelV2::ApplicationDescription  const &description,
    Common::NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    uint64 currentApplicationVersion,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    auto operation = make_shared<BuildSingleInstanceApplicationForUpgradeAsyncOperation>(
        *this,
        activityId,
        description,
        typeName,
        currentTypeVersion,
        targetTypeVersion,
        appName,
        appId,
        currentApplicationVersion,
        timeout,
        callback,
        parent);

    applicationJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

ErrorCode ImageBuilderProxy::EndBuildSingleInstanceApplicationForUpgrade(
   Common::AsyncOperationSPtr const &operation,
   __out std::vector<ServiceModelServiceManifestDescription> &serviceManifests,
   __out std::wstring &applicationManifestContent,
   __out ServiceModel::ApplicationHealthPolicy &healthPolicy,
   __out map<wstring, wstring> &defaultParamList,
   __out DigestedApplicationDescription & currentApplicationDescription,
   __out DigestedApplicationDescription & targetApplicationDescription)
{
    return BuildSingleInstanceApplicationForUpgradeAsyncOperation::End(
        operation,
        serviceManifests,
        applicationManifestContent,
        healthPolicy,
        defaultParamList,
        currentApplicationDescription,
        targetApplicationDescription);
}

AsyncOperationSPtr ImageBuilderProxy::BeginBuildApplicationType(
    ActivityId const & activityId,
    wstring const & buildPath,
    wstring const & downloadPath,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    ProgressDetailsCallback const & progressDetailsCallback,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = shared_ptr<BuildApplicationTypeAsyncOperation>(new BuildApplicationTypeAsyncOperation(
        *this,
        activityId,
        buildPath,
        downloadPath,
        typeName,
        typeVersion,
        progressDetailsCallback,
        timeout,
        callback,
        parent));

    applicationJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

ErrorCode ImageBuilderProxy::EndBuildApplicationType(
    AsyncOperationSPtr const & operation,
    __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
    __out wstring & applicationManifestId,
    __out wstring & applicationManifestContent,
    __out ServiceModel::ApplicationHealthPolicy & healthPolicy,
    __out map<wstring, wstring> & defaultParamList)
{
    return BuildApplicationTypeAsyncOperation::End(
        operation, 
        serviceManifests, 
        applicationManifestId,
        applicationManifestContent,
        healthPolicy, 
        defaultParamList);
}

ErrorCode ImageBuilderProxy::ValidateComposeFile(
    ByteBuffer const &composeFile,
    NamingUri const &appName,
    ServiceModelTypeName const &applicationTypeName,
    ServiceModelVersion const &applicationTypeVersion,
    TimeSpan const &timeout)
{
    wstring cmdLineArgs;
    InitializeCommandLineArguments(cmdLineArgs);
    
    auto outputDirectory = Path::Combine(appTypeOutputBaseDirectory_, applicationTypeName.Value);
    auto error = CreateOutputDirectory(outputDirectory);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Create a file for compose and one for override.
    wstring tempDockerComposeFilename = File::GetTempFileNameW(outputDirectory);
    error = WriteToFile(tempDockerComposeFilename, composeFile);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to write docker compose file for imagebuilder - {1} : Error: {2}",
            NodeTraceComponent::TraceId,
            tempDockerComposeFilename,
            error);

        return error;
    }

    AddImageBuilderArgument(cmdLineArgs, ComposeFilePath, tempDockerComposeFilename);
    AddImageBuilderArgument(cmdLineArgs, CleanupComposeFiles, L"true");

    AddImageBuilderArgument(cmdLineArgs, AppName, appName.ToString());
    AddImageBuilderArgument(cmdLineArgs, AppTypeName, applicationTypeName.Value);
    AddImageBuilderArgument(cmdLineArgs, AppTypeVersion, applicationTypeVersion.Value);

    error = RunImageBuilderExe(
        OperationValidateComposeFile,
        outputDirectory,
        cmdLineArgs,
        timeout);

    // The temp files are deleted by Image Builder
    return error;
}

ErrorCode ImageBuilderProxy::Test_BuildApplication(
    Common::NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    ServiceModelTypeName const & appTypeName,
    ServiceModelVersion const & appTypeVersion,
    map<wstring, wstring> const & applicationParameters,
    TimeSpan const timeout,
    __out DigestedApplicationDescription & applicationResult)
{
    ManualResetEvent event(false);

    auto operation = this->BeginBuildApplication(
        ActivityId(),
        appName,
        appId,
        appTypeName,
        appTypeVersion,
        applicationParameters,
        timeout,
        [&](AsyncOperationSPtr const &) { event.Set(); },
        applicationJobQueue_->CreateAsyncOperationRoot());

    event.WaitOne();

    return this->EndBuildApplication(operation, applicationResult);
}

AsyncOperationSPtr ImageBuilderProxy::BeginBuildApplication(
    Common::ActivityId const & activityId,
    Common::NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    ServiceModelTypeName const & appTypeName,
    ServiceModelVersion const & appTypeVersion,
    map<wstring, wstring> const & applicationParameters,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = shared_ptr<BuildApplicationAsyncOperation>(new BuildApplicationAsyncOperation(
        *this,
        activityId,
        appName,
        appId,
        appTypeName,
        appTypeVersion,
        applicationParameters,
        timeout,
        callback,
        parent));

    applicationJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

ErrorCode ImageBuilderProxy::EndBuildApplication(
    AsyncOperationSPtr const & operation,
    __out DigestedApplicationDescription & currentApplicationResult)
{
    return BuildApplicationAsyncOperation::End(operation, currentApplicationResult);
}

AsyncOperationSPtr ImageBuilderProxy::BeginUpgradeApplication(
    Common::ActivityId const & activityId,
    NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    ServiceModelTypeName const & appTypeName,
    ServiceModelVersion const & targetAppTypeVersion,
    uint64 currentApplicationVersion,
    map<wstring, wstring> const & applicationParameters,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = shared_ptr<UpgradeApplicationAsyncOperation>(new UpgradeApplicationAsyncOperation(
        *this,
        activityId,
        appName,
        appId,
        appTypeName,
        targetAppTypeVersion,
        currentApplicationVersion,
        applicationParameters,
        timeout,
        callback,
        parent));

    upgradeJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

ErrorCode ImageBuilderProxy::EndUpgradeApplication(
    AsyncOperationSPtr const & operation,
    __out DigestedApplicationDescription & currentApplicationResult,
    __out DigestedApplicationDescription & targetApplicationResult)
{
    return UpgradeApplicationAsyncOperation::End(operation, currentApplicationResult, targetApplicationResult);
}

ErrorCode ImageBuilderProxy::GetFabricVersionInfo(
    wstring const & codeFilepath,
    wstring const & clusterManifestFilepath,
    TimeSpan const timeout,
    __out FabricVersion & fabricVersion)
{
    wstring cmdLineArgs;
    InitializeCommandLineArguments(cmdLineArgs);
    AddImageBuilderArgument(cmdLineArgs, FabricCodeFilepath, codeFilepath);
    AddImageBuilderArgument(cmdLineArgs, FabricConfigFilepath, clusterManifestFilepath);

    wstring outputDirectory = fabricOutputBaseDirectory_;
    wstring outputFile = Path::Combine(outputDirectory, FabricVersionOutputFilename);

    ErrorCode error = CreateOutputDirectory(outputDirectory);

    if (error.IsSuccess())
    {
        error = RunImageBuilderExe(
            OperationGetFabricVersion,
            outputFile,
            cmdLineArgs,
            timeout);
    }

    if (error.IsSuccess())
    {
        error = ReadFabricVersion(outputFile, fabricVersion);
    }

    DeleteDirectory(outputDirectory);

    return error;
}

ErrorCode ImageBuilderProxy::ProvisionFabric(
    wstring const & codeFilepath,
    wstring const & clusterManifestFilepath,
    TimeSpan const timeout)
{
    wstring cmdLineArgs;
    InitializeCommandLineArguments(cmdLineArgs);
    AddImageBuilderArgument(cmdLineArgs, FabricCodeFilepath, codeFilepath);
    AddImageBuilderArgument(cmdLineArgs, FabricConfigFilepath, clusterManifestFilepath);

    wstring dataRoot;
    auto error = FabricEnvironment::GetFabricDataRoot(dataRoot);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "{0} failed to get FabricDataRoot because of {1}",
            NodeTraceComponent::TraceId,
            error);
        return error;
    }

    FabricDeploymentSpecification fabricDeploymentSpec(dataRoot);

    AddImageBuilderArgument(cmdLineArgs, InfrastructureManifestFile, fabricDeploymentSpec.GetInfrastructureManfiestFile(nodeName_));

    return RunImageBuilderExe(
        OperationProvisionFabric,
        L"",
        cmdLineArgs,
        timeout);
}

ErrorCode ImageBuilderProxy::GetClusterManifestContents(
    Common::FabricConfigVersion const & version,
    TimeSpan const timeout,
    __out wstring & clusterManifestContents)
{
    wstring cmdLineArgs;
    InitializeCommandLineArguments(cmdLineArgs);
    AddImageBuilderArgument(cmdLineArgs, FabricConfigVersion, version.ToString());

    auto outputDirectory = this->GetImageBuilderTempWorkingDirectory();
    auto error = CreateOutputDirectory(outputDirectory);

    if (error.IsSuccess())
    {
        error = RunImageBuilderExe(
            OperationGetClusterManifest,
            outputDirectory,
            cmdLineArgs,
            timeout);
    }

    if (error.IsSuccess())
    {
        error = ReadClusterManifestContents(
            outputDirectory,
            clusterManifestContents);
    }

    DeleteOrArchiveDirectory(outputDirectory, error);

    return error;
}

ErrorCode ImageBuilderProxy::UpgradeFabric(
    FabricVersion const & currentVersion,
    FabricVersion const & targetVersion,
    TimeSpan const timeout,
    __out bool & isConfigOnly)
{
    isConfigOnly = false;

    wstring cmdLineArgs;
    InitializeCommandLineArguments(cmdLineArgs);
    AddImageBuilderArgument(cmdLineArgs, CurrentFabricVersion, currentVersion.ToString());
    AddImageBuilderArgument(cmdLineArgs, TargetFabricVersion, targetVersion.ToString());

    wstring outputDirectory = fabricOutputBaseDirectory_;
    wstring outputFile = Path::Combine(outputDirectory, FabricUpgradeResultFilename);

    auto error = CreateOutputDirectory(outputDirectory);

    if (error.IsSuccess())
    {
        error = RunImageBuilderExe(
            OperationUpgradeFabric,
            outputFile,
            cmdLineArgs,
            timeout);
    }

    // The result file will not exist for the baseline upgrade since
    // no FabricSettings comparison or validation occurs.
    //
    if (error.IsSuccess() && File::Exists(outputFile))
    {
        FabricVersion currentVersionResult;
        FabricVersion targetVersionResult;
        bool isConfigOnlyResult;

        error = ReadFabricUpgradeResult(
            outputFile, 
            currentVersionResult, 
            targetVersionResult, 
            isConfigOnlyResult);

        // Sanity check that result versions match
        // the expected upgrade versions before using
        // config-only result.
        // 
        if (error.IsSuccess() &&
            currentVersionResult == currentVersion &&
            targetVersionResult == targetVersion)
        {
            isConfigOnly = isConfigOnlyResult;
        }
    }

    DeleteDirectory(outputDirectory);

    return error;
}

ErrorCode ImageBuilderProxy::Test_CleanupApplication(
    ServiceModelTypeName const & appTypeName,
    ServiceModelApplicationId const & appId,
    TimeSpan const timeout)
{
    ManualResetEvent event(false);

    auto operation = this->BeginCleanupApplication(
        ActivityId(),
        appTypeName,
        appId,
        timeout,
        [&](AsyncOperationSPtr const &) { event.Set(); },
        applicationJobQueue_->CreateAsyncOperationRoot());

    event.WaitOne();

    return this->EndCleanupApplication(operation);
}

AsyncOperationSPtr ImageBuilderProxy::BeginCleanupApplication(
    ActivityId const & activityId,
    ServiceModelTypeName const & appTypeName,
    ServiceModelApplicationId const & appId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = shared_ptr<CleanupApplicationAsyncOperation>(new CleanupApplicationAsyncOperation(
        *this,
        activityId,
        appTypeName,
        appId,
        timeout,
        callback,
        parent));

    applicationJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

ErrorCode ImageBuilderProxy::EndCleanupApplication(
    AsyncOperationSPtr const & operation)
{
    return CleanupApplicationAsyncOperation::End(operation);
}

ErrorCode ImageBuilderProxy::Test_CleanupApplicationInstance(
    ApplicationInstanceDescription const & appInstanceDescription,
    bool deleteApplicationPackage,
    vector<ServicePackageReference> const & servicePackageReferences,
    TimeSpan const timeout)
{
    ManualResetEvent event(false);

    auto operation = this->BeginCleanupApplicationInstance(
        ActivityId(),
        appInstanceDescription,
        deleteApplicationPackage,
        servicePackageReferences,
        timeout,
        [&](AsyncOperationSPtr const &) { event.Set(); },
        applicationJobQueue_->CreateAsyncOperationRoot());

    event.WaitOne();

    return this->EndCleanupApplicationInstance(operation);
}

AsyncOperationSPtr ImageBuilderProxy::BeginCleanupApplicationInstance(
    ActivityId const & activityId,
    ApplicationInstanceDescription const & appInstanceDescription,
    bool deleteApplicationPackage,
    vector<ServicePackageReference> const & servicePackageReferences,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = shared_ptr<CleanupApplicationInstanceAsyncOperation>(new CleanupApplicationInstanceAsyncOperation(
        *this,
        activityId,
        appInstanceDescription,
        deleteApplicationPackage,
        servicePackageReferences,
        timeout,
        callback,
        parent));

    applicationJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

ErrorCode ImageBuilderProxy::EndCleanupApplicationInstance(
    AsyncOperationSPtr const & operation)
{
    return CleanupApplicationInstanceAsyncOperation::End(operation);
}

ErrorCode ImageBuilderProxy::CleanupApplicationType(
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    vector<ServiceModelServiceManifestDescription> const & serviceManifests,
    bool isLastApplicationType,
    TimeSpan const timeout)
{
    wstring outputDirectory = Path::Combine(appTypeOutputBaseDirectory_, typeName.Value);
    wstring inputFile = Path::Combine(outputDirectory, CleanupListFilename);

    ErrorCode error = CreateOutputDirectory(outputDirectory);
    if (!error.IsSuccess())
    {
        return error;
    }

    // want paths to be relative from ImageStoreRoot
    StoreLayoutSpecification layout(L"");
    vector<wstring> cleanupFilenames;

    cleanupFilenames.push_back(layout.GetApplicationManifestFile(typeName.Value, typeVersion.Value));

    for (auto it = serviceManifests.begin(); it != serviceManifests.end(); ++it)
    {
        vector<wstring> filenames = it->GetFilenames(layout, typeName);
        for (auto && filename : filenames)
        {
            cleanupFilenames.push_back(move(filename));
        }
    }

    if (isLastApplicationType)
    {
        cleanupFilenames.push_back(layout.GetApplicationTypeFolder(typeName.Value));
    }
    
    error = this->DeleteFiles(inputFile, cleanupFilenames, timeout);

    DeleteDirectory(outputDirectory);

    return error;
}

ErrorCode ImageBuilderProxy::CleanupFabricVersion(
    FabricVersion const & fabricVersion,
    TimeSpan const timeout)
{
    wstring outputDirectory = fabricOutputBaseDirectory_;
    wstring inputFile = Path::Combine(outputDirectory, CleanupListFilename);

    ErrorCode error = CreateOutputDirectory(outputDirectory);
    if (!error.IsSuccess())
    {
        return error;
    }

    // want paths to be relative from ImageStoreRoot
    WinFabStoreLayoutSpecification layout(L"");
    vector<wstring> cleanupFilenames;

    if (fabricVersion.CodeVersion.IsValid)
    {
        cleanupFilenames.push_back(layout.GetPatchFile(fabricVersion.CodeVersion.ToString()));
        cleanupFilenames.push_back(layout.GetCabPatchFile(fabricVersion.CodeVersion.ToString()));
    }

    if (fabricVersion.ConfigVersion.IsValid)
    {        
        cleanupFilenames.push_back(layout.GetClusterManifestFile(fabricVersion.ConfigVersion.ToString()));
    }

    error = this->DeleteFiles(inputFile, cleanupFilenames, timeout);

    DeleteDirectory(outputDirectory);

    return error;
}

Common::ErrorCode Management::ClusterManager::ImageBuilderProxy::CleanupApplicationPackage(std::wstring const & buildPath, Common::TimeSpan const timeout)
{
    wstring cmdLineArgs;
    InitializeCommandLineArguments(cmdLineArgs);

    AddImageBuilderArgument(cmdLineArgs, BuildPath, buildPath);

    auto error = RunImageBuilderExe(
        OperationCleanupApplicationPackage,
        L"",
        cmdLineArgs,
        timeout);

    return error;
}

ErrorCode ImageBuilderProxy::GetManifests(
    wstring const& argumentFile,
    TimeSpan const timeout)
{
    wstring cmdLineArgs;
    InitializeCommandLineArguments(cmdLineArgs);

    AddImageBuilderArgument(cmdLineArgs, Input, argumentFile);
    
    auto error = RunImageBuilderExe(
        OperationGetManifests,
        L"",
        cmdLineArgs,
        timeout);

    return error;
}

void ImageBuilderProxy::InitializeCommandLineArguments(
    __inout wstring & args)
{
    args.append(L"\"");
    args.append(imageBuilderExePath_);
    args.append(L"\" ");
}

void ImageBuilderProxy::AddImageBuilderArgument(
    __inout wstring & args,
    wstring const & argSwitch,
    wstring const & argValue)
{
    args.append(argSwitch);
    args.append(L":\"");
    args.append(argValue);
    args.append(L"\" ");
}

ErrorCode ImageBuilderProxy::AddImageBuilderApplicationParameters(
    __inout wstring & args,
    map<wstring, wstring> const & applicationParameters,
    wstring const & tempWorkingDir)
{
    vector<wstring> parametersList;
    for (auto iter = applicationParameters.begin(); iter != applicationParameters.end(); ++iter)
    {
        if (StringUtility::ContainsNewline(iter->first) || StringUtility::ContainsNewline(iter->second))
        {
            auto msg = wformatString(GET_CM_RC(Invalid_Application_Parameter_Format), iter->first);

            WriteError(
                TraceComponent,
                "{0} failed : {1}",
                NodeTraceComponent::TraceId,
                msg);

            return ErrorCode(ErrorCodeValue::ImageBuilderValidationError, move(msg));
        }

        parametersList.push_back(iter->first);
        parametersList.push_back(iter->second);
    }
    this->AddImageBuilderArgument(args, AppParam, AppParamFilename);

    return this->WriteFilenameList(
        Path::Combine(tempWorkingDir, AppParamFilename),
        parametersList);
}

wstring ImageBuilderProxy::CreatePair(wstring const & key, wstring const & value)
{
    return wformatString("{0}={1}", key, value);
}

wstring ImageBuilderProxy::GetEscapedString(wstring &input)
{
    wstring::size_type startTokenPos = input.find_first_of(L"\"");
    if (startTokenPos == wstring::npos)
    {
        return move(input);
    }

    wstring output;
    for (size_t i = 0; i < input.length(); ++i)
    {
        if (input[i] == '"')
        {
            output += L"\""; 
        }
        else
        {
            output += input[i];
        }
    }

    return output;
}

ErrorCode ImageBuilderProxy::RunImageBuilderExe(
    wstring const & operation,
    wstring const & outputDirectoryOrFile,
    __in wstring & args,
    TimeSpan const timeout,
    map<wstring, wstring> const & applicationParameters)
{
    auto actualTimeout = timeout;
    wstring tempWorkingDirectory;
    wstring errorDetailsFile;
    HANDLE hProcess;
    HANDLE hThread;
    pid_t pid;

    auto error = this->TryStartImageBuilderProcess(
        operation,
        outputDirectoryOrFile,
        args,
        applicationParameters,
        actualTimeout, // inout
        tempWorkingDirectory, // out
        errorDetailsFile, // out
        hProcess, // out
        hThread, // out
        pid);

    if (!error.IsSuccess())
    {
        return error;
    }

    AutoResetEvent waitCompleted(false);
    auto processWait = ProcessWait::CreateAndStart(
        Handle(hProcess, Handle::DUPLICATE()),
        pid,
        [&] (pid_t, ErrorCode const & waitResult, DWORD exitCode)
        {
            error = FinishImageBuilderProcess(
                waitResult,
                exitCode,
                tempWorkingDirectory,
                errorDetailsFile, 
                hProcess, 
                hThread);

            waitCompleted.Set();
        },
        actualTimeout);

    waitCompleted.WaitOne();
    return error;
}

AsyncOperationSPtr ImageBuilderProxy::BeginRunImageBuilderExe(
    ActivityId const & activityId,
    wstring const & operation,
    wstring const & outputDirectoryOrFile,
    __in wstring & args,
    map<wstring, wstring> const & applicationParameters,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RunImageBuilderExeAsyncOperation>(
        *this,
        activityId,
        operation,
        outputDirectoryOrFile,
        args,
        applicationParameters,
        timeout,
        callback,
        parent);
}

ErrorCode ImageBuilderProxy::EndRunImageBuilderExe(
    AsyncOperationSPtr const & operation)
{
    return RunImageBuilderExeAsyncOperation::End(operation);
}

ErrorCode ImageBuilderProxy::TryStartImageBuilderProcess(
    wstring const & operation,
    wstring const & outputDirectoryOrFile,
    __in wstring & args,
    map<wstring, wstring> const & applicationParameters,
    __inout TimeSpan & timeout,
    __out wstring & tempWorkingDirectory,
    __out wstring & errorDetailsFile,
    __out HANDLE & hProcess,
    __out HANDLE & hThread,
    __out pid_t & pid)
{
    pid = 0;

    vector<wchar_t> envBlock(0);
    auto error = ProcessUtility::CreateDefaultEnvironmentBlock(envBlock);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to create EnvironmentBlock for ImageBuilder due to {1}",
            NodeTraceComponent::TraceId,
            error);
        return error;
    }

    wchar_t* env = envBlock.data();

    PROCESS_INFORMATION processInfo = { 0 };
    STARTUPINFO startupInfo = { 0 };
    startupInfo.cb = sizeof(STARTUPINFO);

    // Set the bInheritHandle flag so pipe handles are inherited (from MSDN example)
    SECURITY_ATTRIBUTES secAttr; 
    secAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    secAttr.bInheritHandle = TRUE; 
    secAttr.lpSecurityDescriptor = NULL;     

    tempWorkingDirectory = this->GetImageBuilderTempWorkingDirectory();
    errorDetailsFile = Path::Combine(tempWorkingDirectory, ErrorDetailsFilename);

    error = CreateOutputDirectory(tempWorkingDirectory);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Add common commandline arguments
    //
    AddImageBuilderArgument(args, WorkingDir, tempWorkingDirectory);
    AddImageBuilderArgument(args, Operation, operation);
    AddImageBuilderArgument(args, ErrorDetails, errorDetailsFile);
    if (!outputDirectoryOrFile.empty())
    {
        AddImageBuilderArgument(args, Output, outputDirectoryOrFile);
    }
    
    if (!applicationParameters.empty())
    {
        error = AddImageBuilderApplicationParameters(args, applicationParameters, tempWorkingDirectory);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} failed to add application parameters: {1}",
                NodeTraceComponent::TraceId,
                error);

            DeleteOrArchiveDirectory(tempWorkingDirectory, error);

            return error;
        }
    }

    // Adjust the Image Builder timeout so that we can return an IB-specific timeout error
    // to the client (e.g. avoid client operation timing out before server). Don't adjust
    // if the timeout is already "too small". The client-side will just get a generic timeout
    // error is such cases.
    //
    auto timeoutBuffer = ManagementConfig::GetConfig().ImageBuilderTimeoutBuffer;
    if (timeout.TotalMilliseconds() > timeoutBuffer.TotalMilliseconds() * 2)
    {
        timeout = timeout.SubtractWithMaxAndMinValueCheck(timeoutBuffer);
    }

    AddImageBuilderArgument(args, Timeout, wformatString("{0}", timeout.Ticks));

    CMEvents::Trace->ImageBuilderCreating(
        this->NodeInstance,
        args,
        timeout,
        timeoutBuffer);

    // If ImageStoreConnectionString is encrypted use empty, ImageBuilderExe will look up the StoreRoot through config
    if(!Management::ManagementConfig::GetConfig().ImageStoreConnectionStringEntry.IsEncrypted())
    {
        AddImageBuilderArgument(args, StoreRoot, Management::ManagementConfig::GetConfig().ImageStoreConnectionString.GetPlaintext());
    }

    // Pass the flag indicating if SFVolumeDisk is enabled or not.
    if (Common::CommonConfig::GetConfig().EnableUnsupportedPreviewFeatures && Hosting2::HostingConfig::GetConfig().IsSFVolumeDiskServiceEnabled)
    {
        AddImageBuilderArgument(args, SFVolumeDiskServiceEnabled, L"true");    
    }
    else
    {
        AddImageBuilderArgument(args, SFVolumeDiskServiceEnabled, L"false"); 
    }


#if defined(PLATFORM_UNIX)
    vector<char> ansiEnvironment;
    while(*env)
    {
        string enva;
        wstring envw(env);
        StringUtility::Utf16ToUtf8(envw, enva);
        ansiEnvironment.insert(ansiEnvironment.end(), enva.begin(), enva.end());
        ansiEnvironment.push_back(0);
        env += envw.length() + 1;
    }

#endif

    SecureString secureArgs(move(args));

    BOOL success = FALSE;
    {
        AcquireExclusiveLock lock(processHandlesLock_);

        if (isEnabled_)
        {
            success = ::CreateProcessW(
                NULL,
                const_cast<LPWSTR>(secureArgs.GetPlaintext().c_str()),
                NULL,   // process attributes
                NULL,   // thread attributes
                FALSE,  // do not inherit handles
#if defined(PLATFORM_UNIX)
                0,
                ansiEnvironment.data(),
#else
                CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, // creation flags
                env,
#endif
                NULL,   // current directory
                &startupInfo,
                &processInfo);

            if (success == TRUE)
            {
                processHandles_.push_back(processInfo.hProcess);
            }
        }
        else
        {
            ::SetLastError(ErrorCode(ErrorCodeValue::ObjectClosed).ToHResult());
        }
    }

    if (success == TRUE)
    {
        CMEvents::Trace->ImageBuilderCreated(
            this->NodeInstance,
            HandleToInt(processInfo.hProcess),
            processInfo.dwProcessId);

        hProcess = processInfo.hProcess;
        hThread = processInfo.hThread;
        pid = processInfo.dwProcessId;

        return ErrorCodeValue::Success;
    }
    else
    {
        DWORD winError = ::GetLastError();
        error = ErrorCode::FromWin32Error(winError);

        WriteInfo(
            TraceComponent, 
            "{0} could not create ImageBuilder process due to {1} ({2})",
            NodeTraceComponent::TraceId,
            error,
            winError);

        DeleteOrArchiveDirectory(tempWorkingDirectory, error);

        return error;
    }
}

ErrorCode ImageBuilderProxy::FinishImageBuilderProcess(
    ErrorCode const & waitForProcessError,
    DWORD exitCode,
    wstring const & tempWorkingDirectory,
    wstring const & errorDetailsFile,
    HANDLE hProcess,
    HANDLE hThread)
{
    auto error = waitForProcessError;

    if (error.IsSuccess())
    {
        error = GetImageBuilderError(exitCode, errorDetailsFile);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to wait for ImageBuilder process due to error '{1}': handle = {2}",
            NodeTraceComponent::TraceId,
            error,
            HandleToInt(hProcess));

        // ImageBuilder.exe should consume the /timeout parameter and self-terminate,
        // but also perform a best-effort termination of the process here as well.
        //
        if (::TerminateProcess(hProcess, ImageBuilderAbortedExitCode) == FALSE)
        {
            WriteInfo(
                TraceComponent, 
                "{0} failed to terminate ImageBuilder process on error '{1}': handle = {2}",
                NodeTraceComponent::TraceId,
                error,
                HandleToInt(hProcess));
        }
    }

    if (error.IsError(ErrorCodeValue::Timeout))
    {
        // Return a more specific error code for better debugging
        //
        error = ErrorCodeValue::ImageBuilderTimeout;
    }
    else if (error.IsError(ErrorCodeValue::ImageBuilderAborted))
    {
        // ImageBuilderAborted currently has no corresponding public error code.
        // Expose public HRESULT if/when we find a need for it.
        //
        error = ErrorCodeValue::OperationCanceled;
    }

    DeleteOrArchiveDirectory(tempWorkingDirectory, error);

    // best-effort
    //
    ::CloseHandle(hProcess);
    ::CloseHandle(hThread);

    TryRemoveProcessHandle(hProcess);

    return error;
}

wstring ImageBuilderProxy::Test_GetImageBuilderTempWorkingDirectory()
{
    return this->GetImageBuilderTempWorkingDirectory();
}

wstring ImageBuilderProxy::GetImageBuilderTempWorkingDirectory()
{
    return Path::Combine(
        workingDir_,
        Path::Combine(
            TempWorkingDirectoryPrefix, 
            wformatString("{0}", SequenceNumber::GetNext())));
}

ErrorCode ImageBuilderProxy::GetImageBuilderError(
    DWORD imageBuilderError,
    wstring const & errorDetailsFile)
{
    auto error = ErrorCode::FromWin32Error(imageBuilderError);

    ErrorCodeValue::Enum errorValue = ErrorCodeValue::Success;
    wstring errorStack;
    if (!error.IsSuccess())
    {
        this->PerfCounters.RateOfImageBuilderFailures.Increment();
        if (File::Exists(errorDetailsFile))
        {
            wstring details;
            auto innerError = ReadImageBuilderOutput(errorDetailsFile, details);
            if (innerError.IsSuccess() && !details.empty())
            {
                wstring delimiter = L",";
                size_t found = details.find(delimiter);
                if (found != string::npos)
                {
                    wstring errorCodeString = details.substr(0, found);
                    errorStack = details.substr(found + 1);
                    int errorCodeInt;
                    StringUtility::TryFromWString<int>(errorCodeString, errorCodeInt);
                    errorValue = ErrorCode::FromWin32Error((unsigned int)errorCodeInt).ReadValue();
                }
                else
                {
                    WriteError(
                            TraceComponent, 
                            "{0} failed to get ImageBuilder return code due to wrong format of error file",
                            NodeTraceComponent::TraceId);
                    errorValue = error.ReadValue();
                    errorStack = details;
                }
            }
        }
    }

    if (error.IsError(ErrorCodeValue::AccessDenied))
    {
        // Return a more specific error for better debugging.
        //
        errorValue = ErrorCodeValue::ImageBuilderAccessDenied;
    }
    else if (error.IsError(ErrorCode::FromWin32Error(ImageBuilderDisabledExitCode).ReadValue()))
    { 
        // Error details file will not exist if IB.exe is explicitly terminated
        //
        errorValue = ErrorCodeValue::NotPrimary;
    }
    else if (error.IsError(ErrorCode::FromWin32Error(ImageBuilderAbortedExitCode).ReadValue()))
    { 
        // Error details file will not exist if IB.exe is explicitly terminated
        //
        errorValue = ErrorCodeValue::ImageBuilderAborted;
    }

    if (!error.IsSuccess())
    {
        if (errorValue == ErrorCodeValue::Success)
        {
            auto msg = wformatString(
                    "{0} unexpected success error code on ImageBuilder failure {1} ({2}) - overriding with ImageBuilderUnexpectedError",
                    NodeTraceComponent::TraceId,
                    imageBuilderError,
                    error);

            WriteError(TraceComponent, "{0}", msg);

            Assert::TestAssert("{0}", msg);

            errorValue = ErrorCodeValue::ImageBuilderUnexpectedError;
        }

        error = ErrorCode(errorValue, move(errorStack));

        CMEvents::Trace->ImageBuilderFailed(
                this->NodeInstance,
                error,
                error.Message);
    }

    return error;
}

ErrorCode ImageBuilderProxy::ReadApplicationTypeInfo(
    wstring const & inputFile, 
    __out ServiceModelTypeName & typeNameResult, 
    __out ServiceModelVersion & typeVersionResult)
{
    wstring unparsed;
    ErrorCode error = ReadImageBuilderOutput(inputFile, unparsed);

    if (error.IsSuccess())
    {
        CMEvents::Trace->ImageBuilderParseAppTypeInfo(
            this->NodeInstance,
            unparsed);

        vector<wstring> tokens;
        StringUtility::Split<wstring>(unparsed, tokens, FabricVersion::Delimiter);

        if (tokens.size() == 2)
        {
            typeNameResult = ServiceModelTypeName(tokens[0]);
            typeVersionResult = ServiceModelVersion(tokens[1]);
        }
        else
        {
            auto msg = wformatString(wformatString(GET_RC( ApplicationType_Parse ), unparsed));
            
            WriteWarning(TraceComponent, "{0}", msg); 

            error = ErrorCode(ErrorCodeValue::ImageBuilderValidationError, move(msg));
        }
    }

    return error;
}

ErrorCode ImageBuilderProxy::ReadFabricVersion(
    wstring const & inputFile, 
    __out FabricVersion & fabricVersion) 
{
    wstring unparsed;
    ErrorCode error = ReadImageBuilderOutput(inputFile, unparsed);

    if (error.IsSuccess())
    {
        CMEvents::Trace->ImageBuilderParseFabricVersion(
            this->NodeInstance,
            unparsed);

        // Don't skip empty tokens since we need to distinguish between
        // an unspecified code/config version (empty string before/after ":")
        //
        vector<wstring> tokens;
        StringUtility::Split<wstring>(unparsed, tokens, FabricVersion::Delimiter, false);

        if (tokens.size() == 1)
        {
            // only code version exists
            //
            error = ParseFabricVersion(tokens[0], L"", fabricVersion);
        }
        else if (tokens.size() == 2)
        {
            // both versions exist but code version might be unspecified
            //
            error = ParseFabricVersion(tokens[0], tokens[1], fabricVersion);
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0} invalid fabric version format: '{1}' error = {2}",
                NodeTraceComponent::TraceId,
                unparsed,
                error);

            error = ErrorCodeValue::OperationFailed;
        }
    }

    return error;
}

ErrorCode ImageBuilderProxy::ReadClusterManifestContents(
    wstring const & inputDirectory,
    __out wstring & content)
{
    return XmlReader::ReadXmlFile(
        Path::Combine(inputDirectory, L"ClusterManifest.xml"),
        *ServiceModel::SchemaNames::Element_ClusterManifest,
        *ServiceModel::SchemaNames::Namespace,
        content);
}

ErrorCode ImageBuilderProxy::ReadFabricUpgradeResult(
    wstring const & inputFile, 
    __out FabricVersion & currentVersionResult, 
    __out FabricVersion & targetVersionResult, 
    __out bool & isConfigOnlyResult)
{
    wstring unparsed;
    ErrorCode error = ReadImageBuilderOutput(inputFile, unparsed);

    if (error.IsSuccess())
    {
        CMEvents::Trace->ImageBuilderParseFabricUpgradeResult(
            this->NodeInstance,
            unparsed);

        // Result file will only exist after baseline upgrade has occurred.
        // Do not expect any unknown versions after that point.
        //
        vector<wstring> tokens;
        StringUtility::Split<wstring>(unparsed, tokens, FabricVersion::Delimiter, false);

        if (tokens.size() == 5)
        {
            error = ParseFabricVersion(tokens[0], tokens[1], currentVersionResult);

            if (error.IsSuccess())
            {
                error = ParseFabricVersion(tokens[2], tokens[3], targetVersionResult);
            }

            if (error.IsSuccess())
            {
                isConfigOnlyResult = (tokens[4] == L"True");
            }
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0} invalid fabric upgrade result format: '{1}' error = {2}",
                NodeTraceComponent::TraceId,
                unparsed,
                error);

            error = ErrorCodeValue::OperationFailed;
        }
    }

    return error;
}

ErrorCode ImageBuilderProxy::ReadImageBuilderOutput(
    wstring const & inputFile,
    __out wstring & result,
    __in bool isBOMPresent)
{
    File file;
    auto error = file.TryOpen(
        inputFile, 
        FileMode::Open, 
        FileAccess::Read, 
        FileShare::Read, 
#if defined(PLATFORM_UNIX)
        FileAttributes::Normal
#else
        FileAttributes::ReadOnly
#endif
        );

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to open '{1}' for read: error={2}",
            NodeTraceComponent::TraceId,
            inputFile,
            error);

        return ErrorCodeValue::CMImageBuilderRetryableError;
    }

    error = ReadFromFile(file, result, isBOMPresent);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to read from file '{1}': error={2}",
            NodeTraceComponent::TraceId,
            inputFile,
            error);
    }

    return error;
}

ErrorCode ImageBuilderProxy::ReadFromFile(
    __in File & file,   
    __out wstring & result,
    __in bool isBOMPresent)
{
    int bytesRead = 0;
    int fileSize = static_cast<int>(file.size());
    vector<byte> buffer(fileSize);

    if ((bytesRead = file.TryRead(reinterpret_cast<void*>(buffer.data()), fileSize)) > 0)
    {
        // ensure null termination
        buffer.push_back(0);

        // wide null for unicode
        buffer.push_back(0);
        
        // if the caller indicated that BOM is present, skip byte-order mark
        result = wstring(reinterpret_cast<wchar_t *>(&buffer[isBOMPresent ? 2 : 0]));

        return ErrorCodeValue::Success;
    }

    WriteInfo(
        TraceComponent, 
        "{0} failed to read from file: expected {1} bytes, read {2} bytes",
        NodeTraceComponent::TraceId,
        fileSize,
        bytesRead);

    return ErrorCodeValue::CMImageBuilderRetryableError;
}

ErrorCode ImageBuilderProxy::WriteToFile(
    __in std::wstring const &fileName, 
    __in Common::ByteBuffer const &bytes)
{
    auto openMode = File::Exists(fileName) ? FileMode::Truncate : FileMode::OpenOrCreate;

    File file;
    auto error = file.TryOpen(
        fileName,
        openMode,
        FileAccess::Write,
        FileShare::None,
        FileAttributes::None);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to open '{1}' (truncate={2}) for write: error={3}",
            NodeTraceComponent::TraceId,
            fileName,
            (openMode == FileMode::Truncate),
            error);

        return ErrorCodeValue::CMImageBuilderRetryableError;
    }

    if (bytes.size() > INT_MAX)
    {
        return ErrorCodeValue::EntryTooLarge;
    }

    try
    {
        file.Seek(0, SeekOrigin::Begin);
        file.Write(&bytes[0], static_cast<int>(bytes.size()));
    }
    catch (...)
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to write to '{1}'",
            NodeTraceComponent::TraceId,
            fileName);

        error = ErrorCodeValue::CMImageBuilderRetryableError;
    }

    return error;
}

ErrorCode ImageBuilderProxy::WriteFilenameList(
    wstring const & inputFile,
    vector<wstring> const & filenameList)
{
    auto openMode = File::Exists(inputFile) ? FileMode::Truncate : FileMode::OpenOrCreate;

    File file;
    auto error = file.TryOpen(
        inputFile, 
        openMode,
        FileAccess::Write, 
        FileShare::None, 
        FileAttributes::None);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to open '{1}' (truncate={2}) for write: error={3}",
            NodeTraceComponent::TraceId,
            inputFile,
            (openMode == FileMode::Truncate),
            error);

        error = ErrorCodeValue::CMImageBuilderRetryableError;
    }

    try
    {
        if (error.IsSuccess())
        {
            file.Seek(0, SeekOrigin::Begin);

            file.Write(&(*UnicodeBOM)[0], static_cast<int>(UnicodeBOM->size() * sizeof(wchar_t)));

            for (auto it = filenameList.begin(); it != filenameList.end(); ++it)
            {
                wstring const & buffer = *it;

                file.Write(&buffer[0], static_cast<int>(buffer.size() * sizeof(wchar_t)));
                file.Write(&(*Newline)[0], static_cast<int>(Newline->size() * sizeof(wchar_t)));
            }

            file.Flush();

            error = file.Close2();
        }
    }
    catch (...)
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to write to '{1}'",
            NodeTraceComponent::TraceId,
            inputFile);

        error = ErrorCodeValue::CMImageBuilderRetryableError;
    }

    return error;
}

ErrorCode ImageBuilderProxy::DeleteFiles(
    wstring const & inputFile,
    vector<wstring> const & filepaths,
    TimeSpan const timeout)
{
    ErrorCode error = this->WriteFilenameList(inputFile, filepaths);

    if (error.IsSuccess())
    {
        wstring cmdLineArgs;
        InitializeCommandLineArguments(cmdLineArgs);
        AddImageBuilderArgument(cmdLineArgs, Input, inputFile);

        WriteInfo(
            TraceComponent, 
            "{0} removing files: {1}",
            NodeTraceComponent::TraceId,
            filepaths);

        error = RunImageBuilderExe(
            OperationDelete,
            L"",
            cmdLineArgs,
            timeout);
    }

    return error;
}

AsyncOperationSPtr ImageBuilderProxy::BeginDeleteFiles(
    ActivityId const & activityId,
    wstring const & inputFile,
    vector<wstring> const & filepaths,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeleteFilesAsyncOperation>(
        *this,
        activityId,
        inputFile,
        filepaths,
        timeout,
        callback,
        parent);
}

ErrorCode ImageBuilderProxy::EndDeleteFiles(AsyncOperationSPtr const & operation)
{
    return DeleteFilesAsyncOperation::End(operation);
}

ErrorCode ImageBuilderProxy::ReadApplicationManifest(
    wstring const & appManifestFile,
    __out ServiceModel::ApplicationManifestDescription & appManifestDescription)
{
    return appManifestDescription.FromXml(appManifestFile);
}

ErrorCode ImageBuilderProxy::ReadApplicationManifestContent(
    wstring const & appManifestFile,
    __out wstring & appManifestContent)
{
    auto error = XmlReader::ReadXmlFile(
        appManifestFile,
        *ServiceModel::SchemaNames::Element_ApplicationManifest,
        *ServiceModel::SchemaNames::Namespace,
        appManifestContent);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: XmlReader::ReadXmlFile failed for the file - {1} with error - {2}",
            NodeTraceComponent::TraceId,
            appManifestFile,
            error);
        
    }

    return error;
}

ErrorCode ImageBuilderProxy::ReadServiceManifestContent(
    wstring const & serviceManifestFile,
    __out wstring & serviceManifestContent)
{
   auto error = XmlReader::ReadXmlFile(
        serviceManifestFile,
        *ServiceModel::SchemaNames::Element_ServiceManifest,
        *ServiceModel::SchemaNames::Namespace,
        serviceManifestContent);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: XmlReader::ReadXmlFile failed for the file - {1} with error - {2}",
            NodeTraceComponent::TraceId,
            serviceManifestFile,
            error);
    }

    return error;
}

template<typename LayoutSpecification>
ErrorCode ImageBuilderProxy::ReadServiceManifests(
    ServiceModel::ApplicationManifestDescription const & appManifest,
    LayoutSpecification const & layout,
    __out vector<ServiceManifestDescription> & serviceManifestsResult)
{
    vector<ServiceManifestDescription> serviceManifests;

    for (auto it = appManifest.ServiceManifestImports.begin(); it != appManifest.ServiceManifestImports.end(); ++it)
    {
        wstring filename = GetServiceManifestFileName(layout, appManifest.Name, it->ServiceManifestRef.Name, it->ServiceManifestRef.Version);

        ServiceManifestDescription serviceManifest;
        auto error = serviceManifest.FromXml(filename);

        if (!error.IsSuccess())
        {
             return error;
        }

        serviceManifests.push_back(serviceManifest);
    }

    serviceManifestsResult.swap(serviceManifests);

    return ErrorCodeValue::Success;
}

ErrorCode ImageBuilderProxy::ReadApplication(
    wstring const & appFile,
    __out ServiceModel::ApplicationInstanceDescription & appDescription)
{
    return appDescription.FromXml(appFile);
}

ErrorCode ImageBuilderProxy::ReadPackages(
    ServiceModel::ApplicationInstanceDescription const & appDescription,
    StoreLayoutSpecification const & layout,
    __out vector<ServicePackageDescription> & packagesResult)
{
    vector<ServicePackageDescription> packageDescriptionsTemp;

    for (auto iter = appDescription.ServicePackageReferences.begin();
        iter != appDescription.ServicePackageReferences.end();
        ++iter)
    {
        ServicePackageReference const & packageReference = *iter;

        auto packageFile = layout.GetServicePackageFile(
            appDescription.ApplicationTypeName,
            appDescription.ApplicationId,
            packageReference.Name, 
            packageReference.RolloutVersionValue.ToString());

        ServicePackageDescription packageDescription;
        auto error = packageDescription.FromXml(packageFile);

        if (!error.IsSuccess())
        {
            return error;
        }

        packageDescriptionsTemp.push_back(move(packageDescription));
    }

    packagesResult.swap(packageDescriptionsTemp);

    return ErrorCodeValue::Success;
}

template<typename LayoutSpecification>
ErrorCode ImageBuilderProxy::ParseApplicationManifest(
    ApplicationManifestDescription const & appManifest,
    LayoutSpecification const & layout,
    __out vector<ServiceModelServiceManifestDescription> & serviceManifestsResult)
{
    vector<ServiceModelServiceManifestDescription> serviceModelServiceManifests;
    vector<ServiceManifestDescription> serviceManifests;
    wstring serviceManifestContent;
    wstring serviceManifestPath;
    ErrorCode error = ReadServiceManifests(appManifest, layout, serviceManifests);
    if (error.IsSuccess())
    {
        for (auto it = serviceManifests.begin(); it != serviceManifests.end(); ++it)
        {
            ServiceModelServiceManifestDescription manifestDescription(it->Name, it->Version);

            for (auto it2 = it->ServiceTypeNames.begin(); it2 != it->ServiceTypeNames.end(); ++it2)
            {
                if (it2->ServiceTypeName.size() > static_cast<size_t>(ManagementConfig::GetConfig().MaxServiceTypeNameLength))
                {
                    WriteWarning(
                        TraceComponent, 
                        "{0} service type name '{1}' is too long: length = {2} max = {3}", 
                        NodeTraceComponent::TraceId,
                        it2->ServiceTypeName,
                        it2->ServiceTypeName.size(),
                        ManagementConfig::GetConfig().MaxServiceTypeNameLength);

                    return ErrorCodeValue::EntryTooLarge;
                } 
            }

            // Read the ServiceManifest Content
            serviceManifestPath = GetServiceManifestFileName(layout, appManifest.Name, it->Name, it->Version);
            error = ReadServiceManifestContent(serviceManifestPath, serviceManifestContent);
            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent, 
                    "{0} Unable to read Service Manifest Content from path:{1}", 
                    NodeTraceComponent::TraceId,
                    serviceManifestPath);

                serviceManifestContent = L"";
            }

            manifestDescription.SetContent(move(serviceManifestContent));

            for (auto it2 = it->ServiceTypeNames.begin(); it2 != it->ServiceTypeNames.end(); ++it2)
            {
                manifestDescription.AddServiceModelType(*it2);
            }

            for (auto it2 = it->ServiceGroupTypes.begin(); it2 != it->ServiceGroupTypes.end(); ++it2)
            {
                manifestDescription.AddServiceGroupModelType(*it2);
            }

            for (auto it2 = it->CodePackages.begin(); it2 != it->CodePackages.end(); ++it2)
            {
                manifestDescription.AddCodePackage(it2->Name, it2->Version);
            }

            for (auto it2 = it->ConfigPackages.begin(); it2 != it->ConfigPackages.end(); ++it2)
            {
                manifestDescription.AddConfigPackage(it2->Name, it2->Version);
            }

            for (auto it2 = it->DataPackages.begin(); it2 != it->DataPackages.end(); ++it2)
            {
                manifestDescription.AddDataPackage(it2->Name, it2->Version);
            }

            serviceModelServiceManifests.push_back(manifestDescription);
        }

        serviceManifestsResult.swap(serviceModelServiceManifests);
    }

    return error;
}
    
ErrorCode ImageBuilderProxy::ParseApplication(
    NamingUri const & appName,
    ServiceModel::ApplicationInstanceDescription const & appDescription,
    StoreLayoutSpecification const & layout,
    __out DigestedApplicationDescription & applicationResult)
{
    vector<StoreDataServicePackage> packagesResult;
    DigestedApplicationDescription::CodePackageDescriptionMap codePackagesResult;
    map<ServiceModelTypeName, ServiceTypeDescription> typeDescriptions;
    map<ServicePackageIdentifier, ServicePackageResourceGovernanceDescription> servicePackageRGResult;
    vector<wstring> networksResult;

    ErrorCode error = ParseServicePackages(
        appName, 
        appDescription, 
        layout, 
        /*out*/ packagesResult, 
        /*out*/ codePackagesResult, 
        /*out*/ typeDescriptions,
        /*out*/ servicePackageRGResult,
        /*out*/ networksResult);

    vector<StoreDataServiceTemplate> templatesResult;
    if (error.IsSuccess())
    {
        error = ParseServiceTemplates(appName, appDescription, typeDescriptions, /*out*/ templatesResult);
    } 

    vector<Naming::PartitionedServiceDescriptor> defaultServicesResult;
    if (error.IsSuccess())
    {
        error = ParseDefaultServices(appName, appDescription, typeDescriptions, /*out*/ defaultServicesResult);
    }

    if (error.IsSuccess())
    {
        applicationResult = DigestedApplicationDescription(
            appDescription,
            packagesResult,
            codePackagesResult,
            templatesResult,
            defaultServicesResult,
            servicePackageRGResult,
            networksResult);
    }

    return error;
}

ErrorCode ImageBuilderProxy::ParseServicePackages(
    NamingUri const & appName,
    ServiceModel::ApplicationInstanceDescription const & appDescription,
    StoreLayoutSpecification const & layout,
    __out vector<StoreDataServicePackage> & packagesResult,
    __out DigestedApplicationDescription::CodePackageDescriptionMap & codePackagesResult,
    __out map<ServiceModelTypeName, ServiceModel::ServiceTypeDescription> & typeDescriptionsResult,
    __out ServiceModel::ServicePackageResourceGovernanceMap & servicePackageRGResult,
    __out vector<wstring> & networksResult)
{
    vector<StoreDataServicePackage> tempPackagesResult;
    DigestedApplicationDescription::CodePackageDescriptionMap tempCodePackagesResult;
    map<ServiceModelTypeName, ServiceTypeDescription> tempTypeDescriptions;
    map<ServicePackageIdentifier, ServicePackageResourceGovernanceDescription> tempServicePackageRGDescriptions;
    set<wstring> tempNetworkSet;
    vector<wstring> tempNetworksResult;

    vector<ServicePackageDescription> packageDescriptions;
    ErrorCode error = ReadPackages(appDescription, layout, /*out*/ packageDescriptions);

    if (error.IsSuccess())
    {
        for (auto iter = packageDescriptions.begin();
            iter != packageDescriptions.end();
            ++iter)
        {
            ServicePackageDescription const & packageDescription = *iter;
            tempServicePackageRGDescriptions.insert(make_pair(ServicePackageIdentifier(appDescription.ApplicationId, packageDescription.PackageName), packageDescription.ResourceGovernanceDescription));

            for (auto iter2 = packageDescription.DigestedServiceTypes.ServiceTypes.begin();
                iter2 != packageDescription.DigestedServiceTypes.ServiceTypes.end();
                ++iter2)
            {
                ServiceModelTypeName serviceTypeName(iter2->ServiceTypeName);

                ServicePackageVersion servicePackageVersion(
                    ApplicationVersion(appDescription.ApplicationPackageReference.RolloutVersionValue),
                    iter->RolloutVersionValue);

                tempPackagesResult.push_back(StoreDataServicePackage(
                    ServiceModelApplicationId(appDescription.ApplicationId),
                    appName,
                    serviceTypeName,
                    ServiceModelPackageName(packageDescription.PackageName),
                    ServiceModelVersion(servicePackageVersion.ToString()),
                    *iter2));

                tempTypeDescriptions[serviceTypeName] = *iter2;
            }

            tempCodePackagesResult[ServiceModelPackageName(packageDescription.PackageName)] = packageDescription.DigestedCodePackages;

            for (auto iter3 = packageDescription.NetworkPolicies.ContainerNetworkPolicies.begin();
                iter3 != packageDescription.NetworkPolicies.ContainerNetworkPolicies.end();
                ++iter3)
            {
                auto iter4 = tempNetworkSet.find(iter3->NetworkRef);
                if (iter4 == tempNetworkSet.end())
                {
                    tempNetworkSet.insert(iter3->NetworkRef);
                    tempNetworksResult.push_back(iter3->NetworkRef);
                }
            }
        }

        packagesResult.swap(tempPackagesResult);
        codePackagesResult.swap(tempCodePackagesResult);
        typeDescriptionsResult.swap(tempTypeDescriptions);
        servicePackageRGResult.swap(tempServicePackageRGDescriptions);
        networksResult.swap(tempNetworksResult);
    }

    return error;
}

ErrorCode ImageBuilderProxy::ParseServiceTemplates(
    NamingUri const & appName,
    ServiceModel::ApplicationInstanceDescription const & appDescription,
    map<ServiceModelTypeName, ServiceModel::ServiceTypeDescription> const & typeDescriptions,
    __out vector<StoreDataServiceTemplate> & templatesResult)
{
    ErrorCode error(ErrorCodeValue::Success);

    vector<StoreDataServiceTemplate> tempTemplatesResult;

    for (auto iter = appDescription.ServiceTemplates.begin();
        iter != appDescription.ServiceTemplates.end();
        ++iter)
    {
        ApplicationServiceDescription const & description = *iter;

        ServiceModelTypeName serviceTypeName(description.ServiceTypeName);

        auto iter2 = typeDescriptions.find(serviceTypeName);
        if (iter2 == typeDescriptions.end())
        {
            WriteWarning(
                TraceComponent, 
                "{0} could not find service type '{1}' for template",
                NodeTraceComponent::TraceId,
                serviceTypeName);

            error = ErrorCodeValue::ServiceTypeNotFound;
            break;
        }

        ServiceTypeDescription const & typeDescription = iter2->second;

        vector<Reliability::ServiceCorrelationDescription> correlations;
        for (auto iterCorr = description.ServiceCorrelations.begin(); iterCorr != description.ServiceCorrelations.end(); ++iterCorr)
        {
            correlations.push_back(Reliability::ServiceCorrelationDescription(iterCorr->ServiceName, ServiceCorrelationScheme::ToPublicApi(iterCorr->Scheme)));
        }

        vector<ServiceLoadMetricDescription> const & sourceLoadMetric = description.LoadMetrics.size() > 0 ? description.LoadMetrics : typeDescription.LoadMetrics;

        vector<Reliability::ServiceLoadMetricDescription> loadMetrics;
        for (auto iter3 = sourceLoadMetric.begin(); iter3 != sourceLoadMetric.end(); ++iter3)
        {
            loadMetrics.push_back(Reliability::ServiceLoadMetricDescription(
                iter3->Name, 
                WeightType::ToPublicApi(iter3->Weight), 
                iter3->PrimaryDefaultLoad, 
                iter3->SecondaryDefaultLoad));
        }

        wstring placementConstraints = description.IsPlacementConstraintsSpecified ? description.PlacementConstraints : typeDescription.PlacementConstraints;

        vector<ServicePlacementPolicyDescription> const & placementPolicies = 
            description.ServicePlacementPolicies.size() > 0 ? description.ServicePlacementPolicies : typeDescription.ServicePlacementPolicies;

        FABRIC_MOVE_COST defaultMoveCost = description.IsDefaultMoveCostSpecified ? static_cast<FABRIC_MOVE_COST>(description.DefaultMoveCost) : FABRIC_MOVE_COST_LOW;

        auto scalingPolicies = description.ScalingPolicies;


        Reliability::ServiceDescription serviceDescription(
            L"", // service name will be replaced when applying template
            0, // instance
            0, // update version
            description.Partition.PartitionCount,
            (description.IsStateful ? description.TargetReplicaSetSize : description.InstanceCount),
            description.MinReplicaSetSize,
            description.IsStateful,
            typeDescription.HasPersistedState,
            description.ReplicaRestartWaitDuration,
            description.QuorumLossWaitDuration,
            description.StandByReplicaKeepDuration,
            ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName.Value), // qualification occurs when building a ServiceContext
            correlations,
            placementConstraints,
            0, // scaleout count
            loadMetrics,
            defaultMoveCost, // default move cost
            vector<byte>(), // initialization data,
            appName.ToString(),
            placementPolicies,
            ServicePackageActivationMode::SharedProcess,
            L"",
            scalingPolicies
            );

        // Trigger special handling for ServiceGroup Templates
        if (description.IsServiceGroup)
        {
            error = ParseServiceGroupTemplate(description, serviceDescription);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceComponent,
                    "Error while parsing service group template ServiceTypeName: {0}",
                    description.ServiceTypeName);

                return error;
            }
        }

        Naming::PartitionedServiceDescriptor partitionedDescriptor;
        switch (description.Partition.Scheme)
        {
            case PartitionSchemeDescription::Singleton:
                error = Naming::PartitionedServiceDescriptor::Create(
                    serviceDescription,
                    partitionedDescriptor);
                break;
            case PartitionSchemeDescription::UniformInt64Range:
                error = Naming::PartitionedServiceDescriptor::Create(
                    serviceDescription,
                    description.Partition.LowKey,
                    description.Partition.HighKey,
                    partitionedDescriptor);
                break;
            case PartitionSchemeDescription::Named:
                error = Naming::PartitionedServiceDescriptor::Create(
                    serviceDescription,
                    description.Partition.PartitionNames,
                    partitionedDescriptor);
                break;
            default:
                WriteWarning(
                    TraceComponent, 
                    "{0} Unknown PartitionSchemeDescription value {1}",
                    NodeTraceComponent::TraceId,
                    static_cast<int>(description.Partition.Scheme));
                return ErrorCodeValue::OperationFailed;
        }

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} invalid service type template '{1}': {2}",
                NodeTraceComponent::TraceId,
                serviceTypeName,
                error);
            return ErrorCodeValue::OperationFailed;
        }

        StoreDataServiceTemplate serviceTemplate(
            ServiceModelApplicationId(appDescription.ApplicationId),
            appName,
            serviceTypeName,
            move(partitionedDescriptor));

        tempTemplatesResult.push_back(serviceTemplate);
    }

    if (error.IsSuccess())
    {
        templatesResult.swap(tempTemplatesResult);
    }

    return error;
}

ErrorCode ImageBuilderProxy::ParseServiceGroupTemplate(
    ServiceModel::ApplicationServiceDescription const & description,
    Reliability::ServiceDescription & serviceDescription)
{
    serviceDescription.IsServiceGroup = true;

    CServiceGroupDescription cServiceGroupDescription;

    if (! description.IsStateful)
    {
        cServiceGroupDescription.HasPersistedState = FALSE;
    }
    else
    {
        cServiceGroupDescription.HasPersistedState = TRUE;
    }

    cServiceGroupDescription.ServiceGroupMemberData.resize(description.ServiceGroupMembers.size());
    for (int i = 0; i < description.ServiceGroupMembers.size(); i++)
    {
        CServiceGroupMemberDescription & cServiceGroupMemberDescription = cServiceGroupDescription.ServiceGroupMemberData[i];
        cServiceGroupMemberDescription.Identifier = Common::Guid::NewGuid().AsGUID();
        if (! description.IsStateful)
        {
            cServiceGroupMemberDescription.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        }
        else
        {
            cServiceGroupMemberDescription.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        }
        cServiceGroupMemberDescription.ServiceType = description.ServiceGroupMembers[i].ServiceTypeName;
        cServiceGroupMemberDescription.ServiceName = description.ServiceGroupMembers[i].MemberName;

        for (int j = 0; j < description.ServiceGroupMembers[i].LoadMetrics.size(); j++)
        {
            CServiceGroupMemberLoadMetricDescription cServiceGroupMemberLoadMetricDescription;
            cServiceGroupMemberLoadMetricDescription.Name = description.ServiceGroupMembers[i].LoadMetrics[j].Name;
            cServiceGroupMemberLoadMetricDescription.Weight = WeightType::ToPublicApi(description.ServiceGroupMembers[i].LoadMetrics[j].Weight);
            cServiceGroupMemberLoadMetricDescription.PrimaryDefaultLoad = description.ServiceGroupMembers[i].LoadMetrics[j].PrimaryDefaultLoad;
            cServiceGroupMemberLoadMetricDescription.SecondaryDefaultLoad = description.ServiceGroupMembers[i].LoadMetrics[j].SecondaryDefaultLoad;

            cServiceGroupMemberDescription.Metrics.push_back(cServiceGroupMemberLoadMetricDescription);
        }
    }

    vector<byte> serializedInitializationData;
    ErrorCode errorCode = Common::FabricSerializer::Serialize(&cServiceGroupDescription, serializedInitializationData);
    if (!errorCode.IsSuccess())
    {
        WriteError(
            TraceComponent,
            "Error while serializing service group object into initialization data");

        return errorCode;
    }
    serviceDescription.put_InitializationData(move(serializedInitializationData));

    return errorCode;
}

ErrorCode ImageBuilderProxy::ParseDefaultServices(
    NamingUri const & appName,
    ServiceModel::ApplicationInstanceDescription const & appDescription,
    map<ServiceModelTypeName, ServiceModel::ServiceTypeDescription> const & typeDescriptions,
    __out vector<Naming::PartitionedServiceDescriptor> & defaultServicesResult)
{
    ErrorCode error(ErrorCodeValue::Success);

    vector<Naming::PartitionedServiceDescriptor> tempDefaultServicesResult;

    for (auto iter = appDescription.DefaultServices.begin();
        iter != appDescription.DefaultServices.end();
        ++iter)
    {
        ApplicationServiceDescription const & description = iter->DefaultService;

        NamingUri serviceName;
        if (!appName.TryCombine(iter->Name, serviceName))
        {
            WriteWarning(
                TraceComponent, 
                "{0} could not combine '{1}' and '{2}' as a Naming Uri",
                NodeTraceComponent::TraceId,
                appName,
                iter->Name);

            error = ErrorCodeValue::InvalidNameUri;
            break;
        }

		if (!(iter->ServiceDnsName.empty()))
		{
			if (!ClusterManagerReplica::IsDnsServiceEnabled())
			{
				WriteWarning(
					TraceComponent,
					"{0} DNS name requested '{1}' but DNS feature is disabled. AppName=[{2}], DefaultServiceName=[{3}].",
					NodeTraceComponent::TraceId,
					iter->ServiceDnsName,
					appName,
					iter->Name);

				error = ErrorCodeValue::DnsServiceNotFound;
				break;
			}

			error = ClusterManagerReplica::ValidateServiceDnsName(iter->ServiceDnsName);
			if (!error.IsSuccess())
			{
				WriteWarning(
					TraceComponent,
					"{0} Service DNS name '{1}' is not valid for AppName=[{2}], DefaultServiceName=[{3}]. Error=[{4}]",
					NodeTraceComponent::TraceId,
					iter->ServiceDnsName,
					appName,
					iter->Name,
					error);

				break;
			}
		}

        ServiceModelTypeName serviceTypeName(description.ServiceTypeName);

        auto iter2 = typeDescriptions.find(serviceTypeName);
        if (iter2 == typeDescriptions.end())
        {
            WriteWarning(
                TraceComponent, 
                "{0} could not find service type '{1}' for required service",
                NodeTraceComponent::TraceId,
                serviceTypeName);

            error = ErrorCodeValue::ServiceTypeNotFound;
            break;
        }

        ServiceTypeDescription const & typeDescription = iter2->second;

        vector<Reliability::ServiceCorrelationDescription> correlations;
        for (auto iterCorr = description.ServiceCorrelations.begin(); iterCorr != description.ServiceCorrelations.end(); ++iterCorr)
        {
            correlations.push_back(Reliability::ServiceCorrelationDescription(iterCorr->ServiceName, ServiceCorrelationScheme::ToPublicApi(iterCorr->Scheme)));
        }

        vector<byte> initializationData;

        vector<ServiceLoadMetricDescription> memberLoadMetrics;
        
        if (description.IsServiceGroup)
        {
            auto defaultServiceGroupError = ParseDefaultServiceGroup(
                typeDescription,
                description,
                serviceName,
                memberLoadMetrics,
                initializationData);

            if (!defaultServiceGroupError.IsSuccess())
            {
                return defaultServiceGroupError;
            }
        }

        vector<ServiceLoadMetricDescription> const & sourceLoadMetric = 
            description.LoadMetrics.size() > 0 ? description.LoadMetrics : // service metrics take precedence over
            (memberLoadMetrics.size() > 0 ? memberLoadMetrics :            // member metrics take precedence over
            typeDescription.LoadMetrics);                                  // service type metrics

        vector<Reliability::ServiceLoadMetricDescription> loadMetrics;   

        for (auto iter3 = sourceLoadMetric.begin(); iter3 != sourceLoadMetric.end(); ++iter3)
        {
            loadMetrics.push_back(Reliability::ServiceLoadMetricDescription(
                iter3->Name, 
                WeightType::ToPublicApi(iter3->Weight), 
                iter3->PrimaryDefaultLoad,
                iter3->SecondaryDefaultLoad));
        }
        
        wstring placementConstraints = description.IsPlacementConstraintsSpecified ? description.PlacementConstraints : typeDescription.PlacementConstraints;

        vector<ServicePlacementPolicyDescription> const & placementPolicies = 
            description.ServicePlacementPolicies.size() > 0 ? description.ServicePlacementPolicies : typeDescription.ServicePlacementPolicies;

        // (RDBug 3274985 - application upgrade will check this value for default services so we have to keep it as it was prior to 3.1 - zero, unless user specified it).
        FABRIC_MOVE_COST defaultMoveCost = description.IsDefaultMoveCostSpecified ? static_cast<FABRIC_MOVE_COST>(description.DefaultMoveCost) : FABRIC_MOVE_COST_ZERO;

        auto scalingPolicies = description.ScalingPolicies;

        WriteNoise(
            TraceComponent,
            "{0} ParseDefaultServices(): ServicePackageActivationMode=[{1}], ServiceDnsName=[{2}].",
            NodeTraceComponent::TraceId,
            iter->PackageActivationMode,
			iter->ServiceDnsName);

        Reliability::ServiceDescription serviceDescription(
            serviceName.ToString(),
            0, // instance
            0, // update version
            description.Partition.PartitionCount,
            (description.IsStateful ? description.TargetReplicaSetSize : description.InstanceCount),
            description.MinReplicaSetSize,
            description.IsStateful,
            typeDescription.HasPersistedState,
            description.ReplicaRestartWaitDuration,
            description.QuorumLossWaitDuration,
            description.StandByReplicaKeepDuration,
            ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName.Value),   // qualification occurs when building a ServiceContext
            correlations,
            placementConstraints,
            0, // scaleout count
            loadMetrics,
            defaultMoveCost, 
            initializationData,
            appName.ToString(),
            placementPolicies, // Service placement policies
            iter->PackageActivationMode,
			iter->ServiceDnsName,
            scalingPolicies);

        Naming::PartitionedServiceDescriptor partitionedDescriptor;
        switch (description.Partition.Scheme)
        {
            case PartitionSchemeDescription::Singleton:
                error = Naming::PartitionedServiceDescriptor::Create(
                    serviceDescription,
                    partitionedDescriptor);
                break;
            case PartitionSchemeDescription::UniformInt64Range:
                error = Naming::PartitionedServiceDescriptor::Create(
                    serviceDescription,
                    description.Partition.LowKey,
                    description.Partition.HighKey,
                    partitionedDescriptor);
                break;
            case PartitionSchemeDescription::Named:
                error = Naming::PartitionedServiceDescriptor::Create(
                    serviceDescription,
                    description.Partition.PartitionNames,
                    partitionedDescriptor);
                break;
            default:
                 WriteWarning(
                    TraceComponent, 
                    "{0} Unknown PartitionSchemeDescription value {1}",
                    NodeTraceComponent::TraceId,
                    serviceTypeName,
                    static_cast<int>(description.Partition.Scheme));
                return ErrorCodeValue::OperationFailed;
        }

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} invalid default service '{1}': {2}",
                NodeTraceComponent::TraceId,
                serviceName,
                error);
            return ErrorCodeValue::OperationFailed;
        }

        partitionedDescriptor.IsServiceGroup = description.IsServiceGroup;

        WriteNoise(
            TraceComponent,
            "{0} ParseDefaultServices(): PSD={1}.",
            NodeTraceComponent::TraceId,
            partitionedDescriptor);

        tempDefaultServicesResult.push_back(move(partitionedDescriptor));        
    }

    if (error.IsSuccess())
    {
        defaultServicesResult.swap(tempDefaultServicesResult);
    }

    return error;
}

ErrorCode ImageBuilderProxy::ParseDefaultServiceGroup(
    __in ServiceModel::ServiceTypeDescription const & typeDescription,
    __in ServiceModel::ApplicationServiceDescription const & serviceDescription,
    __in Common::NamingUri const & serviceName,
    __out vector<ServiceModel::ServiceLoadMetricDescription> & memberLoadMetrics,
    __out vector<byte> & initData)
{
    CServiceGroupDescription serviceGroupDescription;
    serviceGroupDescription.HasPersistedState = typeDescription.HasPersistedState;

    map<wstring, ServiceLoadMetricDescription> memberMetricMap;
    for (auto member = begin(serviceDescription.ServiceGroupMembers); member != end(serviceDescription.ServiceGroupMembers); ++member)
    {
        CServiceGroupMemberDescription serviceGroupMemberDescription;
        serviceGroupMemberDescription.ServiceType = member->ServiceTypeName;
        serviceGroupMemberDescription.ServiceDescriptionType = typeDescription.IsStateful ? FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL : FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        serviceGroupMemberDescription.Identifier = Guid::NewGuid().AsGUID();

        // create the FQ member name by adding the member name as fragment to the service name
        NamingUri memberName;
        if (!NamingUri::TryParse(
                Uri(serviceName.Scheme, serviceName.Authority, serviceName.Path, serviceName.Query, member->MemberName).ToString(), 
                memberName)
            )
        {
            WriteWarning(
                TraceComponent, 
                "{0} could not combine '{1}' and '{2}' as a Naming Uri",
                NodeTraceComponent::TraceId,
                serviceName,
                member->MemberName);

            return ErrorCodeValue::InvalidNameUri;
        }

        serviceGroupMemberDescription.ServiceName = memberName.ToString();

        // aggregate service member load metrics
        for (auto metric = begin(member->LoadMetrics); metric != end(member->LoadMetrics); ++metric)
        {
            auto hasMemberMetric = memberMetricMap.find(metric->Name);
            if (hasMemberMetric != end(memberMetricMap))
            {
                if ((hasMemberMetric->second.PrimaryDefaultLoad > ~(metric->PrimaryDefaultLoad)) ||
                    (hasMemberMetric->second.SecondaryDefaultLoad > ~(metric->SecondaryDefaultLoad)))
                {
                    WriteWarning(
                        TraceComponent, 
                        "{0} could not combine default load for metric '{2}' in service group '{1}'. Result would overflow.",
                        NodeTraceComponent::TraceId,
                        serviceName,
                        metric->Name);

                    return ErrorCode(ErrorCodeValue::InvalidArgument);
                }

                hasMemberMetric->second.PrimaryDefaultLoad += metric->PrimaryDefaultLoad;
                hasMemberMetric->second.SecondaryDefaultLoad += metric->SecondaryDefaultLoad;
                hasMemberMetric->second.Weight = max(hasMemberMetric->second.Weight, metric->Weight);
            }
            else
            {
                memberMetricMap.insert(make_pair(metric->Name, *metric));
            }

            CServiceGroupMemberLoadMetricDescription serviceGroupMemberLoadMetricDescription;

            serviceGroupMemberLoadMetricDescription.Name = metric->Name;
            serviceGroupMemberLoadMetricDescription.Weight = WeightType::ToPublicApi(metric->Weight);
            serviceGroupMemberLoadMetricDescription.PrimaryDefaultLoad = metric->PrimaryDefaultLoad;
            serviceGroupMemberLoadMetricDescription.SecondaryDefaultLoad = metric->SecondaryDefaultLoad;

            serviceGroupMemberDescription.Metrics.push_back(move(serviceGroupMemberLoadMetricDescription));
        }

        // add member to service group description
        serviceGroupDescription.ServiceGroupMemberData.push_back(move(serviceGroupMemberDescription));
    }

    // copy the aggregated load metrics
    for (auto namedMetric = begin(memberMetricMap); namedMetric != end(memberMetricMap); ++namedMetric)
    {
        memberLoadMetrics.push_back(move(namedMetric->second));
    }

    // the service group description becomes the init data of the service
    return FabricSerializer::Serialize(&serviceGroupDescription, initData);
}

ErrorCode ImageBuilderProxy::ParseFabricVersion(
    wstring const & code, 
    wstring const & config,
    __out FabricVersion & result)
{
    ErrorCode error(ErrorCodeValue::Success);

    FabricCodeVersion codeVersion(FabricCodeVersion::Invalid);
    if (!code.empty())
    {
        error = FabricCodeVersion::FromString(code, codeVersion);
    }

    if (error.IsSuccess())
    {
        Common::FabricConfigVersion configVersion;
        if (!config.empty())
        {
            error = configVersion.FromString(config);
        }

        if (error.IsSuccess())
        {
            result = FabricVersion(codeVersion, configVersion);
        }
    }

    return error;
}

std::wstring ImageBuilderProxy::GetOutputDirectoryPath(Common::ActivityId const & activityId)
{
    return Path::Combine(appTypeOutputBaseDirectory_, activityId.Guid.ToString());
}

ErrorCode ImageBuilderProxy::CreateOutputDirectory(wstring const & directory)
{
    if (Directory::Exists(directory))
    {
        WriteNoise(
            TraceComponent, 
            "{0} directory '{1}' already exists",
            NodeTraceComponent::TraceId,
            directory);
        return ErrorCodeValue::Success;
    }

    ErrorCode error = Directory::Create2(directory);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "{0} could not create ImageBuilder output directory '{1}' due to {2}",
            NodeTraceComponent::TraceId,
            directory,
            error);
    }

    return error;
}

void ImageBuilderProxy::DeleteDirectory(wstring const & directory)
{
    if (!Directory::Exists(directory))
    {
        WriteNoise(
            TraceComponent, 
            "{0} directory '{1}' not found - skipping delete",
            NodeTraceComponent::TraceId,
            directory);
        return;
    }

    ErrorCode error = Directory::Delete(directory, true);

    // best effort clean-up (do not fail the operation even if clean-up fails)
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not cleanup ImageBuilder output directory '{1}' due to {2}",
            NodeTraceComponent::TraceId,
            directory,
            error);
    }
}

void ImageBuilderProxy::Test_DeleteOrArchiveDirectory(wstring const & directory, ErrorCode const & error)
{
    this->DeleteOrArchiveDirectory(directory, error);
}

void ImageBuilderProxy::DeleteOrArchiveDirectory(wstring const & directory, ErrorCode const & error)
{
    bool shouldArchive = error.IsError(ErrorCodeValue::ImageBuilderValidationError);
    bool archiveEnabled = !ManagementConfig::GetConfig().DisableImageBuilderDirectoryArchives;

    if (shouldArchive && archiveEnabled)
    {
        auto archiveRoot = Path::Combine(workingDir_, ArchiveDirectoryPrefix);

        if (!Directory::Exists(archiveRoot))
        {
            auto createError = Directory::Create2(archiveRoot);

            if (!createError.IsSuccess())
            {
                WriteWarning(
                    TraceComponent, 
                    "{0} failed to create archive root {1} for {2} on error {3}: createError={4}",
                    NodeTraceComponent::TraceId,
                    archiveRoot,
                    directory,
                    error,
                    createError);

                return;
            }
        }

        auto archiveDirectory = Path::Combine(archiveRoot, Path::GetFileName(directory));

        // Best-effort archival
        //
        auto archiveError = Directory::Rename(directory, archiveDirectory);

        if (archiveError.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} archived {1} -> {2} on error {3}",
                NodeTraceComponent::TraceId,
                directory,
                archiveDirectory,
                error);
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0} failed to archive {1} -> {2} on error={3}: archiveError={4}",
                NodeTraceComponent::TraceId,
                directory,
                archiveDirectory,
                error,
                archiveError);
        }
    }
    else
    {
        if (shouldArchive)
        {
            WriteInfo(
                TraceComponent, 
                "{0} skip archive of {1} on {2}: archiveEnabled={3}",
                NodeTraceComponent::TraceId,
                directory,
                error,
                archiveEnabled);
        }

        DeleteDirectory(directory);
    }
}

int64 ImageBuilderProxy::HandleToInt(HANDLE handle)
{
    return reinterpret_cast<int64>(handle);
}

wstring ImageBuilderProxy::GetServiceManifestFileName(
    BuildLayoutSpecification const &layout,
    wstring const &appTypeName,
    wstring const &serviceManName,
    wstring const &serviceManVersion)
{
    UNREFERENCED_PARAMETER(appTypeName);
    UNREFERENCED_PARAMETER(serviceManVersion);
    return layout.GetServiceManifestFile(serviceManName);
}

wstring ImageBuilderProxy::GetServiceManifestFileName(
    StoreLayoutSpecification const &layout,
    wstring const &appTypeName,
    wstring const &serviceManName,
    wstring const &serviceManVersion)
{
    return layout.GetServiceManifestFile(appTypeName, serviceManName, serviceManVersion);
}
