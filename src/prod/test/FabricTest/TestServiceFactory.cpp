// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace std;
using namespace Common;
using namespace TestCommon;
using namespace TestHooks;

StringLiteral const TraceSource("FabricTest.StatelessServiceFactory");

TestServiceFactory::TestServiceFactory(
    Federation::NodeId nodeId,
    Api::IClientFactoryPtr const & clientFactory)
    : nodeId_(nodeId),
    clientFactory_(clientFactory),
    activeServices_(), 
    storeServices_(),
    calculatorServices_(),
    lock_()
{
    TestSession::WriteNoise(TraceSource, "Ctor TestServiceFactory at {0}", nodeId_);

    //Insert all known service types into this vector
    supportedStatelessServiceTypes_.push_back(CalculatorService::DefaultServiceType);
    supportedStatefulServiceTypes_.push_back(TestStoreService::DefaultServiceType);
    supportedStatefulServiceTypes_.push_back(TestPersistedStoreService::DefaultServiceType);
    supportedStatefulServiceTypes_.push_back(TXRService::DefaultServiceType);
	supportedStatefulServiceTypes_.push_back(TStoreService::DefaultServiceType);
}

TestServiceFactory::~TestServiceFactory()
{
    TestSession::WriteNoise(TraceSource, "Dtor TestServiceFactory at {0}", nodeId_);
}

CalculatorServiceSPtr TestServiceFactory::CreateCalculatorService(FABRIC_PARTITION_ID partitionId, wstring serviceName, wstring initDataStr)
{
    TestSession::WriteNoise(TraceSource, "Creating CalculatorServiceType with name {0} and initialization-data:'{1}'", serviceName, initDataStr);

    CalculatorServiceSPtr calculatorServiceSPtr = make_shared<CalculatorService>(nodeId_, partitionId, serviceName, initDataStr);

    //Keeps the TestServiceFactory alive
    auto selfRoot = CreateComponentRoot();

    calculatorServiceSPtr->OnOpenCallback = 
        [this, selfRoot] (TestServiceInfo const & serviceInfo, ComponentRootSPtr const& root, bool hasPersistedState)
    {
        ProcessOpenCallback(serviceInfo, root, hasPersistedState);
    };

    calculatorServiceSPtr->OnCloseCallback = 
        [this, selfRoot] (wstring const& location)
    {
        ProcessCloseCallback(location);
    };

    return calculatorServiceSPtr;
}

TestStoreServiceSPtr TestServiceFactory::CreateTestStoreService(FABRIC_PARTITION_ID partitionId, wstring serviceName, wstring initDataStr)
{
    TestSession::WriteNoise(TraceSource, "Creating TestStoreServiceType with name {0} and initialization-data:'{1}'", serviceName, initDataStr);

    TestStoreServiceSPtr storeServiceSPtr = make_shared<TestStoreService>(
        partitionId,
        serviceName, 
        nodeId_, 
        FabricTestSessionConfig::GetConfig().ChangeServiceLocationOnChangeRole,
        initDataStr);

    //Keeps the TestServiceFactory alive
    auto selfRoot = CreateComponentRoot();

    storeServiceSPtr->OnOpenCallback = 
        [this, selfRoot] (TestServiceInfo const & serviceInfo, ComponentRootSPtr const& root, bool hasPersistedState)
    {
        ProcessOpenCallback(serviceInfo, root, hasPersistedState);
    };

    storeServiceSPtr->OnCloseCallback = 
        [this, selfRoot] (wstring const& location)
    {
        ProcessCloseCallback(location);
    };

    storeServiceSPtr->OnChangeRoleCallback = 
        [this, selfRoot] (wstring const& oldLocation, wstring const& newLocation, FABRIC_REPLICA_ROLE replicaRole)
    {
        ProcessChangeRoleCallback(oldLocation, newLocation, replicaRole);
    };

    return storeServiceSPtr;
}

TestPersistedStoreServiceSPtr TestServiceFactory::CreateTestPersistedStoreService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    wstring serviceName,
    wstring initDataStr)
{
    TestSession::WriteInfo(TraceSource, "Creating TestPersistedStoreServiceType with name {0} and initialization-data:'{1}'", serviceName, initDataStr);

    auto storeServiceSPtr = make_shared<TestPersistedStoreService>(
        Guid(partitionId),
        replicaId,
        serviceName,
        nodeId_,
        FabricTestSessionConfig::GetConfig().ChangeServiceLocationOnChangeRole,
        FABRICSESSION,
        initDataStr,
        TestPersistedStoreService::DefaultServiceType,
        L"");

    ReplicatorSettingServiceInitDataParser rsParser(initDataStr);
    auto replicatorSettings = rsParser.CreateReplicatorSettings(initDataStr, Guid(partitionId).ToString());

    ServiceInitDataParser parser(initDataStr);
    bool enableIncrementalBackup = false;
    parser.GetValue<bool>(L"Store_EnableIncrementalBackup", enableIncrementalBackup);

    auto & config = FabricTestCommonConfig::GetConfig();
    auto workingDir = FABRICSESSION.FabricDispatcher.GetWorkingDirectory(nodeId_.ToString());

    auto error = storeServiceSPtr->InitializeForTestServices(
        config.EnableTStore,
        workingDir,
        TestPersistedStoreService::TestPersistedStoreDirectory,
        TestPersistedStoreService::SharedLogFilename, 
        config,
        Guid(partitionId),
        move(replicatorSettings),
        Store::ReplicatedStoreSettings(
            StoreConfig::GetConfig().EnableUserServiceFlushOnDrain,
            Store::SecondaryNotificationMode::BlockSecondaryAck),
        Store::EseLocalStoreSettings(
            TestPersistedStoreService::EseDatabaseFilename, 
            Path::Combine(workingDir, TestPersistedStoreService::TestPersistedStoreDirectory),
            enableIncrementalBackup),
        clientFactory_,
        NamingUri(serviceName),
        false); // allowRepairUpToQuorum

    TestSession::FailTestIf(!error.IsSuccess(), "InitializeForTestServices({0}) failed: {1}", serviceName, error);

    storeServiceSPtr->ReplicatedStore->Test_SetTestHookContext(TestHookContext(nodeId_, serviceName));

    //Keeps the TestServiceFactory alive
    auto selfRoot = CreateComponentRoot();

    storeServiceSPtr->OnOpenCallback = 
        [this, selfRoot] (TestServiceInfo const & serviceInfo, ComponentRootSPtr const& root, bool hasPersistedState)
    {
        ProcessOpenCallback(serviceInfo, root, hasPersistedState);
    };

    storeServiceSPtr->OnCloseCallback = 
        [this, selfRoot] (wstring const& location)
    {
        ProcessCloseCallback(location);
    };

    storeServiceSPtr->OnChangeRoleCallback = 
        [this, selfRoot] (wstring const& oldLocation, wstring const& newLocation, FABRIC_REPLICA_ROLE replicaRole)
    {
        ProcessChangeRoleCallback(oldLocation, newLocation, replicaRole);
    };

    return storeServiceSPtr;
}

TXRServiceSPtr TestServiceFactory::CreateTXRService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    wstring serviceName,
    wstring initDataStr)
{
    TestSession::WriteInfo(TraceSource, "Creating TXRService with name {0} and initialization-data:'{1}'", serviceName, initDataStr);

    TXRServiceSPtr txrServiceSPtr = shared_ptr<TXRService>(
        new TXRService(
            partitionId,
            replicaId,
            serviceName,
            nodeId_,
            FABRICSESSION.FabricDispatcher.GetWorkingDirectory(nodeId_.ToString()),
            FABRICSESSION,
            initDataStr));

    //Keeps the TestServiceFactory alive
    auto selfRoot = CreateComponentRoot();

    txrServiceSPtr->OnOpenCallback = 
        [this, selfRoot] (TestServiceInfo const & serviceInfo, ComponentRootSPtr const& root, bool hasPersistedState)
    {
        ProcessOpenCallback(serviceInfo, root, hasPersistedState);
    };

    txrServiceSPtr->OnCloseCallback = 
        [this, selfRoot] (wstring const& location)
    {
        ProcessCloseCallback(location);
    };

    txrServiceSPtr->OnChangeRoleCallback = 
        [this, selfRoot] (wstring const& oldLocation, wstring const& newLocation, FABRIC_REPLICA_ROLE replicaRole)
    {
        ProcessChangeRoleCallback(oldLocation, newLocation, replicaRole);
    };

    return txrServiceSPtr;
}

TStoreServiceSPtr TestServiceFactory::CreateTStoreService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    wstring serviceName,
    wstring initDataStr)
{
    TestSession::WriteInfo(TraceSource, "Creating StoreService with name {0} and initialization-data:'{1}'", serviceName, initDataStr);

    TStoreServiceSPtr tstoreServiceSPtr = shared_ptr<TStoreService>(
        new TStoreService(
            partitionId,
            replicaId,
            serviceName,
            nodeId_,
            FABRICSESSION.FabricDispatcher.GetWorkingDirectory(nodeId_.ToString()),
            FABRICSESSION,
            initDataStr));

    //Keeps the TestServiceFactory alive
    auto selfRoot = CreateComponentRoot();

    tstoreServiceSPtr->OnOpenCallback =
        [this, selfRoot] (TestServiceInfo const & serviceInfo, ComponentRootSPtr const& root, bool hasPersistedState)
    {
        ProcessOpenCallback(serviceInfo, root, hasPersistedState);
    };

    tstoreServiceSPtr->OnCloseCallback =
        [this, selfRoot] (wstring const& location)
    {
        ProcessCloseCallback(location);
    };

    tstoreServiceSPtr->OnChangeRoleCallback =
        [this, selfRoot] (wstring const& oldLocation, wstring const& newLocation, FABRIC_REPLICA_ROLE replicaRole)
    {
        ProcessChangeRoleCallback(oldLocation, newLocation, replicaRole);
    };

    return tstoreServiceSPtr;
}

bool TestServiceFactory::TryGetServiceInfo(wstring const& location, TestServiceInfo & testServiceInfo) const
{
    auto it = activeServices_.find(location);
    if (it != activeServices_.end())
    {
        testServiceInfo = (*it).second;
        return true;
    }

    return false;
}

void TestServiceFactory::ProcessChangeRoleCallback(wstring const& oldLocation, wstring const& newLocation, FABRIC_REPLICA_ROLE replicaRole)
{
    TestSession::WriteNoise(TraceSource, "ProcessChangeRoleCallback with new location {0} and old location {1}", newLocation, oldLocation);
    AcquireWriteLock grab(lock_);
    TestSession::FailTestIf(!ContainsServiceInfo(oldLocation), "{0} does not exist", oldLocation);
    if(newLocation.empty())
    {
        activeServices_[oldLocation].StatefulServiceRole = replicaRole;
    }
    else
    {
        activeServices_[newLocation] = activeServices_[oldLocation];
        activeServices_[newLocation].StatefulServiceRole = replicaRole;
        activeServices_[newLocation].ServiceLocation = newLocation;
        activeServices_.erase(oldLocation);
    }
}

bool TestServiceFactory::ContainsServiceInfo(wstring const& location) const
{
    TestServiceInfo testServiceInfo;
    return (TryGetServiceInfo(location, testServiceInfo));
}

void TestServiceFactory::ProcessOpenCallback(TestServiceInfo const & serviceInfo, ComponentRootSPtr const& root, bool)
{
    TestSession::WriteNoise(TraceSource, "ProcessOpenCallback with location {0}", serviceInfo.ServiceLocation);
    AcquireWriteLock grab(lock_);
    TestSession::FailTestIf(ContainsServiceInfo(serviceInfo.ServiceLocation), "{0} already exists", serviceInfo.ServiceLocation);
    activeServices_[serviceInfo.ServiceLocation] = serviceInfo;

    auto rootCopy = const_pointer_cast<ComponentRoot>(root);

    if(serviceInfo.ServiceType == TestStoreService::DefaultServiceType)
    {
        auto storeServiceSPtr = dynamic_pointer_cast<TestStoreService>(rootCopy);
        TestSession::FailTestIf(storeServiceSPtr.get() == nullptr, "Failed cast to derived service");
        storeServices_.push_back(storeServiceSPtr);
    }
    else if(serviceInfo.ServiceType == TestPersistedStoreService::DefaultServiceType)
    {
        auto storeServiceSPtr = dynamic_pointer_cast<TestPersistedStoreService>(rootCopy);
        TestSession::FailTestIf(storeServiceSPtr.get() == nullptr, "Failed cast to derived service");
        storeServices_.push_back(storeServiceSPtr);
    }
    else if(serviceInfo.ServiceType == CalculatorService::DefaultServiceType)
    {
        auto calculatorServiceSPtr = dynamic_pointer_cast<CalculatorService>(rootCopy);
        TestSession::FailTestIf(calculatorServiceSPtr.get() == nullptr, "Failed cast to derived service");
        calculatorServices_.push_back(calculatorServiceSPtr);
    }
    else if (serviceInfo.ServiceType == TXRService::DefaultServiceType)
    {
        auto txrServiceSPtr = dynamic_pointer_cast<TXRService>(rootCopy);
        TestSession::FailTestIf(txrServiceSPtr.get() == nullptr, "Failed cast to derived service");
        storeServices_.push_back(txrServiceSPtr);
    }
    else if (serviceInfo.ServiceType == TStoreService::DefaultServiceType)
    {
        auto tstoreServiceSPtr = dynamic_pointer_cast<TStoreService>(rootCopy);
        TestSession::FailTestIf(tstoreServiceSPtr.get() == nullptr, "Failed cast to derived service");
        storeServices_.push_back(tstoreServiceSPtr);
    }
    else
    {
        TestSession::FailTest("Unexpected service type {0}", serviceInfo.ServiceType);
    }
}

void TestServiceFactory::ProcessCloseCallback(wstring const& location)
{
    TestSession::WriteNoise(TraceSource, "ProcessCloseCallback with location '{0}'", location);

    AcquireWriteLock grab(lock_);
    TestSession::FailTestIf(!ContainsServiceInfo(location), "'{0}' does not exist", location);

    if(activeServices_[location].IsStateful)
    {
        DeleteTestStoreService(location);
    }
    else
    {
        DeleteCalculatorService(location);
    }

    activeServices_.erase(location);
}

void TestServiceFactory::DeleteCalculatorService(wstring const& location)
{
    for ( auto iterator = calculatorServices_.begin() ; iterator != calculatorServices_.end(); iterator++ )
    {
        if((*iterator)->ServiceLocation.compare(location) == 0)
        {
            calculatorServices_.erase(iterator);
            return;
        }
    }

    TestSession::FailTest("No calculator service found at location {0}", location);
}

void TestServiceFactory::DeleteTestStoreService(wstring const& location)
{
    for ( auto iterator = storeServices_.begin() ; iterator != storeServices_.end(); iterator++ )
    {
        if((*iterator)->GetServiceLocation().compare(location) == 0)
        {
            storeServices_.erase(iterator);
            return;
        }
    }

    TestSession::FailTest("No test store service found at location {0}", location);
}

void TestServiceFactory::DeleteAtomicGroupService(wstring const& location)
{
    for ( auto iterator = scriptableServices_.begin() ; iterator != scriptableServices_.end(); iterator++ )
    {
        if((*iterator)->GetServiceLocation().compare(location) == 0)
        {
            scriptableServices_.erase(iterator);
            return;
        }
    }

    TestSession::FailTest("No atomic group service found at location {0}", location);
}

void TestServiceFactory::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    AcquireReadLock grab(lock_);
    if(activeServices_.size() == 0)
    {
        writer.Write("No active services");
    }

    for ( auto iterator = activeServices_.begin() ; iterator != activeServices_.end(); iterator++ )
    {
        TestServiceInfo const & serviceInfo = (*iterator).second;
        writer.WriteLine("{0}", serviceInfo);
    }
}

bool TestServiceFactory::TryFindStoreService(std::wstring const& serviceLocation, ITestStoreServiceSPtr & storeServiceSPtr)
{
    AcquireReadLock grab(lock_);
    for ( auto iterator = storeServices_.begin() ; iterator != storeServices_.end(); iterator++ )
    {
        if((*iterator)->GetServiceLocation().compare(serviceLocation) == 0)
        {
            storeServiceSPtr = (*iterator);
            return true;
        }
    }

    return false;
}

bool TestServiceFactory::TryFindStoreService(std::wstring const & serviceName, NodeId const & nodeId, ITestStoreServiceSPtr & storeServiceSPtr)
{
    AcquireReadLock grab(lock_);
    for ( auto iterator = storeServices_.begin() ; iterator != storeServices_.end(); iterator++ )
    {
        if((*iterator)->GetServiceName().compare(serviceName) == 0 && (*iterator)->GetNodeId() == nodeId)
        {
            storeServiceSPtr = (*iterator);
            return true;
        }
    }

    return false;
}

bool TestServiceFactory::TryFindCalculatorService(std::wstring const & serviceName, NodeId const & nodeId, CalculatorServiceSPtr & calculatorServicePtr)
{
    AcquireReadLock grab(lock_);
    for ( auto iterator = calculatorServices_.begin() ; iterator != calculatorServices_.end(); iterator++ )
    {
        if((*iterator)->GetServiceName().compare(serviceName) == 0 && (*iterator)->GetNodeId() == nodeId)
        {
            calculatorServicePtr = (*iterator);
            return true;
        }
    }

    return false;
}

bool TestServiceFactory::TryFindScriptableService(__in std::wstring const & serviceLocation, __out ITestScriptableServiceSPtr & scriptableServiceSPtr)
{
    AcquireReadLock grab(lock_);
    for (auto iterator = begin(scriptableServices_); iterator != end(scriptableServices_); ++iterator)
    {
        if((*iterator)->GetServiceLocation().compare(serviceLocation) == 0)
        {
            scriptableServiceSPtr = (*iterator);
            return true;
        }
    }

    return false;
}

bool TestServiceFactory::TryFindScriptableService(__in std::wstring const & serviceName, Federation::NodeId const & nodeId, __out ITestScriptableServiceSPtr & scriptableServiceSPtr)
{
    AcquireReadLock grab(lock_);
    for (auto iterator = begin(scriptableServices_); iterator != end(scriptableServices_); ++iterator)
    {
        auto & entry = *iterator;
        if(entry->GetServiceName().compare(serviceName) == 0 && entry->GetNodeId() == nodeId)
        {
            scriptableServiceSPtr = entry;
            return true;
        }
    }

    return false;
}

