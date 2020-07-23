// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricNode/FabricNodeEventSource.h"
#include "Federation/FederationConfig.h"
#include "LeaseAgent/LeaseAgent.h"

using namespace Common;
using namespace Client;
using namespace Communication;
using namespace Fabric;
using namespace Federation;
using namespace Naming;
using namespace Reliability;
using namespace std;
using namespace Transport;
using namespace Management;
using namespace ImageModel;
using namespace Hosting2;
using namespace Api;
using namespace ServiceModel;
using namespace Testability;
using namespace BackupRestoreService;

const FabricNodeEventSource Events;

static StringLiteral const TraceLifeCycle("LifeCycle");
static StringLiteral const TraceSecurity("Security");
static StringLiteral const TraceHealth("Health");

class FabricNode::DummyIRouter : public Federation::IRouter
{
    DENY_COPY( DummyIRouter );

public:
    DummyIRouter (){};
          

    virtual Common::AsyncOperationSPtr OnBeginRoute(
        Transport::MessageUPtr && ,
        Federation::NodeId ,
        uint64 ,
        std::wstring const &,
        bool ,
        Common::TimeSpan ,
        Common::TimeSpan ,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & root)
    {   
        Assert::TestAssert("Not implemented");
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::NotImplemented, callback, root);
    }

    virtual Common::ErrorCode EndRoute(Common::AsyncOperationSPtr const & asyncOperation) 
    {   
        Assert::TestAssert("Not implemented");
        return AsyncOperation::End<CompletedAsyncOperation>(asyncOperation)->Error;
    }

    virtual Common::AsyncOperationSPtr OnBeginRouteRequest(
        Transport::MessageUPtr && ,
        Federation::NodeId ,
        uint64 ,
        std::wstring const &,
        bool ,
        Common::TimeSpan ,
        Common::TimeSpan ,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & root) 
    {   
        Assert::TestAssert("Not implemented");
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::NotImplemented, callback, root);
    }

    virtual Common::ErrorCode EndRouteRequest(Common::AsyncOperationSPtr const &asyncOperation, Transport::MessageUPtr &) const
    {   
        Assert::TestAssert("Not implemented");
        return AsyncOperation::End<CompletedAsyncOperation>(asyncOperation)->Error;
    }
  };


// ********************************************************************************************************************
// FabricNode::OpenAsyncOperation Implementation
//
class FabricNode::OpenAsyncOperation : public AsyncOperation
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        FabricNode & fabricNode,
        TimeSpan timeout, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & asyncOperationParent)
        : AsyncOperation(callback, asyncOperationParent),
        fabricNode_(fabricNode),
        timeoutHelper_(timeout)
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
        if(!fabricNode_.constructError_.IsSuccess())
        {
            TryComplete(thisSPtr, fabricNode_.constructError_);
        }
        else
        {            
            if (!fabricNode_.IsInZombieMode)
            {
                fabricNode_.RegisterEvents();
            }

            fabricNode_.StartTracingTimer();
            fabricNode_.EnableTransportHealthReporting();
            fabricNode_.EnableCrlHealthReportIfNeeded();

            if (!fabricNode_.IsInZombieMode)
            {            
                OpenFederation(thisSPtr);
            }
            else 
            {                             
                auto op = fabricNode_.zombieModeEntreeService_->BeginOpen(
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const &operation)
                    {
                        auto error = this->fabricNode_.zombieModeEntreeService_->EndOpen(operation);
#if !defined(PLATFORM_UNIX)
                        fabricNode_.InitializeStopExpirationTimer();
#endif
                        TryComplete(operation->Parent, error);
                    },
                    thisSPtr);

            }
        }
    }

    void OnCompleted()
    {
        AsyncOperation::OnCompleted();

        if (this->Error.IsSuccess())
        {
            fabricNode_.RegisterForConfigUpdate();
        }
    }

private:
    void OpenFederation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "opening federation");
        auto operation = fabricNode_.federation_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & asyncOperation) { this->FinishFederationOpen(asyncOperation, false); },
            thisSPtr);
        FinishFederationOpen(operation, true);
    }

    void FinishFederationOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = fabricNode_.federation_->EndOpen(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceLifeCycle, fabricNode_.TraceId, "federation open failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }   

        UnreliableTransportConfig::GetConfig().StartMonitoring(fabricNode_.config_->WorkingDir, fabricNode_.config_->NodeId);

        if (!fabricNode_.TryPutReservedTraceId(fabricNode_.Instance.ToString()))
        {
            WriteWarning(TraceLifeCycle, fabricNode_.TraceId, "failed to update TraceId after federation open");
        }
        
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "federation open succeeded");        

        OpenIpcServer(operation->Parent);
    }
    
    void OpenIpcServer(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "opening IPC server");
        auto errorCode = fabricNode_.ipcServer_->Open();
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceLifeCycle, fabricNode_.TraceId, "IPC server open failed with {0}", errorCode);
            TryComplete(thisSPtr, errorCode);
            return;
        }
        
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "IPC server open succeeded.");
        OpenKtlLogger(thisSPtr);
    }

    void OpenKtlLogger(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "opening ktllogger");
        auto operation = fabricNode_.ktlLogger_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & asyncOperation) { this->FinishKtlLoggerOpen(asyncOperation, false); },
            thisSPtr);
        FinishKtlLoggerOpen(operation, true);
    }

    void FinishKtlLoggerOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = fabricNode_.ktlLogger_->EndOpen(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceLifeCycle, fabricNode_.TraceId, "ktlLogger open failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }   

        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "ktlLogger open succeeded");        

        if (fabricNode_.ktlLogger_->ApplicationSharedLogSettings.get() != nullptr)
        {
            WriteInfo(
                TraceLifeCycle, 
                fabricNode_.TraceId, 
                "Application shared log settings: {0}", 
                *(fabricNode_.ktlLogger_->ApplicationSharedLogSettings));
        }

        if (fabricNode_.ktlLogger_->SystemServicesSharedLogSettings.get() != nullptr)
        {
            WriteInfo(
                TraceLifeCycle, 
                fabricNode_.TraceId, 
                "System service shared log settings: {0}", 
                *(fabricNode_.ktlLogger_->SystemServicesSharedLogSettings));
        }

        OpenTestabilitySubsystem(operation->Parent);
    }

    void OpenTestabilitySubsystem(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "opening Testability");

        auto operation = fabricNode_.testability_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishTestabilitySubsystemOpen(operation, false); },
            thisSPtr);

        FinishTestabilitySubsystemOpen(operation, true);
    }

    void FinishTestabilitySubsystemOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = fabricNode_.testability_->EndOpen(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceLifeCycle, fabricNode_.TraceId, "Testability open failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }

        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "Testability open succeeded");
        OpenHostingSubsystem(operation->Parent);
    }

    void OpenHostingSubsystem(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "opening hosting subsystm");
        auto operation = fabricNode_.hosting_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->FinishHostingOpen(operation, false); },
            thisSPtr);
        FinishHostingOpen(operation, true);
    }

    void FinishHostingOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = fabricNode_.hosting_->EndOpen(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceLifeCycle, fabricNode_.TraceId, "hosting open failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }

        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "hosting open succeeded");
        OpenNetworkInventoryAgentSubsystem(operation->Parent);
    }

    void OpenNetworkInventoryAgentSubsystem(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "opening NetworkInventoryAgentSubsystem subsystem");
        auto operation = fabricNode_.networkInventoryAgent_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->FinishNetworkInventoryAgentOpen(operation, false); },
            thisSPtr);
        FinishNetworkInventoryAgentOpen(operation, true);
    }

    void FinishNetworkInventoryAgentOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = fabricNode_.hosting_->EndOpen(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceLifeCycle, fabricNode_.TraceId, "NetworkInventoryAgentSubsystem open failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }

        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "NetworkInventoryAgentSubsystem open succeeded");
        OpenRuntimeSharingHelper(operation->Parent);
    }    

    void OpenRuntimeSharingHelper(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "opening runtime sharing helper");
        
        auto operation = fabricNode_.runtimeSharingHelper_->BeginOpen(
            fabricNode_.ipcServer_->TransportListenAddress,
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->FinishRuntimeSharingOpenOpen(operation, false); },
            thisSPtr);
        FinishRuntimeSharingOpenOpen(operation, true);
    }
    
    void FinishRuntimeSharingOpenOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
     
        auto errorCode = fabricNode_.runtimeSharingHelper_->EndOpen(operation);
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceLifeCycle, fabricNode_.TraceId, "runtime sharing helper open failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }

        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "runtime sharing helper open succeeded");
        OpenReliability(operation->Parent);
    }

    void OpenReliability(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "opening reliability");
        auto errorCode = fabricNode_.reliability_->Open();
        if (!errorCode.IsSuccess())
        {
            WriteError(TraceLifeCycle, fabricNode_.TraceId, "reliability open failed with {0}", errorCode);
            TryComplete(thisSPtr, errorCode);
            return;
        }

        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "reliability open succeeded");
        OpenBackupRestoreAgent(thisSPtr);
    }

    void OpenBackupRestoreAgent(AsyncOperationSPtr const & thisSPtr)
    {
#if !defined(PLATFORM_UNIX)
        if (nullptr != fabricNode_.backupRestoreAgent_)
        {
            WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "opening BackupRestoreAgent");
            auto errorCode = fabricNode_.backupRestoreAgent_->Open();
            if (!errorCode.IsSuccess())
            {
                WriteError(TraceLifeCycle, fabricNode_.TraceId, "BackupRestoreAgent open failed with {0}", errorCode);
                return;
            }

            WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "BackupRestoreAgent open succeeded");
        }
#endif
        OpenCommunication(thisSPtr);
    }

    void OpenCommunication(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "opening communication");

        auto operation = fabricNode_.communication_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishCommunicationOpen(operation, false); },
            thisSPtr);
        FinishCommunicationOpen(operation, true);
    }

    void FinishCommunicationOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = fabricNode_.communication_->EndOpen(operation);
        if (! errorCode.IsSuccess())
        {
            WriteError(TraceLifeCycle, fabricNode_.TraceId, "communication open failed with {0}", errorCode);
            TryComplete(operation->Parent, errorCode);
            return;
        }

        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "communication open succeeded");
        
        OpenManagement(operation->Parent);
    }
    
    void OpenManagement(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "opening management");
        auto errorCode = fabricNode_.management_->Open();
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "management open returned {0}", errorCode);
        if (errorCode.IsSuccess())
        {
            WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "trying to delete target information file");
            errorCode = DeleteTargetInformationFile();
        }

        TryComplete(thisSPtr, errorCode);
    }
    
    ErrorCode DeleteTargetInformationFile()
    {
        wstring fabricDataRoot;
        auto error = FabricEnvironment::GetFabricDataRoot(fabricDataRoot);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceLifeCycle,
                fabricNode_.TraceId,
                "Error getting FabricDataRoot. Error:{0}",
                error);

            return error;
        }

        FabricDeploymentSpecification fabricDeploymentSpec(fabricDataRoot, L"");
        wstring targetInformationFilePath = fabricDeploymentSpec.GetTargetInformationFile();
        // For scalemin deployments, there will be more than one nodes trying to delete the same file. hence serializing access through a lock file
        wstring lockFilePath = Path::Combine(Path::GetDirectoryName(targetInformationFilePath), L"targetinformationfileaccess.lock");
        {
            ExclusiveFile lockFile(lockFilePath);
            if (!lockFile.Acquire(timeoutHelper_.GetRemainingTime()).IsSuccess())
            {
                WriteWarning(
                    TraceLifeCycle,
                    fabricNode_.TraceId,
                    "Failed to acquire lock file {0}",
                    lockFilePath);
                
                return ErrorCodeValue::Timeout;
            }
            
            if (!File::Exists(targetInformationFilePath))
            {
                WriteInfo(
                    TraceLifeCycle,
                    fabricNode_.TraceId,
                    "Target Information file {0} not found",
                    targetInformationFilePath);

                return ErrorCodeValue::Success;
            }

            WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "targetinformation file found at {0}", targetInformationFilePath);
            // Parse the file and delete only if the target version matches with the current version
            ServiceModel::TargetInformationFileDescription desc;
            error = desc.FromXml(targetInformationFilePath);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceLifeCycle,
                    fabricNode_.TraceId,
                    "Error {0} while parsing the target information file '{1}'",
                    error,
                    targetInformationFilePath);

                return error;
            }

#if !defined(PLATFORM_UNIX)
            auto currentVersion = Environment::GetCurrentModuleFileVersion();
#else
            wstring currentVersion = L""; 
            error = FabricEnvironment::GetFabricVersion(currentVersion);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceLifeCycle,
                    fabricNode_.TraceId,
                    "Error {0} when trying to read cluster version",
                    error);

                return error;
            }
#endif

            if (desc.TargetInstallation.TargetVersion != currentVersion)
            {
                WriteWarning(
                    TraceLifeCycle,
                    fabricNode_.TraceId,
                    "Skipping deletion of target information file '{0}' because current binary version '{1} did not match with the target version '{2}' in the file",
                    targetInformationFilePath,
                    currentVersion,
                    desc.TargetInstallation.TargetVersion);
                return ErrorCodeValue::Success;
            }
                
            error = File::Delete2(targetInformationFilePath);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceLifeCycle,
                    fabricNode_.TraceId,
                    "Error {0} deleting the target information file {1}",
                    error,
                    targetInformationFilePath);

                return error;
            }
        }

        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "Successfully deleted target information file");
        return ErrorCodeValue::Success;
    }

    FabricNode & fabricNode_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// FabricNode::CreateAndOpenAsyncOperation Implementation
//
class FabricNode::CreateAndOpenAsyncOperation : public AsyncOperation
{
    DENY_COPY(CreateAndOpenAsyncOperation)

public:
    CreateAndOpenAsyncOperation(
        FabricNodeConfigSPtr const & config,
        Common::EventHandler const & nodeFailEventHandler,
        Common::EventHandler const & fmReadyEventHandler,
        TimeSpan timeout,
        AsyncCallback const & callback)
        : AsyncOperation(callback, AsyncOperationSPtr()),
        config_(std::move(config)),
        nodeFailEventHandler_(nodeFailEventHandler),
        fmReadyEventHandler_(fmReadyEventHandler),
        timeout_(timeout)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation, __out FabricNodeSPtr & fabricNode)
    {
        auto thisPtr = AsyncOperation::End<CreateAndOpenAsyncOperation>(operation);
        fabricNode = std::move(AsyncOperation::Get<CreateAndOpenAsyncOperation>(operation)->fabricNode_);
        if(!thisPtr->Error.IsSuccess() && fabricNode)
        {
            fabricNode->Abort();
            fabricNode.reset();
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        CreateAndOpen(thisSPtr);
    }

private:
    FabricNodeConfigSPtr config_;
    EventHandler nodeFailEventHandler_;
    EventHandler fmReadyEventHandler_;
    TimeSpan timeout_;
    FabricNodeSPtr fabricNode_;

    void CreateAndOpen(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = FabricNode::Create(config_, fabricNode_);
        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        fabricNode_->CreateDataDirectoriesIfNeeded();
        fabricNode_->RegisterNodeFailedEvent(nodeFailEventHandler_);
        fabricNode_->RegisterFailoverManagerReadyEvent(fmReadyEventHandler_);
        auto operation = fabricNode_->BeginOpen(
            timeout_,
            [this] (AsyncOperationSPtr const & asyncOperation) { this->FinishFabricNodeOpen(asyncOperation, false); },
            thisSPtr);
        FinishFabricNodeOpen(operation, true);
    }

    void FinishFabricNodeOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        TryComplete(operation->Parent, fabricNode_->EndOpen(operation));
    }
};

// ********************************************************************************************************************
// FabricNode::CloseAsyncOperation Implementation
//
class FabricNode::CloseAsyncOperation : public AsyncOperation
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        FabricNode & fabricNode,
        TimeSpan timeout, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & asyncOperationParent)
        : fabricNode_(fabricNode),
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
        if (!fabricNode_.IsInZombieMode)
        {
            fabricNode_.UnRegisterEvents();
        }

        fabricNode_.CancelTimers();

        if (!fabricNode_.IsInZombieMode)
        {
            CloseManagement(thisSPtr);
        }
        else
        {
            auto op = fabricNode_.zombieModeEntreeService_->BeginClose(
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const &operation)
                {
                    auto error = this->fabricNode_.zombieModeEntreeService_->EndClose(operation);
                    TryComplete(operation->Parent, error);
                },
                thisSPtr);
        }
    }

private:
    void CloseManagement(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "closing management");
        ErrorCode error = fabricNode_.management_->Close();
        if(!error.IsSuccess())
        {
            WriteWarning(TraceLifeCycle, fabricNode_.TraceId, "closed management completed with error {0}", error);
            TryComplete(thisSPtr, error);
            return;
        }
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "management closed");
        
        CloseBackupRestoreAgent(thisSPtr);
    }

    void CloseBackupRestoreAgent(AsyncOperationSPtr const & thisSPtr)
    {
#if !defined(PLATFORM_UNIX)
        if (nullptr != fabricNode_.backupRestoreAgent_)
        {
            WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "Closing BackupRestoreAgent");
            fabricNode_.backupRestoreAgent_->Close();
            WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "Closed BackupRestoreAgent");
        }
#endif
        CloseCommunication(thisSPtr);
    }

    void CloseCommunication(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "closing communication");
        auto operation = fabricNode_.communication_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->FinishCommunicationClose(operation, false); },
            thisSPtr);
        FinishCommunicationClose(operation, true);
    }

    void FinishCommunicationClose(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = fabricNode_.communication_->EndClose(operation);
        if(!error.IsSuccess())
        {
            WriteWarning(TraceLifeCycle, fabricNode_.TraceId, "closed communication completed with error {0}", error);
            TryComplete(operation->Parent, error);
            return;
        }
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "communication closed");
        CloseReliability(operation->Parent);
    }

    void CloseReliability(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "closing reliability");
        auto error = fabricNode_.reliability_->Close();
        if(!error.IsSuccess())
        {
            WriteWarning(TraceLifeCycle, fabricNode_.TraceId, "closed reliability with error {0}", error);
            TryComplete(thisSPtr, error);
            return;
        }

        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "reliability closed");
        CloseRuntimeSharingHelper(thisSPtr);
    }

    void CloseRuntimeSharingHelper(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "closing runtime sharing helper");

        auto operation = fabricNode_.runtimeSharingHelper_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->FinishRuntimeSharingHelperClose(operation, false); },
            thisSPtr);
        FinishRuntimeSharingHelperClose(operation, true);
    }

private:
    void FinishRuntimeSharingHelperClose(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = fabricNode_.runtimeSharingHelper_->EndClose(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceLifeCycle, fabricNode_.TraceId, "closed runtime sharing helper with error {0}", error);
            TryComplete(operation->Parent, error);
            return;
        }
      
        CloseHosting(operation->Parent);
    }

    void CloseHosting(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "closing hosting");

        auto operation = fabricNode_.hosting_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & asyncOperation) { FinishHostingClose(asyncOperation, false); },
            thisSPtr);
        FinishHostingClose(operation, true);
    }

    void FinishHostingClose(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
     
        auto error = fabricNode_.hosting_->EndClose(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceLifeCycle, fabricNode_.TraceId, "closed hosting with error {0}", error);
            TryComplete(operation->Parent, error);
            return;
        }
        
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "hosting closed");
        CloseNetworkInventoryAgent(operation->Parent);
    }

    void CloseNetworkInventoryAgent(AsyncOperationSPtr const & thisSPtr)
    {
         WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "closing CloseNetworkInventoryAgent");

        auto operation = fabricNode_.networkInventoryAgent_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & asyncOperation) { FinishNetworkInventoryAgent(asyncOperation, false); },
            thisSPtr);
        FinishNetworkInventoryAgent(operation, true);
    }

    void FinishNetworkInventoryAgent(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
     
        auto error = fabricNode_.networkInventoryAgent_->EndClose(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceLifeCycle, fabricNode_.TraceId, "closed NetworkInventoryAgent with error {0}", error);
            TryComplete(operation->Parent, error);
            return;
        }
        
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "NetworkInventoryAgent closed");
#if defined(PLATFORM_UNIX)
        CloseIpcServer(operation->Parent);
#else
        CloseKtlLogger(operation->Parent);
#endif        

        // KTL deactivation can't be called from a KTL thread
        //
        auto thisSPtr = operation->Parent;
        Threadpool::Post([this, thisSPtr] { this->CloseKtlLogger(thisSPtr); });
    }

    void CloseKtlLogger(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "closing ktllogger");
        auto operation = fabricNode_.ktlLogger_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & asyncOperation) { this->FinishKtlLoggerClose(asyncOperation, false); },
            thisSPtr);
        FinishKtlLoggerClose(operation, true);
    }

    void FinishKtlLoggerClose(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = fabricNode_.ktlLogger_->EndClose(operation);
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "ktlLogger closed with {0}", error);
        
        CloseIpcServer(operation->Parent);
    }

    void CloseIpcServer(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "closing IPC server");
        auto error = fabricNode_.ipcServer_->Close();
        if (!error.IsSuccess())
        {
            WriteWarning(TraceLifeCycle, fabricNode_.TraceId, "closed IPC server with error {0}", error);
            TryComplete(thisSPtr, error);
            return;
        }

        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "IPC server closed");

        CloseFederation(thisSPtr);
    }

    void CloseFederation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "closing federation");
        auto operation = fabricNode_.federation_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const & asyncOperation) { this->FinishFederationClose(asyncOperation, false); },
            thisSPtr);
        FinishFederationClose(operation, true);
    }

    void FinishFederationClose(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = fabricNode_.federation_->EndClose(operation);
        WriteInfo(TraceLifeCycle, fabricNode_.TraceId, "federation closed with {0}", error);

        TryComplete(operation->Parent, error);
        return;
    }

    FabricNode & fabricNode_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// FabricNode::InitializeSystemServiceAsyncOperationBase Implementation
//

class FabricNode::InitializeSystemServiceAsyncOperationBase : public AsyncOperation
{
public:
    InitializeSystemServiceAsyncOperationBase(
        FabricNode & owner,
        wstring const & serviceName,
        ConsistencyUnitId const & firstCuid,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , owner_(owner)
        , serviceName_(serviceName)
        , firstCuid_(firstCuid)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<InitializeSystemServiceAsyncOperationBase>(operation)->Error;
    }

protected:

    __declspec (property(get=get_Owner)) FabricNode & Owner;
    FabricNode & get_Owner() const { return owner_; }

    __declspec (property(get=get_FirstCuid)) ConsistencyUnitId const & FirstCuid;
    ConsistencyUnitId const & get_FirstCuid() const { return firstCuid_; }

    virtual AsyncOperationSPtr BeginCreateSystemService(TimeSpan const timeout, AsyncCallback const &, AsyncOperationSPtr const & root) = 0;
    virtual ErrorCode EndCreateSystemService(AsyncOperationSPtr const & operation) = 0;
    virtual void OnResolvedSystemService(ErrorCode const &) { return; }
    virtual void OnCreatedSystemService(ErrorCode const &) { return; }
    virtual void OnSchedulingRetry() { return; }

    void OnStart(AsyncOperationSPtr const &)
    {
        // Intentional no-op: Usage is to CreateAndStart<T> under a lock,
        // then call StartOutsideLock() to handle races with Cancel()
    }

public:

    _Requires_no_locks_held_
    void StartOutsideLock(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceLifeCycle, this->Owner.TraceId, "Initializing {0} service ...", serviceName_);

        this->ResolveSystemService(thisSPtr);
    }

protected:

    void OnCancel()
    {
        AcquireExclusiveLock lock(timerLock_);

        this->TryCancelTimer();
    }

    _Requires_lock_held_(timerLock_)
    void TryCancelTimer()
    {
        if (retryTimerSPtr_)
        {
            retryTimerSPtr_->Cancel();
        }
    }

    void ResolveSystemService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->Owner.reliability_->Resolver.BeginResolveServicePartition(
            firstCuid_,
            FabricActivityHeader(),
            this->Owner.Config()->SystemServiceInitializationTimeout,
            [this] (AsyncOperationSPtr const & operation) { this->OnResolveSystemServiceComplete(operation, false); },
            thisSPtr);
        this->OnResolveSystemServiceComplete(operation, true);
    }

    void OnResolveSystemServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        vector<ServiceTableEntry> systemServiceLocations;
        GenerationNumber unused;
        ErrorCode error = this->Owner.reliability_->Resolver.EndResolveServicePartition(operation, systemServiceLocations, unused);

        this->OnResolvedSystemService(error);

        if (error.IsSuccess())
        {
            // Intentional no-op: system service has been created and placed already
            WriteInfo(TraceLifeCycle, this->Owner.TraceId, "Resolved {0} service", serviceName_);
            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else if (error.IsError(ErrorCodeValue::PartitionNotFound) || error.IsError(ErrorCodeValue::ServiceOffline))
        {
            WriteInfo(TraceLifeCycle, this->Owner.TraceId, "Creating {0} service ...", serviceName_);

            auto op = this->BeginCreateSystemService(
                this->Owner.Config()->SystemServiceInitializationTimeout,
                [this](AsyncOperationSPtr const & operation) { this->OnCreateSystemServiceComplete(operation, false); },
                thisSPtr);
            this->OnCreateSystemServiceComplete(op, true);
        }
        else
        {
            this->ScheduleInitializeSystemService(thisSPtr);
        }
    }

    void OnCreateSystemServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = this->EndCreateSystemService(operation);

        this->OnCreatedSystemService(error);

        if (error.IsSuccess() || error.IsError(ErrorCodeValue::FMServiceAlreadyExists))
        {
            // Intentional no-op: Service has been created (but may not have been placed yet)
            //
            WriteInfo(TraceLifeCycle, this->Owner.TraceId, "Successfully created {0} service", serviceName_);
            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            this->ScheduleInitializeSystemService(thisSPtr);
        }
    }

    void ScheduleInitializeSystemService(AsyncOperationSPtr const & thisSPtr)
    {
        {
            AcquireExclusiveLock lock(timerLock_);
            if (this->Owner.State.Value == FabricComponentState::Opened || this->Owner.State.Value == FabricComponentState::Opening)
            {
                this->TryCancelTimer();

                retryTimerSPtr_ = Timer::Create(
                    TimerTagDefault,
                    [this, thisSPtr] (TimerSPtr const& timer)
                    {
                        timer->Cancel();
                        this->ResolveSystemService(thisSPtr);
                    });
            }
        }

        if (retryTimerSPtr_)
        {
            this->OnSchedulingRetry();

            WriteInfo(TraceLifeCycle, this->Owner.TraceId, "Scheduling {0} service initialization ...", serviceName_);
            retryTimerSPtr_->Change(this->Owner.Config()->SystemServiceInitializationRetryInterval);
        }
    }

private:
    FabricNode & owner_;
    wstring serviceName_;
    ConsistencyUnitId firstCuid_;
    TimerSPtr retryTimerSPtr_;
    ExclusiveLock timerLock_;
};

class FabricNode::InitializeNamingServiceAsyncOperation : public FabricNode::InitializeSystemServiceAsyncOperationBase
{
public:
    InitializeNamingServiceAsyncOperation(
        FabricNode & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InitializeSystemServiceAsyncOperationBase(
            owner,
            ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName,
            ConsistencyUnitId(owner.communication_->NamingService.NamingServiceCuids[0]),
            callback, 
            root)
    {
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        Events.ResolvingNamingService(Owner.federation_->IdString, Owner.Instance.InstanceId, this->FirstCuid.Guid);

        InitializeSystemServiceAsyncOperationBase::OnStart(thisSPtr);
    }

    virtual AsyncOperationSPtr BeginCreateSystemService(TimeSpan const timeout, AsyncCallback const & callback, AsyncOperationSPtr const & root)
    {
        Events.CreatingNamingService(
            Owner.federation_->IdString, Owner.Instance.InstanceId,
            this->Owner.Config()->SystemServiceInitializationRetryInterval);

        return this->Owner.communication_->BeginCreateNamingService(timeout, callback, root);
    }

    virtual ErrorCode EndCreateSystemService(AsyncOperationSPtr const & operation)
    {
        return this->Owner.communication_->EndCreateNamingService(operation);
    }

    virtual void OnResolvedSystemService(ErrorCode const & error)
    {
        Events.ResolvedNamingService(Owner.federation_->IdString, Owner.Instance.InstanceId, error.ReadValue());

        switch (error.ReadValue())
        {
            case ErrorCodeValue::Success:
            case ErrorCodeValue::PartitionNotFound:
            case ErrorCodeValue::ServiceOffline:
                // No additional trace
                break;

            default:
                Events.RetryingResolveNamingService(Owner.federation_->IdString, Owner.Instance.InstanceId, error.ReadValue());
        }
    }

    virtual void OnCreatedSystemService(ErrorCode const & error)
    {
        Events.CreatedNamingService(Owner.federation_->IdString, Owner.Instance.InstanceId, error.ReadValue());

        switch (error.ReadValue())
        {
            case ErrorCodeValue::Success:
            case ErrorCodeValue::FMServiceAlreadyExists:
            case ErrorCodeValue::ServiceOffline:
                // No additional trace
                break;

            default:
                Events.RetryingCreateNamingService(Owner.federation_->IdString, Owner.Instance.InstanceId, error.ReadValue());
        }
    }

    virtual void OnSchedulingRetry()
    {
        Events.StartingCreateNamingServiceTimer(Owner.federation_->IdString, Owner.Instance.InstanceId);
    }
};

class FabricNode::InitializeClusterManagerServiceAsyncOperation : public FabricNode::InitializeSystemServiceAsyncOperationBase
{
public:
    InitializeClusterManagerServiceAsyncOperation(
        FabricNode & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InitializeSystemServiceAsyncOperationBase(
            owner,
            ServiceModel::SystemServiceApplicationNameHelper::InternalClusterManagerServiceName,
            ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::CMIdRange),
            callback, 
            root)
    {
    }

protected:

    virtual AsyncOperationSPtr BeginCreateSystemService(TimeSpan const timeout, AsyncCallback const & callback, AsyncOperationSPtr const & root)
    {
        return this->Owner.management_->BeginCreateClusterManagerService(timeout, callback, root);
    }

    virtual ErrorCode EndCreateSystemService(AsyncOperationSPtr const & operation)
    {
        return this->Owner.management_->EndCreateClusterManagerService(operation);
    }
};

class FabricNode::InitializeRepairManagerServiceAsyncOperation : public FabricNode::InitializeSystemServiceAsyncOperationBase
{
public:
    InitializeRepairManagerServiceAsyncOperation(
        FabricNode & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InitializeSystemServiceAsyncOperationBase(
            owner,
            ServiceModel::SystemServiceApplicationNameHelper::InternalRepairManagerServiceName,
            ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::RMIdRange),
            callback,
            root)
    {
    }

protected:

    virtual AsyncOperationSPtr BeginCreateSystemService(TimeSpan const timeout, AsyncCallback const & callback, AsyncOperationSPtr const & root)
    {
        return this->Owner.management_->BeginCreateRepairManagerService(timeout, callback, root);
    }

    virtual ErrorCode EndCreateSystemService(AsyncOperationSPtr const & operation)
    {
        return this->Owner.management_->EndCreateRepairManagerService(operation);
    }
};

class FabricNode::InitializeImageStoreServiceAsyncOperation : public FabricNode::InitializeSystemServiceAsyncOperationBase
{
public:
    InitializeImageStoreServiceAsyncOperation(
        FabricNode & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InitializeSystemServiceAsyncOperationBase(
            owner,
            ServiceModel::SystemServiceApplicationNameHelper::InternalImageStoreServiceName,
            ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::FileStoreIdRange),
            callback,
            root)
    {
    }

protected:

    virtual AsyncOperationSPtr BeginCreateSystemService(TimeSpan const timeout, AsyncCallback const & callback, AsyncOperationSPtr const & root)
    {
        return this->Owner.management_->BeginCreateImageStoreService(timeout, callback, root);
    }

    virtual ErrorCode EndCreateSystemService(AsyncOperationSPtr const & operation)
    {
        return this->Owner.management_->EndCreateImageStoreService(operation);
    }
};

class FabricNode::InitializeFaultAnalysisServiceAsyncOperation : public FabricNode::InitializeSystemServiceAsyncOperationBase
{
public:
    InitializeFaultAnalysisServiceAsyncOperation(
        FabricNode & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InitializeSystemServiceAsyncOperationBase(
            owner,
            ServiceModel::SystemServiceApplicationNameHelper::InternalFaultAnalysisServiceName,
            ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::FaultAnalysisServiceIdRange),
            callback,
            root)
    {
    }

protected:

    virtual AsyncOperationSPtr BeginCreateSystemService(TimeSpan const timeout, AsyncCallback const & callback, AsyncOperationSPtr const & root)
    {
        return this->Owner.management_->BeginCreateFaultAnalysisService(timeout, callback, root);
    }

    virtual ErrorCode EndCreateSystemService(AsyncOperationSPtr const & operation)
    {
        return this->Owner.management_->EndCreateFaultAnalysisService(operation);
    }
};

class FabricNode::InitializeCentralSecretServiceAsyncOperation : public FabricNode::InitializeSystemServiceAsyncOperationBase
{
public:
    InitializeCentralSecretServiceAsyncOperation(
        FabricNode & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InitializeSystemServiceAsyncOperationBase(
            owner,
            ServiceModel::SystemServiceApplicationNameHelper::InternalCentralSecretServiceName,
            ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::CentralSecretServiceIdRange),
            callback,
            root)
    {
    }

protected:

    virtual AsyncOperationSPtr BeginCreateSystemService(TimeSpan const timeout, AsyncCallback const & callback, AsyncOperationSPtr const & root)
    {
        return this->Owner.management_->BeginCreateCentralSecretService(timeout, callback, root);
    }

    virtual ErrorCode EndCreateSystemService(AsyncOperationSPtr const & operation)
    {
        return this->Owner.management_->EndCreateCentralSecretService(operation);
    }
};

class FabricNode::InitializeUpgradeOrchestrationServiceAsyncOperation : public FabricNode::InitializeSystemServiceAsyncOperationBase
{
public:
    InitializeUpgradeOrchestrationServiceAsyncOperation(
        FabricNode & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InitializeSystemServiceAsyncOperationBase(
        owner,
        ServiceModel::SystemServiceApplicationNameHelper::InternalUpgradeOrchestrationServiceName,
        ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::UpgradeOrchestrationServiceIdRange),
        callback,
        root)
    {
    }

protected:

    virtual AsyncOperationSPtr BeginCreateSystemService(TimeSpan const timeout, AsyncCallback const & callback, AsyncOperationSPtr const & root)
    {
        return this->Owner.management_->BeginCreateUpgradeOrchestrationService(timeout, callback, root);
    }

    virtual ErrorCode EndCreateSystemService(AsyncOperationSPtr const & operation)
    {
        return this->Owner.management_->EndCreateUpgradeOrchestrationService(operation);
    }
};

class FabricNode::InitializeBackupRestoreServiceAsyncOperation : public FabricNode::InitializeSystemServiceAsyncOperationBase
{
public:
    InitializeBackupRestoreServiceAsyncOperation(
        FabricNode & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InitializeSystemServiceAsyncOperationBase(
            owner,
            ServiceModel::SystemServiceApplicationNameHelper::InternalBackupRestoreServiceName,
            ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::BackupRestoreServiceIdRange),
            callback,
            root)
    {
    }

protected:

    virtual AsyncOperationSPtr BeginCreateSystemService(TimeSpan const timeout, AsyncCallback const & callback, AsyncOperationSPtr const & root)
    {
        return this->Owner.management_->BeginCreateBackupRestoreService(timeout, callback, root);
    }

    virtual ErrorCode EndCreateSystemService(AsyncOperationSPtr const & operation)
    {
        return this->Owner.management_->EndCreateBackupRestoreService(operation);
    }
};

class FabricNode::InitializeEventStoreServiceAsyncOperation : public FabricNode::InitializeSystemServiceAsyncOperationBase
{
public:
    InitializeEventStoreServiceAsyncOperation(
        FabricNode & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InitializeSystemServiceAsyncOperationBase(
            owner,
            ServiceModel::SystemServiceApplicationNameHelper::InternalEventStoreServiceName,
            ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::EventStoreServiceIdRange),
            callback,
            root)
    {
    }

protected:

    virtual AsyncOperationSPtr BeginCreateSystemService(TimeSpan const timeout, AsyncCallback const & callback, AsyncOperationSPtr const & root)
    {
        return this->Owner.management_->BeginCreateEventStoreService(timeout, callback, root);
    }

    virtual ErrorCode EndCreateSystemService(AsyncOperationSPtr const & operation)
    {
        return this->Owner.management_->EndCreateEventStoreService(operation);
    }
};


// ********************************************************************************************************************
// FabricNode Implementation
//
Common::ErrorCode FabricNode::Create(
    FabricNodeConfigSPtr const & config,
    FabricNodeSPtr & fabricNodeSPtr)
{
    return Test_Create(config, Hosting2::HostingSubsystemFactory, ProcessTerminationService::Create, fabricNodeSPtr);
}

Common::ErrorCode FabricNode::Test_Create(
    FabricNodeConfigSPtr const & config,
    Hosting2::HostingSubsystemFactoryFunctionPtr hostingSubsystemFactory,
    Common::ProcessTerminationServiceFactory processTerminationServiceFactory,
    FabricNodeSPtr & fabricNodeSPtr)
{
    fabricNodeSPtr = FabricNodeSPtr(new FabricNode(config, hostingSubsystemFactory, processTerminationServiceFactory));

    ComponentRootWPtr thisWPtr = fabricNodeSPtr;
    std::function<void(int, wstring const &, wstring const &)> callback = [thisWPtr](int reportCode, wstring const & dynamicProperty, wstring const & extraDescription)
    {
        if (auto thisSPtr = thisWPtr.lock())
        {
            ((FabricNode*)thisSPtr.get())->ReportLeaseHealth(reportCode, dynamicProperty, extraDescription);
        }
    };
    LeaseWrapper::LeaseAgent::SetHealthReportCallback(callback);

    return fabricNodeSPtr->constructError_;
}

FabricNode::FabricNode(
    FabricNodeConfigSPtr const & config,
    Hosting2::HostingSubsystemFactoryFunctionPtr hostingSubsystemFactory,
    Common::ProcessTerminationServiceFactory processTerminationFactory)
    : ComponentRoot(CommonConfig::GetConfig().EnableReferenceTracking)
    , config_(config)
{
    NodeInstance temp(NodeId::MaxNodeId, numeric_limits<uint64>::max());

    // Reserve enough capacity to avoid memory allocation when updating the traceId with the instance
    this->SetTraceId(config_->NodeId, sizeof(temp.ToString()) * 2);

    WriteInfo(TraceLifeCycle, TraceId, "ctor");

    if(!Path::IsPathRooted(config_->WorkingDir))
    {
        workingDir_ = Path::Combine(Directory::GetCurrentDirectory(), config_->WorkingDir);
    }
    else
    {
        workingDir_ = config_->WorkingDir;
    }

    WriteInfo(TraceLifeCycle, TraceId, "WorkingDir is set to {0}", workingDir_);

    NodeId nodeId;
    if(!NodeId::TryParse(config_->NodeId, nodeId))
    {
        Common::Assert::CodingError("Failed to parse {0} as NodeId", config_->NodeId);
    }

    if (HttpGateway::HttpGatewayConfig::GetConfig().IsEnabled)
    {
        auto httpGatewayPort = TcpTransportUtility::ParsePortString(config_->HttpGatewayListenAddress);
        config_->SetHttpGatewayPort(httpGatewayPort);
        WriteInfo(TraceLifeCycle, TraceId, "HttpGatewayPort = {0}", config_->HttpGatewayPort());
    }

    constructError_ = SetClusterSpnIfNeeded();
    if(!constructError_.IsSuccess())
    {
        return;
    }

    SecuritySettings clusterSecuritySettings;
    constructError_ = CreateClusterSecuritySettings(clusterSecuritySettings);
    if(!constructError_.IsSuccess())
    {
        return;
    }

    Common::Uri nodeFaultDomainId;
    if (!config_->NodeFaultDomainIds.empty())
    {
        nodeFaultDomainId = config_->NodeFaultDomainIds[0];
    }

    WriteInfo(TraceSecurity, TraceId, "cluster SecuritySettings initialized to {0}", clusterSecuritySettings);
    auto storeFactory = std::make_shared<Store::StoreFactory>();
    auto federation = std::make_shared<Federation::FederationSubsystem>(
        NodeConfig(nodeId, config_->NodeAddress, config_->LeaseAgentAddress, workingDir_),
        config_->NodeVersion.Version.CodeVersion,
        storeFactory,
        nodeFaultDomainId,
        clusterSecuritySettings,
        *this);

    bool zombieMode = IsZombieModeFilePresent();

    if (zombieMode)
    {        
        InitializeZombieMode(*federation, clusterSecuritySettings);
        if (!constructError_.IsSuccess())
        {
            WriteInfo(
                TraceLifeCycle, 
                TraceId,
                "FabricNode node:{0}, constructError_ is:{1}",
                config_->InstanceName,
                constructError_);
        }
        else
        {
            federation_ = move(federation);            
            auto healthClient = make_shared<HealthReportingComponent>(*zombieModeEntreeService_, TraceId, false);
            healthClient_ = healthClient;
        }

        return;
    }

    auto containerAppsEnabled = HostingConfig::GetConfig().FabricContainerAppsEnabled;

    auto runtimeServiceAddressTls = containerAppsEnabled ? 
        TcpTransportUtility::ConstructAddressString(config_->IPAddressOrFQDN, 0) : L"";

    auto ipcServer = make_unique<IpcServer>(
        *this,
        config_->RuntimeServiceAddress,
        runtimeServiceAddressTls,
        federation->Id.ToString(),
        true /* allow use of unreliable transport */,
        L"FabricNode");

    SecuritySettings ipcServerSecuritySettings;
    constructError_ = SecuritySettings::CreateNegotiateServer(
        FabricConstants::WindowsFabricAllowedUsersGroupName,
        ipcServerSecuritySettings);

    //allow clients in admin role to send oversized messages
    ipcServerSecuritySettings.EnableAdminRole(FabricConstants::WindowsFabricAdministratorsGroupName);

    if(!constructError_.IsSuccess())
    {
        WriteError(
            TraceSecurity, 
            TraceId,
            "ipcServer->SecuritySettings.CreateNegotiateServer error={0}",
            constructError_);
        return;
    }

    WriteInfo(TraceSecurity, TraceId, "Initializing IPC server SecuritySettings to {0}", ipcServerSecuritySettings);
    constructError_ = ipcServer->SetSecurity(ipcServerSecuritySettings);
    if(!constructError_.IsSuccess())
    {
        WriteError(
            TraceSecurity, 
            TraceId,
            "ipcServer->SetSecurity error={0}",
            constructError_);
        return;
    }

    if (containerAppsEnabled)
    {
        SecuritySettings ipcServerTlsSecuritySettings;
        constructError_ = CreateIpcServerTlsSecuritySettings(ipcServerTlsSecuritySettings);
        if (!constructError_.IsSuccess()) { return; }

        WriteInfo(TraceSecurity, TraceId, "Initializing IPC server TLS SecuritySettings to {0}", ipcServerTlsSecuritySettings);
        constructError_ = ipcServer->SetSecurityTls(ipcServerTlsSecuritySettings);
        if(!constructError_.IsSuccess())
        {
            WriteError(
                TraceSecurity, 
                TraceId,
                "ipcServer->SetSecurityTls error={0}",
                constructError_);
            return;
        }
    }

    wstring fabricDataRoot;
    auto error = FabricEnvironment::GetFabricDataRoot(fabricDataRoot);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceLifeCycle,
            TraceId,
            "Error getting FabricDataRoot for KtlLogger. Error:{0}",
            error);

        return;
    }
    
    KtlLogger::KtlLoggerNodeSPtr ktlLogger = make_shared<KtlLogger::KtlLoggerNode>(*this, fabricDataRoot, config);

    HostingSubsystemConstructorParameters hostingParameters = { 0 };
    hostingParameters.Federation = federation;
    hostingParameters.IpcServer = ipcServer.get();
    hostingParameters.NodeConfig = config_;
    hostingParameters.NodeWorkingDirectory = workingDir_;
    hostingParameters.Root = this;
    hostingParameters.ServiceTypeBlocklistingEnabled = !FailoverConfig::GetConfig().DummyPLBEnabled;
    hostingParameters.KtlLoggerNode = ktlLogger;

    constructError_ = HostingSubsystem::GetDeploymentFolder(*config, workingDir_, hostingParameters.DeploymentFolder);
    if (!constructError_.IsSuccess())
    {
        return;
    }
    // Factory comes either from Fabric.Exe where it is the factory exposed by hosting subsystem
    // or from fabric test where it is the wrapper around the hosting subsystem
    auto hosting = hostingSubsystemFactory(hostingParameters);

    TestabilitySubsystemConstructorParameters testabilityContructParameters = { 0 };
    testabilityContructParameters.Federation = federation;
    testabilityContructParameters.IpcServer = ipcServer.get();
    testabilityContructParameters.NodeConfig = config_;
    testabilityContructParameters.NodeWorkingDirectory = workingDir_;
    testabilityContructParameters.HostingSubSytem = hosting;
    testabilityContructParameters.Root = this;

    auto testability = Testability::TestabilitySubsystemFactory(testabilityContructParameters);


    RuntimeSharingHelperConstructorParameters runtimeSharingHelperConstructorParameters;
    runtimeSharingHelperConstructorParameters.Root = this;
    runtimeSharingHelperConstructorParameters.KtlLogger = ktlLogger;
    auto runtimeSharingHelper = Hosting2::RuntimeSharingHelperFactory(runtimeSharingHelperConstructorParameters);

    ReliabilitySubsystemConstructorParameters reliabilityConstructorParameters;
    reliabilityConstructorParameters.Federation = federation;
    reliabilityConstructorParameters.IpcServer = ipcServer.get();
    reliabilityConstructorParameters.HostingSubsystem = hosting.get();
    reliabilityConstructorParameters.RuntimeSharingHelper = runtimeSharingHelper.get();
    reliabilityConstructorParameters.NodeConfig = config;
    reliabilityConstructorParameters.NodeCapacityMap = map<wstring, uint>();
    reliabilityConstructorParameters.SecuritySettings = &clusterSecuritySettings;
    reliabilityConstructorParameters.WorkingDir = workingDir_;
    reliabilityConstructorParameters.Root = this;
    reliabilityConstructorParameters.StoreFactory = storeFactory;
    reliabilityConstructorParameters.ProcessTerminationService = processTerminationFactory();
    reliabilityConstructorParameters.KtlLoggerNode = ktlLogger;

    auto reliability = Reliability::ReliabilitySubsystemFactory(reliabilityConstructorParameters);

    auto networkInventoryAgent = std::make_shared<NetworkInventoryAgent>(*this, *federation, *reliability, hosting);
    hosting->NetworkInventoryAgent = networkInventoryAgent;
        
    auto communication = Common::make_unique<CommunicationSubsystem>(
        *reliability,
        *hosting,
        *runtimeSharingHelper,
        *ipcServer,
        config_->GatewayIpcAddress,
        config_->NamingReplicatorAddress,
        clusterSecuritySettings,
        workingDir_,
        config_,
        *this);

    auto namingMessageProcessorSPtr = communication->NamingService.GetGateway();
    
    // TODO: Since fabricclient constructed here no goes via tcp transport, this should be changed to use
    //       fabricclient's IHealthClient interface.
    auto healthClient = make_shared<HealthReportingComponent>(
        dynamic_cast<HealthReportingTransport&>(*namingMessageProcessorSPtr), TraceId, false);

    reliability->InitializeHealthReportingComponent(healthClient);
    hosting->InitializeHealthReportingComponent(healthClient);
    testability->InitializeHealthReportingComponent(healthClient);

    IClientFactoryPtr clientFactoryPtr;
    constructError_ = ClientFactory::CreateLocalClientFactory(IServiceNotificationEventHandlerPtr(), namingMessageProcessorSPtr, clientFactoryPtr);
    if (!constructError_.IsSuccess())
    {
        WriteError(TraceLifeCycle, TraceId, "Failed to create local client factory: error={0}", constructError_);
        return;
    }

    IClientSettingsPtr settingsPtr;
    constructError_ = clientFactoryPtr->CreateSettingsClient(settingsPtr);
    if (!constructError_.IsSuccess()) 
    { 
        WriteError(TraceLifeCycle, TraceId, "Failed to get settings from local client factory: error={0}", constructError_);
        return;
    }

    auto settings = settingsPtr->GetSettings(); 
    settings.SetClientFriendlyName(wformatString("FabricNode-{0}", federation->Instance));
    constructError_ = settingsPtr->SetSettings(move(settings));
    if (!constructError_.IsSuccess()) 
    { 
        WriteError(TraceLifeCycle, TraceId, "Failed to update settings on local client: error={0}", constructError_);
        return;
    }

#if !defined(PLATFORM_UNIX)
    IQueryClientPtr queryClientPtr;
    auto isBackupRestoreServiceEnabled = BackupRestoreServiceConfig::IsBackupRestoreServiceConfigured();
    if (isBackupRestoreServiceEnabled)
    {
        constructError_ = clientFactoryPtr->CreateQueryClient(queryClientPtr);
        if (!constructError_.IsSuccess())
        {
            WriteError(TraceLifeCycle, TraceId, "Failed to get query client ptr from local client factory: error={0}", constructError_);
            return;
        }
    }
#endif

    hosting->InitializePassThroughClientFactory(clientFactoryPtr);
    reliability->InitializeClientFactory(clientFactoryPtr);
    communication->NamingService.InitializeClientFactory(clientFactoryPtr);

    auto management = Common::make_unique<ManagementSubsystem>(
        reliability->FederationWrapper,        
        *runtimeSharingHelper,
        hosting->FabricActivatorClientObj,
        reliability->AdminClient,
        reliability->Resolver,
        config_->ClientConnectionAddress,
        move(clientFactoryPtr),
        config_->ClusterManagerReplicatorAddress,
        workingDir_,
        config_->InstanceName,
        clusterSecuritySettings,
        *this);

#if !defined(PLATFORM_UNIX)
    BackupRestoreAgentComponent::BackupRestoreAgentUPtr backupRestoreAgent = nullptr;
    if (isBackupRestoreServiceEnabled)
    {
        // Initialize Backup restore agent.
        backupRestoreAgent = BackupRestoreAgentComponent::BackupRestoreAgent::Create(
            *this,
            hosting,
            *federation,
            reliability->Resolver,
            reliability->AdminClient,
            queryClientPtr,
            *ipcServer,
            workingDir_);
    }
#endif

    communication->EnableServiceRoutingAgent();

    // !!! no more subsystem construction beyond this point !!!

    federation_ = std::move(federation);
    ktlLogger_ = std::move(ktlLogger);
    ipcServer_ = move(ipcServer);
    hosting_ = hosting;
    networkInventoryAgent_ = move(networkInventoryAgent);
    runtimeSharingHelper_ = move(runtimeSharingHelper);
    reliability_ = std::move(reliability);
    communication_ = std::move(communication);
    management_ = std::move(management);
    healthClient_ = std::move(healthClient);
    testability_ = testability;
#if !defined(PLATFORM_UNIX)
    backupRestoreAgent_ = isBackupRestoreServiceEnabled ? std::move(backupRestoreAgent) : nullptr;
#endif
    hosting->InitializeLegacyTestabilityRequestHandler(testability_);

    CacheStaticTraceStates();

    stateTraceInterval_ = config_->StateTraceInterval;
}

ErrorCode FabricNode::CreateClusterSecuritySettings(SecuritySettings & clusterSecuritySettings)
{
    SecurityConfig & securityConfig = SecurityConfig::GetConfig();
    auto err = SecuritySettings::FromConfiguration(
        securityConfig.ClusterCredentialType,
        config_->ClusterX509StoreName,
        wformatString(X509Default::StoreLocation()),
        config_->ClusterX509FindType,
        config_->ClusterX509FindValue,
        config_->ClusterX509FindValueSecondary,
        securityConfig.ClusterProtectionLevel,
        securityConfig.ClusterCertThumbprints,
        securityConfig.ClusterX509Names,
        securityConfig.ClusterCertificateIssuerStores,
        securityConfig.ClusterAllowedCommonNames,
        securityConfig.ClusterCertIssuers,
        securityConfig.ClusterSpn,
        securityConfig.ClusterIdentities,
        clusterSecuritySettings);

    if (!err.IsSuccess())
    {
        WriteError(
            TraceSecurity,
            TraceId,
            "clusterSecuritySettings.FromConfiguration error={0}",
            err);
        return err;
    }

    clusterSecuritySettings.EnablePeerToPeerMode();

    if (clusterSecuritySettings.SecurityProvider() == SecurityProvider::Ssl)
    {
        shouldReportCrlHealth_ = true;

        clusterSecuritySettings.SetX509CertType(L"cluster");

        // Use callback for component specific settings so that we can easily take advantage of dynamic settings
        clusterSecuritySettings.SetX509CertChainFlagCallback([] { return FederationConfig::GetConfig().X509CertChainFlags; });
        clusterSecuritySettings.SetCrlOfflineIgnoreCallback([](bool) { return FederationConfig::GetConfig().IgnoreCrlOfflineError; });
    }

    return err;
}

void FabricNode::InitializeZombieMode(FederationSubsystem & federation, SecuritySettings const & clusterSecuritySettings)
{
    WriteInfo(
        TraceLifeCycle, 
        TraceId,
        "Enter InitializeZombieMode");

    unique_ptr<DummyIRouter> dummyIRouter = Common::make_unique<DummyIRouter>();
    unique_ptr<FederationWrapper> federationWrapper =  Common::make_unique<FederationWrapper>(federation);    
    unique_ptr<ServiceResolver> serviceResolver = Common::make_unique<ServiceResolver>(*federationWrapper, *this);
    unique_ptr<ServiceAdminClient> serviceAdminClient = Common::make_unique<ServiceAdminClient>(*federationWrapper);

    WriteInfo(
        TraceLifeCycle, 
        TraceId,
        "InitializeZombieMode - creating EntreeService");

    std::unique_ptr<Naming::EntreeService> entreeService = Common::make_unique<Naming::EntreeService>(
        *dummyIRouter,
        *serviceResolver,
        *serviceAdminClient,
        nullptr,
        NodeInstance(), 
        config_->NodeAddress,
        config_,
        clusterSecuritySettings, 
        *this,
        true);

    zombieModeEntreeService_ = move(entreeService);  
    
    WriteInfo(
        TraceLifeCycle, 
        TraceId,
        "Exit InitializeZombieMode");
}

void FabricNode::EnableCrlHealthReportIfNeeded()
{
    if (!shouldReportCrlHealth_) return;

    ComponentRootWPtr thisWPtr = shared_from_this();
    CrlOfflineErrCache::GetDefault().SetHealthReportCallback(
        [thisWPtr] (size_t errCount, wstring const & report)
        {
            if (auto thisSPtr = thisWPtr.lock())
            {
                ((FabricNode*)thisSPtr.get())->ReportCrlHealth(errCount, report);
            }
        });
}

void FabricNode::ReportCrlHealth(size_t errCount, std::wstring const & report)
{
    auto nodeInfo = ServiceModel::EntityHealthInformation::CreateNodeEntityHealthInformation(
        federation_->Instance.Id.IdValue,
        config_->InstanceName,
        federation_->Instance.InstanceId);
            
    ServiceModel::AttributeList attributes;
    attributes.AddAttribute(*ServiceModel::HealthAttributeNames::NodeName, config_->InstanceName);

    const wstring dynamicProperty(L"FabricNode");
    WriteInfo(TraceHealth, TraceId, "ReportCrlHealth: errCount = {0}, '{1}'", errCount, report);

    ErrorCode error;
    SystemHealthReportCode::Enum reportCode;

    if (errCount > 0)
    {
        reportCode = SystemHealthReportCode::FabricNode_CertificateRevocationListOffline;
        error = healthClient_->AddHealthReport(
            ServiceModel::HealthReport::CreateSystemHealthReport(
                reportCode,
                move(nodeInfo),
                dynamicProperty,
                report,
                SecurityConfig::GetConfig().CrlOfflineHealthReportTtl,
                move(attributes)));
    }
    else
    {
        reportCode = SystemHealthReportCode::FabricNode_CertificateRevocationListOk;
        error = healthClient_->AddHealthReport(
            ServiceModel::HealthReport::CreateSystemRemoveHealthReport(
                reportCode,
                move(nodeInfo),
                dynamicProperty));
    }

    if (!error.IsSuccess())
    {
        WriteInfo(TraceSecurity, TraceId, "ReportCrlHealth: AddHealthReport({0}) returned {0} for '{1}'", reportCode, error, report);
    }
}

void FabricNode::EnableTransportHealthReporting()
{
    weak_ptr<ComponentRoot> rootWPtr = shared_from_this();
    IDatagramTransport::SetHealthReportingCallback([rootWPtr](SystemHealthReportCode::Enum reportCode, wstring const & dynamicProperty, wstring const & description, TimeSpan ttl)
    {
        if (auto thisSPtr = rootWPtr.lock())
        {
            ((FabricNode&)(*thisSPtr)).ReportTransportHealth(reportCode, dynamicProperty, description, ttl);
        }
    });
}

DateTime FabricNode::DetermineStoppedNodeDuration()
{
    // access the file
    wstring pathToFile = zombieModeEntreeService_->Properties.AbsolutePathToStartStopFile;
    wstring lockFile = wformatString("{0}.lock", pathToFile);
    vector<wstring> tokens;

    size_t size = 0;
    string text;
    {
        ExclusiveFile fileLock(lockFile);
        auto err = fileLock.Acquire(TimeSpan::FromSeconds(10));
        if (!err.IsSuccess())
        {
            WriteWarning(
                TraceLifeCycle,
                "{0}: Failed to acquire lock file '{1}': {2}",
                TraceId,
                lockFile,
                err);
        
             Common::Assert::CodingError("Failed to acquire lock file '{0}'", lockFile);
        }

        // Read the file
        std::string absolutePathToStartFile;
        StringUtility::Utf16ToUtf8(pathToFile, absolutePathToStartFile);

        File fileReader;
        auto error = fileReader.TryOpen(pathToFile, Common::FileMode::Open, Common::FileAccess::Read, Common::FileShare::Read);
        if (!error.IsSuccess())
        {
             Common::Assert::CodingError("Failed to open lock file '{0}'", lockFile);
        }

        int64 fileSize = fileReader.size();
        size = static_cast<size_t>(fileSize);
       
        text.resize(size);
        fileReader.Read(&text[0], static_cast<int>(size));
        fileReader.Close();
    }
    
    // Convert to wstring
    wstring convertedFileContents;
    StringWriter(convertedFileContents).Write(text);
 
    // debug
    WriteInfo(
            TraceLifeCycle,
            "{0}:  convertedFileContents='{1}'",
            TraceId,
            convertedFileContents);
    
    StringUtility::Split<wstring>(convertedFileContents, tokens, L" ");
    size = tokens.size();

    WriteInfo(
        TraceLifeCycle,
        "{0}: number of tokens='{1}'",
        TraceId,
        size);
     
    if (size == 1)
    {
        // Don't do anything extra, this node was stopped with (older API) StopNodeAsync which does not take in a duration.  The "duration" is indefinite until StartNodeAsync() is called
        WriteInfo(
            TraceLifeCycle,
            "{0}: This node was stopped using StopNodeAsync(), returning",
            TraceId);
            
        return DateTime::MaxValue;
    }
    else if (size != 3)       
    {
        Common::Assert::CodingError("Failed to parse StartStop file has unexpected format.  Contents='{0}'", convertedFileContents);
    }        

    // Parse the duration
    // The format is [instance id] [expiration date] [expiration time within date]
    int64 instanceIdFromFile = 0;
    wstring expiredDate = tokens[1];
    wstring expiredTime = tokens[2];

    if (!TryParseInt64(tokens[0], instanceIdFromFile))
    {
        WriteError(
            TraceLifeCycle,
            "{0}: Could not parse instance id from string='{1}'",
            TraceId,
            tokens[0]);
        Common::Assert::CodingError("Could not parse instance id from string='{0}'", tokens[0]);
    }

    wstring tempDateTime = expiredDate + L" " + expiredTime;
        
    WriteInfo(
        TraceLifeCycle,
        "{0}: Instance id in file is: {1}, expiredDate is '{2}', expiredTime is '{3}', concat='{4}'",
        TraceId,
        instanceIdFromFile,        
        expiredDate,
        expiredTime,
        tempDateTime);      

    DateTime targetDate;
    if (!DateTime::TryParse(tempDateTime, targetDate))
    {   
        Common::Assert::CodingError("Could not parse DateTime from string='{0}'", tempDateTime);
    }
    
    WriteInfo(
        TraceLifeCycle,
        "{0}: date converted to date time = '{1}'",
        TraceId,
        targetDate);      

    WriteInfo(
        TraceLifeCycle,
        "{0}: stopped node state will expire at '{1}'",
        TraceId,
        targetDate);

    return targetDate;
}

void FabricNode::ReportTransportHealth(SystemHealthReportCode::Enum reportCode, wstring const & dynamicProperty, wstring const & description, TimeSpan ttl)
{
    WriteInfo(
        TraceHealth, TraceId,
        "Reporting transport health: reportCode = {0}, dynamicProperty = {1}, description = {2}",
        reportCode, dynamicProperty, description);

    auto nodeInfo = ServiceModel::EntityHealthInformation::CreateNodeEntityHealthInformation(
        federation_->Instance.Id.IdValue,
        config_->InstanceName,
        federation_->Instance.InstanceId);
            
    ServiceModel::AttributeList attributes;
    attributes.AddAttribute(*ServiceModel::HealthAttributeNames::NodeName, config_->InstanceName);

    healthClient_->AddHealthReport(
        ServiceModel::HealthReport::CreateSystemHealthReport(
            reportCode,
            move(nodeInfo),
            dynamicProperty,
            description,
            ttl,
            move(attributes)));
}

void FabricNode::ReportLeaseHealth(int reportCode, wstring const & dynamicProperty, wstring const & extraDescription)
{
    auto logLevel = LogLevel::Warning;
    auto safetyMargin = SecurityConfig::GetConfig().CertificateExpirySafetyMargin;
    auto now = DateTime::Now();
    
    auto nodeInfo = ServiceModel::EntityHealthInformation::CreateNodeEntityHealthInformation(
        federation_->Instance.Id.IdValue,
        config_->InstanceName,
        federation_->Instance.InstanceId);

    TimeSpan ttl = TimeSpan::FromMinutes(30);// 30 minutes

    ServiceModel::AttributeList attributes;
    attributes.AddAttribute(*ServiceModel::HealthAttributeNames::NodeName, config_->InstanceName);

    WriteTrace(
        logLevel, TraceLifeCycle, TraceId,
        "Reporting health: reportCode = {0}, dynamicProperty = {1}, extraDescription = {2}, ttl = {3}",
        reportCode, extraDescription, extraDescription, ttl);

    healthClient_->AddHealthReport(
        ServiceModel::HealthReport::CreateSystemHealthReport(
            (Common::SystemHealthReportCode::Enum) reportCode,
            move(nodeInfo),
            dynamicProperty,
            extraDescription,
            ttl,
            move(attributes)));
}

ErrorCode FabricNode::SetClusterSpnIfNeeded()
{
    SecurityConfig & securityConfig = SecurityConfig::GetConfig();
    ErrorCode error;
    if (!securityConfig.ClusterSpn.empty())
    {
        return error;
    }

    SecurityProvider::Enum clusterSecurityProvider;
    error = SecurityProvider::FromCredentialType(securityConfig.ClusterCredentialType, clusterSecurityProvider);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceSecurity,
            TraceId,
            "Failed to parse ClusterCredentialType '{0}': {1}",
            securityConfig.ClusterCredentialType,
            error);

        return error;
    }

    SecurityProvider::Enum serverSecurityProvider;
    error = SecurityProvider::FromCredentialType(securityConfig.ServerAuthCredentialType, serverSecurityProvider);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceSecurity,
            TraceId,
            "Failed to parse ServerAuthCredentialType '{0}': {1}",
            securityConfig.ClusterCredentialType,
            error);

        return error;
    }

    if (!SecurityProvider::IsWindowsProvider(clusterSecurityProvider) &&
        !SecurityProvider::IsWindowsProvider(serverSecurityProvider))
    {
        return error;
    }

    TransportSecurity transportSecurity;
    if (transportSecurity.RunningAsMachineAccount())
    {
        if (TcpTransportUtility::IsLoopbackAddress(config_->LeaseAgentAddress))
        {
            // LeaseAgentAddress is a loopback address, so remote lease agent must be running as the
            // same machine account, this is probably a development/testing environment. Setting
            // ClusterSpn to local identity here to avoid adding complexity in lease driver to compute
            // remote SPN for testing.
            securityConfig.ClusterSpn = transportSecurity.LocalWindowsIdentity();
        }

        return error;
    }

    securityConfig.ClusterSpn = transportSecurity.LocalWindowsIdentity();
    WriteWarning(
        TraceSecurity,
        TraceId,
        "Not runnning as machine account, ClusterSpn is not set, fall back on local identity {0}",
        transportSecurity.LocalWindowsIdentity());

    return error;
}

void FabricNode::RegisterForConfigUpdate()
{
    WriteInfo(TraceLifeCycle, TraceId, "Register for configuration updates");
    weak_ptr<ComponentRoot> rootWPtr = this->shared_from_this();

    SecurityConfig & securityConfig = SecurityConfig::GetConfig();

    //X509: cluster settings
    config_->ClusterX509StoreNameEntry.AddHandler(
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });
    config_->ClusterX509FindTypeEntry.AddHandler(
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });
    config_->ClusterX509FindValueEntry.AddHandler(
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });
    config_->ClusterX509FindValueSecondaryEntry.AddHandler(
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });

    securityConfig.ClusterCertThumbprintsEntry.AddHandler(
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.ClusterX509NamesEntry.AddHandler(
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.ClusterAllowedCommonNamesEntry.AddHandler(
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.ClusterCertIssuersEntry.AddHandler(
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.ClusterCertificateIssuerStoresEntry.AddHandler(
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });

    // Negotiate/Kerberos: cluster settings
    securityConfig.ClusterIdentitiesEntry.AddHandler(
        [rootWPtr] (EventArgs const&)
        {
            ClusterSecuritySettingUpdateHandler(rootWPtr);
        });
}

void FabricNode::ClusterSecuritySettingUpdateHandler(weak_ptr<ComponentRoot> const & rootWPtr)
{
    auto rootSPtr = rootWPtr.lock();
    if (rootSPtr)
    {
        FabricNode* thisPtr = static_cast<FabricNode*>(rootSPtr.get());
        if (!thisPtr->OnClusterSecuritySettingUpdated().IsSuccess())
        {
            thisPtr->OnNodeFailed();
        }
    }
}

ErrorCode FabricNode::OnClusterSecuritySettingUpdated()
{
    SecuritySettings clusterSecuritySettings;

    AcquireWriteLock writeLock(clusterSecuritySettingsUpdateLock_);

    auto error = CreateClusterSecuritySettings(clusterSecuritySettings);
    if(!error.IsSuccess())
    {
        WriteError(
            TraceSecurity, TraceId,
            "OnClusterSecuritySettingUpdated: CreateClusterSecuritySettings failed: {0}",
            error);
        return error;
    }

    error = this->SetClusterSecurity(clusterSecuritySettings);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceSecurity, TraceId,
            "OnClusterSecuritySettingUpdated: SetClusterSecurity failed: error={0}",
            error);
        return error;
    }

    WriteInfo(TraceSecurity, TraceId, "OnClusterSecuritySettingUpdated: update is successful");
    return error;
}

ErrorCode FabricNode::SetClusterSecurity(SecuritySettings const & securitySettings)
{
    WriteInfo(TraceSecurity, TraceId, "cluster SecuritySettings updated to {0}", securitySettings);

    auto error = federation_->SetSecurity(securitySettings);
    if (!error.IsSuccess())
    {
        WriteError(TraceSecurity, TraceId, "Failed to set cluster security on federation: {0}", error);
        return error;
    }

    error = reliability_->SetSecurity(securitySettings);
    if (!error.IsSuccess())
    {
        WriteError(TraceSecurity, TraceId, "Failed to set cluster security on reliability: {0}", error);
        return error;
    }

    error = communication_->SetClusterSecurity(securitySettings);
    if (!error.IsSuccess())
    {
        WriteError(TraceSecurity, TraceId, "Failed to set cluster security on communication: {0}", error);
        return error;
    }

    if (management_->IsEnabled)
    {
        error = management_->SetClusterSecurity(securitySettings);
        if (!error.IsSuccess())
        {
            WriteError(TraceSecurity, TraceId, "Failed to set cluster security on management: {0}", error);
            return error;
        }
    }

    if (HostingConfig::GetConfig().FabricContainerAppsEnabled && SecurityConfig::GetConfig().UseClusterCertForIpcServerTlsSecurity)
    {
        SecuritySettings ipcServerTlsSecuritySettings;
        error = CreateIpcServerTlsSecuritySettings(ipcServerTlsSecuritySettings);
        if (!error.IsSuccess()) { return error; }

        WriteInfo(TraceSecurity, TraceId, "Initializing IPC server TLS SecuritySettings to {0}", ipcServerTlsSecuritySettings);
        error = this->ipcServer_->SetSecurityTls(ipcServerTlsSecuritySettings);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceSecurity,
                TraceId,
                "ipcServer_->SetSecurityTls error={0}",
                error);
            return error;
        }
    }

    return error;
}

void FabricNode::CacheStaticTraceStates()
{
    faultDomainString_ = L"";
    for (size_t i = 0; i < config_->NodeFaultDomainIds.size(); ++i)
    {
        if (i > 0)
        {
            faultDomainString_ = faultDomainString_ + L",";
        }
        faultDomainString_ = faultDomainString_ + config_->NodeFaultDomainIds[i].ToString();
    }

    vector<Endpoint> addresses;
    ErrorCode error = TcpTransportUtility::GetLocalAddresses(addresses);
    addressString_ = L"";
    for (size_t i = 0; i < addresses.size(); ++i)
    {
        if (i > 0)
        {
            addressString_ = addressString_ + L",";
        }
        addressString_ = addressString_ + addresses[i].ToString();
    }
    
    error = TcpTransportUtility::GetLocalFqdn(hostNameString_);
    if (!error.IsSuccess())
    {
        hostNameString_ = L"";
    }

    isSeedNode_ = federation_->IsSeedNode(federation_->Id);
}

void FabricNode::TraceState()
{
    Events.State(
        federation_->IdString,
        Instance.InstanceId,
        config_->UpgradeDomainId,
        faultDomainString_, 
        addressString_,
        hostNameString_,
        isSeedNode_,
        wformatString(federation_->Phase),
        wformatString(config_->NodeVersion.Version), 
        config_->InstanceName);

    int64 lowerBound, upperBound;
    bool isAuthority = false;
    int64 epoch = federation_->GetGlobalTimeInfo(lowerBound, upperBound, isAuthority);
    if (epoch > 0)
    {
        Events.Time(federation_->IdString, DateTime::Now(), epoch, DateTime(lowerBound), DateTime(upperBound));
    }
}

void FabricNode::StartTracingTimer()
{
    AcquireExclusiveLock lock(timerLock_);

    auto thisRoot = this->CreateComponentRoot();
    static StringLiteral const TraceTimerTag("FabricNodeTrace");
    tracingTimer_ = Timer::Create(
        TraceTimerTag,
        [this, thisRoot] (TimerSPtr const &) { this->TraceState(); },
        true,
        Throttle::GetThrottle()->MonitorCallbackEnv());

    tracingTimer_->Change(stateTraceInterval_, stateTraceInterval_);
}

void FabricNode::InitializeStopExpirationTimer()
{        
    auto aRoot = this->CreateComponentRoot();
    static StringLiteral const ExpiredTag("StopNodeExpiredTag");
    DateTime expirationTime = DetermineStoppedNodeDuration();
    if (expirationTime.RemainingDuration()== TimeSpan::Zero)
    {
        ExitZombieMode();
    }
    
    if (expirationTime != DateTime::MaxValue)
    {
        stopNodeExpiredTimer_ = Timer::Create(
            ExpiredTag,
            [this, aRoot] (TimerSPtr const &) { this->ExitZombieMode(); },
            true);
            
        stopNodeExpiredTimer_->Change(expirationTime.RemainingDuration());
    }
}

void FabricNode::ExitZombieMode()
{    
    wstring pathToFile = zombieModeEntreeService_->Properties.AbsolutePathToStartStopFile;
    wstring lockFile = wformatString("{0}.lock", pathToFile);

    WriteInfo(
        TraceLifeCycle,
        "{0}: timer has expired",
        TraceId);    

    ExclusiveFile fileLock(lockFile);
    auto err = fileLock.Acquire(TimeSpan::FromSeconds(10));
    if (!err.IsSuccess())
    {
        WriteWarning(
            TraceLifeCycle,
            "{0}: Failed to acquire lock file '{1}': {2}",
            TraceId,
            lockFile,
            err);
        Common::Assert::CodingError("Could not acquire lock file '{0}'", lockFile);
    }
    
    ErrorCode error = File::Delete2(pathToFile);
    if (!error.IsSuccess()) 
    { 
        WriteWarning(
            TraceLifeCycle,
            "{0}: Deleting startstop file '{1}' failed with {2}.",
            TraceId,
            pathToFile,
            error);   
        Common::Assert::CodingError("Could not delete '{0}'", pathToFile);
    }
    
    ExitProcess((UINT)(ErrorCodeValue::ProcessDeactivated & 0x0000FFFF));
}

void FabricNode::CancelTimers()
{
    vector<AsyncOperationSPtr> toCancel;
    {
        AcquireExclusiveLock lock(timerLock_);

        toCancel = move(activeSystemServiceOperations_);

        activeSystemServiceOperations_.clear();

        if (tracingTimer_)
        {
            tracingTimer_->Cancel();
            stateTraceInterval_ = TimeSpan::MaxValue;
        }
    }

    for (auto it = toCancel.begin(); it != toCancel.end(); ++it)
    {
        (*it)->Cancel();
    }
}

AsyncOperationSPtr FabricNode::BeginCreateAndOpen(
    AsyncCallback const & callback,
    Common::EventHandler const & nodeFailEventHandler,
    Common::EventHandler const & fmReadyEventHandler)
{
    FabricNodeConfigSPtr config = make_shared<FabricNodeConfig>();
    return BeginCreateAndOpen(
        config,
        FabricNodeGlobalConfig::GetConfig().OpenTimeout,
        callback,
        nodeFailEventHandler,
        fmReadyEventHandler);
}

AsyncOperationSPtr FabricNode::BeginCreateAndOpen(
    Common::FabricNodeConfigSPtr const & config,
    AsyncCallback const & callback,
    Common::EventHandler const & nodeFailEventHandler,
    Common::EventHandler const & fmReadyEventHandler)
{
    return BeginCreateAndOpen(
        config,
        FabricNodeGlobalConfig::GetConfig().OpenTimeout,
        callback,
        nodeFailEventHandler,
        fmReadyEventHandler);
}

AsyncOperationSPtr FabricNode::BeginCreateAndOpen(
    FabricNodeConfigSPtr const & config,
    TimeSpan timeout,
    AsyncCallback const & callback,
    Common::EventHandler const & nodeFailEventHandler,
    Common::EventHandler const & fmReadyEventHandler)
{
    return AsyncOperation::CreateAndStart<CreateAndOpenAsyncOperation>(
        std::move(config),
        nodeFailEventHandler,
        fmReadyEventHandler,
        timeout,
        callback);
}

AsyncOperationSPtr FabricNode::BeginClose(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return BeginClose(FabricNodeGlobalConfig::GetConfig().CloseTimeout, callback, parent);
}

AsyncOperationSPtr FabricNode::BeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncFabricComponent::BeginClose(timeout, callback, parent);
}

ErrorCode FabricNode::EndCreateAndOpen(AsyncOperationSPtr const & operation, __out FabricNodeSPtr & fabricNode)
{
    return CreateAndOpenAsyncOperation::End(operation, fabricNode);
}

FabricNode::~FabricNode()
{
    WriteInfo(TraceLifeCycle, TraceId, "~dtor");
}

AsyncOperationSPtr FabricNode::OnBeginOpen(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    Events.NodeOpening(
        federation_->IdString,
        Instance.InstanceId,
        config_->InstanceName,
        config_->UpgradeDomainId,
        faultDomainString_, 
        addressString_,
        hostNameString_,
        isSeedNode_,
        wformatString(config_->NodeVersion.Version));

    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode FabricNode::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode error = OpenAsyncOperation::End(asyncOperation);
    Events.NodeOpened(federation_->IdString, Instance.InstanceId, error.ReadValue());
    if (error.IsSuccess())
    {
        Events.NodeOpenedSuccessOperational(
            Common::Guid::NewGuid(),
            config_->InstanceName,
            Instance.InstanceId,
            federation_->IdString,
            config_->UpgradeDomainId,
            faultDomainString_,
            config_->IPAddressOrFQDN,
            hostNameString_,
            isSeedNode_,
            wformatString(config_->NodeVersion.Version));
    }
    else
    {
        Events.NodeOpenedFailedOperational(
            Common::Guid::NewGuid(),
            config_->InstanceName,
            Instance.InstanceId,
            federation_->IdString,
            config_->UpgradeDomainId,
            faultDomainString_,
            config_->IPAddressOrFQDN,
            hostNameString_,
            isSeedNode_,
            wformatString(config_->NodeVersion.Version),
            error.ReadValue());
    }
    return error;
}

AsyncOperationSPtr FabricNode::OnBeginClose(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    Events.NodeClosing(federation_->IdString, Instance.InstanceId);
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode FabricNode::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode error = CloseAsyncOperation::End(asyncOperation);
    Events.NodeClosed(federation_->IdString, Instance.InstanceId, error.ReadValue());
    Events.NodeClosedOperational(
        Common::Guid::NewGuid(),
        config_->InstanceName,
        Instance.InstanceId,
        federation_->IdString,
        error.ReadValue());
    return error;
}

void FabricNode::OnAbort()
{
    Events.NodeAborting(federation_->IdString, Instance.InstanceId);

    if (!IsInZombieMode)
    {
        UnRegisterEvents();
    }

    CancelTimers();

    if (!IsInZombieMode)
    {
#if !defined(PLATFORM_UNIX)
        if (nullptr != backupRestoreAgent_)
        {
            backupRestoreAgent_->Abort();
        }
#endif
        
#if !defined(PLATFORM_UNIX)    
        ktlLogger_->Abort();
#endif        

        WriteInfo(TraceLifeCycle, TraceId, "aborting communication");
        communication_->Abort();

        WriteInfo(TraceLifeCycle, TraceId, "aborting management");
        management_->Abort();        

        WriteInfo(TraceLifeCycle, TraceId, "aborting reliability");
        reliability_->Abort();
    }
    
    // abort runtime sharing helper on a different thread, as RAP inside the application host contained
    // in the runtime sharing helper blocks Abort. Abort rest of the component after that on the same thread
    auto root = this->CreateComponentRoot();
    Threadpool::Post(
        [this, root]()
        {
            if (!IsInZombieMode)
            {
                WriteInfo(TraceLifeCycle, TraceId, "aborting runtime sharing helper");
                runtimeSharingHelper_->Abort();

                WriteInfo(TraceLifeCycle, TraceId, "aborting hosting");
                hosting_->Abort();

                WriteInfo(TraceLifeCycle, TraceId, "aborting ktl logger node");
                ktlLogger_->Abort();

                WriteInfo(TraceLifeCycle, TraceId, "aborting ipc server");
                ipcServer_->Abort();
            }
            
            WriteInfo(TraceLifeCycle, TraceId, "aborting federation");
            federation_->Abort();

            Events.NodeAborted(federation_->IdString, Instance.InstanceId);
            Events.NodeAbortedOperational(
                Common::Guid::NewGuid(),
                config_->InstanceName,
                Instance.InstanceId,
                federation_->IdString,
                config_->UpgradeDomainId,
                faultDomainString_,
                config_->IPAddressOrFQDN,
                hostNameString_,
                isSeedNode_,
                wformatString(config_->NodeVersion.Version));
        });
}

HHandler FabricNode::RegisterFailoverManagerReadyEvent(EventHandler handler)
{
    return fmReadyEvent_.Add(handler);
}

bool FabricNode::UnRegisterFailoverManagerReadyEvent(HHandler hHandler)
{
    return fmReadyEvent_.Remove(hHandler);
}

Common::HHandler FabricNode::RegisterNodeFailedEvent(Common::EventHandler handler)
{
    return nodeFailedEvent_.Add(handler);
}

bool FabricNode::UnRegisterNodeFailedEvent(Common::HHandler hHandler)
{
    return nodeFailedEvent_.Remove(hHandler);
}

void FabricNode::RegisterEvents()
{
    auto thisRoot = this->CreateComponentRoot();
    nodeFailedHHandler_ = federation_->RegisterNodeFailedEvent(
        [this, thisRoot] (EventArgs const &)
        { 
            OnNodeFailed();
        });

    fmFailedHHandler_ = reliability_->RegisterFailoverManagerFailedEvent(
        [this, thisRoot] (EventArgs const &)
        {
            OnNodeFailed();
        });

    fmReadyHHandler_ = reliability_->RegisterFailoverManagerReadyEvent(
        [this, thisRoot] (EventArgs const &)
        {
            OnFailoverManagerReady();
        });
}

void FabricNode::UnRegisterEvents()
{
    federation_->UnRegisterNodeFailedEvent(nodeFailedHHandler_);
    reliability_->UnRegisterFailoverManagerFailedEvent(fmFailedHHandler_);
    reliability_->UnRegisterFailoverManagerReadyEvent(fmReadyHHandler_);
    nodeFailedEvent_.Close();
    fmReadyEvent_.Close();
}

void FabricNode::OnNodeFailed()
{
    WriteWarning(TraceLifeCycle, TraceId, "node failed");
    nodeFailedEvent_.Fire(EventArgs(), true);
    Abort();
}

void FabricNode::OnFailoverManagerReady()
{
    fmReadyEvent_.Fire(EventArgs(), true);
    this->StartInitializeNamingService();

    this->StartInitializeClusterManagerService();

    this->StartInitializeRepairManagerService();

    this->StartInitializeImageStoreService();

    this->StartInitializeFaultAnalysisService();
    
    this->StartInitializeSystemServices();

    this->StartInitializeUpgradeOrchestrationService();

    this->StartInitializeCentralSecretService();

    this->StartInitializeBackupRestoreService();

    this->StartInitializeEventStoreService();
}

bool FabricNode::TryStartSystemServiceOperation(AsyncOperationSPtr const & operation)
{
    bool shouldStart = false;

    {
        AcquireExclusiveLock lock(timerLock_);

        if (this->State.Value == FabricComponentState::Opened || this->State.Value == FabricComponentState::Opening)
        {
            shouldStart = true;

            activeSystemServiceOperations_.push_back(operation);
        }
    }
    
    return shouldStart;
}

void FabricNode::TryStartInitializeSystemService(AsyncOperationSPtr const & operation)
{
    bool shouldStart = this->TryStartSystemServiceOperation(operation);

    if (shouldStart)
    {
        dynamic_pointer_cast<InitializeSystemServiceAsyncOperationBase>(operation)->StartOutsideLock(operation);
    }
    else
    {
        operation->Cancel();
    }
}

void FabricNode::StartInitializeNamingService()
{
    ASSERT_IF(
        communication_->NamingService.NamingServiceCuids.size() == 0,
        "NamingService CUIDs have not been created.");

    auto operation = AsyncOperation::CreateAndStart<InitializeNamingServiceAsyncOperation>(
        *this,
        [] (AsyncOperationSPtr const & operation) { InitializeSystemServiceAsyncOperationBase::End(operation); },
        this->CreateAsyncOperationRoot());

    this->TryStartInitializeSystemService(operation);
}

void FabricNode::StartInitializeClusterManagerService()
{
    if (management_->IsCMEnabled)
    {
        auto operation = AsyncOperation::CreateAndStart<InitializeClusterManagerServiceAsyncOperation>(
            *this,
            [] (AsyncOperationSPtr const & operation) { InitializeSystemServiceAsyncOperationBase::End(operation); },
            this->CreateAsyncOperationRoot());

        this->TryStartInitializeSystemService(operation);
    }
}

void FabricNode::StartInitializeRepairManagerService()
{
    if (management_->IsRMEnabled)
    {
        auto operation = AsyncOperation::CreateAndStart<InitializeRepairManagerServiceAsyncOperation>(
            *this,
            [] (AsyncOperationSPtr const & operation) { InitializeSystemServiceAsyncOperationBase::End(operation); },
            this->CreateAsyncOperationRoot());

        this->TryStartInitializeSystemService(operation);
    }
}

void FabricNode::StartInitializeSystemServices()
{
    // Initialization of this service does not use InitializeSystemServiceAsyncOperationBase, because
    // it does not use well-known reserved partition IDs and hence requires a different technique to resolve
    // the partition IDs.  Initialization and retry is handled internally by the async operation started here.
    auto operation = management_->BeginInitializeSystemServices(
        Config()->SystemServiceInitializationTimeout,
        [this](AsyncOperationSPtr const & operation) { management_->EndInitializeSystemServices(operation); },
        this->CreateAsyncOperationRoot());

    if (!this->TryStartSystemServiceOperation(operation))
    {
        // Cancellation of all other active operations may have already occurred,
        // so need to try to cancel the new operation.
        operation->Cancel();
    }
}

void FabricNode::StartInitializeImageStoreService()
{
    if (management_->IsImageStoreServiceEnabled)
    {
        auto operation = AsyncOperation::CreateAndStart<InitializeImageStoreServiceAsyncOperation>(
            *this,
            [] (AsyncOperationSPtr const & operation) { InitializeSystemServiceAsyncOperationBase::End(operation); },
            this->CreateAsyncOperationRoot());

        this->TryStartInitializeSystemService(operation);
    }
}

void FabricNode::StartInitializeFaultAnalysisService()
{
    if (management_->IsFaultAnalysisServiceEnabled)
    {
        WriteInfo(TraceLifeCycle, "Creating FaultAnalysisService");
        
        auto operation = AsyncOperation::CreateAndStart<InitializeFaultAnalysisServiceAsyncOperation>(
             *this,
             [] (AsyncOperationSPtr const & operation) { InitializeSystemServiceAsyncOperationBase::End(operation); },
             this->CreateAsyncOperationRoot());
    
        this->TryStartInitializeSystemService(operation);
     }
}

void FabricNode::StartInitializeUpgradeOrchestrationService()
{
    if (management_->IsUpgradeOrchestrationServiceEnabled)
    {
        WriteInfo(TraceLifeCycle, "Creating UpgradeOrchestrationService");

        auto operation = AsyncOperation::CreateAndStart<InitializeUpgradeOrchestrationServiceAsyncOperation>(
            *this,
            [](AsyncOperationSPtr const & operation) { InitializeSystemServiceAsyncOperationBase::End(operation); },
            this->CreateAsyncOperationRoot());

        this->TryStartInitializeSystemService(operation);
    }
}

void FabricNode::StartInitializeCentralSecretService()
{
    if (management_->IsCentralSecretServiceEnabled)
    {
        WriteInfo(TraceLifeCycle, "Creating CentralSecretService");

        auto operation = AsyncOperation::CreateAndStart<InitializeCentralSecretServiceAsyncOperation>(
            *this,
            [](AsyncOperationSPtr const & operation) { InitializeSystemServiceAsyncOperationBase::End(operation); },
            this->CreateAsyncOperationRoot());

        this->TryStartInitializeSystemService(operation);
    }
}

void FabricNode::StartInitializeBackupRestoreService()
{
    if (management_->IsBackupRestoreServiceEnabled)
    {
        WriteInfo(TraceLifeCycle, "Creating BackupRestoreService");
        
        auto operation = AsyncOperation::CreateAndStart<InitializeBackupRestoreServiceAsyncOperation>(
             *this,
             [] (AsyncOperationSPtr const & operation) { InitializeSystemServiceAsyncOperationBase::End(operation); },
             this->CreateAsyncOperationRoot());
    
        this->TryStartInitializeSystemService(operation);
     }
}

void FabricNode::StartInitializeEventStoreService()
{
    if (management_->IsEventStoreServiceEnabled)
    {
        WriteInfo(TraceLifeCycle, "Creating EventStoreService");

        auto operation = AsyncOperation::CreateAndStart<InitializeEventStoreServiceAsyncOperation>(
            *this,
            [](AsyncOperationSPtr const & operation) { InitializeSystemServiceAsyncOperationBase::End(operation); },
            this->CreateAsyncOperationRoot());

        this->TryStartInitializeSystemService(operation);
    }
}

void FabricNode::CreateDataDirectoryIfNeeded(
    wstring const & directory,
    wstring const & description)
{
    if (directory.size() > 0 && !Directory::Exists(directory))
    {
        WriteInfo(TraceLifeCycle, "Creating service data directory [{0}]: '{1}'", description, directory);

        try
        {
            Directory::Create(directory);
        }
        catch (std::exception const& e)
        {
            if (!Directory::Exists(directory))
            {
                WriteInfo(TraceLifeCycle, "Failed to create data directory '{0}': '{1}'", directory, e.what());
                throw;
            }
        }
    }
    else
    {
        WriteInfo(TraceLifeCycle, "Skipping creation of service data directory [{0}]: '{1}'", description, directory);
    }
}

ErrorCode FabricNode::RemoveDataDirectory(
    wstring const & directory,
    wstring const & description)
{
    ErrorCode error(ErrorCodeValue::Success);

    if (Directory::Exists(directory))
    {
        WriteInfo(TraceLifeCycle, "Removing service data directory [{0}]: '{1}'", directory, description);
        error = Directory::Delete(directory, true);

        if (!error.IsSuccess())
        {
            WriteInfo(TraceLifeCycle, "Failed to remove directory [{0}]: '{1}'", directory, description);
        }
    }

    return error;
}

//TODO: Either make this an instance method or ensure that the working directory is passed in to use
void FabricNode::CreateDataDirectoriesIfNeeded()
{
    if (Reliability::FailoverConfig::GetConfig().TargetReplicaSetSize == 0)
    {
        //This means we are using a share
        CreateDataDirectoryIfNeeded(Reliability::FailoverConfig::GetConfig().FMStoreDirectory, L"FMStoreDirectory");
    }

    CreateDataDirectoryIfNeeded(workingDir_, L"WorkingDir");
}

ErrorCode FabricNode::RemoveDataDirectories()
{
    ErrorCode error(ErrorCodeValue::Success);

    if (Reliability::FailoverConfig::GetConfig().TargetReplicaSetSize == 0)
    {
        //This means we are using a share
        error = RemoveDataDirectory(Reliability::FailoverConfig::GetConfig().FMStoreDirectory, L"FMStoreDirectory");
    }

    error = ErrorCode::FirstError(error, RemoveDataDirectory(workingDir_, L"WorkingDir"));

    return error;
}

bool FabricNode::IsZombieModeFilePresent()
{
    wstring workingDir = config_->WorkingDir;
    wstring startStopFileName = config_->StartStopFileName;
    wstring absolutePathToStartStopFile = Path::Combine(workingDir, startStopFileName);
      
    bool fileExists = File::Exists(absolutePathToStartStopFile);
    bool zombieMode = false;
    if (fileExists)
    {
        zombieMode = true;

    }

    Events.ZombieModeState(
        config_->InstanceName,
        zombieMode);

    return zombieMode;
}

ErrorCode FabricNode::CreateIpcServerTlsSecuritySettings(SecuritySettings & ipcServerTlsSecuritySettings)
{
    auto error = CreateClusterSecuritySettings(ipcServerTlsSecuritySettings);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (ipcServerTlsSecuritySettings.SecurityProvider() == SecurityProvider::Ssl && SecurityConfig::GetConfig().UseClusterCertForIpcServerTlsSecurity)
    {
        //TODO: Add cert update handling in Hosting to refresh certs in container application host
        WriteInfo(TraceSecurity, TraceId, "Cluster certificate is available and UseClusterCertForIpcServerTlsSecurity set to true. Using Cluster cert for IPC Server TLS Security.");
        return error;
    }

    WriteInfo(TraceSecurity, TraceId, "Cluster certificate is not available or UseClusterCertForIpcServerTlsSecurity is set to false. Using Self generated cert for IPC Server TLS Security.");
    error = SecuritySettings::CreateSelfGeneratedCertSslServer(ipcServerTlsSecuritySettings);

    if (!error.IsSuccess())
    {
        WriteError(
            TraceSecurity,
            TraceId,
            "ipcServer->SecuritySettings.CreateSelfGeneratedCertSslServer error={0}",
            error);
        return error;
    }

    return error;
}
