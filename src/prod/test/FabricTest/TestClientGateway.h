// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Naming/MockServiceTableEntryCache.h"

namespace FabricTest
{
    class TestClientGateway : public ComponentRoot
    {
    public:
        TestClientGateway(Federation::NodeId const &, int replicaCount, int sgMemberCount);

        Common::ErrorCode Open(__out std::wstring & address);

        void Close();

        void SetMockServiceTemplate(
            Naming::PartitionedServiceDescriptor const & psd);

        void SetNotificationBurst(int burstSize);

        Common::ErrorCode GetCurrentMockRsp(
            Common::NamingUri const & name,
            Naming::ServiceResolutionRequestData const & requestData,
            __out Naming::ResolvedServicePartitionSPtr & rsp);

        void GenerateMockServicePsds(
            Common::NamingUri const & prefix, 
            int serviceCount,
            int subnameCount);

        Common::ErrorCode GenerateCacheUpdates(
            Common::NamingUri const & prefix,
            int serviceCount,
            int subnameCount,
            int batchCount);

        Common::ErrorCode GenerateCacheDeletes(
            Common::NamingUri const & prefix,
            int serviceCount,
            int subnameCount,
            int batchCount);

        Common::NamingUri CreateServiceNameFromPrefix(Common::NamingUri const & prefix, int index);

    private: 
        class ProcessIpcRequestAsyncOperation;
        class ProcessClientRequestAsyncOperation;

        Common::ErrorCode GenerateCacheEntries(
            Common::NamingUri const & prefix,
            bool includeEndpoints,
            int serviceCount,
            int subnameCount,
            int batchCount);

        typedef std::function<Common::ErrorCode(
            TestClientGateway &, 
            Transport::ISendTarget::SPtr const &, 
            Transport::MessageUPtr & request, 
            Transport::MessageUPtr & reply)> MessageHandler;

        void ProcessClientMessage(
            Transport::MessageUPtr & request,
            Transport::ISendTarget::SPtr const & sender);

        void OnProcessClientMessageComplete(
            Transport::ISendTarget::SPtr const & sender,
            Common::AsyncOperationSPtr const &operation);

        static Common::ErrorCode ProcessPing(
            TestClientGateway &,
            Transport::ISendTarget::SPtr const &, 
            Transport::MessageUPtr & request,
            __out Transport::MessageUPtr & reply);

        static Common::ErrorCode ProcessGetServiceDescription(
            TestClientGateway &,
            Transport::ISendTarget::SPtr const &, 
            Transport::MessageUPtr & request,
            __out Transport::MessageUPtr & reply);

        static Common::ErrorCode ProcessResolveService(
            TestClientGateway &,
            Transport::ISendTarget::SPtr const &, 
            Transport::MessageUPtr & request,
            __out Transport::MessageUPtr & reply);

        static Common::ErrorCode ProcessNotification(
            TestClientGateway & gateway,
            Transport::ISendTarget::SPtr const &, 
            Transport::MessageUPtr & request,
            __out Transport::MessageUPtr & reply);
    
        Common::ErrorCode GetCurrentMockStes(
            Common::NamingUri const & name,
            bool includeEndpoints,
            __out std::vector<Reliability::ServiceTableEntrySPtr> & result);

        void AddHandler(std::wstring const &, MessageHandler const &);

        Federation::NodeInstance nodeInstance_;
        int replicaCount_;
        int sgMemberCount_;

        Naming::EntreeServiceTransportSPtr internalTransport_;
        Naming::MockServiceTableEntryCache mockCache_;
        Naming::ServiceNotificationManagerUPtr serviceNotificationManager_;
        std::unique_ptr<Naming::ServiceNotificationManagerProxy> serviceNotificationManagerProxy_;
        Transport::IDatagramTransportSPtr publicTransport_;
        Transport::DemuxerUPtr demuxer_;
        Transport::RequestReplySPtr requestReply_;
        std::unique_ptr<Naming::ProxyToEntreeServiceIpcChannel> proxyIpcClient_;

        std::map<std::wstring, MessageHandler> messageHandlers_;

        std::shared_ptr<Naming::PartitionedServiceDescriptor> mockPsdTemplate_;
        std::vector<std::shared_ptr<Naming::ResolvedServicePartition>> mockRspTemplates_;
        std::unordered_map<Common::NamingUri, std::shared_ptr<Naming::PartitionedServiceDescriptor>, NamingUri::Hasher> mockPsdsByServiceName_;
        int notificationBurst_;

        Common::atomic_long currentVersion_;
    };
}
