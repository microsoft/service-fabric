// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <psapi.h>

using namespace std;
using namespace FabricTest;
using namespace Reliability;
using namespace Federation;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ServiceModel;

StringLiteral const TraceSource("FabricTest.Random");

GlobalWString RandomFabricTestSession::CalculatorServicePrefix = make_global<wstring>(L"fabric:/calculatorservice");
GlobalWString RandomFabricTestSession::TestStoreServicePrefix = make_global<wstring>(L"fabric:/teststoreservice");
GlobalWString RandomFabricTestSession::TestPersistedStoreServicePrefix = make_global<wstring>(L"fabric:/testpersistedstoreservice");
GlobalWString RandomFabricTestSession::TXRServicePrefix = make_global<wstring>(L"fabric:/txrservice");
GlobalWString RandomFabricTestSession::AppNamePrefix = make_global<wstring>(L"fabric:/testapp");

void RandomFabricTestSession::CreateSingleton(int maxIterations, int timeout, wstring const& label, bool autoMode, wstring const& serverListenAddress)
{
    shared_ptr<RandomFabricTestSession> randomFabricTestSession = shared_ptr<RandomFabricTestSession>(new RandomFabricTestSession(maxIterations, timeout, label, autoMode, serverListenAddress));
    singleton_ = new shared_ptr<FabricTestSession>(static_pointer_cast<FabricTestSession>(randomFabricTestSession));
}

RandomFabricTestSession::RandomFabricTestSession(int maxIterations, int timeout, std::wstring const& label, bool autoMode, wstring const& serverListenAddress)
    : config_(),
    nodeHistory_(),
    allNodes_(),
    nodeRGCapacities_(L""),
    liveSeedNodes_(),
    createdServices_(),
    deletedServices_(),
    maxIterations_(maxIterations),
    iterations_(0),
    timeout_(timeout <= 0 ? TimeSpan::MaxValue : TimeSpan::FromSeconds(timeout)),
    random_(),
    initialized_(false),
    highMemoryStartTime_(StopwatchTime::Zero),
    seedNodes_(),
    applicationHelper_(),
    FabricTestSession(label, autoMode, serverListenAddress)
{
    ASSERT_IF(maxIterations_ <= 0, "max iterations must be greater than zero {0}", maxIterations_);

    wstring paramsFileName = label + L".params";
    wstring pathToParamsFile = Path::Combine(Environment::GetExecutablePath(), paramsFileName);
    if (!File::Exists(pathToParamsFile))
    {
        Common::Assert::TestAssert("FabricTest Random Type {0} is not supported because params fie {1} is missing.", label, pathToParamsFile );
    }

    shared_ptr<ConfigStore> fileConfigPtr = make_shared<FileConfigStore>(pathToParamsFile, false);
    config_ = make_shared<RandomSessionConfig>(fileConfigPtr);
    applicationHelper_.SetConfig(config_);
}

wstring RandomFabricTestSession::GetInput()
{
    wstring text;
    StringWriter w(text);
    TimeSpan elapsed = TimeSpan::FromTicks(watch_.ElapsedTicks);
    int timePerIteration = iterations_ == 0 ? -1 : static_cast<int>(elapsed.TotalMilliseconds() / iterations_);

    if (maxIterations_ != std::numeric_limits<int>::max())
    {
        w.Write("Iterations : {0}, Time Elapsed: {1}, Time Per Iteration: {2}, Iterations Left: {3}", iterations_, elapsed, TimeSpan::FromMilliseconds(timePerIteration), maxIterations_ - iterations_);
    }
    else
    {
        w.Write("Iterations : {0}, Time Elapsed: {1}, Time Per Iteration: {2}", iterations_, elapsed, TimeSpan::FromMilliseconds(timePerIteration));
    }

    TraceConsoleSink::Write(0x0b, text);
    nodeHistory_.clear();

    if(!initialized_)
    {
        AssignNodeIds();
        SetupConfiguration();
        SetupRing();
        SetupUnreliableTransport();
        SetupWatchDogs();
        SetupForServicesAndApplications();
        EnsureAllServicesAndApplicationsAreCreated();
        initialized_ = true;
        watch_.Start();
    }
    else
    {
        CheckMemory();

        bool rebuild = TryRebuild();
        if (!rebuild)
        {
            InduceSlowCopyInPersistedTestStoreService();

            bool upgrade = TryUpgrade();
            if (!upgrade)
            {
                if (config_->DeleteServiceInterval != 0 && iterations_ % config_->DeleteServiceInterval == 0)
                {
                    CreateDeleteServices();
                }
                else if (config_->ActivateDeactivateNodeInterval != 0 && iterations_ % config_->ActivateDeactivateNodeInterval == 0)
                {
                    ActivateDeactivateNodes();
                }
                else if (config_->UpdateServiceInterval != 0 && iterations_ % config_->UpdateServiceInterval == 0)
                {
                    UpdateService();
                }
                else
                {
                    ExecuteDynamismStage();
                }

                AddInput(FabricTestCommands::VerifyCommand);

                if (config_->NodeRgCapacityProbability > 0)
                {
                    AddInput(FabricTestCommands::VerifyPLBAndLRMSync);
                }

                AddInput(L"addbehavior BlockReportHealth * * ReportHealth 1.0 Max 0 10");
                AddInput(L"addbehavior AllowQueryRequest * * QueryRequest 1.0 0 0 10");
                AddInput(L"addbehavior AllowOperationSuccessAction * * OperationSuccessAction 1.0 0 0 10");

                if (config_->VerifyQueryCount > 0)
                {
                    AddInput(wformatString("{0} {1}", FabricTestCommands::ExecuteQueryCommand, config_->VerifyQueryCount));
                }

                if (FabricTestSessionConfig::GetConfig().HealthFullVerificationEnabled)
                {
                    AddInput(FabricTestCommands::QueryHealthCommand);
                }

                AddInput(L"removebehavior AllowOperationSuccessAction");
                AddInput(L"removebehavior AllowQueryRequest");
                AddInput(L"removebehavior BlockReportHealth");

                if (FabricTestSessionConfig::GetConfig().HealthVerificationEnabled)
                {
                    AddInput(FabricTestCommands::WatchDogCommand + L" resume");
                }
            }
        }
    }

    periodicTracer_.OnIteration(iterations_, *this);

    ++iterations_;
    if(iterations_ >= maxIterations_ || timeout_.IsExpired)
    {
        // Disable api faults so that the clients can stop gracefully in case they were waiting for any pending operation like put/get and the 
        // faults continuously fail API's which can lead to puts to get stuck for long durations due to builds not completing etc.
        // The bug that caught this issue was the below.

        // RD: RDBug 12260446 : Unreliable Failure : Client.StopTest failed due to all clients not finishing - Due to builds failing in replicator caused by fault injection during open async

        ApiFaultHelper::Get().SetRandomApiFaultsEnabled(false);

        AddInput(L"#End of test.  Exiting...");
        AddInput(FabricTestCommands::StopClientCommand);
        AddInput(TestSession::QuitCommand);
    }

    return wstring();
}

void RandomFabricTestSession::InduceSlowCopyInPersistedTestStoreService()
{
    if (iterations_ % config_->TestPersistedStoreServiceDelayCopyPumpIterationInterval == 0)
    {
        for (int i = 0; i < config_->TestPersistedStoreServiceWithSmallReplicationQueueCount; i++)
        {
            wstring serviceName = wformatString("{0}{1}", TestPersistedStoreServicePrefix, i);
            if (ContainsName(createdServices_, serviceName))
            {
                wstring injectFailure;
                StringWriter content(injectFailure);
                content.Write(FabricTestCommands::InjectFailureCommand);
                content.Write(FabricTestDispatcher::ParamDelimiter);
                content.Write("*");
                content.Write(FabricTestDispatcher::ParamDelimiter);
                content.Write(serviceName);
                content.Write(FabricTestDispatcher::ParamDelimiter);
                content.Write("provider.getcopystate.delay");
                content.Write(FabricTestDispatcher::ParamDelimiter);
                content.Write(config_->TestPersistedStoreServiceDelayCopyFaultPeriod.TotalSeconds());

                AddInput(injectFailure);
            }
        }
    }
}

void RandomFabricTestSession::AssignNodeIds()
{
    wstring udName;
    for(uint64 i = 0; i < (static_cast<uint64>(config_->MaxNodes) * 2) + config_->RandomVoteCount; ++i)
    {
        LargeInteger id(0, (i + 1) * 16);
        if(i % config_->UpgradeDomainSize == 0 && config_->UpgradeInterval > 0)
        {
            udName.clear();
            StringWriter writer(udName);
            writer.Write("UD");
            for (uint64 j = i; j < i + config_->UpgradeDomainSize && j < (static_cast<uint64>(config_->MaxNodes) * 2) + config_->RandomVoteCount;++j)
            {
                LargeInteger udId(0, (j + 1) * 16);
                writer.Write("_{0}", udId);
            }

            liveNodesPerUpgradeDomain_.insert(make_pair(udName, 0));
        }

        if(allNodes_.find(id) == allNodes_.end())
        {
            allNodes_.insert(make_pair(id, udName));
        }
    }

    // assign seed nodes
    wstring udString;
    while(static_cast<int>(seedNodes_.size()) != config_->RandomVoteCount)
    {
        int r = random_.Next();
        r %= static_cast<int>(allNodes_.size());
        auto iter = allNodes_.begin();
        advance(iter, r);
        LargeInteger id = iter->first;
        wstring udName1 = iter->second;
        if(!ContainsNode(seedNodes_, id) &&
            /*Next check is to ensure every seed node goes to a different domain*/(udName1.empty() || !StringUtility::Contains<wstring>(udString, udName1)))
        {
            seedNodes_.push_back(id);
            udString.append(udName1);
        }
    }
}

void RandomFabricTestSession::SetupConfiguration()
{
    FabricTestCommonConfig::GetConfig().UseRandomReplicatorSettings = true;

    // Ensure the verify timeout is greater than the delay copy pump fault period. Otherwise, we could run into false positive verify failures
    TestSession::FailTestIf(
        config_->TestPersistedStoreServiceDelayCopyFaultPeriod >= config_->VerifyTimeout,
        "TestPersistedStoreServiceDelayCopyFaultPeriod setting must be lesser than the verify timeout");

    //ClearTicket and Store
    AddInput(FabricTestCommands::CleanTestCommand);

    //Set OpenTimeout
    FabricTestSessionConfig::GetConfig().OpenTimeout = config_->OpenTimeout;

    //Calculate minimum quorum
    quorum_ = static_cast<int>(seedNodes_.size()) + 1;

    //Overwrite verification timeout interval
    FabricTestSessionConfig::GetConfig().VerifyTimeout = config_->VerifyTimeout;
    FabricTestSessionConfig::GetConfig().RatioOfVerifyDurationWhenFaultsAreAllowed = config_->RatioOfVerifyDurationWhenFaultsAreAllowed;

    //It is possible client operations do not succeed until the system is stable after any change (dynamism, load balancing etc)
    //hence we set the client timeout to be be greater than the verify timeout
    FabricTestSessionConfig::GetConfig().StoreClientTimeout = TimeSpan::FromSeconds(static_cast<double>(3 * config_->VerifyTimeout.TotalSeconds()));

    //Overwrite naming operation timeout interval
    FabricTestSessionConfig::GetConfig().NamingOperationTimeout = config_->NamingOperationTimeout;

    //Set the naming retry count
    FabricTestSessionConfig::GetConfig().NamingOperationRetryTimeout = config_->VerifyTimeout + /*Adding buffer time*/TimeSpan::FromSeconds(15);
    FabricTestSessionConfig::GetConfig().NamingResolveRetryTimeout = config_->VerifyTimeout + /*Adding buffer time*/TimeSpan::FromSeconds(15);

    // set close timeout for random tests to high value
    FabricTestSessionConfig::GetConfig().HostingOpenCloseTimeout = TimeSpan::FromSeconds(600);

    // Overwrite CM max internal retry timeout.
    // The timeout for inner CM operations (eg. CM to Naming requests) is (originalOperationTimeout + MinOperationTimeout * timeoutCount),
    // with minTimeout*timeoutCount capped at MaxTimeoutRetryBuffer.
    // Total timeout is capped at MaxOperationTimeout.
    // Default value for MaxTimeoutRetryBuffer is 5 minutes, which can be too much when messages are dropped and CM inner operations need to wait full timeout period
    // to start a new retry.
    Management::ManagementConfig::GetConfig().MaxTimeoutRetryBuffer = config_->CMMaxTimeoutRetryBuffer;

    map<wstring, double, Common::IsLessCaseInsensitiveComparer<std::wstring>> randomFaults;
    Common::StringMap keyValues;
    config_->GetKeyValues(L"RandomFaults", keyValues);
    for(auto iter = keyValues.begin(); iter != keyValues.end(); ++iter)
    {
        randomFaults[iter->first] = Double_Parse(iter->second);
    }

    ApiFaultHelper::Get().SetRandomFaults(randomFaults);

    ApiFaultHelper::Get().SetMaxApiDelayInterval(config_->MaxApiDelayInterval);
    ApiFaultHelper::Get().SetMinApiDelayInterval(config_->MinApiDelayInterval);

    if(config_->FullRebuildInterval > 0)
    {
        Reliability::FailoverConfig::GetConfig().TargetReplicaSetSize = config_->MaxDynamism;
        Reliability::FailoverConfig::GetConfig().MinReplicaSetSize = 1;
        Reliability::FailoverConfig::GetConfig().PlacementConstraints = L"(NodeType!=Seed)";
    }

    if(config_->UpdateServiceInterval > 0)
    {
        TestSession::FailTestIfNot(config_->MaxReplicas > 1, "MaxReplicas should atleast be 2 to do update service");
    }

    auto enablePreWriteStatusCatchup = Random().NextDouble() < config_->PreWriteStatusCatchupEnabledProbability;
    auto data = wformatString("PreWriteStatusCatchupEnabled = {0}", enablePreWriteStatusCatchup);
    TraceConsoleSink::Write(LogLevel::Info, data);
    periodicTracer_.AddString(data);
    Reliability::FailoverConfig::GetConfig().IsPreWriteStatusRevokeCatchupEnabled = enablePreWriteStatusCatchup;

    // Generate node capacities for the metrics governed by RG
    GenerateNodeRGCapacities();

    applicationHelper_.UploadApplicationTypes();
    // Disable image caching to reduce disk usage in upgrade random
    Management::ManagementConfig::GetConfig().ImageCachingEnabled = false;

    // Ensure all max replica counts are less than number of upgrade domains
    size_t numberOfUpgradeDomains = liveNodesPerUpgradeDomain_.size();
    if(numberOfUpgradeDomains > 0)
    {
        TestSession::FailTestIf(
            static_cast<size_t>(Reliability::FailoverConfig::GetConfig().TargetReplicaSetSize) > numberOfUpgradeDomains ||
            static_cast<size_t>(Naming::NamingConfig::GetConfig().TargetReplicaSetSize) > numberOfUpgradeDomains ||
            static_cast<size_t>(Management::ManagementConfig::GetConfig().TargetReplicaSetSize) > numberOfUpgradeDomains ||
            static_cast<size_t>(config_->MaxReplicas) > numberOfUpgradeDomains ||
            static_cast<size_t>(config_->AppMaxReplicaCount) > numberOfUpgradeDomains,
            "Replica count setting greater than number of upgrade domains. Check configuration");

        TestSession::FailTestIf(
            static_cast<size_t>(config_->MinNodes) < numberOfUpgradeDomains,
            "MinNodes setting less than number of upgrade domains. Check configuration");
    }

    StringCollection properties;
    StringUtility::Split<wstring>(config_->Properties, properties, L"#");
    for(auto iter = properties.begin(); iter != properties.end(); ++iter)
    {
        StringUtility::TrimWhitespaces(*iter);
        AddInput(FabricTestCommands::SetPropertyCommand + L" " + *iter);
    }

    // IsDeactivationInfoEnabled is used some of the time to get coverage of all scenarios
    bool deactivationInfoValue = (random_.Next() % 2 == 0);
    AddInput(wformatString("set IsDeactivationInfoEnabled {0}", deactivationInfoValue));

    // Enable CM service for all random runs
    AddInput(L"cmservice 7 1");

    // Enable Async Node close for all random runs
    FabricTestSessionConfig::GetConfig().AsyncNodeCloseEnabled = true;

    // Set client put data configuration
    FabricTestSessionConfig::GetConfig().MinClientPutDataSizeInBytes = config_->MinClientPutDataSizeInBytes;
    FabricTestSessionConfig::GetConfig().MaxClientPutDataSizeInBytes = config_->MaxClientPutDataSizeInBytes;
}

void RandomFabricTestSession::SetupRing()
{
    size_t nodesAdded = 0;
    wstring command;
    StringWriter writer(command);
    writer.Write("votes");
    for (LargeInteger seed : seedNodes_)
    {
        writer.Write("{0}{1}", FabricTestDispatcher::ParamDelimiter,seed.ToString());
    }

    AddInput(command);

    for (LargeInteger seed : seedNodes_)
    {
        ++nodesAdded;
        AddNode(seed);
    }

    liveSeedNodes_ = seedNodes_;

    for(int i = 0; i < Reliability::FailoverConfig::GetConfig().TargetReplicaSetSize; i++)
    {
        ++nodesAdded;
        AddNode();
    }

    while(!IsOneNodeUpPerUpgradeDomain())
    {
        ++nodesAdded;
        AddNode();
    }

    while(nodesAdded < static_cast<size_t>(config_->MinNodes))
    {
        ++nodesAdded;
        AddNode();
    }

    AddInput(FabricTestCommands::VerifyCommand);
}

void RandomFabricTestSession::SetupUnreliableTransport()
{
    StringCollection utBehaviors;
    StringUtility::Split<wstring>(config_->UnreliableTransportBehaviors, utBehaviors, L",");
    for (auto iter = utBehaviors.begin(); iter != utBehaviors.end(); ++iter)
    {
        AddInput(L"addbehavior " + *iter);
    }
}

void RandomFabricTestSession::SetupForServicesAndApplications()
{
    //Provision all types
    applicationHelper_.ProvisionApplicationTypes();

    //Start naming client
    if (config_->NamingThreadCount > 0 && config_->NameCount > 0 && config_->PropertyPerNameCount)
    {
        wstring params;
        StringWriter writer(params);
        writer.Write("{0}{1}{2}{3}{4}{5}{6}",
            config_->NamingThreadCount,
            FabricTestDispatcher::ParamDelimiter,
            config_->NameCount,
            FabricTestDispatcher::ParamDelimiter,
            config_->PropertyPerNameCount,
            FabricTestDispatcher::ParamDelimiter,
            config_->NamingOperationInterval.TotalMilliseconds());

        AddInput(FabricTestCommands::StartTestFabricClientCommand + FabricTestDispatcher::ParamDelimiter
            + params);
    }

    //Start test client
    if (config_->ClientThreadCount > 0)
    {
        wstring params;
        StringWriter writer(params);
        writer.Write("{0}{1}{2}{3}{4}",
            config_->ClientThreadCount,
            FabricTestDispatcher::ParamDelimiter,
            config_->ClientPutRatio,
            FabricTestDispatcher::ParamDelimiter,
            config_->ClientOperationInterval.TotalMilliseconds());

        AddInput(FabricTestCommands::StartClientCommand + FabricTestDispatcher::ParamDelimiter
            + params);
    }
}

void RandomFabricTestSession::EnsureAllServicesAndApplicationsAreCreated()
{
    //Create Applications
    for(int i = 0; i < config_->ApplicationCount; i++)
    {
        //Always create at version 1
        wstring appName = wformatString("{0}{1}", AppNamePrefix, Common::Guid::NewGuid().ToString());
        if(createdapps_.find(appName) == createdapps_.end())
        {
            wstring createAppParam = wformatString("{0} {1} {2}", appName, RandomTestApplicationHelper::AppTypeName, applicationHelper_.GetVersionString(0));
            AddInput(FabricTestCommands::CreateApplicationCommand + FabricTestDispatcher::ParamDelimiter
                + createAppParam);

            if (config_->ClientThreadCount > 0)
            {
                vector<wstring> statefulServiceNames;
                applicationHelper_.GetAllStatefulServiceNames(0/*App starts at version 1*/, statefulServiceNames);
                for (auto iter = statefulServiceNames.begin(); iter != statefulServiceNames.end(); ++iter)
                {
                    wstring serviceName = appName + L"/" + *iter;
                    AddInput(FabricTestCommands::StartClientCommand + FabricTestDispatcher::ParamDelimiter + serviceName);
                }
            }

            AddInput(FabricTestCommands::VerifyCommand);
            createdapps_.insert(make_pair(appName, 0));
        }
    }

    //Parse calculator services from config. These services are not deleted
    StringCollection calculatorServices;
    StringUtility::Split<wstring>(config_->CalculatorServices, calculatorServices, L",");

    for (wstring const & service : calculatorServices)
    {
        wstring name;
        wstring param;

        wstring temp = service;
        StringUtility::TrimSpaces(temp);
        size_t i = temp.find(L' ', 0);
        if (i != wstring::npos)
        {
            name = temp.substr(0, i);
            param = temp.substr(i + 1);
        }
        else
        {
            name = temp;
            param = L"1 1";
        }

        name = L"fabric:/" + name;

        if(!ContainsName(createdServices_, name))
        {
            if (!config_->UnreliableTransportBehaviors.empty())
            {
                param = param + L" errors=Success,UserServiceAlreadyExists";
            }

            AddInput(FabricTestCommands::CreateServiceCommand + FabricTestDispatcher::ParamDelimiter +
                name + FabricTestDispatcher::ParamDelimiter +
                CalculatorService::DefaultServiceType + FabricTestDispatcher::ParamDelimiter +
                L"n" + FabricTestDispatcher::ParamDelimiter +
                param);
            AddInput(FabricTestCommands::VerifyCommand);
            createdServices_.push_back(name);
        }
    }

    //Create calculator services based on config params
    for(int i = 0; i < config_->CalculatorServiceCount; i++)
    {
        wstring serviceName = wformatString("{0}{1}", CalculatorServicePrefix , StringUtility::ToWString(i));
        if(!ContainsName(createdServices_, serviceName))
        {
            CreateService(serviceName, CalculatorService::DefaultServiceType);
        }
    }

    //Parse test store services from config. These services are not deleted
    StringCollection testStoreServices;
    StringUtility::Split<wstring>(config_->TestStoreServices, testStoreServices, L",");

    for (wstring const & service : testStoreServices)
    {
        wstring name;
        wstring param;

        wstring temp = service;
        StringUtility::TrimSpaces(temp);
        size_t i = temp.find(L' ', 0);
        if (i != wstring::npos)
        {
            name = temp.substr(0, i);
            param = temp.substr(i + 1);
        }
        else
        {
            name = temp;
            param = L"1 1";
        }

        name = L"fabric:/" + name;

        if(!ContainsName(createdServices_, name))
        {
            if (!config_->UnreliableTransportBehaviors.empty())
            {
                param = param + L" errors=Success,UserServiceAlreadyExists";
            }

            AddInput(FabricTestCommands::CreateServiceCommand + FabricTestDispatcher::ParamDelimiter +
                name + FabricTestDispatcher::ParamDelimiter +
                TestStoreService::DefaultServiceType + FabricTestDispatcher::ParamDelimiter +
                L"y" + FabricTestDispatcher::ParamDelimiter +
                param);

            if (config_->ClientThreadCount > 0)
            {
                AddInput(FabricTestCommands::StartClientCommand + FabricTestDispatcher::ParamDelimiter + name);
            }

            AddInput(FabricTestCommands::VerifyCommand);
            createdServices_.push_back(name);
        }
    }

    //Create test persisted store services based on config params. These services are not deleted
    for(int i = 0; i < config_->TestStoreServiceCount ; i++)
    {
        wstring serviceName = wformatString("{0}{1}", TestStoreServicePrefix , StringUtility::ToWString(i));
        if(!ContainsName(createdServices_, serviceName))
        {
            CreateService(serviceName, TestStoreService::DefaultServiceType);
        }
    }

    //Parse persisted store services from config. These services are not deleted
    StringCollection persistedTestStoreServices;
    StringUtility::Split<wstring>(config_->TestPersistedStoreServices, persistedTestStoreServices, L",");

    for (wstring const & service : persistedTestStoreServices)
    {
        wstring name;
        wstring param;

        wstring temp = service;
        StringUtility::TrimSpaces(temp);
        size_t i = temp.find(L' ', 0);
        if (i != wstring::npos)
        {
            name = temp.substr(0, i);
            param = temp.substr(i + 1);
        }
        else
        {
            name = temp;
            param = L"1 1";
        }

        name = L"fabric:/" + name;

        if(!ContainsName(createdServices_, name))
        {
            if (!config_->UnreliableTransportBehaviors.empty())
            {
                param = param + L" errors=Success,UserServiceAlreadyExists";
            }

            AddInput(FabricTestCommands::CreateServiceCommand + FabricTestDispatcher::ParamDelimiter +
                name + FabricTestDispatcher::ParamDelimiter +
                TestPersistedStoreService::DefaultServiceType + FabricTestDispatcher::ParamDelimiter +
                L"y" + FabricTestDispatcher::ParamDelimiter +
                param + FabricTestDispatcher::ParamDelimiter +
                L"persist");

            if (config_->ClientThreadCount > 0)
            {
                AddInput(FabricTestCommands::StartClientCommand + FabricTestDispatcher::ParamDelimiter + name);
            }

            AddInput(FabricTestCommands::VerifyCommand);
            createdServices_.push_back(name);
        }
    }

    //Create test store services based on config params
    for(int i = 0; i < config_->TestPersistedStoreServiceCount + config_->TestPersistedStoreServiceWithSmallReplicationQueueCount; i++)
    {
        wstring serviceName = wformatString("{0}{1}", TestPersistedStoreServicePrefix , StringUtility::ToWString(i));
        if(!ContainsName(createdServices_, serviceName))
        {
            if ( i < config_->TestPersistedStoreServiceWithSmallReplicationQueueCount)
            {
                CreateServiceWithSmallReplicationQueue(serviceName, TestPersistedStoreService::DefaultServiceType);
            }
            else
            {
                CreateService(serviceName, TestPersistedStoreService::DefaultServiceType);
            }
        }
    }

    //Create TXR Service
    for(int i = 0; i < config_->TXRServiceCount; i++)
    {
        wstring serviceName = wformatString("{0}{1}", TXRServicePrefix , StringUtility::ToWString(i));
        if(!ContainsName(createdServices_, serviceName))
        {
            CreateService(serviceName, TXRService::DefaultServiceType);
        }
    }
}

void RandomFabricTestSession::CreateServiceWithSmallReplicationQueue(wstring serviceName, wstring serviceType)
{
    wstring initData;
    StringWriter writer(initData);

    writer.Write("RE_InitialPrimaryReplicationQueueSize:2;RE_InitialSecondaryReplicationQueueSize:2;");
    writer.Write("RE_MaxPrimaryReplicationQueueSize:{0};", config_->TestPersistedStoreServiceSmallReplicationQueueSize);
    writer.Write("RE_MaxSecondaryReplicationQueueSize:{0};", config_->TestPersistedStoreServiceSmallReplicationQueueSize);

    CreateServiceInner(serviceName, serviceType, initData, config_->TestPersistedStoreServiceReplicaRestartWaitDuration);
}

void RandomFabricTestSession::CreateService(wstring serviceName, wstring serviceType)
{
    CreateServiceInner(serviceName, serviceType, FabricTestDispatcher::ParamDelimiter, Common::TimeSpan::MinValue);
}

void RandomFabricTestSession::CreateServiceInner(wstring serviceName, wstring serviceType, wstring initializationData, Common::TimeSpan replicaRestartWaitDuration)
{
    int minPartitionCount = 1;
    int partitions = random_.Next(config_->MaxPartitions < minPartitionCount ? config_->MaxPartitions : minPartitionCount, config_->MaxPartitions + 1);
    int replicas = random_.Next(1, config_->MaxReplicas + 1);
    int minReplicaSetSize = random_.Next(1, replicas + 1);
    bool isStateful = false;
    bool hasPersistedState = false;

    if(StringUtility::StartsWith<wstring>(serviceName, TestStoreServicePrefix))
    {
        isStateful = true;
        hasPersistedState = false;
    }
    else if (StringUtility::StartsWith<wstring>(serviceName, TestPersistedStoreServicePrefix))
    {
        isStateful = true;
        hasPersistedState = true;
    }
    else if (StringUtility::StartsWith<wstring>(serviceName, CalculatorServicePrefix))
    {
        isStateful = false;
        hasPersistedState = false;
    }
    else if (StringUtility::StartsWith<wstring>(serviceName, TXRServicePrefix))
    {
        isStateful = true;
        hasPersistedState = true;
    }
    else
    {
        TestSession::FailTest("Unknown name {0}", serviceName);
    }

    wstring errors;
    if (!config_->UnreliableTransportBehaviors.empty())
    {
        errors = L"errors=Success,UserServiceAlreadyExists";
    }

    wstring command = FabricTestCommands::CreateServiceCommand + FabricTestDispatcher::ParamDelimiter
        + serviceName + FabricTestDispatcher::ParamDelimiter
        + serviceType + FabricTestDispatcher::ParamDelimiter
        + StringUtility::ToWString(isStateful) + FabricTestDispatcher::ParamDelimiter
        + StringUtility::ToWString(partitions) + FabricTestDispatcher::ParamDelimiter
        + StringUtility::ToWString(replicas) + FabricTestDispatcher::ParamDelimiter
        + L"persist=" + StringUtility::ToWString(hasPersistedState) + FabricTestDispatcher::ParamDelimiter;

    if (isStateful)
    {
        command += L"minreplicasetsize=" + StringUtility::ToWString(minReplicaSetSize) + FabricTestDispatcher::ParamDelimiter;
    }

    if (replicaRestartWaitDuration != TimeSpan::MinValue)
    {
        command += L"replicarestartwaitduration=" + StringUtility::ToWString(replicaRestartWaitDuration.TotalSeconds()) + FabricTestDispatcher::ParamDelimiter;
    }

    command += errors + FabricTestDispatcher::ParamDelimiter;

    // Add the initialization data to the end
    if (initializationData != FabricTestDispatcher::ParamDelimiter)
    {
        command += L"initdata=" + initializationData + FabricTestDispatcher::ParamDelimiter;
    }

    AddInput(command);

    if (isStateful && config_->ClientThreadCount > 0)
    {
        AddInput(FabricTestCommands::StartClientCommand + FabricTestDispatcher::ParamDelimiter + serviceName);
    }

    if (StringUtility::StartsWith<wstring>(serviceName, TXRServicePrefix))
    {
        wstring startTimestampValidationCmd = wformatString(
            "{0}{1}{2}{3}{4}",
            FabricTestCommands::EnableLogTruncationTimestampValidation,
            FabricTestDispatcher::ParamDelimiter,
            serviceName,
            FabricTestDispatcher::ParamDelimiter,
            L"start");

        AddInput(startTimestampValidationCmd);
    }

    AddInput(FabricTestCommands::VerifyCommand);
    createdServices_.push_back(serviceName);
}

void RandomFabricTestSession::CheckMemory()
{
    //todo, memory check should be consistent with Transport/Throttle.cpp
    //on Linux, ProcessInfo::DataSize() is checked for throttling
    //on Windows, PagefileUsage is checked for throttling
    size_t MaxAllowedPrivateBytes = static_cast<size_t>(config_->MaxAllowedMemoryInMB) * 1024 * 1024;

#ifdef PLATFORM_UNIX
    auto pmi = ProcessInfo::GetSingleton();
    string summary;
    auto rss = pmi->GetRssAndSummary(summary);

    if(MaxAllowedPrivateBytes != 0 /*Memory check disabled if 0*/ && rss > MaxAllowedPrivateBytes)
    {
        TestSession::WriteError(
            "FabricTest.Memory",
            "FabricTest session: workingSet/rss exceeds limit {0}, {1}", 
            MaxAllowedPrivateBytes,
            summary);

        if (highMemoryStartTime_ == StopwatchTime::Zero)
        {
            highMemoryStartTime_ = Stopwatch::Now();
        }

        bool result = highMemoryStartTime_ + config_->MaxAllowedMemoryTimeout > Stopwatch::Now();
        ASSERT_IF(!result && config_->AssertOnMemoryCheckFailure, "Memory check failed");
    }

    TestSession::WriteInfo("FabricTest.Memory", "FabricTest session: {0}", summary);
    highMemoryStartTime_ = StopwatchTime::Zero;

#else
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS pmc;

    hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        GetCurrentProcessId());
    TestSession::FailTestIf(NULL == hProcess, "OpenProcess failed");
    TestSession::FailTestIf(FALSE == GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)), "GetProcessMemoryInfo failed");

    CloseHandle( hProcess );

    if (MaxAllowedPrivateBytes != 0 /*Memory check disabled if 0*/ && pmc.WorkingSetSize > MaxAllowedPrivateBytes)
    {
        TestSession::WriteError(
            "FabricTest.Memory",
            "Fabric session WorkingSet {0} more than {1}. PeakWorkingSetSize {2} MB, PageFileUsage {3} MB, PeakPagefileUsage {4} MB",
            pmc.WorkingSetSize / (1024 * 1024),
            config_->MaxAllowedMemoryInMB,
            pmc.PeakWorkingSetSize / (1024 * 1024),
            pmc.PagefileUsage / (1024 * 1024),
            pmc.PeakPagefileUsage / (1024 * 1024));

        if (highMemoryStartTime_ == StopwatchTime::Zero)
        {
            highMemoryStartTime_ = Stopwatch::Now();
        }

        bool result = highMemoryStartTime_ + config_->MaxAllowedMemoryTimeout > Stopwatch::Now();
        ASSERT_IF(!result && config_->AssertOnMemoryCheckFailure, "Memory check failed");
    }
    else
    {
        TestSession::WriteInfo(
            "FabricTest.Memory",
            "Fabric session current memory = {0} MB. PeakWorkingSetSize {1} MB, PageFileUsage {2} MB, PeakPagefileUsage {3} MB",
            pmc.WorkingSetSize / (1024 * 1024),
            pmc.PeakWorkingSetSize / (1024 * 1024),
            pmc.PagefileUsage / (1024 * 1024),
            pmc.PeakPagefileUsage / (1024 * 1024));
        highMemoryStartTime_ = StopwatchTime::Zero;
    }
#endif
}

void RandomFabricTestSession::RemoveDeletedApplicationFolders()
{
    FabricNodeConfig emptyCodeConfig;
    vector<wstring> appIdsToRemove;
    for(auto appIdIter = deletedAppIds_.begin(); appIdIter != deletedAppIds_.end(); ++appIdIter)
    {
        wstring const& appId = *appIdIter;
        bool appIdFolderFound = false;
        for(auto nodeIdIter = allNodes_.begin(); nodeIdIter != allNodes_.end(); ++nodeIdIter)
        {
            LargeInteger const& nodeId = nodeIdIter->first;
            wstring nodeWorkingDirectory = FabricDispatcher.GetWorkingDirectory(nodeId.ToString());
            wstring deploymentFolder;
            auto err = Hosting2::HostingSubsystem::GetDeploymentFolder(emptyCodeConfig, nodeWorkingDirectory, deploymentFolder);
            if (!err.IsSuccess())
            {
                TestSession::WriteWarning(TraceSource, "Failed to get deploymentfolder {0}", err);
            }
            Management::ImageModel::RunLayoutSpecification runLayout(deploymentFolder);

            wstring appIdFolder = runLayout.GetApplicationFolder(appId);
            if(Directory::Exists(appIdFolder))
            {
                appIdFolderFound = true;
                if(!FabricDispatcher.Federation.ContainsNode(NodeId(nodeId)))
                {
                    //Delete app id folder if node is down
                    auto error = Directory::Delete(appIdFolder, true);
                    if(error.IsSuccess())
                    {
                        TestSession::WriteInfo(TraceSource, "Succesfully deleted app data from '{0}'", appIdFolder);
                    }
                    else
                    {
                        TestSession::WriteWarning(TraceSource, "Failed to delete app data from '{0}' due to {1}", appIdFolder, error);
                    }
                }
            }
        }

        if(!appIdFolderFound)
        {
            appIdsToRemove.push_back(appId);
        }
    }

    for(auto appIdIter = appIdsToRemove.begin(); appIdIter != appIdsToRemove.end(); ++appIdIter)
    {
        wstring appIdToRemove = *appIdIter;
        auto iter = find(deletedAppIds_.begin(), deletedAppIds_.end(), appIdToRemove);
        TestSession::FailTestIf(iter == deletedAppIds_.end(), "{0} not present in deletedAppIds_", appIdToRemove);
        deletedAppIds_.erase(iter);
    }
}

bool RandomFabricTestSession::TryRebuild()
{
    bool doRebuild = config_->FullRebuildInterval > 0 && iterations_ % config_->FullRebuildInterval == 0;
    if(doRebuild)
    {
        CauseFullRebuild();
        return true;
    }

    return false;
}

bool RandomFabricTestSession::TryUpgrade()
{
    if(!upgradedApp_.empty())
    {
        ServiceModel::ApplicationIdentifier appId = FabricDispatcher.ApplicationMap.GetAppId(upgradedApp_);
        if(FabricDispatcher.ApplicationMap.IsUpgradeOrRollbackPending(upgradedApp_) && upgradeUntil_ > DateTime::Now())
        {
            ExecuteDynamismStage();
            AddInput(WaitCommand + L"," + StringUtility::ToWString(config_->OpenTimeout.TotalSeconds()));
            AddInput(PauseCommand + L",25");
        }
        else
        {
            size_t version = createdapps_[upgradedApp_];
            vector<wstring> appServices;
            applicationHelper_.GetAllServiceNames(version, appServices);

            vector<wstring> appServicesToRemoveFromQuorumLoss;
            for (auto iter = appServices.begin(); iter != appServices.end(); ++iter)
            {
                appServicesToRemoveFromQuorumLoss.push_back(upgradedApp_ + L"/" + *iter);
            }

            // Remove all older services still in new version from quorum loss
            // ignoreNotCreatedServices is set to true because it is possible that upgrade has not completed at this point
            // and the new services after upgrade have not been created.
            RecoverFromQuorumLoss(appServicesToRemoveFromQuorumLoss, false/*do verify*/, true/*skip not created services*/);

            AddInput(FabricTestCommands::VerifyUpgradeAppCommand + FabricTestDispatcher::ParamDelimiter + upgradedApp_);
            AddInput(FabricTestCommands::VerifyCommand);

            // Start clients for all new services
            if (config_->ClientThreadCount > 0)
            {
                for (auto iter = newStatefulServicesAfterUpgrade_.begin(); iter != newStatefulServicesAfterUpgrade_.end(); ++iter)
                {
                    AddInput(FabricTestCommands::StartClientCommand + FabricTestDispatcher::ParamDelimiter + *iter);
                }
            }

            newStatefulServicesAfterUpgrade_.clear();
            upgradedApp_.clear();
            RemoveDeletedApplicationFolders();
        }

        return true;
    }

    bool doUpgrade = config_->UpgradeInterval > 0 && iterations_ % config_->UpgradeInterval == 0;
    if(doUpgrade)
    {
        UpgradeApplication();
        upgradeUntil_ = DateTime::Now() + (config_->VerifyTimeout/2);
        return true;
    }

    return false;
}

std::wstring RandomFabricTestSession::GenerateAppName()
{
    return wformatString("{0}{1}", AppNamePrefix, Common::Guid::NewGuid().ToString());
}

void RandomFabricTestSession::UpgradeApplication()
{
    int appIndex = random_.Next(0, static_cast<int>(createdapps_.size()));
    auto appIter = createdapps_.begin();
    advance(appIter, appIndex);
    size_t currentVersionIndex = appIter->second;
    wstring appName = appIter->first;
    size_t newVersionIndex = 0;
    if(config_->DoRandomUpgrade)
    {
        do
        {
            newVersionIndex = random_.Next(0, config_->ApplicationVersionsCount);
        }while(newVersionIndex == currentVersionIndex);
    }
    else
    {
        newVersionIndex = currentVersionIndex + 1;
    }

    if(newVersionIndex >= static_cast<size_t>(config_->ApplicationVersionsCount))
    {
        // Reached the end of provisioned versions so either delete and recreate at version 1 or do downgrade
        // Disable downgrades until Bug#886396 (FailoverUnit for successfully deleted service coming back to life) is fixed
        bool doDelete = random_.NextDouble() <= 1.0;
        if(doDelete)
        {
            vector<wstring> currentVersionServices;
            applicationHelper_.GetAllServiceNames(currentVersionIndex, currentVersionServices);

            vector<wstring> currentVersionServiceFullUris;
            for (auto service : currentVersionServices)
            {
                currentVersionServiceFullUris.push_back(appName + L"/" + service);
            }

            // Delete all services of this application running in current/not-newer version, with verify command = true.
            SafeDeleteService(currentVersionServiceFullUris);

            deletedAppIds_.push_back(FabricDispatcher.ApplicationMap.GetAppId(appName).ToString());

            AddInput(FabricTestCommands::DeleteApplicationCommand + FabricTestDispatcher::ParamDelimiter
                + appName);

            AddInput(FabricTestCommands::VerifyCommand);

            wstring newAppName = GenerateAppName();
            wstring createAppParam = wformatString("{0} {1} {2}", newAppName, RandomTestApplicationHelper::AppTypeName, applicationHelper_.GetVersionString(0));
            AddInput(FabricTestCommands::CreateApplicationCommand + FabricTestDispatcher::ParamDelimiter
                + createAppParam);

            if (config_->ClientThreadCount > 0)
            {
                vector<wstring> statefulServiceNames;
                applicationHelper_.GetAllStatefulServiceNames(0/*App starts at version 1*/, statefulServiceNames);
                for (auto iter = statefulServiceNames.begin(); iter != statefulServiceNames.end(); ++iter)
                {
                    wstring serviceName = newAppName + L"/" + *iter;
                    AddInput(FabricTestCommands::StartClientCommand + FabricTestDispatcher::ParamDelimiter + serviceName);
                }
            }

            AddInput(FabricTestCommands::VerifyCommand);

            createdapps_.erase(appName);
            createdapps_[newAppName] = 0;
            upgradedApp_.clear();
        }
        else
        {
            do
            {
                newVersionIndex = random_.Next(0, config_->ApplicationVersionsCount);
            }while(newVersionIndex == currentVersionIndex);
        }
    }

    if(newVersionIndex < static_cast<size_t>(config_->ApplicationVersionsCount))
    {
        vector<wstring> currentVersionServices;
        applicationHelper_.GetAllServiceNames(currentVersionIndex, currentVersionServices);
        vector<wstring> newVersionServices;
        applicationHelper_.GetAllServiceNames(newVersionIndex, newVersionServices);
        vector<wstring> servicesToBeDeleted;
        vector<wstring> serviceToBringOutOfQuorumLoss;
        // Find all older version services which are not going to be in the new version.
        for (auto service : currentVersionServices)
        {
            if (!ContainsName(newVersionServices, service))
            {
                servicesToBeDeleted.push_back(appName + L"/" + service);
            }

            serviceToBringOutOfQuorumLoss.push_back(appName + L"/" + service);
        }

        // Remove all current version services from quorum loss
        RecoverFromQuorumLoss(serviceToBringOutOfQuorumLoss, true);

        // Safe delete is not needed since the previous operaiton RecoverFromQuorumLoss will remove all the
        // services from quorum loss
        for (auto serviceName : servicesToBeDeleted)
        {
            // Now delete this service
            DeleteService(serviceName);
        }

        AddInput(FabricTestCommands::VerifyCommand);


        for (auto serIter = newVersionServices.begin(); serIter != newVersionServices.end(); ++serIter)
        {
            wstring & newVersionService = *serIter;
            if (!ContainsName(currentVersionServices, newVersionService))
            {
                if (applicationHelper_.IsStatefulService(newVersionIndex, newVersionService))
                {
                    wstring serviceName = appName + L"/" + newVersionService;
                    newStatefulServicesAfterUpgrade_.push_back(serviceName);
                }
            }
        }

        wstring upgradeAppParam = wformatString("{0} {1} Rolling", appName, applicationHelper_.GetVersionString(newVersionIndex));
        AddInput(FabricTestCommands::UpgradeApplicationCommand + FabricTestDispatcher::ParamDelimiter
            + upgradeAppParam);
        createdapps_[appName] = static_cast<int>(newVersionIndex);
        upgradedApp_ = appName;
    }
}

void RandomFabricTestSession::CauseFullRebuild()
{
    auto fmm = FabricDispatcher.GetFMM();
    TestSession::FailTestIf(!fmm, "FMM not ready");

    auto gfum = fmm->SnapshotGfum();
    AddInput(L"addbehavior b1 * * ChangeNotification");
    AddInput(L"addbehavior b2 * * ReplicaDown");

    for(auto fmmfu = gfum.begin(); fmmfu != gfum.end(); ++fmmfu)
    {
        auto replicas = fmmfu->GetReplicas();
        for(auto iter = replicas.begin(); iter != replicas.end(); iter++)
        {
            auto const & replica = *iter;
            if(replica.IsInCurrentConfiguration)
            {
                wstring removeNodeCommand(FabricTestCommands::RemoveCommand);
                removeNodeCommand.append(replica.FederationNodeId.ToString());

                if (FabricTestCommonConfig::GetConfig().EnableTStore)
                {
                    removeNodeCommand.append(L" ");
                    removeNodeCommand.append(FabricTestCommands::RemoveDataParameter);
                }

                AddInput(removeNodeCommand);
                AddInput(L"!wait");
                AddInput(FabricTestCommands::ResetStoreCommand + L" nodes=" + replica.FederationNodeId.ToString() + L" type=FM");
                nodeHistory_.push_back(replica.FederationNodeId.IdValue);
            }
        }
    }

    AddInput(L"!wait");
    AddInput(L"removebehavior b1");
    AddInput(L"removebehavior b2");
    AddInput(FabricTestCommands::VerifyCommand);
}

void RandomFabricTestSession::UpdateService()
{
    int serviceIndex = random_.Next(0, static_cast<int>(createdServices_.size()));
    wstring const& serviceName = createdServices_[serviceIndex];
    int replicaCount = FABRICSESSION.FabricDispatcher.ServiceMap.GetTargetReplicaForService(serviceName);
    int minReplicaSetSize = FABRICSESSION.FabricDispatcher.ServiceMap.GetMinReplicaSetSizeForService(serviceName);

    int newReplicaCount = 0;
    do
    {
        newReplicaCount = random_.Next(1, config_->MaxReplicas + 1);
    }
    while(newReplicaCount == replicaCount);

    int newMinReplicaSetSize = random_.Next(1, newReplicaCount + 1);

    if (FABRICSESSION.FabricDispatcher.ServiceMap.IsStatefulService(serviceName))
    {
        if (newMinReplicaSetSize == minReplicaSetSize)
        {
            AddInput(wformatString("{0} {1} Stateful TargetReplicaSetSize={2}",
                FabricTestCommands::UpdateServiceCommand,
                serviceName,
                newReplicaCount));
        }
        else
        {
            AddInput(wformatString("{0} {1} Stateful TargetReplicaSetSize={2} MinReplicaSetSize={3}",
                FabricTestCommands::UpdateServiceCommand,
                serviceName,
                newReplicaCount,
                newMinReplicaSetSize));
        }
    }
    else
    {
        AddInput(wformatString("{0} {1} Stateless InstanceCount={2}",
            FabricTestCommands::UpdateServiceCommand,
            serviceName,
            newReplicaCount));
    }
}

void RandomFabricTestSession::ActivateDeactivateNodes()
{
    vector<NodeId> activatedNodeIds;
    vector<NodeId> deActivatedNodeIds;

    // Decide whether to activate or deactivate
    // If no node is deactivated then deactivate
    // If more than the MaxRatioOfFabricNodesToDeactivate is deactivated then activate
    // Else randomly activate or deactivate a node
    FABRICSESSION.FabricDispatcher.Federation.ForEachFabricTestNode([&activatedNodeIds, &deActivatedNodeIds] (FabricTestNodeSPtr const & node)
    {
        if (node->IsActivated)
        {
            activatedNodeIds.push_back(node->Id);
        }
        else
        {
            deActivatedNodeIds.push_back(node->Id);
        }
    });

    size_t const total = activatedNodeIds.size() + deActivatedNodeIds.size();
    size_t const activatedNodesCount = activatedNodeIds.size();

    bool activate = false;
    if (static_cast<double>(activatedNodesCount) < (total * (1.0 - config_->MaxRatioOfFabricNodesToDeactivate)))
    {
        activate = true;
    }
    else if (activatedNodesCount == 0)
    {
        activate = false;
    }
    else if (activatedNodesCount == total)
    {
        activate = false;
    }
    else
    {
        activate = random_.Next() % 2 == 0;
    }

    NodeId target;
    if (!activate)
    {
        int index = random_.Next() % activatedNodeIds.size();
        target = activatedNodeIds[index];

        AddInput(wformatString("deactivatenode {0} Restart NodeDeactivationRequestReceived NodeDeactivationInProgress", target));
    }
    else
    {
        int index = random_.Next() % deActivatedNodeIds.size();
        target = deActivatedNodeIds[index];

        AddInput(wformatString("activatenode {0}", target));
    }
}

void RandomFabricTestSession::CreateDeleteServices()
{
    size_t nServicesToDelete = 2;
    if(deletedServices_.size() == 0 && createdServices_.size() > nServicesToDelete)
    {
        vector<wstring> servicesToBeDeleted;
        // Choose services to be deleted.
        for (size_t i = 0; i < nServicesToDelete; i++)
        {
            // Randomly choose a service to delete.
            int serviceIndex = random_.Next(0, static_cast<int>(createdServices_.size()));
            wstring const& serviceName = createdServices_[serviceIndex];

            // Delete this service "safely" -- avoiding any deadlock by any quorum-lost Failover Unit.
            servicesToBeDeleted.push_back(serviceName);
            createdServices_.erase(createdServices_.begin() + serviceIndex);
        }

        SafeDeleteService(servicesToBeDeleted);
    }
    else
    {
        EnsureAllServicesAndApplicationsAreCreated();
        deletedServices_.clear();
    }
}

wstring RandomFabricTestSession::GetServiceType(std::wstring const& serviceName)
{
    if(StringUtility::StartsWith<wstring>(serviceName, CalculatorServicePrefix))
    {
        return CalculatorService::DefaultServiceType;
    }
    else if(StringUtility::StartsWith<wstring>(serviceName, TestStoreServicePrefix))
    {
        return TestStoreService::DefaultServiceType;
    }
    else if(StringUtility::StartsWith<wstring>(serviceName, TestPersistedStoreServicePrefix))
    {
        return TestPersistedStoreService::DefaultServiceType;
    }
    else if (StringUtility::StartsWith<wstring>(serviceName, TXRServicePrefix))
    {
        return TXRService::DefaultServiceType;
    }

    TestSession::FailTest("Invalid service name: {0}", serviceName);
}

void RandomFabricTestSession::DeleteService(wstring serviceName)
{
    if(FabricDispatcher.ServiceMap.IsStatefulService(serviceName) && config_->ClientThreadCount > 0)
    {
        AddInput(
            FabricTestCommands::StopClientCommand +
            FabricTestDispatcher::ParamDelimiter +
            serviceName);

        wstring stopTimestampValidationCmd = wformatString(
            "{0}{1}{2}{3}{4}",
            FabricTestCommands::EnableLogTruncationTimestampValidation,
            FabricTestDispatcher::ParamDelimiter,
            serviceName,
            FabricTestDispatcher::ParamDelimiter,
            L"stop");

        AddInput(stopTimestampValidationCmd);
    }

    wstring errors;
    if (!config_->UnreliableTransportBehaviors.empty())
    {
        errors = L"errors=Success,UserServiceNotFound";
    }

    AddInput(
        FabricTestCommands::DeleteServiceCommand +
        FabricTestDispatcher::ParamDelimiter +
        serviceName +
        FabricTestDispatcher::ParamDelimiter +
        errors);

    deletedServices_.push_back(serviceName);
}


void RandomFabricTestSession::SafeDeleteService(wstring serviceName)
{
    // vector with one element
    vector<wstring> serviceNames (1, serviceName);

    // Call the overload of this method.
    SafeDeleteService(serviceNames);
}

// This method will make sure that deleting a service does not lead to a deadlock
// because of quorum-lost state of any FailoverUnit of the service.
void RandomFabricTestSession::SafeDeleteService(vector<wstring> serviceNames)
{
    // Bringup nodes in down/deactivated state and causing quorum-lost state.
    RecoverFromQuorumLoss(serviceNames, true);

    for( auto serviceName : serviceNames)
    {
        // Now delete this service
        DeleteService(serviceName);
    }

    // Add Verify command.
    AddInput(FabricTestCommands::VerifyCommand);
}

void RandomFabricTestSession::RecoverFromQuorumLoss(wstring serviceName, bool doVerify)
{
    // vector with one element
    vector<wstring> serviceNames (1, serviceName);

    // Call the overload of this method.
    RecoverFromQuorumLoss(serviceNames, doVerify, false);
}

// Bring up all the nodes which are causing quorum-lost state of any failover-unit of the services.
void RandomFabricTestSession::RecoverFromQuorumLoss(vector<wstring> serviceNames, bool doVerify)
{
    RecoverFromQuorumLoss(serviceNames, doVerify, false);
}

// Bring up all the nodes which are causing quorum-lost state of any failover-unit of the services.
void RandomFabricTestSession::RecoverFromQuorumLoss(vector<wstring> serviceNames, bool doVerify, bool ignoreNotCreatedServices)
{
    // These nodes are down and to be added back to insure DeleteService command doesn't get stuck due to quorum lost-state.
    set<NodeId> downNodeIdsToBeAdded;

    // These nodes are not-active and to be activated to insure DeleteService command doesn't get stuck due to quorum loss.
    set<NodeId> deactivatedNodeIdsToBeActivated;

    for( auto serviceName : serviceNames)
    {
        TestSession::WriteNoise(TraceSource, "RecoverFromQuorumLoss for service '{0}'", serviceName);

        if (ignoreNotCreatedServices && !FabricDispatcher.ServiceMap.IsCreatedService(serviceName))
        {
            // If ignoreNotCreatedServices is set to true and service does not exist ignore this service
            continue;
        }

        // If the service is NOT stateful-persistated then nothing to be done.
        if(!FabricDispatcher.ServiceMap.HasPersistedState(serviceName))
        {
            continue;
        }

        // Otherwise find out all the nodes which down/deavtivated and causing any FailoverUnit to be in quorum-lost state.
        // Check whether quorum is lost for any service FailoverUnit and in that case
        // add/activate node back for all down replicas
        // This call will return down/deactive nodeIds for all FailoverUnits which are in quorum-lost state.
        FabricDispatcher.RemoveFromQuorumLoss(serviceName, downNodeIdsToBeAdded, deactivatedNodeIdsToBeActivated);
    }
    // Bring nodes up in downNodeIdsToBeAdded
    for (auto downNodeId : downNodeIdsToBeAdded)
    {
        AddNode(downNodeId.IdValue);
    }

    // Activate nodes in deactivatedNodeIdsToBeActivated
    for ( auto deactivatedNodeId : deactivatedNodeIdsToBeActivated)
    {
        ActivateNode(deactivatedNodeId.IdValue);
    }

    if (doVerify)
    {
        // Add Verify command.
        AddInput(FabricTestCommands::VerifyCommand);
    }
}

void RandomFabricTestSession::ExecuteDynamismStage()
{
    int federationCount = static_cast<int>(FabricDispatcher.Federation.Count);
    TestSession::FailTestIf(federationCount == 0, "Federation count cannot be 0");
    if(federationCount < config_->MinNodes || config_->MaxDynamism > 0)
    {
        int dynamism = random_.Next(1, config_->MaxDynamism + 1);
        for(int i = 0; i < dynamism; i++)
        {
            if (i > 0 && (iterations_ & 0x01) == 0)
            {
                int interval = random_.Next(0, static_cast<int>(config_->MaxDynamismInterval.TotalPositiveMilliseconds()));
                AddInput(PauseCommand + L"," + wformatString(interval) + L"ms");
            }

            int ratio = 100 * (federationCount - config_->MinNodes) / (config_->MaxNodes - config_->MinNodes);

            int r = random_.Next() % 100;
            if ((r >= ratio || ratio == 0) && ratio != 100)
            {
                AddNode();
                ++federationCount;
            }
            else
            {
                RemoveNode();
                --federationCount;
            }
        }
    }

    AddInput(FabricTestCommands::ListCommand);
}

bool RandomFabricTestSession::IsOneNodeUpPerUpgradeDomain()
{
    for (auto iter = liveNodesPerUpgradeDomain_.begin(); iter != liveNodesPerUpgradeDomain_.end(); ++iter)
    {
        if(iter->second == 0)
        {
            return false;
        }
    }

    return true;
}

void RandomFabricTestSession::GenerateNodeRGCapacities()
{
    // Generate node RG resource capacities with configured probability
    if (random_.NextDouble() < config_->NodeRgCapacityProbability)
    {
        int nodeCpuCapacity = random_.Next(config_->NodeCpuCapacityMin, config_->NodeCpuCapacityMax + 1);
        int nodeMemoryCapacity = random_.Next(config_->NodeMemoryCapacityMin, config_->NodeMemoryCapacityMax + 1);
        nodeRGCapacities_ = wformatString(L"{0}:{1},{2}:{3}",
            ServiceModel::Constants::SystemMetricNameCpuCores,
            nodeCpuCapacity,
            ServiceModel::Constants::SystemMetricNameMemoryInMB,
            nodeMemoryCapacity);
        
        // Set the maximal allowed values for the RG metrics,
        // used for constraining SP and CP random limits generations
        int maxNumOfReplicas = config_->ApplicationCount * config_->AppMaxServiceTypes * config_->AppMaxPartitionCount * config_->AppMaxReplicaCount;
        int clusterCpuCapacity = config_->MinNodes * nodeCpuCapacity;
        int clusterMemoryCapacity = config_->MinNodes * nodeMemoryCapacity;
        applicationHelper_.PackageRGCapacities.insert(make_pair(L"CPU", static_cast<int>(double(clusterCpuCapacity)/maxNumOfReplicas)));
        applicationHelper_.PackageRGCapacities.insert(make_pair(L"Memory", static_cast<int>(double(clusterMemoryCapacity)/maxNumOfReplicas)));
    }
}

void RandomFabricTestSession::AddNode()
{
    LargeInteger node;

    do
    {
        int r = random_.Next();
        r %= static_cast<int>(allNodes_.size());
        auto iter = allNodes_.begin();
        advance(iter, r);
        node = iter->first;
    } while (ContainsNode(nodeHistory_, node) || FabricDispatcher.Federation.ContainsNode(NodeId(node)));

    AddNode(node);
}

void RandomFabricTestSession::AddNode(LargeInteger const& node)
{
    wstring udName = allNodes_[node];
    wstring udOption = udName.empty() ? L"" : (L" ud=" + udName);
    if(!udName.empty())
    {
        liveNodesPerUpgradeDomain_[udName] += 1;
    }

    wstring nodeRGCap = L"";
    // Set node RG resource capacities if generated
    if (nodeRGCapacities_.size() > 0)
    {
        nodeRGCap = wformatString(L" cap={0}", nodeRGCapacities_);
    }

    if(IsSeedNode(node) && !ContainsNode(liveSeedNodes_, node))
    {
        liveSeedNodes_.push_back(node);
        AddInput(FabricTestCommands::AddCommand + node.ToString() + udOption + nodeRGCap + L" nodeprops=NodeType:Seed" + L" retryerrors=" + config_->NodeOpenRetryErrors);
    }
    else
    {
        AddInput(FabricTestCommands::AddCommand + node.ToString() + udOption + nodeRGCap + L" nodeprops=NodeType:NonSeed" + L" retryerrors=" + config_->NodeOpenRetryErrors);
    }

    nodeHistory_.push_back(node);
}

void RandomFabricTestSession::ActivateNode(LargeInteger const& node)
{
     AddInput(wformatString("activatenode {0}", node));
}

void RandomFabricTestSession::RemoveNode()
{
    int r = random_.Next();

    bool done = false;
    // Kill runtime or code package only if either we have adhoc services or apps
    bool killRuntime = random_.NextDouble() < config_->KillRuntimeRatio && (createdapps_.size() > 0 || createdServices_.size() > 0);
    while (!done)
    {
        r %= FabricDispatcher.Federation.Count;
        NodeId id = FabricDispatcher.Federation.GetNodeIdAt(r);
        if (!ContainsNode(nodeHistory_, id.IdValue))
        {
            if(killRuntime)
            {
                bool killCodePackage = false;
                if(createdapps_.size() > 0 && createdServices_.size() == 0)
                {
                    // No self adhoc services so set kill code package to true
                    killCodePackage = true;
                }
                else if(createdapps_.size() == 0 && createdServices_.size() > 0)
                {
                    // Only adhoc services so set it to false
                    killCodePackage = false;
                }
                else
                {
                    killCodePackage = random_.NextDouble() > 0.5;
                }

                if(killCodePackage)
                {
                    AddInput(
                        FabricTestCommands::KillCodePackageCommand +
                        FabricTestDispatcher::ParamDelimiter +
                        id.IdValue.ToString());
                }
                else
                {
                    wstring runtimeTypeParam = random_.NextDouble() > 0.5 ? L"y" : L"n";

                    //remove runtime
                    AddInput(
                        FabricTestCommands::RemoveRuntimeCommand +
                        FabricTestDispatcher::ParamDelimiter +
                        id.IdValue.ToString() +
                        FabricTestDispatcher::ParamDelimiter +
                        runtimeTypeParam);

                    //Todo: make this configurable if needed
                    int minWaitMilliseconds = 2000;
                    int maxWaitMilliseconds = 6000;
                    int interval = random_.Next(minWaitMilliseconds, static_cast<int>(maxWaitMilliseconds));
                    AddInput(PauseCommand + L"," + wformatString(interval) + L"ms");

                    //add runtime back
                    AddInput(
                        FabricTestCommands::AddRuntimeCommand +
                        FabricTestDispatcher::ParamDelimiter +
                        id.IdValue.ToString() +
                        FabricTestDispatcher::ParamDelimiter +
                        runtimeTypeParam);
                }
            }
            else
            {
                if(IsSeedNode(id.IdValue))
                {
                    if(liveSeedNodes_.size() > static_cast<size_t>(quorum_))
                    {
                        vector<LargeInteger>::const_iterator result = find(liveSeedNodes_.begin(), liveSeedNodes_.end(), id.IdValue);
                        if(!RemoveNode(id.IdValue))
                        {
                            // Could not remove seed node due to upgrade domain constraints hence continue
                            r++;
                            continue;
                        }
                        else
                        {
                            liveSeedNodes_.erase(result);
                        }
                    }
                    else
                    {
                        // Cannot remove seed node since no quorum hence continue
                        r++;
                        continue;
                    }
                }
                else
                {
                    // Not a seed node try to remove else continue
                    if(!RemoveNode(id.IdValue))
                    {
                        r++;
                        continue;
                    }
                }
            }

            nodeHistory_.push_back(id.IdValue);

            done = true;
        }
        r++;
    }
}

bool RandomFabricTestSession::RemoveNode(LargeInteger const& node)
{
    wstring udName = allNodes_[node];
    wstring udOption = udName.empty() ? L"" : (L" ud=" + udName);
    if(!udName.empty() && liveNodesPerUpgradeDomain_[udName] == 1)
    {
        return false;
    }
    else
    {
        liveNodesPerUpgradeDomain_[udName] -= 1;
    }

    if ((config_->AbortRatio > 0) && (random_.NextDouble() < config_->AbortRatio))
    {
        AddInput(FabricTestCommands::AbortCommand + L" " + node.ToString());
    }
    else
    {
        AddInput(FabricTestCommands::RemoveCommand + node.ToString());
    }

    return true;
}

bool RandomFabricTestSession::IsSeedNode(LargeInteger const& node)
{
    for (LargeInteger seed : seedNodes_)
    {
        if (seed == node)
        {
            return true;
        }
    }

    return false;
}

bool RandomFabricTestSession::ContainsNode(std::vector<Common::LargeInteger> const& nodes, Common::LargeInteger const& node)
{
    vector<LargeInteger>::const_iterator result = find(nodes.begin(), nodes.end(), node);
    bool res = result != nodes.end();
    return res;
}

bool RandomFabricTestSession::ContainsName(std::vector<std::wstring> const& names, std::wstring const& name)
{
    auto iter = find(names.begin(), names.end(), name);
    bool res = iter != names.end();
    return res;
}

void RandomFabricTestSession::SetupWatchDogs()
{
    if (FabricTestSessionConfig::GetConfig().HealthVerificationEnabled)
    {
        for (int i = 0; i < config_->WatchDogCount; i++)
        {
            AddInput(wformatString("{0} start WD{1} {2}",
                FabricTestCommands::WatchDogCommand,
                i + 1,
                config_->WatchDogReportInterval.TotalMilliseconds() * 0.001));
        }
    }
}
