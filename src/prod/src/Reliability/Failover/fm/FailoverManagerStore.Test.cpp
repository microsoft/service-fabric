// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace FailoverManagerUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Federation;
    using namespace Reliability;
    using namespace Reliability::FailoverManagerComponent;

    class FailoverManagerStoreTest
    {
    protected:
        FailoverManagerStoreTest() { BOOST_REQUIRE(TestSetup(testStoreType)); }
    
        ~FailoverManagerStoreTest() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_CLEANUP(TestCleanup);

        static wstring const testStoreType;

    public:
        static wstring const FMStoreDirectoryPrefix;
        static wstring const FMStoreFilenamePrefix;
        static wstring const FMStoreFileExtension;
        bool TestSetup(wstring storetype);
        void BasicFailoverManagerStoreTest(const wstring StoreType);
        shared_ptr<FailoverManagerStore> InitializeStore(
            wstring ownerId,
            bool shouldPass,
            bool existingStore,
            Common::Guid const & partitionId,
            ::FABRIC_REPLICA_ID replicaId,
            Common::ComponentRoot const & root,
            const wstring storeType);

        FailoverUnitUPtr CreateFailoverUnit(
            ConsistencyUnitDescription const& consistencyUnitDescription,
            ServiceInfoSPtr & serviceInfo,
            NodeInfoSPtr & node);

        ServiceInfoSPtr CreateServiceInfo(wstring name, ServiceTypeSPtr const& serviceType);

        NodeInfoSPtr CreateNodeInfo(int64 low);
    };

    BOOST_FIXTURE_TEST_SUITE(FailoverManagerStoreTestSuite, FailoverManagerStoreTest)

    BOOST_AUTO_TEST_CASE(BasicFailoverManagerStoreTestCase)
    {
#if !defined(PLATFORM_UNIX)
        BasicFailoverManagerStoreTest(testStoreType);
#endif
    }

    BOOST_AUTO_TEST_SUITE_END()

    wstring const FailoverManagerStoreTest::testStoreType(L"ESENT");
    wstring const FailoverManagerStoreTest::FMStoreDirectoryPrefix(L"FMStoreTestFiles");
    wstring const FailoverManagerStoreTest::FMStoreFilenamePrefix(L"FMStore");
    wstring const FailoverManagerStoreTest::FMStoreFileExtension(L"edb");

    wstring GetEseDirectory()
    {
        wstring directory;
        StringWriter writer(directory);
        writer.Write(FailoverManagerStoreTest::FMStoreDirectoryPrefix);
        writer.Write(L"\\");

        wstring rootPath;
        Environment::GetEnvironmentVariable(
            wstring(L"TEMP"),
            rootPath,
            Environment::GetExecutablePath());

        return Path::Combine(rootPath, directory);
    }

    wstring GetEseFilename()
    {
        wstring filename;
        StringWriter writer(filename);

        writer.Write(FailoverManagerStoreTest::FMStoreFilenamePrefix);
        writer.Write(L".");
        writer.Write(FailoverManagerStoreTest::FMStoreFileExtension);

        return filename;
    }

    void DeleteStoreFiles()
    {
        wstring directory = GetEseDirectory();

        if (Directory::Exists(directory))
        {
            int retryCount = 10;
            ErrorCode error = Directory::Delete(directory, true);
            while (!error.IsSuccess() && retryCount > 0)
            {
                Trace.WriteWarning(TestConstants::TestSource, "Unable to delete store directory {0} due to error {1}. Retrying...", directory, error);
                Sleep(1000);
                error = Directory::Delete(directory, true);
                retryCount--;
            }

            ASSERT_IFNOT(error.IsSuccess(), "Unable to delete store directory {0} due to error {1}", directory, error);
        }
    }

    FailoverUnitUPtr FailoverManagerStoreTest::CreateFailoverUnit(
        ConsistencyUnitDescription const& consistencyUnitDescription,
        ServiceInfoSPtr & serviceInfo,
        NodeInfoSPtr & node)
    {
        auto failoverUnit = new FailoverUnit(
            FailoverUnitId(consistencyUnitDescription.ConsistencyUnitId.Guid),
            consistencyUnitDescription,
            true,
            true,
            serviceInfo->Name,
            serviceInfo);

        FailoverUnitUPtr failoverUnitPtr = FailoverUnitUPtr(failoverUnit);
        failoverUnitPtr->CreateReplica(NodeInfoSPtr(node));
        return failoverUnitPtr;
    }

    ServiceInfoSPtr FailoverManagerStoreTest::CreateServiceInfo(wstring name, ServiceTypeSPtr const& serviceType)
    {
        wstring placementConstraints = L"";
        int scaleoutCount = 0;
        return make_shared<ServiceInfo>(
            ServiceDescription(name, 0, 0, 10, 3, 2, true, true, TimeSpan::FromSeconds(60.0), TimeSpan::MaxValue, TimeSpan::FromSeconds(300.0), serviceType->Type, vector<ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<ServiceLoadMetricDescription>(), 0, vector<byte>()),
            serviceType,
            FABRIC_INVALID_SEQUENCE_NUMBER,
            false);
    }

    NodeInfoSPtr FailoverManagerStoreTest::CreateNodeInfo(int64 low)
    {
        NodeId nodeId(LargeInteger(0, low));
        NodeInstance nodeInstance(nodeId, 0);
        Common::FabricVersionInstance versionInstance;
        std::wstring defaultNodeUpgradeDomainId;
        std::vector<Common::Uri> defaultNodeFaultDomainIds;
        std::map<std::wstring, std::wstring> defaultNodeProperties;
        std::map<std::wstring, uint> defaultCapacityRatios;
        std::map<std::wstring, uint> defaultCapacities;
        std::wstring defaultNodeName;
        std::wstring defaultNodeType;
        std::wstring defaultIpAddressOrFQDN;
        ULONG defaultClientConnectionPort = 0;

        NodeDescription nodeDescription(
            versionInstance,
            defaultNodeUpgradeDomainId,
            defaultNodeFaultDomainIds,
            defaultNodeProperties,
            defaultCapacityRatios,
            defaultCapacities,
            defaultNodeName,
            defaultNodeType,
            defaultIpAddressOrFQDN,
            defaultClientConnectionPort,
            0);

        return make_shared<NodeInfo>(nodeInstance, move(nodeDescription), true);
    }

    bool FailoverManagerStoreTest::TestSetup(wstring storeType)
    {
        FailoverConfig::GetConfig().Test_Reset();

        if (storeType == L"ESENT")
        {
            DeleteStoreFiles();
            Directory::Create(GetEseDirectory());
        }

        return true;
    }

    bool FailoverManagerStoreTest::TestCleanup()
    {
        return true;
    }
    shared_ptr<FailoverManagerStore> FailoverManagerStoreTest::InitializeStore(
        wstring ownerId,
        bool shouldPass,
        bool existingStore,
        Common::Guid const & partitionId,
        ::FABRIC_REPLICA_ID replicaId,
        Common::ComponentRoot const & root,
        const wstring storeType)
    {
        UNREFERENCED_PARAMETER(ownerId);
        UNREFERENCED_PARAMETER(shouldPass);


        if (storeType == L"ESENT")
        {
            auto replicatedStore = Store::KeyValueStoreReplica::CreateForUnitTests(
                partitionId,
                replicaId,
                Store::EseLocalStoreSettings(GetEseFilename(), GetEseDirectory()),
                root);
            ErrorCode error = replicatedStore->InitializeLocalStoreForUnittests(existingStore);

            shared_ptr<FailoverManagerStore> store = make_shared<FailoverManagerStore>(move(replicatedStore));

            if (shouldPass)
            {
                VERIFY_ARE_EQUAL(ErrorCodeValue::Success, error.ReadValue(), L"store.Open did not return success");
                return store;
            }
            else
            {
                VERIFY_ARE_NOT_EQUAL(ErrorCodeValue::Success, error.ReadValue(), L"store.Open returned success");
                return nullptr;
            }
        }

        Common::Assert::CodingError("StoreType not found in properties");
    }
    void FailoverManagerStoreTest::BasicFailoverManagerStoreTest(const wstring storeType)
    {
        Config cfg;

        wstring ownerId = L"TestOwner1";
        shared_ptr<ComponentRoot> componentRoot = make_shared<ComponentRoot>();
        Common::Guid partitionId = Guid::NewGuid();
        ::FABRIC_REPLICA_ID replicaId = 0;
        shared_ptr<FailoverManagerStore> storeSPtr = InitializeStore(ownerId, true, false, partitionId, replicaId, *componentRoot, storeType);
        std::vector<FailoverUnitUPtr> failoverUnits;
        std::vector<ServiceTypeSPtr> serviceTypes;
        std::vector<ServiceInfoSPtr> serviceInfos;
        std::vector<NodeInfoSPtr> nodes;

        int64 commitDuration;

        //Verify FM store is empty
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->LoadAll(serviceTypes)).ReadValue(), L"GetAllServiceTypes did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->LoadAll(serviceInfos)).ReadValue(), L"GetAllServiceInfos did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->LoadAll(failoverUnits)).ReadValue(), L"GetAllFailoverUnits did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->LoadAll(nodes)).ReadValue(), L"GetNodes did not return success");

        VERIFY_ARE_EQUAL(0u, failoverUnits.size(), L"GetAllFailoverUnits did not return empty FailoverUnit vector");
        VERIFY_ARE_EQUAL(0u, serviceTypes.size(), L"GetAllServiceTypes did not return empty ServiceType vector");
        VERIFY_ARE_EQUAL(0u, serviceInfos.size(), L"GetAllServiceInfos did not return empty service info vector");
        VERIFY_ARE_EQUAL(0u, nodes.size(), L"GetNodes did not return empty vector");

        //Add items
        ServiceModel::ApplicationIdentifier appId;
        ServiceModel::ApplicationIdentifier::FromString(L"TestApp_App0", appId);
        ApplicationInfoSPtr applicationInfo = make_shared<ApplicationInfo>(appId, NamingUri(L"fabric:/TestApp"), 1);
        ApplicationEntrySPtr applicationEntry = make_shared<CacheEntry<ApplicationInfo>>(move(applicationInfo));
        ServiceTypeSPtr serviceType = make_shared<ServiceType>(ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(appId, L"TestPackage"), L"TestServiceType"), applicationEntry);
        ServiceInfoSPtr serviceInfo = CreateServiceInfo(L"TestService", serviceType);
        NodeInfoSPtr nodeInfo = CreateNodeInfo(100);
        FailoverUnitUPtr failoverunit = CreateFailoverUnit(ConsistencyUnitDescription(), serviceInfo, nodeInfo);

        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->UpdateData(*serviceType, commitDuration)).ReadValue(), L"UpdateServiceType did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->UpdateData(*serviceInfo, commitDuration)).ReadValue(), L"UpdateServiceInfo did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->UpdateData(*failoverunit, commitDuration)).ReadValue(), L"UpdateFailoverUnit did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->UpdateData(*nodeInfo, commitDuration)).ReadValue(), L"UpdateNode did not return success");

        //Read back items
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->LoadAll(serviceTypes)).ReadValue(), L"GetAllServiceTypes did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->LoadAll(serviceInfos)).ReadValue(), L"GetAllServiceInfos did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->LoadAll(failoverUnits)).ReadValue(), L"GetAllFailoverUnits did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->LoadAll(nodes)).ReadValue(), L"GetNodes did not return success");

        VERIFY_ARE_EQUAL(1u, failoverUnits.size(), L"GetAllFailoverUnits did not return 1 FailoverUnit");
        VERIFY_ARE_EQUAL(1u, failoverUnits[0]->UpReplicaCount, L"FailoverUnit doesn't have 1 replica");
        VERIFY_ARE_EQUAL(1u, serviceTypes.size(), L"GetAllServiceTypes did not return 1 ServiceType");
        VERIFY_ARE_EQUAL(1u, serviceInfos.size(), L"GetAllServiceInfos did not return 1 service info");
        VERIFY_ARE_EQUAL(1u, nodes.size(), L"GetNodes did not return 1 node");

        // update the service info and node info which is required for verify consistency
        serviceInfos[0] = make_shared<ServiceInfo>(serviceInfos[0]->ServiceDescription, serviceTypes[0], FABRIC_INVALID_SEQUENCE_NUMBER, false);
        failoverUnits[0]->ServiceInfoObj = serviceInfos[0];
        failoverUnits[0]->ForEachReplica([&](Replica & replica)
        {
            replica.NodeInfoObj = nodes[0];
        });
        failoverUnits[0]->VerifyConsistency();

        //Delete FailoverUnit
        failoverunit->PersistenceState = PersistenceState::ToBeDeleted;
        FailoverUnitUPtr deletedFailoverunit;
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (storeSPtr->UpdateData(*failoverunit, commitDuration)).ReadValue(), L"UpdateFailoverUnit did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::FMFailoverUnitNotFound, (storeSPtr->GetFailoverUnit(failoverunit->Id, deletedFailoverunit)).ReadValue(), L"GetFailoverUnit did not return FailoverUnitNotFound");

        //Create new store
        wstring newOwnerId = L"TestOwner2";

        Store::StoreConfig::GetConfig().OpenDatabaseRetryCount = 1;
        
        //After reset the new store create should succeed
        storeSPtr->Dispose(true /* isStoreCloseNeeded */);
        storeSPtr.reset();
        auto newStoreSPtr = InitializeStore(newOwnerId, true, true, partitionId, replicaId, *componentRoot, storeType);

        //New store should work just fine
        failoverunit->PersistenceState = PersistenceState::ToBeInserted;
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (newStoreSPtr->UpdateData(*failoverunit, commitDuration)).ReadValue(), L"UpdateFailoverUnit did not return success");

        failoverUnits.clear();
        serviceInfos.clear();
        nodes.clear();

        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (newStoreSPtr->LoadAll(serviceInfos)).ReadValue(), L"GetAllServiceInfos did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (newStoreSPtr->LoadAll(failoverUnits)).ReadValue(), L"GetAllFailoverUnits did not return success");
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, (newStoreSPtr->LoadAll(nodes)).ReadValue(), L"GetNodes did not return success");

        VERIFY_ARE_EQUAL(1u, failoverUnits.size(), L"GetAllFailoverUnits did not return empty FailoverUnit vector");
        VERIFY_ARE_EQUAL(1u, serviceInfos.size(), L"GetAllFailoverUnits did not return empty service info vector");
        VERIFY_ARE_EQUAL(1u, nodes.size(), L"GetNodes did not return empty vector");

        //Now dispose the new store
        newStoreSPtr->Dispose(true /* isStoreCloseNeeded */);

        VERIFY_ARE_EQUAL(ErrorCodeValue::FMStoreNotUsable, (newStoreSPtr->LoadAll(serviceInfos)).ReadValue(), L"GetAllServiceInfos did not return FailoverManagerStoreDisposed");
        VERIFY_ARE_EQUAL(ErrorCodeValue::FMStoreNotUsable, (newStoreSPtr->LoadAll(failoverUnits)).ReadValue(), L"GetAllFailoverUnits did not return FailoverManagerStoreDisposed");
        VERIFY_ARE_EQUAL(ErrorCodeValue::FMStoreNotUsable, (newStoreSPtr->LoadAll(nodes)).ReadValue(), L"GetNodes did not return FailoverManagerStoreDisposed");

        VERIFY_ARE_EQUAL(ErrorCodeValue::FMStoreNotUsable, (newStoreSPtr->UpdateData(*serviceInfo, commitDuration)).ReadValue(), L"UpdateServiceInfo did not return FailoverManagerStoreDisposed");
        VERIFY_ARE_EQUAL(ErrorCodeValue::FMStoreNotUsable, (newStoreSPtr->UpdateData(*failoverunit, commitDuration)).ReadValue(), L"UpdateFailoverUnit did not return FailoverManagerStoreDisposed");
        VERIFY_ARE_EQUAL(ErrorCodeValue::FMStoreNotUsable, (newStoreSPtr->UpdateData(*nodeInfo, commitDuration)).ReadValue(), L"UpdateNode did not return FailoverManagerStoreDisposed");
    }
}
