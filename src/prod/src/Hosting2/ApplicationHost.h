// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // NodeHost Closed
    typedef Common::EventT<NodeHostProcessClosedEventArgs> NodeHostProcessClosedEvent;
    typedef NodeHostProcessClosedEvent::EventHandler NodeHostProcessClosedEventHandler;
    typedef NodeHostProcessClosedEvent::HHandler NodeHostProcessClosedEventHHandler;

    class ApplicationHost :
        public Common::ComponentRoot,
        public Common::AsyncFabricComponent, 
        public IApplicationHost,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ApplicationHost)

    public:
        virtual ~ApplicationHost();
              
        __declspec(property(get=get_ApplicationHostContext)) ApplicationHostContext const & HostContext;
        inline ApplicationHostContext const & get_ApplicationHostContext() const { return hostContext_; }

        __declspec(property(get=get_Id)) std::wstring const & Id;
        inline std::wstring const & get_Id() const { return hostContext_.HostId; };

        __declspec(property(get=get_Type)) ApplicationHostType::Enum Type;
        inline ApplicationHostType::Enum get_Type() const { return hostContext_.HostType; };

        __declspec(property(get=get_Client)) Transport::IpcClient & Client;
        inline Transport::IpcClient & get_Client() const { return *ipcClient_.get(); };

        __declspec(property(get=get_ReconfigurationAgentProxy)) Reliability::ReconfigurationAgentComponent::IReconfigurationAgentProxy & ReconfigurationAgentProxyObj;
        inline Reliability::ReconfigurationAgentComponent::IReconfigurationAgentProxy & get_ReconfigurationAgentProxy() const { return *raProxy_; };

#if !defined (PLATFORM_UNIX)
        __declspec(property(get = get_BackupRestoreAgentProxy)) Management::BackupRestoreAgentComponent::IBackupRestoreAgentProxy & BackupRestoreAgentProxyObj;
        inline Management::BackupRestoreAgentComponent::IBackupRestoreAgentProxy & get_BackupRestoreAgentProxy() const { return *baProxy_; };
#endif

        __declspec(property(get = get_ThrottledIpcHealthReportingClient)) Client::IpcHealthReportingClient & ThrottledIpcHealthReportingClientObj;
        inline Client::IpcHealthReportingClient & get_ThrottledIpcHealthReportingClient() const { return *throttledIpcHealthReportingClient_; };

        __declspec(property(get = get_IpcHealthReportingClient)) Client::IpcHealthReportingClient & IpcHealthReportingClientObj;
        inline Client::IpcHealthReportingClient & get_IpcHealthReportingClient() const { return *ipcHealthReportingClient_; };

        __declspec(property(get=get_ComReplicatorFactoryObj)) Reliability::ReplicationComponent::IReplicatorFactory & ComReplicatorFactoryObj;
        inline Reliability::ReplicationComponent::IReplicatorFactory & get_ComReplicatorFactoryObj() const { return *comReplicatorFactory_; }

        __declspec(property(get=get_ComTransactionalReplicatorFactoryObj)) TxnReplicator::ITransactionalReplicatorFactory & ComTransactionalReplicatorFactoryObj;
        inline TxnReplicator::ITransactionalReplicatorFactory & get_ComTransactionalReplicatorFactoryObj() const { return *comTransactionalReplicatorFactory_; }

        void UnregisterRuntimeAsync(std::wstring const & runtimeId);

        // Gets a registered FabricRuntime.
        Common::ErrorCode GetFabricRuntime(std::wstring const & runtimeId, __out FabricRuntimeImplSPtr & runtime);

        Common::ErrorCode GetFabricNodeContext(FabricNodeContextSPtr & nodeContext);

        bool IsLeaseExpired();

        NodeHostProcessClosedEventHHandler RegisterNodeHostProcessClosedEventHandler(NodeHostProcessClosedEventHandler const & handler);
        bool UnregisterNodeHostProcessClosedEventHandler(NodeHostProcessClosedEventHHandler const & hHandler);

        // Creates a FabricRuntime and registers it with FabricNode. The returned ComFabricRuntime object controls 
        // the lifetime of the FabricRuntime. If ComFabricRuntime object is released, it unregisters the associated 
        // FabricRuntime from FabricNode and removes its entry from the ApplicationHost's runtime table.
        Common::AsyncOperationSPtr BeginCreateComFabricRuntime(
            FabricRuntimeContextUPtr && fabricRuntimeContextUPtr,
            Common::ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::AsyncOperationSPtr BeginCreateComFabricRuntime(
            Common::ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::AsyncOperationSPtr BeginCreateComFabricRuntime(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::AsyncOperationSPtr BeginCreateComFabricRuntime(
            FabricRuntimeContextUPtr && fabricRuntimeContextUPtr,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndCreateComFabricRuntime(
            Common::AsyncOperationSPtr const & operation,
            __out Common::ComPointer<ComFabricRuntime> & comFabricRuntime);

#if !defined (PLATFORM_UNIX)
        Common::ErrorCode CreateBackupRestoreAgent(__out Common::ComPointer<ComFabricBackupRestoreAgent> & comBackupRestoreAgent);
#endif
        Common::ErrorCode GetCodePackageActivationContext(
            CodePackageContext const & codeContext,
            __out CodePackageActivationContextSPtr & codePackageActivationContext);

        Common::ErrorCode GetCodePackageActivator(
            _Out_ ApplicationHostCodePackageActivatorSPtr & codePackageActivator);

        Common::ErrorCode GetKtlSystem(
            __out KtlSystem ** ktlSystem) override;

        Common::AsyncOperationSPtr BeginGetFactoryAndCreateInstance(
            std::wstring const & runtimeId,
            ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & serviceName,
            std::vector<byte> const & initializationData,
            Common::Guid const & partitionId,
            int64 instanceId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndGetFactoryAndCreateInstance(
            Common::AsyncOperationSPtr const & operation,
            __out Common::ComPointer<IFabricStatelessServiceInstance> & statelessService) override;

        Common::AsyncOperationSPtr BeginGetFactoryAndCreateReplica(
            std::wstring const & runtimeId,
            ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & serviceName,
            std::vector<byte> const & initializationData,
            Common::Guid const & partitionId,
            int64 replicaId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndGetFactoryAndCreateReplica(
            Common::AsyncOperationSPtr const & operation,
            __out  Common::ComPointer<IFabricStatefulServiceReplica> & statefulService) override;

        Common::ErrorCode GetHostInformation(
            std::wstring & nodeIdOut,
            uint64 & nodeInstanceIdOut,
            std::wstring & hostIdOut,
            std::wstring & nodeNameOut) override;

        Common::ErrorCode GetCodePackageActivationContext(
            __in std::wstring const & runtimeId,
            __out Common::ComPointer<IFabricCodePackageActivationContext> & codePackageActivationContext) override;

        Common::ErrorCode GetCodePackageActivator(
            _Out_ Common::ComPointer<IFabricCodePackageActivator> & codePackageActivator) override;

    protected:
         ApplicationHost(
            ApplicationHostContext const & hostContext,
            std::wstring const & runtimeServiceAddress,
            Common::PCCertContext certContext,
            wstring const & serverThumbprint,
            bool useSystemServiceSharedLogSettings,
            KtlSystem *);

        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const & asyncOperation);

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const & asyncOperation);

        virtual void OnAbort();

        virtual void ProcessIpcMessage(Transport::Message & message, Transport::IpcReceiverContextUPtr & context);

        virtual Common::ErrorCode OnCreateAndAddFabricRuntime(
            FabricRuntimeContextUPtr const & fabricRuntimeContextUPtr,
            Common::ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
            __out FabricRuntimeImplSPtr & fabricRuntime) = 0;

        virtual Common::ErrorCode OnGetCodePackageActivationContext(
            CodePackageContext const & codeContext,
            __out CodePackageActivationContextSPtr & codePackageActivationContext) = 0;

        virtual Common::ErrorCode OnGetCodePackageActivator(
            _Out_ ApplicationHostCodePackageActivatorSPtr & codePackageActivator);

        virtual Common::ErrorCode OnUpdateCodePackageContext(
            CodePackageContext const & codeContext) = 0;

        virtual Common::ErrorCode OnCodePackageEvent(
            CodePackageEventDescription const & eventDescription);

        Common::ErrorCode AddFabricRuntime(FabricRuntimeImplSPtr const & runtime);

        Common::ErrorCode AddCodePackage(
            CodePackageActivationId const & activationId, 
            CodePackageActivationContextSPtr const & activationContext,
            bool validate = false);

        Common::ErrorCode FindCodePackage(
            CodePackageInstanceIdentifier const & codePackageInstanceId, 
            __out CodePackageActivationId & activationId, 
            __out CodePackageActivationContextSPtr & activationContext,
            __out bool & isValid);

        Common::ErrorCode ValidateCodePackage(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            CodePackageActivationId const & activationId);

        Common::ErrorCode RemoveCodePackage(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            CodePackageActivationId const & activationId,
            __out bool & isValid);

        Client::IpcHealthReportingClientSPtr throttledIpcHealthReportingClient_;
        Client::IpcHealthReportingClientSPtr ipcHealthReportingClient_;

    private:
        Common::AsyncOperationSPtr BeginRegisterFabricRuntime(
            FabricRuntimeImplSPtr const & runtime,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRegisterFabricRuntime(
            Common::AsyncOperationSPtr const & operation);

        void RegisterIpcMessageHandler();
        void UnregisterIpcMessageHandler();
        Common::ErrorCode InitializeNodeHostMonitoring(DWORD nodeHostProcessId);
        Common::ErrorCode InitializeLeaseMonitoring(HANDLE leaseHandle, int64 initialExpirationInTicks, Common::TimeSpan const leaseDuration);
        void IpcMessageHandler(Transport::Message & message, Transport::IpcReceiverContextUPtr & context);
        void ProcessInvalidateLeaseRequest(Transport::Message &, Transport::IpcReceiverContextUPtr & context);
        void ProcessUpdateCodePackageContextRequest(Transport::Message &, Transport::IpcReceiverContextUPtr & context);
        void ProcessCodePackageEventRequest(Transport::Message &, Transport::IpcReceiverContextUPtr & context);
        void OnNodeHostProcessDown(Common::ErrorCode const & waitResult);
        void OnUnregisterRuntimeAsyncCompleted(Common::AsyncOperationSPtr const & operation);
        void FinishUnregisterRuntimeAsync(Common::AsyncOperationSPtr const & operation);
        void RaiseNodeHostProcessClosedEvent();
        Common::ErrorCode ShutdownKtlSystem(Common::TimeSpan const & timeout);

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class CreateComFabricRuntimeAsyncOperation;
        class RegisterFabricRuntimeAsyncOperation;
        class UnregisterFabricRuntimeAsyncOperation;
        class RuntimeTable;
        class CodePackageTable;
        class GetFactoryAndCreateInstanceAsyncOperation;
        class GetFactoryAndCreateReplicaAsyncOperation;
        class KtlSystemWrapper;

    private:
        FabricNodeContextSPtr nodeContext_;
        Common::RwLock nodeContextLock_;
        std::wstring runtimeServiceAddress_;
        ApplicationHostContext hostContext_;
        std::unique_ptr<Transport::IpcClient> ipcClient_;
        Common::atomic_bool ipcUseSsl_;
        Common::CertContextUPtr certContext_;
        std::wstring serverThumbprint_;
        Reliability::ReplicationComponent::IReplicatorFactoryUPtr comReplicatorFactory_;
        TxnReplicator::ITransactionalReplicatorFactoryUPtr comTransactionalReplicatorFactory_;
        Reliability::ReconfigurationAgentComponent::IReconfigurationAgentProxyUPtr  raProxy_;
#if !defined (PLATFORM_UNIX)
        Management::BackupRestoreAgentComponent::IBackupRestoreAgentProxyUPtr baProxy_;
#endif
        std::unique_ptr<RuntimeTable> runtimeTable_;
        Common::ProcessWaitSPtr nodeHostProcessWait_;
        LeaseMonitorUPtr leaseMonitor_;
        Common::atomic_bool nodeHostProcessClosed_;
        NodeHostProcessClosedEvent nodeHostProcessClosedEvent_;
        std::unique_ptr<CodePackageTable> codePackageTable_;

        std::unique_ptr<KtlSystemWrapper> ktlSystemWrapper_;

        Data::Log::LogManager::SPtr logManager_;
        bool useSystemServiceSharedLogSettings_;
        KtlLogger::SharedLogSettingsSPtr applicationSharedLogSettings_;
        KtlLogger::SharedLogSettingsSPtr systemServicesSharedLogSettings_;
    };
}
