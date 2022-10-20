// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Client;
using namespace ClientServerTransport;
using namespace Federation;
using namespace Transport;
using namespace Query;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Management;
using namespace Management::CentralSecretService;

StringLiteral const TraceType("HostingSubsystem");

class HostingSubsystem::ServicePackageInstanceInfoMap
{
    DENY_COPY(ServicePackageInstanceInfoMap)

private:
    struct InstanceInfo
    {
        wstring ServiceName;
        wstring PublicActivationId;
        wstring ApplicationName;
    };

    bool TryGetInstanceInfo(
        ServicePackageIdentifier const & servicePackageId,
        ServicePackageActivationContext const & activationContext,
        __out InstanceInfo & instanceInfo) const
    {
        auto key = this->CreateKey(servicePackageId, activationContext);

        {
            AcquireReadLock readLock(lock_);

            auto iter = map_.find(key);
            if (iter == map_.end())
            {
                return false;
            }

            instanceInfo = iter->second;
        }

        return true;
    }

public:
    ServicePackageInstanceInfoMap(__in HostingSubsystem & hosting)
        : lock_()
        , map_()
        , hosting_(hosting)
    {
    }

    bool TryGetActivationId(
        ServicePackageIdentifier const & servicePackageId,
        ServicePackageActivationContext const & activationContext,
        __out wstring & activationId) const
    {
        if (!activationContext.IsExclusive)
        {
            activationId = L"";
            return true;
        }

        InstanceInfo instanceInfo;
        if (!TryGetInstanceInfo(servicePackageId, activationContext, instanceInfo))
        {
            return false;
        }

        activationId = instanceInfo.PublicActivationId;

        return true;
    }

    bool TryGetServiceName(
        ServicePackageIdentifier const & servicePackageId,
        ServicePackageActivationContext const & activationContext,
        __out wstring & serviceName) const
    {
        ASSERT_IFNOT(activationContext.IsExclusive, "ServiceName requested for shared service package activation.");

        InstanceInfo instanceInfo;
        if (!TryGetInstanceInfo(servicePackageId, activationContext, instanceInfo))
        {
            return false;
        }

        serviceName = instanceInfo.ServiceName;

        return true;
    }

    bool TryGetApplicationName(
        ServicePackageIdentifier const & servicePackageId,
        ServicePackageActivationContext const & activationContext,
        __out wstring & applicationName) const
    {
        InstanceInfo instanceInfo;
        if (!TryGetInstanceInfo(servicePackageId, activationContext, instanceInfo))
        {
            return false;
        }

        applicationName = instanceInfo.ApplicationName;

        return true;
    }

    wstring GetOrAddActivationId(
        ServicePackageIdentifier const & servicePackageId,
        ServicePackageActivationContext const & activationContext,
        wstring const & serviceName,
        wstring const & applicationName)
    {
        wstring activationId;
        if (this->TryGetActivationId(servicePackageId, activationContext, activationId))
        {
            return activationId;
        }

        ASSERT_IF(serviceName.empty(), "ServiceName cannot be empty for exclusive service package activation.");

        auto key = this->CreateKey(servicePackageId, activationContext);

        {
            AcquireWriteLock writeLock(lock_);

            auto iter = map_.find(key);
            if (iter != map_.end())
            {
                return iter->second.PublicActivationId;
            }
            
            InstanceInfo info;
            info.ServiceName = serviceName;
            info.PublicActivationId = Guid::NewGuid().ToString();
            info.ApplicationName = applicationName;

            map_.insert(make_pair(key, info));

            WriteNoise(
                TraceType,
                hosting_.Root.TraceId,
                "Added ServiceName=[{0}], ApplicationName=[{1}], ServicePackagePublicActivationId=[{2}] for ServicePackageActivationContext=[{3}].",
                info.ServiceName,
                info.ApplicationName,
                info.PublicActivationId,
                activationContext);

            return info.PublicActivationId;
        }
    }

private:
    wstring CreateKey(
        ServicePackageIdentifier const & servicePackageId, 
        ServicePackageActivationContext const & activationContext) const
    {
        return wformatString("{0}_{1}", servicePackageId.ToString(), activationContext.ActivationGuid.ToString());
    }

private:
    HostingSubsystem & hosting_;
    mutable RwLock lock_;
    
    //
    // uses <ServicePackageIdentifier.ToString()>_<Guid.ToString()> as key.
    //
    map<wstring, InstanceInfo, IsLessCaseInsensitiveComparer<wstring>> map_;
};

class HostingSubsystem::HostingSubsystemQueryHandler : public AsyncOperation
{
public:
    HostingSubsystemQueryHandler(
        __in HostingQueryManagerUPtr & hostingQueryManager,
        MessageUPtr && request,
        RequestReceiverContextUPtr && requestContext,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback, 
        Common::AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , hostingQueryManager_(hostingQueryManager)
        , request_(move(request))
        , requestContext_(move(requestContext))
        , timeoutHelper_(timeout)
        , reply_()
    {
    }

    static Common::ErrorCode End(
        Common::AsyncOperationSPtr const & asyncOperation,
        __out Transport::MessageUPtr &reply,
        __out Federation::RequestReceiverContextUPtr & requestContext)
    {
        auto casted = AsyncOperation::End<HostingSubsystemQueryHandler>(asyncOperation);

        requestContext = move(casted->requestContext_);

        if (casted->reply_)
        {
            reply = move(casted->reply_);
        }

        return casted->Error;
    }

protected:

    void OnStart(Common::AsyncOperationSPtr const& thisSPtr)
    {
        auto operation = hostingQueryManager_->BeginProcessIncomingQueryMessage(
                *request_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation){ OnQueryProcessCompleted(operation, false); },
                thisSPtr);
            OnQueryProcessCompleted(operation, true);
    }

private:

    void OnQueryProcessCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = hostingQueryManager_->EndProcessIncomingQueryMessage(operation, reply_);

        TryComplete(operation->Parent, error);
    }

    HostingQueryManagerUPtr & hostingQueryManager_;
    Transport::MessageUPtr request_;
    Transport::MessageUPtr reply_;
    Federation::RequestReceiverContextUPtr requestContext_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// HostingSubsystem::OpenAsyncOperation Implementation
//
class HostingSubsystem::OpenAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        HostingSubsystem & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
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
        owner_.ipcServer_.RegisterMessageHandler(
            Actor::Hosting,
            [root = owner_.Root.CreateComponentRoot(), hostingSubsystem = &owner_] (MessageUPtr & message, IpcReceiverContextUPtr & context)
            {
                hostingSubsystem->ProcessIpcMessage(message, context);
            },
            true);

        auto error = owner_.passThroughClientFactoryPtr_->CreateQueryClient(owner_.queryClientPtr_);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to create QueryClient: ErrorCode={0}",
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        if (FabricHostConfig::GetConfig().EnableRestartManagement)
        {
            error = owner_.passThroughClientFactoryPtr_->CreateRepairManagementClient(owner_.repairMgmtClientPtr_);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "Failed to create RepairManagementClient: ErrorCode={0}",
                    error);
                TryComplete(thisSPtr, error);
                return;
            }
        }

        if (CentralSecretServiceConfig::IsCentralSecretServiceEnabled())
        {
            error = owner_.passThroughClientFactoryPtr_->CreateSecretStoreClient(owner_.secretStoreClient_);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "Failed to create SecretStoreClient: ErrorCode={0}",
                    error);
                TryComplete(thisSPtr, error);
                return;
            }
        }

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Opening HostingSubsystem: Timeout={0}",
            timeoutHelper_.GetRemainingTime());

        OpenLocalResourceManager(thisSPtr);
    }

private:
    void OpenLocalResourceManager(AsyncOperationSPtr thisSPtr)
    {
        auto error = owner_.LocalResourceManagerObj->Open();

        if (!error.IsSuccess())
        {
            WriteWarning(TraceType,
                owner_.Root.TraceId,
                "Failed to create LocalResourceManager: ErrorCode={0}",
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        OpenEventDispatcher(thisSPtr);        
    }

    void OpenEventDispatcher(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.EventDispatcherObj->Open();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "OpenEventDispatcher: ErrorCode={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        owner_.RegisterMessageHandler();

        OpenFabricActivatorClient(thisSPtr);        
    }

    void OpenFabricActivatorClient(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Begin(OpenFabricActivatorClient): Timeout={0}",
            timeoutHelper_.GetRemainingTime());

        auto operation = owner_.fabricActivatorClient_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishActivatorClientOpen(operation, false); },
            thisSPtr);

        FinishActivatorClientOpen(operation, true);
    }
    void FinishActivatorClientOpen(AsyncOperationSPtr operation, bool expectedCompleteSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        auto error = owner_.fabricActivatorClient_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "OpenFabricActivatorClient: ErrorCode={0}",
            error);
        if(!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        if (FabricHostConfig::GetConfig().EnableRestartManagement)
        {
            OpenRestartManagerClient(operation->Parent);
        }
        else
        {
            OpenApplicationHostManager(operation->Parent);
        }
    }

    void OpenRestartManagerClient(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Begin(OpenFabricActivatorClient): Timeout={0}",
            timeoutHelper_.GetRemainingTime());

        auto operation = owner_.nodeRestartManagerClient_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishRestartManagerClientOpen(operation, false); },
            thisSPtr);

        FinishRestartManagerClientOpen(operation, true);
    }

    void FinishRestartManagerClientOpen(AsyncOperationSPtr operation, bool expectedCompleteSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        auto error = owner_.nodeRestartManagerClient_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "OpenRestartManagerClient: ErrorCode={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        OpenApplicationHostManager(operation->Parent);
    }

    void OpenApplicationHostManager(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        WriteInfo(
            TraceType, 
            owner_.Root.TraceId, 
            "Begin(OpenApplicationHostManager): Timeout={0}", 
            timeout);

        auto operation = owner_.ApplicationHostManagerObj->BeginOpen(
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->FinishApplicationHostManagerOpen(operation, false); },
            thisSPtr);

        this->FinishApplicationHostManagerOpen(operation, true);
    }    

    void FinishApplicationHostManagerOpen(AsyncOperationSPtr const & operation, bool expectedCompleteSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        auto error = owner_.ApplicationHostManagerObj->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(OpenApplicationHostManager): ErrorCode={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        OpenGuestServiceTypeHostManager(operation->Parent);
    }

    void OpenGuestServiceTypeHostManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Begin(OpenGuestServiceTypeHostManager): Timeout={0}",
            timeout);

        auto operation = owner_.GuestServiceTypeHostManagerObj->BeginOpen(
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->FinishGuestServiceTypeHostManagerOpen(operation, false); },
            thisSPtr);

        this->FinishGuestServiceTypeHostManagerOpen(operation, true);
    }

    void FinishGuestServiceTypeHostManagerOpen(AsyncOperationSPtr const & operation, bool expectedCompleteSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        auto error = owner_.GuestServiceTypeHostManagerObj->EndOpen(operation);
        
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(OpenGuestServiceTypeHostManager): ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        OpenDownloadManager(operation->Parent);
    }

    void OpenDownloadManager(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Begin(OpenDownloadManager): Timeout={0}",
            timeoutHelper_.GetRemainingTime());

        auto operation = owner_.downloadManager_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishDownloadManagerOpen(operation, false); },
            thisSPtr);

        this->FinishDownloadManagerOpen(operation, true);       
    }    

    void FinishDownloadManagerOpen(AsyncOperationSPtr const & operation, bool expectedCompleteSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        auto error = owner_.downloadManager_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(OpenDownloadManager): ErrorCode={0}",
            error);
        if(!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        OpenApplicationManager(operation->Parent);
    }

    void OpenApplicationManager(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        WriteNoise(
            TraceType, 
            owner_.Root.TraceId, 
            "Begin(OpenApplicationManager): Timeout={0}", 
            timeout);

        auto applicationManagerOpenOperation = owner_.ApplicationManagerObj->BeginOpen(
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnApplicationManagerOpenCompleted(operation); },
            thisSPtr);
        if (applicationManagerOpenOperation->CompletedSynchronously)
        {
            FinishApplicationManagerOpen(applicationManagerOpenOperation);
        }
    }

    void OnApplicationManagerOpenCompleted(AsyncOperationSPtr const & applicationManagerOpenOperation)
    {
        if (!applicationManagerOpenOperation->CompletedSynchronously)
        {
            FinishApplicationManagerOpen(applicationManagerOpenOperation);
        }
    }

    void FinishApplicationManagerOpen(AsyncOperationSPtr const & applicationManagerOpenOperation)
    {
        auto error = owner_.ApplicationManagerObj->EndOpen(applicationManagerOpenOperation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(OpenApplicationManager): ErrorCode={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(applicationManagerOpenOperation->Parent, error);
            return;
        }

        OpenFabricRuntimeManager(applicationManagerOpenOperation->Parent);
    }

    void OpenFabricRuntimeManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.FabricRuntimeManagerObj->Open();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "OpenFabricRuntimeManager: ErrorCode={0}",
            error);

        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
        else
        {
            OpenFabricUpgradeManager(thisSPtr);
        }
    }

    void OpenFabricUpgradeManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.FabricUpgradeManagerObj->Open();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "OpenFabricUpgradeManager: ErrorCode={0}",
            error);        

        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
        else
        {
            OpenHostingHealthManager(thisSPtr);
        }
    }

    void OpenHostingHealthManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.HealthManagerObj->Open();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "OpenHostingHealthManager: ErrorCode={0}",
            error);
        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
        OpenDeletionManager(thisSPtr);
    }

    void OpenDeletionManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.deletionManager_->Open();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "OpenDeletionManager: Error {0}",
            error);        

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        OpenLocalSecretServiceManager(thisSPtr);
    }

    void OpenLocalSecretServiceManager(AsyncOperationSPtr const & thisSPtr)
    {
        // Perform LSS open operations only if SecretStoreService is enabled in the cluster manifest
        ErrorCode error(ErrorCodeValue::Success);
        if (CentralSecretServiceConfig::IsCentralSecretServiceEnabled())
        {
          error = owner_.LocalSecretServiceManagerObj->Open();
          WriteTrace(
              error.ToLogLevel(),
              TraceType,
              owner_.Root.TraceId,
              "OpenLocalSecretServiceManager: Error {0}",
              error);
        }

        TryComplete(thisSPtr, error);       
    }

private:
    HostingSubsystem & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// HostingSubsystem::CloseAsyncOperation Implementation
//
class HostingSubsystem::CloseAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        HostingSubsystem & owner,
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
        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Closing HostingSubsystem: Timeout={0}",
            timeoutHelper_.GetRemainingTime());

        CloseEventDispatcher(thisSPtr);
        owner_.hostingQueryManager_->Close();
        owner_.federation_.UnRegisterMessageHandler(Actor::Hosting);
        owner_.dnsEnvManager_->StopMonitor();
    }

private:
    void CloseEventDispatcher(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.EventDispatcherObj->Close();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "CloseEventDispatcher: ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        CloseHostingHealthManager(thisSPtr);        
    }

    void CloseHostingHealthManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.HealthManagerObj->Close();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "CloseHostingHealthManager: ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        CloseFabricUpgradeManager(thisSPtr);        
    }

    void CloseFabricUpgradeManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.FabricUpgradeManagerObj->Close();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "CloseFabricUpgradeManager: ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        CloseFabricRuntimeManager(thisSPtr);
    }

    void CloseFabricRuntimeManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.FabricRuntimeManagerObj->Close();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "CloseFabricRuntimeManager: ErrorCode={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        CloseApplicationManager(thisSPtr);
    }

    void CloseApplicationManager(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        WriteNoise(
            TraceType, 
            owner_.Root.TraceId, 
            "Begin(CloseApplicationManager): Timeout={0}", 
            timeout);

        auto applicationManagerCloseOperation = owner_.ApplicationManagerObj->BeginClose(
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnApplicationManagerCloseCompleted(operation); },
            thisSPtr);
        if (applicationManagerCloseOperation->CompletedSynchronously)
        {
            FinishCloseApplicationManager(applicationManagerCloseOperation);
        }
    }

    void OnApplicationManagerCloseCompleted(AsyncOperationSPtr const & applicationManagerCloseOperation)
    {
        if (!applicationManagerCloseOperation->CompletedSynchronously)
        {
            FinishCloseApplicationManager(applicationManagerCloseOperation);
        }
    }

    void FinishCloseApplicationManager(AsyncOperationSPtr const & applicationManagerCloseOperation)
    {
        auto error = owner_.ApplicationManagerObj->EndClose(applicationManagerCloseOperation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(CloseApplicationManager): ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(applicationManagerCloseOperation->Parent, error);
            return;
        }

        CloseDownloadManager(applicationManagerCloseOperation->Parent);
    }

    void CloseDownloadManager(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Begin(CloseDownloadManager): Timeout={0}",
            timeoutHelper_.GetRemainingTime());

        auto operation = owner_.downloadManager_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishDownloadManagerClose(operation, false); },
            thisSPtr);
           
        this->FinishDownloadManagerClose(operation, true);
    }

    void FinishDownloadManagerClose(AsyncOperationSPtr operation, bool expectedCompleteSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        auto error = owner_.downloadManager_->EndClose(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(CloseDownloadManager): ErrorCode={0}",
            error);
        if(!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        CloseApplicationHostManager(operation->Parent);
    }

    void CloseApplicationHostManager(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        WriteNoise(
            TraceType, 
            owner_.Root.TraceId, 
            "Begin(CloseApplicationHostManager): Timeout={0}", 
            timeout);

        auto applicationHostManagerCloseOperation = owner_.ApplicationHostManagerObj->BeginClose(
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnApplicationHostManagedCloseCompleted(operation); },
            thisSPtr);
        if (applicationHostManagerCloseOperation->CompletedSynchronously)
        {
            FinishApplicationHostManagedClose(applicationHostManagerCloseOperation);
        }
    }

    void OnApplicationHostManagedCloseCompleted(AsyncOperationSPtr const & applicationHostManagerCloseOperation)
    {
        if (!applicationHostManagerCloseOperation->CompletedSynchronously)
        {
            FinishApplicationHostManagedClose(applicationHostManagerCloseOperation);
        }
    }

    void FinishApplicationHostManagedClose(AsyncOperationSPtr const & applicationHostManagerCloseOperation)
    {
        auto error = owner_.ApplicationHostManagerObj->EndClose(applicationHostManagerCloseOperation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(CloseApplicationHostManager): ErrorCode={0}",
            error);

        if(!error.IsSuccess())
        {
            TryComplete(applicationHostManagerCloseOperation->Parent, error);
            return;
        }

        CloseGuestServiceTypeHostManager(applicationHostManagerCloseOperation->Parent);
    }

    void CloseGuestServiceTypeHostManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto  timeout = timeoutHelper_.GetRemainingTime();
        
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Begin(CloseGuestServiceTypeHostManager): Timeout={0}",
            timeout);

        auto typeHostManagerCloseOperation = owner_.GuestServiceTypeHostManagerObj->BeginClose(
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->FinishGuestServiceTypeHostManagerClose(operation, false); },
            thisSPtr);

        FinishGuestServiceTypeHostManagerClose(typeHostManagerCloseOperation, true);
    }

    void FinishGuestServiceTypeHostManagerClose(AsyncOperationSPtr const & typeHostManagerCloseOperation, bool expectedCompletedSynchronously)
    {
        if (typeHostManagerCloseOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.GuestServiceTypeHostManagerObj->EndClose(typeHostManagerCloseOperation);
        
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(CloseGuestServiceTypeHostManager): ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(typeHostManagerCloseOperation->Parent, error);
            return;
        }

        CloseFabricActivatorClient(typeHostManagerCloseOperation->Parent);
    }

    void CloseFabricActivatorClient(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType, 
            owner_.Root.TraceId, 
            "Begin(CloseFabricActivatorClient): Timeout={0}", 
            timeoutHelper_.GetRemainingTime());

        auto operation = owner_.fabricActivatorClient_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishActivatorClientClose(operation, false); },
            thisSPtr);

        FinishActivatorClientClose(operation, true);
    }

    void FinishActivatorClientClose(AsyncOperationSPtr operation, bool expectedCompleteSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        auto error = owner_.fabricActivatorClient_->EndClose(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "CloseFabricActivatorClient for Node {0}: ErrorCode={1}",
            owner_.NodeId,
            error);
        if(!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        CloseDeletionManager(operation->Parent);
    }

    void CloseDeletionManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.deletionManager_->Close();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "CloseDeletionManager: Error={0}",
            error);
          if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        CloseLocalResourceManager(thisSPtr);
    }

    void CloseLocalResourceManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.LocalResourceManagerObj->Close();

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "CloseLocalResourceManager: Error={0}",
            error
        );

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        CloseLocalSecretServiceManager(thisSPtr);
    }

    void CloseLocalSecretServiceManager(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error(ErrorCodeValue::Success);
        if (CentralSecretServiceConfig::IsCentralSecretServiceEnabled())
        {
            error = owner_.LocalSecretServiceManagerObj->Close();

            WriteTrace(
                error.ToLogLevel(),
                TraceType,
                owner_.Root.TraceId,
                "CloseLocalSecretServiceManager: Error={0}",
                error
            );
        }

        TryComplete(thisSPtr, error);
    }

private:
    HostingSubsystem & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// HostingSubsystem Implementation
//
IHostingSubsystemSPtr Hosting2::HostingSubsystemFactory(HostingSubsystemConstructorParameters & parameters)
{
    return std::make_shared<HostingSubsystem>(
        *parameters.Root,
        *parameters.Federation,
        parameters.NodeWorkingDirectory,
        parameters.NodeConfig,
        *parameters.IpcServer,
        parameters.ServiceTypeBlocklistingEnabled,
        parameters.DeploymentFolder,
        parameters.KtlLoggerNode);
}

HostingSubsystem::HostingSubsystem(
    ComponentRoot const & root,
    Federation::FederationSubsystem & federation,
    wstring const & nodeWorkingDir,
    __in FabricNodeConfigSPtr const & fabricNodeConfig,
    __in Transport::IpcServer & ipcServer,
    bool serviceTypeBlocklistingEnabled,
    wstring const & deploymentFolder,
    __in KtlLogger::KtlLoggerNodeSPtr const & ktlLoggerNode)
    : RootedObject(root),
    nodeId_(federation.Id.ToString()),
    runtimeManager_(),
    applicationManager_(),
    downloadManager_(),
    deletionManager_(),
    localResourceManager_(),
    localSecretServiceManager_(),
    fabricActivatorClient_(),
    nodeRestartManagerClient_(),
    hostManager_(),
    typeHostManager_(),
    healthManager_(),
    eventDispatcher_(),
    ipcServer_(ipcServer),
    federation_(federation),
    fabricNodeConfig_(fabricNodeConfig),
    deploymentFolder_(deploymentFolder),
    ktlLoggerNode_(ktlLoggerNode),
    fabricUpgradeDeploymentFolder_(GetFabricUpgradeDeploymentFolder(*fabricNodeConfig, nodeWorkingDir)),
    imageCacheFolder_(GetImageCacheFolder(*fabricNodeConfig, nodeWorkingDir)),
    startApplicationPortRange_(fabricNodeConfig->StartApplicationPortRange),
    fabricBinFolder_(Environment::GetExecutablePath()),
    endApplicationPortRange_(fabricNodeConfig->EndApplicationPortRange),
    clientConnectionAddress_(fabricNodeConfig->ClientConnectionAddress),
    hostPathInitializationLock_(),
    dllHostPath_(),
    dllHostArguments_(),
    typeHostPath_(),
    hostingQueryManager_(nullptr),
    runLayout_(deploymentFolder_),
    sharedLayout_(Path::Combine(deploymentFolder_, Constants::SharedFolderName)),
    nodeWorkFolder_(nodeWorkingDir),
    svcPkgInstanceInfoMap_(),
    sequenceNumberTracker_(DateTime::Now().Ticks)
{
    LargeInteger nodeIdAsLargeInteger;
    bool success = LargeInteger::TryParse(nodeId_, nodeIdAsLargeInteger);
    ASSERT_IF(!success, "NodeId should be parsable as LargeInteger");

    nodeIdAsLargeInteger_ = nodeIdAsLargeInteger;

    auto runtimeManager = make_unique<FabricRuntimeManager>(root, *this);
    auto applicationManager = make_unique<ApplicationManager>(root, *this);    
    auto downloadManager = make_unique<DownloadManager>(root, *this, fabricNodeConfig_);
    auto deletionManager = make_unique<DeletionManager>(root, *this);
    auto hostManager = make_unique<ApplicationHostManager>(root, *this);
    auto typeHostManager = make_unique<GuestServiceTypeHostManager>(root, *this);
    auto fabricUpgradeManager = make_unique<FabricUpgradeManager>(root, *this);
    auto eventDispatcher = make_unique<EventDispatcher>(root, *this);
    auto hostingQueryManager = make_unique<HostingQueryManager>(root, *this);
    auto healthManager = make_unique<HostingHealthManager>(root, *this);
    auto activatorClient = make_shared<FabricActivatorClient>(root, *this, nodeId_, fabricBinFolder_, federation_.Instance.getInstanceId());
    auto restartManagerClient = make_shared<NodeRestartManagerClient>(root, *this, nodeId_);
    auto svcPkgInstanceInfoMap = make_shared<ServicePackageInstanceInfoMap>(*this);
    auto localResourceManager = make_unique<LocalResourceManager>(root, fabricNodeConfig_, *this);
    auto localSecretServiceManager = make_unique<LocalSecretServiceManager>(root, *this);
    auto dnsEnvManager = make_unique<DnsServiceEnvironmentManager>(root, *this);

    runtimeManager_ = move(runtimeManager);
    applicationManager_ = move(applicationManager);
    downloadManager_ = move(downloadManager);
    deletionManager_ = move(deletionManager);
    hostManager_ = move(hostManager);
    typeHostManager_ = move(typeHostManager);
    localResourceManager_ = move(localResourceManager);
    localSecretServiceManager_ = move(localSecretServiceManager);

    fabricUpgradeManager_ = move(fabricUpgradeManager);
    eventDispatcher_ = move(eventDispatcher);
    hostingQueryManager_ = move(hostingQueryManager);
    healthManager_ = move(healthManager);
    fabricActivatorClient_ = move(activatorClient);
    nodeRestartManagerClient_ = move(restartManagerClient);
    svcPkgInstanceInfoMap_ = move(svcPkgInstanceInfoMap);
    dnsEnvManager_ = move(dnsEnvManager);

    WriteInfo(
        TraceType,
        Root.TraceId,
        "Using {0} as the application deployment folder.",
        deploymentFolder_);

    if (imageCacheFolder_.empty())
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "Disabling ImageCache as per the configuration, deployment of applications may take longer.");
    }
    else
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "Using {0} as the ImageCache folder.",
            imageCacheFolder_);
    }

    ServiceTypeBlocklistingEnabled = serviceTypeBlocklistingEnabled;
}

HostingSubsystem::~HostingSubsystem()
{
    WriteNoise(TraceType, Root.TraceId, "HostingSubsystem.destructor");
}

KtlLogger::SharedLogSettingsSPtr HostingSubsystem::get_ApplicationSharedLogSettings() const 
{
    return ktlLoggerNode_->ApplicationSharedLogSettings; 
}

KtlLogger::SharedLogSettingsSPtr HostingSubsystem::get_SystemServicesSharedLogSettings() const
{
    return ktlLoggerNode_->SystemServicesSharedLogSettings;
}

bool HostingSubsystem::get_BlocklistingEnabled() const 
{ 
    return FabricRuntimeManagerObj->ServiceTypeStateManagerObj->ServiceTypeBlocklistingEnabled; 
}

void HostingSubsystem::set_BlocklistingEnabled(bool value) 
{ 
    FabricRuntimeManagerObj->ServiceTypeStateManagerObj->ServiceTypeBlocklistingEnabled = value; 
}

AsyncOperationSPtr HostingSubsystem::OnBeginOpen(
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode HostingSubsystem::OnEndOpen(AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr HostingSubsystem::OnBeginClose(
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode HostingSubsystem::OnEndClose(AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void HostingSubsystem::OnAbort()
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Aborting HostingSubsystem");

    EventDispatcherObj->Abort();
    HealthManagerObj->Abort();
    FabricUpgradeManagerObj->Abort();
    FabricRuntimeManagerObj->Abort();
    ApplicationManagerObj->Abort();
    downloadManager_->Abort();
    GuestServiceTypeHostManagerObj->Abort();
    ApplicationHostManagerObj->Abort();
    fabricActivatorClient_->Abort();
    federation_.UnRegisterMessageHandler(Actor::Hosting);
    hostingQueryManager_->Abort();
    deletionManager_->Abort();
    localResourceManager_->Abort();
    localSecretServiceManager_->Abort();
    dnsEnvManager_->StopMonitor();

    WriteNoise(
        TraceType,
        Root.TraceId,
        "HostingSubsystem Aborted.");
}

ErrorCode HostingSubsystem::IncrementUsageCount(
    ServiceTypeIdentifier const & serviceTypeId,
    ServicePackageActivationContext const & activationContext)
{
    if (serviceTypeId.ApplicationId.IsAdhoc()) 
    { 
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        wstring svcPkgPublicActivationId;
        auto res = this->svcPkgInstanceInfoMap_->TryGetActivationId(
            serviceTypeId.ServicePackageId, 
            activationContext, 
            svcPkgPublicActivationId);

        ASSERT_IF(!res, "ServicePackagePublicActivationId must me present during HostingSubsystem::IncrementUsageCount() call.")

        ServicePackageInstanceIdentifier svcPkgIntanceId(
            serviceTypeId.ServicePackageId, activationContext, svcPkgPublicActivationId);

        return this->ApplicationManagerObj->IncrementUsageCount(svcPkgIntanceId);
    }
}

void HostingSubsystem::DecrementUsageCount(
    ServiceTypeIdentifier const & serviceTypeId,
    ServicePackageActivationContext const & activationContext)
{
    if (!serviceTypeId.ApplicationId.IsAdhoc())
    {
        wstring svcPkgPublicActivationId;
        auto res = this->svcPkgInstanceInfoMap_->TryGetActivationId(
            serviceTypeId.ServicePackageId,
            activationContext,
            svcPkgPublicActivationId);

        ASSERT_IF(!res, "ServicePackagePublicActivationId must me present during HostingSubsystem::IncrementUsageCount() call.")

        ServicePackageInstanceIdentifier svcPkgIntanceId(
            serviceTypeId.ServicePackageId, activationContext, svcPkgPublicActivationId);

        this->ApplicationManagerObj->DecrementUsageCount(svcPkgIntanceId);
    }
}

ErrorCode HostingSubsystem::FindServiceTypeRegistration(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    Reliability::ServiceDescription const & serviceDescription,
    ServicePackageActivationContext const & activationContext,
    __out uint64 & sequenceNumber,
    __out ServiceTypeRegistrationSPtr & registration)
{
    return FabricRuntimeManagerObj->FindServiceTypeRegistration(
        versionedServiceTypeId,
        serviceDescription,
        activationContext,
        sequenceNumber,
        registration);
}

ErrorCode HostingSubsystem::GetHostId(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    wstring const & applicationName,
    __out wstring & hostId)
{
    return FabricRuntimeManagerObj->GetHostId(
        versionedServiceTypeId,
        applicationName,
        hostId);
}

ErrorCode HostingSubsystem::GetHostId(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    wstring const & applicationName,
    ServicePackageActivationContext const & activationContext,
    __out wstring & hostId)
{
    return FabricRuntimeManagerObj->GetHostId(
        versionedServiceTypeId,
        applicationName,
        activationContext,
        hostId);
}

void HostingSubsystem::InitializeHealthReportingComponent(HealthReportingComponentSPtr const & healthClient)
{
    HealthManagerObj->InitializeHealthReportingComponent(healthClient);
}

void HostingSubsystem::InitializeLegacyTestabilityRequestHandler(LegacyTestabilityRequestHandlerSPtr const & legacyTestabilityRequestHandler)
{
    hostingQueryManager_->InitializeLegacyTestabilityRequestHandler(legacyTestabilityRequestHandler);
}

void HostingSubsystem::InitializePassThroughClientFactory(Api::IClientFactoryPtr const &passThroughClientFactoryPtr)
{
    passThroughClientFactoryPtr_ = passThroughClientFactoryPtr;
    downloadManager_->InitializePassThroughClientFactory(passThroughClientFactoryPtr);
}

AsyncOperationSPtr HostingSubsystem::BeginDownloadApplication(
    ApplicationDownloadSpecification const & appDownloadSpec,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return ApplicationManagerObj->BeginDownloadApplication(
        appDownloadSpec,
        callback,
        parent);
}

ErrorCode HostingSubsystem::EndDownloadApplication(
    AsyncOperationSPtr const & operation)
{
    OperationStatusMapSPtr appDownloadStatus;
    return ApplicationManagerObj->EndDownloadApplication(
        operation,
        appDownloadStatus);
}

ErrorCode HostingSubsystem::AnalyzeApplicationUpgrade(
    ApplicationUpgradeSpecification const & appUpgradeSpec,
    __out CaseInsensitiveStringSet & affectedRuntimeIds)
{	
    return this->ApplicationManagerObj->AnalyzeApplicationUpgrade(appUpgradeSpec, affectedRuntimeIds);	
}

AsyncOperationSPtr HostingSubsystem::BeginUpgradeApplication(
    ApplicationUpgradeSpecification const & appUpgradeSpec,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->ApplicationManagerObj->BeginUpgradeApplication(appUpgradeSpec, callback, parent);
}

ErrorCode HostingSubsystem::EndUpgradeApplication(
    AsyncOperationSPtr const & operation)
{
    return this->ApplicationManagerObj->EndUpgradeApplication(operation);
}

AsyncOperationSPtr HostingSubsystem::BeginDownloadFabric(
    FabricVersion const & fabricVersion,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->FabricUpgradeManagerObj->BeginDownloadFabric(
        fabricVersion,
        callback,
        parent);
}

ErrorCode HostingSubsystem::EndDownloadFabric(
    AsyncOperationSPtr const & operation)
{
    return this->FabricUpgradeManagerObj->EndDownloadFabric(operation);
}

AsyncOperationSPtr HostingSubsystem::BeginValidateFabricUpgrade(
    ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->FabricUpgradeManagerObj->BeginValidateFabricUpgrade(
        fabricUpgradeSpec,
        callback,
        parent);
}

ErrorCode HostingSubsystem::EndValidateFabricUpgrade(  
    __out bool & shouldRestartReplica,
    AsyncOperationSPtr const & operation)
{
    return this->FabricUpgradeManagerObj->EndValidateFabricUpgrade(
        shouldRestartReplica,
        operation);
}

AsyncOperationSPtr HostingSubsystem::BeginFabricUpgrade(
    ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
    bool const shouldRestartReplica,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{    
    return this->FabricUpgradeManagerObj->BeginFabricUpgrade(
        fabricUpgradeSpec,
        shouldRestartReplica,
        callback,
        parent);
}

ErrorCode HostingSubsystem::EndFabricUpgrade(
    AsyncOperationSPtr const & operation)
{
    return this->FabricUpgradeManagerObj->EndFabricUpgrade(operation);
}

AsyncOperationSPtr HostingSubsystem::BeginTerminateServiceHost(
    wstring const & hostId,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    if (StringUtility::AreEqualCaseInsensitive(hostId, Constants::NativeSystemServiceHostId))
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "BeginTerminateServiceHost: Killing node on command from RA");

        ExitProcess(ProcessActivator::ProcessDeactivateExitCode);
    }

    return ApplicationHostManagerObj->BeginTerminateServiceHost(
        hostId,
        callback,
        parent);
}

void HostingSubsystem::EndTerminateServiceHost(
    AsyncOperationSPtr const & operation)
{
    auto error = ApplicationHostManagerObj->EndTerminateServiceHost(operation);
    ASSERT_IFNOT(error.IsSuccess(), "TerminateServiceHost should complete with success. ErrorCode:{0}", error);
}

ServiceTypeRegisteredEventHHandler HostingSubsystem::RegisterServiceTypeRegisteredEventHandler(
    ServiceTypeRegisteredEventHandler const & handler)
{
    return this->EventDispatcherObj->RegisterServiceTypeRegisteredEventHandler(handler);
}

bool HostingSubsystem::UnregisterServiceTypeRegisteredEventHandler(
    ServiceTypeRegisteredEventHHandler const & hHandler)
{
    return this->EventDispatcherObj->UnregisterServiceTypeRegisteredEventHandler(hHandler);
}

ServiceTypeDisabledEventHHandler HostingSubsystem::RegisterServiceTypeDisabledEventHandler(
    ServiceTypeDisabledEventHandler const & handler)
{
    return this->EventDispatcherObj->RegisterServiceTypeDisabledEventHandler(handler);
}

bool HostingSubsystem::UnregisterServiceTypeDisabledEventHandler(
    ServiceTypeDisabledEventHHandler const & hHandler)
{
    return this->EventDispatcherObj->UnregisterServiceTypeDisabledEventHandler(hHandler);
}

ServiceTypeEnabledEventHHandler HostingSubsystem::RegisterServiceTypeEnabledEventHandler(
    ServiceTypeEnabledEventHandler const & handler)
{
    return this->EventDispatcherObj->RegisterServiceTypeEnabledEventHandler(handler);
}

bool HostingSubsystem::UnregisterServiceTypeEnabledEventHandler(
    ServiceTypeEnabledEventHHandler const & hHandler)
{
    return this->EventDispatcherObj->UnregisterServiceTypeEnabledEventHandler(hHandler);
}

RuntimeClosedEventHHandler HostingSubsystem::RegisterRuntimeClosedEventHandler(
    RuntimeClosedEventHandler const & handler)
{
    return this->EventDispatcherObj->RegisterRuntimeClosedEventHandler(handler);
}

bool HostingSubsystem::UnregisterRuntimeClosedEventHandler(
    RuntimeClosedEventHHandler const & hHandler)
{
    return this->EventDispatcherObj->UnregisterRuntimeClosedEventHandler(hHandler);
}

ApplicationHostClosedEventHHandler HostingSubsystem::RegisterApplicationHostClosedEventHandler(
    ApplicationHostClosedEventHandler const & handler)
{
    return this->EventDispatcherObj->RegisterApplicationHostClosedEventHandler(handler);
}

bool HostingSubsystem::UnregisterApplicationHostClosedEventHandler(
    ApplicationHostClosedEventHHandler const & hHandler)
{
    return this->EventDispatcherObj->UnregisterApplicationHostClosedEventHandler(hHandler);
}

AvailableContainerImagesEventHHandler HostingSubsystem::RegisterSendAvailableContainerImagesEventHandler(
    AvailableContainerImagesEventHandler const & handler)
{
    return this->EventDispatcherObj->RegisterSendAvailableContainerImagesEventHandler(handler);
}

bool HostingSubsystem::UnregisterSendAvailableContainerImagesEventHandler(
    AvailableContainerImagesEventHHandler const & hHandler)
{
    return this->EventDispatcherObj->UnregisterSendAvailableContainerImagesEventHandler(hHandler);
}

ErrorCode HostingSubsystem::GetDeploymentFolder(
    __in FabricNodeConfig & fabricNodeConfig,
    wstring const & nodeWorkingDirectory,
    __out wstring & deploymentFolder)
{
    ErrorCode error(ErrorCodeValue::Success);
    
    if (ManagementConfig::GetConfig().EnableDeploymentAtDataRoot)
    {
        error = FabricEnvironment::GetFabricDataRoot(deploymentFolder);
        if (error.IsSuccess())
        {
            Path::CombineInPlace(deploymentFolder, FabricConstants::AppsFolderName);
        }
    }
    else
    {
        deploymentFolder = ManagementConfig::GetConfig().DeploymentDirectory;
    }
    if (deploymentFolder.empty())
    {
        // deployment folder path is not specified, use the node working directory
        deploymentFolder = nodeWorkingDirectory;

    }
    else if (!Path::IsPathRooted(deploymentFolder))
    {
        // deployment folder path is specified but it is not rooted, treat it as relative to working directory
        deploymentFolder = Path::Combine(nodeWorkingDirectory, deploymentFolder);
    }
    else if (fabricNodeConfig.IsScaleMin)
    {
        // deployment folder path is specified and it is rooted, and this is a scalemin deployment
        deploymentFolder = Path::Combine(deploymentFolder, fabricNodeConfig.InstanceName);
    }

    return error;
}


wstring HostingSubsystem::GetFabricUpgradeDeploymentFolder(__in FabricNodeConfig & fabricNodeConfig, wstring const & nodeWorkingDirectory)
{
    wstring fabricUpgradedeploymentFolder(ManagementConfig::GetConfig().FabricUpgradeDeploymentDirectory);
    if (fabricUpgradedeploymentFolder.empty())
    {
        // deployment folder path is not specified, use the node working directory
        fabricUpgradedeploymentFolder = nodeWorkingDirectory;
    }
    else if (!Path::IsPathRooted(fabricUpgradedeploymentFolder))
    {
        // deployment folder path is specified but it is not rooted, treat it as relative to working directory
        fabricUpgradedeploymentFolder = Path::Combine(nodeWorkingDirectory, fabricUpgradedeploymentFolder);
    }
    else if (fabricNodeConfig.IsScaleMin)
    {
        // deployment folder path is specified and it is rooted, and this is a scalemin deployment
        fabricUpgradedeploymentFolder = Path::Combine(fabricUpgradedeploymentFolder, fabricNodeConfig.InstanceName);
    }

    return fabricUpgradedeploymentFolder;
}


wstring HostingSubsystem::GetImageCacheFolder(__in FabricNodeConfig & fabricNodeConfig, wstring const & nodeWorkingDirectory)
{
    wstring imageCacheFolder(ManagementConfig::GetConfig().ImageCacheDirectory);

    if (ManagementConfig::GetConfig().ImageCachingEnabled)
    {
        if (!imageCacheFolder.empty())
        {
            if (!Path::IsPathRooted(imageCacheFolder))
            {
                imageCacheFolder = Path::Combine(nodeWorkingDirectory, imageCacheFolder);
            }
            else
            {
                if (fabricNodeConfig.IsScaleMin)
                {
                    imageCacheFolder = Path::Combine(imageCacheFolder, fabricNodeConfig.InstanceName);
                }
            }
        }

        return imageCacheFolder;
    }
    else
    {
        return L"";
    }
}

void HostingSubsystem::RegisterMessageHandler()
{
    // Register the query handlers


    auto error = hostingQueryManager_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Hosting QueryMessageHandler.Open failed with {0}.",
            error);
    }

    // Register the one way and request-reply message handlers
    federation_.RegisterMessageHandler(
        Actor::Hosting,
        [this] (MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext)
    { 
        WriteError(
            TraceType,
            "{0} received a oneway message: {1}",
            Root.TraceId,
            *message);
        oneWayReceiverContext->Reject(ErrorCodeValue::InvalidMessage);
    },
        [this] (Transport::MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
    { 
        this->RequestMessageHandler(message, requestReceiverContext); 
    },
        false /*dispatchOnTransportThread*/);
}

void HostingSubsystem::RequestMessageHandler(Transport::MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
{
    auto timeout = TimeoutHeader::FromMessage(*message).Timeout;
    auto activityId = FabricActivityHeader::FromMessage(*message).ActivityId;

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Received message {0} at HostingSubsystem",
        activityId);

    // Processing can continue asynchronously after unregistering the message handler
    auto selfRoot = this->Root.CreateComponentRoot();

    BeginProcessRequest(
        move(message),
        move(requestReceiverContext),
        timeout,
        [this, selfRoot, activityId](AsyncOperationSPtr const & asyncOperation) mutable
    {
        this->OnProcessRequestComplete(activityId, asyncOperation);
    },
        this->Root.CreateAsyncOperationRoot());
}

AsyncOperationSPtr HostingSubsystem::BeginProcessRequest(
    MessageUPtr && request,
    RequestReceiverContextUPtr && requestContext,
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    ErrorCode error(ErrorCodeValue::Success);
    wstring const& action = request->Action;

    if (action == QueryTcpMessage::QueryAction)
    {
        return AsyncOperation::CreateAndStart<HostingSubsystemQueryHandler>(
            hostingQueryManager_,
            move(request),
            move(requestContext),
            timeout,
            callback,
            parent);
    }
    else
    {
        WriteError(
            TraceType,
            Root.TraceId,
            "Invalid action {0} at HostingSubsystem",
            action);
        requestContext->Reject(ErrorCode(ErrorCodeValue::InvalidOperation));
        return AsyncOperationSPtr();
    }
}

AsyncOperationSPtr HostingSubsystem::BeginRestartDeployedPackage(
    int64 codePackageInstanceId,
    wstring applicationNameArgument,
    wstring serviceManifestNameArgument,
    wstring codePackageNameArgument,
    ActivityId const activityId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return hostingQueryManager_->BeginRestartDeployedPackage(
        codePackageInstanceId,
        applicationNameArgument,
        serviceManifestNameArgument,
        codePackageNameArgument,
        activityId,
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr HostingSubsystem::BeginRestartDeployedPackage(
    int64 codePackageInstanceId,
    wstring const & applicationNameArgument,
    wstring const & serviceManifestNameArgument,
    wstring const & servicePackageActivationId,
    wstring const & codePackageNameArgument,
    ActivityId const activityId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return hostingQueryManager_->BeginRestartDeployedPackage(
        codePackageInstanceId,
        applicationNameArgument,
        serviceManifestNameArgument,
        servicePackageActivationId,
        codePackageNameArgument,
        activityId,
        timeout,
        callback,
        parent);
}

ErrorCode HostingSubsystem::EndRestartDeployedPackage(
    Common::AsyncOperationSPtr const &operation,
    __out Transport::MessageUPtr& reply)
{
    return hostingQueryManager_->EndRestartDeployedPackage(operation, reply);
}

ErrorCode HostingSubsystem::EndProcessRequest(
    AsyncOperationSPtr const & asyncOperation,
    __out Transport::MessageUPtr & reply,
    __out Federation::RequestReceiverContextUPtr & requestContext)
{
    return HostingSubsystemQueryHandler::End(asyncOperation, reply, requestContext);
}

void HostingSubsystem::OnProcessRequestComplete(
    Common::ActivityId const & activityId,
    AsyncOperationSPtr const & asyncOperation)
{
    MessageUPtr reply;
    RequestReceiverContextUPtr requestContext;
    auto error = EndProcessRequest(asyncOperation, reply, requestContext);

    if (error.IsSuccess())
    {            
        WriteNoise(
            TraceType,
            Root.TraceId,
            "Query Request with ActivityId {0} Succeeded",
            activityId);

        reply->Headers.Replace(FabricActivityHeader(activityId));
        requestContext->Reply(std::move(reply));
    }
    else
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "Query Request with ActivityId {0} failed with {1}",
            activityId,
            error);

        requestContext->Reject(error);
    }
}

void HostingSubsystem::SendToHealthManager(
    MessageUPtr && messagePtr,
    IpcReceiverContextUPtr && ipcTransportContext)
{
    healthManager_->ForwardHealthReportFromApplicationHostToHealthManager(move(messagePtr), move(ipcTransportContext));
}

ErrorCode HostingSubsystem::GetDllHostPathAndArguments(wstring & dllHostPath, wstring & dllHostArguments)
{
    {
        AcquireReadLock readLock(this->hostPathInitializationLock_);
        if (!this->dllHostPath_.empty())
        {
            dllHostPath = this->dllHostPath_;
            dllHostArguments = this->dllHostArguments_;
            return ErrorCode(ErrorCodeValue::Success);
        }
    }

    {
        AcquireWriteLock writeLock(this->hostPathInitializationLock_);
        if (!this->dllHostPath_.empty())
        {
            dllHostPath = this->dllHostPath_;
            dllHostArguments = this->dllHostArguments_;
            return ErrorCode(ErrorCodeValue::Success);
        }
        else
        {
            // initialize
            wstring configuredPath = HostingConfig::GetConfig().DllHostExePath;
            wstring dllHostExePathAndArguments;
            if (!Environment::ExpandEnvironmentStringsW(configuredPath, dllHostExePathAndArguments))
            {
                WriteError(
                    TraceType,
                    Root.TraceId,
                    "ExpandEnvrionmentString failed for {0}.",
                    configuredPath);
                return ErrorCode(ErrorCodeValue::OperationFailed);
            }

            wstring exePath;
            wstring arguments;
            auto error = ProcessUtility::ParseCommandLine(dllHostExePathAndArguments, exePath, arguments);
            if (!error.IsSuccess()) { return error; }

            if (!Path::IsPathRooted(exePath))
            {
                exePath = Path::Combine(this->FabricBinFolder, exePath);
            }

            if (!File::Exists(exePath))
            {
                WriteError(
                    TraceType,
                    Root.TraceId,
                    "GetDllHostPathAndArguments: Executable path of configured DllHost {0} not found.",
                    configuredPath);
                return ErrorCode(ErrorCodeValue::HostingDllHostNotFound);
            }

            this->dllHostPath_ = exePath;
            this->dllHostArguments_ = arguments;

            dllHostPath = this->dllHostPath_;
            dllHostArguments = this->dllHostArguments_;
            return ErrorCode(ErrorCodeValue::Success);
        }
    }
}

ErrorCode HostingSubsystem::GetTypeHostPath(wstring & typeHostPath, bool useReplicatedStore)
{
    {
        AcquireReadLock readLock(this->hostPathInitializationLock_);
        if (!this->typeHostPath_.empty())
        {
            typeHostPath = this->typeHostPath_;
            return ErrorCode(ErrorCodeValue::Success);
        }
    }

    {
        AcquireWriteLock writeLock(this->hostPathInitializationLock_);
        if (!this->typeHostPath_.empty())
        {
            typeHostPath = this->typeHostPath_;
            return ErrorCode(ErrorCodeValue::Success);
        }
        else
        {
            // initialize
            wstring configuredPath;
            if (useReplicatedStore)
            {
                configuredPath = HostingConfig::GetConfig().SFBlockStoreSvcPath;
            }
            else
            {
                configuredPath = HostingConfig::GetConfig().FabricTypeHostPath;
            }
            wstring fabricTypeHostExePath;
            if (!Environment::ExpandEnvironmentStringsW(configuredPath, fabricTypeHostExePath))
            {
                WriteError(
                    TraceType,
                    Root.TraceId,
                    "ExpandEnvrionmentString failed for {0}, continuing with the configured value.",
                    configuredPath);
                return ErrorCode(ErrorCodeValue::OperationFailed);
            }

            if (!Path::IsPathRooted(fabricTypeHostExePath))
            {
                fabricTypeHostExePath = Path::Combine(this->FabricBinFolder, fabricTypeHostExePath);
            }

            if (!File::Exists(fabricTypeHostExePath))
            {
                WriteError(
                    TraceType,
                    Root.TraceId,
                    "GetTypeHostPath: Executable path of configured FabricTypeHostPath {0} not found.",
                    configuredPath);
                return ErrorCode(ErrorCodeValue::HostingTypeHostNotFound);
            }

            this->typeHostPath_ = fabricTypeHostExePath;

            typeHostPath = this->typeHostPath_;
            return ErrorCode(ErrorCodeValue::Success);
        }
    }
}

void HostingSubsystem::Test_SetFabricActivatorClient(
    IFabricActivatorClientSPtr  && testFabricActivatorClient)
{
    WriteInfo(
        TraceType,
        Root.TraceId,
        "Setting testFabricActivatorClient");

    if (testFabricActivatorClient)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "Moving testFabricActivator to fabricActivatorClient");
        fabricActivatorClient_ = move(testFabricActivatorClient);
    }
}

void HostingSubsystem::Test_SetSecretStoreClient(Api::ISecretStoreClientPtr testSecretStoreClient)
{
    WriteInfo(
            TraceType,
            Root.TraceId,
            "Moving testSecretStoreClient to secretStoreClient");
    secretStoreClient_ = testSecretStoreClient;
}

bool HostingSubsystem::TryGetServicePackagePublicActivationId(
    ServicePackageIdentifier const & servicePackageId,
    ServicePackageActivationContext const & activationContext,
    wstring & activationId)
{
    return svcPkgInstanceInfoMap_->TryGetActivationId(servicePackageId, activationContext, activationId);
}

bool HostingSubsystem::TryGetExclusiveServicePackageServiceName(
    ServicePackageIdentifier const & servicePackageId,
    ServicePackageActivationContext const & activationContext,
    wstring & serviceName) const
{
    return svcPkgInstanceInfoMap_->TryGetServiceName(servicePackageId, activationContext, serviceName);
}

bool HostingSubsystem::TryGetServicePackagePublicApplicationName(
    ServicePackageIdentifier const & servicePackageId,
    ServicePackageActivationContext const & activationContext,
    wstring & applicationName) const
{
    return svcPkgInstanceInfoMap_->TryGetApplicationName(servicePackageId, activationContext, applicationName);
}

wstring HostingSubsystem::GetOrAddServicePackagePublicActivationId(
    ServicePackageIdentifier const & servicePackageId,
    ServicePackageActivationContext const & activationContext,
    wstring const & serviceName,
    wstring const & applicationName)
{
    return svcPkgInstanceInfoMap_->GetOrAddActivationId(servicePackageId, activationContext, serviceName, applicationName);
}

int64 HostingSubsystem::GetNextSequenceNumber() const
{
    auto nextVal = ++sequenceNumberTracker_;

    return (int64)nextVal;
}

void HostingSubsystem::ProcessIpcMessage(MessageUPtr & message, IpcReceiverContextUPtr & context)
{
    message;

    TimeSpan leaseTTL;
    LONG ttlInMilliseconds = 0;
    LONGLONG currentTime = 0;
    if (federation_.LeaseAgent.GetLeasingApplicationExpirationTimeFromIPC(&ttlInMilliseconds, &currentTime) == FALSE)
    {
        auto err = ::GetLastError();
        WriteWarning(TraceType, "ProcessIpcMessage: GetLeasingApplicationExpirationTimeFromIPC failed: 0x{0:x}", err);
    }

    if (MAXLONG == ttlInMilliseconds)
    {
        leaseTTL = federation_.LeaseAgent.LeaseDuration();
        WriteNoise(TraceType, Root.TraceId, "ProcessIpcMessage: leaseTTL = {0}, GetLeasingApplicationExpirationTimeFromIPC returned MAXLONG TTL, LeaseDuration is used", leaseTTL); 
    }
    else
    {
        leaseTTL = TimeSpan::FromMilliseconds(ttlInMilliseconds);
        WriteNoise(TraceType, Root.TraceId, "ProcessIpcMessage: leaseTTL = {0}", leaseTTL); 
    }

    LeaseReply leaseReply(leaseTTL); 
    WriteNoise(
        TraceType,
        Root.TraceId,
        "ProcessIpcMessage: ttl = {0}",
        leaseReply.TTL());

    auto reply = make_unique<Message>(leaseReply);
    context->Reply(move(reply));
}

uint64 HostingSubsystem::GetResourceNodeCapacity(std::wstring const& resourceName) const
{
    return this->LocalResourceManagerObj->GetResourceNodeCapacity(resourceName);
}

void HostingSubsystem::Test_SetFabricNodeConfig(Common::FabricNodeConfigSPtr && fabricNodeConfig)
{
    if (fabricNodeConfig)
    {
        fabricNodeConfig_ = move(fabricNodeConfig);
    }
}

