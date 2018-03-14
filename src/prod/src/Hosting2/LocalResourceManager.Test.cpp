// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Hosting2/FabricNodeHost.Test.h"

// Helper macro to change config value for one test only
#define HostingConfigScopeChange(key, type, newValue) \
    class ScopeChangeClass##key \
    { \
    public: \
    ScopeChangeClass##key() : original_(HostingConfig::GetConfig().key) {} \
    ~ScopeChangeClass##key() { SetValue(original_); } \
    void SetValue(type value) { HostingConfig::GetConfig().key = value; } \
    private: \
    type original_; \
    } ScopeChangeObject##key; \
    ScopeChangeObject##key.SetValue(newValue);

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

const StringLiteral TraceType("LocalResourceManagerTest");

class LocalResourceManagerTest
{
protected:
    LocalResourceManagerTest() :
        fabricNodeHost_(make_shared<TestFabricNodeHost>()) { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);

    ~LocalResourceManagerTest() { }

    LocalResourceManager & GetLocalResourceManager()
    {
        return *this->fabricNodeHost_->GetHosting().LocalResourceManagerObj;
    }

    bool OpenFabricNode(FabricNodeConfigSPtr config)
    {
        return fabricNodeHost_->Open(move(config));
    }

    bool CloseFabricNode() 
    {
        return fabricNodeHost_->Close();
    }

    FabricNodeConfigSPtr GetCapacityConfig(uint numCores, uint memoryInMb)
    {
        FabricNodeConfigSPtr defaultConfig = std::make_shared<FabricNodeConfig>();
        FabricNodeConfig::NodeCapacityCollectionMap defaultCapacities;
        defaultCapacities.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, numCores));
        defaultCapacities.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, memoryInMb));
        defaultConfig->NodeCapacities = defaultCapacities;
        return defaultConfig;
    }

    FabricNodeConfigSPtr GetDefaultCapacityConfig()
    {
        return GetCapacityConfig(numberOfAvailableCores_, availableMemoryInMB_);
    }

    Common::ErrorCode RegisterSharedServicePackage(LocalResourceManager & lrm,
        ServicePackageIdentifier spIdentifier,
        ServicePackageResourceGovernanceDescription rg)
    {
        return Hosting2::LocalResourceManagerTestHelper::RegisterServicePackage(lrm,
            ServicePackageInstanceIdentifier(spIdentifier, ServiceModel::ServicePackageActivationContext(), L""),
            rg);
    }

    void UnregisterSharedServicePackage(LocalResourceManager & lrm,
        ServicePackageIdentifier spIdentifier,
        ServicePackageResourceGovernanceDescription rg)
    {
        Hosting2::LocalResourceManagerTestHelper::UnregisterServicePackage(lrm,
            ServicePackageInstanceIdentifier(spIdentifier, ServiceModel::ServicePackageActivationContext(), L""),
            rg);
    }

    Common::ErrorCode RegisterServicePackageWithPartition(LocalResourceManager & lrm,
        ServicePackageIdentifier spIdentifier,
        Common::Guid partitionId,
        ServicePackageResourceGovernanceDescription rg)
    {
        return Hosting2::LocalResourceManagerTestHelper::RegisterServicePackage(lrm,
            ServicePackageInstanceIdentifier(spIdentifier, ServiceModel::ServicePackageActivationContext(partitionId), L""),
            rg);
    }

    void UnregisterServicePackageWithPartition(LocalResourceManager & lrm,
        ServicePackageIdentifier spIdentifier,
        Common::Guid partitionId,
        ServicePackageResourceGovernanceDescription rg)
    {
        Hosting2::LocalResourceManagerTestHelper::UnregisterServicePackage(lrm,
            ServicePackageInstanceIdentifier(spIdentifier, ServiceModel::ServicePackageActivationContext(partitionId), L""),
            rg);
    }

    ServicePackageResourceGovernanceDescription GetSPRGDescription(wstring spName, double numCores, uint memory)
    {
        ServicePackageResourceGovernanceDescription rg;
        rg.IsGoverned = true;
        rg.CpuCores = numCores;
        rg.MemoryInMB = memory;
        return rg;
    }

    void VerifyResourceUsage(LocalResourceManager & lrm, double numCPUsUsed, uint64 memoryInMBUsed)
    {
        VERIFY_ARE_EQUAL(numCPUsUsed, lrm.GetResourceUsage(*ServiceModel::Constants::SystemMetricNameCpuCores));
        VERIFY_ARE_EQUAL(memoryInMBUsed, lrm.GetResourceUsage(*ServiceModel::Constants::SystemMetricNameMemoryInMB));
    }

    uint numberOfAvailableCores_;
    uint availableMemoryInMB_;
    shared_ptr<TestFabricNodeHost> fabricNodeHost_;
};


BOOST_FIXTURE_TEST_SUITE(LocalResourceManagerTestSuite, LocalResourceManagerTest)


BOOST_AUTO_TEST_CASE(TestOpen)
{
    // Test case:
    //  - Read how many resources are available on the machine.
    //  - All of the openings will be successful, so that Fabric can start at all.

    // Open fabric node with regular capacities 
    // Opening FabricNode will start opening LRM
    // Closing FabricNode -> closing LRM
    VERIFY_IS_TRUE(OpenFabricNode(GetDefaultCapacityConfig()));
    CloseFabricNode();

    //  - Try to open with 1 extra core, verify it's successful
    VERIFY_IS_TRUE(OpenFabricNode(GetCapacityConfig(numberOfAvailableCores_ + 1, availableMemoryInMB_)));
    CloseFabricNode();

    //  - Try to open with 1MB of extra memory, allow that so that Fabric can also start.
    VERIFY_IS_TRUE(OpenFabricNode(GetCapacityConfig(numberOfAvailableCores_, availableMemoryInMB_ + 1)));
    CloseFabricNode();
}

BOOST_AUTO_TEST_CASE(BasicRegisteringTest)
{
    // 1. Try to register SP that will need 1 extra core, check that it fails.
    // 2. Try to register SP that will need 1MB of extra memory, check that it fails.
    // 3. Register 2 SPs (if possible) that will use all resources.
    VERIFY_IS_TRUE(OpenFabricNode(GetDefaultCapacityConfig()));

    LocalResourceManager& lrm = GetLocalResourceManager();
    VerifyResourceUsage(lrm, 0, 0);

    wstring spName = L"ServicePackage1";
    ServicePackageIdentifier dummySPId(ApplicationIdentifier(L"AppTypeName", 1), spName);

    auto error = RegisterSharedServicePackage(lrm, dummySPId, GetSPRGDescription(spName, numberOfAvailableCores_ + 1, availableMemoryInMB_));
    VERIFY_IS_TRUE(error.ReadValue() == ErrorCodeValue::NotEnoughCPUForServicePackage);

    VerifyResourceUsage(lrm, 0, 0);

    error = RegisterSharedServicePackage(lrm, dummySPId, GetSPRGDescription(spName, numberOfAvailableCores_, availableMemoryInMB_ + 1024));
    VERIFY_IS_TRUE(error.ReadValue() == ErrorCodeValue::NotEnoughMemoryForServicePackage);

    VerifyResourceUsage(lrm, 0, 0);

    // At least one core per SP
    uint numCoresPerSp = numberOfAvailableCores_ > 1 ? numberOfAvailableCores_/ 2 : 1;
    uint memoryPerSp = availableMemoryInMB_ > 1 ? availableMemoryInMB_ / 2 : 1;

    error = RegisterSharedServicePackage(lrm,
        ServicePackageIdentifier(ApplicationIdentifier(L"AppTypeName", 1), L"SP1"),
        GetSPRGDescription(L"SP1", numCoresPerSp, memoryPerSp));
    VERIFY_IS_TRUE(error.IsSuccess());

    VerifyResourceUsage(lrm, numCoresPerSp, memoryPerSp);

    if (numberOfAvailableCores_ > 1 && availableMemoryInMB_ > 1)
    {
        error = RegisterSharedServicePackage(lrm,
            ServicePackageIdentifier(ApplicationIdentifier(L"AppTypeName", 1), L"SP2"),
            GetSPRGDescription(L"SP2", numCoresPerSp, memoryPerSp));
        VERIFY_IS_TRUE(error.IsSuccess());
    }
    CloseFabricNode();
}

BOOST_AUTO_TEST_CASE(RegisterUnregisterTest)
{
    // Test case:
    //  - Create SP (large SP) that will use all available memory and CPU.
    //  - Try to create another SP (small SP), verify that it fails.
    //  - Unregister first SP
    //  - Try to register smaller SP again, check that it will pass.
    //  - Try to register large SP again, check that it fails.
    //  - Register another small SP, and unregister it.
    //  - Unregister all SPs
    //  - Register large SP again, check that it passes.
    VERIFY_IS_TRUE(OpenFabricNode(GetDefaultCapacityConfig()));
    LocalResourceManager& lrm = GetLocalResourceManager();

    wstring spName = L"ServicePackage1";
    ServicePackageIdentifier wholeNodeSPId(ApplicationIdentifier(L"AppTypeName", 1), spName);
    auto largeRGSettings = GetSPRGDescription(spName, numberOfAvailableCores_, availableMemoryInMB_);
    auto error = RegisterSharedServicePackage(lrm, wholeNodeSPId, largeRGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    VerifyResourceUsage(lrm, numberOfAvailableCores_, availableMemoryInMB_);

    ServicePackageIdentifier smallPackage1(ApplicationIdentifier(L"SmallAppTypeName", 1), spName);
    ServicePackageIdentifier smallPackage2(ApplicationIdentifier(L"SmallAppTypeName", 2), spName);

    // At least one core per SP
    uint numCoresPerSp = numberOfAvailableCores_ > 1 ? numberOfAvailableCores_ / 2 : 1;
    uint memoryPerSp = availableMemoryInMB_ > 1 ? availableMemoryInMB_ / 2 : 1;
    auto smallRGSettings = GetSPRGDescription(spName, numCoresPerSp, memoryPerSp);
    error = RegisterSharedServicePackage(lrm, smallPackage1, smallRGSettings);

    // This one needs to fail because entire node is used.
    VERIFY_IS_FALSE(error.IsSuccess());

    UnregisterSharedServicePackage(lrm, wholeNodeSPId, largeRGSettings);

    // Retry the registraion of smaller package.
    error = RegisterSharedServicePackage(lrm, smallPackage1, smallRGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    VerifyResourceUsage(lrm, numCoresPerSp, memoryPerSp);

    // Try the registration of large package and check that it fails.
    error = RegisterSharedServicePackage(lrm, wholeNodeSPId, largeRGSettings);
    VERIFY_IS_FALSE(error.IsSuccess());

    if (numberOfAvailableCores_ > 1 && availableMemoryInMB_ > 1)
    {
        // If possible, allocate one more SP
        error = RegisterSharedServicePackage(lrm, smallPackage2, smallRGSettings);
        VERIFY_IS_TRUE(error.IsSuccess());

        UnregisterSharedServicePackage(lrm, smallPackage2, smallRGSettings);

        // Try the registration of large package and check that it fails (see that unregister did not break something).
        error = RegisterSharedServicePackage(lrm, wholeNodeSPId, largeRGSettings);
        VERIFY_IS_FALSE(error.IsSuccess());
    }

    UnregisterSharedServicePackage(lrm, smallPackage1, smallRGSettings);

    // LRM is now empty, this should pass
    error = RegisterSharedServicePackage(lrm, wholeNodeSPId, largeRGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    CloseFabricNode();
}

BOOST_AUTO_TEST_CASE(MultipleRegistrationsWithMemoryTest)
{
    // Test case: multiple registrations with SPs that are using only memory.
    HostingConfigScopeChange(LocalResourceManagerTestMode, bool, true);
    VERIFY_IS_TRUE(OpenFabricNode(GetCapacityConfig(32 /* cores */, 2048 /* MB */)));
    LocalResourceManager& lrm = GetLocalResourceManager();

    VerifyResourceUsage(lrm, 0, 0);

    // This one is impossible to register
    wstring spName = L"ServicePackage1";
    ServicePackageIdentifier impossibleSPId(ApplicationIdentifier(L"AppTypeName", 314), spName);
    auto impossibleMemoryRGSettings = GetSPRGDescription(spName, 1, 3141592);
    auto error = RegisterSharedServicePackage(lrm, impossibleSPId, impossibleMemoryRGSettings);
    VERIFY_IS_FALSE(error.IsSuccess());

    VerifyResourceUsage(lrm, 0, 0);

    // Register a SP that will use half of the available memory.
    ServicePackageIdentifier halfNodeSP1Id(ApplicationIdentifier(L"AppTypeName", 1), spName);
    auto halfNodeMemoryRGSettings = GetSPRGDescription(spName, 1, 1024);
    error = RegisterSharedServicePackage(lrm, halfNodeSP1Id, halfNodeMemoryRGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    VerifyResourceUsage(lrm, 1, 1024);

    // Try to register a SP that will use 3/4 of available memory.
    ServicePackageIdentifier twoThirdsNodeSP2Id(ApplicationIdentifier(L"AppTypeName", 2), spName);
    auto twoThirdsNodeMemoryRGSettings = GetSPRGDescription(spName, 1, 1536);
    error = RegisterSharedServicePackage(lrm, twoThirdsNodeSP2Id, twoThirdsNodeMemoryRGSettings);
    VERIFY_IS_FALSE(error.IsSuccess());

    VerifyResourceUsage(lrm, 1, 1024);

    // Register one more SP with that will use half of memory
    ServicePackageIdentifier halfNodeSP3Id(ApplicationIdentifier(L"AppTypeName", 3), spName);
    error = RegisterSharedServicePackage(lrm, halfNodeSP3Id, halfNodeMemoryRGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    VerifyResourceUsage(lrm, 2, 2048);

    // Unregister both packages.
    UnregisterSharedServicePackage(lrm, halfNodeSP1Id, halfNodeMemoryRGSettings);
    VerifyResourceUsage(lrm, 1, 1024);

    UnregisterSharedServicePackage(lrm, halfNodeSP3Id, halfNodeMemoryRGSettings);
    VerifyResourceUsage(lrm, 0, 0);

    // Register SP that will use full node memory.
    error = RegisterSharedServicePackage(lrm, twoThirdsNodeSP2Id, twoThirdsNodeMemoryRGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());
    VerifyResourceUsage(lrm, 1, 1536);

    CloseFabricNode();
}

BOOST_AUTO_TEST_CASE(MultipleRegistrationsWithCPUTest)
{
    // Test case: multiple registrations with SPs that are using only CPU.
    HostingConfigScopeChange(LocalResourceManagerTestMode, bool, true);
    VERIFY_IS_TRUE(OpenFabricNode(GetCapacityConfig(4 /* cores */, 2048 /* MB */)));
    LocalResourceManager& lrm = GetLocalResourceManager();

    // This one is impossible to register.
    wstring spName = L"ServicePackage1";
    ServicePackageIdentifier impossibleSPId(ApplicationIdentifier(L"AppTypeName", 314), spName);
    auto impossibleCPURGSettings = GetSPRGDescription(spName, 314, 1);
    auto error = RegisterSharedServicePackage(lrm, impossibleSPId, impossibleCPURGSettings);
    VERIFY_IS_FALSE(error.IsSuccess());

    // Register a SP that will use half of the available CPUs (2 cores).
    ServicePackageIdentifier halfNodeSP1Id(ApplicationIdentifier(L"AppTypeName", 1), spName);
    auto halfNodeCPURGSettings = GetSPRGDescription(spName, 2, 1);
    error = RegisterSharedServicePackage(lrm, halfNodeSP1Id, halfNodeCPURGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    // Try to register a SP that will use 3 out of 4 available CPUs.
    ServicePackageIdentifier twoThirdsNodeSP2Id(ApplicationIdentifier(L"AppTypeName", 2), spName);
    auto twoThirdsNodeCPURGSettings = GetSPRGDescription(spName, 3, 1);
    error = RegisterSharedServicePackage(lrm, twoThirdsNodeSP2Id, twoThirdsNodeCPURGSettings);
    VERIFY_IS_FALSE(error.IsSuccess());

    // Register one more SP with that will use half of cores
    ServicePackageIdentifier halfNodeSP3Id(ApplicationIdentifier(L"AppTypeName", 3), spName);
    error = RegisterSharedServicePackage(lrm, halfNodeSP3Id, halfNodeCPURGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    // Unregister both packages.
    UnregisterSharedServicePackage(lrm, halfNodeSP1Id, halfNodeCPURGSettings);
    UnregisterSharedServicePackage(lrm, halfNodeSP3Id, halfNodeCPURGSettings);

    // Register SP that will 3 out of 4 CPUs.
    error = RegisterSharedServicePackage(lrm, twoThirdsNodeSP2Id, twoThirdsNodeCPURGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    CloseFabricNode();
}

BOOST_AUTO_TEST_CASE(BasicSubcoreAllocationTest)
{
    // Test case: multiple registrations with SPs that are using subcore allocations.
    HostingConfigScopeChange(LocalResourceManagerTestMode, bool, true);
    VERIFY_IS_TRUE(OpenFabricNode(GetCapacityConfig(2 /* cores */, 2048 /* MB */)));
    LocalResourceManager& lrm = GetLocalResourceManager();

    VerifyResourceUsage(lrm, 0, 0);

    // This one is impossible to register (wants 2.5 cores out of 2 available).
    wstring spName = L"ServicePackage1";
    ServicePackageIdentifier impossibleSPId(ApplicationIdentifier(L"AppTypeName", 314), spName);
    auto impossibleMemoryRGSettings = GetSPRGDescription(spName, 2.5, 1024);
    auto error = RegisterSharedServicePackage(lrm, impossibleSPId, impossibleMemoryRGSettings);
    VERIFY_IS_FALSE(error.IsSuccess());

    VerifyResourceUsage(lrm, 0, 0);

    // Register a SP that will use 1/2 of a core
    ServicePackageIdentifier halfCoreSP1ID(ApplicationIdentifier(L"AppTypeName", 1), spName);
    auto halfCoreRGSettings = GetSPRGDescription(spName, 0.5, 1);
    error = RegisterSharedServicePackage(lrm, halfCoreSP1ID, halfCoreRGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    VerifyResourceUsage(lrm, 0.5, 1);

    // Register one more SP that will use 1/2 core
    ServicePackageIdentifier halfCoreSP2ID(ApplicationIdentifier(L"AppTypeName", 2), spName);
    error = RegisterSharedServicePackage(lrm, halfCoreSP2ID, halfCoreRGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    VerifyResourceUsage(lrm, 1, 2);

    // Try to register one SP that will use 1.5 cores and fail
    ServicePackageIdentifier largeSPID(ApplicationIdentifier(L"AppTypeName", 3), spName);
    auto largeRGSettings = GetSPRGDescription(spName, 1.5, 1);
    error = RegisterSharedServicePackage(lrm, largeSPID, largeRGSettings);
    VERIFY_IS_FALSE(error.IsSuccess());

    // Unregister one smaller package.
    UnregisterSharedServicePackage(lrm, halfCoreSP1ID, halfCoreRGSettings);
    VerifyResourceUsage(lrm, 0.5, 1);

    // Try to register large package one more time and fail
    error = RegisterSharedServicePackage(lrm, largeSPID, largeRGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());
    VerifyResourceUsage(lrm, 2, 2);

    CloseFabricNode();
}

BOOST_AUTO_TEST_CASE(SmallSubcoreAllocationsTest)
{
    // Test case: 10e-5 and 1.0 - 10e-5 allocations (edge cases).
    HostingConfigScopeChange(LocalResourceManagerTestMode, bool, true);
    VERIFY_IS_TRUE(OpenFabricNode(GetCapacityConfig(1 /* cores */, 2048 /* MB */)));
    LocalResourceManager& lrm = GetLocalResourceManager();

    VerifyResourceUsage(lrm, 0, 0);

    // Register a SP that will use 0.0001 of a core
    wstring spName = L"ServicePackage1";
    ServicePackageIdentifier smallSP1ID(ApplicationIdentifier(L"AppTypeName", 1), spName);
    auto smallRGSettings = GetSPRGDescription(spName, 0.0001, 1);
    auto error = RegisterSharedServicePackage(lrm, smallSP1ID, smallRGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    VerifyResourceUsage(lrm, 0.0001, 1);

    // Register a SP that will use 0.9999 cores
    ServicePackageIdentifier largeSPID(ApplicationIdentifier(L"AppTypeName", 314), spName);
    auto largeRGSettings = GetSPRGDescription(spName, 0.9999, 1);
    error = RegisterSharedServicePackage(lrm, largeSPID, largeRGSettings);
    VERIFY_IS_TRUE(error.IsSuccess());

    VerifyResourceUsage(lrm, 1.0, 2);

    // Try registering one more SP with 0.0001 cores and check that it fails
    ServicePackageIdentifier smallSP2ID(ApplicationIdentifier(L"AppTypeName", 3), spName);
    error = RegisterSharedServicePackage(lrm, smallSP2ID, smallRGSettings);
    VERIFY_IS_FALSE(error.IsSuccess());

    VerifyResourceUsage(lrm, 1.0, 2);

    // Unregister smaller package.
    UnregisterSharedServicePackage(lrm, smallSP1ID, smallRGSettings);
    VerifyResourceUsage(lrm, 0.9999, 1);

    // Unregister larger package.
    UnregisterSharedServicePackage(lrm, largeSPID, largeRGSettings);
    VerifyResourceUsage(lrm, 0.0, 0);

    CloseFabricNode();
}

BOOST_AUTO_TEST_SUITE_END()

bool LocalResourceManagerTest::TestSetup()
{
    uint64 helpAvailableMemoryInMB = 0;
    Common::ErrorCode errorMemory = Environment::GetAvailableMemoryInBytes(helpAvailableMemoryInMB);
    if (!errorMemory.IsSuccess())
    {
        return false;
    }

    double cpuPercent = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().CpuPercentageNodeCapacity;
    double memoryPercent = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MemoryPercentageNodeCapacity;
    availableMemoryInMB_ = static_cast<uint>(helpAvailableMemoryInMB * memoryPercent / (1024 * 1024));

    uint64 systemCpuCores = Environment::GetNumberOfProcessors();
    numberOfAvailableCores_ = (systemCpuCores == 1) ? 1: static_cast<uint>(systemCpuCores * cpuPercent);

    Trace.WriteInfo(TraceType,
        "LocalResourceManagerTest starting with {0} cores and {1} availableMemory",
        numberOfAvailableCores_,
        availableMemoryInMB_);

    return true;
}
