// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;
using namespace FabricInstallerService;

StringLiteral const TraceType("FabricUpgradeManager");

// ********************************************************************************************************************
// FabricUpgradeManager::OpenAsyncOperation Implementation
//
class FabricUpgradeManager::OpenAsyncOperation : public AsyncOperation
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        FabricUpgradeManager & fabricUpgradeManager,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperationParent)
        : AsyncOperation(callback, asyncOperationParent),
        fabricUpgradeManager_(fabricUpgradeManager),
        timeoutHelper_(timeout),
        fabricUpgradeContext_()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, FabricUpgradeContext & fabricUpgradeContext)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            fabricUpgradeContext = move(thisPtr->fabricUpgradeContext_);
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & operation)
    {
        auto error = InitializeUpgradeContext();
        TryComplete(operation, error);
    }

private:
    ErrorCode InitializeUpgradeContext()
    {
        auto error = FabricEnvironment::GetFabricDataRoot(fabricUpgradeContext_.fabricDataRoot);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Error getting FabricDataRoot. Error:{0}",
                error);

            return error;
        }

        error = FabricEnvironment::GetFabricLogRoot(fabricUpgradeContext_.fabricLogRoot);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Error getting FabricLogRoot. Error:{0}",
                error);

            return error;
        }

        error = FabricEnvironment::GetFabricCodePath(fabricUpgradeContext_.fabricCodePath);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Error getting FabricCodePath. Error:{0}",
                error);

            return error;
        }

        error = FabricEnvironment::GetFabricRoot(fabricUpgradeContext_.fabricRoot);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Error getting FabricRoot. Error:{0}",
                error);

            return error;
        }

        fabricUpgradeContext_.restartValidationFilePath = Path::Combine(fabricUpgradeContext_.fabricRoot, Constants::RestartValidationFileName);

        FabricDeploymentSpecification fabricDeploymentSpec(fabricUpgradeContext_.fabricDataRoot, fabricUpgradeContext_.fabricLogRoot);
        fabricUpgradeContext_.targetInformationFilePath = fabricDeploymentSpec.GetTargetInformationFile();
        if (!File::Exists(fabricUpgradeContext_.targetInformationFilePath))
        {
            WriteInfo(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Target Information file {0} not found, exiting ..",
                fabricUpgradeContext_.targetInformationFilePath);

            return ErrorCodeValue::NotFound;
        }

        error = fabricUpgradeContext_.targetInformationFileDescription.FromXml(fabricUpgradeContext_.targetInformationFilePath);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Error {0} reading target information from file {1}",
                error,
                fabricUpgradeContext_.targetInformationFilePath);

            return error;
        }

        return ErrorCodeValue::Success;
    }

private:
    FabricUpgradeContext fabricUpgradeContext_;
    FabricUpgradeManager & fabricUpgradeManager_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// FabricUpgradeManager::UpgradeAsyncOperation Implementation
//
class FabricUpgradeManager::UpgradeAsyncOperation : public AsyncOperation
{
    DENY_COPY(UpgradeAsyncOperation)

public:
    UpgradeAsyncOperation(
        FabricUpgradeContext const & fabricUpgradeContext,
        FabricUpgradeManager& fabricUpgradeManager,
        TimeSpan timeout,
        bool autoclean,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperationParent)
        : fabricUpgradeContext_(fabricUpgradeContext),
        fabricUpgradeManager_(fabricUpgradeManager),
        timeoutHelper_(timeout),
        upgradeTimeoutHelper_(timeout),
        autoclean_(autoclean),
        AsyncOperation(callback, asyncOperationParent)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<UpgradeAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & operation)
    {
        Threadpool::Post([this, operation]()
        {
            this->DoUpgrade(operation);
        });
    }

    void OnTimerCallback(AsyncOperationSPtr const & operation, wstring clientEndpoint, SecuritySettings securitySettings)
    {
        WriteNoise(
            TraceType,
            fabricUpgradeManager_.TraceId, 
            "OnTimerCallback invoked");
        wstring targetInformationFilePath = fabricUpgradeContext_.targetInformationFilePath;
        if (File::Exists(targetInformationFilePath))
        {
            if (this->upgradeTimeoutHelper_.IsExpired)
            {
                WriteError(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Target information file exists. This would indicate that Fabric node open or Fabric uninstall didn't happen successfully. Rolling back..");
                this->retryTimer_->Cancel();
                this->DoRollback(operation);
                return;
            }
            else
            {
                // Target information file exists but upgradeTimeout hasn't expired
                // Just return to check again on the next poll
                return;
            }
        }
        else
        {
            // No target information file is present and hence upgrade should be done
            WriteInfo(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Completing upgrade operation");
            this->retryTimer_->Cancel();

            if (autoclean_)
            {
                RunCleanup();
            }
            TryComplete(operation, ErrorCodeValue::Success);
        }
    }

private:
    void DoUpgrade(AsyncOperationSPtr const & operation)
    {
        WriteInfo(
            TraceType,
            fabricUpgradeManager_.TraceId,
            "Upgrade started on machine {0} with {1}",
            Environment::GetMachineName(),
            fabricUpgradeContext_.ToString());

        if (operation->IsCancelRequested)
        {
            TryComplete(operation, ErrorCodeValue::OperationCanceled);
            return;
        }

        if (timeoutHelper_.IsExpired)
        {
            TryComplete(operation, ErrorCodeValue::Timeout);
            return;
        }

        ErrorCode error(ErrorCodeValue::Success);
        RestartManagerWrapper upgradeRmWrapper;
        wstring seedNodeEndpoint;
        SecuritySettings securitySettings;

        // Check if this is a post-reboot run for validation
        bool isValidationRun = false;
        wstring const & validationFilePath = fabricUpgradeContext_.restartValidationFilePath;
        if (File::Exists(validationFilePath))
        {
            DateTime lastRebootTime = Environment::GetLastRebootTime();
            DateTime validationFileWriteTime;
            error = File::GetLastWriteTime(validationFilePath, validationFileWriteTime);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Validation file modified time query returned error {0}.",
                    error);
                TryComplete(operation, error);
                return;
            }

            if (lastRebootTime < validationFileWriteTime) // Reboot hasn't happened since validation file write
            {
                WriteWarning(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Reboot hasn't happened since validation file write. Rebooting.");
                Utility::RebootMachine();
                TryComplete(operation, ErrorCodeValue::RebootRequired);
                return;
            }

            // Recovery in case reboot didn't successfully move UT folder
            wstring utDir = Path::Combine(fabricUpgradeContext_.fabricRoot, Constants::UpgradeTempDirectoryName);
            if (Directory::Exists(utDir)) // MoveFileEx with MOVEFILE_DELAY_UNTIL_REBOOT didn't move all files as required
            {
                error = RecoverFabricFilesPostReboot(utDir, fabricUpgradeContext_.fabricRoot);
                if (!error.IsSuccess())
                {
                    WriteError(TraceType, fabricUpgradeManager_.TraceId, "UT dir recovery failed with {0}. Please manually delete {1}, {2} and redeploy machine.", error, utDir, validationFilePath);
                    TryComplete(operation, error);
                    return;
                }
            }

            // Validation
            WriteInfo(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Validating swapped versions based on file {0}.",
                validationFilePath);

            error = Utility::ValidateUpgrade(validationFilePath, fabricUpgradeContext_.fabricRoot);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Upgrade version validation failed with {0}",
                    error);

                File::Delete(validationFilePath);

                // Something unexpected happened
                WriteError(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Upgrade failed on validation. Rolling back.");
                this->DoRollback(operation);
                return;
            }

            File::Delete(validationFilePath);

            isValidationRun = true;
        }

        if (fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.IsValid || fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.IsValid)
        {
            // Originally this was only called if CurrentInstallation.IsValid, but it was realized that for some deployments with a reconfigured node, 
            // FabricHost was already running before first invocation of this service, so the TargetInformationFile only had a valid TargetInstallation element.
            // In order to make sure the subsequent operations are safe, FabricHost should be stopped.
            error = StopFabricHostService();
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Error {0} while trying to stop fabric host service",
                    error);

                // In typical deployment cases, CurrentInstallation will not be set, and FabricHostSvc won't be installed yet. 
                // In this case if an error is surfaced it should not persist a failure.
                if (fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.IsValid)
                {
                    TryComplete(operation, error);
                    return;
                }

                error.Reset();
            }
        }

        // Current Installation in the target information file could be missing e.g. for the first upgrade.
        // Perform uninstallation only if the current installaiton is valid.
        if (fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.IsValid)
        {
            if (!isValidationRun)
            {
                // If the current installation is MSI, then uninstall the MSI, else invoke the undoupgrade entry point
                if (isMSI(fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.MSILocation))
                {
                    WriteInfo(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "Uninstalling MSI {0}", fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.MSILocation);
                    wstring logFile = Path::Combine(fabricUpgradeContext_.fabricLogRoot, wformatString("MsiUninstall_FabricInstallerService_{0}.log", DateTime::Now().Ticks));
                    wstring uninstallMsiCommandLine = wformatString(Constants::MsiUninstallCommandLine,
                        fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.MSILocation,
                        logFile);

                    error = ProcessUtility::ExecuteCommandLine(uninstallMsiCommandLine);
                    if (error.IsWin32Error(ERROR_UNKNOWN_PRODUCT))
                    {
                        error = ErrorCodeValue::Success;
                    }

                    if (!error.IsSuccess())
                    {
                        WriteWarning(
                            TraceType,
                            fabricUpgradeManager_.TraceId,
                            "Error while uninstalling the current installed MSI. Execution of commandline {0} returned error:{1}.",
                            uninstallMsiCommandLine,
                            error);

                        // MSI uninstallation failed. If we ignore and continue, then the MSI will show up as installed in Add/Remove programs, which could be confusing.
                        // The best option is to start the fabric host service and fail the operation.
                        ServiceController::StartServiceByName(
                            FabricInstallerService::Constants::FabricHostServiceName, 
                            true, 
                            TimeSpan::FromMinutes(Constants::ServiceStartTimeoutInMinutes));
                        TryComplete(operation, error);
                        return;
                    }
                }
                else
                {
                    if (!fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.UndoUpgradeEntryPointExe.empty())
                    {
                        // Only close handles for upgrade. Cluster removal will sort itself out.
                        if (fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.IsValid)
                        {
                            error = CloseFabricProcesses(upgradeRmWrapper, fabricUpgradeContext_.fabricRoot);
                            if (!error.IsSuccess())
                            {
                                WriteWarning(
                                    TraceType,
                                    fabricUpgradeManager_.TraceId,
                                    "Error {0} while closing Fabric asset-holding processes before Uninstall (Best-effort).",
                                    error);
                                error.Reset();
                            }
                        }

                        WriteInfo(
                            TraceType,
                            fabricUpgradeManager_.TraceId,
                            "Invoking fabric setup to undo the current installation");
                        wstring currentUndoUpgradeEntryPointExeFullPath = Path::Combine(
                            fabricUpgradeContext_.fabricCodePath,
                            fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.UndoUpgradeEntryPointExe);

                        wstring undoUpgradeCommandLine = wformatString("{0} {1}",
                            currentUndoUpgradeEntryPointExeFullPath,
                            fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.UndoUpgradeEntryPointExeParameters);
                        WriteInfo(
                            TraceType,
                            fabricUpgradeManager_.TraceId,
                            "UndoUpgrade executing: {0}",
                            undoUpgradeCommandLine);

                        error = ProcessUtility::ExecuteCommandLine(undoUpgradeCommandLine);
                        if (!error.IsSuccess())
                        {
                            WriteWarning(
                                TraceType,
                                fabricUpgradeManager_.TraceId,
                                "Error while uninstalling the current package. Execution of commandline {0} returned error:{1}.",
                                undoUpgradeCommandLine,
                                error);

                            // Ignoring this error here is okay, since installation will be able to handle this.
                        }
                    }
                    else
                    {
                        WriteWarning(
                            TraceType,
                            fabricUpgradeManager_.TraceId,
                            "Current installation was not MSI but UndoUpgradeEntryPointExe was empty.");
                    }
                }
            }
        }

        // If the target installation is valid
        if (fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.IsValid)
        {
            // If the target installation is MSI, then install the MSI, else invoke the upgrade entry point
            if (isMSI(fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.MSILocation))
            {
                error = DeleteFabricRootFiles(fabricUpgradeContext_.fabricRoot, upgradeRmWrapper);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "DeleteFabricRootFiles ran into a conflict cleaning FabricRoot. Force rebooting to clear handles.",
                        error);
                    Utility::RebootMachine();
                    TryComplete(operation, ErrorCodeValue::RebootRequired);
                    return;
                }

                WriteInfo(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Installing target MSI {0}", fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.MSILocation);
                wstring logFile = Path::Combine(fabricUpgradeContext_.fabricLogRoot, wformatString("MsiInstall_FabricInstallerService_{0}.log", DateTime::Now().Ticks));
                wstring installMsiCommandLine = wformatString(Constants::MsiInstallCommandLine,
                    fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.MSILocation,
                    logFile);

                error = ProcessUtility::ExecuteCommandLine(installMsiCommandLine);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "Error while installing the target MSI. Execution of commandline {0} returned error:{1}.",
                        installMsiCommandLine,
                        error);

                    this->DoRollback(operation);
                    return;
                }
            }
            else
            {
                // Paths: 
                // 1. CAB deployment for Server, CAB will be extracted
                // 2. Directory deployment for Azure, where by this point files are already dropped at fabricRoot, 
                //      and TargetInstallation.MSILocation is unspecified
                // 3. CAB Upgrade (basic), with CAB extract
                // 4. CAB Upgrade post-reboot after validating placement of new files (no CAB extract).

                bool usingCabInstallation = false;
                if (isCAB(fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.MSILocation) && !isValidationRun)
                {
                    usingCabInstallation = true;

                    // Extract package contents related to fabric installation
                    error = InstallFabricPackage(
                        fabricUpgradeContext_.fabricRoot,
                        fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.MSILocation,
                        upgradeRmWrapper);
                    
                    if (error.IsError(ErrorCodeValue::RebootRequired))
                    {
                        WriteWarning(
                            TraceType,
                            fabricUpgradeManager_.TraceId,
                            "Fabric bits require a reboot to be swapped. Post-reboot moves have been arranged.",
                            error);
                        Utility::RebootMachine();
                        TryComplete(operation, error);
                        return;
                    }
                    
                    if (!error.IsSuccess())
                    {
                        WriteWarning(
                            TraceType,
                            fabricUpgradeManager_.TraceId,
                            "Error {0} while extracting the target package from {1} to {2}. Rolling back..",
                            error,
                            fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.MSILocation,
                            fabricUpgradeContext_.fabricRoot);

                        this->DoRollback(operation);
                        return;
                    }
                }

                // Call upgrade on the target installation
                if (!fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.UpgradeEntryPointExe.empty())
                {
                    WriteInfo(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "Calling upgrade entry point");
                    wstring targetUpgradeEntryPointExeFullPath = Path::Combine(
                        fabricUpgradeContext_.fabricCodePath,
                        fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.UpgradeEntryPointExe);

                    wstring upgradeCommandLine = wformatString("{0} {1}",
                        targetUpgradeEntryPointExeFullPath,
                        fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.UpgradeEntryPointExeParameters);
                    WriteInfo(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "Upgrade executing: {0}",
                        upgradeCommandLine);

                    do
                    {
                        error = ProcessUtility::ExecuteCommandLine(upgradeCommandLine);
                        if (!error.IsSuccess())
                        {
                            WriteWarning(
                                TraceType,
                                fabricUpgradeManager_.TraceId,
                                "Error while installing the target package. Execution of commandline {0} returned error: {1}. Retrying if timeout is not hit..",
                                upgradeCommandLine,
                                error);
                            ::Sleep(5000);
                        }
                    } while (!error.IsSuccess() && !timeoutHelper_.IsExpired);

                    if (!error.IsSuccess() && timeoutHelper_.IsExpired)
                    {
                        WriteWarning(
                            TraceType,
                            fabricUpgradeManager_.TraceId,
                            "Upgrade commandline \"{0}\" failed with error {1}, and timeout for retry was hit. Rolling back..",
                            upgradeCommandLine,
                            error);

                        this->DoRollback(operation);
                        return;
                    }
                }
            }

            // Restart stopped processes
            RestartFabricProcesses(upgradeRmWrapper);

            // Start FabricHost Service
            WriteInfo(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "starting fabric host service");
            error = ServiceController::StartServiceByName(
                FabricInstallerService::Constants::FabricHostServiceName, 
                true, 
                TimeSpan::FromMinutes(Constants::ServiceStartTimeoutInMinutes));
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Error {0} while waiting fabric host service to start. Rolling back..",
                    error);

                // Fabric Host failed to start, rollback
                this->DoRollback(operation);
                return;
            }
        }

        // when fabric node opens in the target version, target information file will be deleted.
        // If it does not, then that would mean something wrong in the new fabric.exe, So a rollback will be performed.
        WriteInfo(
            TraceType,
            fabricUpgradeManager_.TraceId,
            "Waiting for the upgrade or FabricSetup to complete, at which time target information file is expected to get deleted");

        upgradeTimeoutHelper_.SetRemainingTime(TimeSpan::FromSeconds(Constants::FabricNodeUpgradeCompleteTimeoutInSeconds));
        retryTimer_ = Timer::Create(TimerTagDefault, [this, operation, seedNodeEndpoint, securitySettings](Common::TimerSPtr const &)
        { 
            this->OnTimerCallback(operation, seedNodeEndpoint, securitySettings);
        }, false);
        retryTimer_->Change(TimeSpan::FromSeconds(0), TimeSpan::FromSeconds(Constants::FabricNodeUpgradeCompletePollIntervalInSeconds));
    }

    void DoRollback(AsyncOperationSPtr const & operation)
    {
        WriteInfo(
            TraceType,
            fabricUpgradeManager_.TraceId,
            "Rollback started with {0}", fabricUpgradeContext_.ToString());

        // If the current installation is empty, then a rollback cannot be performed. Hence fail
        if (!fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.IsValid)
        {
            WriteWarning(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Rollback cannot be performed since the current installation is not present or invalid");

            TryComplete(operation, ErrorCodeValue::UpgradeFailed);
            return;
        }

        // Swap current and target installations
        auto temp = fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation;
        fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation = fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation;
        fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation = temp;

        // Write the content back to target information file for fabric deployer to use the correct cluster manifest for deployment
        WriteInfo(
            TraceType,
            fabricUpgradeManager_.TraceId,
            "Persisting upgrade context '{0}' to target information file", fabricUpgradeContext_.ToString());
        fabricUpgradeContext_.targetInformationFileDescription.ToXml(fabricUpgradeContext_.targetInformationFilePath);

        // Post a thread to do the rollback
        // This is done on a separate thread because on a machine where upgrade fails continously, we will flip between current and target versions 
        // If done on the same thread, this will eventually lead to stack overflow.
        Threadpool::Post([this, operation]()
        {
            this->DoUpgrade(operation);
        });
    }

    ErrorCode CloseFabricProcesses(RestartManagerWrapper& rmWrapper, wstring const & fabricRoot)
    {
        ErrorCode error(ErrorCodeValue::Success);
        if (!Directory::Exists(fabricRoot))
        {
            return error;
        }

        // File registration is only run once per RestartManagerWrapper
        if (!rmWrapper.IsSessionValid)
        {
            vector<wstring> registeredDirs;
            for (int i = 0; i < Constants::ChildrenFolderCount; ++i)
            {
                wstring directoryPath = Path::Combine(fabricRoot, Constants::ChildrenFolders[i]);
                registeredDirs.push_back(directoryPath);
                directoryPath.append(Constants::OldDirectoryRenameSuffix);
                registeredDirs.push_back(directoryPath);
            }

            rmWrapper.EnumerateAndRegisterFiles(registeredDirs);
        }

        error = rmWrapper.ShutdownProcesses();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Restart manager returned code {0} while shutting down processes; sessionHandle:{1} key:{2}.",
                error,
                rmWrapper.SessionHandle,
                rmWrapper.SessionKey);

            error = rmWrapper.ForceCloseNonCriticalProcesses();
        }
        else
        {
            WriteInfo(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Restart Manager sessionHandle:{0} key:{1} completed.",
                rmWrapper.SessionHandle,
                rmWrapper.SessionKey);
        }

        return error;
    }

    void RestartFabricProcesses(RestartManagerWrapper& rmWrapper)
    {
        ErrorCode error = rmWrapper.RestartProcesses();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Error {0} while restarting processes that were shutdown by restart manager",
                error);
            //Swallow the error; There is no point rolling back
        }
    }

    ErrorCode ExtractFabric(wstring const & fabricRoot, wstring const & cabPackagePath)
    {
        WriteInfo(
            TraceType,
            fabricUpgradeManager_.TraceId,
            "Extracting fabric CAB package {0}",
            fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.MSILocation);

        ErrorCode error(ErrorCodeValue::Success);
        vector<wstring> fabricExtractFilter(Constants::ChildrenFolders, Constants::ChildrenFolders + Constants::ChildrenFolderCount);
        int errorCode = CabOperations::ExtractFiltered(
            cabPackagePath,
            fabricRoot,
            fabricExtractFilter,
            /*inclusive =*/true);
        if (errorCode != S_OK)
        {
            error = ErrorCode::FromWin32Error(errorCode);
            WriteTrace(
                error.ToLogLevel(),
                TraceType,
                fabricUpgradeManager_.TraceId,
                "ExtractFabricPackage CabOperations::ExtractFiltered : Error:{0}",
                error);
        }
        else
        {
#if !defined(PLATFORM_UNIX)
            if (Path::IsPathRooted(fabricRoot))
            {
                wchar_t drive = Path::GetDriveLetter(fabricRoot);

                WriteInfo(TraceType, "Flushing volume file buffer for drive \'{0}\'.", drive);
                error = File::FlushVolume(drive);
                if (!error.IsSuccess())
                {
                    WriteTrace(
                        error.ToLogLevel(),
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "ExtractFabric FlushVolume: Error:{0}",
                        error);
                }
            }
#endif
        }

        return error;
    }

    ErrorCode MoveFabricBits(wstring const & srcRoot, wstring const & dstRoot)
    {
        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        ErrorCode error(ErrorCodeValue::Success);
        vector<pair<wstring, wstring>> conflictingSwaps;
        vector<wstring> conflictingOldDirs;
        // For each child dir, attempt swap
        for (int i = 0; i < Constants::ChildrenFolderCount; ++i)
        {
            wstring fabricRootSubDirectory = Path::Combine(dstRoot, Constants::ChildrenFolders[i]);
            wstring upgradeTempSubDirectory = Path::Combine(srcRoot, Constants::ChildrenFolders[i]);
            ASSERT_IF(!Directory::Exists(upgradeTempSubDirectory), "MoveFabricBits UpgradeTemp directory \"{0}\" did not exist", upgradeTempSubDirectory);

            bool fabricRootSubDirectoryExists;
            error = Directory::Exists(fabricRootSubDirectory, fabricRootSubDirectoryExists);
            ASSERT_IFNOT(error.IsSuccess(), "MoveFabricBits Directory::Exists failed for {0} with error {1}", fabricRootSubDirectory, error);

            if (fabricRootSubDirectoryExists) // If child directory existed in previous install
            {
                WriteInfo(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "MoveFabricBits Replacing directory: {0}",
                    fabricRootSubDirectory);
                wstring frRenameDir = fabricRootSubDirectory + Constants::OldDirectoryRenameSuffix;

                bool frRenameDirExists;
                error = Directory::Exists(frRenameDir, frRenameDirExists);
                ASSERT_IFNOT(error.IsSuccess(), "MoveFabricBits Directory::Exists failed for {0} with error {1}", frRenameDir, error);

                if (frRenameDirExists)
                {
                    WriteInfo(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "MoveFabricBits Deleting temp old directory from previous run: {0}",
                        frRenameDir);
                    error = Directory::Delete_WithRetry(frRenameDir, true, true);
                    if (!error.IsSuccess())
                    {
                        // This case happens when files are movable yet are not deleteable due to persisted handles.
                        WriteWarning(
                            TraceType,
                            fabricUpgradeManager_.TraceId,
                            "MoveFabricBits Deleting temp old dir \"{0}\" hit error: {1}",
                            frRenameDir,
                            error);

                        conflictingSwaps.push_back(make_pair(upgradeTempSubDirectory, fabricRootSubDirectory));
                        conflictingOldDirs.push_back(frRenameDir);
                        continue;
                    }
                }

                // Retry required since back and forth rename appears to not be transactional and occasionally returns error E_ACCESSDENIED
                error = Directory::Rename_WithRetry(fabricRootSubDirectory, frRenameDir, true);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "MoveFabricBits move conflict error {0}: from {1} to {2}.",
                        error,
                        fabricRootSubDirectory,
                        frRenameDir);

                    conflictingSwaps.push_back(make_pair(upgradeTempSubDirectory, fabricRootSubDirectory));
                    continue;
                }

                error = Directory::Exists(fabricRootSubDirectory, fabricRootSubDirectoryExists);
                ASSERT_IFNOT(error.IsSuccess(), "MoveFabricBits Directory::Exists failed for fabricRootSubDirectory {0}. Error: {1}.", fabricRootSubDirectory, error);
                ASSERT_IF(fabricRootSubDirectoryExists, "MoveFabricBits failed to rename fabricRootSubDirectory {0}. Still exists.", fabricRootSubDirectory);

                error = Directory::Delete_WithRetry(frRenameDir, true, true);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "MoveFabricBits Delete post swap failed for directory {0}. Will be rebooting to avoid unusual resource locks.",
                        frRenameDir);
                    conflictingOldDirs.push_back(frRenameDir);
                }
            }

            // No directory conflicts should exist, but on rare occasion Windows appears
            // to perform strange locks on files causing the below rename to fail.
            error = Directory::Rename_WithRetry(upgradeTempSubDirectory, fabricRootSubDirectory, true);
            ASSERT_IFNOT(error.IsSuccess(),
                "MoveFabricBits temporary dir move conflict error {0}: from {1} to {2}.", error, upgradeTempSubDirectory, fabricRootSubDirectory);

            bool upgradeTempSubDirectoryExists;
            error = Directory::Exists(upgradeTempSubDirectory, upgradeTempSubDirectoryExists);

            ASSERT_IFNOT(error.IsSuccess(),
                "MoveFabricBits Directory::Exists failed for upgradeTempSubDirectory {0}: error: {1}.", upgradeTempSubDirectory, error);
            ASSERT_IF(upgradeTempSubDirectoryExists, "MoveFabricBits upgradeTempSubDirectory {0} was supposed to be renamed. It still exists.", upgradeTempSubDirectory);

            WriteInfo(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "MoveFabricBits bits swapped successfully at: {0}",
                fabricRootSubDirectory);
        }

        if (conflictingSwaps.empty() && conflictingOldDirs.empty())
        {
            // No conflicting renames. Delete temporary directory
            error = Directory::Delete(srcRoot, true, true);
            if (!error.IsSuccess())
            {
                // Best effort, but isn't expected to ever fail
                WriteWarning(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "MoveFabricBits failed to delete temporary directory {0} ",
                    srcRoot);
            }
        }
        else // Schedule failed swaps for next reboot
        {
            WriteInfo(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "MoveFabricBits scheduling move & cleanup operations at reboot.");

            if (!conflictingSwaps.empty())
            {
                for (auto& swapDirPair : conflictingSwaps)
                {
                    error = Utility::ReplaceDirectoryAtReboot(swapDirPair.first, swapDirPair.second);
                    ASSERT_IFNOT(error.IsSuccess(), "ReplaceDirectoryAtReboot error {0}: from {1} to {2}.", error, swapDirPair.first, swapDirPair.second);
                }

                error = Utility::DeleteFileAtReboot(srcRoot);
                ASSERT_IFNOT(error.IsSuccess(), "MoveFabricBits OnReboot schedule temp dir delete error {0} for {1} to {2}.", error, srcRoot);
            }

            if (!conflictingOldDirs.empty())
            {
                for (auto& oldDir : conflictingOldDirs)
                {
                    error = Utility::DeleteDirectoryAtReboot(oldDir);
                    ASSERT_IFNOT(error.IsSuccess(), "MoveFabricBits DeleteDirectoryAtReboot error {0}: for directory {1}.", error, oldDir);
                }
            }

            wstring const & validationFilePath = fabricUpgradeContext_.restartValidationFilePath;
            vector<wstring> versionTargetFiles;
            // Get all files in swap directory
            for (auto& swapDirPair : conflictingSwaps)
            {
                vector<wstring> newFiles = Directory::GetFiles(swapDirPair.first, Constants::FindAllSearchPattern, true, false);
                versionTargetFiles.insert(std::end(versionTargetFiles), std::begin(newFiles), std::end(newFiles));
            }

            // Write file with relative paths of all to-be-swapped files in upgrade temp directory along 
            // with their corresponding versions. File versions will be used to validate on reboot that the
            // swap has completed successfully.
            error = Utility::WriteUpgradeValidationFile(validationFilePath, versionTargetFiles, srcRoot);
            ASSERT_IFNOT(error.IsSuccess(),
                "MoveFabricBits WriteUpgradeValidationFile error: {0} failed to write validation file {1} based on files to be copied from {2}.",
                error,
                validationFilePath,
                srcRoot);

            return ErrorCodeValue::RebootRequired;
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode InstallFabricPackage(wstring const & fabricRoot, wstring const & cabPackagePath, RestartManagerWrapper& rmWrapper)
    {
        ErrorCode error(ErrorCodeValue::Success);
        if (ContainsFabricRuntime(fabricRoot))
        {
            ErrorCode closeConflictsError(ErrorCodeValue::Success);
            // Close upgrade-conflicting handles in Fabric root directory
            closeConflictsError = CloseFabricProcesses(rmWrapper, fabricRoot);
            if (!closeConflictsError.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Error {0} while closing Fabric handles before CAB extract (Best-effort).",
                    closeConflictsError);
            }

            // Extract to temp directory & attempt swap
            wstring tempDirectory = Path::Combine(fabricRoot, Constants::UpgradeTempDirectoryName) + L'\\';
            if (Directory::Exists(tempDirectory)) // Normally shouldn't be present
            {
                WriteInfo(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Deleting UT directory from previous run: {0}",
                    tempDirectory);
                error = Directory::Delete(tempDirectory, true, true);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "Deleting UT dir {0} hit error: {1}",
                        tempDirectory,
                        error);
                }
            }
            error = ExtractFabric(tempDirectory, cabPackagePath);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "ExtractFabric hit errorcode {0}.",
                    error);
                return error;
            }

            error = MoveFabricBits(tempDirectory, fabricRoot);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "MoveFabricBits returned errorcode {0}.",
                    error);
                return error;
            }
        }
        else
        {
            WriteInfo(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "Fabric runtime not found. Extracting directly to {0}",
                fabricRoot);
            error = ExtractFabric(fabricRoot, cabPackagePath);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "ExtractFabric hit errorcode {0}.",
                    error);
                return error;
            }
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode DeleteFabricRootFiles(wstring const & fabricRoot, RestartManagerWrapper& rmWrapper)
    {
        ErrorCode error(ErrorCodeValue::Success);
        ErrorCode nonReturnedError(ErrorCodeValue::Success);

        if (!Directory::Exists(fabricRoot))
        {
            WriteInfo(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "FabricRoot does not exist so does not require cleaning.");

            return ErrorCodeValue::Success;
        }

        WriteInfo(
            TraceType,
            fabricUpgradeManager_.TraceId,
            "Deleting FabricRoot files before MSI installation.");

        nonReturnedError = CloseFabricProcesses(rmWrapper, fabricRoot);
        if (!nonReturnedError.IsSuccess())
        {
            WriteWarning(
                TraceType,
                fabricUpgradeManager_.TraceId,
                "DeleteFabricRootFiles CloseFabricProcesses (Best-effort): Error {0}.",
                nonReturnedError);
        }

        vector<pair<wstring, wstring>> renamePairs;
        for (int i = 0; i < Constants::ChildrenFolderCount; ++i)
        {
            wstring fabricRootSubDirectory = Path::Combine(fabricRoot, Constants::ChildrenFolders[i]);
            if (!Directory::Exists(fabricRootSubDirectory))
            {
                WriteInfo(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "DeleteFabricRootFiles directory {0} did not exist.",
                    fabricRootSubDirectory);
                continue;
            }

            wstring frRenameDir = fabricRootSubDirectory + Constants::OldDirectoryRenameSuffix;
            if (Directory::Exists(frRenameDir))
            {
                WriteInfo(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "DeleteFabricRootFiles Deleting temp old directory from previous run: {0}",
                    frRenameDir);
                error = Directory::Delete_WithRetry(frRenameDir, true, true);
                if(!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType,
                        fabricUpgradeManager_.TraceId, 
                        "DeleteFabricRootFiles Deleting temp old dir {0} hit error: {1}", 
                        frRenameDir,
                        error);
                    break;
                }
            }

            error = Directory::Rename_WithRetry(fabricRootSubDirectory, frRenameDir, true);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "DeleteFabricRootFiles move conflict error {0}: from {1} to {2}.",
                    error,
                    fabricRootSubDirectory,
                    frRenameDir);
                break;
            }

            renamePairs.push_back(make_pair(fabricRootSubDirectory, frRenameDir));
        }

        if (!error.IsSuccess()) 
        {
            // Undo renames
            for (auto& pair : renamePairs)
            {
                nonReturnedError = Directory::Rename(pair.second, pair.first, true);
                if (!nonReturnedError.IsSuccess())
                {
                    WriteWarning(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "DeleteFabricRootFiles move conflict error {0}: from {1} to {2}.",
                        nonReturnedError,
                        pair.second,
                        pair.first);
                }
            }
        }
        else
        {
            // Delete directories since all renames went through fine
            for (auto& pair : renamePairs)
            {
                WriteInfo(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "DeleteFabricRootFiles deleting directory {0}.",
                    pair.second);
                nonReturnedError = Directory::Delete_WithRetry(pair.second, true, true);
                if (!nonReturnedError.IsSuccess())
                {
                    WriteWarning(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "DeleteFabricRootFiles failed to delete {0}. Error: {1}",
                        pair.second,
                        nonReturnedError);
                }
            }
        }

        return error;
    }

    ErrorCode StopFabricHostService()
    {
        ErrorCode error(ErrorCodeValue::Success);
        WriteInfo(
            TraceType,
            fabricUpgradeManager_.TraceId,
            "Stopping fabric host on machine {0}",
            Environment::GetMachineName());
        ServiceController sc(FabricInstallerService::Constants::FabricHostServiceName);
        sc.Stop();
        int currentRetryCount = 0;
        for (;;)
        {
            error = sc.WaitForServiceStop(TimeSpan::FromMinutes(FabricInstallerService::Constants::ServiceStopTimeoutInMinutes));
            if (error.IsSuccess())
            {
                break;
            } else if ((error.IsWin32Error(ERROR_SERVICE_DOES_NOT_EXIST)
                && (!fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.IsValid || !fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.IsValid)))
                // Initial bootstrap or Uninstall scenarios; don't care if service did not exist
            {
                wstring scenario = !fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.IsValid ? L"installation"
                    : (!fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.IsValid ? L"uninstallation" : L"??");
                WriteInfo(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Fabric host service did not exist. Fine for {0}.",
                    scenario);
                break;
            }
            else
            {
                WriteInfo(
                    TraceType,
                    fabricUpgradeManager_.TraceId,
                    "Error {0} while waiting for fabric host service to stop.",
                    error);

                if (error.IsError(ErrorCodeValue::Timeout) && currentRetryCount < FabricInstallerService::Constants::FabricHostServiceStopRetryCount)
                {
                    ++currentRetryCount;
                    WriteWarning(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "Retrying the wait for fabric host service to stop. Current retry count {0}", currentRetryCount);
                    continue;
                }
                else
                {
                    WriteError(
                        TraceType,
                        fabricUpgradeManager_.TraceId,
                        "Unable to stop fabric host service; error {0}", error.Message);
                    return ErrorCodeValue::OperationFailed;
                }
            }
        }

        return ErrorCodeValue::Success;
    }

    void RunCleanup()
    {
        ErrorCode error(ErrorCodeValue::Success);
        // Run for XCopy uninstall scenario
        if (fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.IsValid
            && !fabricUpgradeContext_.targetInformationFileDescription.TargetInstallation.IsValid
            && !fabricUpgradeContext_.targetInformationFileDescription.CurrentInstallation.UndoUpgradeEntryPointExe.empty())
        {
            // Delete installation bin directory - best effort
            static_assert(Constants::ChildrenFolderCount > 0, "Children folders must be set.");
            RestartManagerWrapper cleanupRmWrapper;
            error = CloseFabricProcesses(cleanupRmWrapper, fabricUpgradeContext_.fabricRoot);
            if (!error.IsSuccess())
            {
                WriteWarning(TraceType, fabricUpgradeManager_.TraceId, "CloseFabricProcesses returned error {0}.", error);
                return;
            }

            wstring binDir = Path::Combine(fabricUpgradeContext_.fabricRoot, Constants::ChildrenFolders[0]);
            if (Directory::Exists(binDir))
            {
                wstring binDirRename = binDir + L".del";
                if (Directory::Exists(binDirRename)) // In case of old run terminating unexpectedly
                {
                    error = Directory::Delete(binDirRename, true, true);
                    if (!error.IsSuccess())
                    {
                        WriteWarning(TraceType, fabricUpgradeManager_.TraceId, "RunCleanup old dir: {0} delete error {1}.", binDirRename, error);
                        return;
                    }
                }

                error = Directory::Rename(binDir, binDirRename);
                if (!error.IsSuccess())
                {
                    WriteWarning(TraceType, fabricUpgradeManager_.TraceId, "RunCleanup rename error {0}: from {1} to {2}.", error, binDir, binDirRename);
                    return;
                }

                error = Directory::Delete(binDirRename, true, true);
                if (!error.IsSuccess())
                {
                    WriteWarning(TraceType, fabricUpgradeManager_.TraceId, "RunCleanup dir: {0} delete error {1}.", binDirRename, error);
                    return;
                }
            }
        }
    }

    ErrorCode RecoverFabricFilesPostReboot(wstring utDir, wstring fabricRoot)
    {
        ErrorCode error(ErrorCodeValue::Success);
        vector<wstring> utSubs = Directory::GetSubDirectories(utDir, Constants::FindAllSearchPattern, false, true);
        wstring sourceDir;
        wstring destinationDir;
        for (auto& sub : utSubs)
        {
            sourceDir = Path::Combine(utDir, sub);
            destinationDir = Path::Combine(fabricRoot, sub);
            if (Directory::Exists(destinationDir))
            {
                WriteInfo(TraceType, fabricUpgradeManager_.TraceId, "Directory already exists: {0}. Deleting.", destinationDir);
                error = Directory::Delete(destinationDir, true, true);
                if (!error.IsSuccess())
                {
                    WriteError(TraceType, fabricUpgradeManager_.TraceId, "RecoverFabricFilesPostReboot delete of {0} failed with error: {1}.", destinationDir, error);
                    return error;
                }
            }
            error = Directory::Rename(sourceDir, destinationDir);
            if (!error.IsSuccess())
            {
                WriteError(TraceType, fabricUpgradeManager_.TraceId, "RecoverFabricFilesPostReboot rename failed from {0} to {1} with error: {2}.", sourceDir, destinationDir, error);
                return error;
            }
            WriteInfo(TraceType, fabricUpgradeManager_.TraceId, "Successfully moved {0} to {1}.", sourceDir, destinationDir);

            Directory::Delete(utDir, true, true);
        }

        return ErrorCodeValue::Success;
    }

    bool ContainsFabricRuntime(wstring const & fabricRoot)
    {
        for (int i = 0; i < Constants::ChildrenFolderCount; ++i)
        {
            wstring dir = Path::Combine(fabricRoot, Constants::ChildrenFolders[i]);
            if (Directory::Exists(dir))
            {
                return true;
            }
        }

        return false;
    }

    bool isMSI(wstring const & packagePath)
    {
        return Path::GetExtension(packagePath) == L".msi";
    }

    bool isCAB(wstring const & packagePath)
    {
        return Path::GetExtension(packagePath) == L".cab";
    }

private:
    FabricUpgradeContext fabricUpgradeContext_;
    FabricUpgradeManager & fabricUpgradeManager_;
    TimeoutHelper timeoutHelper_;
    TimeoutHelper upgradeTimeoutHelper_;
    TimerSPtr retryTimer_;
    ExclusiveLock timerLock_;
    bool autoclean_;
};

// ********************************************************************************************************************
// FabricUpgradeManager::CloseAsyncOperation Implementation
//
class FabricUpgradeManager::CloseAsyncOperation : public AsyncOperation
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        FabricUpgradeManager& fabricUpgradeManager,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperationParent)
        : fabricUpgradeManager_(fabricUpgradeManager),
        timeout_(timeout),
        AsyncOperation(callback, asyncOperationParent)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & operation)
    {
        ErrorCode error(ErrorCodeValue::Success);
        if (!fabricUpgradeManager_.closeEvent_.WaitOne(timeout_))
        {
            error = ErrorCodeValue::Timeout;
        }

        TryComplete(operation, error);
    }

private:
    FabricUpgradeManager & fabricUpgradeManager_;
    TimeSpan timeout_;
};

FabricUpgradeManager::FabricUpgradeManager(bool autoclean)
    : closeEvent_(true)
    , autoclean_(autoclean)
{
}

FabricUpgradeManager::~FabricUpgradeManager()
{
    WriteInfo(TraceType, TraceId, "destructor called");
}

AsyncOperationSPtr FabricUpgradeManager::OnBeginOpen(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode FabricUpgradeManager::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    FabricUpgradeContext fabricUpgradeContext;
    ErrorCode error = OpenAsyncOperation::End(asyncOperation, fabricUpgradeContext);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        // This errorcode will only be returned when target information file is not found. Return success;
        WriteInfo(
            TraceType,
            this->TraceId,
            "Starting FabricHost service");
        error = ServiceController::StartServiceByName(
            Constants::FabricHostServiceName, 
            true, 
            TimeSpan::FromMinutes(Constants::ServiceStartTimeoutInMinutes));
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                this->TraceId,
                "Error {0} while waiting fabric host service to start in OnEndOpen.",
                error);
            return error;
        }
        Threadpool::Post([]()
        {
            // Delay here is to ensure that Service.Start returns correctly
            ::Sleep(5000);
            ServiceController::StopServiceByName(Constants::ServiceName, true, TimeSpan::FromMinutes(Constants::ServiceStopTimeoutInMinutes));
        });

        return ErrorCodeValue::Success;
    }
    else if (error.IsSuccess())
    {
        this->closeEvent_.Reset();
        AsyncOperation::CreateAndStart<UpgradeAsyncOperation>(
            fabricUpgradeContext,
            *this, 
            TimeSpan::FromMinutes(FabricInstallerService::Constants::UpgradeTimeoutInMinutes),
            autoclean_,
            [this](AsyncOperationSPtr const& operation)
            {
                auto error = UpgradeAsyncOperation::End(operation);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType,
                        this->TraceId,
                        "Upgrade finished with error {0}",
                        error);
                }
                else
                {
                    WriteInfo(
                        TraceType,
                        this->TraceId,
                        "Fabric Upgrade Completed successfully");
                }

                this->closeEvent_.Set();
                ServiceController::StopServiceByName(Constants::ServiceName, true, TimeSpan::FromMinutes(Constants::ServiceStopTimeoutInMinutes));
            },
            this->CreateAsyncOperationRoot());
    }

    return error;
}

AsyncOperationSPtr FabricUpgradeManager::OnBeginClose(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode FabricUpgradeManager::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode error = CloseAsyncOperation::End(asyncOperation);
    return error;
}

void FabricUpgradeManager::OnAbort()
{
}
