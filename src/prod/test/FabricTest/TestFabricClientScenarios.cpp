// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Transport/TransportConfig.h"

using namespace Api;
using namespace Common;
using namespace Client;
using namespace FabricTest;
using namespace Naming;
using namespace Reliability;
using namespace ServiceModel;
using namespace Transport;
using namespace std;
using namespace TestCommon;
using namespace Management;
using namespace Management::ClusterManager;

const StringLiteral TraceSource("FabricTest.TestFabricClientScenarios");

//
// MockServiceNotificationEventHandler
//

class TestFabricClientScenarios::MockServiceNotificationEventHandler 
    : public IServiceNotificationEventHandler
    , public ComponentRoot
{
public:
    MockServiceNotificationEventHandler()
        : ComponentRoot()
        , completionEvent_(false)
        , expectedCount_(0)
        , pendingCount_(0)
        , isTracingEnabled_(false)
        , stopwatch_()
    {
    }

    void ResetExpectedNotifications(LONG count)
    {
        completionEvent_.Reset();
        expectedCount_ = count;
        pendingCount_.store(count);
        stopwatch_.Reset();
        stopwatch_.Start();
    }

    bool WaitForExpectedNotifications(TimeSpan const & timeout, __out TimeSpan & elapsed)
    {
        auto success = completionEvent_.WaitOne(timeout);

        stopwatch_.Stop();

        if (success)
        {
            TestSession::WriteInfo(
                TraceSource, 
                "Received {0} notifications in {1}: {2} /sec", 
                expectedCount_,
                stopwatch_.Elapsed,
                (expectedCount_ / (double)stopwatch_.ElapsedMilliseconds) * 1000);

            elapsed = stopwatch_.Elapsed;
        }

        return success;
    }

public:

    // 
    // IServiceNotificationEventHandler
    //

    virtual ErrorCode OnNotification(IServiceNotificationPtr const & notification)
    {
        UNREFERENCED_PARAMETER(notification);

        auto remaining = --pendingCount_;

        auto const & ste = notification->GetServiceTableEntry();

        if (remaining  % 1000 == 0)
        {
            TestSession::WriteInfo(
                TraceSource, 
                "Notification: version={0} remaining={1}",
                ste.Version,
                remaining);
        }

        if (remaining == 0)
        {
            completionEvent_.Set();
        }

        return ErrorCodeValue::Success;
    }

private:
    ManualResetEvent completionEvent_;
    LONG expectedCount_;
    Common::atomic_long pendingCount_;
    bool isTracingEnabled_;

    Stopwatch stopwatch_;
};

//
// TestFabricClientScenarios
//

TestFabricClientScenarios::TestFabricClientScenarios(FabricTestDispatcher & dispatcher, std::shared_ptr<TestFabricClient> const & testClientSPtr)
    : dispatcher_(dispatcher)
    , testClientSPtr_(testClientSPtr)
    , scenarioMap_()
{
    this->AddScenario(L"ParallelCreateDeleteEnumerateNames", ParallelCreateDeleteEnumerateNames);
    this->AddScenario(L"ClusterManagerLargeKeys", ClusterManagerLargeKeys);
    this->AddScenario(L"ResolveServicePerformance", ResolveServicePerformance);
    this->AddScenario(L"PushNotificationsPerformance", PushNotificationsPerformance);
    this->AddScenario(L"PushNotificationsLeak", PushNotificationsLeak);
    this->AddScenario(L"DeleteServiceLeak", DeleteServiceLeak);
    this->AddScenario(L"ImageBuilderApplicationScale", ImageBuilderApplicationScale);
    this->AddScenario(L"ApplicationThroughput", ApplicationThroughput);
}

void TestFabricClientScenarios::AddScenario(
    wstring const & scenarioName,
    ScenarioCallback const & scenario)
{
    scenarioMap_.insert(make_pair(scenarioName, scenario));
}

bool TestFabricClientScenarios::RunScenario(StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ScenariosCommand);
        return false;
    }

    wstring const & scenarioName = params[0];
    vector<wstring> scenarioParams;
    for (auto ix = 1; ix < params.size(); ++ix) 
    { 
        scenarioParams.push_back(move(params[ix])); 
    }

    auto findIt = scenarioMap_.find(scenarioName);
    if (findIt == scenarioMap_.end())
    {
        TestSession::WriteError(TraceSource, "Scenario '{0}' not found - use AddScenario() in TestFabricClientScenarios::ctor to add it.", scenarioName);
        return false;
    }

    return findIt->second(dispatcher_, testClientSPtr_, scenarioParams);
}

// Test stability of name creation/deletion in parallel with name enumeration
//
bool TestFabricClientScenarios::ParallelCreateDeleteEnumerateNames(
    FabricTestDispatcher & dispatcher,
    shared_ptr<TestFabricClient> const & client,
    StringCollection const & params)
{
    UNREFERENCED_PARAMETER(dispatcher);

    if (params.size() != 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ScenariosCommand);
        return false;
    }

    NamingUri rootName;
    if (!NamingUri::TryParse(params[0], rootName))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    auto childName = NamingUri::Combine(rootName, L"789586");
    ManualResetEvent wait;
    bool run = true;
    int n = 0;

    Threadpool::Post([&]() 
    {
        for (; run; n++) 
        { 
            client->CreateName(childName,  L"",S_OK); 
            client->DeleteName(childName, L"", S_OK); 
        }
        wait.Set();
    });

    int maxResults = numeric_limits<int>::max();
    FABRIC_ENUMERATION_STATUS expectedStatus = static_cast<FABRIC_ENUMERATION_STATUS>(FABRIC_ENUMERATION_CONSISTENT_MASK & FABRIC_ENUMERATION_FINISHED_MASK);

    for (int i=0; (i<100)||(n<100); i++)
    {
        StringCollection expectedNames;
        client->EnumerateNames(rootName, false, L"", maxResults, false, expectedNames, false, expectedStatus, S_OK);
    }
    run = false;
    wait.WaitOne();

    return true;
}

// Test error-checking against large persisted store keys in Cluster Manager
//
bool TestFabricClientScenarios::ClusterManagerLargeKeys(
    FabricTestDispatcher & dispatcher,
    std::shared_ptr<TestFabricClient> const & client, 
    Common::StringCollection const & params)
{
    UNREFERENCED_PARAMETER(params);

    std::wstring TestBuildPath1(L"TestBuildPath1");
    std::wstring TestBuildPath2(L"TestBuildPath2");
    std::wstring TestBuildPath3(L"TestBuildPath3");
    std::wstring TestBuildPath4(L"TestBuildPath4");

    std::wstring goodAppTypeName(Management::ManagementConfig::GetConfig().MaxApplicationTypeNameLength, 't');
    std::wstring badAppTypeName(Management::ManagementConfig::GetConfig().MaxApplicationTypeNameLength + 1, 'T');
    badAppTypeName[badAppTypeName.size() - 1] = 'X';

    std::wstring goodAppTypeVersion(Management::ManagementConfig::GetConfig().MaxApplicationTypeVersionLength, 'v');
    std::wstring badAppTypeVersion(Management::ManagementConfig::GetConfig().MaxApplicationTypeVersionLength + 1, 'V');
    badAppTypeVersion[badAppTypeVersion.size() - 1] = 'X';

    std::wstring goodServiceTypeName(Management::ManagementConfig::GetConfig().MaxServiceTypeNameLength, 's');
    std::wstring badServiceTypeName(Management::ManagementConfig::GetConfig().MaxServiceTypeNameLength + 1, 'S');
    badServiceTypeName[badServiceTypeName.size() - 1] = 'X';

    std::wstring DefaultServiceName(L"DefaultServiceA");

    TestFabricClientScenarios::UploadHelper(
        TestBuildPath1,
        badAppTypeName,
        goodAppTypeVersion,
        goodServiceTypeName,
        DefaultServiceName,
        dispatcher);

    TestFabricClientScenarios::UploadHelper(
        TestBuildPath2,
        goodAppTypeName,
        badAppTypeVersion,
        goodServiceTypeName,
        DefaultServiceName,
        dispatcher);

    TestFabricClientScenarios::UploadHelper(
        TestBuildPath3,
        goodAppTypeName,
        goodAppTypeVersion,
        badServiceTypeName,
        DefaultServiceName,
        dispatcher);
    
    TestFabricClientScenarios::UploadHelper(
        TestBuildPath4,
        goodAppTypeName,
        goodAppTypeVersion,
        goodServiceTypeName,
        DefaultServiceName,
        dispatcher);
    
    TestFabricClientUpgrade upgradeClient(client);

    upgradeClient.ProvisionApplicationTypeImpl(Path::Combine(L"incoming", TestBuildPath1), false, ApplicationPackageCleanupPolicy::Enum::Manual, FABRIC_E_VALUE_TOO_LARGE);
    upgradeClient.ProvisionApplicationTypeImpl(Path::Combine(L"incoming", TestBuildPath2), false, ApplicationPackageCleanupPolicy::Enum::Manual, FABRIC_E_VALUE_TOO_LARGE);
    upgradeClient.ProvisionApplicationTypeImpl(Path::Combine(L"incoming", TestBuildPath3), false, ApplicationPackageCleanupPolicy::Enum::Manual, FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR);
    upgradeClient.ProvisionApplicationTypeImpl(Path::Combine(L"incoming", TestBuildPath4), false, ApplicationPackageCleanupPolicy::Enum::Manual, FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR);

    return true;
}

void TestFabricClientScenarios::UploadHelper(
    wstring const & buildPath,
    wstring const & appTypeName,
    wstring const & appTypeVersion,
    wstring const & serviceTypeName,
    wstring const & defaultServiceName,
    FabricTestDispatcher & dispatcher)
{
    wstring ServicePackageName(L"ServicePackageA");
    wstring ServicePackageVersion(L"vA");
    wstring CodePackageName(L"CodePackageA");
    wstring CodePackageVersion(L"vA");

    StringCollection builderParams;
    builderParams.push_back(buildPath);
    builderParams.push_back(appTypeName);
    builderParams.push_back(appTypeVersion);

    dispatcher.ExecuteApplicationBuilderCommand(
        ApplicationBuilder::ApplicationBuilderCreateApplicationType,
        builderParams);

    builderParams.clear();
    builderParams.push_back(buildPath);
    builderParams.push_back(ServicePackageName);
    builderParams.push_back(wformatString("version={0}", ServicePackageVersion));

    dispatcher.ExecuteApplicationBuilderCommand(
        ApplicationBuilder::ApplicationBuilderSetServicePackage,
        builderParams);

    builderParams.clear();
    builderParams.push_back(buildPath);
    builderParams.push_back(ServicePackageName);
    builderParams.push_back(serviceTypeName);
    builderParams.push_back(L"stateless");

    dispatcher.ExecuteApplicationBuilderCommand(
        ApplicationBuilder::ApplicationBuilderSetServiceTypes,
        builderParams);

    builderParams.clear();
    builderParams.push_back(buildPath);
    builderParams.push_back(ServicePackageName);
    builderParams.push_back(CodePackageName);
    builderParams.push_back(wformatString("version={0}", CodePackageVersion));
    wstring types(wformatString("types={0}", serviceTypeName));

    dispatcher.ExecuteApplicationBuilderCommand(
        ApplicationBuilder::ApplicationBuilderSetCodePackage,
        builderParams);

    builderParams.clear();
    builderParams.push_back(buildPath);
    builderParams.push_back(defaultServiceName);
    builderParams.push_back(serviceTypeName);
    builderParams.push_back(L"stateless");

    dispatcher.ExecuteApplicationBuilderCommand(
        ApplicationBuilder::ApplicationBuilderSetDefaultService,
        builderParams);

    builderParams.clear();
    builderParams.push_back(buildPath);

    dispatcher.ExecuteApplicationBuilderCommand(
        ApplicationBuilder::ApplicationBuilderUpload,
        builderParams);
}

bool TestFabricClientScenarios::ResolveServicePerformance(
    FabricTestDispatcher & dispatcher,
    shared_ptr<TestFabricClient> const & client,
    StringCollection const & params)
{
    UNREFERENCED_PARAMETER(dispatcher);

    CommandLineParser parser(params, 0);

    vector<wstring> connectionStrings;
    shared_ptr<TestClientGateway> mockGateway;

    int bucketCount;
    parser.TryGetInt(L"buckets", bucketCount, 1024);

    int keyCount;
    parser.TryGetInt(L"keys", keyCount, 1);

    int partitionCount;
    parser.TryGetInt(L"partitions", partitionCount, 1);

    int replicaCount;
    parser.TryGetInt(L"replicas", replicaCount, 3);

    int sgMemberCount;
    parser.TryGetInt(L"sgmembers", sgMemberCount, 0);

    if (parser.GetBool(L"mockgateway"))
    {
        PartitionedServiceDescriptor mockPsd;
        auto error = PartitionedServiceDescriptor::Create(
            CreateServiceDescription(L"fabric:/mock_service_template", partitionCount),
            0, keyCount-1, mockPsd);

        if (!error.IsSuccess())
        {
            TestSession::WriteError(
                TraceSource, 
                "Failed to create mock PSD template: {0}",
                error);
            return false;
        }

        Random rand;
        NodeId nodeId;
        if (!NodeId::TryParse(wformatString("{0}", rand.Next(0, 1024)), nodeId))
        {
            TestSession::WriteError(
                TraceSource, 
                "Failed to parse nodeId");
            return false;
        }

        mockGateway = make_shared<TestClientGateway>(
            nodeId,
            replicaCount,
            sgMemberCount);

        mockGateway->SetMockServiceTemplate(mockPsd);

        wstring address;
        error = mockGateway->Open(address);

        if (!error.IsSuccess())
        {
            TestSession::WriteError(
                TraceSource, 
                "Failed to open mock gateway: {0}",
                error);
            return false;
        }

        connectionStrings.push_back(address);
    }
    else
    {
        FABRICSESSION.FabricDispatcher.Federation.GetEntreeServiceAddress(connectionStrings);

        bool success = client->PerformanceTestVariation2(params);
        if (!success) { return success; }
    }

    bool printDetails = parser.GetBool(L"printdetails");

    bool usePreviousRsp = parser.GetBool(L"previousrsp");

    int serviceCount;
    parser.TryGetInt(L"serviceCount", serviceCount, 1000);

    int notificationCount;
    parser.TryGetInt(L"notificationCount", notificationCount, 0);

    int expectedNotificationCount;
    parser.TryGetInt(L"expectedNotificationCount", expectedNotificationCount, 0);

    int sharedCount;
    parser.TryGetInt(L"sharedCount", sharedCount, 0);

    vector<NamingUri> fullNames = client->GetPerformanceTestNames(
        notificationCount + serviceCount, 
        parser);

    if (serviceCount + notificationCount != static_cast<int>(fullNames.size()))
    {
        TestSession::WriteWarning(
            TraceSource, 
            "count != size: {0} + {1} != {2}",
            serviceCount,
            notificationCount,
            fullNames.size());
        return false;
    }

    if (sharedCount > serviceCount)
    {
        TestSession::WriteWarning(
            TraceSource, 
            "sharedCount > serviceCount: {0} > {1}",
            sharedCount,
            serviceCount);
        return false;
    }

    auto resolveTimeout = FabricTestSessionConfig::GetConfig().NamingOperationTimeout;

    // V1 cache will cache each member in addition to the SG name
    //
    int cacheLimit;
    parser.TryGetInt(L"cachelimit", cacheLimit, (serviceCount + notificationCount) * (sgMemberCount + 1));

    int iterationCount;
    parser.TryGetInt(L"iterations", iterationCount, 2);

    auto clientImpl = make_shared<FabricClientImpl>(move(connectionStrings));
    auto settings = clientImpl->GetSettings();
    settings.SetClientFriendlyName(L"CacheTestScenario");
    settings.SetServiceChangePollIntervalInSeconds(1);
    settings.SetPartitionLocationCacheLimit(cacheLimit);
    settings.SetPartitionLocationCacheBucketCount(bucketCount);
    clientImpl->SetSettings(move(settings));

    IResolvedServicePartitionResultPtr previousRsp;
    auto operationRoot = clientImpl->CreateAsyncOperationRoot();

    Stopwatch stopwatch;

    auto operationCount = serviceCount * keyCount * (sgMemberCount > 0 ? sgMemberCount : 1);

    Common::atomic_long pendingCount(operationCount);
    Common::atomic_long failuresCount(0);

    Common::atomic_long notificationSuccessCount(0);
    Common::atomic_long notificationErrorCount(0);

    ManualResetEvent completedEvent(false);
    ManualResetEvent notificationsEvent(false);

    for (auto iteration = 0; iteration < iterationCount; ++iteration)
    {
        vector<LONGLONG> handlerIds;

        pendingCount.store(operationCount);
        notificationSuccessCount.store(0);
        failuresCount.store(0);
        completedEvent.Reset();
        notificationsEvent.Reset();

        stopwatch.Restart();

        for (auto ix = 0; ix < notificationCount + sharedCount; ++ix)
        {
            for (auto jx = 0; jx< (sgMemberCount > 0 ? sgMemberCount : 1); ++jx)
            {
                auto serviceName = fullNames[ix];

                if (sgMemberCount > 0 && 
                    !NamingUri::TryParse(wformatString("{0}#Member{1}", serviceName, jx), serviceName))
                {
                    TestSession::WriteError(
                        TraceSource, 
                        "Failed to create SG name: {0} ({1})",
                        serviceName,
                        jx);
                    return false;
                }

                for (auto kx = 0; kx < keyCount; ++kx)
                {
                    PartitionKey pKey(kx);
                    ServiceResolutionRequestData requestData(pKey);

                    LONGLONG handlerId;
                    auto error = clientImpl->RegisterServicePartitionResolutionChangeHandler(
                        serviceName,
                        pKey,
                        [&](LONGLONG handlerId, IResolvedServicePartitionResultPtr const & rsp, ErrorCode error)
                        { 
                            ProcessNotification(
                                expectedNotificationCount, 
                                notificationSuccessCount, 
                                notificationErrorCount, 
                                handlerId, 
                                rsp, 
                                error, 
                                notificationsEvent,
                                printDetails);
                        },
                        &handlerId);

                    if (!error.IsSuccess())
                    {
                        TestSession::WriteError(
                            TraceSource, 
                            "Failed to register notification: {0}, {1}",
                            serviceName,
                            requestData);
                        return false;
                    }

                    if (printDetails)
                    {
                        TestSession::WriteInfo(
                            TraceSource, 
                            "Registered notification[{0}]: {1}, {2}",
                            handlerId,
                            serviceName,
                            requestData);
                    }

                    handlerIds.push_back(handlerId);
                } // keys
            } // sg members
        } // notifications

        for (auto ix = notificationCount; ix < notificationCount + serviceCount; ++ix)
        {
            for (auto jx = 0; jx< (sgMemberCount > 0 ? sgMemberCount : 1); ++jx)
            {
                auto serviceName = fullNames[ix];

                if (sgMemberCount > 0 && 
                    !NamingUri::TryParse(wformatString("{0}#Member{1}", serviceName, jx), serviceName))
                {
                    TestSession::WriteError(
                        TraceSource, 
                        "Failed to create SG name: {0} ({1})",
                        serviceName,
                        jx);
                    return false;
                }

                for (auto kx = 0; kx < keyCount; ++kx)
                {
                    PartitionKey pKey(kx);
                    ServiceResolutionRequestData requestData(pKey);

                    if (mockGateway && usePreviousRsp)
                    {
                        ResolvedServicePartitionSPtr rsp;
                        auto error = mockGateway->GetCurrentMockRsp(serviceName, requestData, rsp);

                        if (!error.IsSuccess())
                        {
                            TestSession::WriteError(
                                TraceSource, 
                                "Failed to get current mock RSP for {0}",
                                requestData);
                            return false;
                        }

                        auto rspResult = make_shared<ResolvedServicePartitionResult>(rsp);
                        previousRsp = RootedObjectPointer<IResolvedServicePartitionResult>(
                            rspResult.get(),
                            rspResult->CreateComponentRoot());
                    }

                    clientImpl->BeginResolveServicePartition(
                        serviceName,
                        requestData,
                        previousRsp,
                        resolveTimeout,
                        [&, serviceName](AsyncOperationSPtr const & operation) 
                        { 
                            IResolvedServicePartitionResultPtr result;
                            auto error = clientImpl->EndResolveServicePartition(operation, result);

                            ProcessResolveResult(
                                serviceName, 
                                error, 
                                result, 
                                pendingCount, 
                                failuresCount, 
                                completedEvent, 
                                printDetails);
                        },
                        operationRoot);
                } // keys
            } // sg members
        } // resolution

        if (serviceCount > 0)
        {
            completedEvent.WaitOne();
        }

        if (expectedNotificationCount > 0)
        {
            notificationsEvent.WaitOne();
        }

        stopwatch.Stop();

        for (auto it = handlerIds.begin(); it != handlerIds.end(); ++it)
        {
            auto error = clientImpl->UnRegisterServicePartitionResolutionChangeHandler(*it);

            if (!error.IsSuccess())
            {
                TestSession::WriteError(
                    TraceSource, 
                    "Failed to unregister notification: {0}, {1}",
                    *it,
                    error);
                return false;
            }
        }

        TestSession::WriteWarning(
            TraceSource, 
            "Resolve[{0}:{1}] count={2} keys={3} sg={4} cache={5} elapsed={6} ops={7}/sec failures={8}",
            bucketCount,
            iteration,
            serviceCount,
            keyCount,
            sgMemberCount,
            cacheLimit,
            stopwatch.Elapsed,
            ((double)operationCount / stopwatch.ElapsedMilliseconds) * 1000,
            failuresCount.load());

        if (notificationCount > 0)
        {
            TestSession::WriteWarning(
                TraceSource, 
                "Notification[{0}:{1}] count={2} ops={3}/sec failures={4}",
                bucketCount,
                iteration,
                notificationSuccessCount.load(),
                ((double)notificationSuccessCount.load() / stopwatch.ElapsedMilliseconds) * 1000,
                notificationErrorCount.load());
        }

        Sleep(5000);

    } // for iterations

    clientImpl->Close();

    if (mockGateway)
    {
        mockGateway->Close();
    }

    return true;
}

bool TestFabricClientScenarios::PushNotificationsPerformance(
    FabricTestDispatcher & dispatcher,
    shared_ptr<TestFabricClient> const & client,
    StringCollection const & params)
{
    UNREFERENCED_PARAMETER(dispatcher);
    UNREFERENCED_PARAMETER(client);

    Transport::TransportConfig::GetConfig().DefaultSendQueueSizeLimit = 0;

    CommandLineParser parser(params, 0);

    vector<wstring> connectionStrings;
    shared_ptr<TestClientGateway> mockGateway;

    NamingUri uriPrefix(L"pNotifications");

    int keyCount;
    parser.TryGetInt(L"keys", keyCount, 1);

    int partitionCount;
    parser.TryGetInt(L"partitions", partitionCount, 1);

    int replicaCount;
    parser.TryGetInt(L"replicas", replicaCount, 3);

    int sgMemberCount;
    parser.TryGetInt(L"sgmembers", sgMemberCount, 0);

    int serviceCount;
    parser.TryGetInt(L"serviceCount", serviceCount, 10);

    int subnameCount;
    parser.TryGetInt(L"subnameCount", subnameCount, 1);

    int expectedNotificationCount;
    parser.TryGetInt(
        L"expectedNotificationCount", 
        expectedNotificationCount, serviceCount * subnameCount * partitionCount * (sgMemberCount > 0 ? sgMemberCount : 1));

    int batchCount;
    parser.TryGetInt(L"batchCount", batchCount, 10);

    int cacheLimit;
    parser.TryGetInt(L"cacheLimit", cacheLimit, serviceCount * subnameCount * partitionCount);

    int notificationLimit;
    parser.TryGetInt(L"notificationLimit", notificationLimit, expectedNotificationCount);

    int iterations;
    parser.TryGetInt(L"iterations", iterations, 5);

    int iterationSleep;
    parser.TryGetInt(L"iterationSleep", iterationSleep, 30);

    int sleepBeforeDelete;
    parser.TryGetInt(L"sleepBeforeDelete", sleepBeforeDelete, 30);

    int notificationBurst;
    parser.TryGetInt(L"notificationBurst", notificationBurst, 0);

    {
        PartitionedServiceDescriptor mockPsd;
        auto error = PartitionedServiceDescriptor::Create(
            CreateServiceDescription(L"fabric:/[placeholder]", partitionCount),
            0, keyCount-1, mockPsd);

        if (!error.IsSuccess())
        {
            TestSession::WriteError(
                TraceSource, 
                "Failed to create mock PSD template: {0}",
                error);
            return false;
        }

        Random rand;
        NodeId nodeId;
        if (!NodeId::TryParse(wformatString("{0}", rand.Next(0, 1024)), nodeId))
        {
            TestSession::WriteError(
                TraceSource, 
                "Failed to parse nodeId");
            return false;
        }

        mockGateway = make_shared<TestClientGateway>(
            nodeId,
            replicaCount,
            sgMemberCount);

        mockGateway->SetMockServiceTemplate(mockPsd);

        mockGateway->GenerateMockServicePsds(uriPrefix, serviceCount, subnameCount);

        if (notificationBurst > 0)
        {
            mockGateway->SetNotificationBurst(notificationBurst);
        }

        wstring address;
        error = mockGateway->Open(address);

        if (!error.IsSuccess())
        {
            TestSession::WriteError(
                TraceSource, 
                "Failed to open mock gateway: {0}",
                error);
            return false;
        }

        connectionStrings.push_back(address);
    }

    auto notificationEventHandler = make_shared<MockServiceNotificationEventHandler>();
    auto ptr = IServiceNotificationEventHandlerPtr(notificationEventHandler.get(), notificationEventHandler->CreateComponentRoot());

    auto clientImpl = make_shared<FabricClientImpl>(move(connectionStrings), ptr);

    auto settings = clientImpl->GetSettings();
    settings.SetClientFriendlyName(L"PushNotificationsScenario");
    settings.SetPartitionLocationCacheLimit(cacheLimit);
    clientImpl->SetSettings(move(settings));

    auto operationRoot = clientImpl->CreateAsyncOperationRoot();

    for (auto ix=0; ix<serviceCount; ++ix)
    {
        auto name = mockGateway->CreateServiceNameFromPrefix(uriPrefix, ix);

        ServiceNotificationFilterFlags flags = ServiceNotificationFilterFlags::NamePrefix;
        auto filter = make_shared<ServiceNotificationFilter>(name, flags);
            
        ManualResetEvent callbackEvent(false);

        auto operation = clientImpl->BeginRegisterServiceNotificationFilter(
            filter,
            TimeSpan::FromSeconds(10),
            [&callbackEvent](AsyncOperationSPtr const &) { callbackEvent.Set(); },
            operationRoot);

        callbackEvent.WaitOne();

        uint64 filterId;
        auto error = clientImpl->EndRegisterServiceNotificationFilter(operation, filterId);
        if (!error.IsSuccess())
        {
            TestSession::WriteError(
                TraceSource, 
                "Failed to register filter: {0} ({1}) error={2}",
                *filter,
                filterId,
                error);
            return false;
        }
    }

    ServiceModelConfig::GetConfig().MaxOutstandingNotificationsPerClient = notificationLimit;

    TimeSpan elapsed;
    vector<TimeSpan> updateResults;
    vector<TimeSpan> deleteResults;

    for (auto iteration=0; iteration<iterations; ++iteration)
    {
        if (iteration != 0)
        {
            Sleep(iterationSleep * 1000);
        }

        TestSession::WriteInfo(
            TraceSource, 
            "Starting iteration {0}",
            iteration);

        notificationEventHandler->ResetExpectedNotifications(expectedNotificationCount);

        auto rounds = expectedNotificationCount / (serviceCount * subnameCount * partitionCount * (sgMemberCount > 0 ? sgMemberCount : 1));
        
        TestSession::WriteInfo(
            TraceSource, 
            "Generating {0} cache updates in {1} rounds, {2} updates per batch.",
            expectedNotificationCount,
            rounds,
            batchCount);

        for (auto ix=0; ix<rounds; ++ix)
        {
            mockGateway->GenerateCacheUpdates(uriPrefix, serviceCount, subnameCount, batchCount);
        }

        TestSession::WriteInfo(
            TraceSource, 
            "Waiting for {0} notification updates...",
            expectedNotificationCount);

        auto success = notificationEventHandler->WaitForExpectedNotifications(TimeSpan::FromSeconds(600), elapsed);
        if (!success)
        {
            TestSession::WriteError(
                TraceSource, 
                "Timed out waiting to receive expected notifications");
            return false;
        }

        updateResults.push_back(elapsed);

        if (sleepBeforeDelete > 0)
        {
            TestSession::WriteInfo(
                TraceSource, 
                "Waiting {0} seconds before generating deletes...",
                sleepBeforeDelete);

            Sleep(sleepBeforeDelete * 1000);

            notificationEventHandler->ResetExpectedNotifications(expectedNotificationCount);

            TestSession::WriteInfo(
                TraceSource, 
                "Generating {0} cache deletes in {1} rounds, {2} updates per batch.",
                expectedNotificationCount,
                rounds,
                batchCount);

            for (auto ix=0; ix<rounds; ++ix)
            {
                mockGateway->GenerateCacheDeletes(uriPrefix, serviceCount, subnameCount, batchCount);
            }

            TestSession::WriteInfo(
                TraceSource, 
                "Waiting for {0} notification deletes...",
                expectedNotificationCount);

            success = notificationEventHandler->WaitForExpectedNotifications(TimeSpan::FromSeconds(600), elapsed);
            if (!success)
            {
                TestSession::WriteError(
                    TraceSource, 
                    "Timed out waiting to receive expected notifications");
                return false;
            }

            deleteResults.push_back(elapsed);
        }
    }

    TestSession::WriteInfo(
        TraceSource, 
        "Update Results: serviceCount={0} subnameCount={1} partitionCount={2} batchCount={3}",
        serviceCount,
        subnameCount,
        partitionCount,
        batchCount);

    for (auto ix=0; ix<updateResults.size(); ++ix)
    {
        TestSession::WriteInfo(
            TraceSource, 
            "{0}: count={1} elapsed={2} throughput={3} /sec",
            ix,
            expectedNotificationCount,
            updateResults[ix],
            (expectedNotificationCount / (double)updateResults[ix].TotalMilliseconds()) * 1000);
    }

    if (sleepBeforeDelete > 0)
    {
        TestSession::WriteInfo(
            TraceSource, 
            "Delete Results: serviceCount={0} subnameCount={1} partitionCount={2} batchCount={3}",
            serviceCount,
            subnameCount,
            partitionCount,
            batchCount);

        for (auto ix=0; ix<deleteResults.size(); ++ix)
        {
            TestSession::WriteInfo(
                TraceSource, 
                "{0}: count={1} elapsed={2} throughput={3} /sec",
                ix,
                expectedNotificationCount,
                deleteResults[ix],
                (expectedNotificationCount / (double)deleteResults[ix].TotalMilliseconds()) * 1000);
        }
    }

    auto error = clientImpl->Close();
    if (!error.IsSuccess())
    {
        TestSession::WriteError(
            TraceSource, 
            "Failed to close client: {0}",
            error);
        return false;
    }

    mockGateway->Close();

    return true;
}

// Not used in any automation scripts - run manually
//
bool TestFabricClientScenarios::PushNotificationsLeak(
    FabricTestDispatcher & dispatcher,
    shared_ptr<TestFabricClient> const & client,
    StringCollection const & params)
{
    UNREFERENCED_PARAMETER(dispatcher);
    UNREFERENCED_PARAMETER(client);

    Transport::TransportConfig::GetConfig().DefaultSendQueueSizeLimit = 0;

    CommandLineParser parser(params, 0);

    int filterCount;
    parser.TryGetInt(L"filtercount", filterCount, 8000);

    for (auto iteration=0;;++iteration)
    {
        TestSession::WriteInfo(TraceSource, "Registration iteration {0}: filters={1}", iteration, filterCount);

        auto notificationEventHandler = make_shared<MockServiceNotificationEventHandler>();
        auto ptr = IServiceNotificationEventHandlerPtr(notificationEventHandler.get(), notificationEventHandler->CreateComponentRoot());

        vector<wstring> connectionStrings;
        FABRICSESSION.FabricDispatcher.Federation.GetEntreeServiceAddress(connectionStrings);
        auto clientImpl = make_shared<FabricClientImpl>(move(connectionStrings), ptr);

        auto settings = clientImpl->GetSettings();
        settings.SetClientFriendlyName(L"PushNotificationsLeak");
        clientImpl->SetSettings(move(settings));

        auto operationRoot = clientImpl->CreateAsyncOperationRoot();

        for (auto ix=0; ix<filterCount; ++ix)
        {
            auto name = NamingUri(wformatString("leak_test_{0}", ix));

            ServiceNotificationFilterFlags flags = ServiceNotificationFilterFlags::NamePrefix;
            auto filter = make_shared<ServiceNotificationFilter>(name, flags);
                
            ManualResetEvent callbackEvent(false);

            auto operation = clientImpl->BeginRegisterServiceNotificationFilter(
                filter,
                TimeSpan::FromSeconds(10),
                [&callbackEvent](AsyncOperationSPtr const &) { callbackEvent.Set(); },
                operationRoot);

            callbackEvent.WaitOne();

            uint64 filterId;
            auto error = clientImpl->EndRegisterServiceNotificationFilter(operation, filterId);
            if (!error.IsSuccess())
            {
                TestSession::WriteError(
                    TraceSource, 
                    "Failed to register filter: {0} ({1}) error={2}",
                    *filter,
                    filterId,
                    error);
                return false;
            }
        }

        auto error = clientImpl->Close();
        if (!error.IsSuccess())
        {
            TestSession::WriteError(
                TraceSource, 
                "Failed to close client: {0}",
                error);
            return false;
        }
    }

    return true;
}

// Not used in any automation scripts - run manually
//
bool TestFabricClientScenarios::DeleteServiceLeak(
    FabricTestDispatcher & dispatcher,
    shared_ptr<TestFabricClient> const & client,
    StringCollection const & params)
{
    UNREFERENCED_PARAMETER(dispatcher);
    UNREFERENCED_PARAMETER(client);

    CommandLineParser parser(params, 0);

    wstring svcName;
    parser.TryGetString(L"name", svcName, L"");

    auto name = NamingUri(svcName);

    int timeoutInSeconds;
    parser.TryGetInt(L"timeout", timeoutInSeconds, 10);

    vector<wstring> connectionStrings;
    FABRICSESSION.FabricDispatcher.Federation.GetEntreeServiceAddress(connectionStrings);
    auto clientImpl = make_shared<FabricClientImpl>(move(connectionStrings));

    auto settings = clientImpl->GetSettings();
    settings.SetClientFriendlyName(L"DeleteServiceLeak");
    clientImpl->SetSettings(move(settings));

    auto operationRoot = clientImpl->CreateAsyncOperationRoot();

    bool success = false;
    while (!success)
    {
        TestSession::WriteInfo(TraceSource, "Deleting {0}", svcName);

        ManualResetEvent callbackEvent(false);

        ServiceModel::DeleteServiceDescription deleteServiceDescription(name);
        auto operation = clientImpl->BeginDeleteService(
            deleteServiceDescription,
            TimeSpan::FromSeconds(timeoutInSeconds),
            [&callbackEvent](AsyncOperationSPtr const &) { callbackEvent.Set(); },
            operationRoot);

        callbackEvent.WaitOne();

        auto error = clientImpl->EndDeleteService(operation);
        if (error.IsError(ErrorCodeValue::Timeout))
        {
            // no-op: retry
        }
        else if (error.IsSuccess() || error.IsError(ErrorCodeValue::UserServiceNotFound))
        {
            success = true;
        }
        else if (!error.IsSuccess())
        {
            TestSession::WriteError(
                TraceSource, 
                "Failed to delete service {0}: error={1}",
                name,
                error);
            return false;
        }
    }

    auto error = clientImpl->Close();
    if (!error.IsSuccess())
    {
        TestSession::WriteError(
            TraceSource, 
            "Failed to close client: {0}",
            error);
        return false;
    }
    
    TestSession::WriteInfo(
        TraceSource, 
        "Finished running DeleteServiceLeak");

    return true;
}

bool TestFabricClientScenarios::ImageBuilderApplicationScale(
    FabricTestDispatcher & dispatcher,
    shared_ptr<TestFabricClient> const & client,
    StringCollection const & params)
{
    UNREFERENCED_PARAMETER(dispatcher);
    UNREFERENCED_PARAMETER(client);

    Transport::TransportConfig::GetConfig().DefaultSendQueueSizeLimit = 0;

    Random rand(static_cast<int>(DateTime::Now().Ticks));
    CommandLineParser parser(params, 0);

    wstring appName;
    parser.TryGetString(L"appname", appName, L"fabric:/AppName");

    wstring appTypeName;
    parser.TryGetString(L"apptype", appTypeName, L"AppType");

    wstring appVersion1;
    parser.TryGetString(L"appversion1", appVersion1, L"AppVersion1");

    wstring appVersion2;
    parser.TryGetString(L"appversion2", appVersion2, L"AppVersion2");

    int applicationCount;
    parser.TryGetInt(L"appcount", applicationCount, 1000);

    int timeoutInSeconds;
    parser.TryGetInt(L"timeout", timeoutInSeconds, 60);

    bool updateThrottles = parser.GetBool(L"updatethrottles");

    int minThrottle;
    parser.TryGetInt(L"minthrottle", minThrottle, 5);

    int maxThrottle;
    parser.TryGetInt(L"maxthrottle", maxThrottle, 50);

    TimeSpan timeout = TimeSpan::FromSeconds(timeoutInSeconds);

    vector<wstring> connectionStrings;
    FABRICSESSION.FabricDispatcher.Federation.GetEntreeServiceAddress(connectionStrings);

    auto clientImpl = make_shared<FabricClientImpl>(move(connectionStrings));

    auto settings = clientImpl->GetSettings();
    settings.SetClientFriendlyName(L"ImageBuilderApplicationScale");
    clientImpl->SetSettings(move(settings));

    auto operationRoot = clientImpl->CreateAsyncOperationRoot();

    Common::atomic_long pendingOperations(applicationCount);
    Common::atomic_long successCount(0);
    ManualResetEvent event(false);

    //
    // Create application
    //
    for (auto ix=0; ix<applicationCount; ++ix)
    {
        ApplicationDescriptionWrapper appDesc(
            wformatString("{0}/{1}", appName, ix),
            appTypeName,
            appVersion1,
            map<wstring, wstring>());

        clientImpl->BeginCreateApplication(
            appDesc,
            timeout,
            [&](AsyncOperationSPtr const & operation)
            {
                auto error = clientImpl->EndCreateApplication(operation);

                if (error.IsSuccess())
                {
                    auto count = ++successCount;
                    if (count % 10 == 0)
                    {
                        TestSession::WriteInfo(
                            TraceSource,
                            "CreateApplication success: {0}",
                            count);

                        if (updateThrottles)
                        {
                            Management::ManagementConfig::GetConfig().ImageBuilderJobQueueThrottle = rand.Next(minThrottle, maxThrottle);
                            Management::ManagementConfig::GetConfig().OnUpdate(L"ClusterManager", L"ImageBuilderJobQueueThrottle");

                            Management::ManagementConfig::GetConfig().ImageBuilderUpgradeJobQueueThrottle = rand.Next(minThrottle, maxThrottle);
                            Management::ManagementConfig::GetConfig().OnUpdate(L"ClusterManager", L"ImageBuilderUpgradeJobQueueThrottle");
                        }
                    }
                }
                else
                {
                    TestSession::WriteError(
                        TraceSource,
                        "CreateApplication: {0}",
                        error);
                }

                if (--pendingOperations == 0)
                {
                    event.Set();
                }
            },
            operationRoot);
    }

    bool waitResult = event.WaitOne(timeout);

    if (!waitResult)
    {
        TestSession::WriteError(
            TraceSource,
            "++ Failed: total={0} success={1} waitResult={2}",
            applicationCount,
            successCount.load(),
            waitResult);

        return waitResult;
    }

    pendingOperations.store(applicationCount);
    successCount.store(0);
    event.Reset();

    //
    // Upgrade application
    //
    for (auto ix=0; ix<applicationCount; ++ix)
    {
        auto fullAppName = wformatString("{0}/{1}", appName, ix);

        FABRIC_APPLICATION_UPGRADE_DESCRIPTION upgradeDesc = { 0 };
        FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION policyDesc;

        upgradeDesc.ApplicationName = fullAppName.c_str();
        upgradeDesc.TargetApplicationTypeVersion = appVersion2.c_str();
        upgradeDesc.UpgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
        upgradeDesc.UpgradePolicyDescription = &policyDesc;

        policyDesc.RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO;
        policyDesc.ForceRestart = FALSE;
        policyDesc.UpgradeReplicaSetCheckTimeoutInSeconds = 0;
        policyDesc.Reserved = 0;

        ApplicationUpgradeDescriptionWrapper upgradeWrapper;
        auto error = upgradeWrapper.FromPublicApi(upgradeDesc);

        if (!error.IsSuccess())
        {
            TestSession::WriteError(
                TraceSource,
                "++ Failed to create ApplicationUpgradeDescriptionWrapper: error={0}",
                error);

            return false;
        }

        clientImpl->BeginUpgradeApplication(
            upgradeWrapper,
            timeout,
            [&](AsyncOperationSPtr const & operation)
            {
                auto error = clientImpl->EndUpgradeApplication(operation);

                if (error.IsSuccess())
                {
                    auto count = ++successCount;
                    if (count % 10 == 0)
                    {
                        TestSession::WriteInfo(
                            TraceSource,
                            "UpgradeApplication success: {0}",
                            count);

                        if (updateThrottles)
                        {
                            Management::ManagementConfig::GetConfig().ImageBuilderJobQueueThrottle = rand.Next(minThrottle, maxThrottle);
                            Management::ManagementConfig::GetConfig().OnUpdate(L"ClusterManager", L"ImageBuilderJobQueueThrottle");

                            Management::ManagementConfig::GetConfig().ImageBuilderUpgradeJobQueueThrottle = rand.Next(minThrottle, maxThrottle);
                            Management::ManagementConfig::GetConfig().OnUpdate(L"ClusterManager", L"ImageBuilderUpgradeJobQueueThrottle");
                        }
                    }
                }
                else
                {
                    TestSession::WriteError(
                        TraceSource,
                        "UpgradeApplication: {0}",
                        error);
                }

                if (--pendingOperations == 0)
                {
                    event.Set();
                }
            },
            operationRoot);
    }

    waitResult = event.WaitOne(timeout);

    if (!waitResult)
    {
        TestSession::WriteError(
            TraceSource,
            "++ Failed: total={0} success={1} waitResult={2}",
            applicationCount,
            successCount.load(),
            waitResult);

        return waitResult;
    }

    pendingOperations.store(applicationCount);
    successCount.store(0);
    event.Reset();

    //
    // Delete application
    //
    for (auto ix=0; ix<applicationCount; ++ix)
    {
        auto fullAppName = wformatString("{0}/{1}", appName, ix);

        NamingUri appUri;
        if (!NamingUri::TryParse(fullAppName, appUri))
        {
            TestSession::WriteError(
                TraceSource,
                "++ Failed to parse NamingUri: '{0}'",
                fullAppName);

            return false;
        }

        ServiceModel::DeleteApplicationDescription deleteApplicationDescription(appUri);
        clientImpl->BeginDeleteApplication(
            deleteApplicationDescription,
            timeout,
            [&](AsyncOperationSPtr const & operation)
            {
                auto error = clientImpl->EndDeleteApplication(operation);

                if (error.IsSuccess())
                {
                    auto count = ++successCount;
                    if (count % 10 == 0)
                    {
                        TestSession::WriteInfo(
                            TraceSource,
                            "DeleteApplication success: {0}",
                            count);

                        if (updateThrottles)
                        {
                            Management::ManagementConfig::GetConfig().ImageBuilderJobQueueThrottle = rand.Next(minThrottle, maxThrottle);
                            Management::ManagementConfig::GetConfig().OnUpdate(L"ClusterManager", L"ImageBuilderJobQueueThrottle");

                            Management::ManagementConfig::GetConfig().ImageBuilderUpgradeJobQueueThrottle = rand.Next(minThrottle, maxThrottle);
                            Management::ManagementConfig::GetConfig().OnUpdate(L"ClusterManager", L"ImageBuilderUpgradeJobQueueThrottle");
                        }
                    }
                }
                else
                {
                    TestSession::WriteError(
                        TraceSource,
                        "DeleteApplication: {0}",
                        error);
                }

                if (--pendingOperations == 0)
                {
                    event.Set();
                }
            },
            operationRoot);
    }

    waitResult = event.WaitOne(timeout);

    TestSession::WriteWarning(
        TraceSource,
        "++ Done: total={0} success={1} waitResult={2}",
        applicationCount,
        successCount.load(),
        waitResult);

    return waitResult;
}

bool TestFabricClientScenarios::ApplicationThroughput(
    FabricTestDispatcher & dispatcher,
    shared_ptr<TestFabricClient> const & client,
    StringCollection const & params)
{
    UNREFERENCED_PARAMETER(dispatcher);

    Transport::TransportConfig::GetConfig().DefaultSendQueueSizeLimit = 0;

    CommandLineParser parser(params, 0);

    wstring appNamePrefix;
    parser.TryGetString(L"appnameprefix", appNamePrefix, L"AppName");

    wstring appTypeName;
    parser.TryGetString(L"apptype", appTypeName, L"AppType");

    wstring appVersion;
    parser.TryGetString(L"appversion", appVersion, L"AppVersion");

    int applicationCount;
    parser.TryGetInt(L"appcount", applicationCount, 1000);

    int timeoutInSeconds;
    parser.TryGetInt(L"timeout", timeoutInSeconds, 60);
    TimeSpan timeout = TimeSpan::FromSeconds(timeoutInSeconds);

    StringCollection defaultServiceNames;
    ApplicationBuilder::GetDefaultServices(appTypeName, appVersion, defaultServiceNames);

    vector<wstring> connectionStrings;
    FABRICSESSION.FabricDispatcher.Federation.GetEntreeServiceAddress(connectionStrings);

    auto clientImpl = make_shared<FabricClientImpl>(move(connectionStrings));

    auto settings = clientImpl->GetSettings();
    settings.SetClientFriendlyName(L"ApplicationThroughput");
    clientImpl->SetSettings(move(settings));

    auto operationRoot = clientImpl->CreateAsyncOperationRoot();

    Common::atomic_long pendingOperations(applicationCount);
    Common::atomic_long successCount(0);
    AutoResetEvent event(false);

    Stopwatch stopwatch;
    stopwatch.Start();

    //
    // Create application
    //
    for (auto ix=0; ix<applicationCount; ++ix)
    {
        auto appName = wformatString("fabric:/{0}_{1}", appNamePrefix, ix);

        ApplicationDescriptionWrapper appDesc(
            appName,
            appTypeName,
            appVersion,
            map<wstring, wstring>());

        StringCollection fullDefaultServiceNames;
        for (auto const & svcName : defaultServiceNames)
        {
            fullDefaultServiceNames.push_back(wformatString("{0}/{1}", appName, svcName));
        }

        clientImpl->BeginCreateApplication(
            appDesc,
            timeout,
            [&, appName, fullDefaultServiceNames](AsyncOperationSPtr const & operation)
            {
                auto error = clientImpl->EndCreateApplication(operation);

                if (error.IsSuccess())
                {
                    FABRICSESSION.FabricDispatcher.ApplicationMap.AddApplication(
                        appName,
                        appTypeName, 
                        ApplicationBuilder::GetApplicationBuilderName(appTypeName, appVersion));

                    vector<PartitionedServiceDescriptor> psds;
                    client->GetPartitionedServiceDescriptors(fullDefaultServiceNames, psds);

                    for (auto & psd : psds)
                    {
                        FABRICSESSION.FabricDispatcher.ServiceMap.AddPartitionedServiceDescriptor(psd.Service.Name, move(psd));
                    }

                    auto count = ++successCount;
                    if (count % 10 == 0)
                    {
                        TestSession::WriteInfo(
                            TraceSource,
                            "CreateApplication success: {0}",
                            count);
                    }
                }
                else
                {
                    TestSession::WriteError(
                        TraceSource,
                        "CreateApplication: {0}",
                        error);
                }

                if (--pendingOperations == 0)
                {
                    event.Set();
                }

                if (error.IsSuccess())
                {
                }
            },
            operationRoot);
    }

    event.WaitOne();

    auto createDelay = stopwatch.Elapsed;

    auto createThroughput = (applicationCount / (double)createDelay.TotalMilliseconds()) * 1000;

    TestSession::WriteWarning(
        TraceSource,
        "++ Create: total={0} success={1} elapsed={2} throughput={3} app/sec",
        applicationCount,
        successCount.load(),
        createDelay,
        createThroughput);

    dispatcher.Verify();

    auto verifyCreateDelay = stopwatch.Elapsed;

    TestSession::WriteWarning(
        TraceSource,
        "++ Verify Create: delay={0}",
        verifyCreateDelay);

    pendingOperations.store(applicationCount);
    successCount.store(0);

    stopwatch.Restart();

    for (auto ix=0; ix<applicationCount; ++ix)
    {
        auto appName = wformatString("fabric:/{0}_{1}", appNamePrefix, ix);

        NamingUri appUri;
        bool success = NamingUri::TryParse(appName, appUri);
        if (!success) { return false; }

        clientImpl->BeginDeleteApplication(
            DeleteApplicationDescription(appUri),
            timeout,
            [&, appName](AsyncOperationSPtr const & operation)
            {
                auto error = clientImpl->EndDeleteApplication(operation);

                if (error.IsSuccess())
                {
                    FABRICSESSION.FabricDispatcher.ApplicationMap.DeleteApplication(appName);
                    FABRICSESSION.FabricDispatcher.ServiceMap.MarkAppAsDeleted(appName);

                    auto count = ++successCount;
                    if (count % 10 == 0)
                    {
                        TestSession::WriteInfo(
                            TraceSource,
                            "DeleteApplication success: {0}",
                            count);
                    }
                }
                else
                {
                    TestSession::WriteError(
                        TraceSource,
                        "DeleteApplication: {0}",
                        error);
                }

                if (--pendingOperations == 0)
                {
                    event.Set();
                }
            },
            operationRoot);
    }

    event.WaitOne();

    auto deleteDelay = stopwatch.Elapsed;

    auto deleteThroughput = (applicationCount / (double)deleteDelay.TotalMilliseconds()) * 1000;

    TestSession::WriteWarning(
        TraceSource,
        "++ Delete: total={0} success={1} elapsed={2} throughput={3} app/sec",
        applicationCount,
        successCount.load(),
        deleteDelay,
        deleteThroughput);

    dispatcher.Verify();

    auto verifyDeleteDelay = stopwatch.Elapsed;

    TestSession::WriteWarning(
        TraceSource,
        "++ Verify Delete: delay={0}",
        verifyDeleteDelay);

    return true;
}

void TestFabricClientScenarios::ProcessNotification(
    int expectedNotificationCount,
    Common::atomic_long & notificationSuccessCount,
    Common::atomic_long & notificationErrorCount,
    LONGLONG handlerId,
    IResolvedServicePartitionResultPtr const & result,
    ErrorCode const & error,
    ManualResetEvent & notificationEvent,
    bool printDetails)
{
    LONG count = 0;

    if (error.IsSuccess())
    {
        count = ++notificationSuccessCount;

        if (printDetails)
        {
            auto const & rsp = dynamic_cast<ResolvedServicePartitionResult*>(result.get())->ResolvedServicePartition;

            TestSession::WriteInfo(
                TraceSource, 
                "Notification({0}, {1}/{2}): {3}",
                handlerId,
                count,
                expectedNotificationCount,
                rsp);
        }
    }
    else
    {
        count = ++notificationErrorCount;

        TestSession::WriteError(
            TraceSource,
            "Notification({0}, {1}): {2}",
            handlerId,
            count,
            error);
    }

    if (expectedNotificationCount > 0 && count == expectedNotificationCount)
    {
        notificationEvent.Set();
    }
}

void TestFabricClientScenarios::ProcessResolveResult(
    NamingUri const & serviceName,
    ErrorCode const & error,
    IResolvedServicePartitionResultPtr const & result,
    Common::atomic_long & pendingCount,
    Common::atomic_long & failuresCount,
    ManualResetEvent & completedEvent,
    bool printDetails)
{
    if (error.IsSuccess())
    {
        if (printDetails)
        {
            auto const & rsp = dynamic_cast<ResolvedServicePartitionResult*>(result.get())->ResolvedServicePartition;

            TestSession::WriteWarning(
                TraceSource, 
                "Resolved: {0}",
                rsp);
        }
    }
    else
    {
        TestSession::WriteError(
            TraceSource, 
            "Resolving {0} failed: {1}",
            serviceName,
            error);

        ++failuresCount;
    }

    if (--pendingCount == 0)
    {
        completedEvent.Set();
    }
}

class ClientConnectionEventHandler
    : public ComponentRoot
    , public IClientConnectionEventHandler
{
public:

    ErrorCode OnConnected(GatewayDescription const & gateway)
    {
        TestSession::WriteInfo(
            TraceSource, 
            "test client connected to {0}",
            gateway);

        return ErrorCodeValue::Success;
    }

    ErrorCode OnDisconnected(GatewayDescription const & gateway, ErrorCode const & error)
    {
        TestSession::WriteInfo(
            TraceSource, 
            "test client disconnected from {0}: error={1}",
            gateway,
            error);

        return ErrorCodeValue::Success;
    }
};

ServiceDescription TestFabricClientScenarios::CreateServiceDescription(
    wstring const & name,
    int partitionCount)
{
    return ServiceDescription(
        name,
        0,
        0,
        partitionCount, 
        1,  // replica count 
        1,  // min write
        false,  // is stateful
        false,  // has persisted
        TimeSpan::Zero, // replica restart wait duration
        TimeSpan::Zero, // quorum loss wait duration
        TimeSpan::Zero, // standby replica keep duration
        ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(), L"MockPSD"), 
        vector<Reliability::ServiceCorrelationDescription>(),
        L"",    // placement constraints
        0, // scaleout count
        vector<Reliability::ServiceLoadMetricDescription>(), 
        0, // default move cost
        vector<byte>()); // initialization data
}
