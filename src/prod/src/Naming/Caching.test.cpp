// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "FauxStore.h"
#include "FauxFM.h"
#include "RouterTestHelper.h"
#include "TestNodeWrapper.h"
#include "client/client.h"
#include "client/Client.Internal.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Client;
    using namespace Transport;
    using namespace Federation;
    using namespace Reliability;
    using namespace TestHelper;
    using namespace Client;
    using namespace ServiceModel;

    class DummyGatewayManager : public IGatewayManager
    {
    public:
        bool RegisterGatewaySettingsUpdateHandler()
        {
            return true;
        }

        AsyncOperationSPtr BeginActivateGateway(
            wstring const&,
            TimeSpan const&,
            AsyncCallback const &callback,
            AsyncOperationSPtr const &parent)
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                ErrorCodeValue::Success,
                callback,
                parent);
        }

        ErrorCode EndActivateGateway(
            AsyncOperationSPtr const &)
        {
            return ErrorCodeValue::Success;
        }

        AsyncOperationSPtr BeginDeactivateGateway(
            TimeSpan const&,
            AsyncCallback const &callback,
            AsyncOperationSPtr const &parent)
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                ErrorCodeValue::Success,
                callback,
                parent);
        }

        ErrorCode EndDeactivateGateway(
            AsyncOperationSPtr const &)
        {
            return ErrorCodeValue::Success;
        }

        void AbortGateway()
        {
        }
    };

    struct GatewayRoot;
    typedef shared_ptr<GatewayRoot> RootSPtr;
    struct GatewayRoot : public ComponentRoot
    {
        static RootSPtr Create(
            NodeConfig const & nodeConfig,
            wstring const & entreeServiceAddress,
            wstring const & ipcServerAddress,
            NodeConfig const & fmNodeConfig)
        {
            RootSPtr result = make_shared<GatewayRoot>();
            result->namingServiceDescription_ = ServiceDescription(
                ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName,
                0,
                0,
                NamingConfig::GetConfig().PartitionCount,
                NamingConfig::GetConfig().TargetReplicaSetSize,
                NamingConfig::GetConfig().MinReplicaSetSize,
                true,
                true,
                NamingConfig::GetConfig().ReplicaRestartWaitDuration,
                TimeSpan::MaxValue,
                NamingConfig::GetConfig().StandByReplicaKeepDuration,
                *ServiceModel::ServiceTypeIdentifier::NamingStoreServiceTypeId,
                vector<Reliability::ServiceCorrelationDescription>(),
                L"",
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>());

            Transport::SecuritySettings defaultSettings;
            result->fs_ = make_shared<FederationSubsystem>(nodeConfig, FabricCodeVersion(), nullptr, Common::Uri(), defaultSettings, *result);
            result->fsForFm_ = make_shared<FederationSubsystem>(fmNodeConfig, FabricCodeVersion(), nullptr, Common::Uri(), defaultSettings, *result);
            result->testNode_ = make_unique<TestHelper::TestNodeWrapper>(*(result->fs_), result->router_);
            result->adminClient_ = make_unique<ServiceAdminClient>(*(result->testNode_));
            result->resolver_ = make_unique<ServiceResolver>(*(result->testNode_), *result);

            NodeInstance nodeInstance(nodeConfig.Id, SequenceNumber::GetNext());

            result->nodeConfig_ = make_shared<FabricNodeConfig>();
            result->nodeConfig_->InstanceName = wformatString("{0}",  nodeInstance);

            result->entreeService_ = make_unique<EntreeServiceWrapper>(
                result->router_,
                *(result->resolver_),
                *(result->adminClient_),
                nodeInstance,
                result->nodeConfig_,
                entreeServiceAddress,
                ipcServerAddress,
                defaultSettings,
                *result);
            VERIFY_IS_TRUE(result->entreeService_->Open().IsSuccess());

            return result;
        }

        AsyncOperationSPtr CreateMultiRoot(FabricClientImpl & client)
        {
            vector<ComponentRootSPtr> roots;
            roots.push_back(client.CreateComponentRoot());
            roots.push_back(this->CreateComponentRoot());

            return this->CreateAsyncOperationMultiRoot(move(roots));
        }

        void ServiceResolverProcessWouldBeBroadcastMessage(
            ConsistencyUnitId id, 
            wstring const & serviceName,
            FauxFM & fm)
        {
            int64 oldVersion = 1;
            int64 newVersion = fm.GetFMVersion();
            vector<ConsistencyUnitId> ids;
            ids.push_back(id);
            ServiceResolverProcessWouldBeBroadcastMessage(ids, serviceName, fm, oldVersion, newVersion);
        }

         void ServiceResolverProcessWouldBeBroadcastMessage(
            ConsistencyUnitId id, 
            wstring const & serviceName,
            FauxFM & fm,
            int64 oldVersion,
            int64 newVersion)
        {
            vector<ConsistencyUnitId> ids;
            ids.push_back(id);
            ServiceResolverProcessWouldBeBroadcastMessage(ids, serviceName, fm, oldVersion, newVersion);
        }
                
         void ServiceResolverProcessWouldBeBroadcastMessage(
            vector<ConsistencyUnitId> ids,
            wstring const & serviceName, 
            FauxFM & fm,
            wstring const & primary = L"Location",
            vector<wstring> const & locations = vector<wstring>())
        {
            int64 oldVersion = 1;
            int64 newVersion = fm.GetFMVersion();
            ServiceResolverProcessWouldBeBroadcastMessage(ids, serviceName, fm, oldVersion, newVersion, primary, locations);
        }

        void ServiceResolverProcessWouldBeBroadcastMessage(
            vector<ConsistencyUnitId> ids,
            wstring const & serviceName, 
            FauxFM & fm,
            int64 oldVersion,
            int64 newVersion,
            wstring const & primary = L"Location",
            vector<wstring> const & locations = vector<wstring>())
        {
            vector<ServiceTableEntry> entries;
            
            for (size_t i = 0; i < ids.size(); ++i)
            {
                wstring primaryLocation = primary;
                vector<wstring> replicaLocations = locations;

                ServiceTableEntry newOne(
                    ids[i],
                    serviceName,
                    ServiceReplicaSet(
                        true,
                        primary != L"",
                        move(primaryLocation),
                        move(replicaLocations),
                        fm.GetCuidFMVersion(ids[i])));
                entries.push_back(newOne);
            }

            ServiceTableUpdateMessageBody body(move(entries), GenerationNumber(), VersionRangeCollection(oldVersion, newVersion), newVersion, false);
            MessageUPtr message = RSMessage::GetServiceTableUpdate().CreateMessage<ServiceTableUpdateMessageBody>(body);
            resolver_->ProcessServiceTableUpdateBroadcast(*message, CreateComponentRoot());
        }

        bool WaitForNotificationRegisteredForBroadcastUpdates(
            NamingUri const & name,
            ConsistencyUnitId const & cuid, 
            TimeSpan const & timeout = TimeSpan::FromSeconds(30))
        {
            TimeoutHelper timeoutHelper(timeout);
            while (!timeoutHelper.IsExpired)
            {
                // Check whether broadcast event manager has registered updates for this name and cuids
                if (entreeService_->BroadcastEventManager.NotificationRequests.Test_Contains(name, cuid))
                {
                    return true;
                }

                Sleep(100);
            }

            return false;
        }        

        void Close()
        {
            VERIFY_IS_TRUE(entreeService_->Close().IsSuccess());
            VERIFY_IS_TRUE(router_.Close().IsSuccess());
        }

        class EntreeServiceWrapper : public FabricComponent, public RootedObject
        {
        public:
            EntreeServiceWrapper(
                __in Federation::IRouter & innerRingCommunication,
                __in Reliability::ServiceResolver & fmClient,
                __in Reliability::ServiceAdminClient & adminClient,
                Federation::NodeInstance const & instance,
                FabricNodeConfigSPtr const & nodeConfig,
                wstring const & listenAddress,
                wstring const & ipcServerAddress,
                Transport::SecuritySettings const& securitySettings,
                Common::ComponentRoot const & root)
                : RootedObject(root)
            {
                proxyIpcClient_ = make_unique<ProxyToEntreeServiceIpcChannel>(root, instance.ToString(), ipcServerAddress);

                entreeServiceProxy_ = make_unique<EntreeServiceProxy>(
                    root,
                    listenAddress,
                    *proxyIpcClient_,
                    securitySettings);

                entreeService_ = make_unique<EntreeService>(
                    innerRingCommunication,
                    fmClient,
                    adminClient,
                    make_shared<DummyGatewayManager>(),
                    instance,
                    ipcServerAddress,
                    nodeConfig,
                    securitySettings,
                    root);
            }

            ErrorCode OnOpen()
            {
                // ipcClient and EntreeService should be open before the EntreeServiceProxy is opened.
                auto manualResetEvent = make_shared<ManualResetEvent>();
                manualResetEvent->Reset();
                auto operation = entreeService_->BeginOpen(
                    TimeSpan::FromSeconds(30),
                    [this, manualResetEvent](AsyncOperationSPtr const &)
                {
                    manualResetEvent->Set();
                },
                    Root.CreateAsyncOperationRoot());

                manualResetEvent->WaitOne();
                auto error = entreeService_->EndOpen(operation);
                VERIFY_IS_TRUE(error.IsSuccess());

                manualResetEvent->Reset();
                operation = proxyIpcClient_->BeginOpen(
                    TimeSpan::FromSeconds(30), 
                    [this, manualResetEvent](AsyncOperationSPtr const &)
                    {
                        manualResetEvent->Set();
                    },
                    Root.CreateAsyncOperationRoot());

                manualResetEvent->WaitOne();
                error = proxyIpcClient_->EndOpen(operation);
                VERIFY_IS_TRUE(error.IsSuccess());

                return entreeServiceProxy_->Open();
            }

            ErrorCode OnClose()
            {
                auto error = entreeServiceProxy_->Close();
                VERIFY_IS_TRUE(error.IsSuccess());

                auto manualResetEvent = make_shared<ManualResetEvent>();
                manualResetEvent->Reset();
                auto operation = proxyIpcClient_->BeginClose(
                    TimeSpan::FromSeconds(30), 
                    [this, manualResetEvent](AsyncOperationSPtr const &)
                    {
                        manualResetEvent->Set();
                    },
                    Root.CreateAsyncOperationRoot());

                manualResetEvent->WaitOne();
                error = proxyIpcClient_->EndClose(operation);
                VERIFY_IS_TRUE(error.IsSuccess());

                manualResetEvent->Reset();
                operation = entreeService_->BeginClose(
                    TimeSpan::FromSeconds(30),
                    [this, manualResetEvent](AsyncOperationSPtr const &)
                {
                    manualResetEvent->Set();
                },
                    Root.CreateAsyncOperationRoot());

                manualResetEvent->WaitOne();

                return entreeService_->EndClose(operation);
            }

            void OnAbort() {}

            __declspec(property(get=get_NamingServiceCuids)) std::vector<Reliability::ConsistencyUnitId> const & NamingServiceCuids;
            std::vector<Reliability::ConsistencyUnitId> const & get_NamingServiceCuids() const { return entreeService_->NamingServiceCuids; }

            __declspec(property(get=get_BroadcastEventManager)) Naming::BroadcastEventManager & BroadcastEventManager;
            Naming::BroadcastEventManager & get_BroadcastEventManager() { return entreeService_->Properties.BroadcastEventManager; }

        private:
            unique_ptr<EntreeService> entreeService_;
            unique_ptr<EntreeServiceProxy> entreeServiceProxy_;
            unique_ptr<ProxyToEntreeServiceIpcChannel> proxyIpcClient_;
        };

        FabricNodeConfigSPtr nodeConfig_;
        TestHelper::RouterTestHelper router_;
        FederationSubsystemSPtr fs_;
        FederationSubsystemSPtr fsForFm_;
        unique_ptr<TestHelper::TestNodeWrapper> testNode_;
        unique_ptr<ServiceAdminClient> adminClient_;
        unique_ptr<ServiceResolver> resolver_;
        unique_ptr<EntreeServiceWrapper> entreeService_;
        ServiceDescription namingServiceDescription_;
};

    class CachingTest
    {
    protected:
		CachingTest()
			: root_()
			, clientSPtr_()
			, clientSPtr2_()
		{
			BOOST_REQUIRE(MethodSetup());
		}
        TEST_METHOD_SETUP(MethodSetup)
        ~CachingTest() { BOOST_REQUIRE(MethodCleanup()); }
        TEST_METHOD_CLEANUP(MethodCleanup)

           

        RootSPtr CreateRoot(wstring & gatewayAddress);

        ServiceDescription CreateServiceDescription(NamingUri const &, int partitionCount, int replicaCount, int minWrite, bool isStateful = true);

        void SynchronousCreateName(__in FabricClientImpl & client, NamingUri const & name);

        void SynchronousCreateService(__in FabricClientImpl & client, PartitionedServiceDescriptor const & psd, __in FauxFM & fm, bool placeServiceInFM = true);

        void SynchronousDeleteService(__in FabricClientImpl & client, NamingUri const & name, __in FauxFM & fm);

        void GetNotificationsInBatches(
            __in FabricClientImpl & client,
            vector<ServiceLocationNotificationRequestData> const & requests,
            ErrorCodeValue::Enum const expectedError,
            int minExpectedMessageCount,
            __out vector<ResolvedServicePartitionSPtr> & results,
            __out vector<AddressDetectionFailureSPtr> & failures);

        void StartGetNotifications(
            __in FabricClientImpl & client,
            __in AutoResetEvent & wait,
            vector<ServiceLocationNotificationRequestData> const & requests,
            ErrorCodeValue::Enum const expectedError,
            __out vector<ResolvedServicePartitionSPtr> & results,
            __out vector<AddressDetectionFailureSPtr> & failures);

        void FinishWait(__in AutoResetEvent & wait, TimeSpan const timeout, bool success = true);       

        AsyncOperationSPtr BeginResolveServicePartition(
            __in FabricClientImpl & client,
            NamingUri const & name, 
            PartitionKey const & partitionKey, 
            TimeSpan timeout,
            AsyncCallback const & callback, 
            AsyncOperationSPtr const & root_ = AsyncOperationSPtr(),
            ResolvedServicePartitionSPtr const & previous = nullptr);

        void EndResolveServicePartition(
            AsyncOperationSPtr const & resolveOperation,
            __in FabricClientImpl & client,
            __out ResolvedServicePartitionSPtr & partition,
            ErrorCodeValue::Enum expected = ErrorCodeValue::Success);

        ResolvedServicePartitionSPtr SyncResolveService(
            __in FabricClientImpl & client,
            NamingUri const & name, 
            PartitionKey const & partitionKey, 
            ResolvedServicePartitionSPtr const & previous,
            ErrorCodeValue::Enum expectedError = ErrorCodeValue::Success);

        static void CheckResolvedServicePartition(
            ResolvedServicePartitionSPtr const & location,
            __in FauxFM & fm,
            __in FauxStore & store,
            ConsistencyUnitId const & cuid);

        static void CheckResolvedServicePartition(
            ResolvedServicePartitionSPtr const & location,
            __in FauxFM & fm,
            int64 storeVersion,
            ConsistencyUnitId const & cuid);

        void CheckResolvedServicePartition(
            vector<ResolvedServicePartitionSPtr> const & partitions,
            size_t expectedCount,
            __in FauxFM & fm,
            __in FauxStore & store,
            __out vector<NamingUri> & serviceName,
            __out map<ConsistencyUnitId, ServiceLocationVersion> & serviceLocations);

        void CompareResolvedServicePartitions(
            ResolvedServicePartitionSPtr const & left,
            ResolvedServicePartitionSPtr const & right,
            LONG expectedResult,
            bool expectedEqual = true);

        void InnerNotificationBasicTest(
            NamingUri const & name,
            PartitionedServiceDescriptor const & psd,
            NamingUri const & name2,
            PartitionKey const & partitionKey2,
            PartitionedServiceDescriptor const & psd2);

        void InnerNotificationMultipleParams(
            int serviceCount,
            int partitionCount,
            int replicaCount,
            int keyRequestsPerPartition,
            int replicaNameSize,
            int maxNotificationReplyEntryCount = 0);

        void InnerCacheTest(
            NamingUri const & name,
            PartitionKey const & partitionKey,
            PartitionedServiceDescriptor const & psd);

        void InnerRefreshTest(
            NamingUri const & name,
            PartitionKey const & partitionKey,
            PartitionedServiceDescriptor const & psd);
    
        void InnerResolveAllTest(
            NamingUri const & name,
            PartitionKey const & partitionKey,
            PartitionedServiceDescriptor const & psd,
            bool isStateful);

        void InnerDeleteServiceClearing(
            NamingUri const & name,
            PartitionKey const & partitionKey,
            PartitionedServiceDescriptor const & psd,
            PartitionedServiceDescriptor const & psd2);

        void InnerReplacingEmptySTEs(
            NamingUri const & name,
            PartitionKey const & partitionKey,
            PartitionedServiceDescriptor const & psd);

        void InnerServiceOfflineTest(
            NamingUri const & name,
            PartitionKey const & partitionKey,
            PartitionedServiceDescriptor const & psd);
        
        static void CheckLocation(ResolvedServicePartitionSPtr const & location, bool isStateful);
        static ServiceLocationNotificationRequestData CreateRequestData(
            NamingUri const & name, 
            std::map<Reliability::ConsistencyUnitId, ServiceLocationVersion> const & previousResolves,
            PartitionKey const & pk = PartitionKey::Test_GetWholeServicePartitionKey());
        static ServiceLocationNotificationRequestData CreateRequestData(
            NamingUri const & name, 
            AddressDetectionFailureSPtr const & failure,
            PartitionKey const & pk = PartitionKey::Test_GetWholeServicePartitionKey());
        static ServiceLocationNotificationRequestData CreateRequestData(
            NamingUri const & name, 
            PartitionKey const & pk = PartitionKey::Test_GetWholeServicePartitionKey());

        RootSPtr root_;
        shared_ptr<FabricClientImpl> clientSPtr_;
        shared_ptr<FabricClientImpl> clientSPtr2_;
    };




    BOOST_FIXTURE_TEST_SUITE(CachingTestSuite,CachingTest)

    BOOST_AUTO_TEST_CASE(Notification_Basic_Int64Key)
    {
        NamingUri name(L"Notification_Basic_Int64Key_1");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 2, 1, 1),
            0,
            100,
            psd).IsSuccess());

        NamingUri name2(L"Notification_Basic_Int64Key_2");
        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name2, 4, 1, 1),
            0,
            100,
            psd2).IsSuccess());
        PartitionKey pk2(66);
        InnerNotificationBasicTest(name, psd, name2, pk2, psd2);
    }

    BOOST_AUTO_TEST_CASE(Notification_Basic_SingletonKey)
    {
        NamingUri name(L"Notification_Basic_SingletonKey_1");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            psd).IsSuccess());

        NamingUri name2(L"Notification_Basic_SingletonKey_2");
        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name2, 1, 1, 1),
            psd2).IsSuccess());
        PartitionKey pk2;
        InnerNotificationBasicTest(name, psd, name2, pk2, psd2);
    }

    BOOST_AUTO_TEST_CASE(Notification_Basic_NamedKey)
    {
        NamingUri name(L"Notification_Basic_NamedKey_1");        
        vector<wstring> partitionNames;
        partitionNames.push_back(L"A");
        partitionNames.push_back(L"B");
        partitionNames.push_back(L"C");
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 3, 1, 1),
            partitionNames,
            psd).IsSuccess());

        NamingUri name2(L"Notification_Basic_NamedKey_2");               
        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name2, 3, 1, 1),
            partitionNames,
            psd2).IsSuccess());
        PartitionKey pk2(L"B");

        InnerNotificationBasicTest(name, psd, name2, pk2, psd2);
    }

       
    BOOST_AUTO_TEST_CASE(Notification_MultiParams)
    {
        // Set message size to a smaller number and start many requests
        // to test that all notifications are received
        InnerNotificationMultipleParams(
            5 /*serviceCount*/, 
            4 /*partitionCount*/, 
            3 /*replicaCount*/, 
            2 /*keyRequestsPerPartition*/, 
            128 /*replicaNameSizeInChars*/);
    }

    BOOST_AUTO_TEST_CASE(Notification_MultiParams_ReachMsgSize)
    {
        // Set message content buffer ration to a smaller number and start many requests
        // to test that all notifications are received
        InnerNotificationMultipleParams(
            5 /*serviceCount*/, 
            6 /*partitionCount*/, 
            5 /*replicaCount*/, 
            1 /*keyRequestsPerPartition*/, 
            64 /*replicaNameSizeInChars*/);
    }

    BOOST_AUTO_TEST_CASE(Notification_MultiParams_ReachMsgCount)
    {
        // Pass small maxNotificationReplyEntryCount to force the gateway
        // to stop adding notification in one batch
        // All notifications should be eventually received
        InnerNotificationMultipleParams(
            10 /*serviceCount*/, 
            8 /*partitionCount*/, 
            5 /*replicaCount*/, 
            1 /*keyRequestsPerPartition*/, 
            128 /*replicaNameSizeInChars*/,
            5 /*maxNotificationReplyEntryCount*/);
    }

    BOOST_AUTO_TEST_CASE(Notification_NoUpdates)
    {
        // Make the poll interval very small
        ClientConfig::GetConfig().ConnectionInitializationTimeout = TimeSpan::FromSeconds(1);
        ClientConfig::GetConfig().ServiceChangePollInterval = TimeSpan::FromSeconds(5);

        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);          
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        // Register for notifications for a name that doesn't exist
        // Failure will be raised first, then no more notifications will fire
        vector<ResolvedServicePartitionSPtr> results;
        vector<AddressDetectionFailureSPtr> failures;
        vector<ServiceLocationNotificationRequestData> requests;
        NamingUri name(L"Notification_NoUpdates");
        requests.push_back(CreateRequestData(name));
        
        // Start waiting for notifications;
        // since the service doesn't exist, receive failure UserServiceNotFound
        AutoResetEvent wait;
        StartGetNotifications(client, wait, requests, ErrorCodeValue::Success, results, failures);
        FinishWait(wait, TimeSpan::FromSeconds(10));
        VERIFY_ARE_EQUAL(0U, results.size());
        VERIFY_ARE_EQUAL(1U, failures.size());
		VERIFY_IS_TRUE_FMT(
            failures[0]->Error == ErrorCodeValue::UserServiceNotFound, 
            "Failure received {0}, expected {1}", failures[0]->Error, ErrorCodeValue::UserServiceNotFound);

        // New request with previous included will not return any data
        vector<ServiceLocationNotificationRequestData> requests1;
        requests1.push_back(CreateRequestData(name, failures[0]));
        StartGetNotifications(client, wait, requests1, ErrorCodeValue::Success, results, failures);
        FinishWait(wait, TimeSpan::FromSeconds(10));

        VERIFY_ARE_EQUAL(0U, results.size());
        VERIFY_ARE_EQUAL(0U, failures.size());

        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(Notification_CreateDeleteService)
    {
        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);          
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   
        AutoResetEvent wait;

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        vector<ResolvedServicePartitionSPtr> results;
        vector<AddressDetectionFailureSPtr> failures;
        vector<ServiceLocationNotificationRequestData> requests;
        NamingUri name(L"Notification_CreateDeleteService");
        SynchronousCreateName(client, name);
                
        // create, resolve, then delete service
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
        PartitionKey pk(66);
        ConsistencyUnitId cuid;
        psd.TryGetCuid(pk, /*out*/cuid);
        requests.push_back(CreateRequestData(name, pk));

        // Start waiting for notifications;
        // since the service doesn't exist, receive failure UserServiceNotFound
        StartGetNotifications(client, wait, requests, ErrorCodeValue::Success, results, failures);
        FinishWait(wait, TimeSpan::FromSeconds(30));
        VERIFY_ARE_EQUAL(0U, results.size());
        VERIFY_ARE_EQUAL(1U, failures.size());
		VERIFY_IS_TRUE_FMT(
            failures[0]->Error == ErrorCodeValue::UserServiceNotFound, 
            "Failure received {0}, expected {1}", failures[0]->Error, ErrorCodeValue::UserServiceNotFound);

        // Create service and receive notification with 1 address 
        // for the request with the previous error included.
        // First poll will hit UserServiceNotFound; since the error is not newer than previous one,
        // wait for a broadcast and return the addresses after service is created.
        vector<ServiceLocationNotificationRequestData> requests1;
        requests1.push_back(CreateRequestData(name, failures[0], pk));
        StartGetNotifications(client, wait, requests1, ErrorCodeValue::Success, results, failures);

        SynchronousCreateService(client, psd, fm);
        fm.IncrementCuidFMVersion(cuid);

        VERIFY_IS_TRUE(root_->WaitForNotificationRegisteredForBroadcastUpdates(name, cuid));
        root_->ServiceResolverProcessWouldBeBroadcastMessage(cuid, name.ToString(), fm);

        FinishWait(wait, TimeSpan::FromSeconds(30));
        VERIFY_ARE_EQUAL(1U, results.size());
        VERIFY_ARE_EQUAL(0U, failures.size());
        CheckResolvedServicePartition(results[0], fm, store, cuid);
                
        // Wait again for notification;
        // Change will be received after fm broadcast increase is received
        map<ConsistencyUnitId, ServiceLocationVersion> previous;
        previous[cuid] = ServiceLocationVersion(results[0]->FMVersion, results[0]->Generation, results[0]->StoreVersion);
        vector<ServiceLocationNotificationRequestData> requests2;
        requests2.push_back(CreateRequestData(name, previous, pk));
        StartGetNotifications(client, wait, requests2, ErrorCodeValue::Success, results, failures);
        // increment fm version and broadcast change    
        fm.IncrementCuidFMVersion(cuid);
        VERIFY_IS_TRUE(root_->WaitForNotificationRegisteredForBroadcastUpdates(name, cuid));
        root_->ServiceResolverProcessWouldBeBroadcastMessage(cuid, name.ToString(), fm);

        FinishWait(wait, TimeSpan::FromSeconds(30));
        VERIFY_ARE_EQUAL(1U, results.size());
        CheckResolvedServicePartition(results[0], fm, store, cuid);
        
        // setup another client to do the delete
        listenAddresses.push_back(gatewayAddress);
        clientSPtr2_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client2 = *clientSPtr2_;
        SynchronousDeleteService(client2, name, fm);        

        vector<ServiceLocationNotificationRequestData> requests3;
        previous[cuid] = ServiceLocationVersion(results[0]->FMVersion, results[0]->Generation, results[0]->StoreVersion);
        requests3.push_back(CreateRequestData(name, previous, pk));
        StartGetNotifications(client, wait, requests3, ErrorCodeValue::Success, results, failures);
        FinishWait(wait, TimeSpan::FromSeconds(30));
        VERIFY_ARE_EQUAL(0U, results.size());
        VERIFY_ARE_EQUAL(1U, failures.size());
        VERIFY_IS_TRUE(failures[0]->Error == ErrorCodeValue::UserServiceNotFound, L"Expected failure received");

        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
        VERIFY_IS_TRUE(client2.Close().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(Notification_AdvDeleteService)
    {
        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);    
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   
        AutoResetEvent wait;

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        vector<ResolvedServicePartitionSPtr> results;
        vector<AddressDetectionFailureSPtr> failures;
        vector<ServiceLocationNotificationRequestData> requests;
        NamingUri name(L"Notification_AdvDeleteService");
        SynchronousCreateName(client, name);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
        SynchronousCreateService(client, psd, fm);
        ConsistencyUnitId cuid;
        PartitionKey pk(66);
        psd.TryGetCuid(pk, /*out*/cuid);

        fm.IncrementCuidFMVersion(cuid);
        root_->ServiceResolverProcessWouldBeBroadcastMessage(cuid, name.ToString(), fm);

        // get notifications - should use cached version
        requests.push_back(CreateRequestData(name, pk));
        StartGetNotifications(client, wait, requests, ErrorCodeValue::Success, results, failures);
        FinishWait(wait, TimeSpan::FromSeconds(30));
        CheckResolvedServicePartition(results[0], fm, store, cuid);
                
        // get notifications - should see empty caches on gateway and get an empty result
        vector<ServiceLocationNotificationRequestData> requests2;
        map<ConsistencyUnitId, ServiceLocationVersion> previous;
        previous[cuid] = ServiceLocationVersion(results[0]->FMVersion, results[0]->Generation, results[0]->StoreVersion);
        requests2.push_back(CreateRequestData(name, previous));
        
        StartGetNotifications(client, wait, requests2, ErrorCodeValue::Success, results, failures);
        VERIFY_IS_TRUE(root_->WaitForNotificationRegisteredForBroadcastUpdates(name, cuid));
        
        // setup another client to do the delete
        listenAddresses.push_back(gatewayAddress);
        clientSPtr2_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client2 = *clientSPtr2_;
        SynchronousDeleteService(client2, name, fm);        

        // broadcast delete
        fm.IncrementCuidFMVersion(cuid);
        root_->ServiceResolverProcessWouldBeBroadcastMessage(cuid, name.ToString(), fm);
        
        FinishWait(wait, TimeSpan::FromSeconds(30));
        
        VERIFY_ARE_EQUAL(0U, results.size());
        VERIFY_ARE_EQUAL(1U, failures.size());
        VERIFY_IS_TRUE(failures[0]->Error == ErrorCodeValue::UserServiceNotFound, L"Expected failure received");

        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
        VERIFY_IS_TRUE(client2.Close().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(Notification_MultiService)
    {
        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);      
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   
        AutoResetEvent wait;

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        // create first service
        NamingUri name1(L"MultiService1");        
        SynchronousCreateName(client, name1);
        PartitionedServiceDescriptor psd1;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name1, 3, 1, 1),
            0,
            100,
            psd1).IsSuccess());
        SynchronousCreateService(client, psd1, fm);        

        vector<ServiceLocationNotificationRequestData> requests;
        requests.push_back(CreateRequestData(name1));

        // get notifications - should use cached version
        vector<ResolvedServicePartitionSPtr> results;
        vector<AddressDetectionFailureSPtr> failures;
        StartGetNotifications(client, wait, requests, ErrorCodeValue::Success, results, failures);
        
        // broadcast created service
        fm.IncrementFMVersion();
        VERIFY_IS_TRUE(root_->WaitForNotificationRegisteredForBroadcastUpdates(name1, psd1.Cuids[0]));
        root_->ServiceResolverProcessWouldBeBroadcastMessage(psd1.Cuids, name1.ToString(), fm);

        FinishWait(wait, TimeSpan::FromSeconds(30));
        
        map<ConsistencyUnitId, ServiceLocationVersion> service1Previous;
        vector<NamingUri> serviceNames;
        CheckResolvedServicePartition(results, 3U, fm, store, serviceNames, service1Previous);

        for (auto name : serviceNames)
        {
            VERIFY_ARE_EQUAL(name, name1);
        }

        // create second service
        NamingUri name2(L"MultiService2");        
        SynchronousCreateName(client, name2);
        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name2, 3, 1, 1),
            0,
            100,
            psd2).IsSuccess());
        SynchronousCreateService(client, psd2, fm);        

        // broadcast created service
        fm.IncrementFMVersion();
        root_->ServiceResolverProcessWouldBeBroadcastMessage(psd2.Cuids, name2.ToString(), fm);

        // get notifications
        requests.clear();
        requests.push_back(CreateRequestData(name1, service1Previous));
        requests.push_back(CreateRequestData(name2));
        StartGetNotifications(client, wait, requests, ErrorCodeValue::Success, results, failures);
        
        FinishWait(wait, TimeSpan::FromSeconds(30));
        
        // results should have only the second service
        map<ConsistencyUnitId, ServiceLocationVersion> service2Previous;
        vector<NamingUri> serviceNames2;
        CheckResolvedServicePartition(results, 3U, fm, store, serviceNames2, service2Previous);

        for (auto name : serviceNames2)
        {
            VERIFY_ARE_EQUAL(name, name2);
        }

        // create third service
        NamingUri name3(L"MultiService3");        
        SynchronousCreateName(client, name3);
        PartitionedServiceDescriptor psd3;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name3, 3, 1, 1),
            0,
            100,
            psd3).IsSuccess());
        SynchronousCreateService(client, psd3, fm);        

        // broadcast created service and update service1
        for (auto it = psd3.Cuids.begin(); it != psd3.Cuids.end(); ++it)
        {
            fm.IncrementCuidFMVersion(*it);
        }
        for (auto it = psd1.Cuids.begin(); it != psd1.Cuids.end(); ++it)
        {
            fm.IncrementCuidFMVersion(*it);
        }

        root_->ServiceResolverProcessWouldBeBroadcastMessage(psd3.Cuids, name3.ToString(), fm);
        root_->ServiceResolverProcessWouldBeBroadcastMessage(psd1.Cuids, name1.ToString(), fm);

        // get notifications
        requests.clear();
        requests.push_back(CreateRequestData(name1, service1Previous));
        requests.push_back(CreateRequestData(name2, service2Previous));
        requests.push_back(CreateRequestData(name3));

        StartGetNotifications(client, wait, requests, ErrorCodeValue::Success, results, failures);
        FinishWait(wait, TimeSpan::FromSeconds(30));

        // results should have the first and third service
        map<ConsistencyUnitId, ServiceLocationVersion> service3Previous;
        vector<NamingUri> serviceNames3;
        CheckResolvedServicePartition(results, 6U, fm, store, serviceNames3, service3Previous);

        for (auto name : serviceNames3)
        {
            if ((name != name1) && (name != name3))
            {
                VERIFY_FAIL(L"Names don't match");
            }
        }

        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
    }
    
    BOOST_AUTO_TEST_CASE(Notification_Wrong_PartitionKey)
    {
         // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);        
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   
        AutoResetEvent wait;

        // Change the config poll interval, as the notification operation waits for this time before giving up
        ClientConfig::GetConfig().ServiceChangePollInterval = TimeSpan::FromSeconds(5);

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        // create service with Int64 partition key
        NamingUri name(L"Notification_Wrong_PartitionKey");        
        SynchronousCreateName(client, name);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
        SynchronousCreateService(client, psd, fm);

        PartitionKey pk(66);
        // Resolve service to bring the int64 partition key in the entree service cache
        auto location = SyncResolveService(client, name, pk, nullptr);

        vector<ResolvedServicePartitionSPtr> results;
        vector<AddressDetectionFailureSPtr> failures;
        vector<ServiceLocationNotificationRequestData> requests;
        requests.push_back(CreateRequestData(name, PartitionKey(L"WrongPartitionKey")));

        StartGetNotifications(client, wait, requests, ErrorCodeValue::Success, results, failures);
        FinishWait(wait, TimeSpan::FromSeconds(30));
        VERIFY_ARE_EQUAL(1U, failures.size());
		VERIFY_IS_TRUE_FMT(
            failures[0]->Error == ErrorCodeValue::InvalidServicePartition, 
            "Failure received {0}, expected {1}", failures[0]->Error, ErrorCodeValue::InvalidServicePartition);

        bool entryExists = client.Test_TryGetCachedServiceResolution(name, pk, location);
        VERIFY_IS_TRUE(entryExists, L"The entry shouldn't have been removed from cache");
        
        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(Notification_UnplacedService)
    {
        ClientConfig::GetConfig().ServiceChangePollInterval = TimeSpan::FromSeconds(7);

        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);  
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   
        AutoResetEvent wait;

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        // create service
        NamingUri name(L"Notification_UnplacedService");        
        SynchronousCreateName(client, name);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
        SynchronousCreateService(client, psd, fm, false);
        PartitionKey partitionKey(0);

        vector<ResolvedServicePartitionSPtr> results;
        vector<AddressDetectionFailureSPtr> failures;
        vector<ServiceLocationNotificationRequestData> requests;
        requests.push_back(CreateRequestData(name, partitionKey));

        StartGetNotifications(client, wait, requests, ErrorCodeValue::Success, results, failures);

        // Wait for the notification to register for broadcast messages
        VERIFY_IS_TRUE(root_->WaitForNotificationRegisteredForBroadcastUpdates(name, psd.Cuids[0]));

        fm.ChangeCuidLocation(psd.Cuids[0], L"");
        root_->ServiceResolverProcessWouldBeBroadcastMessage(psd.Cuids, name.ToString(), fm, L"");

        FinishWait(wait, TimeSpan::FromSeconds(30));
        
        VERIFY_ARE_EQUAL(1U, failures.size());
		VERIFY_IS_TRUE_FMT(
            failures[0]->Error == ErrorCodeValue::ServiceOffline, 
            "Failure received {0}, expected {1}", failures[0]->Error, ErrorCodeValue::ServiceOffline);

        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
    }
    
    BOOST_AUTO_TEST_CASE(GetCachedVersion_Int64Key)
    {
        NamingUri name(L"GetCached_Int64Key");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
        PartitionKey pk(0);
        InnerCacheTest(name, pk, psd);
    }

    BOOST_AUTO_TEST_CASE(GetCachedVersion_SingletonKey)
    {
        NamingUri name(L"GetCached_SingletonKey");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            psd).IsSuccess());
        PartitionKey pk;
        InnerCacheTest(name, pk, psd);
    }

    BOOST_AUTO_TEST_CASE(GetCachedVersion_NamedKey)
    {
        NamingUri name(L"GetCached_NamedKey");        
        
        vector<wstring> partitionNames;
        partitionNames.push_back(L"A");
        partitionNames.push_back(L"B");
        partitionNames.push_back(L"C");
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 3, 1, 1),
            partitionNames,
            psd).IsSuccess());
        // Pass the last partition added, so the FMVersion check passes
        PartitionKey pk(L"A");
        InnerCacheTest(name, pk, psd);
    }

   
    BOOST_AUTO_TEST_CASE(Refresh_PartitionVersion_Int64Key)
    {
        NamingUri name(L"Refresh_PartitionVersion_Int64Key");
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
        PartitionKey pk(0);
        InnerRefreshTest(name, pk, psd);
    }

    BOOST_AUTO_TEST_CASE(Refresh_PartitionVersion_SingletonKey)
    {
        NamingUri name(L"Refresh_SingletonKey");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            psd).IsSuccess());
        PartitionKey pk;
        InnerRefreshTest(name, pk, psd);
    }

    BOOST_AUTO_TEST_CASE(Refresh_PartitionVersion_NamedKey)
    {
        NamingUri name(L"Refresh_PartitionVersion_NamedKey");        
        
        vector<wstring> partitionNames;
        partitionNames.push_back(L"A");
        partitionNames.push_back(L"B");
        partitionNames.push_back(L"C");
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 3, 1, 1),
            partitionNames,
            psd).IsSuccess());
        PartitionKey pk(L"A");
        InnerRefreshTest(name, pk, psd);
    }

  
 
    BOOST_AUTO_TEST_CASE(Refresh_StoreVersion)
    {
        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);  
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        // create service
        NamingUri name(L"TriggerRefresh");        
        SynchronousCreateName(client, name);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
        SynchronousCreateService(client, psd, fm);
        PartitionKey partitionKey(0);

        // resolve once to populate cache
        ResolvedServicePartitionSPtr location = SyncResolveService(client, name, partitionKey, nullptr);
        ConsistencyUnitId partitionCuid = location->Locations.ConsistencyUnitId;
        
        int64 firstFmVersion = location->FMVersion;
        int64 firstStoreVersion = location->StoreVersion;
        Trace.WriteInfo(Constants::TestSource, "Location[1]: storeVer={0}, fmVersion={1}", firstStoreVersion, firstFmVersion);
        CheckResolvedServicePartition(location, fm, store, partitionCuid);
        
        // update Store
        store.IncrementStoreVersion();

        // refresh and verify update (PSD will be cached at gateway)
        location = SyncResolveService(client, name, partitionKey, location);
        int64 secondStoreVersion = location->StoreVersion;
        Trace.WriteInfo(Constants::TestSource, "Location[2]: storeVer={0}, fmVersion={1}", secondStoreVersion, location->FMVersion);
        CheckResolvedServicePartition(location, fm, firstStoreVersion, partitionCuid);

        // update Store and resolve using a second client.
        //
        // The second client has an empty PSD cache, so it will fetch
        // the PSD. The gateway will not refresh its PSD cache
        // unless the service is deleted (or re-created).
        //
        store.IncrementStoreVersion();

        listenAddresses.push_back(gatewayAddress);
        clientSPtr2_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client2 = *clientSPtr2_;

        // get FM version
        ResolvedServicePartitionSPtr location2 = SyncResolveService(client2, name, partitionKey, nullptr);

        int64 thirdStoreVersion = location2->StoreVersion;
        Trace.WriteInfo(Constants::TestSource, "Location[3]: storeVer={0}, fmVersion={1}", thirdStoreVersion, location2->FMVersion);
        CheckResolvedServicePartition(location, fm, secondStoreVersion, partitionCuid);

        // update Store again (first clientVersion < storeVersion)
        store.IncrementStoreVersion();

        // calling refresh on first client will bring the store version:
        // since Client1 cache version < gateway cache:
        // cached PSD store version is higher than received one, but service resolver is up to date, so refresh triggers going to the store
        location = SyncResolveService(client, name, partitionKey, location);
        
        int64 fourthStoreVersion = location->StoreVersion;
        Trace.WriteInfo(Constants::TestSource, "Location[4]: storeVer={0}, fmVersion={1}", fourthStoreVersion, location->FMVersion);
        CheckResolvedServicePartition(location, fm, thirdStoreVersion, partitionCuid);

        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(Refresh_UnplacedService)
    {
        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);  
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        // create service
        NamingUri name(L"UnplacedService");        
        SynchronousCreateName(client, name);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
        SynchronousCreateService(client, psd, fm, false);
        PartitionKey partitionKey(0);

        // view empty cache
        ResolvedServicePartitionSPtr location;
        bool entryExists = client.Test_TryGetCachedServiceResolution(name, partitionKey, location);
        VERIFY_IS_FALSE(entryExists, L"Initial GetCachedServiceResolution");

        // resolve
        location = SyncResolveService(client, name, partitionKey, nullptr, ErrorCodeValue::ServiceOffline);
    }

    BOOST_AUTO_TEST_CASE(DeleteServiceClearing_Int64Key)
    {
        NamingUri name(L"DeleteServiceClearing_Int64Key");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
                
        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd2).IsSuccess());

        PartitionKey pk(0);
        InnerDeleteServiceClearing(name, pk, psd, psd2);
    }

    BOOST_AUTO_TEST_CASE(DeleteServiceClearing_SingletonKey)
    {
        NamingUri name(L"DeleteServiceClearing_SingletonKey");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            psd).IsSuccess());

        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            psd2).IsSuccess());

        PartitionKey pk;
        InnerDeleteServiceClearing(name, pk, psd, psd2);
    }
     
    BOOST_AUTO_TEST_CASE(DeleteServiceClearing_NamedKey)
    {
        NamingUri name(L"DeleteServiceClearing_NamedKey");        
        vector<wstring> partitionNames;
        partitionNames.push_back(L"A");
        partitionNames.push_back(L"B");
        partitionNames.push_back(L"C");
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 3, 1, 1),
            partitionNames,
            psd).IsSuccess());
        
        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 3, 1, 1),
            partitionNames,
            psd2).IsSuccess());

        PartitionKey pk(L"B");
        InnerDeleteServiceClearing(name, pk, psd, psd2);
    }

  
    BOOST_AUTO_TEST_CASE(BypassStoreRequest)
    {
        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);  
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        // create service
        NamingUri name(L"BypassStore");        
        SynchronousCreateName(client, name);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
        SynchronousCreateService(client, psd, fm); 
        PartitionKey partitionKey(0);

        // resolve once to populate caches
        ResolvedServicePartitionSPtr location = SyncResolveService(client, name, partitionKey, nullptr);
        ConsistencyUnitId partitionCuid = location->Locations.ConsistencyUnitId;
        CheckResolvedServicePartition(location, fm, store, partitionCuid);
        int64 firstFmVersion = location->FMVersion;
        int64 firstStoreVersion = location->StoreVersion;
        
        // update FM and broadcast
        fm.IncrementCuidFMVersion(partitionCuid);
        root_->ServiceResolverProcessWouldBeBroadcastMessage(partitionCuid, name.ToString(), fm);

        // update store
        store.IncrementStoreVersion();

        // resolve with trigger and ensure new refreshed store version
        location = SyncResolveService(client, name, partitionKey, location);
        CheckResolvedServicePartition(location, fm, firstStoreVersion, partitionCuid);
        int64 secondFmVersion = location->FMVersion;
        VERIFY_ARE_NOT_EQUAL(firstFmVersion, secondFmVersion);
        int64 resolvedStoreVersion = location->StoreVersion;
        VERIFY_ARE_EQUAL(firstStoreVersion, resolvedStoreVersion);
        
        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(ReplacingEmptySTEs_Int64Key)
    {
        NamingUri name(L"ReplacingEmptySTEs_Int64Key");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
        PartitionKey pk(0);
        InnerReplacingEmptySTEs(name, pk, psd);
    }

    BOOST_AUTO_TEST_CASE(ReplacingEmptySTEs_SingletonKey)
    {
        NamingUri name(L"ReplacingEmptySTEs_SingletonKey");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            psd).IsSuccess());
        PartitionKey pk;
        InnerReplacingEmptySTEs(name, pk, psd);
    }
    
    BOOST_AUTO_TEST_CASE(ReplacingEmptySTEs_NamedKey)
    {
        NamingUri name(L"ReplacingEmptySTEs_NamedKey");        

        vector<wstring> partitionNames;
        partitionNames.push_back(L"A");
        partitionNames.push_back(L"B");
        partitionNames.push_back(L"C");
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 3, 1, 1),
            partitionNames,
            psd).IsSuccess());
        PartitionKey pk(L"A");
        InnerReplacingEmptySTEs(name, pk, psd);
    }

   

    BOOST_AUTO_TEST_CASE(CacheSizeLimit)
    {
        // set cache size to 4
        int cacheLimit = 4;
        int serviceCount = cacheLimit + 1;
        ClientConfig::GetConfig().PartitionLocationCacheLimit = cacheLimit;

        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);  
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        // create services
        vector<NamingUri> names;
        for (auto ix=0; ix<serviceCount; ++ix)
        {
            NamingUri name(wformatString("SizeLimit_{0}", ix));
            SynchronousCreateName(client, name);
            PartitionedServiceDescriptor psd;
            VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
                CreateServiceDescription(name, 10, 1, 1),
                0,
                100,
                psd).IsSuccess());
            SynchronousCreateService(client, psd, fm);

            names.push_back(name);
        }

        VERIFY_IS_TRUE(serviceCount > 0);
        VERIFY_IS_TRUE(names.size() == serviceCount);

        // resolve a partition location
        ResolvedServicePartitionSPtr location = SyncResolveService(client, names.front(), PartitionKey(0), nullptr);
        vector<ConsistencyUnitId> cuids;
        ConsistencyUnitId partitionCuid = location->Locations.ConsistencyUnitId;
        cuids.push_back(partitionCuid);

        Trace.WriteInfo(Constants::TestSource, "Check 1: rsp={0}", location);
        CheckResolvedServicePartition(location, fm, store, partitionCuid);
        
        // resolve 4 additional services to fill the client cache
        for (int ix = 1; ix < serviceCount; ++ix)
        {
            location = SyncResolveService(client, names[ix], PartitionKey(20 * ix), nullptr);
            auto cuid = location->Locations.ConsistencyUnitId;

            Trace.WriteInfo(Constants::TestSource, "Check 2: rsp={0}", location);
            CheckResolvedServicePartition(location, fm, store, cuid);
            cuids.push_back(cuid);
        }

        // update FM version and broadcast
        fm.IncrementCuidFMVersion(partitionCuid);
        root_->ServiceResolverProcessWouldBeBroadcastMessage(partitionCuid, names.front().ToString(), fm);

        // re-resolve first entry from earlier with NO_REFRESH - should return gateway's updated copy instead of client's
        location = SyncResolveService(client, names.front(), PartitionKey(0), nullptr);

        Trace.WriteInfo(Constants::TestSource, "Check 3: rsp={0}", location);
        CheckResolvedServicePartition(location, fm, store, cuids[0]);

        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
    }    

    BOOST_AUTO_TEST_CASE(ClientCacheUpdate)
    {
        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);  
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        // create service
        NamingUri name(L"ClientCacheUpdate");        
        SynchronousCreateName(client, name);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 10, 1, 1),
            0,
            1000,
            psd).IsSuccess());
        SynchronousCreateService(client, psd, fm);

        // resolve a partition location
        ResolvedServicePartitionSPtr location = SyncResolveService(client, name, PartitionKey(0), nullptr);
        ConsistencyUnitId partitionCuid = location->Locations.ConsistencyUnitId;
        int64 firstFmVersion = location->FMVersion;
        CheckResolvedServicePartition(location, fm, store, partitionCuid);
        
        // update FM
        fm.IncrementCuidFMVersion(partitionCuid);

        // refreshing location
        location = SyncResolveService(client, name, PartitionKey(0), location);
        CheckResolvedServicePartition(location, fm, store, partitionCuid);
        int64 secondFmVersion = location->FMVersion;
        VERIFY_ARE_NOT_EQUAL(firstFmVersion, secondFmVersion);
        
        // resolve again with no refresh
        auto location2 = SyncResolveService(client, name, PartitionKey(0), nullptr);
        CompareResolvedServicePartitions(location, location2, 0);
        CheckResolvedServicePartition(location2, fm, store, partitionCuid);
        VERIFY_ARE_EQUAL(secondFmVersion, location2->FMVersion);
        
        // update FM
        fm.IncrementCuidFMVersion(partitionCuid);

        // resolve again with no refresh
        location = SyncResolveService(client, name, PartitionKey(0), nullptr);
        VERIFY_ARE_EQUAL(partitionCuid, location->Locations.ConsistencyUnitId);
        VERIFY_ARE_NOT_EQUAL(fm.GetFMVersion(), location->FMVersion);
        VERIFY_ARE_EQUAL(secondFmVersion, location->FMVersion);
        VERIFY_ARE_EQUAL(store.GetStoreVersion(), location->StoreVersion);

        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(ServiceOffline_Int64Key)
    {
        NamingUri name(L"ServiceOffline_Int64Key");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 10, 1, 1),
            0,
            1000,
            psd).IsSuccess());
        PartitionKey pk(0);
        InnerServiceOfflineTest(name, pk, psd);
    }

    BOOST_AUTO_TEST_CASE(ServiceOffline_SingletonKey)
    {
        NamingUri name(L"ServiceOffline_SingletonKey");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            psd).IsSuccess());
        PartitionKey pk;
        InnerServiceOfflineTest(name, pk, psd);
    }

    BOOST_AUTO_TEST_CASE(ServiceOffline_NamedKey)
    {
        NamingUri name(L"ServiceOffline_NamedKey");        
        
        vector<wstring> partitionNames;
        partitionNames.push_back(L"A");
        partitionNames.push_back(L"B");
        partitionNames.push_back(L"C");
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 3, 1, 1),
            partitionNames,
            psd).IsSuccess());
        PartitionKey pk(L"A");
        InnerServiceOfflineTest(name, pk, psd);
    }

    

    BOOST_AUTO_TEST_CASE(ResolveAllStateful_Int64Key)
    {
        NamingUri name(L"ResolveAllStateful_Int64Key");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 3, 3),
            0,
            100,
            psd).IsSuccess());
        
        PartitionKey pk(0);
        InnerResolveAllTest(name, pk, psd, true /*isStafeul*/);
    }

    BOOST_AUTO_TEST_CASE(ResolveAllStateful_SingletonKey)
    {
        NamingUri name(L"ResolveAllStateful_SingletonKey");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 3, 3),
            psd).IsSuccess());
        PartitionKey pk;
        InnerResolveAllTest(name, pk, psd, true /*isStafeul*/);
    }

    BOOST_AUTO_TEST_CASE(ResolveAllStateful_NamedKey)
    {
        NamingUri name(L"ResolveAllStateful_NamedKey");        
        vector<wstring> partitionNames;
        partitionNames.push_back(L"A");
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 3, 3),
            partitionNames,
            psd).IsSuccess());
        PartitionKey pk(L"A");
        InnerResolveAllTest(name, pk, psd, true /*isStafeul*/);
    }

    BOOST_AUTO_TEST_CASE(ResolveAllStateless_Int64Key)
    {
        NamingUri name(L"ResolveAllStateless_Int64Key");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 3, 3, false),
            0,
            100,
            psd).IsSuccess());
        
        PartitionKey pk(0);
        InnerResolveAllTest(name, pk, psd, false /*isStafeul*/);
    }

    BOOST_AUTO_TEST_CASE(ResolveAllStateless_SingletonKey)
    {
        NamingUri name(L"ResolveAllStateless_SingletonKey");        
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 3, 3, false),
            psd).IsSuccess());
        PartitionKey pk;
        InnerResolveAllTest(name, pk, psd, false /*isStafeul*/);
    }

    BOOST_AUTO_TEST_CASE(ResolveAllStateless_NamedKey)
    {
        NamingUri name(L"ResolveAllStateless_NamedKey");        
        vector<wstring> partitionNames;
        partitionNames.push_back(L"A");
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 3, 3, false),
            partitionNames,
            psd).IsSuccess());
        PartitionKey pk(L"A");
        InnerResolveAllTest(name, pk, psd, false /*isStafeul*/);
    }

   

    BOOST_AUTO_TEST_CASE(ParallelResolveWithDifferentPrevious)
    {
        Trace.WriteInfo(Constants::TestSource, "Start ParallelResolveWithDifferentPrevious");
        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);  
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        NamingUri name(L"ParallelResolveWithDifferentPrevious");
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 1, 1),
            0,
            100,
            psd).IsSuccess());
        PartitionKey pk(0);
        PartitionKey pk2(1);
        PartitionKey pk3(2);

        // create service
        SynchronousCreateName(client, name);
        SynchronousCreateService(client, psd, fm);
        
        // Resolve once to populate the gateway cache
        ResolvedServicePartitionSPtr location = SyncResolveService(client, name, pk, nullptr);
        ConsistencyUnitId cuid = location->Locations.ConsistencyUnitId;
        CheckResolvedServicePartition(location, fm, store, cuid);
        
        // update FM
        fm.IncrementCuidFMVersion(cuid);

        // Create another client that has 2 gateways: one is invalid, the other is valid
        // The client will first try the first gateway, timeout and move to the second, succeeding the operation.
        // This is done to delay the processing of the first naming operations.
        vector<wstring> listenAddresses2;
        listenAddresses2.push_back(L"127.0.0.1:2333");
        listenAddresses2.push_back(gatewayAddress);
        clientSPtr2_ = make_shared<FabricClientImpl>(move(listenAddresses2));
        FabricClientImpl & client2 = *clientSPtr2_;

        // Start multiple resolves in parallel, for different keys (of the same partition) and different previous
        // Validate that the previous values are respected.
        // Ensure that the primary that goes over the wire has no previous and that its processing takes some time,
        // so the next operations can link to it.

        // Linking chain:
        // Primary(PK)-NULL ; Secondary(PK)-location
        // Primary(PK2)-location
        // Primary(PK3)-NULL ; Secondary(PK3)-location

        // First start Primary(PK)-NULL and add delay before starting the others.
        // When it completes, it will complete Primary(PK3)-NULL only
        // (unless one of operations with non-null previous completes first,
        // due to PING timeout timing relative to starting the operations,
        // in which case all operations should complete with the newer result).
        // Then the others will complete with the updated result. 
        vector<pair<PartitionKey, ResolvedServicePartitionSPtr>> requests;
        requests.push_back(pair<PartitionKey, ResolvedServicePartitionSPtr>(pk, nullptr));
        requests.push_back(pair<PartitionKey, ResolvedServicePartitionSPtr>(pk, location));
        requests.push_back(pair<PartitionKey, ResolvedServicePartitionSPtr>(pk2, location));
        requests.push_back(pair<PartitionKey, ResolvedServicePartitionSPtr>(pk3, nullptr));
        requests.push_back(pair<PartitionKey, ResolvedServicePartitionSPtr>(pk3, location));

        atomic_uint64 ops(static_cast<uint64>(requests.size()));
        ManualResetEvent waitAll;

        // The first resolve has no previous, becomes primary and goes to the gateway to resolve;
        // it will return the cached gateway entry.
        bool first = true;
        for (auto it = requests.begin(); it != requests.end(); ++it)
        {
            ResolvedServicePartitionSPtr previous = it->second;
            bool hasPrevious = (previous != nullptr);
            BeginResolveServicePartition(
                client2,
                name,
                it->first,
                TimeSpan::FromSeconds(20),
                [this, &client2, &location, &ops, &waitAll, hasPrevious, first](AsyncOperationSPtr const & resolveOperation)
                {
                    ResolvedServicePartitionSPtr partition;
                    EndResolveServicePartition(resolveOperation, client2, /*out*/partition);
                    if (hasPrevious)
                    {
                        // The result should be newer than the first location
                        CompareResolvedServicePartitions(location, partition, -1);
                    }
                    else if (first)
                    {
                        // Either 0 or -1 are valid results, based on the timing between starting requests
                        // and ping timeout to the invalid gatewy
                        CompareResolvedServicePartitions(location, partition, 1, false);
                    }

                    if (--ops == 0)
                    {
                        waitAll.Set();
                    }
                },
                AsyncOperationSPtr(),
                move(previous));

            first = false;
            if (!previous)
            {
                // Delay just enough to ensure the operation without Previous starts processing first
                Sleep(100);
            }
        }

        VERIFY_IS_TRUE(waitAll.WaitOne(TimeSpan::FromSeconds(60)));
        
        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
        VERIFY_IS_TRUE(client2.Close().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(ResolvePartition)
    {
        // setup gateway
        wstring gatewayAddress;
        root_ = CreateRoot(gatewayAddress);  
        FauxStore store(NodeId(LargeInteger(0,1)));
        FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);        
        root_->router_.Initialize(store, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store.Id, store.Id);   

        // setup client
        vector<wstring> listenAddresses;
        listenAddresses.push_back(gatewayAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
        FabricClientImpl & client = *clientSPtr_;

        // create service
        NamingUri name(L"ResolvePartition");        
        SynchronousCreateName(client, name);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(name, 1, 3, 3, false),
            0,
            100,
            psd).IsSuccess());
        SynchronousCreateService(client, psd, fm);


        // resolve a partition location
        ResolvedServicePartitionSPtr location;
        ErrorCode error;
        AutoResetEvent wait;
        client.BeginResolvePartition(
            psd.Cuids[0], 
            TimeSpan::FromSeconds(20),
            [this, &client, &error, &location, &wait] (AsyncOperationSPtr const & operation)
        {
            error = client.EndResolvePartition(operation, location);
            wait.Set();
        },
        root_->CreateMultiRoot(client));

        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(30)));
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, error.ReadValue());
        int64 firstFMVersion = location->FMVersion;
        VERIFY_ARE_EQUAL(fm.GetFMVersion(), location->FMVersion);

        fm.IncrementFMVersion();

        client.BeginResolvePartition(
            psd.Cuids[0], 
            TimeSpan::FromSeconds(20),
            [this, &client, &error, &location, &wait] (AsyncOperationSPtr const & operation)
        {
            error = client.EndResolvePartition(operation, location);
            wait.Set();
        },
        root_->CreateMultiRoot(client));

        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(30)));
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, error.ReadValue());
        VERIFY_ARE_EQUAL(fm.GetFMVersion(), location->FMVersion);
        VERIFY_ARE_NOT_EQUAL(firstFMVersion, location->FMVersion);

        root_->Close();
        VERIFY_IS_TRUE(client.Close().IsSuccess());
    }

    BOOST_AUTO_TEST_SUITE_END()

	void CachingTest::InnerCacheTest(
		NamingUri const & name,
		PartitionKey const & partitionKey,
		PartitionedServiceDescriptor const & psd)
	{
			// setup gateway
			wstring gatewayAddress;
			root_ = CreateRoot(gatewayAddress);
			FauxStore store(NodeId(LargeInteger(0, 1)));
			FauxFM fm(root_->fsForFm_);
			fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
			root_->router_.Initialize(store, fm.Id, fm.Id);
			root_->router_.Initialize(fm, store.Id, store.Id);

			// setup client
			vector<wstring> listenAddresses;
			listenAddresses.push_back(gatewayAddress);
			clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
			FabricClientImpl & client = *clientSPtr_;

			// create service
			SynchronousCreateName(client, name);
			SynchronousCreateService(client, psd, fm, true);

			// view empty cache
			ResolvedServicePartitionSPtr location, location1;
			bool entryExists = client.Test_TryGetCachedServiceResolution(name, partitionKey, location);
			VERIFY_IS_FALSE(entryExists, L"Initial GetCachedServiceResolution");
			VERIFY_IS_TRUE(location == nullptr, L"GetCachedServiceResolution: no location");

			// do refresh
			location = SyncResolveService(client, name, partitionKey, nullptr /*previous*/);
			ConsistencyUnitId cuid = location->Locations.ConsistencyUnitId;
			CheckResolvedServicePartition(location, fm, store, cuid);

			// see if cache has updated
			ResolvedServicePartitionSPtr cacheLocation;
			entryExists = client.Test_TryGetCachedServiceResolution(name, partitionKey, cacheLocation);
			VERIFY_IS_TRUE(entryExists, L"GetCachedServiceResolution");
			CompareResolvedServicePartitions(cacheLocation, location, 0);

			// update FM version and send broadcast
			fm.IncrementCuidFMVersion(cuid);
			root_->ServiceResolverProcessWouldBeBroadcastMessage(cuid, name.ToString(), fm);

			// check there is no change in cached entry
			ResolvedServicePartitionSPtr cacheLocation2;
			entryExists = client.Test_TryGetCachedServiceResolution(name, partitionKey, cacheLocation2);
			VERIFY_IS_TRUE(entryExists, L"GetCachedServiceResolution");
			CompareResolvedServicePartitions(cacheLocation2, location, 0);

			root_->Close();
			VERIFY_IS_TRUE(client.Close().IsSuccess());
		}

	void CachingTest::InnerRefreshTest(
		NamingUri const & name,
		PartitionKey const & partitionKey,
		PartitionedServiceDescriptor const & psd)
	{
		// setup gateway
		wstring gatewayAddress;
		root_ = CreateRoot(gatewayAddress);
		FauxStore store(NodeId(LargeInteger(0, 1)));
		FauxFM fm(root_->fsForFm_);
		fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
		root_->router_.Initialize(store, fm.Id, fm.Id);
		root_->router_.Initialize(fm, store.Id, store.Id);

		// setup client
		vector<wstring> listenAddresses;
		listenAddresses.push_back(gatewayAddress);
		clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
		FabricClientImpl & client = *clientSPtr_;

		// create service
		SynchronousCreateName(client, name);
		SynchronousCreateService(client, psd, fm);

		// resolve once to populate cache
		ResolvedServicePartitionSPtr location = SyncResolveService(client, name, partitionKey, nullptr);
		ConsistencyUnitId partitionCuid = location->Locations.ConsistencyUnitId;
		CheckResolvedServicePartition(location, fm, store, partitionCuid);

		int64 firstFmVersion = location->FMVersion;
		int64 firstStoreVersion = location->StoreVersion;

		// refresh and verify same results
		location = SyncResolveService(client, name, partitionKey, location);
		CheckResolvedServicePartition(location, fm, store, partitionCuid);

		// update FM
		fm.IncrementCuidFMVersion(partitionCuid);

		// resolve and verify update
		ResolvedServicePartitionSPtr location2 = SyncResolveService(client, name, partitionKey, location/*previous*/);

		CheckResolvedServicePartition(location2, fm, store, partitionCuid);
		VERIFY_ARE_EQUAL(firstStoreVersion, store.GetStoreVersion());
		int64 secondFmVersion = location2->FMVersion;
		VERIFY_ARE_NOT_EQUAL(firstFmVersion, secondFmVersion);
		CompareResolvedServicePartitions(location2, location, 1);

		// update FM and broadcast
		fm.IncrementCuidFMVersion(partitionCuid);
		root_->ServiceResolverProcessWouldBeBroadcastMessage(partitionCuid, name.ToString(), fm);

		// update FM again with no broadcast (clientVersion < gatewayVersion < fmVersion)
		fm.IncrementCuidFMVersion(partitionCuid);

		// refresh and verify returned data matches gateway's cache, not the FM's entry
		ResolvedServicePartitionSPtr location3 = SyncResolveService(client, name, partitionKey, location2 /*previous*/);
		CompareResolvedServicePartitions(location2, location3, -1);

		VERIFY_ARE_EQUAL(partitionCuid, location3->Locations.ConsistencyUnitId);
		int64 thirdFmVersion = location3->FMVersion;
		VERIFY_ARE_NOT_EQUAL(secondFmVersion, thirdFmVersion);
		VERIFY_ARE_NOT_EQUAL(fm.GetCuidFMVersion(partitionCuid), thirdFmVersion);
		VERIFY_ARE_EQUAL(firstStoreVersion, location->StoreVersion);

		// refresh and verify returned data matches FM's entry
		ResolvedServicePartitionSPtr location4 = SyncResolveService(client, name, partitionKey, location3 /*previous*/);
		CheckResolvedServicePartition(location4, fm, store, partitionCuid);
		CompareResolvedServicePartitions(location4, location3, 1);

		root_->Close();
		VERIFY_IS_TRUE(client.Close().IsSuccess());
	}

	void CachingTest::InnerDeleteServiceClearing(
		NamingUri const & name,
		PartitionKey const & partitionKey,
		PartitionedServiceDescriptor const & psd,
		PartitionedServiceDescriptor const & psd2)
	{
		// setup gateway
		wstring gatewayAddress;
		root_ = CreateRoot(gatewayAddress);
		FauxStore store(NodeId(LargeInteger(0, 1)));
		FauxFM fm(root_->fsForFm_);
		fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
		root_->router_.Initialize(store, fm.Id, fm.Id);
		root_->router_.Initialize(fm, store.Id, store.Id);

		// setup client
		vector<wstring> listenAddresses;
		listenAddresses.push_back(gatewayAddress);
		clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
		FabricClientImpl & client = *clientSPtr_;

		// create service
		SynchronousCreateName(client, name);
		SynchronousCreateService(client, psd, fm);

		// resolve once to populate cache
		ResolvedServicePartitionSPtr location = SyncResolveService(client, name, partitionKey, nullptr);
		ConsistencyUnitId partitionCuid = location->Locations.ConsistencyUnitId;
		CheckResolvedServicePartition(location, fm, store, partitionCuid);
		int64 firstFmVersion = location->FMVersion;
		int64 firstStoreVersion = location->StoreVersion;

		// increase store and fm versions
		store.IncrementStoreVersion();
		fm.IncrementCuidFMVersion(partitionCuid);

		// delete service        
		SynchronousDeleteService(client, name, fm);

		// try to get cached data and fail
		bool entryExists = client.Test_TryGetCachedServiceResolution(name, partitionKey, location);
		VERIFY_IS_FALSE(entryExists, L"Initial GetCachedServiceResolution");

		// re-resolve and fail
		location = SyncResolveService(client, name, partitionKey, nullptr, ErrorCodeValue::UserServiceNotFound);

		// create the service again and resolve
		SynchronousCreateService(client, psd2, fm);
		location = SyncResolveService(client, name, partitionKey, nullptr);
		ConsistencyUnitId partitionCuid2 = location->Locations.ConsistencyUnitId;
		CheckResolvedServicePartition(location, fm, store, partitionCuid2);

		VERIFY_ARE_NOT_EQUAL(partitionCuid, partitionCuid2);
		int64 secondFmVersion = location->FMVersion;
		VERIFY_ARE_NOT_EQUAL(firstFmVersion, secondFmVersion);
		int64 secondStoreVersion = location->StoreVersion;
		VERIFY_ARE_NOT_EQUAL(firstStoreVersion, secondStoreVersion);

		root_->Close();
		VERIFY_IS_TRUE(client.Close().IsSuccess());
	}

	void CachingTest::InnerServiceOfflineTest(
		NamingUri const & name,
		PartitionKey const & partitionKey,
		PartitionedServiceDescriptor const & psd)
	{
			// setup gateway
			wstring gatewayAddress;
			root_ = CreateRoot(gatewayAddress);
			FauxStore store(NodeId(LargeInteger(0, 1)));
			FauxFM fm(root_->fsForFm_);
			fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
			root_->router_.Initialize(store, fm.Id, fm.Id);
			root_->router_.Initialize(fm, store.Id, store.Id);

			// setup client
			vector<wstring> listenAddresses;
			listenAddresses.push_back(gatewayAddress);
			clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
			FabricClientImpl & client = *clientSPtr_;

			// create service without adding it to the FM
			SynchronousCreateName(client, name);
			SynchronousCreateService(client, psd, fm, false);
			fm.DirectlyAddService(psd.Service, psd.Cuids, false);

			// resolve locations - should be empty
			SyncResolveService(client, name, partitionKey, nullptr, ErrorCodeValue::ServiceOffline);

			// "place" the service
			fm.IncrementFMVersion();
			fm.PlaceCuids(psd.Cuids);

			// resolve again        
			ResolvedServicePartitionSPtr location = SyncResolveService(client, name, partitionKey, nullptr);
			ConsistencyUnitId partitionCuid = location->Locations.ConsistencyUnitId;
			CheckResolvedServicePartition(location, fm, store, partitionCuid);

			root_->Close();
			VERIFY_IS_TRUE(client.Close().IsSuccess());
	}

	void CachingTest::InnerResolveAllTest(
		NamingUri const & name,
		PartitionKey const & partitionKey,
		PartitionedServiceDescriptor const & psd,
		bool isStateful)
	{
		// setup gateway
		wstring gatewayAddress;
		root_ = CreateRoot(gatewayAddress);
		FauxStore store(NodeId(LargeInteger(0, 1)));
		FauxFM fm(root_->fsForFm_);
		fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
		root_->router_.Initialize(store, fm.Id, fm.Id);
		root_->router_.Initialize(fm, store.Id, store.Id);

		// setup client
		vector<wstring> listenAddresses;
		listenAddresses.push_back(gatewayAddress);
		clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
		FabricClientImpl & client = *clientSPtr_;

		// create service
		SynchronousCreateName(client, name);
		SynchronousCreateService(client, psd, fm);

		// update FM and broadcast, so secondary locations are set to known values
		fm.IncrementFMVersion();

		ResolvedServicePartitionSPtr location = SyncResolveService(client, name, partitionKey, nullptr);
		CheckLocation(location, isStateful);

		location = SyncResolveService(client, name, partitionKey, location);
		CheckLocation(location, isStateful);

		VERIFY_IS_TRUE(client.Close().IsSuccess());

		// setup another client
		listenAddresses.push_back(gatewayAddress);
		clientSPtr2_ = make_shared<FabricClientImpl>(move(listenAddresses));
		FabricClientImpl & client2 = *clientSPtr2_;

		location = SyncResolveService(client2, name, partitionKey, location);
		CheckLocation(location, isStateful);

		location = SyncResolveService(client2, name, partitionKey, nullptr);
		CheckLocation(location, isStateful);

		root_->Close();
		VERIFY_IS_TRUE(client2.Close().IsSuccess());
	}
	void CachingTest::InnerNotificationBasicTest(
		NamingUri const & name,
		PartitionedServiceDescriptor const & psd,
		NamingUri const & name2,
		PartitionKey const & partitionKey2,
		PartitionedServiceDescriptor const & psd2)
	{
			// setup gateway
			wstring gatewayAddress;
			root_ = CreateRoot(gatewayAddress);
			FauxStore store(NodeId(LargeInteger(0, 1)));
			FauxFM fm(root_->fsForFm_);
			fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
			root_->router_.Initialize(store, fm.Id, fm.Id);
			root_->router_.Initialize(fm, store.Id, store.Id);

			// setup client
			vector<wstring> listenAddresses;
			listenAddresses.push_back(gatewayAddress);
			clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
			FabricClientImpl & client = *clientSPtr_;

			// create services
			SynchronousCreateName(client, name);
			SynchronousCreateService(client, psd, fm);

			SynchronousCreateName(client, name2);
			SynchronousCreateService(client, psd2, fm);

			vector<ResolvedServicePartitionSPtr> results;
			vector<ServiceLocationNotificationRequestData> requests;
			vector<AddressDetectionFailureSPtr> failures;
			requests.push_back(CreateRequestData(name));
			requests.push_back(CreateRequestData(name2, partitionKey2));

			// CHeck that there is no entry in the client cache
			ResolvedServicePartitionSPtr location;
			bool entryExists = client.Test_TryGetCachedServiceResolution(name2, partitionKey2, location);
			VERIFY_IS_FALSE(entryExists, L"Initial GetCachedServiceResolution");
			VERIFY_IS_TRUE(location == nullptr, L"GetCachedServiceResolution: no location");

			ConsistencyUnitId cuid;
			psd2.TryGetCuid(partitionKey2, cuid);

			// Broadcast just the change of the cuid;
			// The gateway should detect missing broadcast and ask for the rest
			int64 startVersion = fm.GetCuidFMVersion(cuid);
			fm.IncrementCuidFMVersion(cuid);
			root_->ServiceResolverProcessWouldBeBroadcastMessage(cuid, name2.ToString(), fm, startVersion, fm.GetFMVersion());

			AutoResetEvent wait;
			StartGetNotifications(client, wait, requests, ErrorCodeValue::Success, results, failures);
			FinishWait(wait, TimeSpan::FromSeconds(20));

			// Receive notifications for *all* partitions of the first service
			// and only the requested partition of the second
			map<ConsistencyUnitId, ServiceLocationVersion> previousResults;
			vector<NamingUri> serviceNames;
			size_t servicePartitionCount = psd.Cuids.size();
			size_t partitionCount = servicePartitionCount + 1;

			Trace.WriteInfo(Constants::TestSource, "Check 1: rsps={0}", results);
			CheckResolvedServicePartition(results, partitionCount, fm, store, serviceNames, previousResults);

			size_t notifNameCount = 0;
			size_t notifName2Count = 0;
			for (auto const & s : serviceNames)
			{
				if (s == name)
				{
					++notifNameCount;
				}
				else if (s == name2)
				{
					++notifName2Count;
				}
				else
				{
					VERIFY_FAIL(L"The notifications should be for the requested services");
				}
			}

			VERIFY_ARE_EQUAL(servicePartitionCount, notifNameCount, L"Expected number of notif received for first service");
			VERIFY_ARE_EQUAL(1u, notifName2Count, L"Expected number of notif received for second service");

			// Start next poll, passing previous results
			auto it = previousResults.find(cuid);
			VERIFY_IS_TRUE(it != previousResults.end(), L"cuid found in previous results");
			map<ConsistencyUnitId, ServiceLocationVersion> previousResults2;
			previousResults2[cuid] = it->second;
			previousResults.erase(it);

			vector<ServiceLocationNotificationRequestData> requests2;
			requests2.push_back(CreateRequestData(name, previousResults));
			requests2.push_back(CreateRequestData(name2, previousResults2, partitionKey2));

			StartGetNotifications(client, wait, requests2, ErrorCodeValue::Success, results, failures);

			fm.IncrementCuidFMVersion(cuid);
			VERIFY_IS_TRUE(root_->WaitForNotificationRegisteredForBroadcastUpdates(name2, cuid));
			root_->ServiceResolverProcessWouldBeBroadcastMessage(cuid, name2.ToString(), fm);

			FinishWait(wait, TimeSpan::FromSeconds(20));

			VERIFY_ARE_EQUAL(1U, results.size());

			Trace.WriteInfo(Constants::TestSource, "Check 2: rsps={0}", results);
			CheckResolvedServicePartition(results[0], fm, store, cuid);

			// The client cache should be updated with the new entry
			entryExists = client.Test_TryGetCachedServiceResolution(name2, partitionKey2, location);
			VERIFY_IS_TRUE(entryExists, L"GetCachedServiceResolution");
			CompareResolvedServicePartitions(location, results[0], 0);

			// increment FM version, send broadcast and resolve to bring in the newer result
			fm.IncrementCuidFMVersion(cuid);
			root_->ServiceResolverProcessWouldBeBroadcastMessage(cuid, name.ToString(), fm);
			ResolvedServicePartitionSPtr location2 = SyncResolveService(client, name2, partitionKey2, location /*previous*/);
			entryExists = client.Test_TryGetCachedServiceResolution(name2, partitionKey2, location2);
			VERIFY_IS_TRUE(entryExists, L"GetCachedServiceResolution");
			CompareResolvedServicePartitions(results[0], location2, -1);

			map<ConsistencyUnitId, ServiceLocationVersion> previous;
			previous[cuid] = ServiceLocationVersion(results[0]->FMVersion, results[0]->Generation, results[0]->StoreVersion);
			vector<ServiceLocationNotificationRequestData> requests3;
			requests3.push_back(CreateRequestData(name2, previous, partitionKey2));

			// Notification should return the new result, cached on the client
			StartGetNotifications(client, wait, requests3, ErrorCodeValue::Success, results, failures);
			FinishWait(wait, TimeSpan::FromSeconds(20));

			VERIFY_ARE_EQUAL(1U, results.size());
			CompareResolvedServicePartitions(results[0], location2, 0);

			root_->Close();
			VERIFY_IS_TRUE(client.Close().IsSuccess());
		}
		void CachingTest::InnerReplacingEmptySTEs(
			NamingUri const & name,
			PartitionKey const & partitionKey,
			PartitionedServiceDescriptor const & psd)
		{
			// setup gateway1
			wstring gatewayAddress;
			root_ = CreateRoot(gatewayAddress);
			FauxStore store(NodeId(LargeInteger(0, 1)));
			FauxFM fm(root_->fsForFm_);
			fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
			root_->router_.Initialize(store, fm.Id, fm.Id);
			root_->router_.Initialize(fm, store.Id, store.Id);

			// setup client1
			vector<wstring> listenAddresses;
			listenAddresses.push_back(gatewayAddress);
			shared_ptr<FabricClientImpl> clientSPtr = make_shared<FabricClientImpl>(move(listenAddresses));
			FabricClientImpl & client = *clientSPtr;

			// setup gateway2
			wstring gatewayAddress2(L"127.0.0.1:12351");
			RootSPtr root2 = CreateRoot(gatewayAddress2);
			root2->router_.Initialize(store, fm.Id, fm.Id);
			root2->router_.Initialize(fm, store.Id, store.Id);

			// setup client2
			vector<wstring> listenAddresses2;
			listenAddresses2.push_back(gatewayAddress2);
			shared_ptr<FabricClientImpl> clientSPtr2 = make_shared<FabricClientImpl>(move(listenAddresses2));
			FabricClientImpl & client2 = *clientSPtr2;

			// create service with client1
			SynchronousCreateName(client, name);
			SynchronousCreateService(client, psd, fm);

			// Client1 will make a service-resolution through Gateway1 & Client2 will make a service-resolution with Gateway2. So both of them will have the same entries.
			ResolvedServicePartitionSPtr location = SyncResolveService(client, name, partitionKey, nullptr);
			ConsistencyUnitId partitionCuid = location->Locations.ConsistencyUnitId;
			CheckResolvedServicePartition(location, fm, store, partitionCuid);

			ResolvedServicePartitionSPtr location2 = SyncResolveService(client2, name, partitionKey, nullptr);
			CompareResolvedServicePartitions(location, location2, 0);

			// Client1 will make delete-service request from Gateway1. So the caches will be deleted from gateway1/Client1 and the StoreService
			SynchronousDeleteService(client, name, fm);

			// FM will send a broadcast message for this service to all the nodes. 
			// PSD is removed from StoreService due to the DeleteService.
			// PSD is removed from Gateway1 due to the DeleteService.
			// PSD will exist in Gateway2 until FM broadcasts the delete.
			//
			Trace.WriteInfo(Constants::TestSource, "Broadcasting live entries");

			for (auto const & cuid : psd.Cuids)
			{
				fm.IncrementCuidFMVersion(cuid);
			}

			root_->ServiceResolverProcessWouldBeBroadcastMessage(psd.Cuids, name.ToString(), fm);
			root2->ServiceResolverProcessWouldBeBroadcastMessage(psd.Cuids, name.ToString(), fm);

			Trace.WriteInfo(Constants::TestSource, "Resolving after broadcasted live entry");

			SyncResolveService(client, name, partitionKey, location, ErrorCodeValue::UserServiceNotFound);
			ResolvedServicePartitionSPtr location3 = SyncResolveService(client2, name, partitionKey, location2);
			CheckResolvedServicePartition(location3, fm, store, partitionCuid);

			// Once FM broadcasts the delete, then the PSD is removed from Gateway2 as well.

			Trace.WriteInfo(Constants::TestSource, "Broadcasting deletes");

			for (auto const & cuid : psd.Cuids)
			{
				fm.IncrementCuidFMVersion(cuid);
			}


			root_->ServiceResolverProcessWouldBeBroadcastMessage(psd.Cuids, name.ToString(), fm, L"", vector<wstring>());
			root2->ServiceResolverProcessWouldBeBroadcastMessage(psd.Cuids, name.ToString(), fm, L"", vector<wstring>());

			Trace.WriteInfo(Constants::TestSource, "Resolving after broadcasted delete");

			SyncResolveService(client, name, partitionKey, nullptr, ErrorCodeValue::UserServiceNotFound);
			SyncResolveService(client2, name, partitionKey, location3, ErrorCodeValue::UserServiceNotFound);
			SyncResolveService(client2, name, partitionKey, nullptr, ErrorCodeValue::UserServiceNotFound);

			root2->Close();
			root_->Close();
			VERIFY_IS_TRUE(client.Close().IsSuccess());
			VERIFY_IS_TRUE(client2.Close().IsSuccess());
		}
    bool CachingTest::MethodSetup()
    {
        Config cfg;
        return true;
    }

    bool CachingTest::MethodCleanup()
    {
        if (root_)
        {
            if (root_->entreeService_->State.Value != FabricComponentState::Closed)
            {
                VERIFY_IS_TRUE(root_->entreeService_->Close().IsSuccess());
            }

            root_.reset();
        }

        if (clientSPtr_)
        {
            if (clientSPtr_->State.Value != FabricComponentState::Closed)
            {
                VERIFY_IS_TRUE(clientSPtr_->Close().IsSuccess());
            }

            clientSPtr_.reset();
        }

        if (clientSPtr2_)
        {
            if (clientSPtr2_->State.Value != FabricComponentState::Closed)
            {
                VERIFY_IS_TRUE(clientSPtr2_->Close().IsSuccess());
            }

            clientSPtr2_.reset();
        }

        NamingConfig::GetConfig().Test_Reset();
        ClientConfig::GetConfig().Test_Reset();
        ServiceModelConfig::GetConfig().Test_Reset();

        return true;
    }

    RootSPtr CachingTest::CreateRoot(wstring & gatewayAddress)
    {
        USHORT basePort = 0;
        TestPortHelper::GetPorts(5, basePort);
        wstring addr1 = wformatString("127.0.0.1:{0}", basePort++);
        wstring addr2 = wformatString("127.0.0.1:{0}", basePort++);
        wstring addr3 = wformatString("127.0.0.1:{0}", basePort++);
        wstring addr4 = wformatString("127.0.0.1:{0}", basePort++);
        gatewayAddress = addr3;
        NodeId entreeId(LargeInteger(0,1));
        NodeConfig nodeConfig(entreeId, addr1, addr2, L"");
        NodeConfig fmConfig(NodeId::MinNodeId, addr1, addr2, L"");
        return GatewayRoot::Create(nodeConfig, addr3, addr4, fmConfig);
    }


    ServiceDescription CachingTest::CreateServiceDescription(
        NamingUri const & name,
        int partitionCount,
        int replicaCount,
        int minWrite,
        bool isStateful)
    {
        return ServiceDescription(
            name.ToString(),
            0,
            0,
            partitionCount, 
            replicaCount, 
            minWrite, 
            isStateful,
            false,  // has persisted
            TimeSpan::Zero,
            TimeSpan::Zero,
            TimeSpan::Zero,
            ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(), L"TestType"), 
            vector<Reliability::ServiceCorrelationDescription>(),
            L"",    // placement constraints
            0, 
            vector<Reliability::ServiceLoadMetricDescription>(),
            0,
            vector<byte>());
    }

    void CachingTest::SynchronousCreateName(__in FabricClientImpl & client, NamingUri const & nameToTest)
    {
        AutoResetEvent wait;
        auto ptr = client.BeginCreateName(
            nameToTest,
            TimeSpan::FromSeconds(20),
            [this, &wait] (AsyncOperationSPtr const &)
        {
            wait.Set();
        },
        root_->CreateMultiRoot(client));
        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(30)), L"CreateName");

        ErrorCode error = client.EndCreateName(ptr);
        Trace.WriteInfo(Constants::TestSource, "EndCreateName returned error {0}", error);
		VERIFY_IS_TRUE_FMT(error.IsSuccess(), "SynchronousCreateName {0}", nameToTest.Name.c_str());
    }

    void CachingTest::SynchronousCreateService(__in FabricClientImpl & client, PartitionedServiceDescriptor const & psd, __in FauxFM & fm, bool placeServiceInFM)
    {
        AutoResetEvent wait;
        ServiceModel::PartitionedServiceDescWrapper wrapper;
        psd.ToWrapper(wrapper);
        auto ptr = client.BeginInternalCreateService(
            psd,
            ServiceModel::ServicePackageVersion(),
            0,            
            TimeSpan::FromSeconds(20),
            [this, &wait] (AsyncOperationSPtr const &)
        {                
            wait.Set();
        },
        root_->CreateMultiRoot(client));
        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(30)), L"CreateService");

        ErrorCode error = client.EndCreateService(ptr);
        Trace.WriteInfo(Constants::TestSource, "EndCreateService returned error {0}.", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"SynchronousCreateService");
        fm.DirectlyAddService(psd.Service, psd.Cuids, placeServiceInFM);
    }

    void CachingTest::SynchronousDeleteService(__in FabricClientImpl & client, NamingUri const & name, __in FauxFM & fm)
    {
        AutoResetEvent wait;
        auto ptr = client.BeginInternalDeleteService(
            ServiceModel::DeleteServiceDescription(name),
            ActivityId(Guid::NewGuid()),
            TimeSpan::FromSeconds(20),
            [this, &wait] (AsyncOperationSPtr const &)
        {                
            wait.Set();
        },
        root_->CreateMultiRoot(client));
        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(30)), L"DeleteService");

        bool unused;
        ErrorCode error = client.EndInternalDeleteService(ptr, unused);
        Trace.WriteInfo(Constants::TestSource, "EndDeleteService returned error {0}.", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"SynchronousDeleteService");
        fm.DirectlyRemoveService(name.ToString());
    }

    void CachingTest::GetNotificationsInBatches(
        __in FabricClientImpl & client,
        vector<ServiceLocationNotificationRequestData> const & requests,
        ErrorCodeValue::Enum const expectedError,
        int minExpectedMessageCount,
        __out vector<ResolvedServicePartitionSPtr> & results,
        __out vector<AddressDetectionFailureSPtr> & failures)
    {
        Trace.WriteInfo("CachingTestSource", "Register for notifications for {0} requests", requests.size());

        // Maintain a copy of the requests to compute future requests based on it
        vector<ServiceLocationNotificationRequestData> sentRequests(requests);
        results.clear();
        failures.clear();
        std::unique_ptr<NameRangeTuple> firstNonProcessedRequest = nullptr;
        int maxRetry = minExpectedMessageCount + 20;
        int messageCount = -1;

        AutoResetEvent wait;
        for (int opIndex = 0; opIndex < maxRetry; ++opIndex)
        {
            Trace.WriteInfo("CachingTestSource", "Start message {0}", opIndex);
            wait.Reset();
            
            vector<ResolvedServicePartitionSPtr> partialResults;
            vector<AddressDetectionFailureSPtr> partialFailures;
            bool updateFirstNonProcessedRequest;
            vector<ServiceLocationNotificationRequestData> copy(sentRequests);
            auto operation = client.BeginInternalLocationChangeNotificationRequest(
                std::move(copy), 
                std::move(FabricActivityHeader()), 
                [&] (AsyncOperationSPtr const & op)
                {
                    FabricActivityHeader activityId;
                    ErrorCode error = client.EndInternalLocationChangeNotificationRequest(op, partialResults, partialFailures, updateFirstNonProcessedRequest, firstNonProcessedRequest, activityId);
                    VERIFY_ARE_EQUAL_FMT(
                        static_cast<int>(expectedError), 
                        error.ReadValue(),
                        "{0}: EndGetNotification returned {1}, expected {2}", 
                        "{3}: EndGetNotification returned {4}, expected {5}", 
                            activityId.Guid.ToString().c_str(),
                            error.ErrorCodeValueToString().c_str(), 
                            ErrorCode(expectedError).ErrorCodeValueToString().c_str());

                    wait.Set();
                }, 
                root_->CreateMultiRoot(client));

            auto result = wait.WaitOne(30000);
			VERIFY_IS_TRUE_FMT(result, "Notification wait expected success, actual {0}", result);

            for (auto it = partialResults.begin(); it != partialResults.end(); ++it)
            {
                results.push_back(move(*it));
            }

            for (auto it = partialFailures.begin(); it != partialFailures.end(); ++it)
            {
                failures.push_back(move(*it));
            }

            if (firstNonProcessedRequest)
            {
                // Search for the first non-processed request and start enumerating from there
                Trace.WriteInfo(Constants::TestSource, "Search requests for firstNonProcessed={0}", *firstNonProcessedRequest);
                VERIFY_IS_TRUE(updateFirstNonProcessedRequest, L"UpdateFirstNonProcessedRequest should be true");
                bool found = false;
                size_t i = 0;
                while(i < requests.size() && (!found))
                {
                    if (requests[i].Name == firstNonProcessedRequest->Name && 
                        firstNonProcessedRequest->Info.RangeContains(requests[i].Partition))
                    {
                        found = true;
                    }
                    else
                    {
                        ++i;
                    }
                }

                VERIFY_IS_TRUE(found, L"Previous firstNonProcessedRequest is found");

                // Copy items fron here to the end
                // Then from the beginning to this point (using previous returned results, to ask for newer results)
                Trace.WriteInfo("CachingTestSource", "Build request: count={0}, firstNonProcessed at index {1}", requests.size(), i);

                vector<ServiceLocationNotificationRequestData> newRequests;
                size_t j = i;
                
                do
                {
                    if (j == sentRequests.size())
                    {
                        // start from the beginning
                        j = 0;
                    }
                
                    // Get the previous result, if any
                    auto const & initialRequest = sentRequests[j];
                    auto itResults = find_if(results.begin(), results.end(), 
                        [&initialRequest](ResolvedServicePartitionSPtr const & rsp) -> bool
                        { 
                            NamingUri name;
                            return (rsp->TryGetName(name)) && (name == initialRequest.Name) && 
                                rsp->PartitionData.RangeContains(initialRequest.Partition);
                        });

                    if (itResults != results.end())
                    {
                        Trace.WriteInfo(Constants::TestSource, "{0}: Add {1} - previous RSP", j, initialRequest);
                        auto rsp = *itResults;
                        map<ConsistencyUnitId, ServiceLocationVersion> previousResults;
                        previousResults[rsp->Locations.ConsistencyUnitId] = ServiceLocationVersion(rsp->FMVersion, rsp->Generation, rsp->StoreVersion);
                        newRequests.push_back(CreateRequestData(initialRequest.Name, previousResults, initialRequest.Partition));                            
                    }
                    else
                    {
                        auto itFailures = find_if(failures.begin(), failures.end(), 
                            [&initialRequest](AddressDetectionFailureSPtr const & failure) 
                            { 
                                return (failure->PartitionData == initialRequest.Partition) && (failure->Name == initialRequest.Name);
                            });

                        if (itFailures != failures.end())
                        {
                            Trace.WriteInfo(Constants::TestSource, "{0}: Add {1} - previous failure", j, initialRequest);
                            newRequests.push_back(CreateRequestData(initialRequest.Name, *itFailures, initialRequest.Partition));
                        }
                        else
                        {
                            Trace.WriteInfo(Constants::TestSource, "{0}: Add {1} - no previous", j, initialRequest);
                            newRequests.push_back(initialRequest);
                        }
                    }

                    ++j;
                }
                while (j != i);

                swap(sentRequests, newRequests);
                if (sentRequests.size() != requests.size())
                {
                    VERIFY_FAIL_FMT("Not all requests sent {0} != {1}", sentRequests.size(), requests.size());
                }
            }
            else /*firstNonProcessedRequest == nullptr*/
            {
                messageCount = opIndex + 1;
				VERIFY_IS_TRUE_FMT(result, "Null firstProcessedRequest. Message count = {0}", messageCount);
                break;
            }
        }

		VERIFY_IS_TRUE_FMT(messageCount > 0 && messageCount < maxRetry, "NotificationOperationCount is in limits -- MessageCount {0}, MaxRetry {1}", messageCount, maxRetry);
        VERIFY_IS_TRUE(messageCount >= minExpectedMessageCount, L"There should be multiple messages sent to resolve all entries");
    }

    void CachingTest::StartGetNotifications(
        __in FabricClientImpl & client,
        __in AutoResetEvent & wait,
        vector<ServiceLocationNotificationRequestData> const & requests,
        ErrorCodeValue::Enum const expectedError,
        __out vector<ResolvedServicePartitionSPtr> & results,
        __out vector<AddressDetectionFailureSPtr> & failures)
    {
        vector<ServiceLocationNotificationRequestData> copy(requests);
        results.clear();
        failures.clear();
        auto operation = client.BeginInternalLocationChangeNotificationRequest(
            std::move(copy), 
            std::move(FabricActivityHeader()), 
            [this, &client, &results, &failures, expectedError, &wait] (AsyncOperationSPtr const & op)
            {
                FabricActivityHeader activityId;
                std::unique_ptr<NameRangeTuple> firstNonProcessedRequest;
                bool useCached;
                ErrorCode error = client.EndInternalLocationChangeNotificationRequest(op, results, failures, useCached, firstNonProcessedRequest, activityId);
                VERIFY_ARE_EQUAL_FMT(
                    static_cast<int>(expectedError), 
                    error.ReadValue(),
                    "{0}: EndGetNotification returned {1}, expected {2}", 
                    "{3}: EndGetNotification returned {4}, expected {5}", 
                        activityId.Guid.ToString().c_str(),
                        error.ErrorCodeValueToString().c_str(), 
                        ErrorCode(expectedError).ErrorCodeValueToString());

                VERIFY_IS_TRUE(
                    firstNonProcessedRequest == nullptr, 
                    L"EndGetNotification returned empty firstNonProcessedRequest");
                wait.Set();
            }, 
            root_->CreateMultiRoot(client));
    }
    
    void CachingTest::FinishWait(
        __in AutoResetEvent & wait,
        TimeSpan const timeout, 
        bool success)
    {
        auto result = wait.WaitOne(timeout);
        
        VERIFY_ARE_EQUAL_FMT(
            success, 
            result, 
            "Notification wait: actual {0}, expected {1}", result, success);
    }

    AsyncOperationSPtr CachingTest::BeginResolveServicePartition(
        __in FabricClientImpl & client,
        NamingUri const & name, 
        PartitionKey const & partitionKey, 
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root,
        ResolvedServicePartitionSPtr const & previous)
    {
        ResolvedServicePartitionSPtr copy = previous;

        ResolvedServicePartitionResultSPtr previousSPtr = 
            make_shared<ResolvedServicePartitionResult>(copy);
        Api::IResolvedServicePartitionResultPtr previousPtr = RootedObjectPointer<Api::IResolvedServicePartitionResult>(
            previousSPtr.get(),
            previousSPtr->CreateComponentRoot());

        return client.BeginResolveServicePartition(
            name,
            ServiceResolutionRequestData(partitionKey), 
            previousPtr,
            timeout, 
            callback, 
            root);
    }

    void CachingTest::EndResolveServicePartition(
        AsyncOperationSPtr const & resolveOperation,
        __in FabricClientImpl & client,
        __out ResolvedServicePartitionSPtr & partition,
        ErrorCodeValue::Enum expected)
    {
        Api::IResolvedServicePartitionResultPtr resultPtr; 
        ErrorCode error = client.EndResolveServicePartition(resolveOperation, resultPtr);
        VERIFY_ARE_EQUAL(expected, error.ReadValue(), L"EndResolveServicePartition");

        if (error.IsSuccess())
        {
            if (resultPtr.get())
            {
                ResolvedServicePartitionSPtr resultSPtr;
                partition = ((ResolvedServicePartitionResult*)(resultPtr.get()))->ResolvedServicePartition;
            }
        }
    }

    ResolvedServicePartitionSPtr CachingTest::SyncResolveService(
        __in FabricClientImpl & client,
        NamingUri const & name, 
        PartitionKey const & partitionKey, 
        ResolvedServicePartitionSPtr const & previous,
        ErrorCodeValue::Enum expected)
    {
        AutoResetEvent wait;
        ResolvedServicePartitionSPtr partition;
        BeginResolveServicePartition(
            client, 
            name, 
            partitionKey,
            TimeSpan::FromSeconds(20),
            [this, &client, &partition, expected, &wait] (AsyncOperationSPtr const & resolveOperation)
        {
            EndResolveServicePartition(resolveOperation, client, partition, expected);
            wait.Set();
        },
            AsyncOperationSPtr(),
            previous);

        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(30)));
        return partition;
    }

    void CachingTest::CheckResolvedServicePartition(
        vector<ResolvedServicePartitionSPtr> const & partitions,
        size_t expectedCount,
        __in FauxFM & fm,
        __in FauxStore & store,
        __out vector<NamingUri> & serviceNames,
        __out map<ConsistencyUnitId, ServiceLocationVersion> & serviceLocations)
    {
        VERIFY_ARE_EQUAL(partitions.size(), expectedCount);
        for (ResolvedServicePartitionSPtr const & result : partitions)
        {
            auto cuid = result->Locations.ConsistencyUnitId;
            CheckResolvedServicePartition(result, fm, store, cuid);
            NamingUri serviceName;
            VERIFY_IS_TRUE(result->TryGetName(serviceName));
            serviceNames.push_back(move(serviceName));
            serviceLocations[cuid] = ServiceLocationVersion(result->FMVersion, result->Generation, result->StoreVersion);
        }
    }

    void CachingTest::CheckResolvedServicePartition(
        ResolvedServicePartitionSPtr const & location,
        __in FauxFM & fm,
        __in FauxStore & store,
        ConsistencyUnitId const & cuid)
    {
        VERIFY_ARE_EQUAL(fm.GetCuidFMVersion(cuid), location->FMVersion);
        VERIFY_ARE_EQUAL(store.GetStoreVersion(), location->StoreVersion);
        VERIFY_ARE_EQUAL(cuid, location->Locations.ConsistencyUnitId);
    }

    void CachingTest::CheckResolvedServicePartition(
        ResolvedServicePartitionSPtr const & location,
        __in FauxFM & fm,
        int64 storeVersion,
        ConsistencyUnitId const & cuid)
    {
        VERIFY_ARE_EQUAL(fm.GetCuidFMVersion(cuid), location->FMVersion);
        VERIFY_ARE_EQUAL(storeVersion, location->StoreVersion);
        VERIFY_ARE_EQUAL(cuid, location->Locations.ConsistencyUnitId);
    }

    void CachingTest::CheckLocation(ResolvedServicePartitionSPtr const & location, bool isStateful)
    {
        if (isStateful)
        {
            VERIFY_IS_TRUE(location->Locations.ServiceReplicaSet.IsPrimaryLocationValid);
        }
        else
        {
            VERIFY_IS_FALSE(location->IsStateful);
        }
    }

    void CachingTest::CompareResolvedServicePartitions(
        ResolvedServicePartitionSPtr const & left,
        ResolvedServicePartitionSPtr const & right,
        LONG expectedResult, 
        bool expectedEqual)
    {
        LONG compareResult;
        VERIFY_IS_TRUE(left->CompareVersion(right, compareResult).IsSuccess(), L"Compare rsps");

        bool actualEqual = (expectedResult == compareResult);
        VERIFY_ARE_EQUAL_FMT(
            expectedEqual,
            actualEqual, 
            "Compare RSPs: expected {0}, actual {1}, expected equal = {2}", expectedResult, compareResult, expectedEqual);
    }
        
    ServiceLocationNotificationRequestData CachingTest::CreateRequestData(
        NamingUri const & name,
        PartitionKey const & pk)
    {
        return CreateRequestData(name, map<ConsistencyUnitId, ServiceLocationVersion>(), pk);
    }

    ServiceLocationNotificationRequestData CachingTest::CreateRequestData(
        NamingUri const & name,
        std::map<Reliability::ConsistencyUnitId, ServiceLocationVersion> const & previousResolves,
        PartitionKey const & pk)
    {
        return ServiceLocationNotificationRequestData(
            name, 
            previousResolves,
            nullptr,
            pk);
    }

    ServiceLocationNotificationRequestData CachingTest::CreateRequestData(
        NamingUri const & name,
        AddressDetectionFailureSPtr const & failure,
        PartitionKey const & pk)
    {
        return ServiceLocationNotificationRequestData(
            name, 
            map<ConsistencyUnitId, ServiceLocationVersion>(),
            failure,
            pk);
    }

	void CachingTest::InnerNotificationMultipleParams(
		int serviceCount,
		int partitionCount,
		int replicaCount,
		int keyRequestsPerPartition,
		int replicaNameSize,
		int maxNotificationReplyEntryCount)
	{
		Trace.WriteInfo(
			Constants::TestSource,
			"InnerNotificationMultipleParams: serviceCount={0}, partitionCount={1}, replicaCount={2}, keyRequestsPerPartition={3}, maxNotificationReplyEntryCount={4}",
			serviceCount,
			partitionCount,
			replicaCount,
			keyRequestsPerPartition,
			replicaNameSize,
			maxNotificationReplyEntryCount);

		// setup gateway
		wstring gatewayAddress;
		root_ = CreateRoot(gatewayAddress);
		FauxStore store(NodeId(LargeInteger(0, 1)));
		FauxFM fm(root_->fsForFm_);
		fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
		root_->router_.Initialize(store, fm.Id, fm.Id);
		root_->router_.Initialize(fm, store.Id, store.Id);

		// setup client
		vector<wstring> listenAddresses;
		listenAddresses.push_back(gatewayAddress);
		clientSPtr_ = make_shared<FabricClientImpl>(move(listenAddresses));
		FabricClientImpl & client = *clientSPtr_;

		// create replica names of the desired length, randomly generated using numbers
		wstring primary;
		vector<wstring> secondaries;
		Random rand;
		bool first = true;
		for (int replIndex = 0; replIndex < replicaCount; ++replIndex)
		{
			wstring replicaName;
			StringWriter writer(replicaName);
			for (int i = 0; i < replicaNameSize; ++i)
			{
				writer << rand.Next(0, 9);
			}

			if (first)
			{
				primary = move(replicaName);
				first = false;
			}
			else
			{
				secondaries.push_back(move(replicaName));
			}
		}

		// create services
		wstring nameUriBase(L"InnerNotificationMultipleParams");
		vector<ServiceLocationNotificationRequestData> requests;
		int requestSize = 0;
		for (int i = 0; i < serviceCount; ++i)
		{
			wstring uri;
			StringWriter(uri).Write("{0}-{1}", nameUriBase, i);
			NamingUri name(move(uri));

			int partitionSize = keyRequestsPerPartition + 100;
			uint64 maxPartition = partitionCount * partitionSize;
			Trace.WriteInfo(
				Constants::TestSource,
				"Create service for {0}, pk {1}-{2}",
				name.ToString(),
				0,
				maxPartition);

			PartitionedServiceDescriptor psd;
			VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
				CreateServiceDescription(name, partitionCount, replicaCount, 1),
				0,
				maxPartition,
				psd).IsSuccess());

			SynchronousCreateName(client, name);
			SynchronousCreateService(client, psd, fm);

			for (int j = 0; j < partitionCount; ++j)
			{
				auto partitionStartKey = j * partitionSize + 1;
				Trace.WriteInfo(
					Constants::TestSource,
					"Create requests for {0}, partition {1} (partitionStartKey={2})",
					name.ToString(),
					j,
					partitionStartKey);

				for (int k = 0; k < keyRequestsPerPartition; ++k)
				{
					auto request = move(CreateRequestData(name, PartitionKey(partitionStartKey + k)));
					requestSize += static_cast<int>(request.EstimateSize());
					requests.push_back(move(request));
				}

				ConsistencyUnitId cuid;
				psd.TryGetCuid(PartitionKey(partitionStartKey), /*out*/cuid);

				Trace.WriteInfo(
					Constants::TestSource,
					"Pk {0} -> CUID {1}",
					partitionStartKey,
					cuid);

				fm.IncrementCuidFMVersion(cuid);

				vector<ConsistencyUnitId> cuids;
				cuids.push_back(cuid);
				root_->ServiceResolverProcessWouldBeBroadcastMessage(cuids, name.ToString(), fm, 1, fm.GetCuidFMVersion(cuid), primary, secondaries);
			}
		}

		// Register for notifications for each service for the requested numbers of keys per partition       
		vector<ResolvedServicePartitionSPtr> results;
		vector<AddressDetectionFailureSPtr> failures;

		// Estimate the size of the requests and set message size to a higher value
		Trace.WriteInfo("CachingTestSource", "Estimated requests size = {0}", requestSize);
		ServiceModelConfig::GetConfig().MaxMessageSize = requestSize + 128 /*buffer*/;

		int minExpectedMessageCount = 1;
		if (maxNotificationReplyEntryCount > 0)
		{
			NamingConfig::GetConfig().MaxNotificationReplyEntryCount = maxNotificationReplyEntryCount;
			minExpectedMessageCount = static_cast<int>(ceil(static_cast<double>(serviceCount * partitionCount) / static_cast<double>(maxNotificationReplyEntryCount)));
		}
		else
		{
			// Reduce the message buffer content ratio to force the gateway to send the reply in multiple messages
			ServiceModelConfig::GetConfig().MessageContentBufferRatio = 0.3;
			if (keyRequestsPerPartition == 1)
			{
				minExpectedMessageCount = 2;
			}
		}

		Trace.WriteInfo("CachingTestSource", "Expected min message count = {0}", minExpectedMessageCount);

		GetNotificationsInBatches(client, requests, ErrorCodeValue::Success, minExpectedMessageCount, results, failures);

		// Since each service has 1 partition, the result should contain one entry per service
		size_t expectedResultCount = static_cast<size_t>(serviceCount * partitionCount);
		VERIFY_ARE_EQUAL_FMT(
			expectedResultCount,
			results.size(),
			"result count: expected = {0}, actual = {1}", expectedResultCount, results.size());

		VERIFY_IS_TRUE_FMT(
			failures.empty(),
			"failure count: expected = 0, actual = {0}", failures.size());

		size_t estimateSize = 0;

		// check the results:
		//    - have correct name, correct number of partitions per name, correct number of replicas per rsp
		//    - and print the approximate message size
		map<NamingUri, size_t> rspPerName;
		for (size_t i = 0; i < results.size(); ++i)
		{
			estimateSize += results[i]->EstimateSize();
			NamingUri serviceName;
			if (!results[i]->TryGetName(serviceName))
			{
				VERIFY_FAIL(L"ServiceName doesn't exist");
			}

			if (!StringUtility::Contains<wstring>(serviceName.ToString(), nameUriBase))
			{
				VERIFY_FAIL_FMT("ServiceName {0} doesn't look right", serviceName.Name.c_str());
			}

			auto it = rspPerName.find(serviceName);
			if (it == rspPerName.end())
			{
				rspPerName[serviceName] = 1u;
			}
			else
			{
				rspPerName[serviceName] += 1u;
			}

			size_t secondariesCount = results[i]->Locations.ServiceReplicaSet.SecondaryLocations.size();
			if (secondaries.size() != secondariesCount ||
				primary != results[i]->Locations.ServiceReplicaSet.PrimaryLocation)
			{
				VERIFY_FAIL(L"replicas are not set correctly");
			}
		}

		Trace.WriteInfo("CachingTestSource", "PRSCount: {0}, total size: {1}. Max message size {2}", results.size(), estimateSize, ServiceModelConfig().GetConfig().MaxMessageSize);

		VERIFY_ARE_EQUAL_FMT(
			static_cast<size_t>(serviceCount),
			rspPerName.size(),
			"Services notif: expected = {0}, actual = {1}", serviceCount, rspPerName.size());
		for (auto it = rspPerName.begin(); it != rspPerName.end(); ++it)
		{
			if (static_cast<size_t>(partitionCount) != it->second)
			{
				VERIFY_FAIL_FMT("Expected partitions received: expected = {0}, actual = {1}", partitionCount, it->second);
			}
		}

		root_->Close();
		VERIFY_IS_TRUE(client.Close().IsSuccess());
	}

}
