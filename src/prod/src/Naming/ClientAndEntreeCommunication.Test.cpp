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
    using namespace Common;
    using namespace std;
    using namespace Transport;
    using namespace Reliability;
    using namespace Federation;
    using namespace ServiceModel;
    using namespace Client;
    using namespace Api;
    using namespace Client;
    using namespace ClientServerTransport;

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

    struct Root;
    typedef shared_ptr<Root> RootSPtr;
    struct Root : public ComponentRoot
    {
        static unique_ptr<ProxyToEntreeServiceIpcChannel> CreateProxyIpcClient(
            ComponentRoot const &root,
            std::wstring const &traceId,
            std::wstring const &ipcServerAddress)
        {
            return make_unique<ProxyToEntreeServiceIpcChannel>(root, traceId, ipcServerAddress);
        }

        static RootSPtr Create(
            NodeConfig const & nodeConfig,
            wstring const & entreeServiceAddress,
            wstring const & ipcServerAddress,
            NodeConfig const & fmNodeConfig)
        {
            RootSPtr result = make_shared<Root>();
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

            NodeInstance nodeInstance(nodeConfig.Id, SequenceNumber::GetNext());
            result->nodeConfig_ = make_shared<FabricNodeConfig>();
            result->nodeConfig_->InstanceName = wformatString("{0}", nodeInstance);

            Transport::SecuritySettings defaultSettings;
            result->fs_ = make_shared<FederationSubsystem>(nodeConfig, FabricCodeVersion(), nullptr, Common::Uri(), defaultSettings, *result);
            result->fsForFm_ = make_shared<FederationSubsystem>(fmNodeConfig, FabricCodeVersion(), nullptr, Common::Uri(), defaultSettings, *result);
            result->testNode_ = make_unique<TestHelper::TestNodeWrapper>(*(result->fs_), result->router_);
            result->adminClient_ = make_unique<ServiceAdminClient>(*(result->testNode_));
            result->resolver_ = make_unique<ServiceResolver>(*(result->testNode_), *result);

            result->proxyIpcClient_ = CreateProxyIpcClient(*result, nodeInstance.Id.ToString(), ipcServerAddress);
            result->ipcServerAddress_ = ipcServerAddress;

            result->entreeService_ = make_unique<EntreeServiceWrapper>(
                result->router_,
                *(result->resolver_),
                *(result->adminClient_),
                nodeInstance,
                result->nodeConfig_,
                entreeServiceAddress,
                ipcServerAddress,
                *(result->proxyIpcClient_),
                defaultSettings,
                *result);

            return result;
        }

        class EntreeServiceWrapper : public EntreeServiceProxy
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
                ProxyToEntreeServiceIpcChannel &proxyIpcClient,
                Transport::SecuritySettings const& securitySettings,
                Common::ComponentRoot const & root) 
                : EntreeServiceProxy(
                    root,
                    listenAddress,
                    proxyIpcClient,
                    securitySettings)
                , dropMessages_(false)
                , messagesReceived_()
                , lock_()
                , entreeService_(make_unique<EntreeService>(
                    innerRingCommunication,
                    fmClient,
                    adminClient,
                    make_shared<DummyGatewayManager>(),
                    instance,
                    ipcServerAddress,
                    nodeConfig,
                    securitySettings,
                    root))
                , proxyIpcClient_(proxyIpcClient)
            {
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
                operation = proxyIpcClient_.BeginOpen(
                    TimeSpan::FromSeconds(30), 
                    [this, manualResetEvent](AsyncOperationSPtr const &)
                    {
                        manualResetEvent->Set();
                    },
                    Root.CreateAsyncOperationRoot());

                manualResetEvent->WaitOne();
                error = proxyIpcClient_.EndOpen(operation);
                VERIFY_IS_TRUE(error.IsSuccess());

                return EntreeServiceProxy::OnOpen();
            }

            ErrorCode OnClose()
            {
                auto error = EntreeServiceProxy::OnClose();
                VERIFY_IS_TRUE(error.IsSuccess());

                auto manualResetEvent = make_shared<ManualResetEvent>();
                manualResetEvent->Reset();
                auto operation = proxyIpcClient_.BeginClose(
                    TimeSpan::FromSeconds(30), 
                    [this, manualResetEvent](AsyncOperationSPtr const &)
                    {
                    manualResetEvent->Set();
                    },
                    Root.CreateAsyncOperationRoot());

                manualResetEvent->WaitOne();
                error = proxyIpcClient_.EndClose(operation);
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

            __declspec(property(get=get_DropMessages, put=set_DropMessages)) bool DropMessages;
            bool get_DropMessages() const { return dropMessages_; }
            void set_DropMessages(bool value) { dropMessages_ = value; }
        
            __declspec(property(get=get_NamingServiceCuids)) std::vector<Reliability::ConsistencyUnitId> const & NamingServiceCuids;
            std::vector<Reliability::ConsistencyUnitId> const & get_NamingServiceCuids() const { return entreeService_->NamingServiceCuids; }

            void ProcessClientMessage(
                Transport::MessageUPtr && message,
                Transport::ISendTarget::SPtr const & sender)
            {
                {
                    AcquireExclusiveLock aquire(lock_);
                    auto it = messagesReceived_.find(message->Action);
                    int count = 1;
                    if (it == messagesReceived_.end())
                    {
                        messagesReceived_.insert(pair<wstring, int>(message->Action, count));
                    }
                    else
                    {
                        count = it->second;
                        it->second = ++count;
                    }

                    Trace.WriteNoise(Constants::TestSource, "EntreeService {0} processing {1} ({2})", this->ListenAddress, message->Action, count);
                }

                if (!dropMessages_)
                {
                    EntreeServiceProxy::ProcessClientMessage(move(message), sender);
                }
                else 
                {
                    // Do nothing
                }
            }

            int GetMessageCount(wstring const & action) const
            {
                AcquireExclusiveLock aquire(lock_);
                auto it = messagesReceived_.find(action);
                int count = (it == messagesReceived_.end()) ? 0 : it->second;
                Trace.WriteNoise(
                    Constants::TestSource, 
                    "EntreeService {0} received {1} {2} times ({3} total messages)", 
                    this->ListenAddress, 
                    action, 
                    count,
                    messagesReceived_.size());
                return count;
            }

        private:
            unique_ptr<EntreeService> entreeService_;
            bool dropMessages_;
            map<wstring, int> messagesReceived_;
            mutable RwLock lock_;
            ProxyToEntreeServiceIpcChannel &proxyIpcClient_; // reference for opening and closing
        };

        TestHelper::RouterTestHelper router_;
        FederationSubsystemSPtr fs_;
        FederationSubsystemSPtr fsForFm_;
        unique_ptr<TestHelper::TestNodeWrapper> testNode_;
        unique_ptr<ServiceAdminClient> adminClient_;
        unique_ptr<ServiceResolver> resolver_;
        unique_ptr<ProxyToEntreeServiceIpcChannel> proxyIpcClient_;
        wstring ipcServerAddress_;
        unique_ptr<EntreeServiceWrapper> entreeService_;
        ServiceDescription namingServiceDescription_;
        FabricNodeConfigSPtr nodeConfig_;
    };
        
    class ComDummyAddressChangeHandler : 
        public IFabricServicePartitionResolutionChangeHandler
        , public Common::ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(ComDummyAddressChangeHandler)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricServicePartitionResolutionChangeHandler)
            COM_INTERFACE_ITEM(IID_IFabricServicePartitionResolutionChangeHandler,IFabricServicePartitionResolutionChangeHandler)
        END_COM_INTERFACE_LIST()
    public:
        void STDMETHODCALLTYPE OnChange(
            /* [in] */ IFabricServiceManagementClient *,
            /* [in] */ LONGLONG handlerId,
            /* [in] */ IFabricResolvedServicePartitionResult *,
            /* [in] */ HRESULT error)
        {
            Trace.WriteInfo(Constants::TestSource, "OnChange for {0}: error {1}", handlerId, error);
        }
    };

    class ClientAndEntreeCommunicationTest
    {
    protected:
        ~ClientAndEntreeCommunicationTest() { BOOST_REQUIRE(MethodCleanup()); }
        TEST_METHOD_CLEANUP( MethodCleanup );

        ClientAndEntreeCommunicationTest()
          : store_(NodeId(LargeInteger(0,1)))
        {
        }        

        RootSPtr CreateRoot(wstring & entreeAddress);

        ServiceDescription CreateServiceDescription(NamingUri const &, int partitionCount, int replicaCount, int minWrite, bool isStateful = true);

        void SynchronousCreateName(
            __in  FabricClientImpl&,
            NamingUri const &,
            ErrorCodeValue::Enum expected = ErrorCodeValue::Success,
            TimeSpan timeout = TimeSpan::FromSeconds(5));

        void SynchronousCreateName(
            __in  IPropertyManagementClientPtr&,
            NamingUri const &,
            ErrorCodeValue::Enum expected = ErrorCodeValue::Success,
            TimeSpan timeout = TimeSpan::FromSeconds(5));

        void SynchronousCreateNameWithRetries(
            __in FabricClientImpl &,
            NamingUri const &,
            int retries);

        void SynchronousDeleteName(
            __in FabricClientImpl &,
            NamingUri const &,
            ErrorCodeValue::Enum expected = ErrorCodeValue::Success);

        void SynchronousNameExists(
            __in FabricClientImpl &,
            NamingUri const &,
            bool,
            ErrorCodeValue::Enum expected = ErrorCodeValue::Success);

        void SynchronousCreateService(
            __in FabricClientImpl &,
            PartitionedServiceDescriptor const &,
            ErrorCodeValue::Enum expected = ErrorCodeValue::Success);

        void SynchronousDeleteService(
            __in FabricClientImpl &, 
            NamingUri const &, 
            ErrorCodeValue::Enum expected = ErrorCodeValue::Success);

        void SynchronousSubmitPropertyBatch(
            __in FabricClientImpl&,
            NamePropertyOperationBatch &&,
            NamePropertyOperationBatchResult const &,
            ErrorCodeValue::Enum expected = ErrorCodeValue::Success);

        void SynchronousEnumerateSubNames(
            __in FabricClientImpl &,            
            NamingUri const &,
            EnumerateSubNamesToken const & token,
            ErrorCodeValue::Enum expected = ErrorCodeValue::Success);

        static vector<byte> GetBytes(wstring const & content);

        FauxStore store_;
        RootSPtr root_;
        RootSPtr root2_;
        IClientFactoryPtr clientFactoryPtr_;
        shared_ptr<FabricClientImpl> clientSPtr_;
    };



    BOOST_FIXTURE_TEST_SUITE(ClientAndEntreeCommunicationTestSuite,ClientAndEntreeCommunicationTest)

    BOOST_AUTO_TEST_CASE(PingTest)
    {
        Trace.WriteInfo(Constants::TestSource, "*** PingTest");

        wstring entreeServiceListenAddress;
        root_ = CreateRoot(entreeServiceListenAddress);  
        VERIFY_IS_TRUE(root_->entreeService_->Open().IsSuccess());

        TestHelper::TestNodeWrapper fmNode(*(root_->fsForFm_), root_->router_);
        TestHelper::FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
        
        root_->router_.Initialize(store_, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store_.Id, store_.Id);

        NamingUri nameToTest(L"PingTest");

        // All addresses are invalid - request should timeout
        //
        {
            vector<wstring> services;
            services.push_back(L"127.0.0.1:22222");
            services.push_back(L"127.0.0.1:33333");
            services.push_back(L"127.0.0.1:44444");
            clientSPtr_ = make_shared<FabricClientImpl>(move(services));
            FabricClientImpl & client = *clientSPtr_;

            SynchronousCreateName(client, nameToTest, ErrorCodeValue::GatewayUnreachable, TimeSpan::FromSeconds(6));

            VERIFY_IS_TRUE(clientSPtr_->Close().IsSuccess());
        }

        // Valid address exists - request should use valid address after retries.
        // Also cover loading addresses from config rather than initializing via ctor.
        //
        {
            wstring configString(L"127.0.0.1:22222,127.0.0.1:33333,127.0.0.1:44444,");
            configString.append(entreeServiceListenAddress);
            ClientConfig::GetConfig().NodeAddresses = configString;
            clientSPtr_ = make_shared<FabricClientImpl>(vector<wstring>());
            FabricClientImpl & client = *clientSPtr_;

            SynchronousCreateName(client, nameToTest, ErrorCodeValue::Success, TimeSpan::FromSeconds(8));
            SynchronousCreateName(client, NamingUri(L"PingTestA1"));
            SynchronousCreateName(client, NamingUri(L"PingTestA2"));

            VERIFY_IS_TRUE(clientSPtr_->Close().IsSuccess());
        }

        // Invalid hostname format - missing port
        //
        {
            wstring configString(L"127.0.0.1:22222,myhost,");
            configString.append(entreeServiceListenAddress);
            ClientConfig::GetConfig().NodeAddresses = configString;
            clientSPtr_ = make_shared<FabricClientImpl>(vector<wstring>());
            ErrorCode error = clientSPtr_->Open();
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidAddress));
        }
        
        // Invalid hostname format - invalid port
        //
        {
            wstring configString(L"127.0.0.1:22222,myhost:invalidport,");
            configString.append(entreeServiceListenAddress);
            ClientConfig::GetConfig().NodeAddresses = configString;
            clientSPtr_ = make_shared<FabricClientImpl>(vector<wstring>());
            ErrorCode error = clientSPtr_->Open();
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidAddress));
        }
        
        // Invalid hostname format - invalid port range
        //
        {
            wstring configString(L"127.0.0.1:22222,myhost:70000,");
            configString.append(entreeServiceListenAddress);
            ClientConfig::GetConfig().NodeAddresses = configString;
            clientSPtr_ = make_shared<FabricClientImpl>(vector<wstring>());
            ErrorCode error = clientSPtr_->Open();
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidAddress));
        }
        
        // Valid hostname format - DNS resolution failure
        // Should skip failure and try other addresses
        //
        {
            wstring configString(L"127.0.0.1:22222,myhost:42,");
            configString.append(entreeServiceListenAddress);
            ClientConfig::GetConfig().NodeAddresses = configString;
            clientSPtr_ = make_shared<FabricClientImpl>(vector<wstring>());
            FabricClientImpl & client = *clientSPtr_;

            SynchronousCreateName(client, NamingUri(L"PingTestB0"), ErrorCodeValue::Success, TimeSpan::FromSeconds(16));
            SynchronousCreateName(client, NamingUri(L"PingTestB1"));
            SynchronousCreateName(client, NamingUri(L"PingTestB2"));
        }
    }

    BOOST_AUTO_TEST_CASE(BasicThreeScenario)
    {
        Trace.WriteInfo(Constants::TestSource, "*** BasicThreeScenario");

        wstring entreeServiceListenAddress;
        root_ = CreateRoot(entreeServiceListenAddress);  
        VERIFY_IS_TRUE(root_->entreeService_->Open().IsSuccess());

        TestHelper::TestNodeWrapper fmNode(*(root_->fsForFm_), root_->router_);
        TestHelper::FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
        
        root_->router_.Initialize(store_, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store_.Id, store_.Id);

        vector<wstring> services;
        services.push_back(entreeServiceListenAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(services));
        FabricClientImpl & client = *clientSPtr_;

        NamingUri nameToTest(L"Neumos");

        SynchronousCreateName(client, nameToTest);
        SynchronousNameExists(client, nameToTest, true);
        SynchronousDeleteName(client, nameToTest);        
        SynchronousNameExists(client, nameToTest, false);
    }

    BOOST_AUTO_TEST_CASE(PropertyScenario)
    {
        Trace.WriteInfo(Constants::TestSource, "*** PropertyScenario");

        wstring entreeServiceListenAddress;
        root_ = CreateRoot(entreeServiceListenAddress);  
        VERIFY_IS_TRUE(root_->entreeService_->Open().IsSuccess());

        TestHelper::TestNodeWrapper fmNode(*(root_->fsForFm_), root_->router_);
        TestHelper::FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
        
        root_->router_.Initialize(store_, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store_.Id, store_.Id);

        vector<wstring> services;
        services.push_back(entreeServiceListenAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(services));
        FabricClientImpl & client = *clientSPtr_;

        NamingUri nameToTest(L"El_Corazon");
        wstring property1 = L"Alexisonfire";
        wstring property2 = L"Nile";
        
        wstring property1Value(L"property1Value");
        ULONG value1Length = static_cast<ULONG>(sizeof(wchar_t) * (property1Value.size() + 1));

        wstring property2Value(L"property2WithDifferentValue");
        ULONG value2Length = static_cast<ULONG>(sizeof(wchar_t) * (property2Value.size() + 1));

        SynchronousCreateName(client, nameToTest);
        
        // Unsuccessful batch: check operation fails, no put operation should be executed
        {
            Trace.WriteInfo("ClientAndEntreeCommunicationTestSource", "****Submit batch1");
            vector<byte> data;
            NamePropertyOperationBatch batch(nameToTest);
            batch.AddPutPropertyOperation(property1, move(data), ::FABRIC_PROPERTY_TYPE_BINARY);
            batch.AddCheckExistenceOperation(property1, true);
            batch.AddCheckExistenceOperation(property2, true);

            NamePropertyOperationBatchResult expectedResult;
            expectedResult.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, 2u);
            
            SynchronousSubmitPropertyBatch(client, std::move(batch), expectedResult);
        }

        // Successful batch: put property1, get value
        {
            Trace.WriteInfo("ClientAndEntreeCommunicationTestSource", "****Submit batch2");
            NamePropertyOperationBatch batch(nameToTest);
            batch.AddCheckExistenceOperation(property1, false);
            batch.AddPutPropertyOperation(property1, GetBytes(property1Value), ::FABRIC_PROPERTY_TYPE_WSTRING);
            batch.AddGetPropertyOperation(property1, true /*includeValues*/);
            
            NamePropertyOperationBatchResult expectedResult;
            expectedResult.AddProperty(
                NamePropertyResult(
                    NamePropertyMetadataResult(
                        nameToTest,
                        NamePropertyMetadata(property1, ::FABRIC_PROPERTY_TYPE_WSTRING, value1Length, wstring()),
                        DateTime::Now(),
                        1,
                        2),
                    GetBytes(property1Value)));
            SynchronousSubmitPropertyBatch(client, std::move(batch), expectedResult);
        }

        // Successful batch: Put property2 with custom type, get metadata for both properties
        {
            Trace.WriteInfo("ClientAndEntreeCommunicationTestSource", "****Submit batch3");
            NamePropertyOperationBatch batch(nameToTest);

            wstring customTypeId(L"PropertyScenarioSpecialType");
            wstring submitCustomTypeId(customTypeId);
            wstring resultCustomTypeId(customTypeId);
            
            batch.AddPutCustomPropertyOperation(property2, GetBytes(property2Value), ::FABRIC_PROPERTY_TYPE_BINARY, move(submitCustomTypeId));
            batch.AddGetPropertyOperation(property1, false /*includeValues*/);
            batch.AddCheckExistenceOperation(property1, true);    
            batch.AddGetPropertyOperation(property2, false /*includeValues*/);
            batch.AddCheckValuePropertyOperation(property1, GetBytes(property1Value), ::FABRIC_PROPERTY_TYPE_WSTRING);
            
            NamePropertyOperationBatchResult expectedResult;
            wstring resultEmptyCustomTypeId;
            expectedResult.AddMetadata(
                    NamePropertyMetadataResult(
                        nameToTest,
                        NamePropertyMetadata(property1, ::FABRIC_PROPERTY_TYPE_WSTRING, value1Length, wstring()),
                        DateTime::Now(),
                        1,
                        1));
            expectedResult.AddMetadata(
                    NamePropertyMetadataResult(
                        nameToTest,
                        NamePropertyMetadata(property2, ::FABRIC_PROPERTY_TYPE_BINARY, value2Length, move(resultCustomTypeId)),
                        DateTime::Now(),
                        1,
                        3));

            SynchronousSubmitPropertyBatch(client, std::move(batch), expectedResult);
        }

        // Unsuccessful batch - check property value with type mismatch
        {
            Trace.WriteInfo("ClientAndEntreeCommunicationTestSource", "****Submit batch4");
            NamePropertyOperationBatch batch(nameToTest);

            batch.AddCheckValuePropertyOperation(property2, GetBytes(property2Value), ::FABRIC_PROPERTY_TYPE_BINARY);
            batch.AddCheckValuePropertyOperation(property1, GetBytes(property1Value), ::FABRIC_PROPERTY_TYPE_GUID);
            
            NamePropertyOperationBatchResult expectedResult;
            expectedResult.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, 1u);

            SynchronousSubmitPropertyBatch(client, std::move(batch), expectedResult);
        }

        // Unsuccessful batch - check property value with value mismatch
        {
            Trace.WriteInfo("ClientAndEntreeCommunicationTestSource", "****Submit batch5");
            NamePropertyOperationBatch batch(nameToTest);
            batch.AddCheckValuePropertyOperation(property2, GetBytes(property1Value), ::FABRIC_PROPERTY_TYPE_BINARY);
            
            NamePropertyOperationBatchResult expectedResult;
            expectedResult.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, 0u);

            SynchronousSubmitPropertyBatch(client, std::move(batch), expectedResult);
        }

        // Unsuccessful batch
        // Note that delete is successful (due to a limitation of FauxStore, that doesn't undo the operations when batch fails)
        {
            Trace.WriteInfo("ClientAndEntreeCommunicationTestSource", "****Submit batch6");
            NamePropertyOperationBatch batch(nameToTest);

            batch.AddDeletePropertyOperation(property1);
            batch.AddDeletePropertyOperation(property2);
            batch.AddCheckExistenceOperation(property1, false);        
            batch.AddCheckExistenceOperation(property2, true);

            NamePropertyOperationBatchResult expectedResult;
            expectedResult.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, 3u);
            
            SynchronousSubmitPropertyBatch(client, std::move(batch), expectedResult);
        }
    }

    BOOST_AUTO_TEST_CASE(ErrorToClientScenario)
    {
        Trace.WriteInfo(Constants::TestSource, "*** ErrorToClientScenario");

        wstring entreeServiceListenAddress;
        root_ = CreateRoot(entreeServiceListenAddress);  
        VERIFY_IS_TRUE(root_->entreeService_->Open().IsSuccess());

        TestHelper::TestNodeWrapper fmNode(*(root_->fsForFm_), root_->router_);
        TestHelper::FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);

        root_->router_.Initialize(fm, store_.Id, store_.Id);        
        root_->router_.Initialize(store_, fm.Id, fm.Id);   

        vector<wstring> services;
        services.push_back(entreeServiceListenAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(services));
        FabricClientImpl & client = *clientSPtr_;

        NamingUri nameToTest(L"Sunset");
        NamingUri nameToTest2(L"Marymoor");

        wstring placementConstraints = L"";
        int scaleoutCount = 0;

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            CreateServiceDescription(nameToTest2, 5, 5, 5, false),
            0,
            100,
            psd).IsSuccess());
        PartitionedServiceDescriptor invalidPsd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(L"InvalidName", 0, 0, 5, 5, 5, false, false, TimeSpan::Zero, TimeSpan::Zero, TimeSpan::Zero, ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(), L"Test"), vector<Reliability::ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<Reliability::ServiceLoadMetricDescription>(), 0, vector<byte>()),
            0,
            100,
            invalidPsd).IsSuccess());
        PartitionedServiceDescriptor invalidPsd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(L"fabric:/Crocodile/Belltown/", 0, 0, 5, 5, 5, false, false, TimeSpan::Zero, TimeSpan::Zero, TimeSpan::Zero, ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(), L"Test"), vector<Reliability::ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<Reliability::ServiceLoadMetricDescription>(), 0, vector<byte>()),
            0,
            100,
            invalidPsd2).IsSuccess()); 
        PartitionedServiceDescriptor invalidPsd3;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(L"fabric:\bMoore", 0, 0, 5, 5, 5, false, false, TimeSpan::Zero, TimeSpan::Zero, TimeSpan::Zero, ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(), L"Test"), vector<Reliability::ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<Reliability::ServiceLoadMetricDescription>(), 0, vector<byte>()),
            0,
            100,
            invalidPsd3).IsSuccess());

        SynchronousCreateName(client, nameToTest);

        Trace.WriteInfo(Constants::TestSource, "Try failures - ensure the client doesn't AV");

        SynchronousCreateName(client, nameToTest, ErrorCodeValue::OperationFailed);
        SynchronousDeleteName(client, nameToTest2, ErrorCodeValue::OperationFailed);        
        SynchronousCreateService(client, psd, ErrorCodeValue::OperationFailed);
        SynchronousCreateService(client, invalidPsd, ErrorCodeValue::InvalidNameUri);
        SynchronousCreateService(client, invalidPsd2, ErrorCodeValue::InvalidNameUri);
        SynchronousCreateService(client, invalidPsd3, ErrorCodeValue::InvalidNameUri);
        SynchronousDeleteService(client, nameToTest2, ErrorCodeValue::OperationFailed);
        EnumerateSubNamesToken token;
        SynchronousEnumerateSubNames(client, nameToTest2, token, ErrorCodeValue::OperationFailed);        

        NamePropertyOperationBatch batch(nameToTest2);
        batch.AddCheckExistenceOperation(L"Paramount", false);
        SynchronousSubmitPropertyBatch(client, std::move(batch), NamePropertyOperationBatchResult(), ErrorCodeValue::OperationFailed);
    }

    BOOST_AUTO_TEST_CASE(CommunicationStateCheck)
    {
        Trace.WriteInfo(Constants::TestSource, "*** CommunicationStateCheck");

        wstring entreeServiceListenAddress;
        root_ = CreateRoot(entreeServiceListenAddress);  
            
        TestHelper::TestNodeWrapper fmNode(*(root_->fsForFm_), root_->router_);
        TestHelper::FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);

        root_->router_.Initialize(fm, store_.Id, store_.Id);
        root_->router_.Initialize(store_, fm.Id, fm.Id);  

        vector<wstring> services;
        services.push_back(entreeServiceListenAddress);
        clientSPtr_ = make_shared<FabricClientImpl>(move(services));
        FabricClientImpl & client = *clientSPtr_;

        NamingUri nameToTest(L"Studio7");
        NamingUri nameToTest2(L"Showbox");
        
        Trace.WriteInfo(Constants::TestSource, "Try to do something before opening the entree service");
        SynchronousCreateName(client, nameToTest, ErrorCodeValue::GatewayUnreachable);
        
        Trace.WriteInfo(Constants::TestSource, "Perform action in good state");
        VERIFY_IS_TRUE(root_->entreeService_->Open().IsSuccess());
        SynchronousCreateName(client, nameToTest);

        Trace.WriteInfo(Constants::TestSource, "Try to do something after closing entree service");
        VERIFY_IS_TRUE(root_->entreeService_->Close().IsSuccess());

        // Replace this sleep with a wait for the disconnection event on the client
        // once it's implemented. Before the disconnection event fires, we may
        // see a timeout instead.
        //
        Sleep(2000);

        SynchronousCreateName(client, nameToTest2, ErrorCodeValue::GatewayUnreachable);

        Trace.WriteInfo(Constants::TestSource, "Try to do something after closing naming client");
        VERIFY_IS_TRUE(client.Close().IsSuccess());
        SynchronousCreateName(client, nameToTest2, ErrorCodeValue::ObjectClosed);
    }

    BOOST_AUTO_TEST_CASE(EntreeServiceSwitchTest)
    {
        Trace.WriteInfo(Constants::TestSource, "*** EntreeServiceSwitchTest");

        // create one service
        wstring entreeServiceListenAddress;
        root_ = CreateRoot(entreeServiceListenAddress);  
        VERIFY_IS_TRUE(root_->entreeService_->Open().IsSuccess());
            
        TestHelper::TestNodeWrapper fmNode(*(root_->fsForFm_), root_->router_);
        TestHelper::FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);

        root_->router_.Initialize(fm, store_.Id, store_.Id);
        root_->router_.Initialize(store_, fm.Id, fm.Id);

        // create a second service
        wstring entreeServiceListenAddress2 = L"127.0.0.1:12366";
        root2_ = CreateRoot(entreeServiceListenAddress2);
        VERIFY_IS_TRUE(root2_->entreeService_->Open().IsSuccess());  
        
        root2_->router_.Initialize(fm, store_.Id, store_.Id);
        root2_->router_.Initialize(store_, fm.Id, fm.Id);

        // create client
        vector<wstring> services;
        services.push_back(entreeServiceListenAddress);
        services.push_back(entreeServiceListenAddress2);
        clientSPtr_ = make_shared<FabricClientImpl>(move(services));
        FabricClientImpl & client = *clientSPtr_;
                
        NamingUri nameToTest(L"ChopSuey");
        NamingUri nameToTest2(L"Comet");
        NamingUri nameToTest3(L"Nectar");

        // do something in a good state
        SynchronousCreateName(client, nameToTest);

        // close the entree service and try an action
        wstring currentGatewayAddress;
        VERIFY_IS_TRUE(client.GetCurrentGatewayAddress(currentGatewayAddress).IsSuccess());
        if (currentGatewayAddress == entreeServiceListenAddress)
        {
            VERIFY_IS_TRUE(root_->entreeService_->Close().IsSuccess());
        }
        else
        {
            VERIFY_IS_TRUE(root2_->entreeService_->Close().IsSuccess());
        }

        SynchronousCreateNameWithRetries(client, nameToTest2, 3);

        // swap the entree services and try an action
        VERIFY_IS_TRUE(client.GetCurrentGatewayAddress(currentGatewayAddress).IsSuccess());
        if (currentGatewayAddress == entreeServiceListenAddress)
        {
            VERIFY_IS_TRUE(root_->entreeService_->Close().IsSuccess());
        }
        else
        {
            VERIFY_IS_TRUE(root2_->entreeService_->Close().IsSuccess());
        }
        
        NodeInstance nodeInstance(NodeId::MinNodeId, SequenceNumber::GetNext());
        root_->nodeConfig_ = make_shared<FabricNodeConfig>();
        root_->nodeConfig_->InstanceName = wformatString("{0}", nodeInstance);
        root_->proxyIpcClient_ = Root::CreateProxyIpcClient(*root_, nodeInstance.Id.ToString(), root_->ipcServerAddress_);

        root_->entreeService_ = make_unique<Root::EntreeServiceWrapper>(
            root_->router_,
            *(root_->resolver_),
            *(root_->adminClient_),
            nodeInstance,
            root_->nodeConfig_,
            currentGatewayAddress == entreeServiceListenAddress ? entreeServiceListenAddress2 : entreeServiceListenAddress,
            root_->ipcServerAddress_,
            *(root_->proxyIpcClient_),
            SecuritySettings(),
            *root_);
        VERIFY_IS_TRUE(root_->entreeService_->Open().IsSuccess());

        SynchronousCreateNameWithRetries(client, nameToTest3, 3);
    }

    BOOST_AUTO_TEST_CASE(NotificationsPingCount)
    {
        Trace.WriteInfo(Constants::TestSource, "*** NotificationsPingCount");

        wstring entreeServiceListenAddress;
        root_ = CreateRoot(entreeServiceListenAddress);  
        VERIFY_IS_TRUE(root_->entreeService_->Open().IsSuccess());

        TestHelper::TestNodeWrapper fmNode(*(root_->fsForFm_), root_->router_);
        TestHelper::FauxFM fm(root_->fsForFm_);
        fm.DirectlyAddService(root_->namingServiceDescription_, root_->entreeService_->NamingServiceCuids);
        
        root_->router_.Initialize(store_, fm.Id, fm.Id);
        root_->router_.Initialize(fm, store_.Id, store_.Id);

        vector<wstring> addresses;
        addresses.push_back(entreeServiceListenAddress);
        
        ClientConfig::GetConfig().ServiceChangePollInterval = TimeSpan::FromSeconds(4);

        auto error = Client::ClientFactory::CreateClientFactory(move(addresses), clientFactoryPtr_);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ClientFactory creation failed");

        // Create ComFabricClient to be able to use it for IFabricServiceManagementClient
        ComPointer<Api::ComFabricClient> comClient;
        auto hr = Api::ComFabricClient::CreateComFabricClient(clientFactoryPtr_, comClient);
        VERIFY_SUCCEEDED(hr, L"ComFabricClient creation failed");

        ComPointer<IFabricServiceManagementClient> svcClient;
        hr = comClient->QueryInterface(
            IID_IFabricServiceManagementClient, 
            reinterpret_cast<void**>(svcClient.InitializationAddress()));
        VERIFY_SUCCEEDED(hr, L"QI succeeded");

        NamingUri nameToTest(L"NotificationsPingCount");

        ComPointer<IFabricServicePartitionResolutionChangeHandler> comCallback = make_com<ComDummyAddressChangeHandler, IFabricServicePartitionResolutionChangeHandler>();
        LONGLONG callbackHandle;
        wstring nameString = nameToTest.Name;
        FABRIC_URI name = nameString.c_str();
        hr = comClient->RegisterServicePartitionResolutionChangeHandler(
            name, 
            FABRIC_PARTITION_KEY_TYPE_NONE,
            NULL,
            comCallback.GetRawPointer(),
            /*out*/&callbackHandle);
        VERIFY_SUCCEEDED(hr, L"Register service callback succeeded");

        // In the background, there will be continuous poll for service location;
        // First poll returns error, the next return success with no entries
        int pingCount = 0;
        int pollCount = 0;
        for (int i = 0; i < 20; ++i)
        {
            pingCount = root_->entreeService_->GetMessageCount(NamingTcpMessage::PingRequestAction);
            pollCount = root_->entreeService_->GetMessageCount(NamingTcpMessage::ServiceLocationChangeListenerAction);
            if (pollCount > 2)
            {
                break;
            }

            Sleep(500);
        }

        VERIFY_IS_TRUE_FMT(pingCount == 1, "Ping count: {0}", pingCount);
        VERIFY_IS_TRUE_FMT(pollCount > 1, "Poll count: {0}", pollCount);

        hr = comClient->UnregisterServicePartitionResolutionChangeHandler(callbackHandle);
        VERIFY_SUCCEEDED(hr, L"Unregister service callback succeeded");
    }    

    BOOST_AUTO_TEST_SUITE_END()

    RootSPtr ClientAndEntreeCommunicationTest::CreateRoot(wstring & entreeAddress)
    {
        USHORT basePort = 0;
        TestPortHelper::GetPorts(5, basePort);
        wstring addr1 = wformatString("127.0.0.1:{0}", basePort++);
        wstring addr2 = wformatString("127.0.0.1:{0}", basePort++);
        wstring addr3 = wformatString("127.0.0.1:{0}", basePort++);
        wstring addr4 = wformatString("127.0.0.1:{0}", basePort++);
        entreeAddress = addr3;
        NodeId entreeId(LargeInteger(0,1));
        
        NodeConfig nodeConfig(entreeId, addr1, addr2, L"");
        NodeConfig fmConfig(NodeId::MinNodeId, addr1, addr2, L"");
        return Root::Create(nodeConfig, addr3, addr4, fmConfig);
    }

    bool ClientAndEntreeCommunicationTest::MethodCleanup()
    {
        if (root_)
        {
            if (root_->entreeService_->State.Value != FabricComponentState::Closed)
            {
                VERIFY_IS_TRUE(root_->entreeService_->Close().IsSuccess());
            }

            root_.reset();
        }

        if (root2_)
        {
            if (root2_->entreeService_->State.Value != FabricComponentState::Closed)
            {
                VERIFY_IS_TRUE(root2_->entreeService_->Close().IsSuccess());
            }

            root2_.reset();
        }

        if (clientSPtr_)
        {
            if (clientSPtr_->State.Value != FabricComponentState::Closed)
            {
                VERIFY_IS_TRUE(clientSPtr_->Close().IsSuccess());
            }

            clientSPtr_.reset();
        }

        ClientConfig::GetConfig().Test_Reset();
        NamingConfig::GetConfig().Test_Reset();

        return true;
    }

        
    ServiceDescription ClientAndEntreeCommunicationTest::CreateServiceDescription(
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

    void ClientAndEntreeCommunicationTest::SynchronousCreateName(
        __in FabricClientImpl & client,
        NamingUri const & nameToTest,
        ErrorCodeValue::Enum expected,
        TimeSpan timeout)
    {
        AutoResetEvent wait;
        auto ptr = client.BeginCreateName(
            nameToTest,
            timeout,
            [this, &wait] (AsyncOperationSPtr const &) -> void
            {
                wait.Set();
            },
            client.CreateAsyncOperationRoot());

        TimeSpan waitTimeout = timeout + TimeSpan::FromSeconds(5);
        VERIFY_IS_TRUE_FMT(
            wait.WaitOne(waitTimeout),
            "CreateName {0} completed on time, waitTimeout={1} msec", nameToTest.ToString().c_str(), waitTimeout.TotalMilliseconds());

        ErrorCode error = client.EndCreateName(ptr);
        Trace.WriteInfo(Constants::TestSource, "EndCreateName {0} returned error {1}; expected {2}", nameToTest.ToString(), error, expected);
        VERIFY_ARE_EQUAL(expected, error.ReadValue());

        if (expected == ErrorCodeValue::Success)
        {
            VERIFY_IS_TRUE_FMT(
                store_.NameExists(nameToTest),
                "Name {0} exists after create", nameToTest.ToString().c_str());
        }
    }

    void ClientAndEntreeCommunicationTest::SynchronousCreateName(
        __in IPropertyManagementClientPtr & client,
        NamingUri const & nameToTest,
        ErrorCodeValue::Enum expected,
        TimeSpan timeout)
    {
        AutoResetEvent wait;
        auto ptr = client->BeginCreateName(
            nameToTest,
            timeout,
            [this, &wait] (AsyncOperationSPtr const &) -> void
            {
                wait.Set();
            },
            root_->CreateAsyncOperationRoot());

        TimeSpan waitTimeout = timeout + TimeSpan::FromSeconds(5);
        VERIFY_IS_TRUE_FMT(
            wait.WaitOne(waitTimeout),
            "CreateName {0} completed on time, waitTimeout={1} msec", nameToTest.ToString().c_str(), waitTimeout.TotalMilliseconds());

        ErrorCode error = client->EndCreateName(ptr);
        Trace.WriteInfo(Constants::TestSource, "EndCreateName {0} returned error {1}; expected {2}", nameToTest.ToString(), error, expected);
        VERIFY_ARE_EQUAL(expected, error.ReadValue());

        if (expected == ErrorCodeValue::Success)
        {
			VERIFY_IS_TRUE_FMT(
                store_.NameExists(nameToTest),
                "Name {0} exists after create", nameToTest.ToString().c_str());
        }
    }

    void ClientAndEntreeCommunicationTest::SynchronousDeleteName(
        __in FabricClientImpl & client,
        NamingUri const & nameToTest,
        ErrorCodeValue::Enum expected)
    {
        AutoResetEvent wait;
        auto ptr = client.BeginDeleteName(
            nameToTest,
            TimeSpan::FromSeconds(10),
            [this, &wait] (AsyncOperationSPtr const &) -> void
            {
                wait.Set();
            },
            root_->CreateAsyncOperationRoot());
        VERIFY_IS_TRUE_FMT(
            wait.WaitOne(TimeSpan::FromSeconds(15)),
            "DeleteName {0} completed on time", nameToTest.ToString().c_str());

        ErrorCode error = client.EndDeleteName(ptr);
        Trace.WriteInfo(Constants::TestSource, "EndDeleteName {0} returned error {1}; expected {2}", nameToTest.ToString(), error, expected);
        VERIFY_ARE_EQUAL(expected, error.ReadValue());

        if (expected == ErrorCodeValue::Success)
        {
            VERIFY_IS_FALSE(store_.NameExists(nameToTest), wformatString("{0} shouldn't exist", nameToTest.ToString()).c_str());
        }
    }

    void ClientAndEntreeCommunicationTest::SynchronousNameExists(
        __in FabricClientImpl & client,
        NamingUri const & nameToTest,
        bool shouldExist,
        ErrorCodeValue::Enum expected)
    {
        AutoResetEvent wait;
        auto ptr = client.BeginNameExists(
            nameToTest,
            TimeSpan::FromSeconds(10),
            [this, &wait] (AsyncOperationSPtr const &) -> void
            {                
                wait.Set();
            },
            root_->CreateAsyncOperationRoot());
			VERIFY_IS_TRUE_FMT(
            wait.WaitOne(TimeSpan::FromSeconds(15)),
            "NameExists {0} completed on time", nameToTest.ToString().c_str());

        bool exists;
        ErrorCode error = client.EndNameExists(ptr, exists);
        Trace.WriteInfo(Constants::TestSource, "EndNameExists returned error {0}; expected {1}", error, expected);
        VERIFY_ARE_EQUAL(expected, error.ReadValue());
        if (expected == ErrorCodeValue::Success)
        {
            VERIFY_ARE_EQUAL_FMT(shouldExist, exists, "{0} exists", nameToTest.ToString().c_str());
        }

        if (expected == ErrorCodeValue::Success)
        {
            if (shouldExist)
            {
				VERIFY_IS_TRUE_FMT(store_.NameExists(nameToTest), "{0} should exist", nameToTest.ToString().c_str());
            }
            else
            {
                VERIFY_IS_FALSE(store_.NameExists(nameToTest), wformatString("{0} shouldn't exist", nameToTest.ToString()).c_str());
            }
        }
    }

    void ClientAndEntreeCommunicationTest::SynchronousCreateService(
        __in FabricClientImpl & client,
        PartitionedServiceDescriptor const & psd,
        ErrorCodeValue::Enum expected)
    {
        AutoResetEvent wait;
        auto ptr = client.BeginInternalCreateService(
            psd,
            ServiceModel::ServicePackageVersion(),
            0,            
            TimeSpan::FromSeconds(10),
            [this, &wait] (AsyncOperationSPtr const &) -> void
            {                
                wait.Set();
            },
            root_->CreateAsyncOperationRoot());

			VERIFY_IS_TRUE_FMT(
            wait.WaitOne(TimeSpan::FromSeconds(15)),
            "CreateService {0} completed on time", psd.ToString().c_str());

        ErrorCode error = client.EndInternalCreateService(ptr);
        Trace.WriteInfo(Constants::TestSource, "EndCreateService {0} returned error {1}; expected {2}", psd.ToString(), error, expected);
        VERIFY_ARE_EQUAL(expected, error.ReadValue());
    }

    void ClientAndEntreeCommunicationTest::SynchronousDeleteService(
        __in FabricClientImpl & client,
        NamingUri const & nameToTest,
        ErrorCodeValue::Enum expected)
    {
        AutoResetEvent wait;
        auto ptr = client.BeginInternalDeleteService(
            ServiceModel::DeleteServiceDescription(nameToTest),
            ActivityId(Guid::NewGuid()),
            TimeSpan::FromSeconds(10),
            [this, &wait] (AsyncOperationSPtr const &) -> void
            {
                wait.Set();
            },
            root_->CreateAsyncOperationRoot());
			VERIFY_IS_TRUE_FMT(
            wait.WaitOne(TimeSpan::FromSeconds(15)),
            "DeleteService {0} completed on time", nameToTest.ToString().c_str());

        bool unused;
        ErrorCode error = client.EndInternalDeleteService(ptr, unused);
        Trace.WriteInfo(Constants::TestSource, "EndDeleteService {0} returned error {1}; expected {2}", nameToTest.ToString(), error, expected);
        VERIFY_ARE_EQUAL(expected, error.ReadValue());
    }

    void ClientAndEntreeCommunicationTest::SynchronousSubmitPropertyBatch(
        __in FabricClientImpl & client,
        NamePropertyOperationBatch && batch,
        NamePropertyOperationBatchResult const & expectedResult,
        ErrorCodeValue::Enum expected)
    {
        AutoResetEvent wait;
        auto ptr = client.BeginSubmitPropertyBatch(
            std::move(batch),
            TimeSpan::FromSeconds(10),
            [this, &wait] (AsyncOperationSPtr const &) -> void
            {                
                wait.Set();
            },
            root_->CreateAsyncOperationRoot());

			VERIFY_IS_TRUE_FMT(
            wait.WaitOne(TimeSpan::FromSeconds(15)),
            "SubmitPropertyBatch completed on time");

        NamePropertyOperationBatchResult result;
        ErrorCode error = client.EndSubmitPropertyBatch(ptr, result);
        Trace.WriteInfo(
            Constants::TestSource,
            "EndSubmitPropertyBatch returned error {0}; expected {1}", 
            error,
            expected);
        VERIFY_ARE_EQUAL(expected, error.ReadValue());
        if (expected == ErrorCodeValue::Success)
        {
            VERIFY_ARE_EQUAL(expectedResult.Error.ReadValue(), result.Error.ReadValue());
            VERIFY_ARE_EQUAL(expectedResult.FailedOperationIndex, result.FailedOperationIndex);

            VERIFY_ARE_EQUAL(expectedResult.Metadata.size(), result.Metadata.size());
            for (size_t i = 0; i < expectedResult.Metadata.size(); ++i)
            {
                VERIFY_ARE_EQUAL(expectedResult.Metadata[i].PropertyName, result.Metadata[i].PropertyName);
                VERIFY_ARE_EQUAL(expectedResult.Metadata[i].Name, result.Metadata[i].Name);
                VERIFY_ARE_EQUAL(expectedResult.Metadata[i].CustomTypeId, result.Metadata[i].CustomTypeId);
                VERIFY_ARE_EQUAL(expectedResult.Metadata[i].TypeId, result.Metadata[i].TypeId);
            }

            VERIFY_ARE_EQUAL(expectedResult.Properties.size(), result.Properties.size());
            for (size_t i = 0; i < expectedResult.Properties.size(); ++i)
            {
                VERIFY_ARE_EQUAL(expectedResult.Properties[i].Bytes.size(), result.Properties[i].Bytes.size());
                VERIFY_ARE_EQUAL(expectedResult.Properties[i].Metadata.CustomTypeId, result.Properties[i].Metadata.CustomTypeId);
                VERIFY_ARE_EQUAL(expectedResult.Properties[i].Metadata.PropertyName, result.Properties[i].Metadata.PropertyName);
                VERIFY_ARE_EQUAL(expectedResult.Properties[i].Metadata.Name, result.Properties[i].Metadata.Name);
                VERIFY_ARE_EQUAL(expectedResult.Properties[i].Metadata.TypeId, result.Properties[i].Metadata.TypeId);
                for (size_t ix = 0; ix < expectedResult.Properties[i].Bytes.size(); ++ix)
                {
                    if(expectedResult.Properties[i].Bytes[ix] != result.Properties[i].Bytes[ix])
                    {
                        VERIFY_FAIL(L"SynchronousSubmitPropertyBatch: Property value check failed");
                    }
                }
            }
        }
    }

    void ClientAndEntreeCommunicationTest::SynchronousEnumerateSubNames(
        __in FabricClientImpl & client,
        NamingUri const & nameToTest,
        EnumerateSubNamesToken const & token,
        ErrorCodeValue::Enum expected)
    {
        // This operation is not currently supported by the FauxStore
        // Use this method for error-path testing only
        AutoResetEvent wait;
        auto ptr = client.BeginEnumerateSubNames(
            nameToTest,
            token,
            false,
            TimeSpan::FromSeconds(10),
            [this, &wait] (AsyncOperationSPtr const &) -> void
            {                
                wait.Set();
            },
            root_->CreateAsyncOperationRoot());
			VERIFY_IS_TRUE_FMT(
            wait.WaitOne(TimeSpan::FromSeconds(15)),
            "EnumerateSubNames {0} completed on time", nameToTest.ToString().c_str());

        EnumerateSubNamesResult result;
        ErrorCode error = client.EndEnumerateSubNames(ptr, result);
        Trace.WriteInfo(
            Constants::TestSource,
            "EndEnumerateSubNames for {0} returned error {1}; expected {2}", 
            nameToTest.ToString(),
            error,
            expected);
        VERIFY_ARE_EQUAL(expected, error.ReadValue());
    }

    void ClientAndEntreeCommunicationTest::SynchronousCreateNameWithRetries(
        __in FabricClientImpl & client,
        NamingUri const & nameToTest,
        int retries)
    {
        ErrorCode previousError;
        int tries = 0;
        AutoResetEvent wait;
        do
        {
            auto ptr = client.BeginCreateName(
                nameToTest,
                TimeSpan::FromSeconds(10),
                [this, &wait] (AsyncOperationSPtr const &) -> void
                {                    
                    wait.Set();
                },
                root_->CreateAsyncOperationRoot());

				VERIFY_IS_TRUE_FMT(
                wait.WaitOne(TimeSpan::FromSeconds(15)),
                "CreateName {0} completed on time", nameToTest.ToString().c_str());

            ErrorCode error = client.EndCreateName(ptr);
            Trace.WriteInfo(Constants::TestSource, "EndCreateName {0} returned error: {1} on try {2}", nameToTest.ToString(), error, tries);
            previousError = error;
            ++tries;
        }
        while (!previousError.IsSuccess() && tries < retries);

        VERIFY_IS_TRUE(previousError.IsSuccess());
        VERIFY_IS_TRUE(store_.NameExists(nameToTest));
    }

    vector<byte> ClientAndEntreeCommunicationTest::GetBytes(wstring const & content)
    {
        vector<byte> data;
        data.insert(
            data.end(),
            reinterpret_cast<BYTE const *>(content.c_str()),
            reinterpret_cast<BYTE const *>(content.c_str() + content.size() + 1));

        return move(data);
    }
}
