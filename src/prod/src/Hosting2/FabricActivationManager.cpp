// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Transport;
using namespace Hosting2;

StringLiteral const TraceType_ActivationManager("FabricActivationManager");
GlobalWString FabricActivationManager::SecurityGroupFileName = make_global<wstring>(L"WFAppGroupCleanup.txt");

class FabricActivationManager::FabricSetupAsyncOperation: public AsyncOperation
{
    DENY_COPY(FabricSetupAsyncOperation)

public:
    FabricSetupAsyncOperation(
        FabricActivationManager & activationManager,
        TimeSpan const & timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & asyncOperationParent)
        : AsyncOperation(callback, asyncOperationParent)
        , activationManager_(activationManager)
        , timeoutHelper_(timeout)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<FabricSetupAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        RunFabricSetup(thisSPtr);
    }

    void RunFabricSetup(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error = ErrorCodeValue::Success;

        if (this->activationManager_.skipFabricSetup_)
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        wstring fabricDataRoot;
        error = FabricEnvironment::GetFabricDataRoot(fabricDataRoot);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceType_ActivationManager,
                this->activationManager_.TraceId,
                "Completing FabricSetup with error as registry is not written. This will cause a retry.");
            TryComplete(thisSPtr, error);
            return;
        }

        wstring currentDir = Directory::GetCurrentDirectoryW();
        wstring binPath;
        if(File::Exists(Path::Combine(currentDir, Hosting2::Constants::FabricSetup::ExeName)))
        {
            binPath = currentDir;
        }
        else
        {
            binPath = currentDir.append(Hosting2::Constants::FabricSetup::RelativePathToFabricHost);
        }

        if(!File::Exists(Path::Combine(binPath, Hosting2::Constants::FabricSetup::ExeName)))
        {
            Trace.WriteError(TraceType_ActivationManager,
                this->activationManager_.TraceId,
                "Unable to find the {0} in {1}", Constants::FabricSetup::ExeName, binPath);
            error = ErrorCodeValue::FileNotFound;
            TryComplete(thisSPtr, error);
            return;
        }

        Common::EnvironmentMap emap;
        if (!Environment::GetEnvironmentMap(emap))
        {
            Trace.WriteError(TraceType_ActivationManager,
                this->activationManager_.TraceId,
                "Unable to get environment map");
            error = ErrorCode(ErrorCodeValue::OperationFailed);
            TryComplete(thisSPtr, error);
            return;
        }

        ProcessDescription processDescriptor(
            Path::Combine(binPath, Hosting2::Constants::FabricSetup::ExeName),
            Hosting2::Constants::FabricSetup::InstallArguments, 
            binPath, 
            emap, 
            currentDir,
            currentDir, 
            currentDir,
            currentDir, 
            this->activationManager_.hostedServiceActivateHidden_,
            false,
            true, /*This will enable the child processes to break away from the Job object*/
            false);

        auto operation = activationManager_.processActivator_->BeginActivate(
            nullptr, 
            processDescriptor, 
            binPath,
            [this, thisSPtr](pid_t, Common::ErrorCode const & error, DWORD exitCode) { this->OnProcessExit(thisSPtr, error, exitCode); },
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->FinishActivate(operation, false); },
            thisSPtr);
        FinishActivate(operation, true);
    }

    void OnProcessExit(AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & error, DWORD processExitCode)
    {
        if(thisSPtr->TryStartComplete())
        {
            if(!error.IsSuccess())
            {
                WriteWarning(TraceType_ActivationManager,
                    this->activationManager_.TraceId,
                    "FabricSetup process wait result: {0}",
                    error);
                thisSPtr->FinishComplete(thisSPtr, error);
                return;
            }

            WriteInfo(
                TraceType_ActivationManager,
                this->activationManager_.TraceId,
                "FabricSetup process exited with {0}",
                processExitCode);

            auto err = ErrorCode::FromWin32Error(processExitCode);
            if (!err.IsSuccess())
            {
                thisSPtr->FinishComplete(thisSPtr, err);
                return;
            }
            //best effort cleanup, no need to fail open for this.
            err = this->FixupApplicationGroupMemberships();
            WriteTrace(err.ToLogLevel(),
                TraceType_ActivationManager,
                this->activationManager_.TraceId,
                "Clearing out local app group memberships returned {0}",
                err);

            thisSPtr->FinishComplete(thisSPtr, error);
        }
    }

    void FinishActivate(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = activationManager_.processActivator_->EndActivate(operation, this->activationContext_);
        if(!error.IsSuccess())
        {            
            TryComplete(operation->Parent, error);
            return;
        } 
    }

    ErrorCode FixupApplicationGroupMemberships()
    {
        wstring dataPath;
        auto error = FabricEnvironment::GetFabricDataRoot(dataPath);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType_ActivationManager, activationManager_.TraceId, "Failed to get fabricdatapath", error);
        }
        wstring fullPath = Path::Combine(dataPath, *FabricActivationManager::SecurityGroupFileName);
        if (!File::Exists(fullPath))
        {
            error = ClearApplicationGroupMemberships();
            if (!error.IsSuccess())
            {
                return error;
            }
            try
            {
                FileWriter fileWriter;
                error = fileWriter.TryOpen(fullPath);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        activationManager_.TraceId,
                        "Failed to open SecurityCleanup file {0}, error={1}",
                        fullPath,
                        error);
                    return error;
                }

                wstring text;
                text.append(L"Cleanup success");
                std::string result;
                StringUtility::UnicodeToAnsi(text, result);
                fileWriter << result;
            }
            catch (std::exception const& e)
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    activationManager_.TraceId,
                    "Failed to generate SecurityCleanup file {0}, exception ={1}",
                    fullPath,
                    e.what());
                return ErrorCodeValue::OperationFailed;
            }
        }
        return ErrorCodeValue::Success;
    }

    ErrorCode ClearApplicationGroupMemberships()
    {
    #if !defined(PLATFORM_UNIX)
        ErrorCode lastError;
        LPLOCALGROUP_INFO_1 pBuff = NULL;
        DWORD entriesRead = 0;
        DWORD totalEntries = 0;
        DWORD_PTR resumeHandle = 0;
        NET_API_STATUS status = ::NetLocalGroupEnum(
            NULL /*local server*/,
            1 /*LOCALGROUP_INFO level*/,
            (LPBYTE *)&pBuff,
            MAX_PREFERRED_LENGTH,
            &entriesRead,
            &totalEntries,
            &resumeHandle /*resume handle*/);

        lastError = ErrorCode::FromWin32Error(status);

        if (status == NERR_Success)
        {
            LPLOCALGROUP_INFO_1 p;
            if ((p = pBuff) != NULL)
            {
                for (DWORD i = 0; i < entriesRead; i++)
                {
                    wstring comment(p->lgrpi1_comment);
                    wstring name(p->lgrpi1_name);
                    if (StringUtility::StartsWithCaseInsensitive(name, *ApplicationPrincipals::GroupNamePrefix) &&
                        StringUtility::StartsWithCaseInsensitive(comment, *ApplicationPrincipals::WinFabAplicationLocalGroupComment))
                    {
                        ErrorCode err = SecurityPrincipalHelper::SetLocalGroupMembers(name, vector<PSID>());
                        WriteTrace(err.ToLogLevel(),
                            TraceType_ActivationManager,
                            this->activationManager_.TraceId,
                            "SetLocalGroupMembers for application security group {0}, error {1}",
                            name,
                            err);
                        if (!err.IsSuccess())
                        {
                            lastError = err;
                        }
                    }
                    ++p;
                }
            }
        }
        if (pBuff != NULL)
        {
            NetApiBufferFree(pBuff);
        }
        return lastError;
    #else
        return ErrorCodeValue::Success;
    #endif
    }

private:
    FabricActivationManager & activationManager_;
    ProcessActivatorUPtr processActivator_;
    IProcessActivationContextSPtr activationContext_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// FabricActivationManager::OpenAsyncOperation Implementation
//
// NOTE: WE SHOULD NOT BE ACCESSING ANY HOSTING CONFIGS BEFORE WE EXECUTE FABRIC SETUP
// AS FABRIC SETUP UPDATES THE CONFIG VALUES AND WILL CAUSE FABRIC HOST RESTART.

class FabricActivationManager::OpenAsyncOperation : public AsyncOperation
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        FabricActivationManager & activationManager,
        TimeSpan timeout, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & asyncOperationParent)
        : AsyncOperation(callback, asyncOperationParent),
        activationManager_(activationManager),
        timeoutHelper_(timeout),
        setupRetryCount_(0)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        OpenTraceSessionManager(thisSPtr);
    }

private:
    void OpenTraceSessionManager(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "opening TraceSessionManager");
        if (!activationManager_.skipFabricSetup_)
        {
            auto errorCode = activationManager_.traceSessionManager_->Open();
            if (!errorCode.IsSuccess())
            {
                WriteError(TraceType_ActivationManager, activationManager_.TraceId, "TraceSessionManager open failed with {0}", errorCode);
                TryComplete(thisSPtr, errorCode);
                return;
            }
        }

        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "TraceSessionManager open succeeded");

        CleanupFirewallRules(thisSPtr);
    }

    void CleanupFirewallRules(AsyncOperationSPtr const & thisSPtr)
    {
        // Schedule a cleanup before running FabricSetup. If we do the cleanup after FabricSetup has executed and there are too many firewall rules,
        // FabricSetup would timeout and we may never be able to complete the FabricSetup.

#if !defined(PLATFORM_UNIX)
        std::vector<BSTR> rules;
        auto error = FirewallSecurityProviderHelper::GetOrRemoveFirewallRules(rules, FirewallSecurityProviderHelper::firewallGroup_);

        if (!error.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, activationManager_.TraceId, "GetFirewallRules for cleanup failed with {0}", error);
            TryComplete(thisSPtr, error);
            return;
        }

        if (!rules.empty())
        {
            wstring traceId = activationManager_.TraceId;

            //Cleanup firewall on a thread
            Threadpool::Post([rules, traceId]() {

                auto error = FirewallSecurityProviderHelper::RemoveRules(rules);
                if (!error.IsSuccess())
                {
                    WriteError(TraceType_ActivationManager, traceId, "RemoveFirewallRules failed with {0}", error);
                }
            });
        }
#endif

        RunFabricSetup(thisSPtr);
    }

    void RunFabricSetup(AsyncOperationSPtr const & thisSPtr)
    {
            auto operation = AsyncOperation::CreateAndStart<FabricSetupAsyncOperation>(
                this->activationManager_,
                this->timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            { 
                this->FinishFabricSetup(operation, false); 
            },
                thisSPtr);
            FinishFabricSetup(operation, true);
    }

    void FinishFabricSetup(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = FabricSetupAsyncOperation::End(operation);

         WriteTrace(
             error.ToLogLevel(),
            TraceType_ActivationManager,
            activationManager_.TraceId,
            "FabricSetup operation returned errorcode {0}",
            error);


        if(error.IsSuccess())
        {
            error = this->activationManager_.Initialize();
            WriteTrace(
                error.ToLogLevel(),
                TraceType_ActivationManager,
                activationManager_.TraceId,
                "FabricActivationManager initialize failed errorcode {0}",
                error);
            if (!error.IsSuccess())
            {
                TryComplete(operation->Parent, error);
                return;
            }
            
            SetupContainerLogRootDirectory(operation->Parent);
        }
        else if (!operation->CompletedSynchronously &&
            setupRetryCount_ < FabricHostConfig::GetConfig().ActivationMaxFailureCount)
        {
            setupRetryCount_++;

            int64 delay = FabricHostConfig::GetConfig().ActivationRetryBackoffInterval.Ticks * setupRetryCount_;
            if (delay < timeoutHelper_.GetRemainingTime().Ticks)
            {

                AsyncOperationSPtr thisSPtr = operation->Parent;

                AcquireExclusiveLock lock(activationManager_.timerLock_);
                activationManager_.setupRetryTimer_ = Timer::Create(
                    "Hosting.ActivationManager.Setup",
                    [this, thisSPtr](TimerSPtr const & timer)
                {
                    timer->Cancel();
                    this->RunFabricSetup(thisSPtr);
                });
                activationManager_.setupRetryTimer_->Change(TimeSpan::FromTicks(delay));
            }
            else
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    activationManager_.TraceId,
                    "Not retrying fabricsetup since timeout has expired error {0}, retry count {1}",
                    error,
                    setupRetryCount_);
                TryComplete(operation->Parent, error);
            }
        }
        else
        {
            WriteWarning(
                TraceType_ActivationManager,
                activationManager_.TraceId,
                "Not retrying fabricsetup since retry attempts have exceeded max retry attempts. error {0}, retry count {1}",
                error,
                setupRetryCount_);
            TryComplete(operation->Parent, error);
        }
        return;
    }

    void SetupContainerLogRootDirectory(AsyncOperationSPtr const& thisSPtr)
    {
        // This need to happen before FabricDCA is launched by FabricHost.
        // If this folder is not there FabricDCA will fail to monitor container log directory for runtime traces.
        wstring fabricLogRoot;
        auto error = FabricEnvironment::GetFabricLogRoot(fabricLogRoot);
        if (!error.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, activationManager_.TraceId, "SetupContainerLogRootDirectory failed to determine FabricLogRoot with error {0}", error);
            TryComplete(thisSPtr, error);
            return;
        }

        wstring fabricContainerLogRoot = Path::Combine(fabricLogRoot, L"Containers");

        if (!Directory::Exists(fabricContainerLogRoot))
        {
            error = Directory::Create2(fabricContainerLogRoot);
            if (!error.IsSuccess())
            {
                WriteError(TraceType_ActivationManager, activationManager_.TraceId, "SetupContainerLogRootDirectory failed with {0} while creaing directory {1}", error, fabricContainerLogRoot);
                TryComplete(thisSPtr, error);
                return;
            }
        }

#if defined(PLATFORM_UNIX)
        // Permission needs to be fixed for Upgrade scenarios as well because release 6.4+ Fabric will create the directories under "Containers".
        // If you don't set the permissions all new container creations will fail because Fabric won't be able to create the directory for them.
        // On windows Fabric runs as network service and fabricContainerLogRoot inherits permissions from parent directory which already has access.
        error = File::AllowAccessToAll(fabricContainerLogRoot);
        if (!error.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, activationManager_.TraceId, "SetupContainerLogRootDirectory failed trying to setup permissions for {0} with {1}", fabricContainerLogRoot, error);
            TryComplete(thisSPtr, error);
            return;
        }
#endif

        OpenCertificateAclingManager(thisSPtr);
    }

    void OpenCertificateAclingManager(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "opening certificate acling manager");

        auto errorCode = this->activationManager_.certificateAclingManager_->Open();
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, activationManager_.TraceId, "Certificate acling manager open failed with {0}", errorCode);
            TryComplete(thisSPtr, errorCode);
            return;
        }

        WriteInfo(
            TraceType_ActivationManager,
            activationManager_.TraceId,
            "Certificate acling manager succeeded.");

        OpenIpcServer(thisSPtr);
    }

    void OpenIpcServer(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "opening IPC server");
        auto errorCode = activationManager_.ipcServer_->Open();
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, activationManager_.TraceId, "IPC server open failed with {0}", errorCode);
            TryComplete(thisSPtr, errorCode);
            return;
        }

        WriteInfo(
            TraceType_ActivationManager,
            activationManager_.TraceId,
            "IPC server open succeeded. Transport address {0}",
            activationManager_.ipcServer_->TransportListenAddress);

        //
        // Open ProcessActivationManager before HostedServiceActivationManager as it needs
        // to be in opened state before it starts getting Ipc messages from hosted services.
        //
        OpenProcessActivationManager(thisSPtr);
    }

    void OpenProcessActivationManager(AsyncOperationSPtr thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "opening ProcessActivationManager");
        auto operation = activationManager_.processActivationManager_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & asyncOperation) { this->FinishProcessActivationManagerOpen(asyncOperation, false); },
            thisSPtr);
        FinishProcessActivationManagerOpen(operation, true);
    }

    void FinishProcessActivationManagerOpen(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = activationManager_.processActivationManager_->EndOpen(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, activationManager_.TraceId, "ProcessActivationManager open failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }

        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "ProcessActivationManager open succeeded");

        OpenServiceActivator(operation->Parent);
    }

    void OpenServiceActivator(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "opening ServiceActivator");
        auto operation = activationManager_.serviceActivator_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & asyncOperation) { this->FinishServiceActivatorOpen(asyncOperation, false); },
            thisSPtr);
        FinishServiceActivatorOpen(operation, true);
    }

    void FinishServiceActivatorOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = activationManager_.serviceActivator_->EndOpen(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, activationManager_.TraceId, "ServiceActivator open failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "ServiceActivator open succeeded");

        if (Hosting2::FabricHostConfig::GetConfig().EnableRestartManagement)
        {
            OpenFabricRestartManager(operation->Parent);
        }
        else
        {
            SetupNatNetworkProvider(operation->Parent);
        }
    }

    void OpenFabricRestartManager(AsyncOperationSPtr thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "opening FabricRestartManager");
        auto operation = activationManager_.fabricRestartManager_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & asyncOperation) { this->FinishFabricRestartManagerOpen(asyncOperation, false); },
            thisSPtr);
        FinishFabricRestartManagerOpen(operation, true);
    }

    void FinishFabricRestartManagerOpen(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = activationManager_.fabricRestartManager_->EndOpen(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, activationManager_.TraceId, "FabricRestartManager open failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }
        
        WriteInfo(TraceType_ActivationManager, activationManager_.TraceId, "FabricRestartManager open succeeded");
        //Setup Local Nat network
        SetupNatNetworkProvider(operation->Parent);
    }

    void SetupNatNetworkProvider(AsyncOperationSPtr const& operation)
    {
        ErrorCode error;
        if (HostingConfig::GetConfig().LocalNatIpProviderEnabled)
        {
            WriteInfo(
                TraceType_ActivationManager,
                activationManager_.TraceId,
                "Starting SetupNatNetworkProvider");
            auto & processActivationManager = activationManager_.ProcessActivationManagerObj;

            error = processActivationManager->ContainerActivatorObj->RegisterNatIpAddressProvider();
            WriteInfo(
                TraceType_ActivationManager,
                activationManager_.TraceId,
                "Register NatIPAddressProvider error {0}.",
                error);

#if defined(LOCAL_NAT_NETWORK_SETUP)
            if(!error.IsSuccess())
            {
                TryComplete(operation, error);
                return;
            }
#if !defined(PLATFORM_UNIX)
            error = FirewallSecurityProvider::AddLocalNatFirewallRule(
                processActivationManager->ContainerActivatorObj->NatIPAddressProviderObj.GatewayIP,
                HostingConfig::GetConfig().LocalNatIpProviderNetworkRange
            );
            
            WriteTrace(
                error.ToLogLevel(),
                TraceType_ActivationManager,
                activationManager_.TraceId,
                "Create firewall rule for container to host communication error {0}.",
                error);
#endif
#endif // defined(LOCAL_NAT_NETWORK_SETUP)
        }
        TryComplete(operation, error);
    }

private:
    FabricActivationManager & activationManager_;
    TimeoutHelper timeoutHelper_;
    ProcessActivatorUPtr processActivator_;
    ProcessActivationContextSPtr activationContext_;
    int setupRetryCount_;
};

// ********************************************************************************************************************
// FabricActivationManager::CloseAsyncOperation Implementation
//
class FabricActivationManager::CloseAsyncOperation : public AsyncOperation
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        FabricActivationManager & activationManager,
        TimeSpan timeout, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & asyncOperationParent)
        : activationManager_(activationManager),
        timeoutHelper_(timeout),
        AsyncOperation(callback, asyncOperationParent)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
#if defined(LOCAL_NAT_NETWORK_SETUP)
        RemoveNatIpAddressProvider(thisSPtr);
#else
        CloseServiceActivator(thisSPtr);
#endif
    }

    void CloseIpcServer(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "closing IPC server");
        auto error = activationManager_.ipcServer_->Close();
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType_ActivationManager, activationManager_.TraceId, "IPC server close failed with error {0}", error);
            TryComplete(thisSPtr, error);
            return;
        }
        WriteInfo(TraceType_ActivationManager, activationManager_.TraceId, "IPC server closed");
        CloseCertificateAclingManager(thisSPtr);
    }

    void CloseCertificateAclingManager(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "closing certificate acling manager");
        activationManager_.certificateAclingManager_->Close();
        WriteInfo(TraceType_ActivationManager, activationManager_.TraceId, "certificate acling manager closed");
        CloseTraceSessionManager(thisSPtr);
    }

    void CloseTraceSessionManager(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "closing TraceSessionManager");
        Common::ErrorCode errorCode;
        if (!activationManager_.skipFabricSetup_)
        { 
            errorCode = activationManager_.traceSessionManager_->Close();
            if (!errorCode.IsSuccess())
            {
                WriteError(TraceType_ActivationManager, activationManager_.TraceId, "TraceSessionManager close failed with {0}", errorCode);
                TryComplete(thisSPtr, errorCode);
                return;
            }
        }
        WriteInfo(TraceType_ActivationManager, activationManager_.TraceId, "TraceSessionManager close succeeded");
        TryComplete(thisSPtr, errorCode);
    }
    
    void RemoveNatIpAddressProvider(AsyncOperationSPtr const & thisSPtr)
    {
        if (HostingConfig::GetConfig().LocalNatIpProviderEnabled)
        {
            WriteInfo(TraceType_ActivationManager, activationManager_.TraceId, "Start RemoveNatIpAddressProvider");
#if !defined(PLATFORM_UNIX)
            auto error = FirewallSecurityProvider::RemoveLocalNatFirewallRule();
            if (error.IsSuccess())
            {
                WriteInfo(TraceType_ActivationManager, activationManager_.TraceId, "Removed container to host firewall rule");
            }
            else
            {
                WriteError(TraceType_ActivationManager, activationManager_.TraceId, "Failed to remove container to host firewall rule");
            }

#endif // !defined(PLATFORM_UNIX)
            auto errorCode = activationManager_.ProcessActivationManagerObj->ContainerActivatorObj->UnregisterNatIpAddressProvider();
            if (!errorCode.IsSuccess())
            {
                WriteError(TraceType_ActivationManager, activationManager_.TraceId, "RemoveNatIpAddressProvider failed with {0}", errorCode);
                TryComplete(thisSPtr, errorCode);
                return;
            }
        }
        CloseServiceActivator(thisSPtr);
    }

    void CloseServiceActivator(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "Close ServiceActivator");
        auto operation = activationManager_.serviceActivator_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & asyncOperation) { this->FinishServiceActivatorClose(asyncOperation, false); },
            thisSPtr);
        FinishServiceActivatorClose(operation, true);
    }

    void FinishServiceActivatorClose(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = activationManager_.serviceActivator_->EndClose(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, activationManager_.TraceId, "ServiceActivator close failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }
        WriteInfo(TraceType_ActivationManager, activationManager_.TraceId, "ServiceActivator close succeeded");
        CloseProcessActivationManager(operation->Parent);
    }

    void CloseProcessActivationManager(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "Close ProcessActivationManager");
        auto operation = activationManager_.processActivationManager_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & asyncOperation) { this->FinishProcessActivationManagerClose(asyncOperation, false); },
            thisSPtr);
        FinishProcessActivationManagerClose(operation, true);
    }

    void FinishProcessActivationManagerClose(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = activationManager_.processActivationManager_->EndClose(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, activationManager_.TraceId, "ProcessActivationManager close failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }
        WriteInfo(TraceType_ActivationManager, activationManager_.TraceId, "ProcessActivationManager close succeeded");
        if (Hosting2::FabricHostConfig::GetConfig().EnableRestartManagement)
        {
            CloseFabricRestartManager(operation->Parent);
        }
        else
        {
            CloseIpcServer(operation->Parent);
        }
    }

    void CloseFabricRestartManager(AsyncOperationSPtr thisSPtr)
    {
        WriteNoise(TraceType_ActivationManager, activationManager_.TraceId, "closing FabricRestartManager");
        auto operation = activationManager_.fabricRestartManager_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & asyncOperation) { this->FinishFabricRestartManagerClose(asyncOperation, false); },
            thisSPtr);
        FinishFabricRestartManagerClose(operation, true);
    }

    void FinishFabricRestartManagerClose(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = activationManager_.fabricRestartManager_->EndClose(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, activationManager_.TraceId, "FabricRestartManager close failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }

        WriteInfo(TraceType_ActivationManager, activationManager_.TraceId, "FabricRestartManager close succeeded");
        CloseIpcServer(operation->Parent);
    }

private:
    FabricActivationManager & activationManager_;
    TimeoutHelper timeoutHelper_;
};

FabricActivationManager::FabricActivationManager(
    bool activateHidden,
    bool skipFabricSetup)
    : hostedServiceActivateHidden_(activateHidden),
    skipFabricSetup_(skipFabricSetup),
    serviceActivator_(),
    setupRetryTimer_(),
    timerLock_(),
    certificateAclingManager_()
{
    auto activator = make_unique<ProcessActivator>(*this);
    auto traceSessionManager = make_unique<TraceSessionManager>(*this);

    processActivator_ = move(activator);
    traceSessionManager_ = move(traceSessionManager);
}

FabricActivationManager::~FabricActivationManager()
{
    WriteInfo(TraceType_ActivationManager, TraceId, "destructor called");
}

AsyncOperationSPtr FabricActivationManager::OnBeginOpen(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ErrorCode error = ErrorCodeValue::Success;
    if (skipFabricSetup_)
    {
        Trace.WriteInfo(
            TraceType_ActivationManager,
            TraceId,
            "Skipping server role setup since SkipServerRole is specified");
    }
    else
    {
        bool isUserAdmin;
        error = SecurityUtility::IsCurrentUserAdmin(isUserAdmin);
        if (!error.IsSuccess())
        {
            WriteError(TraceType_ActivationManager, TraceId, "IsCurrentUserAdmin failed: {0}", error);
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
        }

        if (!isUserAdmin)
        {
            Trace.WriteInfo(TraceType_ActivationManager,
                TraceId,
                "Skipping server role setup since current user does not have admininstrator rights");
            skipFabricSetup_ = true;
        }
    }

    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode FabricActivationManager::Initialize()
{
    auto ipcServer = make_unique<IpcServer>(
        *this,
        FabricHostConfig::GetConfig().ActivatorServiceAddress,
        FabricHostConfig::GetConfig().ActivatorServerId,
        false /* disallow use of unreliable transport */,
        L"FabricActivationManager");

    //Disable incoming message size limit since only WindowsFabricAdministratorsGroupName is allowed to connect, see code below
    ipcServer->DisableIncomingMessageSizeLimit();

    SecuritySettings ipcServerSecuritySettings;
    auto error = SecuritySettings::CreateNegotiateServer(
        FabricConstants::WindowsFabricAdministratorsGroupName,
        ipcServerSecuritySettings);

    if(!error.IsSuccess())
    {
        WriteError(
            TraceType_ActivationManager,
            "ipcServer->SecuritySettings.CreateNegotiateServer error={0}",
            error);
        return error;
    }

    error = ipcServer->SetSecurity(ipcServerSecuritySettings);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType_ActivationManager,
            "ipcServer->SetSecurity error={0}",
            error);
        return error;
    }

    ipcServer_ = move(ipcServer);

    auto serviceActivator = make_shared<HostedServiceActivationManager>(
        *this, // Common::ComponentRoot const &
        *this, // FabricActivationManager const &
        hostedServiceActivateHidden_,
        skipFabricSetup_);
    serviceActivator_ = move(serviceActivator);

    auto processActivationManager = make_shared<ProcessActivationManager>(
        *this,
        *this);
    processActivationManager_ = move(processActivationManager);

    fabricRestartManager_ = make_unique<FabricRestartManager>(
        *this,
        *this);

    certificateAclingManager_ = make_shared<CertificateAclingManager>(*this);
    return error;
}

ErrorCode FabricActivationManager::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode error = OpenAsyncOperation::End(asyncOperation);
    return error;
}

AsyncOperationSPtr FabricActivationManager::OnBeginClose(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode FabricActivationManager::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode error = CloseAsyncOperation::End(asyncOperation);
    return error;
}

void FabricActivationManager::OnAbort()
{
    {
        AcquireExclusiveLock lock(timerLock_);
        if (this->setupRetryTimer_)
        {
            this->setupRetryTimer_->Cancel();
            this->setupRetryTimer_.reset();
        }
    }
    if(processActivationManager_)
    {
        this->processActivationManager_->ContainerActivatorObj->AbortNatIpAddressProvider();
    }
    if(ipcServer_)
    {
        this->ipcServer_->Abort();
    }
    if(serviceActivator_)
    {
        this->serviceActivator_->Abort();
    }
    if(processActivationManager_)
    {
        this->processActivationManager_->Abort();
    }
    if (traceSessionManager_)
    {
        this->traceSessionManager_->Abort();
    }
    if (certificateAclingManager_)
    {
        certificateAclingManager_->Abort();
    }
    if (fabricRestartManager_)
    {
        fabricRestartManager_->Abort();
    }
}