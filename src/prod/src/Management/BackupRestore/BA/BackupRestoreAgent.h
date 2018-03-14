// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    { 
        class BackupRestoreAgent :
            public Common::RootedObject,
            public Common::TextTraceComponent<Common::TraceTaskCodes::BA>
        {
            DENY_COPY(BackupRestoreAgent);

        public:

            BackupRestoreAgent(
                Common::ComponentRoot const & root,
                Hosting2::IHostingSubsystemSPtr hosting,
                FederationWrapperUPtr && federationWrapper,
                Reliability::ServiceAdminClient & serviceAdminClient,
                Api::IQueryClientPtr queryClientPtr,
                Transport::IpcServer& ipcServer,
                IBaToBapTransportUPtr && bapTransport,
                std::wstring const & workingDirectory);

            ~BackupRestoreAgent();

            static BackupRestoreAgentUPtr Create(
                Common::ComponentRoot const & root,
                Hosting2::IHostingSubsystemSPtr hosting,
                Federation::FederationSubsystem& federationSubsystem, 
                Reliability::ServiceResolver & serviceResolver,
                Reliability::ServiceAdminClient & adminClient,
                Api::IQueryClientPtr queryClientPtr,
                Transport::IpcServer & ipcServer,
                std::wstring const & workingDirectory);

#pragma region BA LifeCycle
            Common::ErrorCode Open();
            void Abort();
            void Close();
#pragma endregion

            void RegisterFederationSubsystemEvents();
            void ProcessTransportRequest(Transport::MessageUPtr & message, Federation::OneWayReceiverContextUPtr && oneWayReceiverContext);
            void ProcessTransportRequestResponseRequest(Transport::MessageUPtr & message, Federation::RequestReceiverContextUPtr && requestReceiverContext);


            void RegisterIpcMessageHandler();
            void ProcessTransportIpcRequest(Transport::MessageUPtr & message, Transport::IpcReceiverContextUPtr && receiverContext);

        private:
            // Start: Remove below code before merge to develop.
            void BeginUploadDummyBackup(ActivityId const& activityId);
            void OnUploadDummyBackupCompleted(AsyncOperationSPtr const & uploadDummyBackupOperation, bool expectedCompletedSynchronously);
            // End: Remove code before merge to develop.
            void OnProcessTransportIpcRequestCompleted(Common::AsyncOperationSPtr const & operation, bool completedLocally, Common::ActivityId const&, bool expectedCompletedSynchronously);
            void OnProcessTransportRequestResponseRequestCompleted(Common::AsyncOperationSPtr const & operation, Common::ActivityId const&, bool expectedCompletedSynchronously);

            FederationWrapperUPtr federationWrapper_;
            IBaToBapTransportUPtr bapTransport_;
            std::wstring nodeInstanceIdStr_;
            std::wstring workingDir_;
            Hosting2::IHostingSubsystemSPtr hosting_;
            Transport::IpcServer & ipcServer_;
            Reliability::ServiceAdminClient & serviceAdminClient_;
            Api::IQueryClientPtr queryClientPtr_;
            BackupCopierProxySPtr backupCopierProxy_;

            __declspec (property(get = get_BAPTransport)) IBaToBapTransport & BAPTransport;
            IBaToBapTransport & get_BAPTransport() { return *bapTransport_; }

            __declspec (property(get = get_NodeInstanceIdStr)) std::wstring const& NodeInstanceIdStr;
            std::wstring const & get_NodeInstanceIdStr() const { return nodeInstanceIdStr_; }
        };
    }
}
