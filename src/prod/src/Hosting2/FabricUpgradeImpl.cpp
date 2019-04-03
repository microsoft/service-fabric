// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Management;
using namespace ImageModel;

StringLiteral const TraceType("FabricUpgradeImpl");

class FabricUpgradeImpl::ValidateFabricUpgradeAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ValidateFabricUpgradeAsyncOperation)

public:
    ValidateFabricUpgradeAsyncOperation(
        FabricUpgradeImpl & owner,
        FabricVersionInstance const & currentFabricVersionInstance,
        FabricUpgradeSpecification const & fabricUpgradeSpec,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        currentFabricVersionInstance_(currentFabricVersionInstance),
        fabricUpgradeSpec_(fabricUpgradeSpec),
        shouldRestartReplica_(false),
        fabricDeployerProcessWait_(),
        fabricDeployerProcessHandle_()
    {
    }

    virtual ~ValidateFabricUpgradeAsyncOperation()
    {
    }

    static ErrorCode End(
        __out bool & shouldRestartReplica,
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ValidateFabricUpgradeAsyncOperation>(operation);
        shouldRestartReplica = thisPtr->shouldRestartReplica_;
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(            
            TraceType,
            owner_.Root.TraceId,
            "ValidateAndAnalyzeFabricUpgradeImpl: CurrentVersion: {0}, TargetVersion: {1}",
            currentFabricVersionInstance_,
            FabricVersionInstance(fabricUpgradeSpec_.Version, fabricUpgradeSpec_.InstanceId));

        if(currentFabricVersionInstance_.Version.CodeVersion != fabricUpgradeSpec_.Version.CodeVersion)
        {
            shouldRestartReplica_ = true;
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }
   
        if(currentFabricVersionInstance_.Version.ConfigVersion != fabricUpgradeSpec_.Version.ConfigVersion)
        {
            HandleUPtr fabricDeployerThreadHandle;           

            wstring tempOutputFile = File::GetTempFileNameW(owner_.hosting_.NodeWorkFolder);
            wstring tempErrorFile = File::GetTempFileNameW(owner_.hosting_.NodeWorkFolder);

            wstring commandLineArguments = wformatString(
                Constants::FabricDeployer::ValidateAndAnalyzeArguments, 
                owner_.fabricUpgradeRunLayout_.GetClusterManifestFile(fabricUpgradeSpec_.Version.ConfigVersion.ToString()),                
                owner_.hosting_.NodeName,
                owner_.hosting_.NodeType,
                tempOutputFile,
                tempErrorFile);

            WriteInfo(            
                TraceType,
                owner_.Root.TraceId,
                "ValidateAndAnalyzeFabricUpgradeImpl: FabricDeployerProcess called with {0}",
                commandLineArguments);

            wstring deployerPath = Path::Combine(Environment::GetExecutablePath(), Constants::FabricDeployer::ExeName);
            vector<wchar_t> envBlock(0);

#if defined(PLATFORM_UNIX)   
            // Environment map is only needed in linux since the environment is not 
            // passed in by default in linux
            EnvironmentMap environmentMap;
            auto getEnvResult = Environment::GetEnvironmentMap(environmentMap);
            auto createEnvError = ProcessUtility::CreateDefaultEnvironmentBlock(envBlock);

            if (!createEnvError.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "Failed to create EnvironmentBlock for ValidateFabricUpgradeAsyncOperation due to {0}",
                    createEnvError);

                TryComplete(thisSPtr, createEnvError);
                return;
            }
#endif

            pid_t pid = 0;
            auto error = ProcessUtility::CreateProcessW(
                ProcessUtility::GetCommandLine(deployerPath, commandLineArguments),
                L"",
                envBlock,
                DETACHED_PROCESS /* Do not inherit console */,
                fabricDeployerProcessHandle_,
                fabricDeployerThreadHandle,
                pid);

            if (!error.IsSuccess())
            {            
                WriteWarning(            
                    TraceType,
                    owner_.Root.TraceId,
                    "ValidateAndAnalyzeFabricUpgradeImpl: Create FabricDeployerProcess failed with {0}",
                    error);

                TryComplete(thisSPtr, error);
                return;
            }

            fabricDeployerProcessWait_ = ProcessWait::CreateAndStart(
                Handle(fabricDeployerProcessHandle_->Value, Handle::DUPLICATE()),
                pid,
                [this, thisSPtr, tempOutputFile, tempErrorFile](pid_t, ErrorCode const & waitResult, DWORD exitCode) { OnFabricDeployerProcessTerminated(thisSPtr, waitResult, exitCode, tempOutputFile, tempErrorFile); });
        }
        else
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }
    }

    void OnFabricDeployerProcessTerminated(
        AsyncOperationSPtr const & thisSPtr,
        Common::ErrorCode const & waitResult,
        DWORD fabricDeployerExitCode,
        wstring const & tempOutputFile, 
        wstring const & tempErrorFile)
    {
        if(!waitResult.IsSuccess())
        {
            TryComplete(thisSPtr, waitResult);
            return;
        }

        // Exit Code 1(256) and 2(512) is used by deployer currently to indicate whether restart is required or not
        // Exit Code 2 is restart not required and treated as success. On windows the exit code can go below 0 but 
        // on linux it does not so basically anything other than 0, 1(256) or 2(512) is an error and we try to read the error file
        ErrorCode error;
        if (fabricDeployerExitCode != 0 && fabricDeployerExitCode != Constants::FabricDeployer::ExitCode_RestartRequired && fabricDeployerExitCode != Constants::FabricDeployer::ExitCode_RestartNotRequired)
        {
            wstring fileContent;
            error = owner_.ReadFabricDeployerTempFile(tempErrorFile, fileContent);
            if(error.IsSuccess())
            {
                error = ErrorCode(ErrorCodeValue::FabricNotUpgrading, move(fileContent));
            }
            else
            {
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "CheckIfRestartRequired: Could not read error file: {0}", tempErrorFile);                
            }            
        }

        WriteTrace(            
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "ValidateAndAnalyzeFabricUpgradeImpl: FabricDeployer process terminated. CurrentVersion: {0}, TargetVersion: {1}. ExitCode:{2}. Error: {3}.",
            currentFabricVersionInstance_,
            FabricVersionInstance(fabricUpgradeSpec_.Version, fabricUpgradeSpec_.InstanceId),
            fabricDeployerExitCode,
            error);

        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        ASSERT_IF(
            Constants::FabricDeployer::ExitCode_RestartRequired == fabricDeployerExitCode && !File::Exists(tempOutputFile), 
            "The output file should be generated by deployer when exit code is ExitCode_RestartRequired. File:{0}",
            tempOutputFile);

        bool staticParameterModified = false;
        if(Constants::FabricDeployer::ExitCode_RestartRequired == fabricDeployerExitCode)
        {
            error = CheckIfRestartRequired(tempOutputFile, staticParameterModified);
            WriteTrace(            
                error.ToLogLevel(),
                TraceType,
                owner_.Root.TraceId,
                "CheckIfRestartRequired returned {0}. CurrentVersion: {1}, TargetVersion: {2}.",
                error,
                currentFabricVersionInstance_,
                FabricVersionInstance(fabricUpgradeSpec_.Version, fabricUpgradeSpec_.InstanceId));

            if(!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }
        }

        if(staticParameterModified || fabricUpgradeSpec_.UpgradeType == UpgradeType::Rolling_ForceRestart)
        {
            this->shouldRestartReplica_ = true;
        }

        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

    ErrorCode CheckIfRestartRequired(wstring const & validateOutputFileName, __out bool & staticParameterModified)
    {
        staticParameterModified = false;

        wstring fileContent;
        auto error = owner_.ReadFabricDeployerTempFile(validateOutputFileName, fileContent);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "CheckIfRestartRequired: Could not read output file: {0}", validateOutputFileName);

            return error;
        }

        vector<wstring> tokens;
#ifdef PLATFORM_UNIX
        StringUtility::SplitOnString<wstring>(StringUtility::ToWString(fileContent), tokens, L"\n", false);
#else
		StringUtility::SplitOnString<wstring>(StringUtility::ToWString(fileContent), tokens, L"\r\n", false);
#endif

        ASSERT_IF(tokens.size() == 0 || tokens.size() % 4 != 0, "The fileContent is invalid : {0}", fileContent);

        int index = 0;
        do
        {
            wstring section = tokens[index++];
            wstring key = tokens[index++];
            wstring value = tokens[index++];
            bool isEncrypted = StringUtility::AreEqualCaseInsensitive(tokens[index++], L"true");

            auto configStore = ComProxyConfigStore::Create();
            bool canUpgradeWithoutRestart = configStore->CheckUpdate(
                section,
                key,
                value,
                isEncrypted);

            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "CheckIfRestartRequired: Section:{0}, Key:{1}, CanUpgradeWithoutRestart:{2}.",
                section,
                key,
                canUpgradeWithoutRestart);

            if(!canUpgradeWithoutRestart)
            {
                staticParameterModified = true;
                break;
            }
        } while(index < tokens.size());        

        return error;
    }

    virtual void OnCancel()
    {
        fabricDeployerProcessWait_->Cancel();
        ErrorCode error;
        if(!::TerminateProcess(fabricDeployerProcessHandle_->Value, ProcessActivator::ProcessAbortExitCode))
        {
            error = ErrorCode::FromWin32Error();
        }
        
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceType,
            owner_.Root.TraceId,
            "ValidateFabircUpgradeAsyncOperation OnCancel TerminateProcess: ErrorCode={0}",
            error);
    }

private:
    FabricUpgradeImpl & owner_;
    FabricVersionInstance const currentFabricVersionInstance_;
    FabricUpgradeSpecification const fabricUpgradeSpec_;
    bool shouldRestartReplica_;

    ProcessWaitSPtr fabricDeployerProcessWait_;
    HandleUPtr fabricDeployerProcessHandle_;
};

class FabricUpgradeImpl::FabricUpgradeAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(FabricUpgradeAsyncOperation)

public:
    FabricUpgradeAsyncOperation(
        FabricUpgradeImpl & owner,        
        FabricVersionInstance const & currentFabricVersionInstance,
        FabricUpgradeSpecification const & fabricUpgradeSpec,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),        
        currentFabricVersionInstance_(currentFabricVersionInstance),
        fabricUpgradeSpec_(fabricUpgradeSpec),
        timeoutHelper_(HostingConfig::GetConfig().FabricUpgradeTimeout)
    {
    }

    virtual ~FabricUpgradeAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<FabricUpgradeAsyncOperation>(operation);        
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {        
        bool hasCodeChanged = (currentFabricVersionInstance_.Version.CodeVersion != fabricUpgradeSpec_.Version.CodeVersion);
        bool hasConfigChanged = (currentFabricVersionInstance_.Version.ConfigVersion != fabricUpgradeSpec_.Version.ConfigVersion);
        bool hasInstanceIdChanged = (currentFabricVersionInstance_.InstanceId != fabricUpgradeSpec_.InstanceId);  

        WriteInfo(            
            TraceType,
            owner_.Root.TraceId,
            "FabricUpgradeImpl: CurrentVersion: {0}, TargetVersion: {1}",
            currentFabricVersionInstance_,
            FabricVersionInstance(fabricUpgradeSpec_.Version, fabricUpgradeSpec_.InstanceId));

        if(hasCodeChanged)
        {
            this->CodeUpgrade(thisSPtr);            
        }
        else if(hasConfigChanged)
        {
            this->ConfigUpgrade(thisSPtr, false /*instanceIdOnlyUpgrade*/);
        }        
        else if(hasInstanceIdChanged)
        {
            this->ConfigUpgrade(thisSPtr, true /*instanceIdOnlyUpgrade*/);
        }
        else
        {
            TryComplete(thisSPtr, ErrorCodeValue::FabricAlreadyInTargetVersion);
            return;
        }
    }

    virtual void OnCancel()
    {
	AsyncOperation::Cancel();
    }

    void CodeUpgrade(AsyncOperationSPtr const & thisSPtr)
    {
        wstring dataRoot, logRoot;
        auto error = FabricEnvironment::GetFabricDataRoot(dataRoot);
        if(!error.IsSuccess())
        {
            WriteInfo(            
                TraceType,
                owner_.Root.TraceId,
                "Error getting FabricDataRoot. Error:{0}",
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        error = FabricEnvironment::GetFabricLogRoot(logRoot);
        if(!error.IsSuccess())
        {
            WriteInfo(            
                TraceType,
                owner_.Root.TraceId,
                "Error getting FabricLogRoot. Error:{0}",
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        FabricDeploymentSpecification fabricDeploymentSpec(dataRoot, logRoot);
        // Write the installer script only if the target code package is a MSI or Cab File
        bool useFabricInstallerService = false;
        bool isCabFilePresent = false;
		wstring installerFilePath = owner_.fabricUpgradeRunLayout_.GetPatchFile(fabricUpgradeSpec_.Version.CodeVersion.ToString());
        bool isInstallerFilePresent = File::Exists(installerFilePath);
        wstring downloadedFabricPackage(L"");
        if (isInstallerFilePresent)
        {
#if defined(PLATFORM_UNIX)
			downloadedFabricPackage = move(installerFilePath);
#else
            DWORD useFabricInstallerSvc;
            RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, true, true);
            if (regKey.IsValid 
                && regKey.GetValue(FabricConstants::UseFabricInstallerSvcKeyName, useFabricInstallerSvc)
                && useFabricInstallerSvc == 1UL)
            {
                downloadedFabricPackage = move(installerFilePath);
                useFabricInstallerService = true;
                WriteInfo(
                    TraceType,
                    owner_.Root.TraceId,
                    "Hosting CodeUpgrade MSI path using FabricInstallerSvc. MSI: {0}",
                    downloadedFabricPackage);
            }
#endif
        }
        else
        {
            downloadedFabricPackage = move(owner_.fabricUpgradeRunLayout_.GetCabPatchFile(fabricUpgradeSpec_.Version.CodeVersion.ToString()));
            isCabFilePresent = File::Exists(downloadedFabricPackage);
            if (isCabFilePresent)
            {
                bool containsWUfile = false;
                int errorCode = CabOperations::ContainsFile(downloadedFabricPackage, *Constants::FabricUpgrade::FabricWindowsUpdateContainedFile, containsWUfile);
                if (errorCode != S_OK)
                {
                    error = ErrorCode::FromWin32Error(errorCode);
                    WriteTrace(
                        error.ToLogLevel(),
                        TraceType,
                        owner_.Root.TraceId,
                        "CabOperations::ContainsFile : Error:{0}",
                        error);
                    TryComplete(thisSPtr, error);
                    return;
                }
                useFabricInstallerService = !containsWUfile; // WU file identifies package as incompatible with FabricInstallerSvc
            }
            else
            {
                // Directory package no longer supported
                WriteError(TraceType, owner_.Root.TraceId, "CodeUpgrade did not find CAB/MSI package for the given version.");
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidArgument));
                return;
            }
        }
        
        wstring installationScriptFilePath(L"");
        if (useFabricInstallerService)
        {
            // If Fabric installer service is running, wait till it stops
            //LINUXTODO
#if !defined(PLATFORM_UNIX)            
            ServiceController sc(Constants::FabricUpgrade::FabricInstallerServiceName);
            error = sc.WaitForServiceStop(timeoutHelper_.GetRemainingTime());
            if (!error.IsSuccess() && !error.IsWin32Error(ERROR_SERVICE_DOES_NOT_EXIST))
            {
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "Error {0} while waiting for Fabric installer service to stop",
                    error);
                TryComplete(thisSPtr, error);
                return;
            }
#endif    
        }
        else
        {
            installationScriptFilePath = fabricDeploymentSpec.GetInstallerScriptFile(owner_.hosting_.NodeName);
            wstring installationLogFilePath = fabricDeploymentSpec.GetInstallerLogFile(owner_.hosting_.NodeName, fabricUpgradeSpec_.Version.CodeVersion.ToString());
            error = WriteInstallationScriptFile(installationScriptFilePath, installationLogFilePath, isCabFilePresent);
            WriteTrace(
                error.ToLogLevel(),
                TraceType,
                owner_.Root.TraceId,
                "WriteInstallationScriptFile : Error: {0}",
                error);
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }
        }

        wstring targetInformationFilePath = fabricDeploymentSpec.GetTargetInformationFile();
        error = WriteTargetInformationFile(targetInformationFilePath, downloadedFabricPackage, useFabricInstallerService);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "WriteTargetInformationFile : Error: {0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

#if defined(PLATFORM_UNIX)    
		wstring upgradeArgs = targetInformationFilePath;/* arguments */
#else
        wstring upgradeArgs = L"" /* arguments */;
#endif

        if (!useFabricInstallerService)
        {
            WriteNoise(
                TraceType,
                owner_.Root.TraceId,
                "Begin(FabricActivatorClient FabricUpgrade): InstallationScript:{0}",
                installationScriptFilePath);
        }

#if defined(PLATFORM_UNIX)
        error = SwapUpdaterService(downloadedFabricPackage, fabricDeploymentSpec);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "SwapUpdaterService : Error: {0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
#endif
        
        auto operation = owner_.hosting_.FabricActivatorClientObj->BeginFabricUpgrade(
            useFabricInstallerService,
            installationScriptFilePath,
            upgradeArgs /* arguments */,
            downloadedFabricPackage,
            [this, useFabricInstallerService](AsyncOperationSPtr const & operation) { this->FinishCodeUpgrade(operation, useFabricInstallerService, false); },
            thisSPtr);
        FinishCodeUpgrade(operation, useFabricInstallerService, true);
    }    

    void FinishCodeUpgrade(AsyncOperationSPtr const & operation, bool useFabricInstallerService, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.hosting_.FabricActivatorClientObj->EndFabricUpgrade(operation);
        // The assert is only valid if the upgrade was done using a MSI.
        if (!useFabricInstallerService)
        {
            ASSERT_IF(error.IsSuccess(), "The FabricUpgrade call for code should return only with failure. In case of success, the host will be killed.");
        }

        WriteTrace(            
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(FabricActivatorClient FabricUpgrade): {0}",
            error);

        TryComplete(operation->Parent, error);        
    }

    ErrorCode WriteTargetInformationFile(wstring const & targetInformationFilePath, wstring const & targetPackagePath, bool)
    {
        wstring currentInstallationElement(L"");
        wstring currentClusterManifestFile = move(owner_.fabricUpgradeRunLayout_.GetClusterManifestFile(currentFabricVersionInstance_.Version.ConfigVersion.ToString()));
        bool isCurrentMsi = false;
        bool isCurrentCab = false;
        bool isWindowsUpdate = false;

        // If the current version files are not provisioned Rollback will not be supported
        // Current Installation element is set only if both the cluster manifest and MSI/CodePackage are present
        if (File::Exists(currentClusterManifestFile))
        {
	        wstring currentCodeVersion = move(currentFabricVersionInstance_.Version.CodeVersion.ToString());
	        wstring currentMsiPackagePath = move(owner_.fabricUpgradeRunLayout_.GetPatchFile(currentCodeVersion));
            wstring currentCabPackagePath = move(owner_.fabricUpgradeRunLayout_.GetCabPatchFile(currentCodeVersion));
            wstring currentPackagePath(L"");
            if (File::Exists(currentMsiPackagePath))
            {
                currentPackagePath = currentMsiPackagePath;
                isCurrentMsi = true;
            }
            else if (File::Exists(currentCabPackagePath))
            {
                currentPackagePath = currentCabPackagePath;
                isCurrentCab = true;
            }

            // If !isCurrentCab && !isCurrentMsi then we skip writing CurrentInstallation for TargetInformationFile
            if (isCurrentCab)
            {
                int errorCode = CabOperations::ContainsFile(currentPackagePath, *Constants::FabricUpgrade::FabricWindowsUpdateContainedFile, isWindowsUpdate);
                if (errorCode != S_OK)
                {
                    auto error = ErrorCode::FromWin32Error(errorCode);
                    WriteTrace(
                        error.ToLogLevel(),
                        TraceType,
                        owner_.Root.TraceId,
                        "WriteTargetInformationFile CabOperations::ContainsFile : Error:{0}",
                        error);
                    return error;
                }

                if (!isWindowsUpdate)
                {
                    // Current code package exists and is a XCOPY package
                    currentInstallationElement = wformatString(
                        Constants::FabricUpgrade::CurrentInstallationElementForXCopy,
                        currentFabricVersionInstance_.InstanceId,
                        currentFabricVersionInstance_.Version.CodeVersion.ToString(),
                        currentClusterManifestFile,
                        currentPackagePath,
                        owner_.hosting_.NodeName,
                        Constants::FabricSetup::ExeName,
                        Constants::FabricSetup::UpgradeArguments,
                        Constants::FabricSetup::ExeName,
                        Constants::FabricSetup::UndoUpgradeArguments);
                }
            }

            if (isCurrentMsi || isWindowsUpdate)
            {
                currentInstallationElement = wformatString(
                    Constants::FabricUpgrade::CurrentInstallationElement,
                    currentFabricVersionInstance_.InstanceId,
                    currentFabricVersionInstance_.Version.CodeVersion.ToString(),
                    currentClusterManifestFile,
                    currentPackagePath,
                    owner_.hosting_.NodeName);
            }

            if (currentInstallationElement == L"")
            {
                WriteWarning(TraceType, "WriteTargetInformationFile did not find CAB/MSI package for the given version. Skip writing CurrentInstallation element.");
            }
        }

        wstring targetInstallationElement(L"");
        wstring targetCodeVersion = fabricUpgradeSpec_.Version.CodeVersion.ToString();
        if (!isWindowsUpdate && Path::GetExtension(targetPackagePath) == L".cab")
        {
            // Target code package is a XCOPY package
            targetInstallationElement = wformatString(
                Constants::FabricUpgrade::TargetInstallationElementForXCopy,
                fabricUpgradeSpec_.InstanceId,
                fabricUpgradeSpec_.Version.CodeVersion.ToString(),
                owner_.fabricUpgradeRunLayout_.GetClusterManifestFile(fabricUpgradeSpec_.Version.ConfigVersion.ToString()),
                targetPackagePath,
                owner_.hosting_.NodeName,
                Constants::FabricSetup::ExeName,
                Constants::FabricSetup::UpgradeArguments,
                Constants::FabricSetup::ExeName,
                Constants::FabricSetup::UndoUpgradeArguments);
        }
        else
        {
            targetInstallationElement = wformatString(
                Constants::FabricUpgrade::TargetInstallationElement,
                fabricUpgradeSpec_.InstanceId,
                fabricUpgradeSpec_.Version.CodeVersion.ToString(),
                owner_.fabricUpgradeRunLayout_.GetClusterManifestFile(fabricUpgradeSpec_.Version.ConfigVersion.ToString()),
                targetPackagePath,
                owner_.hosting_.NodeName);
        }

#if defined(PLATFORM_UNIX)
       wstring targetInformationContentUtf16 = wformatString(
            Constants::FabricUpgrade::TargetInformationXmlContent,
            currentInstallationElement,
            targetInstallationElement);
        string targetInformationContent = StringUtility::Utf16ToUtf8(targetInformationContentUtf16);
#else
        wstring targetInformationContent = wformatString(
            Constants::FabricUpgrade::TargetInformationXmlContent,
            currentInstallationElement,
            targetInstallationElement);
#endif

        FileWriter fileWriter;
        auto error = fileWriter.TryOpen(targetInformationFilePath);
        if(!error.IsSuccess())
        {
            return error;
        }

#if defined(PLATFORM_UNIX)
        fileWriter.WriteAsciiBuffer(targetInformationContent.c_str(), targetInformationContent.size());
#else
        fileWriter.WriteUnicodeBuffer(targetInformationContent.c_str(), targetInformationContent.size());
#endif
        fileWriter.Close();

        return ErrorCodeValue::Success;
    }

    ErrorCode WriteInstallationScriptFile(wstring const & installationScriptFilePath, wstring const & installationLogFilePath, bool const & useCabFile)
    {
        if (File::Exists(installationScriptFilePath))
        {
            File::Delete2(installationScriptFilePath).ReadValue();
        }

#if defined(PLATFORM_UNIX)
        wstring installerFile = Path::Combine(Environment::GetExecutablePath(), Constants::FabricUpgrade::LinuxPackageInstallerScriptFileName);
        return File::Copy(installerFile, installationScriptFilePath);
#else

        FileWriter fileWriter;
        auto error = fileWriter.TryOpen(installationScriptFilePath);
        if(!error.IsSuccess())
        {            
            return error;
        }                        

        wstring installCommand = L"";
        wstring execCommand = L"";
        if (useCabFile)
        {
            wstring currentFabricCodeVersion = currentFabricVersionInstance_.Version.CodeVersion.ToString();
            wstring upgradeCodeVersion = fabricUpgradeSpec_.Version.CodeVersion.ToString();

            string stopFabricHostSvcAnsi;
            StringUtility::UnicodeToAnsi(*Constants::FabricUpgrade::StopFabricHostServiceCommand, stopFabricHostSvcAnsi);
            fileWriter.WriteLine(stopFabricHostSvcAnsi);

            if (currentFabricCodeVersion > upgradeCodeVersion)
            {
                installCommand = wformatString(
                    Constants::FabricUpgrade::DISMExecUnInstallCommand,
                    owner_.fabricUpgradeRunLayout_.GetCabPatchFile(currentFabricCodeVersion),
                    installationLogFilePath);                
            }
            else
            {
                installCommand = wformatString(
                    Constants::FabricUpgrade::DISMExecCommand,
                    owner_.fabricUpgradeRunLayout_.GetCabPatchFile(upgradeCodeVersion),
                    installationLogFilePath);
            }
        }
        else
        {        
            installCommand = wformatString(
                Constants::FabricUpgrade::MSIExecCommand,
                owner_.fabricUpgradeRunLayout_.GetPatchFile(fabricUpgradeSpec_.Version.CodeVersion.ToString()),
                installationLogFilePath);         
        }

        string installCommandAnsi;        
        StringUtility::UnicodeToAnsi(installCommand, installCommandAnsi);

        string startFabricHostSvcAnsi;
        StringUtility::UnicodeToAnsi(*Constants::FabricUpgrade::StartFabricHostServiceCommand, startFabricHostSvcAnsi);

        fileWriter.WriteLine(installCommandAnsi);
        fileWriter.WriteLine(startFabricHostSvcAnsi);

        fileWriter.Close();

        return ErrorCodeValue::Success;
#endif
    }

#if defined(PLATFORM_UNIX)
    ErrorCode SwapUpdaterService(wstring const& fabricPackagePath, FabricDeploymentSpecification & fabricDeploymentSpec)
    {
        Common::LinuxPackageManagerType::Enum packageManagerType;
        auto error = FabricEnvironment::GetLinuxPackageManagerType(packageManagerType);
        ASSERT_IF(!error.IsSuccess(), "GetLinuxPackageManagerType failed. Type: {0}. Error: {1}", packageManagerType, error);
        CODING_ERROR_ASSERT(packageManagerType != Common::LinuxPackageManagerType::Enum::Unknown);

        DWORD dwErr = ERROR_SUCCESS;
        string command;

        // Extract doupgrade.sh to [DataRoot]/doupgrade.sh
        wstring const & filename = Constants::FabricUpgrade::LinuxUpgradeScriptFileName;
        wstring metaDirectory = Path::Combine(fabricDeploymentSpec.DataRoot, Constants::FabricUpgrade::LinuxUpgradeMetadataDirectoryName);
        wstring upgradeScriptMetaPath = Path::Combine(metaDirectory, filename);

        if (!Directory::Exists(metaDirectory))
        {
            error = Directory::Create2(metaDirectory);
            if (!error.IsSuccess())
            {
                WriteError(TraceType, owner_.Root.TraceId, "SwapUpdaterService: Directory create failed with err {0} ; Path: {1}.", error, metaDirectory);
                return error;
            }
        }

        switch (packageManagerType)
        {
            case Common::LinuxPackageManagerType::Enum::Deb:
            {
                // Write debian control data file to [DataRoot]/meta/
                wstring metaFilename = wformatString(Constants::FabricUpgrade::LinuxUpgradeScriptMetaFileNameFormat, Constants::FabricUpgrade::LinuxUpgradeScript_RollbackCutoffVersion);
                string metaFilePathInArchive = formatString("./{0}", metaFilename);
                wstring metaFilePath = Path::Combine(metaDirectory, metaFilename);

                command = formatString(
                    "dpkg-deb --ctrl-tarfile '{0}' | tar --directory '{1}' -x '{2}'",
                    fabricPackagePath,
                    metaDirectory,
                    metaFilePathInArchive);

                dwErr = system(command.c_str());
                if (dwErr != ERROR_SUCCESS)
                {
                    dwErr = dwErr >> 8;
                    if (dwErr == ERROR_FILE_NOT_FOUND)
                    {
                        WriteWarning(TraceType,
                            owner_.Root.TraceId,
                            "SwapUpdaterService: deb archive {0} did not contain meta file: {1}. Assuming backcompat path; not attempting to extract Vnext upgrade script.",
                            fabricPackagePath,
                            metaFilePathInArchive);
                        return ErrorCodeValue::Success;
                    }

                    WriteError(TraceType, owner_.Root.TraceId, "SwapUpdaterService: dpkg ctrl data query failed with err {0} ; Command: {1}", dwErr, command);
                    return ErrorCode::FromWin32Error(dwErr);
                }

                error = File::MoveTransacted(metaFilePath, upgradeScriptMetaPath, true);
                if (!error.IsSuccess())
                {
                    WriteError(TraceType, owner_.Root.TraceId, "SwapUpdaterService: File Move failed with {0} ; from {1} to {2}.", error, metaFilePath, upgradeScriptMetaPath);
                    return error;
                }

                break;
            }
            case Common::LinuxPackageManagerType::Enum::Rpm:
            {
                string filenameEscaped = formatString("{0}", filename);
                StringUtility::Replace(filenameEscaped, string("."), string("\\."));

                string metaPayloadExtractPathEscaped = Constants::FabricUpgrade::MetaPayloadDefaultExtractPath;
                StringUtility::Replace(metaPayloadExtractPathEscaped, string("."), string("\\."));
                StringUtility::Replace(metaPayloadExtractPathEscaped, string("/"), string("\\/"));

                string metaFileWritePathEscaped = formatString("{0}", upgradeScriptMetaPath);
                StringUtility::Replace(metaFileWritePathEscaped, string("."), string("\\."));
                StringUtility::Replace(metaFileWritePathEscaped, string("/"), string("\\/"));

                command = formatString(
                    "rpm -qp --scripts \"{0}\" | sed '/^#FilePayload: {1}/,/^#EndFilePayload/!d;//d' | sed 's/{2}/\"{3}\"/g' | xargs -0 bash -c && chmod +x \"{4}\"",
                    fabricPackagePath,
                    filenameEscaped,
                    metaPayloadExtractPathEscaped,
                    metaFileWritePathEscaped,
                    upgradeScriptMetaPath);

                dwErr = system(command.c_str());
                if (dwErr != ERROR_SUCCESS)
                {
                    dwErr = dwErr >> 8;
                    if (dwErr == ERROR_INVALID_NAME)
                    {
                        WriteWarning(TraceType,
                            owner_.Root.TraceId,
                            "SwapUpdaterService: rpm archive {0} did not contain meta file: {1}. Assuming backcompat path; not attempting to extract Vnext upgrade script.",
                            fabricPackagePath,
                            filename);
                        return ErrorCodeValue::Success;
                    }

                    WriteError(TraceType, owner_.Root.TraceId, "SwapUpdaterService: Extract RPM meta file failed with err {0} ; Command: {1}", dwErr, command);
                    return ErrorCode::FromWin32Error(dwErr);
                }

                break;
            }
        }

        if (!File::Exists(upgradeScriptMetaPath))
        {
            WriteError(TraceType, owner_.Root.TraceId, "SwapUpdaterService: Upgrade script not found at {0}.", upgradeScriptMetaPath);
            return ErrorCodeValue::NotFound;
        }

        // Move [DataRoot]/meta/doupgrade.sh to data root base
        wstring newServiceScriptFilePath = fabricDeploymentSpec.GetUpgradeScriptFile(owner_.hosting_.NodeName);
        error = File::MoveTransacted(upgradeScriptMetaPath, newServiceScriptFilePath, true);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, owner_.Root.TraceId, "SwapUpdaterService: File Move failed with {0} ; from {1} to {2}.", error, upgradeScriptMetaPath, newServiceScriptFilePath);
            return error;
        }

        // Swap the upgrade service line to use the doupgrade.sh script of the new deb/rpm installer package
        wstring servicePath; 
        error = FabricEnvironment::GetUpdaterServicePath(servicePath);
        ASSERT_IF(!error.IsSuccess(), "GetUpdaterServicePath failed. Error: {0}", error);

        wstring tmpServicePath = Path::Combine(fabricDeploymentSpec.DataRoot, L"tmpUpdaterService.service");
        if (File::Exists(tmpServicePath))
        {
            error = File::Delete2(tmpServicePath, true);
            if (!error.IsSuccess())
            {
                WriteWarning(TraceType, owner_.Root.TraceId, "SwapUpdaterService: Deleting preexisting tmp service file failed with err {0}. Proceeding.", error);
            }
        }

        string replacementExecStart = formatString("ExecStart={0} \\$PathToTargetInfoFile", newServiceScriptFilePath);
        StringUtility::Replace(replacementExecStart, string("/"), string("\\/")); // escape slashes for sed call

        command = formatString("cat \"{0}\" | sed \"s/^ExecStart=.*/{1}/g\" > \"{2}\"", servicePath, replacementExecStart, tmpServicePath);

        dwErr = system(command.c_str());
        if (dwErr != ERROR_SUCCESS)
        {
            dwErr = dwErr >> 8;
            WriteError(TraceType, owner_.Root.TraceId, "SwapUpdaterService: Write tmp service file failed with err {0} ; Command: {1}", dwErr, command);
            return ErrorCode::FromWin32Error(dwErr);
        }

        int64 size;
        error = File::GetSize(tmpServicePath, size);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, owner_.Root.TraceId, "SwapUpdaterService: getting size of tmp service file failed with err {0}", error);
            return error;
        }
        else if (size == 0)
        {
            WriteError(TraceType, owner_.Root.TraceId, "SwapUpdaterService: tmp service file write failed. File is empty.");
            return ErrorCodeValue::OperationFailed;
        }

        command = formatString("cat \"{0}\" > \"{1}\"", tmpServicePath, servicePath);

        dwErr = system(command.c_str());
        if (dwErr != ERROR_SUCCESS)
        {
            dwErr = dwErr >> 8;
            WriteError(TraceType, owner_.Root.TraceId, "SwapUpdaterService: Rewrite service file failed with err {0} ; Command: {1}", dwErr, command);
            return ErrorCode::FromWin32Error(dwErr);
        }

        error = File::Delete2(tmpServicePath);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, owner_.Root.TraceId, "SwapUpdaterService: Deleting temporary upgrade service file failed with err {0}. Path: {1}. Proceeding.", error, tmpServicePath);
        }

        return ErrorCodeValue::Success;
    }
#endif

    void ConfigUpgrade(AsyncOperationSPtr const & thisSPtr, bool instanceIdOnlyUpgrade)
    {                
        wstring tempErrorFile = File::GetTempFileNameW(owner_.hosting_.NodeWorkFolder);
        wstring pathToClusterManifest = owner_.fabricUpgradeRunLayout_.GetClusterManifestFile(fabricUpgradeSpec_.Version.ConfigVersion.ToString());            
        
        wstring upgradeArgument;
        if(instanceIdOnlyUpgrade)
        {
            upgradeArgument = wformatString(
                Constants::FabricDeployer::InstanceIdOnlyUpgradeArguments,
                fabricUpgradeSpec_.Version.CodeVersion.ToString(),
                fabricUpgradeSpec_.InstanceId,
                owner_.hosting_.NodeName,
                tempErrorFile);
        }
        else
        {
            upgradeArgument = wformatString(
                Constants::FabricDeployer::ConfigUpgradeArguments,
                pathToClusterManifest,
                fabricUpgradeSpec_.Version.CodeVersion.ToString(),
                fabricUpgradeSpec_.InstanceId,
                owner_.hosting_.NodeName,
                tempErrorFile);
        }

        wstring deployerPath = Path::Combine(Environment::GetExecutablePath(), Constants::FabricDeployer::ExeName);

        WriteInfo(                        
            TraceType,
            owner_.Root.TraceId,
            "Begin(FabricActivatorClient FabricUpgrade): Program:{0}, Arguments:{1}",
            deployerPath,
            upgradeArgument);

        auto operation = owner_.hosting_.FabricActivatorClientObj->BeginFabricUpgrade(
            false,
            deployerPath,
            upgradeArgument,
            L"",
            [this, tempErrorFile](AsyncOperationSPtr const & operation) { this->FinishConfigUpgrade(operation, false, tempErrorFile); },
            thisSPtr);
        FinishConfigUpgrade(operation, true, tempErrorFile);
    }

    void FinishConfigUpgrade(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously, wstring const & tempErrorFile)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.hosting_.FabricActivatorClientObj->EndFabricUpgrade(operation);        
        if(!error.IsSuccess())
        {
            wstring fileContent;
            auto fileReadError = owner_.ReadFabricDeployerTempFile(tempErrorFile, fileContent);
            if(fileReadError.IsSuccess())
            {
                error = ErrorCode(error.ReadValue(), move(fileContent));
            }
            else
            {
                WriteInfo(TraceType, owner_.Root.TraceId, "End(FabricActivatorClient FabricUpgrade): Could not read FabricDeployer error file.");
            }
        }

        WriteTrace(            
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(FabricActivatorClient FabricUpgrade): {0}",
            error);

        TryComplete(operation->Parent, error);        
    }    

private:
    FabricUpgradeImpl & owner_;
    FabricVersionInstance const currentFabricVersionInstance_;
    FabricUpgradeSpecification const fabricUpgradeSpec_;
    TimeoutHelper timeoutHelper_;
};

FabricUpgradeImpl::FabricUpgradeImpl(
    ComponentRoot const & root, 
    __in HostingSubsystem & hosting)
    : RootedObject(root),
    hosting_(hosting),
    fabricUpgradeRunLayout_(hosting.FabricUpgradeDeploymentFolder)
{
}

FabricUpgradeImpl::~FabricUpgradeImpl()
{
}

AsyncOperationSPtr FabricUpgradeImpl::BeginDownload(
            FabricVersion const & fabricVersion,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
{
    return this->hosting_.DownloadManagerObj->BeginDownloadFabricUpgradePackage(
        fabricVersion,
        callback,
        parent);
}

ErrorCode FabricUpgradeImpl::EndDownload(
    AsyncOperationSPtr const & operation)
{
    return this->hosting_.DownloadManagerObj->EndDownloadFabricUpgradePackage(operation);
}

AsyncOperationSPtr FabricUpgradeImpl::BeginValidateAndAnalyze(
    FabricVersionInstance const & currentFabricVersionInstance,
    FabricUpgradeSpecification const & fabricUpgradeSpec,
    AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ValidateFabricUpgradeAsyncOperation>(
        *this,
        currentFabricVersionInstance,
        fabricUpgradeSpec,
        callback,
        parent);
}

ErrorCode FabricUpgradeImpl::EndValidateAndAnalyze(       
    __out bool & shouldCloseReplica,
    AsyncOperationSPtr const & operation)
{
    return ValidateFabricUpgradeAsyncOperation::End(shouldCloseReplica, operation);
}

AsyncOperationSPtr FabricUpgradeImpl::BeginUpgrade(
    FabricVersionInstance const & currentFabricVersionInstance,
    FabricUpgradeSpecification const & fabricUpgradeSpec,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)  
{
    return AsyncOperation::CreateAndStart<FabricUpgradeAsyncOperation>(
        *this,
        currentFabricVersionInstance,
        fabricUpgradeSpec,
        callback,
        parent);
}

ErrorCode FabricUpgradeImpl::EndUpgrade(
    AsyncOperationSPtr const & operation)
{
    return FabricUpgradeAsyncOperation::End(operation);
}

ErrorCode FabricUpgradeImpl::ReadFabricDeployerTempFile(std::wstring const & filePath, std::wstring & fileContent)
{
    File file;
    auto error = file.TryOpen(
        filePath,
        FileMode::Open,
        FileAccess::Read,
        FileShare::Read);
    if (!error.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Hosting,
            TraceType,
            Root.TraceId,
            "Failed to open file '{0}'. Error: {1}",
            filePath,
            error);
        return ErrorCode(error.ReadValue(), StringResource::Get(IDS_HOSTING_FabricDeployed_Output_Read_Failed));
    }

    int bytesRead = 0;
    int fileSize = static_cast<int>(file.size());
    vector<byte> buffer(fileSize);

    if((bytesRead = file.TryRead(reinterpret_cast<void*>(buffer.data()), fileSize)) > 0)
    {
        buffer.push_back(0);
        buffer.push_back(0);

        // skip byte-order mark
        fileContent = wstring(reinterpret_cast<wchar_t *>(&buffer[2]));
        error = ErrorCodeValue::Success;
    }
    else
    {
        TraceWarning(
            TraceTaskCodes::Hosting,
            TraceType,
            Root.TraceId,
            "Failed to read file '{0}'.",
            filePath);
        error = ErrorCode(ErrorCodeValue::OperationFailed, StringResource::Get(IDS_HOSTING_FabricDeployed_Output_Read_Failed));
    }

    file.Close2();
    File::Delete2(filePath, true /*deleteReadOnlyFiles*/);

    return error;
}



