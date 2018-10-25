// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace Federation;
using namespace std;
using namespace Transport;
using namespace Common;
using namespace ServiceModel;
using namespace Fabric;
using namespace TestCommon;
using namespace FederationTestCommon;
using namespace Reliability;
using namespace ReliabilityTestApi;
using namespace Naming;
using namespace Hosting2;
using namespace Management::HealthManager;

const StringLiteral TraceSource("FabricTest.FabricNodeWrapper");

void FabricNodeWrapper::GetReliabilitySubsystemUnderLock(std::function<void(ReliabilityTestHelper &)> processor) const
{
    Common::AcquireReadLock lock(fabricNodeLock_);

    if (fabricNode_)
    {
        ReliabilityTestHelper testHelper(fabricNode_->Test_Reliability);
        processor(testHelper);
    }
}

FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr FabricNodeWrapper::GetFailoverManager() const
{
    FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr rv;

    GetReliabilitySubsystemUnderLock([&] (ReliabilityTestHelper & reliabilityTestHelper)
    {
        if (fabricNode_ == nullptr)
        {
            return;
        }

        rv = move(reliabilityTestHelper.GetFailoverManager());
    });    

    return rv;
}

FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr FabricNodeWrapper::GetFailoverManagerMaster() const
{
    FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr rv;
    
    GetReliabilitySubsystemUnderLock([&] (ReliabilityTestHelper & reliabilityTestHelper)
    {
        if (fabricNode_ == nullptr)
        {
            return;
        }

        rv = move(reliabilityTestHelper.GetFailoverManagerMaster());
    });    

    return rv;
}

FabricNodeConfigOverride::FabricNodeConfigOverride(
    std::wstring && nodeUpgradeDomainId,
    Common::Uri && nodeFaultDomainId,
    std::wstring && systemServiceInitializationTimeout,
    std::wstring && systemServiceInitializationRetryInterval)
    : NodeUpgradeDomainId(move(nodeUpgradeDomainId))
    , NodeFaultDomainId(move(nodeFaultDomainId))
    , SystemServiceInitializationTimeout(move(systemServiceInitializationTimeout))
    , SystemServiceInitializationRetryInterval(move(systemServiceInitializationRetryInterval))
{
}

//SetFabricNodeConfigEntry and GetFabricNodeConfigEntryValue is for FabricNode section
void FabricNodeWrapper::SetFabricNodeConfigEntry(wstring configEntryName, wstring configEntryValue, bool updateStore)
{
    ConfigParameter tempParam;
    tempParam.Name = configEntryName;
    tempParam.Value = configEntryValue;                                                   
    fabricNodeConfigSection_.Parameters[tempParam.Name] = tempParam;        

    if (updateStore)
    {
        configSettings_.Sections[fabricNodeConfigSection_.Name] = fabricNodeConfigSection_;      
        configStoreSPtr_->Update(configSettings_);               
    }   
}                                                                

wstring FabricNodeWrapper::GetFabricNodeConfigEntryValue(wstring configEntryName)
{
    auto itor = fabricNodeConfigSection_.Parameters.find(configEntryName);
    if ( itor == fabricNodeConfigSection_.Parameters.end())
    {
        return L"";
    }
    else 
    {   
        ConfigParameter tempParam = itor->second;
        return tempParam.Value;
    }    
}

template <typename TKey, typename TValue>
void FabricNodeWrapper::SetConfigSection(ConfigSection & configSection, Common::ConfigCollection<TKey, TValue> sectionContents)
{  
    for (auto itor = sectionContents.begin(); itor!=sectionContents.end(); itor++)    
    {        
        ConfigParameter tempParam;   
        tempParam.Name = itor->first;
        tempParam.Value = wformatString(itor->second);
        configSection.Parameters[tempParam.Name] = tempParam;        
    }    
    configSettings_.Sections[configSection.Name] = configSection;      
}

void FabricNodeWrapper::SetNodeDomainIdsConfigSection(wstring nodeUpgradeDomainIdKey, wstring nodeUpgradeDomainId, Common::FabricNodeConfig::NodeFaultDomainIdCollection faultDomainIds)
{
    ConfigParameter tempParam;
    tempParam.Name = nodeUpgradeDomainIdKey;
    tempParam.Value = nodeUpgradeDomainId;                                                   
    nodeFaultDomainIdsSection_.Parameters[tempParam.Name] = tempParam;     

    for (auto itor = faultDomainIds.begin(); itor!=faultDomainIds.end(); itor++)    
    {        
        ConfigParameter configParam;   
		configParam.Name = L"NodeFaultDomainId";   //refer to FabricNodeConfig::NodeFaultDomainIdCollection::Parse in fabricNodeConfig.cpp
		configParam.Value = wformatString(*itor);
        nodeFaultDomainIdsSection_.Parameters[configParam.Name] = configParam;
    }    
    configSettings_.Sections[nodeFaultDomainIdsSection_.Name] = nodeFaultDomainIdsSection_;      
}



FabricNodeWrapper::FabricNodeWrapper(
    FabricVersionInstance const& fabVerInst,
    NodeId nodeId, 
    FabricNodeConfigOverride const& configOverride,
    FabricNodeConfig::NodePropertyCollectionMap const & nodeProperties,
    FabricNodeConfig::NodeCapacityCollectionMap const & nodeCapacities,
    std::vector<std::wstring> const & nodeCredentials,
    std::vector<std::wstring> const & clusterCredentials,
    bool checkCert)
    : fabricNodeLock_(),
    fabricNode_(),
    nodeProperties_(nodeProperties),
    isFirstCreate_(true),
    fabricNodeConfigSection_(),
    nodePropertiesSection_(),
    nodeCapacitySection_(),
    nodeFaultDomainIdsSection_(),
    configSettings_(),
    configStoreSPtr_(),
    fabricNodeConfigSPtr_(),
    fabricNodeCreateError_(),
    nodeId_(),
    retryCount_(0),
    nodeRebootLock_(),
    nodeOpenCompletedEvent_(),
    nodeOpenTimestamp_(StopwatchTime::Zero)
{
    const int numberOfPorts = 4;
    TestSession::WriteInfo(
        TraceSource,
        "Requesting {0} ports", numberOfPorts);
    vector<USHORT> ports;
    FABRICSESSION.AddressHelperObj.GetAvailablePorts(nodeId.ToString(), ports, numberOfPorts);
    // Always access seed nodes through config since seed nodes can
    // be set from config file or through command line override
    auto votes = FederationConfig::GetConfig().Votes;
    wstring addr;
    auto vote = votes.find(nodeId);
    if (vote != votes.end() && vote->Type == Federation::Constants::SeedNodeVoteType)
    {
        addr = vote->ConnectionString;
    }
    else
    {
        //Let transport chose the port
        addr = AddressHelper::BuildAddress(AddressHelper::GetLoopbackAddress(), wformatString(ports[0]));
    }

    wstring runtimeListenAddress = AddressHelper::BuildAddress(AddressHelper::GetLoopbackAddress(), wformatString(ports[1]));
    wstring namingServiceListenAddress = AddressHelper::BuildAddress(AddressHelper::GetLoopbackAddress(), wformatString(ports[2]));
    wstring httpGatewayAddress = AddressHelper::BuildAddress(AddressHelper::GetLoopbackAddress(), wformatString(ports[3]));
    wstring leaseAddress = AddressHelper::BuildAddress(AddressHelper::GetLoopbackAddress(), wformatString(ports[4]));

    TestSession::WriteInfo(
        TraceSource,
        "Creating FabricTestNode: SiteNodeAddr {0}, RuntimeAddr {1}, NamingAddr {2}, HttpGateway {3}, LeaseAddr {4}",
        addr,
        runtimeListenAddress,
        namingServiceListenAddress,
        httpGatewayAddress,
        leaseAddress);

    // RDBug#1481055: Naming service endpoint is not available to external service hosts,
    // so e.g. FabricRM.exe cannot use fabric client.  This is a workaround, while a better
    // fix might be to have fabrictest push all config changes to per-node config files,
    // similar to a real scale-min deployment.
    //
    wstring envVar(L"_FabricTest_FabricClientEndpoint");
    wstring dummy;
    if (!Environment::GetEnvironmentVariableW(envVar, dummy, Common::NOTHROW()))
    {
        TestSession::WriteInfo(TraceSource, "Setting {0} = {1}", envVar, namingServiceListenAddress);
        Environment::SetEnvironmentVariableW(envVar, namingServiceListenAddress);
    }

    SetSharedLogEnvironmentVariables();

#if defined(PLATFORM_UNIX)
    wstring envVarLibraryPath(L"LD_LIBRARY_PATH");
    wstring dummy2;
    if (!Environment::GetEnvironmentVariableW(envVarLibraryPath, dummy2, Common::NOTHROW()))
    {
        TestSession::WriteInfo(TraceSource, "Setting {0} = {1}", envVarLibraryPath, Environment::GetExecutablePath());
        Environment::SetEnvironmentVariableW(envVarLibraryPath, Environment::GetExecutablePath());
    }
#endif

    fabricNodeConfigSection_.Name = L"FabricNode";
    nodePropertiesSection_.Name = L"NodeProperties";
    nodeCapacitySection_.Name = L"NodeCapacities";
    nodeFaultDomainIdsSection_.Name = L"NodeDomainIds";

    FabricNodeConfig tempconfig;            
    SetFabricNodeConfigEntry(tempconfig.NodeVersionEntry.Key, fabVerInst.ToString());     
    SetFabricNodeConfigEntry(tempconfig.NodeIdEntry.Key, nodeId.ToString());
    SetFabricNodeConfigEntry(tempconfig.NodeAddressEntry.Key, addr);    
    SetFabricNodeConfigEntry(tempconfig.InstanceNameEntry.Key, Federation::NodeIdGenerator::GenerateNodeName(nodeId));
    SetFabricNodeConfigEntry(tempconfig.NodeTypeEntry.Key, L"NodeType");

    int ktlSharedLogSizeInMB = FabricTestSessionConfig::GetConfig().KtlSharedLogSizeInMB;

    TestSession::FailTestIf(
        ktlSharedLogSizeInMB < 256, 
        "Ktl Shared Log Size In MB must be greater than 256. It is {0}", 
        ktlSharedLogSizeInMB);

    SetFabricNodeConfigEntry(tempconfig.SharedLogFileSizeInMBEntry.Key, wformatString("{0}", ktlSharedLogSizeInMB));

    FabricNodeConfig::NodeFaultDomainIdCollection faultDomainIds;
    if (!configOverride.NodeFaultDomainId.Scheme.empty() && !configOverride.NodeFaultDomainId.Path.empty())
    {
        faultDomainIds.push_back(configOverride.NodeFaultDomainId);
    }
    SetNodeDomainIdsConfigSection(tempconfig.UpgradeDomainIdEntry.Key, configOverride.NodeUpgradeDomainId, faultDomainIds);        
    
    SetConfigSection(nodePropertiesSection_, nodeProperties);         
    SetConfigSection(nodeCapacitySection_, nodeCapacities);    

    SetFabricNodeConfigEntry(tempconfig.RuntimeServiceAddressEntry.Key, runtimeListenAddress);    
    SetFabricNodeConfigEntry(tempconfig.ClientConnectionAddressEntry.Key, namingServiceListenAddress);    
    SetFabricNodeConfigEntry(tempconfig.HttpGatewayListenAddressEntry.Key, httpGatewayAddress);
    SetFabricNodeConfigEntry(tempconfig.HttpGatewayProtocolEntry.Key, L"http");    
    SetFabricNodeConfigEntry(tempconfig.WorkingDirEntry.Key, FABRICSESSION.FabricDispatcher.GetWorkingDirectory(nodeId.ToString()));

    SecurityConfig & securityConfig = SecurityConfig::GetConfig();
    SecuritySettings clientServerSecuritySettings;

    if (!clusterCredentials.empty() &&
       (StringUtility::CompareCaseInsensitive(clusterCredentials.front(), L"Windows") == 0))
    {
        securityConfig.ClusterCredentialType = clusterCredentials[0];
        securityConfig.ClusterProtectionLevel = clusterCredentials[1];
        securityConfig.ServerAuthCredentialType = clusterCredentials[0];
        securityConfig.ClientServerProtectionLevel = clusterCredentials[1];
    }
    else if(nodeCredentials.size() > 0)
    {
        TestSession::FailTestIfNot(
            nodeCredentials.size() == 4 && (clusterCredentials.size() == 5 || clusterCredentials.size() >= 8), 
            "Invalid security settings: node={0} cluster={1}",
            nodeCredentials,
            clusterCredentials);

        securityConfig.ClusterCredentialType = clusterCredentials[0];
        securityConfig.ServerAuthCredentialType = clusterCredentials[0];
        securityConfig.ClusterProtectionLevel = clusterCredentials[1];
        securityConfig.ClientServerProtectionLevel = clusterCredentials[1];
        securityConfig.ClusterAllowedCommonNames = clusterCredentials[2];
        securityConfig.ClientAuthAllowedCommonNames = clusterCredentials[3];
        securityConfig.ServerAuthAllowedCommonNames = clusterCredentials[4];

        wstring findValue = nodeCredentials[1];
        if(nodeCredentials[0] == wformatString(X509FindType::FindByThumbprint))
        {
            StringUtility::Replace<wstring>(findValue, L":", L" ");
        }

        SetFabricNodeConfigEntry(tempconfig.ClusterX509FindTypeEntry.Key, nodeCredentials[0]);
        SetFabricNodeConfigEntry(tempconfig.ClusterX509FindValueEntry.Key, findValue);
        SetFabricNodeConfigEntry(tempconfig.ClusterX509StoreNameEntry.Key, nodeCredentials[2]);

        SetFabricNodeConfigEntry(tempconfig.ServerAuthX509FindTypeEntry.Key, nodeCredentials[0]);
        SetFabricNodeConfigEntry(tempconfig.ServerAuthX509FindValueEntry.Key, findValue);
        SetFabricNodeConfigEntry(tempconfig.ServerAuthX509StoreNameEntry.Key, nodeCredentials[2]);

        if (clusterCredentials.size() >= 8)
        {
            securityConfig.ClusterCertThumbprints = clusterCredentials[5];
            securityConfig.ServerCertThumbprints = clusterCredentials[7];
            if (securityConfig.IsClientRoleInEffect())
            {
                TestSession::FailTestIfNot(
                    clusterCredentials.size() > 8, 
                    "Invalid security settings: cluster={0}",
                    clusterCredentials);
              
                securityConfig.ClientCertThumbprints = clusterCredentials[8];
                securityConfig.AdminClientCertThumbprints = wformatString("{0},{1}", findValue, clusterCredentials[6]);
            }
            else
            {
                securityConfig.ClientCertThumbprints = wformatString("{0},{1}", findValue, clusterCredentials[6]);
            }
        }

        SetFabricNodeConfigEntry(tempconfig.ClientAuthX509FindTypeEntry.Key, nodeCredentials[0]);
        SetFabricNodeConfigEntry(tempconfig.ClientAuthX509FindValueEntry.Key, findValue);
        SetFabricNodeConfigEntry(tempconfig.ClientAuthX509StoreNameEntry.Key, nodeCredentials[2]);

        SecurityConfig::X509NameMap clientX509Names;
        auto error = clientX509Names.AddNames(
            securityConfig.ClientAuthAllowedCommonNames,
            securityConfig.ClientCertIssuers);
        
        KInvariant(error.IsSuccess());

        error = SecuritySettings::FromConfiguration(
            securityConfig.ServerAuthCredentialType,
            GetFabricNodeConfigEntryValue(tempconfig.ServerAuthX509StoreNameEntry.Key), //config->ServerAuthX509StoreName,
            wformatString(X509Default::StoreLocation()),
            GetFabricNodeConfigEntryValue(tempconfig.ServerAuthX509FindTypeEntry.Key), //config->ServerAuthX509FindType,
            GetFabricNodeConfigEntryValue(tempconfig.ServerAuthX509FindValueEntry.Key), //config->ServerAuthX509FindValue,
            GetFabricNodeConfigEntryValue(tempconfig.ServerAuthX509FindValueSecondaryEntry.Key), //config->ServerAuthX509FindValueSecondary,
            securityConfig.ClientServerProtectionLevel,
            securityConfig.ClientCertThumbprints,
            clientX509Names,
            securityConfig.ClientCertificateIssuerStores,
            L"",
            L"",
            L"",
            securityConfig.ClientIdentities,
            clientServerSecuritySettings);
    }
    else
    {
        securityConfig.ClusterCredentialType = L"None";
        securityConfig.ServerAuthCredentialType = L"None";
    }

    if (!Directory::Exists(GetFabricNodeConfigEntryValue(tempconfig.WorkingDirEntry.Key)) /*config->WorkingDir)*/)
    {
        Directory::Create(GetFabricNodeConfigEntryValue(tempconfig.WorkingDirEntry.Key) /*config->WorkingDir)*/);
    }

    SetFabricNodeConfigEntry(tempconfig.LeaseAgentAddressEntry.Key, leaseAddress);    

    // Set additional configurations for optimizing runtime
    SetFabricNodeConfigEntry(tempconfig.SystemServiceInitializationRetryIntervalEntry.Key, configOverride.SystemServiceInitializationRetryInterval);
    SetFabricNodeConfigEntry(tempconfig.SystemServiceInitializationTimeoutEntry.Key, configOverride.SystemServiceInitializationTimeout);

    configSettings_.Sections[fabricNodeConfigSection_.Name] = fabricNodeConfigSection_;      
    configStoreSPtr_ = make_shared<ConfigSettingsConfigStore>(configSettings_);       
    fabricNodeConfigSPtr_ = make_shared<FabricNodeConfig>(configStoreSPtr_);

    nodeId_ = nodeId;

    // checkCert is false when the test intentionally supplies invalid credentials (SslBasic.test)
    //
    runtimeManager_ = make_unique<FabricTestRuntimeManager>(
        nodeId_, 
        runtimeListenAddress, 
        namingServiceListenAddress, 
        false, 
       checkCert ? clientServerSecuritySettings : SecuritySettings());
}

FabricNodeWrapper::~FabricNodeWrapper()
{
    if(fabricNodeCreateError_.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "Destructing fabric node wrapper {0}", Instance);
        FABRICSESSION.RemovePendingItem(wformatString("TestNode.{0}", Id));
    }
}

#define SET_FABRIC_TEST_ENVIRONMENT_VARIABLE( EV_Name ) \
{ \
    wstring envName = TestHooks::EnvironmentVariables::FabricTest_##EV_Name; \
    wstring unused; \
    if (!Environment::GetEnvironmentVariableW(envName, unused, Common::NOTHROW())) \
    { \
        wstring envValue = wformatString(FabricTestCommonConfig::GetConfig().EV_Name); \
        TestSession::WriteInfo(TraceSource, "Setting {0} = {1}", envName, envValue); \
        auto success = Environment::SetEnvironmentVariableW(envName, envValue); \
        TestSession::FailTestIfNot(success, "Failed to set environment variable '{0}'='{1}'", envName, envValue); \
    } \
} \

void FabricNodeWrapper::SetSharedLogEnvironmentVariables()
{
    SET_FABRIC_TEST_ENVIRONMENT_VARIABLE( EnableTStore )
    SET_FABRIC_TEST_ENVIRONMENT_VARIABLE( EnableHashSharedLogId )
    SET_FABRIC_TEST_ENVIRONMENT_VARIABLE( EnableSparseSharedLogs )
    SET_FABRIC_TEST_ENVIRONMENT_VARIABLE( SharedLogSize )
    SET_FABRIC_TEST_ENVIRONMENT_VARIABLE( SharedLogMaximumRecordSize )
}

void FabricNodeWrapper::Open(TimeSpan openTimeout, bool expectNodeFailure, bool addRuntime, ErrorCodeValue::Enum openErrorCode, vector<ErrorCodeValue::Enum> const& retryErrors)
{
    Common::AcquireWriteLock lock(nodeRebootLock_);
    OpenInternal( openTimeout,  expectNodeFailure,  addRuntime,  openErrorCode,  retryErrors);      
}

void FabricNodeWrapper::OpenInternal(TimeSpan openTimeout, bool expectNodeFailure, bool addRuntime, ErrorCodeValue::Enum openErrorCode, vector<ErrorCodeValue::Enum> const& retryErrors)
{
    {
        Common::AcquireWriteLock lock(fabricNodeLock_);

        if (SecurityConfig::GetConfig().ClientClaimAuthEnabled)
        {
            SecurityConfig::GetConfig().UseTestClaimsAuthenticator = true;
        }

        fabricNodeCreateError_ = FabricNode::Test_Create(
            fabricNodeConfigSPtr_,
            TestHostingSubsystem::Create,
            ProcessTerminationService::Create,
            fabricNode_);

        if(fabricNodeCreateError_.IsSuccess() && isFirstCreate_)
        {
            TestSession::WriteInfo(TraceSource, "creating fabric test node {0}", fabricNode_->Instance);
            FABRICSESSION.AddPendingItem(wformatString("TestNode.{0}", Id));
            isFirstCreate_ = false;
            FABRICSESSION.Expect("Node {0} Opened with ErrorCode {1}", Id, openErrorCode);            
        }
        else if (!fabricNodeCreateError_.IsSuccess())
        {
            TestSession::WriteInfo(TraceSource, "creating fabric test node {0} failed with {1}", Id, fabricNodeCreateError_);
            TestSession::FailTestIfNot(isFirstCreate_, "FabricNode::Create should not fail on retry");
            FABRICSESSION.Expect("Node Created with ErrorCode {0}", openErrorCode);
            FABRICSESSION.Validate("Node Created with ErrorCode {0}", fabricNodeCreateError_);
            return;
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "{0}:Created fabric node on retry with success", fabricNode_->Instance);
        }
    }

    auto fabricNodeWrapperSPtr = shared_from_this();
    fabricNode_->RegisterNodeFailedEvent( [this, expectNodeFailure, openErrorCode, fabricNodeWrapperSPtr](EventArgs const &)
    {
        if (openErrorCode == ErrorCodeValue::Success)
        {
            if(expectNodeFailure)
            {
                FABRICSESSION.Validate("Node {0} Failed", Id);
            }
            else
            {
                TestSession::FailTest("Node {0} Failed", Instance);
            }
        }
        else
        {
            // Failure might occur
        }
    });

    GetHostingSubsystem().FabricUpgradeManagerObj->Test_SetFabricUpgradeImpl(
        make_shared<TestFabricUpgradeHostingImpl>(fabricNodeWrapperSPtr)
        );

    fabricNode_->BeginOpen(
        openTimeout,
        [this, openTimeout, expectNodeFailure, addRuntime, openErrorCode, retryErrors, fabricNodeWrapperSPtr](AsyncOperationSPtr const& asyncOperation) -> void
    {
        ErrorCode error = fabricNode_->EndOpen(asyncOperation);
        if (error.IsSuccess())
        {
            if (addRuntime)
            {
                RuntimeManager->CreateAllRuntimes();
            }            
            TestSession::WriteInfo(TraceSource, "{0}:Open completed successfully", Instance);
            nodeOpenCompletedEvent_.Set();        
            nodeOpenTimestamp_ = Stopwatch::Now();
        }
        else if (!error.IsSuccess())
        {
            TestSession::WriteInfo(TraceSource, "{0}:Open completed with {1}", Instance, error.ReadValue());

            // Check is retryable error 
            auto iter = find(retryErrors.begin(), retryErrors.end(), error.ReadValue());
            if(iter != retryErrors.end())
            {
                ++retryCount_;
                if(retryCount_ <= FabricTestSessionConfig::GetConfig().NodeOpenMaxRetryCount)
                {
                    TestSession::WriteInfo(TraceSource, "{0}: Retry Count {1}", Instance, retryCount_);
                    // Sleep for 5 seconds before retry
                    Sleep(5000);
                    OpenInternal(openTimeout, expectNodeFailure, addRuntime, openErrorCode, retryErrors);
                    return;
                }
            }
        }

        FABRICSESSION.Validate("Node {0} Opened with ErrorCode {1}", Id, error);
    },
        fabricNode_->CreateAsyncOperationRoot());
}

void FabricNodeWrapper::Close(bool removeData)
{
    Common::AcquireWriteLock lock(nodeRebootLock_);

    TestSession::FailTestIfNot(IsOpened, "Close called when FabricNode is not yet opened");
    RuntimeManager->RemoveStatelessFabricRuntime();
    RuntimeManager->RemoveStatefulFabricRuntime();

    shared_ptr<ManualResetEvent> closeCompleteEvent = make_shared<ManualResetEvent>();
    auto fabricNodeWrapperSPtr = shared_from_this();
    FABRICSESSION.Expect("Fabric node {0} successfully closed", Instance);

    TestSession::WriteNoise(TraceSource,
        "Close async op for fabric node {0} is starting...",
        Instance);
    
    // remove monitoring in unreliable transport
    UnreliableTransportConfig::GetConfig().Test_StopMonitoring(nodeId_.ToString());

    auto cb = [this, closeCompleteEvent, fabricNodeWrapperSPtr](AsyncOperationSPtr const& operation) -> void
    {
        TestSession::WriteNoise(TraceSource,
            "Close async op for fabric node {0} is ending...",
            Instance);
        auto error = fabricNode_->EndClose(operation);
        if (error.IsSuccess())
        {
            FABRICSESSION.Validate("Fabric node {0} successfully closed", Instance);
        }
        else
        {
            TestSession::FailTest("Close for fabric node {0} failed with  ErrorCodeValue = {1}", Instance, error.ReadValue());
        }

        TestSession::WriteNoise(
            TraceSource,
            "Close async op for fabric node {0} has ended.",
            Instance);

        closeCompleteEvent->Set();

        FABRICSESSION.ClosePendingItem(wformatString("TestNode.{0}", Id));
    };

    if (removeData)
    {
        // There are FabricTest scripts that intentionally remove node state
        // between restarting a node (to simulate node dataloss).
        //
        // In the v2 stack, this causes deletion of the dedicated log without
        // cleaning up state from the shared log, which would prevent the
        // node from re-opening (shared log will complain that the dedicated
        // log no longer exists).
        //
        // To support this scenario in FabricTest, we call ChangeRole(none)
        // to cleanly delete the dedicated log and all associated resources
        // from the shared log.
        //
        auto error = fabricNode_->Test_Reliability.Test_GetLocalStore().MarkCleanupInTerminate();

        if (!error.IsSuccess())
        {
            // Expected to fail for KVS/ESE
            //
            TestSession::WriteInfo(TraceSource, "MarkCleanupInTerminate() failed: {0}", error);
        }
    }

    if (FabricTestSessionConfig::GetConfig().StartNodeCloseOnSeparateThread)
    {
        auto root = fabricNode_->CreateAsyncOperationRoot();
        Threadpool::Post([this, root, fabricNodeWrapperSPtr, cb]()
        {
            fabricNode_->BeginClose(
                FabricTestSessionConfig::GetConfig().HostingOpenCloseTimeout,
                cb,
                fabricNode_->CreateAsyncOperationRoot());
        });
    }
    else
    {
        fabricNode_->BeginClose(
            FabricTestSessionConfig::GetConfig().HostingOpenCloseTimeout,
            cb,
            fabricNode_->CreateAsyncOperationRoot());
    }

    if(!FabricTestSessionConfig::GetConfig().AsyncNodeCloseEnabled)
    {
        closeCompleteEvent->WaitOne();
    }
}

void FabricNodeWrapper::Abort()
{
    Common::AcquireWriteLock lock(nodeRebootLock_);
    AbortInternal();
}

void FabricNodeWrapper::AbortInternal()
{   
    TestSession::FailTestIfNot(IsOpened||IsOpening, "Abort called when FabricNode is not yet opened");
    RuntimeManager->RemoveStatelessFabricRuntime(true/*abort*/);
    RuntimeManager->RemoveStatefulFabricRuntime(true/*abort*/);
    
    // remove monitoring in unreliable transport
    UnreliableTransportConfig::GetConfig().Test_StopMonitoring(nodeId_.ToString());

    fabricNode_->Abort();
    FABRICSESSION.ClosePendingItem(wformatString("TestNode.{0}", Id));
}

shared_ptr<Management::ClusterManager::ClusterManagerReplica> FabricNodeWrapper::GetCM()
{
    Common::AcquireReadLock lock(fabricNodeLock_);
    if (!fabricNode_)
    {
        TestSession::WriteInfo(TraceSource, "CM node not ready");
        return nullptr;
    }

    ASSERT_IF(fabricNode_->Test_Management.Test_ClusterManagerFactory.Test_ActiveServices.size() > 1,
        "Unexpected number of CM replicas {0}",
        fabricNode_->Test_Management.Test_ClusterManagerFactory.Test_ActiveServices.size());

    if (fabricNode_->Test_Management.Test_ClusterManagerFactory.Test_ActiveServices.size() == 0)
    {
        TestSession::WriteInfo(TraceSource, "No active CM replicas");
        return nullptr;
    }

    auto it = fabricNode_->Test_Management.Test_ClusterManagerFactory.Test_ActiveServices.begin();

    auto replica = it->second;
    ASSERT_IF(!replica, "CM replica null");

    return replica;
}

HealthManagerReplicaSPtr FabricNodeWrapper::GetHM()
{
    shared_ptr<Management::ClusterManager::ClusterManagerReplica> cm = GetCM();
    if (!cm)
    {
        TestSession::WriteInfo(TraceSource, "CM not ready");
        return nullptr;
    }

    return cm->Test_GetHealthManagerReplica();
}
