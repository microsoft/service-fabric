// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreService
    {
        class BackupRestoreServiceAgent 
            : public Api::IBackupRestoreServiceAgent
            , public Common::ComponentRoot
            , private Federation::NodeTraceComponent<Common::TraceTaskCodes::BackupRestoreService>
        {
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::BackupRestoreService>::WriteNoise;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::BackupRestoreService>::WriteInfo;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::BackupRestoreService>::WriteWarning;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::BackupRestoreService>::WriteError;

        public:
            virtual ~BackupRestoreServiceAgent();

            static Common::ErrorCode Create(__out std::shared_ptr<BackupRestoreServiceAgent> & result);

            /*__declspec(property(get=get_TraceId)) std::wstring const & TraceId;
            std::wstring const & get_TraceId() const { return NodeTraceComponent::TraceId; }*/
            
            virtual void Release();
            
            virtual void RegisterBackupRestoreService(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID,
                Api::IBackupRestoreServicePtr const &,  
                __out std::wstring & serviceAddress);

            virtual void UnregisterBackupRestoreService(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID);

            virtual Common::AsyncOperationSPtr BeginUpdateBackupSchedulePolicy(
                ::FABRIC_BACKUP_PARTITION_INFO * info,
                ::FABRIC_BACKUP_POLICY * policy,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);

            virtual Common::ErrorCode EndUpdateBackupSchedulePolicy(
                Common::AsyncOperationSPtr const &operation);

            virtual Common::AsyncOperationSPtr BeginPartitionBackupOperation(
                ::FABRIC_BACKUP_PARTITION_INFO * info,
                ::FABRIC_BACKUP_OPERATION_ID operationId,
                ::FABRIC_BACKUP_CONFIGURATION* backupConfiguration,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);

            virtual Common::ErrorCode EndPartitionBackupOperation(
                Common::AsyncOperationSPtr const &operation);

        private:
            class RequestReplyAsyncOperationBase;
            class UpdateBackupSchedulePolicyAsyncOperation;
            class PartitionBackupAsyncOperation;

            class DispatchMessageAsyncOperationBase;
            class GetBackupSchedulingPolicyAsyncOperation;
            class GetRestorePointAsyncOperation;
            class ReportBackupCompletionOperation; 
            class ReportRestoreCompletionOperation;
            
            BackupRestoreServiceAgent(Federation::NodeInstance const &, __in Transport::IpcClient &, Common::ComPointer<IFabricRuntime> const &);
            
            Common::ErrorCode Initialize();
            
            void ProcessTransportIpcRequest(Api::IBackupRestoreServicePtr const & servicePtr, Transport::MessageUPtr & message, Transport::IpcReceiverContextUPtr && receiverContext);
            void OnProcessTransportIpcRequestComplete(Common::AsyncOperationSPtr const &, ActivityId const&, bool expectedCompletedSynchronously);

            void SendIpcReply(Transport::MessageUPtr&& reply, Transport::IpcReceiverContext const & receiverContext);
            void OnIpcFailure(Common::ErrorCode const& error, Transport::IpcReceiverContext const & receiverContext, Common::ActivityId const&);

            Common::ComPointer<IFabricRuntime> runtimeCPtr_;

            Api::IBackupRestoreServicePtr servicePtr_;
            Transport::IpcClient & ipcClient_;
        };
    }
}
