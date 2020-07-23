// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class RequestManager :
            public Common::ComponentRoot,
            public Common::StateMachine,
            private Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
            DENY_COPY(RequestManager);

            using Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>::WriteNoise;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>::WriteInfo;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>::WriteWarning;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>::WriteError;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>::WriteTrace;

            STATEMACHINE_STATES_10(
                Created, 
                RecoveryScheduling, 
                RecoveryScheduled, 
                RecoveryStarting, 
                RecoveryStarted,
                RecoveryCompleting,
                Active, 
                Deactivating, 
                Deactivated, 
                Failed);
            STATEMACHINE_TRANSITION(RecoveryScheduling, Created);
            STATEMACHINE_TRANSITION(RecoveryScheduled, RecoveryScheduling|RecoveryStarting);
            STATEMACHINE_TRANSITION(RecoveryStarting, RecoveryScheduled);
            STATEMACHINE_TRANSITION(RecoveryStarted, RecoveryStarting);
            STATEMACHINE_TRANSITION(RecoveryCompleting, RecoveryStarted);
            STATEMACHINE_TRANSITION(Active, RecoveryCompleting);
            STATEMACHINE_TRANSITION(Deactivating, Active);
            STATEMACHINE_TRANSITION(Deactivated, Deactivating);
            STATEMACHINE_TRANSITION(Failed, RecoveryCompleting|RecoveryStarted);
            STATEMACHINE_ABORTED_TRANSITION(Created|RecoveryScheduled|RecoveryStarted|Active|Deactivated|Failed);
            STATEMACHINE_TERMINAL_STATES(Aborted|Deactivated);

        public:
            RequestManager(
                ImageStoreServiceReplicaHolder const & serviceReplicaHolder, 
                std::wstring const & stagingShareLocation, 
                std::wstring const & storeShareLocation,
                int64 const epochDataLossNumber,
                int64 const epochConfigurationNumber,
                SystemServices::ServiceRoutingAgentProxy & routingAgentProxy);
            ~RequestManager();

            Common::ErrorCode Open();
            Common::ErrorCode Close();

            void ProcessRequest(Transport::MessageUPtr &&, Transport::IpcReceiverContextUPtr &&);
            StoreFileVersion GetNextFileVersion();

            __declspec(property(get=get_LocalStagingLocation)) std::wstring const & LocalStagingLocation;
            inline std::wstring const & get_LocalStagingLocation() const { return localStagingLocation_; }

            __declspec(property(get=get_StoreShareLocation)) std::wstring const & StoreShareLocation;
            inline std::wstring const & get_StoreShareLocation() const { return storeShareLocation_; }

            __declspec(property(get=get_ReplicaObj)) FileStoreServiceReplica const & ReplicaObj;
            inline FileStoreServiceReplica const & get_ReplicaObj() const { return serviceReplicaHolder_.RootedObject; }

            __declspec(property(get = get_ReplicatedStoreWrapper)) unique_ptr<ReplicatedStoreWrapper> const & ReplicaStoreWrapperUPtr;
            inline unique_ptr<ReplicatedStoreWrapper> const & get_ReplicatedStoreWrapper() const { return replicatedStoreWrapper_; }

            __declspec(property(get = get_UploadSessionMap)) std::unique_ptr<Management::FileStoreService::UploadSessionMap> const & UploadSessionMap;
            inline std::unique_ptr<Management::FileStoreService::UploadSessionMap> const & get_UploadSessionMap() const { return uploadSessionMap_; }

            Common::AsyncOperationSPtr BeginOnDataLoss(
                __in Common::AsyncCallback const &,
                __in Common::AsyncOperationSPtr const &);

            Common::ErrorCode EndOnDataLoss(
                __in Common::AsyncOperationSPtr const &,
                __out bool & isStateChanged);

            bool IsDataLossExpected();

        protected:
            virtual void OnAbort();

        private:

            class RecoverAsyncOperation;
            friend class ProcessRequestAsyncOperation;
            friend class StoreTransactionAsyncOperation;
            friend class FileUploadAsyncOperation;
            friend class FileCopyAsyncOperation;
            friend class ReplicatedStoreWrapper;

            void OnProcessRequest(Transport::MessageUPtr &&, Transport::IpcReceiverContextUPtr &&);
            void OnRecoveryCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnProcessRequestComplete(Common::AsyncOperationSPtr const & operation);
            void OnProcessChunkRequestComplete(
                Common::AsyncOperationSPtr const &,
                Common::Guid const &,
                std::wstring const &,
                int64);

            bool ValidateClientMessage(__in Transport::MessageUPtr & message, __out std::wstring & rejectReason);
            bool IsChunkAction(std::wstring const& action);

            std::wstring localStagingLocation_;
            std::wstring stagingShareLocation_;
            std::wstring const storeShareLocation_;
            int64 const epochDataLossNumber_;
            int64 const epochConfigurationNumber_;
            Common::atomic_uint64 versionNumber_;
            
            PendingWriteOperations pendingWriteOperations_;
            std::shared_ptr<RecoverAsyncOperation> recoveryOperation_;
            Common::RwLock recoveryOperationLock_;
            ImageStoreServiceReplicaHolder serviceReplicaHolder_;

            std::unique_ptr<Common::DefaultJobQueue<RequestManager>> requestProcessingJobQueue_;
            std::unique_ptr<Common::DefaultTimedJobQueue<RequestManager>> fileOperationProcessingJobQueue_;
            std::unique_ptr<Common::DefaultTimedJobQueue<RequestManager>> storeTransactionProcessingJobQueue_;

            std::unique_ptr<ReplicatedStoreWrapper> replicatedStoreWrapper_;
            std::unique_ptr<Management::FileStoreService::UploadSessionMap> uploadSessionMap_;

            // This is kept alive as we are holding to serviceReplicaHolder_
            SystemServices::ServiceRoutingAgentProxy & routingAgentProxy_;

            // For creating predictable chaos for test
            Common::atomic_uint64 uploadChunkCount_;
            Common::atomic_uint64 commitUploadChunkCount_;
            Common::atomic_uint64 createUploadChunkCount_;
        };
    }
}
