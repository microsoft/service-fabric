// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        typedef std::function<void(Common::Guid const &)> CloseReplicaCallback;

        class FileStoreServiceReplica :
            public Store::KeyValueStoreReplica,
            protected Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
            using Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>::WriteNoise;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>::WriteInfo;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>::WriteWarning;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>::WriteError;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>::WriteTrace;

            DENY_COPY(FileStoreServiceReplica);

        public:
            struct NodeInfo
            {
                NodeInfo(
                    std::wstring const & nodeName, 
                    Federation::NodeInstance const & nodeInstance, 
                    std::wstring const & workingDir, 
                    std::wstring const ipAddressOrFQDN)
                {
                    NodeName = nodeName;
                    NodeInstance = nodeInstance;
                    WorkingDirectory = workingDir;
                    IPAddressOrFQDN = ipAddressOrFQDN;
                }

                std::wstring NodeName;
                Federation::NodeInstance NodeInstance;
                std::wstring WorkingDirectory;
                std::wstring IPAddressOrFQDN;
            };

        public:
            FileStoreServiceReplica(
                Common::Guid const & partitionId,
                FABRIC_REPLICA_ID replicaId,
                std::wstring const & serviceName,
                __in SystemServices::ServiceRoutingAgentProxy & routingAgentProxy,
                Api::IClientFactoryPtr const & clientFactory,
                NodeInfo const & nodeInfo,
                FileStoreServiceFactoryHolder const & factoryHolder);

            virtual ~FileStoreServiceReplica();

            __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
            Federation::NodeInstance const & get_NodeInstance() const { return nodeInstance_; }

            __declspec(property(get=get_StoreClient)) Api::IInternalFileStoreServiceClientPtr const & StoreClient;
            Api::IInternalFileStoreServiceClientPtr const & get_StoreClient() const { return fileStoreClient_; }

            __declspec(property(get=get_PropertyManagementClient)) Api::IPropertyManagementClientPtr const & PropertyManagementClient;
            Api::IPropertyManagementClientPtr const & get_PropertyManagementClient() const { return propertyManagmentClient_; }

            __declspec(property(get=get_ImageStoreRoot)) std::wstring const & StoreRoot;
            std::wstring const & get_ImageStoreRoot() const { return storeRoot_; }

            __declspec(property(get=get_ShareRoot)) std::wstring const & ShareRoot;
            std::wstring const & get_ShareRoot() const { return storeShareRoot_; }

            __declspec(property(get=get_StagingRoot)) std::wstring const & StagingRoot;
            std::wstring const & get_StagingRoot() const { return stagingRoot_; }

            __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
            std::wstring const & get_ServiceName() const { return serviceName_; }

            __declspec(property(get=get_SmbContext)) ImpersonatedSMBCopyContextSPtr const & SmbContext;
            ImpersonatedSMBCopyContextSPtr const & get_SmbContext() const { return smbCopyContext_; }

            __declspec(property(get=get_FileStoreServiceFactory)) FileStoreServiceFactory & FileStoreServiceFactoryObj;
            FileStoreServiceFactory & get_FileStoreServiceFactory() const { return factoryHolder_.RootedObject; }

            __declspec (property(get = get_FileStoreServiceCounters)) FileStoreServiceCountersSPtr const & FileStoreServiceCounters;
            FileStoreServiceCountersSPtr const & get_FileStoreServiceCounters() const { return fileStoreServiceCounters_; }

            bool Test_TryGetStoreShareRoot(__out std::wstring & storeShareRoot);
            Common::ErrorCode Test_GetReplicaData(
                __out std::vector<FileMetadata> & metadataList, 
                __out uint64 & lastDeletedVersionNumber);

            CloseReplicaCallback OnCloseReplicaCallback;            

        protected:
            // *******************
            // StatefulServiceBase
            // *******************
            virtual Common::ErrorCode OnOpen(Common::ComPointer<IFabricStatefulServicePartition> const & servicePartition);
            virtual Common::ErrorCode OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out std::wstring & serviceLocation);
            virtual Common::ErrorCode OnClose();
            virtual void OnAbort();

        protected:
            //
            // IStoreEventHandler
            //
            virtual Common::AsyncOperationSPtr BeginOnDataLoss(
                __in Common::AsyncCallback const &,
                __in Common::AsyncOperationSPtr const &) override;

            virtual Common::ErrorCode EndOnDataLoss(
                __in Common::AsyncOperationSPtr const &,
                __out bool & isStateChanged) override;

        protected:
            //
            // ISecondaryEventHandler
            //
            virtual Common::ErrorCode OnCopyComplete(Api::IStoreEnumeratorPtr const &) override;
            virtual Common::ErrorCode OnReplicationOperation(Api::IStoreNotificationEnumeratorPtr const &) override;

        private:
            // ****************
            // Message Handling
            // ****************
            void RegisterMessageHandler();
            void UnregisterMessageHandler();

            void RequestMessageHandler(Transport::MessageUPtr &&, Transport::IpcReceiverContextUPtr &&);
            void OnProcessRequestComplete(Common::ActivityId const &, Transport::MessageId const &, Common::AsyncOperationSPtr const &);

            // ****************
            // Helper functions
            // ****************
            Common::ErrorCode CreateName();
            Common::ErrorCode GetServiceLocationSequenceNumber(__out int64 & sequenceNumber);
            Common::ErrorCode RegisterServiceLocation(int64 const sequenceNumber);
            void DeleteStagingAndStoreFolder();

            std::wstring const serviceName_;
            std::wstring const storeRoot_;
            std::wstring const stagingRoot_;
            std::wstring const storeShareRoot_;
            std::wstring const stagingShareRoot_;
#if defined(PLATFORM_UNIX)
            std::wstring const ipAddressOrFQDN;
#endif
            Federation::NodeInstance const nodeInstance_;

            ImpersonatedSMBCopyContextSPtr smbCopyContext_;
            RequestManagerSPtr requestManager_;
            ReplicationManagerSPtr replicationManager_;            

            std::vector<BYTE> serializedInfo_;
            SystemServices::SystemServiceLocation serviceLocation_;
            Api::IInternalFileStoreServiceClientPtr fileStoreClient_;
            Api::IPropertyManagementClientPtr propertyManagmentClient_;
            Api::IClientFactoryPtr clientFactory_;
            Common::RwLock lock_;

            // The reference of the RoutingAgentProxy is held by FileStoreServiceFactory
            SystemServices::ServiceRoutingAgentProxy & routingAgentProxy_;
            FileStoreServiceFactoryHolder factoryHolder_;

            // Performance counters written by this replica
            FileStoreServiceCountersSPtr fileStoreServiceCounters_;

#if !defined(PLATFORM_UNIX)
            void LogFreeSpaceForDisks();
            void CleanupFreeSpaceLoggingTimer();
            Common::ExclusiveLock freeSpaceLoggingtimerLock_;
            Common::TimerSPtr freeSpaceLoggingTimer_;
#endif
        };
    }
}
