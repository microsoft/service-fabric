// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Hosting2/FabricNodeHost.Test.h"


using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;

const StringLiteral TraceType("FabricUpgradeTest");

class DummyFabricUpgradeImpl : public IFabricUpgradeImpl
{
    Common::AsyncOperationSPtr BeginDownload(
        Common::FabricVersion const & fabricVersion,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(fabricVersion);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);    
    }

    Common::ErrorCode EndDownload(
        Common::AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr BeginValidateAndAnalyze(
        Common::FabricVersionInstance const & currentFabricVersionInstance,
        ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(currentFabricVersionInstance);
        UNREFERENCED_PARAMETER(fabricUpgradeSpec);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);    
    }

    Common::ErrorCode EndValidateAndAnalyze(
        __out bool & shouldCloseReplica,
        Common::AsyncOperationSPtr const & operation)
    {
        shouldCloseReplica = false;
        return CompletedAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr BeginUpgrade(
        Common::FabricVersionInstance const & currentFabricVersionInstance,
        ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(currentFabricVersionInstance);
        UNREFERENCED_PARAMETER(fabricUpgradeSpec);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);    
    }

    Common::ErrorCode EndUpgrade(
        Common::AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }
};

class FabricUpgradeTest
{
protected:
	FabricUpgradeTest() : fabricNodeHost_(make_shared<TestFabricNodeHost>()) { BOOST_REQUIRE(Setup()); }
    TEST_METHOD_SETUP(Setup);
    ~FabricUpgradeTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_METHOD_CLEANUP(Cleanup);

    ConfigSettings GetDefaultConfigSettings();

	shared_ptr<TestFabricNodeHost> fabricNodeHost_;
    shared_ptr<ConfigSettingsConfigStore> configStore_;
};


BOOST_FIXTURE_TEST_SUITE(FabricUpgradeTestSuite,FabricUpgradeTest)

BOOST_AUTO_TEST_CASE(TestValidate)
{
    FabricConfigVersion configVersion;
    auto error = configVersion.FromString(L"V2");
    VERIFY_IS_TRUE(error.IsSuccess());

    ManualResetEvent validateDone;    
    FabricVersion targetFabricVersion(
        FabricCodeVersion::Default,
        configVersion);

    FabricUpgradeSpecification fabricUpgradeSpec(targetFabricVersion, 5, UpgradeType::Rolling_NotificationOnly, true, false);

    fabricNodeHost_->GetHosting().BeginValidateFabricUpgrade(
        fabricUpgradeSpec,
        [this, &validateDone](AsyncOperationSPtr const & operation) 
    {        
        bool shouldRestartReplica;
        auto error = fabricNodeHost_->GetHosting().EndValidateFabricUpgrade(shouldRestartReplica, operation);
        VERIFY_IS_FALSE(shouldRestartReplica);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "ValidateFabricUpgrade returned %d", error.ReadValue());
        validateDone.Set();
    },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(validateDone.WaitOne(10000), L"TestValidate completed before timeout.");
}

BOOST_AUTO_TEST_CASE(TestUpgrade)
{
    FabricConfigVersion configVersion;
    auto error = configVersion.FromString(L"V2");
    VERIFY_IS_TRUE(error.IsSuccess());

    ManualResetEvent upgradeDone;    
    FabricVersion targetFabricVersion(
        FabricCodeVersion::Default,
        configVersion);

    FabricUpgradeSpecification fabricUpgradeSpec(targetFabricVersion, 5, UpgradeType::Rolling_NotificationOnly, true, false);

    fabricNodeHost_->GetHosting().BeginFabricUpgrade(
        fabricUpgradeSpec,
        false, /* TODO: Update with fix for #1405455 */
        [this, &upgradeDone](AsyncOperationSPtr const & operation) 
    {        
        auto error = fabricNodeHost_->GetHosting().EndFabricUpgrade(operation);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "FabricUpgrade returned %d", error.ReadValue());
        upgradeDone.Set();
    },
        AsyncOperationSPtr());

    ConfigSettings updatedConfigSettings = GetDefaultConfigSettings();
    updatedConfigSettings.Sections[L"FabricNode"].Parameters[L"NodeVersion"].Value = FabricVersionInstance(fabricUpgradeSpec.Version, fabricUpgradeSpec.InstanceId).ToString();

    configStore_->Update(updatedConfigSettings);

    VERIFY_IS_TRUE(upgradeDone.WaitOne(10000), L"TestUpgrade completed before timeout.");
}

BOOST_AUTO_TEST_CASE(TestParallelOperations)
{
    ManualResetEvent upgradeDone;    
    ManualResetEvent parallelUpgradeDone;    
    ManualResetEvent parallelValidateDone;   

    FabricConfigVersion configVersion;
    auto error = configVersion.FromString(L"V2");
    VERIFY_IS_TRUE(error.IsSuccess());

    FabricVersion targetFabricVersion(
        FabricCodeVersion::Default,
        configVersion);

    FabricUpgradeSpecification fabricUpgradeSpec(targetFabricVersion, 5, UpgradeType::Rolling_NotificationOnly, true, false);

    fabricNodeHost_->GetHosting().BeginFabricUpgrade(
        fabricUpgradeSpec,
        false, /* TODO: Update with fix for #1405455 */
        [this, &upgradeDone](AsyncOperationSPtr const & operation) 
    {        
        auto error = fabricNodeHost_->GetHosting().EndFabricUpgrade(operation);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "FabricUpgrade returned %d", error.ReadValue());
        upgradeDone.Set();
    },
        AsyncOperationSPtr());

    fabricNodeHost_->GetHosting().BeginFabricUpgrade(
        fabricUpgradeSpec,
        false, /* TODO: Update with fix for #1405455 */
        [this, &parallelUpgradeDone](AsyncOperationSPtr const & operation) 
    {        
        auto error = fabricNodeHost_->GetHosting().EndFabricUpgrade(operation);
        VERIFY_IS_TRUE_FMT(!error.IsSuccess(), "Parallel FabricUpgrade returned %d", error.ReadValue());        
        parallelUpgradeDone.Set();
    },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(parallelUpgradeDone.WaitOne(10000), L"ParallelUpgrade completed before timeout.");

    fabricNodeHost_->GetHosting().BeginValidateFabricUpgrade(
        fabricUpgradeSpec,
        [this, &parallelValidateDone](AsyncOperationSPtr const & operation) 
    {        
        bool shouldRestartReplica;;
        auto error = fabricNodeHost_->GetHosting().EndValidateFabricUpgrade(shouldRestartReplica, operation);
        VERIFY_IS_TRUE_FMT(!error.IsSuccess(), "Parallel ValidateFabricUpgrade returned %d", error.ReadValue());        
        parallelValidateDone.Set();
    },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(parallelValidateDone.WaitOne(10000), L"ParallelValidate completed before timeout.");

    ConfigSettings updatedConfigSettings = GetDefaultConfigSettings();
    updatedConfigSettings.Sections[L"FabricNode"].Parameters[L"NodeVersion"].Value = FabricVersionInstance(fabricUpgradeSpec.Version, fabricUpgradeSpec.InstanceId).ToString();

    configStore_->Update(updatedConfigSettings);

    VERIFY_IS_TRUE(upgradeDone.WaitOne(10000), L"TestParallelOperations completed before timeout.");
}

BOOST_AUTO_TEST_CASE(TestCloseDuringUpgrade)
{ 
    FabricConfigVersion configVersion;
    auto error = configVersion.FromString(L"V2");
    VERIFY_IS_TRUE(error.IsSuccess());

    ManualResetEvent upgradeDone;    
    FabricVersion targetFabricVersion(
        FabricCodeVersion::Default,
        configVersion);

    FabricUpgradeSpecification fabricUpgradeSpec(targetFabricVersion, 5, UpgradeType::Rolling_NotificationOnly, true, false);

    fabricNodeHost_->GetHosting().BeginFabricUpgrade(
        fabricUpgradeSpec,
        false, /* TODO: Update with fix for #1405455 */
        [this, &upgradeDone](AsyncOperationSPtr const & operation) 
    {        
        auto error = fabricNodeHost_->GetHosting().EndFabricUpgrade(operation);
        VERIFY_IS_TRUE_FMT(!error.IsSuccess(), "FabricUpgrade returned %d", error.ReadValue());
        upgradeDone.Set();
    },
        AsyncOperationSPtr());

    this->fabricNodeHost_->Close();

    VERIFY_IS_TRUE(upgradeDone.WaitOne(10000), L"TestParallelOperations completed before timeout.");
}

BOOST_AUTO_TEST_SUITE_END()

ConfigSettings FabricUpgradeTest::GetDefaultConfigSettings()
{
    FabricNodeConfigSPtr defaultConfig = std::make_shared<FabricNodeConfig>();

    ConfigParameter nodeId;
    nodeId.Name = defaultConfig->NodeIdEntry.Key;
    nodeId.Value = defaultConfig->NodeId;

    ConfigParameter nodeAddress;
    nodeAddress.Name = defaultConfig->NodeAddressEntry.Key;
    nodeAddress.Value = defaultConfig->NodeAddress;

    ConfigParameter leaseAgentAddress;
    leaseAgentAddress.Name = defaultConfig->LeaseAgentAddressEntry.Key;
    leaseAgentAddress.Value = defaultConfig->LeaseAgentAddress;

    ConfigParameter runtimeServiceAddress;
    runtimeServiceAddress.Name = defaultConfig->RuntimeServiceAddressEntry.Key;
    runtimeServiceAddress.Value = defaultConfig->RuntimeServiceAddress;

    ConfigParameter clientConnectionAddress;
    clientConnectionAddress.Name = defaultConfig->ClientConnectionAddressEntry.Key;
    clientConnectionAddress.Value = defaultConfig->ClientConnectionAddress;

    ConfigParameter workingDir;
    workingDir.Name = defaultConfig->WorkingDirEntry.Key;
    workingDir.Value = defaultConfig->WorkingDir;

    ConfigParameter nodeVersion;
    nodeVersion.Name = defaultConfig->NodeVersionEntry.Key;
    nodeVersion.Value = defaultConfig->NodeVersion.ToString();

    ConfigSection fabricNodeSection;
    fabricNodeSection.Name = L"FabricNode";
    fabricNodeSection.Parameters.insert(make_pair(nodeId.Name, nodeId));
    fabricNodeSection.Parameters.insert(make_pair(nodeAddress.Name, nodeAddress));
    fabricNodeSection.Parameters.insert(make_pair(leaseAgentAddress.Name, leaseAgentAddress));
    fabricNodeSection.Parameters.insert(make_pair(runtimeServiceAddress.Name, runtimeServiceAddress));
    fabricNodeSection.Parameters.insert(make_pair(clientConnectionAddress.Name, clientConnectionAddress));
    fabricNodeSection.Parameters.insert(make_pair(workingDir.Name, workingDir));
    fabricNodeSection.Parameters.insert(make_pair(nodeVersion.Name, nodeVersion));

    ConfigSettings configSettings;
    configSettings.Sections.insert(make_pair(L"FabricNode", fabricNodeSection));

    return configSettings;
}

bool FabricUpgradeTest::Setup()
{
    ConfigSettings configSettings = GetDefaultConfigSettings();
    configStore_ = make_shared<ConfigSettingsConfigStore>(configSettings);  

	return fabricNodeHost_->Open(make_shared<FabricNodeConfig>(configStore_), make_shared<DummyFabricUpgradeImpl>());
}

bool FabricUpgradeTest::Cleanup()
{
    auto retval = fabricNodeHost_->Close();
    if (!retval)
    {
        // wait for the update operations to finish, this is a bug in the upgrade path 
        // that needs to be fixed
        Sleep(5000);
    }

    return retval;
}
