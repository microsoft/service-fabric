// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::ImageModel;
using namespace Hosting2;
using namespace Transport;

StringLiteral const TraceType("SingleCodePackageApplicationHostProxy");

// ********************************************************************************************************************
// SingleCodePackageApplicationHostProxy::GetGetProcessAndContainerDescriptionAsyncOperation Implementation
//
class SingleCodePackageApplicationHostProxy::GetProcessAndContainerDescriptionAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    GetProcessAndContainerDescriptionAsyncOperation(
        _In_ SingleCodePackageApplicationHostProxy & owner,
        bool isContainerHost,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , processDescriptionUPtr_()
        , containerDescriptionUPtr_()
        , isContainerHost_(isContainerHost)
    {
    }

    virtual ~GetProcessAndContainerDescriptionAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        _Out_ ProcessDescriptionUPtr & processDescriptionUPtr,
        _Out_ ContainerDescriptionUPtr & containerDescriptionUPtr)
    {
        auto thisPtr = AsyncOperation::End<GetProcessAndContainerDescriptionAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            processDescriptionUPtr = move(thisPtr->processDescriptionUPtr_);
            containerDescriptionUPtr = move(thisPtr->containerDescriptionUPtr_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ASSERT_IF(
            owner_.codePackageInstance_->EntryPoint.EntryPointType == EntryPointType::DllHost,
            "SingleCodePackageApplicationHostProxy can only handle Executable entry points.");

        map<wstring, wstring> encryptedSettings;
        vector<wstring> secretStoreRefs;
        for (auto const& it : owner_.codePackageInstance_->EnvVariablesDescription.EnvironmentVariables)
        {
            if (StringUtility::AreEqualCaseInsensitive(it.Type, Constants::SecretsStoreRef))
            {
                secretStoreRefs.push_back(it.Value);
            }
        }

        if (isContainerHost_)
        {
            auto containerRepositoryCredentials = owner_.ContainerPolicies.RepositoryCredentials;
            if (StringUtility::AreEqualCaseInsensitive(containerRepositoryCredentials.Type, Constants::SecretsStoreRef))
            {
                secretStoreRefs.push_back(containerRepositoryCredentials.Password);
            }

            // harvest any secret references from the volume driver options
            auto containerVolumes = owner_.ContainerPolicies.Volumes;
            for (auto volIt : containerVolumes)
            {
                for (auto optIt : volIt.DriverOpts)
                {
                    if (StringUtility::AreEqualCaseInsensitive(optIt.Type, Constants::SecretsStoreRef))
                    {
                        secretStoreRefs.push_back(optIt.Value);
                    }
                }
            }
        }

        this->ProcessVariablesForSecretStore(thisSPtr, secretStoreRefs);
    }

private:
    void ProcessVariablesForSecretStore(
        AsyncOperationSPtr const& thisSPtr,
        vector<wstring> const& secretStoreRefs)
    {
        if (!secretStoreRefs.empty())
        {
            auto secretStoreOp = HostingHelper::BeginGetSecretStoreRef(
                owner_.Hosting,
                secretStoreRefs,
                [this](AsyncOperationSPtr const & secretStoreOp)
            {
                this->OnSecretStoreOpCompleted(secretStoreOp, false);
            },
                thisSPtr);
            OnSecretStoreOpCompleted(secretStoreOp, true);
        }
        else
        {
            CreateProcessDescription(thisSPtr, map<wstring, wstring>());
        }
    }
    void OnSecretStoreOpCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        map<wstring, wstring> decryptedSecretStoreRef;
        auto error = HostingHelper::EndGetSecretStoreRef(operation, decryptedSecretStoreRef);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "GetSecretStoreRef values returned error:{0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, move(error));
            return;
        }

        CreateProcessDescription(operation->Parent, decryptedSecretStoreRef);
    }

    ErrorCode ProcessCodePackageEnvironmentVariableValues(
        map<wstring, wstring> const& decryptedSecretStoreRef,
        _Out_ map<wstring, wstring> & encryptedSettings,
        _Inout_ EnvironmentMap & envVars)
    {
        for (auto const& envItr : owner_.codePackageInstance_->EnvVariablesDescription.EnvironmentVariables)
        {
            if (StringUtility::AreEqualCaseInsensitive(envItr.Type, Constants::Encrypted))
            {
                //Decrypt while passing it to docker or while creating process
                encryptedSettings.insert(
                    make_pair(
                        envItr.Name,
                        envItr.Value));
            }
            else if (StringUtility::AreEqualCaseInsensitive(envItr.Type, Constants::SecretsStoreRef))
            {
                auto it = decryptedSecretStoreRef.find(envItr.Value);
                if (it == decryptedSecretStoreRef.end())
                {
                    return ErrorCode(ErrorCodeValue::NotFound,
                        wformatString(StringResource::Get(IDS_HOSTING_SecretStoreReferenceDecryptionFailed), L"EnvironmentVariable", envItr.Value));
                }

                owner_.AddEnvironmentVariable(envVars, envItr.Name, it->second);
            }
            else
            {
                //Defaults to PlainText
                owner_.AddEnvironmentVariable(envVars, envItr.Name, envItr.Value);
            }
        }

        return ErrorCodeValue::Success;
    }

    void CreateProcessDescription(AsyncOperationSPtr const & thisSPtr, map<wstring, wstring> const& decryptedSecretStoreRef)
    {
        bool isExternalExecutable = owner_.codePackageInstance_->EntryPoint.ExeEntryPoint.IsExternalExecutable;

        wstring exePath;
        wstring arguments;

        exePath = owner_.codePackageInstance_->EntryPoint.ExeEntryPoint.Program;
        StringUtility::Trim<wstring>(exePath, L"'");
        StringUtility::Trim<wstring>(exePath, L"\"");
        arguments = owner_.codePackageInstance_->EntryPoint.ExeEntryPoint.Arguments;

        RunLayoutSpecification runLayout(owner_.Hosting.DeploymentFolder);
        auto applicationIdString = owner_.codePackageInstance_->Context.CodePackageInstanceId.ServicePackageInstanceId.ApplicationId.ToString();

        wstring appDirectory = runLayout.GetApplicationFolder(applicationIdString);
        wstring logDirectory = runLayout.GetApplicationLogFolder(applicationIdString);
        wstring tempDirectory = runLayout.GetApplicationTempFolder(applicationIdString);
        wstring workDirectory = runLayout.GetApplicationWorkFolder(applicationIdString);
        wstring codePackageDirectory = runLayout.GetCodePackageFolder(
            applicationIdString,
            owner_.codePackageInstance_->Context.CodePackageInstanceId.ServicePackageInstanceId.ServicePackageName,
            owner_.codePackageInstance_->Context.CodePackageInstanceId.CodePackageName,
            owner_.codePackageInstance_->Version,
            owner_.codePackageInstance_->IsPackageShared);

#if defined(PLATFORM_UNIX)
        if (isExternalExecutable)
        {
            exePath = Path::Combine(HostingConfig::GetConfig().LinuxExternalExecutablePath, exePath);
        }
        else if (!Path::IsPathRooted(exePath))
        {
            exePath = Path::Combine(codePackageDirectory, exePath);
        }
#else
        if (!Path::IsPathRooted(exePath) && !isExternalExecutable)
        {
            exePath = Path::Combine(codePackageDirectory, exePath);
        }
#endif

        // TODO: Check with Rajeet.
        // This will name is not unique per activation of service package in exlusive mode.
        wstring redirectedFileNamePrefix = wformatString(
            "{0}_{1}_{2}",
            owner_.codePackageInstance_->Context.CodePackageInstanceId.CodePackageName,
            owner_.codePackageInstance_->Context.CodePackageInstanceId.ServicePackageInstanceId.ServicePackageName,
            owner_.codePackageInstance_->IsSetupEntryPoint ? L"S" : L"M");


        wstring startInDirectory;
        switch (owner_.codePackageInstance_->EntryPoint.ExeEntryPoint.WorkingFolder)
        {
        case ExeHostWorkingFolder::Work:
            startInDirectory = workDirectory;
            break;
        case ExeHostWorkingFolder::CodePackage:
            startInDirectory = codePackageDirectory;
            break;
        case ExeHostWorkingFolder::CodeBase:
            startInDirectory = Path::GetDirectoryName(exePath);
            break;
        default:
            Assert::CodingError("Invalid WorkingFolder value {0}", static_cast<int>(owner_.codePackageInstance_->EntryPoint.ExeEntryPoint.WorkingFolder));
        }

        EnvironmentMap envVars;
        if (!owner_.removeServiceFabricRuntimeAccess_) // This if check is just an optimization for not doing this work if runtime access is not allowed for this host.
        {
            if (owner_.codePackageInstance_->EntryPoint.EntryPointType != EntryPointType::ContainerHost)
            {
                owner_.GetCurrentProcessEnvironmentVariables(envVars, tempDirectory, owner_.Hosting.FabricBinFolder);
            }
            //Add folders created for application.
            owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Work", Constants::EnvironmentVariable::FoldersPrefix), workDirectory);
            owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Log", Constants::EnvironmentVariable::FoldersPrefix), logDirectory);
            owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Temp", Constants::EnvironmentVariable::FoldersPrefix), tempDirectory);
            owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}Application", Constants::EnvironmentVariable::FoldersPrefix), appDirectory);
            owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}Application_OnHost", Constants::EnvironmentVariable::FoldersPrefix), appDirectory);

            auto error = owner_.AddHostContextAndRuntimeConnection(envVars);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceType,
                    owner_.TraceId,
                    "AddHostContextAndRuntimeConnection: ErrorCode={0}",
                    error);
                TryComplete(thisSPtr, move(error));
                return;
            }

            owner_.codePackageInstance_->Context.ToEnvironmentMap(envVars);

            //Add all the ActivationContext, NodeContext environment variables
            for (auto it = owner_.codePackageInstance_->EnvContext->Endpoints.begin(); it != owner_.codePackageInstance_->EnvContext->Endpoints.end(); it++)
            {
                owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}{1}", Constants::EnvironmentVariable::EndpointPrefix, (*it)->Name), StringUtility::ToWString((*it)->Port));
                owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}{1}", Constants::EnvironmentVariable::EndpointIPAddressOrFQDNPrefix, (*it)->Name), (*it)->IpAddressOrFqdn);
            }

            owner_.AddSFRuntimeEnvironmentVariable(envVars, Constants::EnvironmentVariable::NodeIPAddressOrFQDN, owner_.Hosting.FabricNodeConfigObj.IPAddressOrFQDN);

            // Add partition id even when no runtime is being shared when host is SFBlockstore service.
            // if (StringUtility::AreEqualCaseInsensitive(codePackageInstance_->Name, Constants::BlockStoreServiceCodePackageName))
            {
                // ActivationContext is only available for Exclusive Hosting mode.
                if (owner_.ServicePackageInstanceId.ActivationContext.IsExclusive)
                {
                    owner_.AddSFRuntimeEnvironmentVariable(envVars, Constants::EnvironmentVariable::PartitionId, owner_.ServicePackageInstanceId.ActivationContext.ToString());
                }
            }
        }

        //
        // These environment variables are provided by the code package activator(default or custom - like sf block store service).
        // They dont provide any information that can enable accessing the SF runtime, so they can be set always.
        //
        for (auto const & kvPair : owner_.codePackageInstance_->CodePackageObj.OnDemandActivationEnvironment)
        {
            owner_.AddEnvironmentVariable(envVars, kvPair.first, kvPair.second);
        }

        //Add the env vars in ServiceManifest
        map<wstring, wstring> encryptedSettings;
        auto result = ProcessCodePackageEnvironmentVariableValues(decryptedSecretStoreRef, encryptedSettings, envVars);
        if (!result.IsSuccess())
        {
            TryComplete(thisSPtr, move(result));
            return;
        }

        //Add the env vars for ConfigPackagePolicies
        for (auto const& it : owner_.codePackageInstance_->CodePackageObj.EnvironmentVariableForMounts)
        {
            owner_.AddEnvironmentVariable(envVars, it.first, it.second);
        }

        auto serviceName = owner_.GetServiceName();
        if (!serviceName.empty())
        {
            owner_.AddSFRuntimeEnvironmentVariable(envVars, Constants::EnvironmentVariable::ServiceName, serviceName);
        }

        if (owner_.codePackageInstance_->EntryPoint.EntryPointType == EntryPointType::ContainerHost)
        {
            //Get the container image name based on Os build version
            wstring imageName = owner_.codePackageInstance_->EntryPoint.ContainerEntryPoint.ImageName;

            auto error = ContainerHelper::GetContainerHelper().GetContainerImageName(owner_.codePackageInstance_->ContainerPolicies.ImageOverrides, imageName);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceType,
                    owner_.TraceId,
                    "GetContainerImageName({0}, {1}): ErrorCode={2}",
                    owner_.codePackageInstance_->ContainerPolicies.ImageOverrides,
                    imageName,
                    error);
                TryComplete(thisSPtr, move(error));
                return;
            }

            exePath = imageName;
            arguments = owner_.codePackageInstance_->EntryPoint.ContainerEntryPoint.Commands;

#if !defined(PLATFORM_UNIX)
            //Windows containers only allow mapping volumes to c: today
            //so we need to fix up path for each directory;
            auto appDirectoryOnContainer = HostingConfig::GetConfig().GetContainerApplicationFolder(applicationIdString);

            owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Work", Constants::EnvironmentVariable::FoldersPrefix), Path::Combine(appDirectoryOnContainer, L"work"));
            owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Log", Constants::EnvironmentVariable::FoldersPrefix), Path::Combine(appDirectoryOnContainer, L"log"));
            owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Temp", Constants::EnvironmentVariable::FoldersPrefix), Path::Combine(appDirectoryOnContainer, L"temp"));
            owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}Application", Constants::EnvironmentVariable::FoldersPrefix), appDirectoryOnContainer);
#endif

            if (!owner_.removeServiceFabricRuntimeAccess_)
            {
#if !defined(PLATFORM_UNIX)

                wstring certificateFolderPath;
                wstring certificateFolderName;
                auto passwordMap = owner_.codePackageInstance_->EnvContext->CertificatePasswordPaths;
                for (auto const & iter : owner_.codePackageInstance_->EnvContext->CertificatePaths)
                {
                    if (certificateFolderPath.empty())
                    {
                        certificateFolderName = Path::GetFileName(Path::GetDirectoryName(iter.second));
                        certificateFolderPath = Path::Combine(Path::Combine(appDirectoryOnContainer, L"work"), certificateFolderName);
                    }
                    owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}_{1}_PFX", certificateFolderName, iter.first), Path::Combine(certificateFolderPath, Path::GetFileName(iter.second)));
                    owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}_{1}_Password", certificateFolderName, iter.first), Path::Combine(certificateFolderPath, Path::GetFileName(passwordMap[iter.first])));
                }
#else
                wstring certificateFolderPath;
                wstring certificateFolderName;
                auto passwordMap = owner_.codePackageInstance_->EnvContext->CertificatePasswordPaths;
                for (auto const & iter : owner_.codePackageInstance_->EnvContext->CertificatePaths)
                {
                    if (certificateFolderPath.empty())
                    {
                        certificateFolderName = Path::GetFileName(Path::GetDirectoryName(iter.second));
                        certificateFolderPath = Path::Combine(workDirectory, certificateFolderName);
                    }
                    owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}_{1}_PEM", certificateFolderName, iter.first), Path::Combine(certificateFolderPath, Path::GetFileName(iter.second)));
                    owner_.AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}_{1}_PrivateKey", certificateFolderName, iter.first), Path::Combine(certificateFolderPath, Path::GetFileName(passwordMap[iter.first])));
                }
#endif

                wstring configStoreEnvVarName;
                wstring configStoreEnvVarValue;
                error = ComProxyConfigStore::FabricGetConfigStoreEnvironmentVariable(configStoreEnvVarName, configStoreEnvVarValue);
                if (error.IsSuccess())
                {
                    if (!configStoreEnvVarName.empty())
                    {
                        owner_.AddSFRuntimeEnvironmentVariable(envVars, configStoreEnvVarName, configStoreEnvVarValue);
                    }
                }
            }
        }

        ProcessDebugParameters debugParameters(owner_.codePackageInstance_->DebugParameters);
        if (HostingConfig::GetConfig().EnableProcessDebugging)
        {
            if (!owner_.codePackageInstance_->DebugParameters.WorkingFolder.empty())
            {
                startInDirectory = owner_.codePackageInstance_->DebugParameters.WorkingFolder;
            }

            if (!owner_.codePackageInstance_->DebugParameters.DebugParametersFile.empty())
            {
                File debugParametersFile;
                auto error = debugParametersFile.TryOpen(owner_.codePackageInstance_->DebugParameters.DebugParametersFile);
                if (error.IsSuccess())
                {
                    int fileSize = static_cast<int>(debugParametersFile.size());
                    ByteBufferUPtr json = make_unique<ByteBuffer>();
                    json->resize(fileSize);
                    DWORD bytesRead;
                    error = debugParametersFile.TryRead2(reinterpret_cast<void*>((*json).data()), fileSize, bytesRead);
                    debugParametersFile.Close();
                    json->resize(bytesRead);
                    if (error.IsSuccess())
                    {
                        ProcessDebugParametersJson p;
                        error = JsonHelper::Deserialize(p, json);
                        if (error.IsSuccess())
                        {
                            debugParameters.set_Arguments(p.Arguments);
                            debugParameters.set_ExePath(p.ExePath);
                        }
                    }
                }
            }
        }

        ServiceModel::ResourceGovernancePolicyDescription resourceGovernancePolicy(owner_.codePackageInstance_->ResourceGovernancePolicy);
        ServiceModel::ServicePackageResourceGovernanceDescription spResourceGovernanceDescription(owner_.codePackageInstance_->CodePackageObj.VersionedServicePackageObj.PackageDescription.ResourceGovernanceDescription);

        bool isImplicitCodePackage = owner_.codePackageInstance_->CodePackageObj.IsImplicitTypeHost;
        // Depending on the platform (Windows/Linux) and CP type (container/process) we need to adjust affinity and CPU shares.
        // On Linux, we use cpu quota and period and we also need to pass the cgroupName to use for this code package
        // We should not be adjusting limits for fabric type host nor for setup entry point

        wstring cgroupName = L"";
        bool isContainerGroup = owner_.codePackageInstance_->EnvContext->SetupContainerGroup;

        if (!isImplicitCodePackage && !owner_.codePackageInstance_->IsSetupEntryPoint)
        {
            owner_.Hosting.LocalResourceManagerObj->AdjustCPUPoliciesForCodePackage(owner_.codePackageInstance_->CodePackageObj, resourceGovernancePolicy, owner_.Context.IsContainerHost, isContainerGroup);
#if defined(PLATFORM_UNIX)
            //do not pass cgroup name in case this is a container which is not member of a container group or the container group does not have service package rg limits
            if ((resourceGovernancePolicy.ShouldSetupCgroup() || spResourceGovernanceDescription.IsGoverned) && !HostingConfig::GetConfig().LocalResourceManagerTestMode && (!owner_.Context.IsContainerHost || (isContainerGroup && spResourceGovernanceDescription.IsGoverned)))
            {
                //important to add / because of way docker handles cgroup parent name
                cgroupName = L"/" + Constants::CgroupPrefix + L"/" + owner_.codePackageInstance_->CodePackageInstanceId.ServicePackageInstanceId.ToString();
                //if this is a container group just pass the parent here else append code package name
                if (!owner_.Context.IsContainerHost)
                {
                    cgroupName = cgroupName + L"/" + owner_.codePackageInstance_->CodePackageInstanceId.CodePackageName;
                }
            }
#endif

        }

        // For blockstore service set workdir same as exe location
        if (StringUtility::AreEqualCaseInsensitive(owner_.codePackageInstance_->Name, Constants::BlockStoreServiceCodePackageName))
        {
            startInDirectory = Path::GetDirectoryName(exePath);
        }

        // Container network environment variables
        if (owner_.codePackageInstance_->EntryPoint.EntryPointType == EntryPointType::ContainerHost)
        {
            owner_.networkConfig_.ToEnvironmentMap(envVars);
        }
        
        processDescriptionUPtr_ = make_unique<ProcessDescription>(
            exePath,
            arguments,
            startInDirectory,
            envVars,
            appDirectory,
            tempDirectory,
            workDirectory,
            logDirectory,
            HostingConfig::GetConfig().EnableActivateNoWindow,
            owner_.codePackageInstance_->EntryPoint.ExeEntryPoint.ConsoleRedirectionEnabled,
            false,
            false,
            redirectedFileNamePrefix,
            owner_.codePackageInstance_->EntryPoint.ExeEntryPoint.ConsoleRedirectionFileRetentionCount,
            owner_.codePackageInstance_->EntryPoint.ExeEntryPoint.ConsoleRedirectionFileMaxSizeInKb,
            debugParameters,
            resourceGovernancePolicy,
            spResourceGovernanceDescription,
            cgroupName,
            false,
            encryptedSettings);

        CreateContainerDescription(thisSPtr, decryptedSecretStoreRef);
    }

    void CreateContainerDescription(AsyncOperationSPtr const& thisSPtr, map<wstring, wstring> const& decryptedSecretStoreRef)
    {
        // DNS chain section for containers.
        vector<wstring> dnsServers;
        if (isContainerHost_)
        {
            auto error = owner_.GetContainerDnsServerConfiguration(owner_.networkConfig_, dnsServers);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "Failed to create DNS chain for the container : ErrorCode={0}",
                    error);
                error = ErrorCodeValue::ContainerFailedToCreateDnsChain;
                TryComplete(thisSPtr, error);
                return;
            }
        }

        auto containerPolicies = owner_.ContainerPolicies;

        vector<wstring> securityOptions;
        if (!containerPolicies.SecurityOptions.empty())
        {
            for (auto it = containerPolicies.SecurityOptions.begin(); it != containerPolicies.SecurityOptions.end(); ++it)
            {
                securityOptions.push_back(it->Value);
            }
        }

        bool autoRemove = false;
        if (containerPolicies.AutoRemove.empty())
        {
            if (containerPolicies.ContainersRetentionCount != 0)
            {
                WriteInfo(
                    TraceType,
                    owner_.TraceId,
                    "Containerretentioncount {0} failure count {1}",
                    containerPolicies.ContainersRetentionCount,
                    owner_.codePackageInstance_->ContinuousFailureCount);

                //If retentioncount is negative, we keep retaining all failed containers
                if (containerPolicies.ContainersRetentionCount > 0)
                {
                    if (owner_.codePackageInstance_->ContinuousFailureCount >= (ULONG)containerPolicies.ContainersRetentionCount)
                    {
                        autoRemove = true;
                    }
                }
            }
            else
            {
                autoRemove = true;
            }
        }
        else
        {
            StringUtility::TryFromWString<bool>(containerPolicies.AutoRemove, autoRemove);
        }

        std::vector<ContainerLabelDescription> newLabels(containerPolicies.Labels.size());
        if (containerPolicies.Labels.size() > 0)
        {
            std::copy(containerPolicies.Labels.begin(), containerPolicies.Labels.end(), newLabels.begin());
        }

        auto logConfig = containerPolicies.LogConfig;

        if (isContainerHost_)
        {
            GetLogConfigAndLogLabel(logConfig, newLabels);
        }

#if defined(PLATFORM_UNIX)
        ContainerPodDescription podDesc(L"",
            owner_.codePackageInstance_->EnvContext->ContainerGroupIsolated ?
            ContainerIsolationMode::hyperv : ContainerIsolationMode::process,
            vector<PodPortMapping>());
#endif

        auto error = ProcessRepositoryCredentials(decryptedSecretStoreRef, containerPolicies.RepositoryCredentials);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Failed to process repository credentials secret references for the container : ErrorCode={0}",
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        // process any volume driver options
        error = ProcessVolumeDriverSecurityOptions(decryptedSecretStoreRef, containerPolicies.Volumes);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Failed to process volume driver option secret references for the container : ErrorCode={0}",
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        auto containerName = owner_.GetUniqueContainerName();
        error = CreateContainerTracesFolder(containerName);
        if (!error.IsSuccess())
        {
            CreateContainerTracesFolder(containerName, true /*cleanup*/).ReadValue();
        }
        else
        {
            containerDescriptionUPtr_ = make_unique<ContainerDescription>(
                owner_.ApplicationName,
                owner_.GetServiceName(),
                owner_.ApplicationId.ToString(),
                containerName,
                owner_.codePackageInstance_->EntryPoint.ContainerEntryPoint.EntryPoint,
                containerPolicies.Isolation,
                containerPolicies.Hostname,
                owner_.Hosting.DeploymentFolder,
                owner_.Hosting.NodeWorkFolder,
                owner_.PortBindings,
                logConfig,
                containerPolicies.Volumes,
                newLabels,
                dnsServers,
                containerPolicies.RepositoryCredentials,
                containerPolicies.HealthConfig,
                owner_.networkConfig_,
                securityOptions,
#if defined(PLATFORM_UNIX)
                podDesc,
#endif
                owner_.removeServiceFabricRuntimeAccess_,
                owner_.codePackageInstance_->EnvContext->GroupContainerName,
                containerPolicies.UseDefaultRepositoryCredentials,
                containerPolicies.UseTokenAuthenticationCredentials,
                autoRemove,
                containerPolicies.RunInteractive,
                false /*isContainerRoot*/,
                owner_.codePackageInstance_->Context.CodePackageInstanceId.CodePackageName,
                owner_.ServicePackageInstanceId.PublicActivationId,
                owner_.ServicePackageInstanceId.ActivationContext.ToString(),
                owner_.codePackageInstance_->CodePackageObj.BindMounts);
        }

        TryComplete(thisSPtr, error);
    }

    ErrorCode GetLogConfigAndLogLabel(
        _Out_ LogConfigDescription & logConfig,
        _Out_ std::vector<ContainerLabelDescription> & newLabels)
    {
        auto containerPolicies = owner_.ContainerPolicies;
        LogConfigDescription tempLogConfig;

        auto error = GetLogConfigDescription(containerPolicies.LogConfig, tempLogConfig);

        if (error.IsSuccess())
        {
            // If the log driver is the sblogdriver then we want to add a label to the container containing the value.
            logConfig = move(tempLogConfig);
            for (auto& it : logConfig.DriverOpts)
            {
                if (it.Name == Constants::ContainerLogDriverOptionLogBasePathKey)
                {
                    ContainerLabelDescription logRootPathLabel;
                    logRootPathLabel.Name = Constants::ContainerLabels::LogBasePathKeyName;
                    logRootPathLabel.Value = it.Value;
                    newLabels.push_back(logRootPathLabel);
                }
            }
        }
        else
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Failed to parse log driver config. Proceeding without making changes to the logConfig.");
        }
        return ErrorCode::Success();
    }

    ErrorCode GetLogConfigDescription(
        LogConfigDescription const & appLogConfig,
        _Out_ LogConfigDescription & logConfig)
    {
        // logConfig will be populated with appLogConfig if log driver is specified in appLogConfig, otherwise it will be populated from ContainerLogDriverConfig
        if (!appLogConfig.Driver.empty())
        {
            // populate from appLogConfig because it is not empty or the cluster manifest has no default
            logConfig = appLogConfig;
        }
        else if (!Hosting2::ContainerLogDriverConfig::GetConfig().Type.empty())
        {
            // populate from ContainerLogDriverConfig
            logConfig.Driver = Hosting2::ContainerLogDriverConfig::GetConfig().Type;

            ServiceModel::ContainerLogDriverConfigDescription logDriverOpts;

            auto error = JsonHelper::Deserialize(logDriverOpts, Hosting2::ContainerLogDriverConfig::GetConfig().Config);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "GetLogConfigDescription failed to deserialize log driver options from ContainerLogDriver.Config {0}. Returning with error {1}.",
                    Hosting2::ContainerLogDriverConfig::GetConfig().Config,
                    error);

                return error;
            }

            WriteInfo(
                TraceType,
                owner_.TraceId,
                "GetlogConfigDescription successfully deserialized ContainerLogDriver.Config. Will proceed to use log driver configs from this value {0}.",
                Hosting2::ContainerLogDriverConfig::GetConfig().Config);

            for (auto& it : logDriverOpts.Config)
            {
                DriverOptionDescription driverOption;
                driverOption.Name = it.first;   // key
                driverOption.Value = it.second; // value
                driverOption.IsEncrypted = L"false";

                logConfig.DriverOpts.push_back(move(driverOption));
            }
        }
        else
        {
            logConfig = owner_.ContainerPolicies.LogConfig;
        }

        return ErrorCode::Success();
    }

    ErrorCode ProcessRepositoryCredentials(
        map<wstring, wstring> const& decryptedSecretStoreRef,
        _Inout_ RepositoryCredentialsDescription & repositoryCredential)
    {
        if (StringUtility::AreEqualCaseInsensitive(repositoryCredential.Type, Constants::SecretsStoreRef))
        {
            auto it = decryptedSecretStoreRef.find(repositoryCredential.Password);
            if (it == decryptedSecretStoreRef.end())
            {
                return ErrorCode(ErrorCodeValue::NotFound,
                    wformatString(StringResource::Get(IDS_HOSTING_SecretStoreReferenceDecryptionFailed), L"RepositoryCredentials", repositoryCredential.Password));
            }

            repositoryCredential.Password = it->second;
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode  ProcessVolumeDriverSecurityOptions(
        map<wstring, wstring> const& decryptedSecretStoreRef,
        _Inout_ std::vector<ContainerVolumeDescription>& volumeDescriptions)
    {
        // for each volume description
        //  for each volume driver option
        //   if type is secret reference
        //    replace reference with secret value
        for (std::vector<ContainerVolumeDescription>::iterator volIt = volumeDescriptions.begin();
            volIt != volumeDescriptions.end();
            ++volIt)
        {
            for (std::vector<DriverOptionDescription>::iterator optionIt = volIt->DriverOpts.begin();
                optionIt != volIt->DriverOpts.end();
                ++optionIt)
            {
                if (!StringUtility::AreEqualCaseInsensitive(optionIt->Type, Constants::SecretsStoreRef))
                    continue;

                auto secretIt = decryptedSecretStoreRef.find(optionIt->Value);
                if (secretIt == decryptedSecretStoreRef.end())
                {
                    return ErrorCode(
                        ErrorCodeValue::NotFound,
                        wformatString(StringResource::Get(IDS_HOSTING_SecretStoreReferenceDecryptionFailed), L"VolumeDriverOptions", optionIt->Name));
                }

                optionIt->Value = secretIt->second;
            }
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode CreateContainerTracesFolder(wstring containerName, bool isCleanup = false)
    {
        if (!owner_.removeServiceFabricRuntimeAccess_ && isContainerHost_)
        {
            wstring fabricLogRoot;
            auto error = FabricEnvironment::GetFabricLogRoot(fabricLogRoot);
            if (!error.IsSuccess())
            {
                return error;
            }

            wstring fabricContainerLogRoot = Path::Combine(fabricLogRoot, L"Containers");

            if (isCleanup)
            {
                if (Directory::Exists(fabricContainerLogRoot))
                {
                    wstring fabricContainerRoot = Path::Combine(fabricContainerLogRoot, containerName);
                    if (Directory::Exists(fabricContainerRoot))
                    {
                        error = Directory::Delete(fabricContainerRoot, true /*recursive*/);
                        if (!error.IsSuccess())
                        {
                            WriteWarning(
                                TraceType,
                                owner_.TraceId,
                                "Failed to cleanup container log runtime directory:{0} error:{1}",
                                fabricContainerRoot,
                                error);
                        }
                    }
                }

                return ErrorCodeValue::Success;
            }

            wstring fabricContainerRoot = Path::Combine(fabricContainerLogRoot, containerName);
            error = Directory::Create2(fabricContainerRoot);
            if (!error.IsSuccess())
            {
                return error;
            }

            wstring fabricContainerTraceRoot = Path::Combine(fabricContainerRoot, L"Traces");
            error = Directory::Create2(fabricContainerTraceRoot);
            if (!error.IsSuccess())
            {
                return error;
            }

            wstring fabricContainerQueryTraceRoot = Path::Combine(fabricContainerRoot, L"QueryTraces");
            error = Directory::Create2(fabricContainerQueryTraceRoot);
            if (!error.IsSuccess())
            {
                return error;
            }

            wstring fabricContainerOperationalTracesRoot = Path::Combine(fabricContainerRoot, L"OperationalTraces");
            error = Directory::Create2(fabricContainerOperationalTracesRoot);
            if (!error.IsSuccess())
            {
                return error;
            }
        }	

        return ErrorCodeValue::Success;
    }

private:
    SingleCodePackageApplicationHostProxy & owner_;
    ProcessDescriptionUPtr processDescriptionUPtr_;
    ContainerDescriptionUPtr containerDescriptionUPtr_;
    bool isContainerHost_;
};

// ********************************************************************************************************************
// SingleCodePackageApplicationHostProxy::OpenAsyncOperation Implementation
//
class SingleCodePackageApplicationHostProxy::OpenAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        __in SingleCodePackageApplicationHostProxy & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        processPath_(),
        isContainerHost_(owner_.EntryPointType == EntryPointType::Enum::ContainerHost)
    {
    }

    virtual ~OpenAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Opening ApplicationHostProxy: Id={0}, Type={1}, Timeout={2}",
            owner_.HostId,
            owner_.HostType,
            timeoutHelper_.GetRemainingTime());

        // Container network configuration
        if (isContainerHost_)
        {
            wstring assignedIpAddresses;
            auto error = owner_.GetContainerNetworkConfigDescription(owner_.networkConfig_, assignedIpAddresses);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "GetContainerNetworkConfigDescription: ErrorCode={0}",
                    error);
                TryComplete(thisSPtr, error);
                return;
            }

            WriteNoise(
                TraceType,
                owner_.TraceId,
                "GetContainerNetworkConfigDescription: AssignedIps={0}",
                assignedIpAddresses);
        }

        GetProcessAndContainerDescription(thisSPtr);
    }

private:

    void GetProcessAndContainerDescription(AsyncOperationSPtr const & thisSPtr)
    {
        auto getProcessAsyncOp = AsyncOperation::CreateAndStart<GetProcessAndContainerDescriptionAsyncOperation>(
            owner_,
            isContainerHost_,
            [this](AsyncOperationSPtr const & getProcessAsyncOp)
        {
            this->OnGetProcessAndContainerDescriptionCompleted(getProcessAsyncOp, false);
        },
            thisSPtr);

        OnGetProcessAndContainerDescriptionCompleted(getProcessAsyncOp, true);
    }

    void OnGetProcessAndContainerDescriptionCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ProcessDescriptionUPtr processDescription;
        ContainerDescriptionUPtr containerDescription;
        auto error = GetProcessAndContainerDescriptionAsyncOperation::End(operation, processDescription, containerDescription);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "GetProcessDescriptionFailed with error:{0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, move(error));
            return;
        }

        ActivateProcess(operation->Parent, move(processDescription), move(containerDescription));
    }

    void ActivateProcess(AsyncOperationSPtr const & thisSPtr, ProcessDescriptionUPtr && processDescription, ContainerDescriptionUPtr && containerDescription)
    {
        processPath_ = processDescription->ExePath;

        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ProcessActivate): HostId={0}, HostType={1}, Timeout={2}",
            owner_.HostId,
            owner_.HostType,
            timeout);
        auto operation = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->BeginActivateProcess(
            owner_.ApplicationId.ToString(),
            owner_.HostId,
            processDescription,
            owner_.RunAsId,
            isContainerHost_,
            move(containerDescription),
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnProcessActivated(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishActivateProcess(operation);
        }
    }

    void OnProcessActivated(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishActivateProcess(operation);
        }
    }

    void FinishActivateProcess(AsyncOperationSPtr const & operation)
    {
        DWORD processId;
        auto error = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->EndActivateProcess(operation, processId);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(ProcessActivate): ErrorCode={0}, HostId={1}",
            error,
            owner_.HostId);

        if (error.IsSuccess())
        {
            owner_.codePackageRuntimeInformation_ = make_shared<CodePackageRuntimeInformation>(processPath_, processId);
        }

        owner_.activationFailed_.store(!error.IsSuccess());
        owner_.activationFailedError_ = error;

        this->TryComplete(operation->Parent, error);
    }

private:
    SingleCodePackageApplicationHostProxy & owner_;
    TimeoutHelper const timeoutHelper_;
    std::wstring processPath_;
    bool isContainerHost_;
};

// ********************************************************************************************************************
// ApplicationHostProxy::CloseAsyncOperation Implementation
//
class SingleCodePackageApplicationHostProxy::CloseAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        __in SingleCodePackageApplicationHostProxy & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~CloseAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Closing SingleCodePackageApplicationHostProxy: Id={0}, Type={1}, Timeout={2}",
            owner_.HostId,
            owner_.HostType,
            timeout);

        if (!owner_.terminatedExternally_.load())
        {
            owner_.exitCode_.store(ProcessActivator::ProcessDeactivateExitCode);

            auto operation = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->BeginDeactivateProcess(
                owner_.HostId,
                timeout,
                [this](AsyncOperationSPtr const & operation) { this->OnProcessTerminated(operation); },
                thisSPtr);

            if (operation->CompletedSynchronously)
            {
                FinishTerminateProcess(operation);
            }

        }
        else
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "terminatedExternally_ is true. Host process already closed.");

            owner_.NotifyTermination();

            auto error = ErrorCode(ErrorCodeValue::Success);
            TryComplete(thisSPtr, error);
        }
    }

private:
    void OnProcessTerminated(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishTerminateProcess(operation);
        }
    }

    void FinishTerminateProcess(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->EndDeactivateProcess(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "ApplicationHostProxy Closed. ErrorCode={0}",
            error);

        if (error.IsSuccess())
        {
            owner_.NotifyTermination();
        }

        TryComplete(operation->Parent, error);
    }

private:
    SingleCodePackageApplicationHostProxy & owner_;
    TimeoutHelper const timeoutHelper_;
};

// ********************************************************************************************************************
// SingleCodePackageApplicationHostProxy::ApplicationHostCodePackageOperation Implementation
//
class SingleCodePackageApplicationHostProxy::ApplicationHostCodePackageOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    ApplicationHostCodePackageOperation(
        __in SingleCodePackageApplicationHostProxy & owner,
        ApplicationHostCodePackageOperationRequest const & request,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , request_(request)
    {
    }

    static ErrorCode ApplicationHostCodePackageOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ApplicationHostCodePackageOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.Hosting.State.Value > FabricComponentState::Opened ||
            owner_.State.Value > FabricComponentState::Opened ||
            owner_.ShouldNotify() == false)
        {
            //
            // No need to proceed if Hosting or ApplicationHostProxy
            // is closing or ApplicationHost has terminated.
            //
            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
            return;
        }

        auto operation = owner_.codePackageInstance_->BeginApplicationHostCodePackageOperation(
            request_,
            [this](AsyncOperationSPtr const & operation)
            {
            this->OnApplicationHostCodePackageOperationCompleted(operation, false);
            },
            thisSPtr);

        this->OnApplicationHostCodePackageOperationCompleted(operation, true);
    }

private:

    void OnApplicationHostCodePackageOperationCompleted(
        AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.codePackageInstance_->EndApplicationHostCodePackageOperation(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "OnApplicationHostCodePackageOperationCompleted: ErrorCode={0}",
            error);

        this->TryComplete(operation->Parent, error);
    }

private:
    SingleCodePackageApplicationHostProxy & owner_;
    ApplicationHostCodePackageOperationRequest request_;
};

// ********************************************************************************************************************
// SingleCodePackageApplicationHostProxy::CodePackageEventNotificationAsyncOperation Implementation
//
class SingleCodePackageApplicationHostProxy::SendDependentCodePackageEventAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(SendDependentCodePackageEventAsyncOperation)

public:
    SendDependentCodePackageEventAsyncOperation(
        __in SingleCodePackageApplicationHostProxy & owner,
        CodePackageEventDescription const & eventDescription,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , eventDescription_(eventDescription)
        , timeoutHelper_(HostingConfig::GetConfig().RequestTimeout)
    {
    }

    virtual ~SendDependentCodePackageEventAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<SendDependentCodePackageEventAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->SendEvent(thisSPtr);
    }

private:
    void SendEvent(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(SendDependentCodePackageEvent): AppHostContext={0}, CodePackageInstanceId={1}, InstanceId={2}, Event={3}, Timeout={4}",
            owner_.Context,
            owner_.CodePackageInstanceId,
            owner_.codePackageInstance_->InstanceId,
            eventDescription_,
            timeoutHelper_.GetRemainingTime());

        auto request = this->CreateNotificationRequestMessage();

        auto operation = owner_.Hosting.IpcServerObj.BeginRequest(
            move(request),
            owner_.HostId,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            { 
                this->FinishSendEvent(operation, false);
            },
            thisSPtr);
        
        this->FinishSendEvent(operation, true);
    }

    void FinishSendEvent(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.Hosting.IpcServerObj.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "End(SendDependentCodePackageEvent): AppHostContext={0}, CodePackageInstanceId={1}, InstanceId={2}, Event={3}, ErrorCode={4}",
                owner_.Context,
                owner_.CodePackageInstanceId,
                owner_.codePackageInstance_->InstanceId,
                eventDescription_,
                error);

            this->TryComplete(operation->Parent, error);

            if (!error.IsError(ErrorCodeValue::NotFound))
            {
                owner_.Abort();
            }

            return;
        }

        CodePackageEventNotificationReply replyBody;
        if (!reply->GetBody<CodePackageEventNotificationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);

            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<CodePackageEventNotificationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            this->TryComplete(operation->Parent, error);
            return;
        }

        error = replyBody.Error;

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "CodePackageEventNotificationReply: AppHostContext={0}, CodePackageInstanceId={1}, ReplyBody={2}.",
            owner_.Context,
            owner_.CodePackageInstanceId,
            replyBody);

        this->TryComplete(operation->Parent, error);
    }

    MessageUPtr CreateNotificationRequestMessage()
    {
        CodePackageEventNotificationRequest requestBody(eventDescription_);

        auto request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHost));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::CodePackageEventNotification));

        return move(request);
    }

private:
    SingleCodePackageApplicationHostProxy & owner_;
    CodePackageEventDescription eventDescription_;
    TimeoutHelper timeoutHelper_;
};

SingleCodePackageApplicationHostProxy::SingleCodePackageApplicationHostProxy(
    HostingSubsystemHolder const & hostingHolder,
    wstring const & hostId,
    ApplicationHostIsolationContext const & isolationContext,
    CodePackageInstanceSPtr const & codePackageInstance)
    : ApplicationHostProxy(
        hostingHolder,
        ApplicationHostContext(
            hostId,
            ApplicationHostType::Activated_SingleCodePackage,
            codePackageInstance->IsContainerHost,
            codePackageInstance->IsActivator),
        isolationContext,
        codePackageInstance->RunAsId,
        codePackageInstance->CodePackageInstanceId.ServicePackageInstanceId,
        codePackageInstance->EntryPoint.EntryPointType,
        codePackageInstance->CodePackageObj.RemoveServiceFabricRuntimeAccess)
    , codePackageActivationId_(hostId)
    , codePackageInstance_(codePackageInstance)
    , codePackageRuntimeInformation_()
    , networkConfig_()
    , shouldNotify_(true)
    , notificationLock_()
    , terminatedExternally_(false)
    , activationFailed_(false)
    , activationFailedError_()
    , isCodePackageActive_(false)
    , exitCode_(1)
    , procHandle_()
{
}

// ********************************************************************************************************************
// SingleCodePackageApplicationHostProxy::GetContainerLogsAsyncOperation Implementation
//
class SingleCodePackageApplicationHostProxy::GetContainerInfoAsyncOperation : public AsyncOperation
{
public:
    GetContainerInfoAsyncOperation(
        __in SingleCodePackageApplicationHostProxy & owner,
        wstring const & containerInfoType,
        wstring const & containerInfoArgs,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        containerInfoType_(containerInfoType),
        containerInfoArgs_(containerInfoArgs),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode GetContainerInfoAsyncOperation::End(AsyncOperationSPtr const & operation, __out wstring & containerInfo)
    {
        auto thisPtr = AsyncOperation::End<GetContainerInfoAsyncOperation>(operation);
        containerInfo = thisPtr->containerInfo_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "SingleCodePackageApplicationHostProxy: Sending get container info ApplicationHostProxy service. {0}",
            timeoutHelper_.GetRemainingTime());

        auto operation = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->BeginGetContainerInfo(
            owner_.HostId,
            owner_.ServicePackageInstanceId.ActivationContext.IsExclusive,
            containerInfoType_,
            containerInfoArgs_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnGetContainerInfoCompleted(operation, false); },
            thisSPtr);
        OnGetContainerInfoCompleted(operation, true);
    }

private:

    void OnGetContainerInfoCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->EndGetContainerInfo(operation, containerInfo_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "End(GetContainerInfo): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, error);
    }

private:
    SingleCodePackageApplicationHostProxy & owner_;
    wstring containerInfoType_;
    wstring containerInfoArgs_;
    TimeoutHelper const timeoutHelper_;
    wstring containerInfo_;
};

SingleCodePackageApplicationHostProxy::~SingleCodePackageApplicationHostProxy()
{
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::BeginActivateCodePackage(
    CodePackageInstanceSPtr const & codePackage,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(codePackage);
    UNREFERENCED_PARAMETER(timeout);

    if (isCodePackageActive_.load())
    {
        Assert::CodingError(
            "SingleCodePackageApplicationHostProxy::BeginActivateCodePackage should only be called once. CodePackage {0}",
            codePackageInstance_->Context);
    }

    isCodePackageActive_.store(true);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCode::Success(),
        callback,
        parent);
}

ErrorCode SingleCodePackageApplicationHostProxy::EndActivateCodePackage(
    Common::AsyncOperationSPtr const & operation,
    __out CodePackageActivationId & activationId,
    __out CodePackageRuntimeInformationSPtr & codePackageRuntimeInformation)
{
    activationId = codePackageActivationId_;
    codePackageRuntimeInformation = codePackageRuntimeInformation_;

    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::BeginDeactivateCodePackage(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(codePackageInstanceId);
    UNREFERENCED_PARAMETER(activationId);
    UNREFERENCED_PARAMETER(timeout);

    isCodePackageActive_.store(false);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCode::Success(),
        callback,
        parent);
}

ErrorCode SingleCodePackageApplicationHostProxy::EndDeactivateCodePackage(
    AsyncOperationSPtr const & operation)
{
    UNREFERENCED_PARAMETER(operation);

    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::BeginGetContainerInfo(
    wstring const & containerInfoType,
    wstring const & containerInfoArgs,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetContainerInfoAsyncOperation>(
        *this,
        containerInfoType,
        containerInfoArgs,
        timeout,
        callback,
        parent);
}

ErrorCode SingleCodePackageApplicationHostProxy::EndGetContainerInfo(
    AsyncOperationSPtr const & operation,
    __out wstring & containerInfo)
{
    return GetContainerInfoAsyncOperation::End(operation, containerInfo);
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::BeginApplicationHostCodePackageOperation(
    ApplicationHostCodePackageOperationRequest const & requestBody,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ApplicationHostCodePackageOperation>(
        *this,
        requestBody,
        callback,
        parent);
}

ErrorCode SingleCodePackageApplicationHostProxy::EndApplicationHostCodePackageOperation(
    AsyncOperationSPtr const & operation)
{
    return ApplicationHostCodePackageOperation::End(operation);
}

void SingleCodePackageApplicationHostProxy::SendDependentCodePackageEvent(
    CodePackageEventDescription const & eventDescription)
{
    AsyncOperation::CreateAndStart<SendDependentCodePackageEventAsyncOperation>(
        *this,
        eventDescription,
        [this](AsyncOperationSPtr const & operation)
        {
            SendDependentCodePackageEventAsyncOperation::End(operation).ReadValue();
        },
        this->CreateAsyncOperationRoot());
}

bool SingleCodePackageApplicationHostProxy::HasHostedCodePackages()
{
    return isCodePackageActive_.load();
}

void SingleCodePackageApplicationHostProxy::OnApplicationHostRegistered()
{
    // NO-OP
}

void SingleCodePackageApplicationHostProxy::OnApplicationHostTerminated(DWORD exitCode)
{
    this->exitCode_.store(exitCode);

    if (this->State.Value < FabricComponentState::Closing)
    {
        this->terminatedExternally_.store(true);
    }

    //
    // Abort if we are not already closing.
    //
    if (this->State.Value != FabricComponentState::Closing)
    {
        this->Abort();
    }

    WriteInfo(
        TraceType,
        TraceId,
        "OnApplicationHostTerminated called for HostId={0}, ServicePackageInstanceId={1} with ExitCode={2}. Current state is {3}.",
        this->HostId,
        this->ServicePackageInstanceId,
        exitCode,
        this->State);
}

void SingleCodePackageApplicationHostProxy::EmitDiagnosticTrace()
{
    //
    // IMPORTANT
    // Trace emitted here is consumed by Fault Analysis Service (FAS). If you are making a change
    // make sure FAS owners are aware of changes to avoid breaking the diagnostics tools.
    //

    if (activationFailed_.load())
    {
        // TODO: Trace here for failed start.
        return;
    }

    auto exitCode = (DWORD)exitCode_.load();
    auto unexpectedTermination = terminatedExternally_.load();

    if (this->Context.IsContainerHost)
    {
        hostingTrace.ContainerExitedOperational(
            Guid::NewGuid(),
            this->codePackageInstance_->Context.ApplicationName,
            this->GetServiceName(),
            this->ServicePackageInstanceId.ServicePackageName,
            this->ServicePackageInstanceId.PublicActivationId,
            this->ServicePackageInstanceId.ActivationContext.IsExclusive,
            this->codePackageInstance_->CodePackageInstanceId.CodePackageName,
            this->EntryPointType,
            this->codePackageInstance_->EntryPoint.ContainerEntryPoint.ImageName,
            this->GetUniqueContainerName(),
            this->HostId,
            exitCode,
            unexpectedTermination,
            this->codePackageInstance_->GetLastActivationTime());
    }
    else
    {
        DWORD processId = 0;
        if (codePackageRuntimeInformation_ != nullptr)
        {
            processId = codePackageRuntimeInformation_->ProcessId;
        }

        hostingTrace.ProcessExitedOperational(
            Guid::NewGuid(),
            this->codePackageInstance_->Context.ApplicationName,
            this->GetServiceName(),
            this->ServicePackageInstanceId.ServicePackageName,
            this->ServicePackageInstanceId.PublicActivationId,
            this->ServicePackageInstanceId.ActivationContext.IsExclusive,
            this->codePackageInstance_->CodePackageInstanceId.CodePackageName,
            this->EntryPointType,
            this->codePackageInstance_->EntryPoint.ExeEntryPoint.Program,
            processId,
            this->HostId,
            exitCode,
            unexpectedTermination,
            this->codePackageInstance_->GetLastActivationTime());
    }
}

void SingleCodePackageApplicationHostProxy::OnContainerHealthCheckStatusChanged(ContainerHealthStatusInfo const & healthStatusInfo)
{
    if (this->Hosting.State.Value > FabricComponentState::Opened)
    {
        // no need to notify if the Subsystem is closing down
        return;
    }

    WriteNoise(
        TraceType,
        TraceId,
        "OnContainerHealthCheckStatusChanged called for HostId={0}, ServicePackageInstanceId={1} with {2}. Current state is {3}.",
        this->HostId,
        this->ServicePackageInstanceId,
        healthStatusInfo,
        this->State);

    if (this->ShouldNotify())
    {
        codePackageInstance_->OnHealthCheckStatusChanged(codePackageActivationId_, healthStatusInfo);
    }
}

void SingleCodePackageApplicationHostProxy::OnCodePackageTerminated(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId)
{
    UNREFERENCED_PARAMETER(codePackageInstanceId);
    UNREFERENCED_PARAMETER(activationId);

    // NO-OP
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode SingleCodePackageApplicationHostProxy::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode SingleCodePackageApplicationHostProxy::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void SingleCodePackageApplicationHostProxy::OnAbort()
{
    //
    // Abort can happen in three cases:
    // 1) Open of proxy failed.
    // 2) CodePackage itself is shutting down.
    // 3) UpdateContext to ApplicationHost fails.
    // 4) Proxy received termination notification on AppHostManager notification.
    //
    // If proxy open failed CodePackageInstance open will also fail.
    //

    if (!terminatedExternally_.load())
    {
        this->exitCode_.store(ProcessActivator::ProcessAbortExitCode);
        Hosting.ApplicationHostManagerObj->FabricActivator->AbortProcess(this->HostId);
    }

    // We will notify only when CodePackage has activated meaning the process has started.
    // If the process failed to start then open will fail, it will cause CodePackageInstance Start to fail
    // which will trigger reschedule of CodePackageInstance.
    if (!activationFailed_.load())
    {
        this->NotifyTermination();
    }

    this->EmitDiagnosticTrace();
}

Common::ErrorCode SingleCodePackageApplicationHostProxy::GetContainerNetworkConfigDescription(
    __out ContainerNetworkConfigDescription & networkConfig,
    __out wstring & assignedIpAddresses)
{
    auto error = ErrorCode::Success();

    // Get allocated network resources
    wstring openNetworkAssignedIp;
    std::map<std::wstring, std::wstring> overlayNetworkResources;
    error = GetAssignedNetworkResources(openNetworkAssignedIp, overlayNetworkResources, assignedIpAddresses);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        error = ErrorCode::Success();
    }

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            TraceId,
            "GetAssignedNetworkResources error: ErrorCode={0}",
            error);
        return error;
    }

    // create container network config description
    auto networkConfigType = ContainerPolicies.NetworkConfig.Type;
    networkConfigType = (codePackageInstance_->EnvContext->NetworkExists(NetworkType::Enum::Open)) ? (networkConfigType | NetworkType::Enum::Open) : (networkConfigType);
    networkConfigType = (codePackageInstance_->EnvContext->NetworkExists(NetworkType::Enum::Isolated)) ? (networkConfigType | NetworkType::Enum::Isolated) : (networkConfigType);

    // light weight port bindings
    std::map<std::wstring, std::wstring> portBindings;
    for (auto const & portBinding : ContainerPolicies.PortBindings)
    {
        portBindings.insert(make_pair(portBinding.EndpointResourceRef, std::to_wstring(portBinding.ContainerPort)));
    }

    ContainerNetworkConfigDescription containerNetworkConfig(
        openNetworkAssignedIp,
        overlayNetworkResources,
        portBindings,
        Hosting.NodeId,
        Hosting.NodeName,
        Hosting.FabricNodeConfigObj.IPAddressOrFQDN,
        networkConfigType);

    networkConfig = containerNetworkConfig;

    return error;
}

ErrorCode SingleCodePackageApplicationHostProxy::GetAssignedNetworkResources(
    __out wstring & openNetworkAssignedIpAddress,
    __out map<wstring, wstring> & overlayNetworkResources,
    __out wstring & assignedIpAddresses)
{
    auto error = this->codePackageInstance_->EnvContext->GetNetworkResourceAssignedToCodePackage(
        this->codePackageInstance_->Name, 
        openNetworkAssignedIpAddress, 
        overlayNetworkResources);

    // assigned ips
    if (error.IsSuccess())
    {
        if (!openNetworkAssignedIpAddress.empty())
        {
            assignedIpAddresses = openNetworkAssignedIpAddress;
        }

        for (auto const & onr : overlayNetworkResources)
        {
            wstring ipAddress;
            wstring macAddress;
            StringUtility::SplitOnce(onr.second, ipAddress, macAddress, L",");
            if (assignedIpAddresses.empty())
            {
                assignedIpAddresses = ipAddress;
            }
            else
            {
                assignedIpAddresses = wformatString("{0},{1}", assignedIpAddresses, ipAddress);
            }
        }
    }

    return error;
}

Common::ErrorCode SingleCodePackageApplicationHostProxy::GetContainerDnsServerConfiguration(
    ContainerNetworkConfigDescription const & networkConfig,
    __out vector<wstring> & dnsServers)
{
    auto error = ErrorCode::Success();

    // DNS chain section for containers.
    if (DNS::DnsServiceConfig::GetConfig().IsEnabled)
    {
        if (codePackageInstance_->EnvContext->NetworkExists(NetworkType::Enum::Open)
            && !networkConfig.OpenNetworkAssignedIp.empty()
            && !HostingConfig::GetConfig().LocalNatIpProviderEnabled)
        {
            std::wstring dnsServerIP;
            error = Common::IpUtility::GetAdapterAddressOnTheSameNetwork(networkConfig.OpenNetworkAssignedIp, /*out*/dnsServerIP);
            if (error.IsSuccess())
            {
                dnsServers.push_back(dnsServerIP);
            }
            else
            {
                WriteWarning(
                    TraceType, 
                    TraceId,
                    "Failed to get DNS IP address for container with assigned IP={0} : ErrorCode={1}",
                    networkConfig.OpenNetworkAssignedIp, 
                    error);
            }
        }
        else if (codePackageInstance_->EnvContext->NetworkExists(NetworkType::Enum::Isolated))
        {
#if defined(PLATFORM_UNIX)
            auto ipAddressOrFqdn = Hosting.FabricNodeConfigObj.IPAddressOrFQDN;
            std::vector<Endpoint> endpoints;
            error = Transport::TcpTransportUtility::ResolveToAddresses(ipAddressOrFqdn, ResolveOptions::Enum::Unspecified, endpoints);
            if (error.IsSuccess())
            {
                for (auto const & endpoint : endpoints)
                {
                    dnsServers.push_back(endpoint.GetIpString());
                }
            }
            else
            {
                WriteWarning(
                    TraceType,
                    TraceId,
                    "Failed to resolve IP address for ip address or fqdn {0} : ErrorCode={1}",
                    ipAddressOrFqdn,
                    error);
            }
#endif
        }
        // NAT container has to have the special DNS NAT adapter IP address set as the preferred DNS.
        // This is a workaround for the Windows bug which prevents NAT containers to reach the node.
        // Please remove once Windows fixes the NAT bug.
        else if (DNS::DnsServiceConfig::GetConfig().SetContainerDnsWhenPortBindingsAreEmpty
            || !PortBindings.empty()
            || HostingConfig::GetConfig().LocalNatIpProviderEnabled)
        {
#if defined(PLATFORM_UNIX)
            // On Linux, if no port bindings are specified in non-mip scenario, "host" networking mode is used as 
            // default networking mode, unless overriden. Hence, we need to add DNS servers on the host network. 
            // Otherwise, public connectivity will be broken for the container connected to host network. 
            if (PortBindings.empty() &&
                StringUtility::AreEqualCaseInsensitive(HostingConfig::GetConfig().DefaultContainerNetwork, L"host"))
            {
                std::wstring localhost = L"127.0.0.1";
                dnsServers.push_back(localhost);
            }
            else
            {
#endif
                std::wstring dnsServerIP;
                auto err = FabricEnvironment::GetFabricDnsServerIPAddress(dnsServerIP);
                if (err.IsSuccess())
                {
                    dnsServers.push_back(dnsServerIP);
                }
                else
                {
                    WriteWarning(
                        TraceType,
                        TraceId,
                        "Failed to get DNS IP address for the container from the environment : ErrorCode={0}",
                        err);
                }
#if defined(PLATFORM_UNIX)
            }
#endif
        }
    }

    if (error.IsSuccess())
    {
        // format as comma separated string
        wstring dnsServerList = L"";
        for (auto dnsServer : dnsServers)
        {
            dnsServerList = (dnsServerList.empty()) ? (wformatString("{0}", dnsServer)) : (wformatString("{0},{1}", dnsServerList, dnsServer));
        }

        WriteNoise(
            TraceType,
            TraceId,
            "Container DNS setup: SetContainerDnsWhenPortBindingsAreEmpty={0}, IsPortBindingEmpty={1}, DefaultContainerNetwork={2}, ContainerDnsChain={3}",
            DNS::DnsServiceConfig::GetConfig().SetContainerDnsWhenPortBindingsAreEmpty,
            PortBindings.empty(),
            HostingConfig::GetConfig().DefaultContainerNetwork,
            dnsServerList);
    }

    return error;
}

bool SingleCodePackageApplicationHostProxy::GetLinuxContainerIsolation()
{
    bool linuxContainerIsolated = false;

#if defined(PLATFORM_UNIX)
    linuxContainerIsolated = codePackageInstance_->EnvContext->ContainerGroupIsolated;
#endif

    return linuxContainerIsolated;
}

wstring SingleCodePackageApplicationHostProxy::GetUniqueContainerName()
{
    auto name = wformatString("SF-{0}-{1}",
        StringUtility::ToWString(this->ApplicationId.ApplicationNumber),
        this->HostId);
    auto activationId = this->ServicePackageInstanceId.PublicActivationId;
    if (!activationId.empty())
    {
        name = wformatString("{0}_{1}", name, activationId);
    }
    StringUtility::ToLower(name);
    return name;
}

wstring SingleCodePackageApplicationHostProxy::GetContainerGroupId()
{
    return this->ServicePackageInstanceId.ActivationContext.ToString();
}

wstring SingleCodePackageApplicationHostProxy::GetServiceName()
{
    wstring serviceName;
    if (this->ServicePackageInstanceId.ActivationContext.IsExclusive)
    {
        auto res = this->Hosting.TryGetExclusiveServicePackageServiceName(
            this->ServicePackageInstanceId.ServicePackageId,
            this->ServicePackageInstanceId.ActivationContext,
            serviceName);

        ASSERT_IF(res == false, "ServiceName must be present for exlusive service package activation.");
    }
    return serviceName;
}

bool SingleCodePackageApplicationHostProxy::ShouldNotify()
{
    bool shouldNotify;

    {
        AcquireWriteLock readLock(notificationLock_);
        shouldNotify = shouldNotify_;
    }

    return shouldNotify;
}

void SingleCodePackageApplicationHostProxy::NotifyTermination()
{
    if (this->Hosting.State.Value > FabricComponentState::Opened)
    {
        // no need to notify if the Subsystem is closing down
        return;
    }

    {
        AcquireWriteLock writeLock(notificationLock_);
        if (!shouldNotify_)
        {
            return;
        }
        
        shouldNotify_ = false;
    }

    auto exitCode = (DWORD)exitCode_.load();

    codePackageInstance_->OnEntryPointTerminated(
        codePackageActivationId_, 
        exitCode, 
        false /* Ignore reporting */);
}
