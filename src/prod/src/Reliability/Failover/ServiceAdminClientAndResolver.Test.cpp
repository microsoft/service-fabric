// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "WexTestClass.h"

#include "TestConstants.h"

namespace ReliabilityUnitTest
{
    using namespace std;
    using namespace Common;
    using namespace Federation;
    using namespace Reliability;
    using namespace Transport;

    class ServiceAdminClientAndResolver : public WEX::TestClass<ServiceAdminClientAndResolver>
    {
        struct Root;
        typedef shared_ptr<Root> RootSPtr;

    public:
        TEST_CLASS(ServiceAdminClientAndResolver)
        
            
        TEST_METHOD_SETUP(Before)
        TEST_METHOD(TestServiceCreation)
        TEST_METHOD_CLEANUP(After)

    private:
        void Cleanup();

        RootSPtr root_;

        struct Root : public ComponentRoot
        {
            static RootSPtr Create(NodeConfig const & nodeConfig, std::wstring const & runtimeServiceListenAddress)
            {
                // Disable reliable master as the FMM lookup time is not determinstic.
                FailoverConfig::GetConfig().TargetReplicaSetSize = 0;

                RootSPtr result = make_shared<Root>();
                Transport::SecuritySettings defaultSettings;
                result->FS = make_shared<FederationSubsystem>(nodeConfig, defaultSettings, nullptr, *result, Common::Uri());
                std::wstring defaultNodeUpgradeDomainId;
                std::vector<Common::Uri> defaultNodeFaultDomainIds;
                std::map<std::wstring, std::wstring> defaultNodeProperties;
                std::map<std::wstring, double> defaultCapacityRatios;
                std::map<std::wstring, double> defaultCapacities;
                result->Ipc = make_unique<IpcServer>(*result, runtimeServiceListenAddress, result->FS->Id.ToString());
                wstring workingDir;
                result->AM = make_unique<ApplicationManager>(*result, workingDir, L"127.0.0.0:12345");
                result->RuntimeManager = make_unique<FabricRuntimeManager>(
                        *result, 
                        result->FS->Id.ToString(), 
                        result->FS->LeaseAgent, 
                        *(result->Ipc),
                        *(result->AM));
                result->Runtime = make_shared<RuntimeSharingHelper>();
                result->RS = make_unique<ReliabilitySubsystem>(
                    result->FS,
                    *(result->Ipc),
                    *(result->RuntimeManager),
                    result->Runtime,
                    defaultNodeUpgradeDomainId,
                    defaultNodeFaultDomainIds,
                    defaultNodeProperties,
                    defaultCapacityRatios,
                    defaultCapacities,
                    nodeConfig.Id.ToString(),
                    L"",
                    *result);

                return result;
            }

            FederationSubsystemSPtr FS;
            IpcServerUPtr Ipc;
            ApplicationManagerUPtr AM;
            FabricRuntimeManagerUPtr RuntimeManager;
            RuntimeSharingHelperSPtr Runtime;
            ReliabilitySubsystemUPtr RS;
        };
    };

    bool ServiceAdminClientAndResolver::Before()
    {
        Cleanup();

        map<NodeId, StringCollection> seedNodes;
        StringCollection address;
        address.push_back(Federation::Constants::SeedNodeVoteType);
        address.push_back(L"127.0.0.1:12345");
        seedNodes[NodeId::MinNodeId] = address;
        FederationConfig::GetConfig().Votes = seedNodes;

        NodeConfig nodeConfig(NodeId::MinNodeId, L"127.0.0.1:12345", L"127.0.0.1:12346", L"");
        root_ = Root::Create(nodeConfig, L"127.0.0.1:12347");
        root_->RS->RegisterFederationSubsystemEvents();
        root_->RS->RegisterRuntimeManagerEvents();
        AutoResetEvent wait(false);
        root_->FS->BeginOpen(
            TimeSpan::FromSeconds(5), 
            [this, &wait] (AsyncOperationSPtr const & operation)
            { 
                VERIFY_IS_TRUE(this->root_->FS->EndOpen(operation).IsSuccess());
                wait.Set();
            },
            root_->FS->Root.CreateAsyncOperationRoot());
    
        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(5)));

        auto errorCode = root_->Ipc->Open();
        VERIFY_IS_TRUE(errorCode.IsSuccess());

        root_->RuntimeManager->BeginOpen(
            TimeSpan::FromSeconds(5),
            [this, &wait] (AsyncOperationSPtr const & operation)
            { 
                VERIFY_IS_TRUE(this->root_->RuntimeManager->EndOpen(operation).IsSuccess());
                wait.Set();
            },
            root_->FS->Root.CreateAsyncOperationRoot());
    
        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(5)));

        errorCode = root_->Runtime->Open(root_->Ipc->TransportListenAddress);
        VERIFY_IS_TRUE(errorCode.IsSuccess());

        errorCode = root_->RS->Open();
        return errorCode.IsSuccess();
    }

    bool ServiceAdminClientAndResolver::After()
    {
        VERIFY_IS_TRUE(root_->RS->Close().IsSuccess());

        root_->Runtime->Close();

        AutoResetEvent wait(false);
        root_->RuntimeManager->BeginClose(
            TimeSpan::FromSeconds(5),
            [this, &wait] (AsyncOperationSPtr const & operation)
            { 
                VERIFY_IS_TRUE(this->root_->RuntimeManager->EndClose(operation).IsSuccess());
                wait.Set();
            },
            root_->FS->Root.CreateAsyncOperationRoot());
        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(5)));

        VERIFY_IS_TRUE(root_->Ipc->Close().IsSuccess());

        root_->FS->BeginClose(
            TimeSpan::FromSeconds(5), 
            [this, &wait] (AsyncOperationSPtr const & operation)
            { 
                VERIFY_IS_TRUE(this->root_->FS->EndClose(operation).IsSuccess());
                wait.Set();
            },
            root_->FS->Root.CreateAsyncOperationRoot());
        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(5)));

        Cleanup();
        return true;
    }

    void ServiceAdminClientAndResolver::Cleanup()
    {
        // Only ever use one seed node in this test
        wstring filepath = Path::Combine(Environment::GetExecutablePath(), L"0.tkt");
        Trace.WriteInfo(TestConstants::TestSource, "Deleting file '{0}'", filepath);
        File::Delete(filepath, false);

        wstring fmStorePath;
        if (root_)
        {
            fmStorePath = FailoverConfig::GetConfig().FMStoreDirectory + FailoverConfig::GetConfig().FMStoreFilename;            
        }
        else
        {
            fmStorePath = FailoverConfig::Default_FMStoreDirectory() + FailoverConfig::Default_FMStoreFilename();
        }

        Trace.WriteInfo(TestConstants::TestSource, "Deleting FM Store at '{0}'", fmStorePath);
        File::Delete(fmStorePath, false);
    }

    void ServiceAdminClientAndResolver::TestServiceCreation()
    {        
        wstring testServiceName = L"MyTestService";
        wstring loadBalancingDomainName = Reliability::LoadBalancingComponent::Constants::DefaultLoadBalancingDomain;
        wstring placementConstraints = L"";
        int scaleoutCount = 0;
        ServiceDescription description(testServiceName, 3, 1, 1, false, false, L"TestService", loadBalancingDomainName, placementConstraints, scaleoutCount, vector<ServiceLoadMetricDescription>(), vector<byte>());
        vector<ConsistencyUnitDescription> consistencyUnitDescriptions;
        consistencyUnitDescriptions.push_back(ConsistencyUnitDescription());
        consistencyUnitDescriptions.push_back(ConsistencyUnitDescription());
        consistencyUnitDescriptions.push_back(ConsistencyUnitDescription());

        Trace.WriteInfo(TestConstants::TestSource, "Waiting for FM to activate");

        while (!root_->RS->Test_IsFailoverManagerReady())
        {
            Trace.WriteWarning(TestConstants::TestSource, "FM not ready yet. Waiting 500 milliseconds.");
            Sleep(500);
        }
        
        Trace.WriteInfo(TestConstants::TestSource, "Creating service {0}", testServiceName);

        AutoResetEvent wait(false);
        root_->RS->AdminClient.BeginCreateService(
            description,
            consistencyUnitDescriptions,
            FabricActivityHeader(),
            TimeSpan::FromSeconds(15),
            [this, &wait]
            (AsyncOperationSPtr const & operation)
            {
                VERIFY_IS_TRUE(this->root_->RS->AdminClient.EndCreateService(operation).IsSuccess());
                wait.Set();
            },
            root_->RS->Root.CreateAsyncOperationRoot());
        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(5)));

        vector<VersionedCuid> partitions;
        for (auto consistencyUnitDescription : consistencyUnitDescriptions)
        {
            partitions.push_back(VersionedCuid(consistencyUnitDescription.ConsistencyUnitId, 0));
        }
        
        Trace.WriteInfo(TestConstants::TestSource, "Resolving service {0}", testServiceName);
        vector<ServiceTableEntry> serviceLocations;
        root_->RS->Resolver.BeginResolveService(
            partitions,
            CacheMode::Refresh,
            FabricActivityHeader(),
            TimeSpan::FromSeconds(15),
            [this, &wait, &serviceLocations]
            (AsyncOperationSPtr const & operation)
            {
                ErrorCode error = this->root_->RS->Resolver.EndResolveService(operation, serviceLocations);
                VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ServiceOffline));
                wait.Set();
            },
            root_->RS->Root.CreateAsyncOperationRoot());
        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(5)));

        Trace.WriteInfo(TestConstants::TestSource, "Deleting service {0}", testServiceName);
        root_->RS->AdminClient.BeginDeleteService(
            L"MyTestService",
            FabricActivityHeader(),
            TimeSpan::FromSeconds(15),
            [this, &wait]
            (AsyncOperationSPtr const & operation)
            {
                ErrorCode error = root_->RS->AdminClient.EndDeleteService(operation);
                VERIFY_IS_TRUE(error.IsSuccess());
                wait.Set();
            },
            root_->RS->Root.CreateAsyncOperationRoot());
        VERIFY_IS_TRUE(wait.WaitOne(TimeSpan::FromSeconds(5)));
    }
}
