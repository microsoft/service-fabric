// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace Naming;
using namespace TestCommon;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ServiceModel;
using namespace Management::ClusterManager;
using namespace Api;

const uint64 HealthCheckRetryDefault = 600;
const uint64 HealthCheckStableDefault = 120;

const StringLiteral TraceSource("FabricTest.TestFabricClientUpgrade");

DEFINE_CREATE_COM_CLIENT( IFabricApplicationManagementClient, FabricApplicationClient)
DEFINE_CREATE_COM_CLIENT( IFabricApplicationManagementClient2, FabricApplicationClient2)
DEFINE_CREATE_COM_CLIENT( IFabricApplicationManagementClient3, FabricApplicationClient3)
DEFINE_CREATE_COM_CLIENT( IFabricApplicationManagementClient4, FabricApplicationClient4)
DEFINE_CREATE_COM_CLIENT( IFabricApplicationManagementClient5, FabricApplicationClient5)
DEFINE_CREATE_COM_CLIENT( IFabricApplicationManagementClient6, FabricApplicationClient6)
DEFINE_CREATE_COM_CLIENT( IFabricApplicationManagementClient7, FabricApplicationClient7)
DEFINE_CREATE_COM_CLIENT( IFabricApplicationManagementClient8, FabricApplicationClient8)
DEFINE_CREATE_COM_CLIENT( IFabricApplicationManagementClient9, FabricApplicationClient9)
DEFINE_CREATE_COM_CLIENT( IFabricApplicationManagementClient10, FabricApplicationClient10)

DEFINE_CREATE_COM_CLIENT( IFabricClusterManagementClient, FabricClusterClient)
DEFINE_CREATE_COM_CLIENT( IFabricClusterManagementClient2, FabricClusterClient2)
DEFINE_CREATE_COM_CLIENT( IFabricClusterManagementClient3, FabricClusterClient3)
DEFINE_CREATE_COM_CLIENT( IFabricClusterManagementClient4, FabricClusterClient4)

DEFINE_CREATE_COM_CLIENT( IInternalFabricApplicationManagementClient, InternalFabricApplicationClient)
DEFINE_CREATE_COM_CLIENT( IInternalFabricApplicationManagementClient2, InternalFabricApplicationClient2)

class TestFabricClientUpgrade::ApplicationUpgradeContext
{
public:
    static shared_ptr<ApplicationUpgradeContext> Create()
    {
        return ApplicationUpgradeContext::Create(
            ComPointer<IFabricApplicationUpgradeProgressResult3>(),
            L"",
            S_OK,
            UpgradeFailureReason::None,
            UpgradeDomainProgress());
    }

    static shared_ptr<ApplicationUpgradeContext> Create(
        wstring const & expectedHealthEvaluation, 
        UpgradeFailureReason::Enum expectedFailureReason,
        UpgradeDomainProgress const & expectedFailureProgress)
    {
        return ApplicationUpgradeContext::Create(
            ComPointer<IFabricApplicationUpgradeProgressResult3>(),
            expectedHealthEvaluation,
            S_OK,
            expectedFailureReason,
            expectedFailureProgress);
    }

    static shared_ptr<ApplicationUpgradeContext> Create(HRESULT expectedCompletionError)
    {
        return ApplicationUpgradeContext::Create(
            ComPointer<IFabricApplicationUpgradeProgressResult3>(),
            L"",
            expectedCompletionError,
            UpgradeFailureReason::None,
            UpgradeDomainProgress());
    }

    static shared_ptr<ApplicationUpgradeContext> Create(
        ApplicationUpgradeContext const & other,
        ComPointer<IFabricApplicationUpgradeProgressResult3> const & previousUpgradeProgress)
    {
        return ApplicationUpgradeContext::Create(
            previousUpgradeProgress,
            other.GetExpectedHealthEvaluation(),
            other.GetExpectedCompletionError(),
            other.GetExpectedFailureReason(),
            other.GetExpectedFailureProgress());
    }

    ComPointer<IFabricApplicationUpgradeProgressResult3> const & GetPreviousUpgradeProgress() const
    {
        return previousUpgradeProgress_;
    }

    wstring const & GetExpectedHealthEvaluation() const
    {
        return expectedHealthEvaluation_;
    }

    HRESULT GetExpectedCompletionError() const
    {
        return expectedCompletionError_;
    }

    UpgradeFailureReason::Enum GetExpectedFailureReason() const
    {
        return expectedFailureReason_;
    }

    UpgradeDomainProgress const & GetExpectedFailureProgress() const
    {
        return expectedFailureProgress_;
    }

private:
    ApplicationUpgradeContext(
        ComPointer<IFabricApplicationUpgradeProgressResult3> const & previousUpgradeProgress,
        wstring const & expectedHealthEvaluation,
        HRESULT expectedCompletionError,
        UpgradeFailureReason::Enum expectedFailureReason,
        UpgradeDomainProgress const & expectedFailureProgress)
        : previousUpgradeProgress_(previousUpgradeProgress)
        , expectedHealthEvaluation_(expectedHealthEvaluation)
        , expectedCompletionError_(expectedCompletionError)
        , expectedFailureReason_(expectedFailureReason)
        , expectedFailureProgress_(expectedFailureProgress)
    {
    }

    static shared_ptr<ApplicationUpgradeContext> Create(
        ComPointer<IFabricApplicationUpgradeProgressResult3> const & previousUpgradeProgress,
        wstring const & expectedHealthEvaluation,
        HRESULT expectedCompletionError,
        UpgradeFailureReason::Enum expectedFailureReason,
        UpgradeDomainProgress const & expectedFailureProgress)
    {
        return shared_ptr<ApplicationUpgradeContext>(new ApplicationUpgradeContext(
            previousUpgradeProgress,
            expectedHealthEvaluation,
            expectedCompletionError,
            expectedFailureReason,
            expectedFailureProgress));
    }

    ComPointer<IFabricApplicationUpgradeProgressResult3> previousUpgradeProgress_;
    wstring expectedHealthEvaluation_;
    HRESULT expectedCompletionError_;
    UpgradeFailureReason::Enum expectedFailureReason_;
    UpgradeDomainProgress expectedFailureProgress_;
};

class TestFabricClientUpgrade::FabricUpgradeContext
{
public:
    static shared_ptr<FabricUpgradeContext> Create()
    {
        return FabricUpgradeContext::Create(
            ComPointer<IFabricUpgradeProgressResult3>(),
            L"",
            UpgradeFailureReason::None,
            UpgradeDomainProgress());
    }

    static shared_ptr<FabricUpgradeContext> Create(
        wstring const & expectedHealthEvaluation, 
        UpgradeFailureReason::Enum expectedFailureReason,
        UpgradeDomainProgress const & expectedFailureProgress)
    {
        return FabricUpgradeContext::Create(
            ComPointer<IFabricUpgradeProgressResult3>(),
            expectedHealthEvaluation,
            expectedFailureReason,
            expectedFailureProgress);
    }

    static shared_ptr<FabricUpgradeContext> Create(
        FabricUpgradeContext const & other,
        ComPointer<IFabricUpgradeProgressResult3> const & previousUpgradeProgress)
    {
        return FabricUpgradeContext::Create(
            previousUpgradeProgress,
            other.GetExpectedHealthEvaluation(),
            other.GetExpectedFailureReason(),
            other.GetExpectedFailureProgress());
    }

    ComPointer<IFabricUpgradeProgressResult3> const & GetPreviousUpgradeProgress() const
    {
        return previousUpgradeProgress_;
    }

    wstring const & GetExpectedHealthEvaluation() const
    {
        return expectedHealthEvaluation_;
    }

    UpgradeFailureReason::Enum GetExpectedFailureReason() const
    {
        return expectedFailureReason_;
    }

    UpgradeDomainProgress const & GetExpectedFailureProgress() const
    {
        return expectedFailureProgress_;
    }

private:
    FabricUpgradeContext(
        ComPointer<IFabricUpgradeProgressResult3> const & previousUpgradeProgress,
        wstring const & expectedHealthEvaluation,
        UpgradeFailureReason::Enum expectedFailureReason,
        UpgradeDomainProgress const & expectedFailureProgress)
        : previousUpgradeProgress_(previousUpgradeProgress)
        , expectedHealthEvaluation_(expectedHealthEvaluation)
        , expectedFailureReason_(expectedFailureReason)
        , expectedFailureProgress_(expectedFailureProgress)
    {
    }

    static shared_ptr<FabricUpgradeContext> Create(
        ComPointer<IFabricUpgradeProgressResult3> const & previousUpgradeProgress,
        wstring const & expectedHealthEvaluation,
        UpgradeFailureReason::Enum expectedFailureReason,
        UpgradeDomainProgress const & expectedFailureProgress)
    {
        return shared_ptr<FabricUpgradeContext>(new FabricUpgradeContext(
            previousUpgradeProgress,
            expectedHealthEvaluation,
            expectedFailureReason,
            expectedFailureProgress));
    }

    ComPointer<IFabricUpgradeProgressResult3> previousUpgradeProgress_;
    wstring expectedHealthEvaluation_;
    UpgradeFailureReason::Enum expectedFailureReason_;
    UpgradeDomainProgress expectedFailureProgress_;
};

bool TestFabricClientUpgrade::ParallelProvisionApplicationType(StringCollection const & params)
{
    CHECK_PARAMS( 1, 3, FabricTestCommands::ProvisionApplicationTypeCommand);
        
    vector<wstring> buildPaths;
    StringUtility::Split<wstring>(params[0], buildPaths, L";");

    atomic_uint64 pendingOperations(buildPaths.size());
    ManualResetEvent event(false);

    for (auto const & buildPath : buildPaths)
    {
        StringCollection args;
        args.push_back(buildPath);

        for (auto ix=1; ix<params.size(); ++ix)
        {
            args.push_back(params[ix]);
        }

        Threadpool::Post([this, &pendingOperations, &event, args]()
        {
            this->ProvisionApplicationType(args);

            if (--pendingOperations == 0)
            {
                event.Set();
            }
        });
    }

    return event.WaitOne(FabricTestSessionConfig::GetConfig().NamingOperationTimeout);
}

bool TestFabricClientUpgrade::ProvisionApplicationType(StringCollection const & params)
{
    CHECK_PARAMS(1, 6, FabricTestCommands::ProvisionApplicationTypeCommand);

    wstring path = params[0];

    HRESULT expectedError = S_OK;
    if (params.size() > 1)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[1]);
        expectedError = ErrorCode(error).ToHResult();
    }

    CommandLineParser parser(params, 0);
    bool async = parser.GetBool(L"async");

    bool externalPath = parser.GetBool(L"externalPath");

    if (externalPath)
    {
        wstring appTypeName;
        parser.TryGetString(L"appTypeName", appTypeName);
        wstring appTypeVersion;
        parser.TryGetString(L"appTypeVersion", appTypeVersion);

        ProvisionApplicationTypeImpl(path, appTypeName, appTypeVersion, async, expectedError);
    }
    else
    {
        wstring applicationPackageCleanupPolicyStr;
        parser.TryGetString(L"applicationPackageCleanupPolicy", applicationPackageCleanupPolicyStr);
        ApplicationPackageCleanupPolicy::Enum cleanupPolicy = ApplicationPackageCleanupPolicy::Enum::Default;

        wstring incomingBuildPath = Path::Combine(ApplicationBuilder::ApplicationPackageFolderInImageStore, path);
        if (applicationPackageCleanupPolicyStr == L"Automatic")
        {
            cleanupPolicy = ApplicationPackageCleanupPolicy::Enum::Automatic;
        }
        else if (applicationPackageCleanupPolicyStr == L"Manual")
        {
            cleanupPolicy = ApplicationPackageCleanupPolicy::Enum::Manual;
        }
        else if (applicationPackageCleanupPolicyStr == L"Default")
        {
            cleanupPolicy = ApplicationPackageCleanupPolicy::Enum::Default;
        }
        else
        {
            cleanupPolicy = ApplicationPackageCleanupPolicy::Enum::Invalid;
        }

        ProvisionApplicationTypeImpl(incomingBuildPath, async, cleanupPolicy, expectedError);
    }

    return true;
}

bool TestFabricClientUpgrade::ProvisionFabric(StringCollection const & params)
{
    CHECK_PARAMS( 1, 3, FabricTestCommands::ProvisionFabricCommand);

    CommandLineParser parser(params, 0);
    wstring codeVersion;
    wstring codeFilepath;
    wstring configVersion;
    wstring configFilepath;
    size_t numOfCodeConfig = 0;

    // relative to ImageStore root
    wstring incomingDirectory = L"incoming";

    if(parser.TryGetString(L"code", codeVersion))
    {                                                                                           
        codeFilepath = Path::Combine(incomingDirectory, Management::ImageModel::WinFabStoreLayoutSpecification::GetPatchFileName(codeVersion));
        numOfCodeConfig++;
    }                                                                                           

    if(parser.TryGetString(L"config", configVersion))                                                    
    {                                                                                           
        configFilepath = Path::Combine(incomingDirectory, Management::ImageModel::WinFabStoreLayoutSpecification::GetClusterManifestFileName(configVersion));
        numOfCodeConfig++ ;                                                                     
    }                                                                                           

    if (numOfCodeConfig==0)                                                                     
    {                                                                                           
        TestSession::WriteError(TraceSource, "Must provide either code or config");   
        return false;
    }     
    
    HRESULT expectedError = S_OK;
    if (params.size() == numOfCodeConfig+1)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[numOfCodeConfig]);
        expectedError = ErrorCode(error).ToHResult();
    }       
    
    TestSession::WriteNoise(TraceSource, "ProvisionFabric called for codeVersion {0}, configVersion {1}", codeVersion, configVersion);

    auto managementClient = CreateFabricClusterClient2(FABRICSESSION.FabricDispatcher.Federation);    

    TestFabricClient::PerformFabricClientOperation(
        L"ProvisionFabric",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginProvisionFabric(
                (codeFilepath.empty() ? NULL : codeFilepath.c_str()),
                (configFilepath.empty() ? NULL : configFilepath.c_str()),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndProvisionFabric(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_FABRIC_VERSION_ALREADY_EXISTS);   

    return true;
}

bool TestFabricClientUpgrade::UnprovisionApplicationType(StringCollection const & params)
{
    CHECK_PARAMS(2, 4,FabricTestCommands::UnprovisionApplicationTypeCommand);        

    wstring typeName = params[0];
    wstring typeVersion = (params[1] == L"-empty-" ? L"" : params[1]);

    HRESULT expectedError = S_OK;
    if (params.size() > 2)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[2]);
        expectedError = ErrorCode(error).ToHResult();
    }

    CommandLineParser parser(params, 0);
    bool async = parser.GetBool(L"async");

    UnprovisionApplicationTypeImpl(typeName, typeVersion, async, expectedError);

    return true;
}

bool TestFabricClientUpgrade::UnprovisionFabric(StringCollection const & params)
{
    CHECK_PARAMS(1, 3,FabricTestCommands::UnprovisionFabricCommand);        

    CommandLineParser parser(params, 0);
    wstring codeVersion;
    wstring configVersion;
    size_t numOfCodeConfig = 0;

    if (parser.TryGetString(L"code", codeVersion))                                                      
    {                                                                                           
        numOfCodeConfig++;
    }                                                                                           

    if (parser.TryGetString(L"config", configVersion))                                                    
    {                                                                                           
        numOfCodeConfig++;
    }                                                                                           

    if (numOfCodeConfig==0)                                                                     
    {                                                                                           
        TestSession::WriteError(TraceSource, "Must provide either code or config");    
        return false;
    }  
    
    HRESULT expectedError = S_OK;
    if (params.size() == numOfCodeConfig+1)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[numOfCodeConfig]);
        expectedError = ErrorCode(error).ToHResult();
    }
    
    TestSession::WriteNoise(TraceSource, "UnprovisionFabric called for codeVersion {0}, configVersion {1}", codeVersion, configVersion);

    auto managementClient = CreateFabricClusterClient2(FABRICSESSION.FabricDispatcher.Federation);
   
    TestFabricClient::PerformFabricClientOperation(
        L"UnprovisionFabric",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginUnprovisionFabric(
                (codeVersion.empty() ? NULL : codeVersion.c_str()),
                (configVersion.empty() ? NULL : configVersion.c_str()),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndUnprovisionFabric(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_FABRIC_VERSION_NOT_FOUND);

    return true;
}

bool TestFabricClientUpgrade::CreateApplication(StringCollection const & params)
{
    CHECK_PARAMS(3, 999, FabricTestCommands::CreateApplicationCommand);        

    NamingUri name;
    if (!NamingUri::TryParse(params[0], name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    wstring type = params[1];
    wstring version = params[2];

    CommandLineParser parser(params, 3);

    StringCollection defaultServices;
    ApplicationBuilder::GetDefaultServices(type, version, defaultServices);

    map<wstring, wstring> appParameters;
    parser.GetMap(L"appparam", appParameters);

    ScopedHeap heap;
    auto applicationParametersCollection = heap.AddItem<FABRIC_APPLICATION_PARAMETER_LIST>();
    auto applicationParameters = heap.AddArray<FABRIC_APPLICATION_PARAMETER>(appParameters.size());

    applicationParametersCollection->Count = static_cast<ULONG>(applicationParameters.GetCount());
    applicationParametersCollection->Items= applicationParameters.GetRawArray();

    size_t index = 0;
    for(auto it = appParameters.begin(); it != appParameters.end(); it++)
    {
        applicationParameters[index].Name = heap.AddString(it->first);
        applicationParameters[index].Value = heap.AddString(it->second);
        ++index;
    }

    Common::StringCollection scaleoutParameters;
    parser.GetVector(L"scaleout", scaleoutParameters);

    auto applicationDescriptionEx1 = heap.AddItem<FABRIC_APPLICATION_DESCRIPTION_EX1>();
    if (scaleoutParameters.size() > 0)
    {
        if (scaleoutParameters.size() < 2 || (scaleoutParameters.size() - 2) % 4 != 0)
        {
            // Error!
            TestSession::WriteError(
                TraceSource,
                "Invalid specification for application metrics.");
            return false;
        }

        auto applicationCapacityDescription = heap.AddItem<FABRIC_APPLICATION_CAPACITY_DESCRIPTION>();

        applicationCapacityDescription->MinimumNodes = Common::Int32_Parse(scaleoutParameters[0]);
        applicationCapacityDescription->MaximumNodes = Common::Int32_Parse(scaleoutParameters[1]);

        if (scaleoutParameters.size() > 2)
        {
            uint metricCount = (static_cast<uint>(scaleoutParameters.size()) - 2) / 4;
            auto metricList = heap.AddItem<FABRIC_APPLICATION_METRIC_LIST>();
            auto metrics = heap.AddArray<FABRIC_APPLICATION_METRIC_DESCRIPTION>(metricCount);

            metricList->Count = static_cast<ULONG>(metricCount);

            for (uint i = 0; i < metricCount; i++)
            {
                metrics[i].Name = heap.AddString(scaleoutParameters[2 + 4 * i]);
                metrics[i].NodeReservationCapacity = static_cast<ULONG> (Common::Int64_Parse(scaleoutParameters[2 + 4 * i + 1]));
                metrics[i].MaximumNodeCapacity = static_cast<ULONG> (Common::Int64_Parse(scaleoutParameters[2 + 4 * i + 2]));
                metrics[i].TotalApplicationCapacity = static_cast<ULONG> (Common::Int64_Parse(scaleoutParameters[2 + 4 * i + 3]));
            }

            metricList->Capacities = metrics.GetRawArray();
            applicationCapacityDescription->Metrics = metricList.GetRawPointer();
        }
        
        applicationDescriptionEx1->ApplicationCapacity = applicationCapacityDescription.GetRawPointer();
    }

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    StringCollection defaultServiceNames;
    for (auto iter = defaultServices.begin(); iter != defaultServices.end(); iter++)
    { 
        wstring appName = name.ToString();
        wstring slash(L"/");
        if(!Common::StringUtility::EndsWith(appName, slash))
        {
            appName.append(slash);
        }

        defaultServiceNames.push_back(appName.append(*iter));
    }

    bool updateApplicationMapOnError = parser.GetBool(L"updateApplicationMapOnError");
    if(expectedError == S_OK || updateApplicationMapOnError)
    {
        FABRICSESSION.FabricDispatcher.ApplicationMap.AddApplication(name.Name, type, ApplicationBuilder::GetApplicationBuilderName(type, version));
    }

    FABRIC_APPLICATION_DESCRIPTION applicationDescription = { 0 };
    applicationDescription.ApplicationName = heap.AddString(name.Name);
    applicationDescription.ApplicationTypeName = type.c_str();
    applicationDescription.ApplicationTypeVersion = version.c_str();
    applicationDescription.ApplicationParameters = applicationParametersCollection.GetRawPointer();
    applicationDescription.Reserved = applicationDescriptionEx1.GetRawPointer();
    CreateApplicationImpl(applicationDescription, expectedError);

    if(expectedError == S_OK || updateApplicationMapOnError)
    {
        TestSession::WriteNoise(TraceSource, "Application '{0}' has been created with {1}.", name.Name, expectedError);

        //Update appId in ApplicationMap to match with actual Application created at CM.
        Management::ClusterManager::ServiceModelApplicationId appId;

        int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
        do
        {
            --remainingRetries;

            // Get appId from CM
            auto cm = FABRICSESSION.FabricDispatcher.GetCM();

            if (cm)
            {
                auto err = cm->Test_GetApplicationId(name, appId);
                if (err.IsSuccess())
                {
                    break;
                }
                else
                {
                    TestSession::WriteWarning(TraceSource, "Error when getting application id: {0}, remainingRetries {1}", err, remainingRetries);
                }
            }

            Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));

        } while (remainingRetries >= 0);

        TestSession::FailTestIf(remainingRetries < 0, "Could not get application id, all retries are exhausted");

        // Update AppId in ApplicationMap
        FABRICSESSION.FabricDispatcher.ApplicationMap.UpdateApplicationId(name.Name, appId);

        vector<PartitionedServiceDescriptor> serviceDescriptors;
        TestFabricClientSPtr->GetPartitionedServiceDescriptors(defaultServiceNames, serviceDescriptors);

        for (auto && serviceDescriptor : serviceDescriptors)
        { 
            TestSession::FailTestIf(
                FABRICSESSION.FabricDispatcher.ServiceMap.IsCreatedService(serviceDescriptor.Service.Name),
                "Service with name {0} already exists", serviceDescriptor.Service.Name);

            FABRICSESSION.FabricDispatcher.ServiceMap.AddPartitionedServiceDescriptor(serviceDescriptor.Service.Name, move(serviceDescriptor));
        }
    }

    return true;
}

bool TestFabricClientUpgrade::UpdateApplication(StringCollection const & params)
{
    CHECK_PARAMS(2, 999, FabricTestCommands::UpdateApplicationCommand);

    NamingUri name;
    if (!NamingUri::TryParse(params[0], name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    CommandLineParser parser(params, 1);


    ScopedHeap heap;

    bool removeApplicationCapacity = parser.GetBool(L"removeScaleout");

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    FABRIC_APPLICATION_UPDATE_DESCRIPTION updateDescription = { 0 };

    TestSession::WriteInfo(
        TraceSource,
        "Calling updateapp for application {0} with removeScaleout={1}, minNodes={2}, maxNodes={3}, Flags={4}",
        params[0],
        updateDescription.RemoveApplicationCapacity,
        updateDescription.MinimumNodes,
        updateDescription.MaximumNodes,
        updateDescription.Flags = updateDescription.Flags);

    updateDescription.ApplicationName = heap.AddString(name.Name);

    if (removeApplicationCapacity)
    {
        // Just remove all scaleout parameters
        updateDescription.RemoveApplicationCapacity = true;
    }
    else
    { 
        int minCount, maxCount;
        if (parser.TryGetInt(L"minCount", minCount))
        {
            updateDescription.Flags = updateDescription.Flags | FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS::FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS_MINNODES;
            updateDescription.MinimumNodes = minCount;
        }
        if (parser.TryGetInt(L"maxCount", maxCount))
        {
            updateDescription.Flags = updateDescription.Flags | FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS::FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS_MAXNODES;
            updateDescription.MaximumNodes = maxCount;
        }

        Common::StringCollection metricCollection;
        if (parser.GetVector(L"metrics", metricCollection))
        {
            updateDescription.Flags = updateDescription.Flags | FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS::FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS_METRICS;
            if ((metricCollection.size() % 4) != 0)
            {
                TestSession::WriteError(
                    TraceSource,
                    "Invalid specification of application metrics.");
                return false;
            }

            uint metricCount = static_cast<uint>(metricCollection.size()) / 4;
            auto metricList = heap.AddItem<FABRIC_APPLICATION_METRIC_LIST>();
            auto metrics = heap.AddArray<FABRIC_APPLICATION_METRIC_DESCRIPTION>(metricCount);

            metricList->Count = static_cast<ULONG>(metricCount);

            for (uint i = 0; i < metricCount; i++)
            {
                metrics[i].Name = heap.AddString(metricCollection[4 * i]);
                metrics[i].NodeReservationCapacity = static_cast<ULONG>(Common::Int64_Parse(metricCollection[4 * i + 1]));
                metrics[i].MaximumNodeCapacity = static_cast<ULONG>(Common::Int64_Parse(metricCollection[4 * i + 2]));
                metrics[i].TotalApplicationCapacity = static_cast<ULONG>(Common::Int64_Parse(metricCollection[4 * i + 3]));
            }

            metricList->Capacities = metrics.GetRawArray();
            updateDescription.Metrics = metricList.GetRawPointer();
        }
    }

    TestSession::WriteInfo(
        TraceSource,
        "Invoking updateapp for application {0} with removeScaleout={1}, minCount={2}, maxCount={3}, Flags={4}", 
        params[0],
        updateDescription.RemoveApplicationCapacity,
        updateDescription.MinimumNodes,
        updateDescription.MaximumNodes,
        updateDescription.Flags = updateDescription.Flags);


    UpdateApplicationImpl(updateDescription, expectedError);

    return true;
}

bool TestFabricClientUpgrade::DeleteApplication(StringCollection const & params)
{
    if(params.size() > 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeleteApplicationCommand);
        return false;
    }

    NamingUri name;
    if (!NamingUri::TryParse(params[0], name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    CommandLineParser parser(params, 1);
    bool isForce = parser.GetBool(L"isForce");

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();
    
    DeleteApplicationImpl(name, isForce, expectedError);

    if (expectedError == S_OK)
    {
        FABRICSESSION.FabricDispatcher.ApplicationMap.DeleteApplication(name.Name);
        FABRICSESSION.FabricDispatcher.ServiceMap.MarkAppAsDeleted(name.ToString());
    }

    return true;
}

bool TestFabricClientUpgrade::UpgradeApplication(StringCollection const & params)
{
    CHECK_PARAMS(3, 999, FabricTestCommands::UpgradeApplicationCommand);

    wstring appName = params[0];
    NamingUri name;
    if (!NamingUri::TryParse(appName, name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", appName);
        return false;
    }

    wstring targetVersion = params[1];
    FABRIC_APPLICATION_UPGRADE_KIND upgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_INVALID;
    if(params[2] == L"Rolling")
    {
        upgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
    }

    CommandLineParser parser(params, 3);

    map<wstring, wstring> appParameters;
    parser.GetMap(L"appparam", appParameters);

    ScopedHeap heap;
    auto applicationParametersCollection = heap.AddItem<FABRIC_APPLICATION_PARAMETER_LIST>();
    auto applicationParameters = heap.AddArray<FABRIC_APPLICATION_PARAMETER>(appParameters.size());

    applicationParametersCollection->Count = static_cast<ULONG>(applicationParameters.GetCount());
    applicationParametersCollection->Items= applicationParameters.GetRawArray();

    size_t index = 0;
    for(auto it = appParameters.begin(); it != appParameters.end(); it++)
    {
        applicationParameters[index].Name = heap.AddString(it->first);
        applicationParameters[index].Value = heap.AddString(it->second);
        ++index;
    }

    int64 timeoutInSeconds;
    parser.TryGetInt64(L"timeout", timeoutInSeconds, FabricTestSessionConfig::GetConfig().NamingOperationTimeout.TotalSeconds());

    TimeSpan timeout = TimeSpan::FromSeconds(static_cast<double>(timeoutInSeconds));

    vector<wstring> errorStrings;
    parser.GetVector(L"errors", errorStrings, L"Success,ApplicationAlreadyInTargetVersion");
    vector<HRESULT> expectedErrors;
    for (auto it = errorStrings.begin(); it != errorStrings.end(); ++it)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(*it);
        expectedErrors.push_back(ErrorCode(error).ToHResult());
    }

    bool startUpgradeCheck = parser.GetBool(L"startupgradecheck");           

    wstring errorString;
    parser.TryGetString(L"upgradecheckerror", errorString, L"Success");
    ErrorCodeValue::Enum upgradeCheckError = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);

    FABRIC_APPLICATION_UPGRADE_DESCRIPTION upgradeDescription = {0};
    upgradeDescription.ApplicationName = appName.c_str();
    upgradeDescription.TargetApplicationTypeVersion = targetVersion.c_str();
    upgradeDescription.ApplicationParameters = applicationParametersCollection.GetRawPointer();
    upgradeDescription.UpgradeKind = upgradeKind;

    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION policyDescription;
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1 policyDescriptionEx1 = {0};
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY monitoringPolicy;
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1 monitoringPolicyEx1 = {0};
    FABRIC_APPLICATION_HEALTH_POLICY healthPolicy = {0};

    memset( &policyDescription, 0, sizeof(policyDescription) );
    memset( &monitoringPolicy, 0, sizeof(monitoringPolicy) );

    policyDescription.Reserved = &policyDescriptionEx1;
    policyDescriptionEx1.MonitoringPolicy = &monitoringPolicy;
    monitoringPolicy.Reserved = &monitoringPolicyEx1;

    if (upgradeKind == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
    {
        this->GetRollingUpgradePolicyDescriptionForUpgrade(
            parser,
            policyDescription);

        wstring healthPolicyStr;
        parser.TryGetString(L"jsonpolicy", healthPolicyStr, L"");

        if (healthPolicyStr.empty())
        {
            policyDescriptionEx1.HealthPolicy = NULL;
        }
        else
        {
            ApplicationHealthPolicy parsedHealthPolicy;
            ApplicationHealthPolicy::FromString(healthPolicyStr, parsedHealthPolicy);

            parsedHealthPolicy.ToPublicApi(heap, healthPolicy);

            policyDescriptionEx1.HealthPolicy = &healthPolicy;
        }

        upgradeDescription.UpgradePolicyDescription = &policyDescription;
    }
    else
    {
        upgradeDescription.UpgradePolicyDescription = NULL;
    }

    FABRICSESSION.FabricDispatcher.ApplicationMap.InterruptUpgrade(appName);

    startUpgradeCheck = startUpgradeCheck || (find(expectedErrors.begin(), expectedErrors.end(), S_OK) != expectedErrors.end());
    wstring appTypeName = FABRICSESSION.FabricDispatcher.ApplicationMap.GetAppId(appName).ApplicationTypeName;
    wstring previousAppBuilderName = FABRICSESSION.FabricDispatcher.ApplicationMap.GetCurrentApplicationBuilderName(appName);

    bool forceRestart = (policyDescription.ForceRestart == TRUE);

    if (startUpgradeCheck)
    {
        FABRICSESSION.FabricDispatcher.ApplicationMap.StartUpgradeApplication(
            appName, 
            ApplicationBuilder::GetApplicationBuilderName(appTypeName, targetVersion),
            forceRestart);
    }

    UpgradeApplicationImpl(upgradeDescription, timeout, expectedErrors);

    if (startUpgradeCheck)
    {
        CheckUpgradeAppComplete(
            appName, 
            appTypeName, 
            targetVersion,
            previousAppBuilderName,
            ApplicationUpgradeContext::Create(ErrorCode(upgradeCheckError).ToHResult()));
    }

    return true;
}

bool TestFabricClientUpgrade::UpdateApplicationUpgrade(StringCollection const & params)
{
    CHECK_PARAMS(2, 999, FabricTestCommands::UpdateApplicationUpgradeCommand);

    wstring appName = params[0];
    NamingUri name;
    if (!NamingUri::TryParse(appName, name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", appName);
        return false;
    }

    CommandLineParser parser(params, 1);

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    ScopedHeap heap;

    FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION updateDescription = {0};
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION policyDescription;
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1 policyDescriptionEx1 = {0};
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY monitoringPolicy;
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1 monitoringPolicyEx1 = {0};
    FABRIC_APPLICATION_HEALTH_POLICY healthPolicy = {0};

    memset(&policyDescription, 0, sizeof(policyDescription));
    memset(&monitoringPolicy, 0, sizeof(monitoringPolicy));

    updateDescription.ApplicationName = appName.c_str();
    updateDescription.UpgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;

    updateDescription.UpgradePolicyDescription = &policyDescription;
    policyDescription.Reserved = &policyDescriptionEx1;
    policyDescriptionEx1.MonitoringPolicy = &monitoringPolicy;
    policyDescriptionEx1.HealthPolicy = &healthPolicy;
    monitoringPolicy.Reserved = &monitoringPolicyEx1;

    this->GetRollingUpgradePolicyDescriptionForUpdate(
        parser,
        updateDescription.UpdateFlags,
        policyDescription);

    wstring healthPolicyStr;
    if (parser.TryGetString(L"jsonpolicy", healthPolicyStr, L""))
    {
        updateDescription.UpdateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_POLICY;

        ApplicationHealthPolicy parsedHealthPolicy;
        ApplicationHealthPolicy::FromString(healthPolicyStr, parsedHealthPolicy);

        parsedHealthPolicy.ToPublicApi(heap, healthPolicy);
    }

    UpdateApplicationUpgradeImpl(updateDescription, expectedError);

    return true;
}

bool TestFabricClientUpgrade::VerifyApplicationUpgradeDescription(StringCollection const & params)
{
    CHECK_PARAMS(2, 999, FabricTestCommands::VerifyApplicationUpgradeDescriptionCommand);

    wstring appName = params[0];
    NamingUri name;
    if (!NamingUri::TryParse(appName, name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", appName);
        return false;
    }

    CommandLineParser parser(params, 1);

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    ComPointer<IFabricApplicationUpgradeProgressResult3> upgradeProgress;
    UpgradeAppStatusImpl(appName, expectedError, upgradeProgress);

    if (expectedError != S_OK)
    {
        return true;
    }

    FABRIC_APPLICATION_UPGRADE_PROGRESS const * progressDescription = upgradeProgress->get_UpgradeProgress();
    TestSession::FailTestIf(progressDescription == NULL, "get_UpgradeProgress() is NULL");

    FABRIC_APPLICATION_UPGRADE_DESCRIPTION const * upgradeDescription = progressDescription->UpgradeDescription;
    TestSession::FailTestIf(upgradeDescription == NULL, "UpgradeDescription is NULL");
    TestSession::FailTestIf(upgradeDescription->UpgradePolicyDescription == NULL, "UpgradePolicyDescription is NULL");

    Management::ClusterManager::ApplicationUpgradeDescription internalDescription;
    auto error = internalDescription.FromPublicApi(*upgradeDescription);
    TestSession::FailTestIfNot(error.IsSuccess(), "ApplicationUpgradeDescription::FromPublicApi error={0}", error);

    // verify round-tripped application name
    {
        auto const & expected = name;

        NamingUri actual;
        bool parsed = NamingUri::TryParse(upgradeDescription->ApplicationName, actual);
        TestSession::FailTestIfNot(
            parsed,
            "ApplicationName failed to parse from '{0}'",
            wstring(upgradeDescription->ApplicationName));

        TestSession::FailTestIfNot(
            expected == actual, 
            "ApplicationName mismatch: expected={0} actual={1}",
            expected,
            actual);
    }

    wstring version;
    if (parser.TryGetString(L"version", version, L""))
    {
        auto expected = version;

        auto actual = wstring(upgradeDescription->TargetApplicationTypeVersion);

        TestSession::FailTestIfNot(
            expected == actual, 
            "TargetApplicationTypeVersion mismatch: expected={0} actual={1}",
            expected,
            actual);
    }

    map<wstring, wstring> appParams;
    if (parser.GetMap(L"appparam", appParams))
    {
        auto const & expected = appParams;

        auto const & actual = internalDescription.ApplicationParameters;

        TestSession::FailTestIfNot(
            expected.size() == actual.size(), 
            "ApplicationParameters size mismatch: expected={0} actual={1}",
            expected.size(),
            actual.size());

        auto it1 = expected.begin();
        auto it2 = actual.begin();
        while (it1 != expected.end() && it2 != actual.end())
        {
            TestSession::FailTestIfNot(
                it1->first == it2->first,
                "ApplicationParameters key mismatch: expected={0} actual={1}",
                it1->first,
                it2->first);

            TestSession::FailTestIfNot(
                it1->second == it2->second,
                "ApplicationParameters value mismatch: expected={0} actual={1}",
                it1->second,
                it2->second);

            ++it1;
            ++it2;
        }
    }

    auto const * policyDescription = static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION*>(
        upgradeDescription->UpgradePolicyDescription);

    this->VerifyRollingUpgradePolicyDescription(
        parser,
        *policyDescription);

    wstring healthPolicyStr;
    if (parser.TryGetString(L"jsonpolicy", healthPolicyStr, L""))
    {
        TestSession::FailTestIf(policyDescription->Reserved == NULL, "Reserved is NULL");
        auto const * policyDescriptionEx1Ptr = static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*>(policyDescription->Reserved);

        TestSession::FailTestIf(policyDescriptionEx1Ptr->HealthPolicy == NULL, "HealthPolicy is NULL");

        auto const * healthPolicy = static_cast<FABRIC_APPLICATION_HEALTH_POLICY*>(
            policyDescriptionEx1Ptr->HealthPolicy);

        ApplicationHealthPolicy expected;
        ApplicationHealthPolicy::FromString(healthPolicyStr, expected);

        ApplicationHealthPolicy actual;
        error = actual.FromPublicApi(*healthPolicy);

        TestSession::FailTestIfNot(error.IsSuccess(), "ApplicationHealthPolicy::FromPublicApi error={0}", error);

        TestSession::FailTestIfNot(
            expected == actual, 
            "HealthPolicy mismatch: expected={0} actual={1}",
            expected,
            actual);
    }

    return true;
}

bool TestFabricClientUpgrade::RollbackApplicationUpgrade(Common::StringCollection const & params)
{
    CHECK_PARAMS(1, 999, FabricTestCommands::RollbackApplicationUpgradeCommand);

    wstring appName = params[0];
    NamingUri appUri;
    if (!NamingUri::TryParse(appName, appUri))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", appName);
        return false;
    }

    CommandLineParser parser(params);

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();
    RollbackApplicationUpgradeImpl(appUri, expectedError);

    return true;
}

bool TestFabricClientUpgrade::SetRollbackApplication(Common::StringCollection const & params)
{
    CHECK_PARAMS(2, 999, FabricTestCommands::SetRollbackApplicationCommand);

    wstring appName = params[0];
    NamingUri name;
    if (!NamingUri::TryParse(appName, name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", appName);
        return false;
    }

    wstring targetVersion = params[1];

    CommandLineParser parser(params);
    wstring expectedEvaluationReason;
    parser.TryGetString(L"expectedreason", expectedEvaluationReason);
    
    UpgradeFailureReason::Enum expectedFailureReason = ParseUpgradeFailureReason(parser);

    wstring jsonExpectedFailureProgress;
    parser.TryGetString(L"jsonfailureprogress", jsonExpectedFailureProgress, L"");

    UpgradeDomainProgress expectedFailureProgress;
    if (!jsonExpectedFailureProgress.empty())
    {
        UpgradeDomainProgress::FromString(jsonExpectedFailureProgress, expectedFailureProgress);
    }

    wstring previousAppBuilderName = FABRICSESSION.FabricDispatcher.ApplicationMap.GetCurrentApplicationBuilderName(appName);

    wstring appTypeName = FABRICSESSION.FabricDispatcher.ApplicationMap.GetAppId(appName).ApplicationTypeName;
    FABRICSESSION.FabricDispatcher.ApplicationMap.RollbackUpgrade(
        appName, 
        ApplicationBuilder::GetApplicationBuilderName(appTypeName, targetVersion));

    CheckUpgradeAppComplete(
        appName, 
        appTypeName, 
        targetVersion, 
        previousAppBuilderName,
        ApplicationUpgradeContext::Create(
            expectedEvaluationReason, 
            expectedFailureReason,
            expectedFailureProgress));
    
    return true;
}

bool TestFabricClientUpgrade::UpgradeFabric(StringCollection const & params)
{
    CHECK_PARAMS(1, 999, FabricTestCommands::UpgradeFabricCommand);

    const wchar_t* codeVersionPtr = NULL;
    const wchar_t* configVersionPtr = NULL;

    CommandLineParser parser(params, 0);
    wstring codeVersion;
    wstring configVersion;
    int numOfCodeConfig = 0;
    if (parser.TryGetString(L"code", codeVersion))
    {
        codeVersionPtr = codeVersion.c_str();
        numOfCodeConfig++;
    }
    if (parser.TryGetString(L"config", configVersion))
    {
        configVersionPtr = configVersion.c_str();
        numOfCodeConfig++;
    }
    if (numOfCodeConfig == 0)
    {
        TestSession::WriteError(TraceSource, "Must provide either code or config");
        return false;
    }

    FABRIC_UPGRADE_KIND upgradeKind = FABRIC_UPGRADE_KIND_INVALID;
    if (params[numOfCodeConfig] == L"Rolling")
    {
        upgradeKind = FABRIC_UPGRADE_KIND_ROLLING;
    }

    int64 timeoutInSeconds;
    parser.TryGetInt64(L"timeout", timeoutInSeconds, FabricTestSessionConfig::GetConfig().NamingOperationTimeout.TotalSeconds());
    TimeSpan timeout = TimeSpan::FromSeconds(static_cast<double>(timeoutInSeconds));

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    bool startUpgradeCheck = parser.GetBool(L"startupgradecheck");

    bool enableDeltaHealthEvaluation = parser.GetBool(L"enabledeltas");
        
    FABRIC_UPGRADE_DESCRIPTION upgradeDescription = { 0 };
    upgradeDescription.CodeVersion = codeVersionPtr;
    upgradeDescription.ConfigVersion = configVersionPtr;
    upgradeDescription.UpgradeKind = upgradeKind;
    
    ScopedHeap heap;
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION policyDescription;
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1 policyDescriptionEx1 = { 0 };
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY monitoringPolicy;
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1 monitoringPolicyEx1;
    FABRIC_CLUSTER_HEALTH_POLICY healthPolicy = { 0 };
    FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY upgradeHealthPolicy = { 0 };
    FABRIC_APPLICATION_HEALTH_POLICY_MAP applicationHealthPolicyMap = { 0 };
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX2 policyDescriptionEx2 = { 0 };
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX3 policyDescriptionEx3 = { 0 };
    
    memset(&policyDescription, 0, sizeof(policyDescription));
    memset(&monitoringPolicy, 0, sizeof(monitoringPolicy));

    policyDescription.Reserved = &policyDescriptionEx1;
    policyDescriptionEx1.MonitoringPolicy = &monitoringPolicy;
    monitoringPolicy.Reserved = &monitoringPolicyEx1;

    policyDescriptionEx2.EnableDeltaHealthEvaluation = enableDeltaHealthEvaluation ? TRUE : FALSE;
    policyDescriptionEx1.Reserved = &policyDescriptionEx2;

    policyDescriptionEx3.ApplicationHealthPolicyMap = &applicationHealthPolicyMap;
    policyDescriptionEx2.Reserved = &policyDescriptionEx3;
    
    if (upgradeKind == FABRIC_UPGRADE_KIND_ROLLING)
    {
        this->GetRollingUpgradePolicyDescriptionForUpgrade(
            parser,
            policyDescription);

        wstring healthPolicyStr;
        parser.TryGetString(L"jsonpolicy", healthPolicyStr, L"");

        wstring upgradeHealthPolicyStr;
        parser.TryGetString(L"jsonupgradepolicy", upgradeHealthPolicyStr, L"");

        if (healthPolicyStr.empty())
        {
            policyDescriptionEx1.HealthPolicy = NULL;
        }
        else
        {
            ClusterHealthPolicy parsedHealthPolicy;
            ClusterHealthPolicy::FromString(healthPolicyStr, parsedHealthPolicy);

            parsedHealthPolicy.ToPublicApi(heap, healthPolicy);

            policyDescriptionEx1.HealthPolicy = &healthPolicy;
        }

        if (upgradeHealthPolicyStr.empty())
        {
            policyDescriptionEx2.UpgradeHealthPolicy = NULL;
        }
        else
        {
            ClusterUpgradeHealthPolicy parsedUpgradeHealthPolicy;
            ClusterUpgradeHealthPolicy::FromString(upgradeHealthPolicyStr, parsedUpgradeHealthPolicy);

            parsedUpgradeHealthPolicy.ToPublicApi(heap, upgradeHealthPolicy);

            policyDescriptionEx2.UpgradeHealthPolicy = &upgradeHealthPolicy;
        }

        upgradeDescription.UpgradePolicyDescription = &policyDescription;

        ApplicationHealthPolicyMapSPtr applicationHealthPolicies;
        if (!TestFabricClientHealth::GetApplicationHealthPolicies(parser, applicationHealthPolicies))
        {
            return false;
        }

        if (applicationHealthPolicies)
        {
            applicationHealthPolicies->ToPublicApi(heap, applicationHealthPolicyMap);
        }
    }
    else
    {
        upgradeDescription.UpgradePolicyDescription = NULL;
    }

    bool forceRestart = (policyDescription.ForceRestart == TRUE);

    FABRICSESSION.FabricDispatcher.UpgradeFabricData.InterruptUpgrade();
    UpgradeFabricImpl(upgradeDescription, timeout, expectedError);

    if (expectedError == S_OK || startUpgradeCheck)
    {           
        FABRICSESSION.FabricDispatcher.UpgradeFabricData.StartUpgradeFabric(codeVersion, configVersion, forceRestart);
        CheckUpgradeFabricComplete(codeVersion, configVersion, FabricUpgradeContext::Create());
    }
    return true;
}

bool TestFabricClientUpgrade::UpdateFabricUpgrade(StringCollection const & params)
{
    CHECK_PARAMS(1, 999, FabricTestCommands::UpdateFabricUpgradeCommand);

    CommandLineParser parser(params, 0);

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();
        
    ScopedHeap heap;

    FABRIC_UPGRADE_UPDATE_DESCRIPTION updateDescription;
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION policyDescription;
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1 policyDescriptionEx1 = {0};
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY monitoringPolicy;
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1 monitoringPolicyEx1 = {0};
    FABRIC_CLUSTER_HEALTH_POLICY healthPolicy = {0};
    FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY upgradeHealthPolicy = { 0 };
    FABRIC_APPLICATION_HEALTH_POLICY_MAP applicationHealthPolicyMap = { 0 };
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX2 policyDescriptionEx2 = { 0 };
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX3 policyDescriptionEx3 = { 0 };

    memset(&updateDescription, 0, sizeof(updateDescription));
    memset(&policyDescription, 0, sizeof(policyDescription));
    memset(&monitoringPolicy, 0, sizeof(monitoringPolicy));

    updateDescription.UpgradeKind = FABRIC_UPGRADE_KIND_ROLLING;

    updateDescription.UpgradePolicyDescription = &policyDescription;
    policyDescription.Reserved = &policyDescriptionEx1;
    policyDescriptionEx1.MonitoringPolicy = &monitoringPolicy;
    policyDescriptionEx1.HealthPolicy = &healthPolicy;
    monitoringPolicy.Reserved = &monitoringPolicyEx1;

    policyDescriptionEx2.UpgradeHealthPolicy = &upgradeHealthPolicy;
    policyDescriptionEx1.Reserved = &policyDescriptionEx2;

    policyDescriptionEx3.ApplicationHealthPolicyMap = &applicationHealthPolicyMap;
    policyDescriptionEx2.Reserved = &policyDescriptionEx3;

    this->GetRollingUpgradePolicyDescriptionForUpdate(
        parser,
        updateDescription.UpdateFlags,
        policyDescription);

    wstring healthPolicyStr;
    if (parser.TryGetString(L"jsonpolicy", healthPolicyStr, L""))
    {
        updateDescription.UpdateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_POLICY;

        ClusterHealthPolicy parsedHealthPolicy;
        ClusterHealthPolicy::FromString(healthPolicyStr, parsedHealthPolicy);

        parsedHealthPolicy.ToPublicApi(heap, healthPolicy);
    }

    wstring upgradeHealthPolicyStr;
    if (parser.TryGetString(L"jsonupgradepolicy", upgradeHealthPolicyStr, L""))
    {
        updateDescription.UpdateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_HEALTH_POLICY;

        ClusterUpgradeHealthPolicy parsedUpgradeHealthPolicy;
        ClusterUpgradeHealthPolicy::FromString(upgradeHealthPolicyStr, parsedUpgradeHealthPolicy);

        parsedUpgradeHealthPolicy.ToPublicApi(heap, upgradeHealthPolicy);
    }

    wstring enableDeltaString;
    if (parser.TryGetString(L"enabledeltas", enableDeltaString))
    {
        updateDescription.UpdateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_ENABLE_DELTAS;
        bool enableDeltaHealthEvaluation = parser.GetBool(L"enabledeltas");
        policyDescriptionEx2.EnableDeltaHealthEvaluation = enableDeltaHealthEvaluation ? TRUE : FALSE;
    }

    ApplicationHealthPolicyMapSPtr applicationHealthPolicies;
    if (!TestFabricClientHealth::GetApplicationHealthPolicies(parser, applicationHealthPolicies))
    {
        return false;
    }

    if (applicationHealthPolicies)
    {
        updateDescription.UpdateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_APPLICATION_HEALTH_POLICY_MAP;

        applicationHealthPolicies->ToPublicApi(heap, applicationHealthPolicyMap);
    }

    UpdateFabricUpgradeImpl(updateDescription, expectedError);

    return true;
}

bool TestFabricClientUpgrade::VerifyFabricUpgradeDescription(StringCollection const & params)
{
    CHECK_PARAMS(1, 999, FabricTestCommands::VerifyFabricUpgradeDescriptionCommand);

    CommandLineParser parser(params, 1);

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    ComPointer<IFabricUpgradeProgressResult3> upgradeProgress;
    UpgradeFabricStatusImpl(expectedError, FABRIC_E_TIMEOUT, upgradeProgress);

    if (expectedError != S_OK)
    {
        return true;
    }

    FABRIC_UPGRADE_PROGRESS const * progressDescription = upgradeProgress->get_UpgradeProgress();
    TestSession::FailTestIf(progressDescription == NULL, "get_UpgradeProgress() is NULL");

    FABRIC_UPGRADE_DESCRIPTION const * upgradeDescription = progressDescription->UpgradeDescription;
    TestSession::FailTestIf(upgradeDescription == NULL, "UpgradeDescription is NULL");
    TestSession::FailTestIf(upgradeDescription->UpgradePolicyDescription == NULL, "UpgradePolicyDescription is NULL");

    Management::ClusterManager::FabricUpgradeDescription internalDescription;
    auto error = internalDescription.FromPublicApi(*upgradeDescription);
    TestSession::FailTestIfNot(error.IsSuccess(), "FabricUpgradeDescription::FromPublicApi error={0}", error);

    wstring codeversion;
    if (parser.TryGetString(L"code", codeversion, L""))
    {
        auto expected = codeversion;

        auto actual = wstring(upgradeDescription->CodeVersion);

        TestSession::FailTestIfNot(
            expected == actual, 
            "CodeVersion mismatch: expected={0} actual={1}",
            expected,
            actual);
    }

    wstring configversion;
    if (parser.TryGetString(L"config", configversion, L""))
    {
        auto expected = configversion;

        auto actual = wstring(upgradeDescription->ConfigVersion);

        TestSession::FailTestIfNot(
            expected == actual, 
            "ConfigVersion mismatch: expected={0} actual={1}",
            expected,
            actual);
    }

    auto const * policyDescription = static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION*>(
        upgradeDescription->UpgradePolicyDescription);

    this->VerifyRollingUpgradePolicyDescription(
        parser,
        *policyDescription);

    auto const * policyDescriptionEx1Ptr = policyDescription->Reserved == NULL 
        ? NULL
        : static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*>(policyDescription->Reserved);

    wstring healthPolicyStr;
    if (parser.TryGetString(L"jsonpolicy", healthPolicyStr, L""))
    {
        TestSession::FailTestIf(policyDescriptionEx1Ptr == NULL, "Reserved is NULL");

        TestSession::FailTestIf(policyDescriptionEx1Ptr->HealthPolicy == NULL, "HealthPolicy is NULL");
        auto const * healthPolicy = static_cast<FABRIC_CLUSTER_HEALTH_POLICY*>(
            policyDescriptionEx1Ptr->HealthPolicy);

        ClusterHealthPolicy expected;
        ClusterHealthPolicy::FromString(healthPolicyStr, expected);

        ClusterHealthPolicy actual;
        error = actual.FromPublicApi(*healthPolicy);

        TestSession::FailTestIfNot(error.IsSuccess(), "ClusterHealthPolicy::FromPublicApi error={0}", error);

        TestSession::FailTestIfNot(
            expected == actual, 
            "HealthPolicy mismatch: expected={0} actual={1}",
            expected,
            actual);
    }

    auto const * policyDescriptionEx2Ptr =  (policyDescriptionEx1Ptr == nullptr || policyDescriptionEx1Ptr->Reserved == NULL)
        ? NULL
        : static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX2*>(policyDescriptionEx1Ptr->Reserved);

    bool expectedEnableDeltas = parser.GetBool(L"enabledeltas");
    if (policyDescriptionEx2Ptr == NULL)
    {
        TestSession::FailTestIf(expectedEnableDeltas, "EnableDeltaHealthEvaluation mismatch: expected={0} actual=NULL", expectedEnableDeltas);
    }
    else
    {
        auto actualEnableDeltas = (policyDescriptionEx2Ptr->EnableDeltaHealthEvaluation == TRUE);
        TestSession::FailTestIfNot(expectedEnableDeltas == actualEnableDeltas, "EnableDeltaHealthEvaluation mismatch: expected={0} actual={1}", expectedEnableDeltas, actualEnableDeltas);
    }

    wstring updateHealthPolicyStr;
    if (parser.TryGetString(L"jsonupdatepolicy", updateHealthPolicyStr, L""))
    {
        TestSession::FailTestIf(policyDescriptionEx2Ptr == NULL, "Reserved is NULL");

        TestSession::FailTestIf(policyDescriptionEx2Ptr->UpgradeHealthPolicy == NULL, "HealthPolicy is NULL");
        auto const * upgradeHealthPolicy = static_cast<FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY*>(
            policyDescriptionEx2Ptr->UpgradeHealthPolicy);

        ClusterUpgradeHealthPolicy expected;
        ClusterUpgradeHealthPolicy::FromString(updateHealthPolicyStr, expected);

        ClusterUpgradeHealthPolicy actual;
        error = actual.FromPublicApi(*upgradeHealthPolicy);

        TestSession::FailTestIfNot(error.IsSuccess(), "ClusterUpgradeHealthPolicy::FromPublicApi error={0}", error);

        TestSession::FailTestIfNot(
            expected == actual, 
            "UpgradeHealthPolicy mismatch: expected={0} actual={1}",
            expected,
            actual);
    }

    auto const * policyDescriptionEx3Ptr = (policyDescriptionEx2Ptr == nullptr || policyDescriptionEx2Ptr->Reserved == NULL)
        ? NULL
        : static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX3*>(policyDescriptionEx2Ptr->Reserved);

    ApplicationHealthPolicyMapSPtr expectedApplicationHealthPolicies;
    if (!TestFabricClientHealth::GetApplicationHealthPolicies(parser, expectedApplicationHealthPolicies))
    {
        return false;
    }

    bool expectsApplicationHealthPolicies = expectedApplicationHealthPolicies && expectedApplicationHealthPolicies->size() > 0;
    if (policyDescriptionEx3Ptr == NULL)
    {
        if (expectsApplicationHealthPolicies)
        {
            TestSession::FailTest("applicationHealthPolicies mismatch: expected={0} items, actual=NULL", expectedApplicationHealthPolicies->size());
        }
    }
    else if (expectsApplicationHealthPolicies)
    {
        TestSession::FailTestIf(policyDescriptionEx3Ptr->ApplicationHealthPolicyMap == NULL, "ApplicationHealthPolicyMap is NULL");
        ApplicationHealthPolicyMap actual;
        error = actual.FromPublicApi(*policyDescriptionEx3Ptr->ApplicationHealthPolicyMap);
        TestSession::FailTestIfNot(error.IsSuccess(), "ApplicationHealthPolicyMap::FromPublicApi error={0}", error);

        TestSession::FailTestIfNot(
            *expectedApplicationHealthPolicies == actual,
            "ApplicationHealthPolicyMap mismatch: expected={0} actual={1}",
            expectedApplicationHealthPolicies->ToString(),
            actual.ToString());
    }

    return true;
}

DWORD GetDWORDTimeoutInSeconds(int64 intValue)
{
    if (intValue == -1)
    {
        return static_cast<DWORD>(-1);
    }
    else
    {
        DWORD expected;
        auto hr = Int64ToDWord(intValue, &expected);
        TestSession::FailTestIfNot(SUCCEEDED(hr), "Could not convert timeout from int64 to DWORD. hr = {0}", hr);
        return expected;
    }
}

bool CheckDWORDTimeoutInSeconds(int64 expectedInt, DWORD actual, std::wstring const & parameterName)
{
    DWORD expected = GetDWORDTimeoutInSeconds(expectedInt);
    if (expected != actual)
    {
        TestSession::WriteWarning(
            TraceSource,
            "{0} mismatch: expected={1} actual={2}",
            parameterName,
            expected,
            actual);
        return false;
    }

    return true;
}

void TestFabricClientUpgrade::VerifyRollingUpgradePolicyDescription(
    __in CommandLineParser & parser,
    __in FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION const & policyDescription)
{
    wstring upgradeModeString;
    if (parser.TryGetString(L"upgrademode", upgradeModeString, L""))
    {
        auto expected = FABRIC_ROLLING_UPGRADE_MODE_INVALID;

        if (upgradeModeString == L"manual")
        {
            expected = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL;
        }
        else if (upgradeModeString == L"monitored")
        {
            expected = FABRIC_ROLLING_UPGRADE_MODE_MONITORED;
        }
        else if (upgradeModeString == L"auto")
        {
            expected = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO;
        }

        auto actual = policyDescription.RollingUpgradeMode;

        TestSession::FailTestIfNot(
            expected == actual, 
            "RollingUpgradeMode mismatch: expected={0} actual={1}",
            static_cast<int>(expected),
            static_cast<int>(actual));
    }

    wstring forceRestartString;
    if (parser.TryGetString(L"restart", forceRestartString, L""))
    {
        auto expected = parser.GetBool(L"restart") ? TRUE : FALSE;

        auto actual = policyDescription.ForceRestart;

        TestSession::FailTestIfNot(
            expected == actual, 
            "ForceRestart mismatch: expected={0} actual={1}",
            expected,
            actual);
    }

    int64 replicaSetCheckTimeoutInSeconds;
    if (parser.TryGetInt64(L"replicacheck", replicaSetCheckTimeoutInSeconds, -1))
    {
        auto actual = policyDescription.UpgradeReplicaSetCheckTimeoutInSeconds;

        TestSession::FailTestIfNot(
            CheckDWORDTimeoutInSeconds(replicaSetCheckTimeoutInSeconds, actual, L"UpgradeReplicaSetCheckTimeoutInSeconds"),
            "UpgradeReplicaSetCheckTimeoutInSeconds mismatch");
    }

    if (policyDescription.RollingUpgradeMode != FABRIC_ROLLING_UPGRADE_MODE_MONITORED)
    {
        TestSession::FailTestIfNot(policyDescription.Reserved == NULL, "Reserved is not NULL");

        // skip remainder of validation, which only applies to monitored upgrades

        return;
    } 

    TestSession::FailTestIf(policyDescription.Reserved == NULL, "Reserved is NULL");

    auto const * policyDescriptionEx1Ptr = static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*>(
        policyDescription.Reserved);

    TestSession::FailTestIf(policyDescriptionEx1Ptr->MonitoringPolicy == NULL, "MonitoringPolicy is NULL");

    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY const * monitoringPolicyPtr = policyDescriptionEx1Ptr->MonitoringPolicy;

    wstring failActionString;
    if (parser.TryGetString(L"failaction", failActionString, L"invalid"))
    {
        auto expected = FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_INVALID;

        if (failActionString == L"rollback")
        {
            expected = FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_ROLLBACK;
        }
        else if (failActionString == L"manual")
        {
            expected = FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_MANUAL;
        }

        auto actual = monitoringPolicyPtr->FailureAction;

        TestSession::FailTestIfNot(
            expected == actual, 
            "FailureAction mismatch: expected={0} actual={1}",
            static_cast<int>(expected),
            static_cast<int>(actual));
    }

    int64 healthCheckWaitInSeconds;
    if (parser.TryGetInt64(L"healthcheckwait", healthCheckWaitInSeconds, -1))
    {
        auto actual = monitoringPolicyPtr->HealthCheckWaitDurationInSeconds;
        TestSession::FailTestIfNot(
            CheckDWORDTimeoutInSeconds(healthCheckWaitInSeconds, actual, L"HealthCheckWaitDurationInSeconds"),
            "HealthCheckWaitDurationInSeconds mismatch");
    }

    int64 healthCheckStableInSeconds;
    if (parser.TryGetInt64(L"healthcheckstable", healthCheckStableInSeconds, -1))
    {
        auto monitoringPolicyEx1Ptr = static_cast<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1*>(monitoringPolicyPtr->Reserved);
        auto actual = monitoringPolicyEx1Ptr->HealthCheckStableDurationInSeconds;
        TestSession::FailTestIfNot(
            CheckDWORDTimeoutInSeconds(healthCheckStableInSeconds, actual, L"HealthCheckStableDurationInSeconds"),
            "HealthCheckStableDurationInSeconds mismatch");
    }

    int64 healthCheckRetryInSeconds;
    if (parser.TryGetInt64(L"healthcheckretry", healthCheckRetryInSeconds, HealthCheckRetryDefault))
    {
        auto actual = monitoringPolicyPtr->HealthCheckRetryTimeoutInSeconds;
        TestSession::FailTestIfNot(
            CheckDWORDTimeoutInSeconds(healthCheckRetryInSeconds, actual, L"HealthCheckRetryTimeoutInSeconds"),
            "HealthCheckRetryTimeoutInSeconds mismatch");
    }

    int64 upgradeTimeoutInSeconds;
    if (parser.TryGetInt64(L"upgradetimeout", upgradeTimeoutInSeconds, -1))
    {
        auto actual = monitoringPolicyPtr->UpgradeTimeoutInSeconds;
        TestSession::FailTestIfNot(
            CheckDWORDTimeoutInSeconds(upgradeTimeoutInSeconds, actual, L"UpgradeTimeoutInSeconds"),
            "UpgradeTimeoutInSeconds mismatch");
    }

    int64 udTimeoutInSeconds;
    if (parser.TryGetInt64(L"udtimeout", udTimeoutInSeconds, -1))
    {
        auto actual = monitoringPolicyPtr->UpgradeDomainTimeoutInSeconds;

        TestSession::FailTestIfNot(
            CheckDWORDTimeoutInSeconds(udTimeoutInSeconds, actual, L"UpgradeDomainTimeoutInSeconds"),
            "UpgradeDomainTimeoutInSeconds mismatch");
    }

    bool xpolicy = parser.GetBool(L"xpolicy");
    if (xpolicy)
    {
        TestSession::FailTestIfNot(policyDescriptionEx1Ptr->HealthPolicy == NULL, "HealthPolicy is not NULL");
    }

    bool xupgradepolicy = parser.GetBool(L"xupgradepolicy");
    if (xupgradepolicy)
    {
        auto const * policyDescriptionEx2Ptr = static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX2*>(
            policyDescriptionEx1Ptr->Reserved);

        TestSession::FailTestIfNot(policyDescriptionEx2Ptr->UpgradeHealthPolicy == NULL, "UpgradeHealthPolicy is not NULL");
    }
}

void TestFabricClientUpgrade::GetRollingUpgradePolicyDescriptionForUpgrade(
    __in CommandLineParser & parser,
    __inout FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION & policyDescription)
{
    wstring upgradeModeString;
    parser.TryGetString(L"upgrademode", upgradeModeString, L"auto");

    if (upgradeModeString == L"manual")
    {
        policyDescription.RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL;
    }
    else if (upgradeModeString == L"monitored")
    {
        policyDescription.RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_MONITORED;
    }
    else if (upgradeModeString == L"auto")
    {
        policyDescription.RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO;
    }
    else
    {
        policyDescription.RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_INVALID;
    }

    bool forceRestart = parser.GetBool(L"restart");
    policyDescription.ForceRestart = forceRestart ? TRUE : FALSE;

    int64 replicaSetCheckTimeoutInSeconds;
    if (parser.TryGetInt64(L"replicacheck", replicaSetCheckTimeoutInSeconds, -1))
    {
        DWORD replicaSetCheckTimeout = GetDWORDTimeoutInSeconds(replicaSetCheckTimeoutInSeconds);
        policyDescription.UpgradeReplicaSetCheckTimeoutInSeconds = replicaSetCheckTimeout;
    }

    bool defaultPolicy = parser.GetBool(L"xpolicy");

    if (!defaultPolicy)
    {
        FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1 * policyDescriptionEx1Ptr = static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*>(const_cast<void*>(policyDescription.Reserved));
        FABRIC_ROLLING_UPGRADE_MONITORING_POLICY * monitoringPolicyPtr = const_cast<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY*>(policyDescriptionEx1Ptr->MonitoringPolicy);
        FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1 * monitoringPolicyEx1Ptr = static_cast<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1*>(const_cast<void*>(monitoringPolicyPtr->Reserved));

        int64 timeoutInSeconds;
        parser.TryGetInt64(L"timeout", timeoutInSeconds, FabricTestSessionConfig::GetConfig().NamingOperationTimeout.TotalSeconds());

        FABRIC_MONITORED_UPGRADE_FAILURE_ACTION failAction = FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_INVALID;
        wstring failActionString;
        parser.TryGetString(L"failaction", failActionString, L"invalid");
        if (failActionString == L"rollback")
        {
            failAction = FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_ROLLBACK;
        }
        else if (failActionString == L"manual")
        {
            failAction = FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_MANUAL;
        }
        monitoringPolicyPtr->FailureAction = failAction;

        int64 healthCheckWaitInSeconds;
        parser.TryGetInt64(L"healthcheckwait", healthCheckWaitInSeconds, 0);

        DWORD healthCheckWaitDuration = GetDWORDTimeoutInSeconds(healthCheckWaitInSeconds);
        monitoringPolicyPtr->HealthCheckWaitDurationInSeconds = healthCheckWaitDuration;

        int64 healthCheckStableInSeconds;
        parser.TryGetInt64(L"healthcheckstable", healthCheckStableInSeconds, HealthCheckStableDefault);

        DWORD healthCheckStableDuration = GetDWORDTimeoutInSeconds(healthCheckStableInSeconds);
        monitoringPolicyEx1Ptr->HealthCheckStableDurationInSeconds = healthCheckStableDuration;

        int64 healthCheckRetryInSeconds;
        parser.TryGetInt64(L"healthcheckretry", healthCheckRetryInSeconds, HealthCheckRetryDefault);

        DWORD healthCheckRetryTimeout = GetDWORDTimeoutInSeconds(healthCheckRetryInSeconds);
        monitoringPolicyPtr->HealthCheckRetryTimeoutInSeconds = healthCheckRetryTimeout;

        int64 upgradeTimeoutInSeconds;
        parser.TryGetInt64(L"upgradetimeout", upgradeTimeoutInSeconds, -1);

        DWORD upgradeTimeout = GetDWORDTimeoutInSeconds(upgradeTimeoutInSeconds);
        monitoringPolicyPtr->UpgradeTimeoutInSeconds = upgradeTimeout;

        int64 udTimeoutInSeconds;
        parser.TryGetInt64(L"udtimeout", udTimeoutInSeconds, -1);

        DWORD udTimeout = GetDWORDTimeoutInSeconds(udTimeoutInSeconds);
        monitoringPolicyPtr->UpgradeDomainTimeoutInSeconds = udTimeout;
    }
    else
    {
        policyDescription.Reserved = NULL;
    }
}

void TestFabricClientUpgrade::GetRollingUpgradePolicyDescriptionForUpdate(
    __in CommandLineParser & parser,
    __inout DWORD & updateFlags,
    __inout FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION & policyDescription)
{
    TestSession::FailTestIf(policyDescription.Reserved == NULL, "Reserved must be non-NULL");
    auto policyDescriptionEx1Ptr = static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*>(policyDescription.Reserved);

    TestSession::FailTestIf(policyDescriptionEx1Ptr->MonitoringPolicy == NULL, "MonitoringPolicy must be non-NULL");
    auto monitoringPolicyPtr = const_cast<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY*>(policyDescriptionEx1Ptr->MonitoringPolicy);

    wstring upgradeModeString;
    if (parser.TryGetString(L"upgrademode", upgradeModeString, L""))
    {
        updateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_MODE;

        if (upgradeModeString == L"manual")
        {
            policyDescription.RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL;
        }
        else if (upgradeModeString == L"monitored")
        {
            policyDescription.RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_MONITORED;
        }
        else if (upgradeModeString == L"auto")
        {
            policyDescription.RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO;
        }
    }

    wstring forceRestartString;
    if (parser.TryGetString(L"restart", forceRestartString, L""))
    {
        updateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FORCE_RESTART;

        bool forceRestart = parser.GetBool(L"restart");

        policyDescription.ForceRestart = (forceRestart ? TRUE : FALSE);
    }

    int64 replicaSetCheckTimeoutInSeconds;
    if (parser.TryGetInt64(L"replicacheck", replicaSetCheckTimeoutInSeconds, -1))
    {
        updateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_REPLICA_SET_CHECK_TIMEOUT;

        DWORD replicaSetCheckTimeout = GetDWORDTimeoutInSeconds(replicaSetCheckTimeoutInSeconds);
        policyDescription.UpgradeReplicaSetCheckTimeoutInSeconds = replicaSetCheckTimeout;
    }

    wstring failActionString;
    if (parser.TryGetString(L"failaction", failActionString, L"invalid"))
    {
        updateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FAILURE_ACTION;
        
        FABRIC_MONITORED_UPGRADE_FAILURE_ACTION failAction = FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_INVALID;
        if (failActionString == L"rollback")
        {
            failAction = FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_ROLLBACK;
        }
        else if (failActionString == L"manual")
        {
            failAction = FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_MANUAL;
        }

        monitoringPolicyPtr->FailureAction = failAction;
    }

    int64 healthCheckWaitInSeconds;
    if (parser.TryGetInt64(L"healthcheckwait", healthCheckWaitInSeconds, -1))
    {
        updateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_WAIT;

        DWORD healthCheckWaitDuration = GetDWORDTimeoutInSeconds(healthCheckWaitInSeconds);
        monitoringPolicyPtr->HealthCheckWaitDurationInSeconds = healthCheckWaitDuration;
    }

    int64 healthCheckStableInSeconds;
    if (parser.TryGetInt64(L"healthcheckstable", healthCheckStableInSeconds, -1))
    {
        updateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_STABLE;

        DWORD healthCheckStableDuration = GetDWORDTimeoutInSeconds(healthCheckStableInSeconds);

        auto monitoringPolicyEx1Ptr = static_cast<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1*>(monitoringPolicyPtr->Reserved);
        monitoringPolicyEx1Ptr->HealthCheckStableDurationInSeconds = healthCheckStableDuration;
    }

    int64 healthCheckRetryInSeconds;
    if (parser.TryGetInt64(L"healthcheckretry", healthCheckRetryInSeconds, HealthCheckRetryDefault))
    {
        updateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_RETRY;

        DWORD healthCheckRetryTimeout = GetDWORDTimeoutInSeconds(healthCheckRetryInSeconds);
        monitoringPolicyPtr->HealthCheckRetryTimeoutInSeconds = healthCheckRetryTimeout;
    }

    int64 upgradeTimeoutInSeconds;
    if (parser.TryGetInt64(L"upgradetimeout", upgradeTimeoutInSeconds, -1))
    {
        updateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_TIMEOUT;

        DWORD upgradeTimeout = GetDWORDTimeoutInSeconds(upgradeTimeoutInSeconds);
        monitoringPolicyPtr->UpgradeTimeoutInSeconds = upgradeTimeout;
    }

    int64 udTimeoutInSeconds;
    if (parser.TryGetInt64(L"udtimeout", udTimeoutInSeconds, -1))
    {
        updateFlags |= FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_DOMAIN_TIMEOUT;

        DWORD udTimeout = GetDWORDTimeoutInSeconds(udTimeoutInSeconds);
        monitoringPolicyPtr->UpgradeDomainTimeoutInSeconds = udTimeout;
    }

    bool xpolicy = parser.GetBool(L"xpolicy");
    if (xpolicy)
    {
        policyDescriptionEx1Ptr->HealthPolicy = NULL;
    }
}

bool TestFabricClientUpgrade::RollbackFabricUpgrade(Common::StringCollection const & params)
{
    CommandLineParser parser(params);

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    RollbackFabricUpgradeImpl(expectedError);

    return true;
}

bool TestFabricClientUpgrade::SetRollbackFabric(Common::StringCollection const & params)
{
    CHECK_PARAMS(1, 999, FabricTestCommands::SetRollbackFabricCommand);

    CommandLineParser parser(params, 0);                                                        
    wstring codeVersion;                                                                               
    wstring configVersion;                                                                               
    int numOfCodeConfig = 0;                                                                    
    if(parser.TryGetString(L"code", codeVersion))                                                      
    {                                                                                                                                                       
        numOfCodeConfig++ ;                                                                     
    }                                                                                           
    if(parser.TryGetString(L"config", configVersion))                                                    
    {                                                                                                                                                        
        numOfCodeConfig++ ;                                                                     
    }                                                                                           
    if (numOfCodeConfig==0)                                                                     
    {                                                                                           
        TestSession::WriteError(TraceSource, "Must provide either code or config");    
        return false;
    }       

    wstring expectedEvaluationReason;
    parser.TryGetString(L"expectedreason", expectedEvaluationReason);
    
    UpgradeFailureReason::Enum expectedFailureReason = ParseUpgradeFailureReason(parser);

    wstring jsonExpectedFailureProgress;
    parser.TryGetString(L"jsonfailureprogress", jsonExpectedFailureProgress, L"");

    UpgradeDomainProgress expectedFailureProgress;
    if (!jsonExpectedFailureProgress.empty())
    {
        UpgradeDomainProgress::FromString(jsonExpectedFailureProgress, expectedFailureProgress);
    }

    FABRICSESSION.FabricDispatcher.UpgradeFabricData.RollbackUpgrade(codeVersion, configVersion);
    CheckUpgradeFabricComplete(
        codeVersion, 
        configVersion, 
        FabricUpgradeContext::Create(
            expectedEvaluationReason,
            expectedFailureReason,
            expectedFailureProgress));

    return true;
}

void TestFabricClientUpgrade::CheckUpgradeAppComplete(
    wstring const& appName, 
    wstring const& appTypeName, 
    wstring const& targetVersion,
    wstring const& previousAppBuilderName,
    shared_ptr<ApplicationUpgradeContext> const & context = ApplicationUpgradeContext::Create())
{
    if(FABRICSESSION.FabricDispatcher.ApplicationMap.HasUpgradeBeenInterrupted(appName))
    {
        TestSession::WriteInfo(TraceSource, "Upgrade for {0} interrupted. Exiting CheckUpgradeAppComplete loop", appName);
        return;
    }

    bool isUpgradeComplete = false;

    wstring const & expectedHealthEvaluation = context->GetExpectedHealthEvaluation();
    HRESULT expectedCompletionError = context->GetExpectedCompletionError();
    auto expectedFailureReason = context->GetExpectedFailureReason();
    auto const & expectedFailureProgress = context->GetExpectedFailureProgress();

    ComPointer<IFabricApplicationUpgradeProgressResult3> previousProgress = context->GetPreviousUpgradeProgress();
    ComPointer<IFabricApplicationUpgradeProgressResult3> upgradeProgress;

    vector<HRESULT> expectedErrors;
    expectedErrors.push_back(S_OK);

    if (expectedCompletionError != S_OK)
    {
        expectedErrors.push_back(expectedCompletionError);
    }

    HRESULT hr = UpgradeAppStatusImpl(appName, expectedErrors, upgradeProgress);

    if (expectedCompletionError != S_OK && hr == expectedCompletionError)
    {
        TestSession::WriteInfo(TraceSource, "Upgrade progress returned expected error {0}. Stopping checks.", hr);

        return;
    }

    ULONG count;
    FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const * buffer;

    hr = upgradeProgress->GetUpgradeDomains(&count, &buffer);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "upgradeProgress->GetUpgradeDomains failed with {0}", hr);

    FABRICSESSION.FabricDispatcher.ApplicationMap.SetRollingUpgradeMode(
        appName,
        upgradeProgress->get_RollingUpgradeMode());

    FABRICSESSION.FabricDispatcher.ApplicationMap.SetUpgradeState(
        appName,
        upgradeProgress->get_UpgradeState());

    for (ULONG ix = 0; ix < count; ++ix)
    {
        FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const & status = *(buffer + ix);

        if (status.State == FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED)
        {
            FABRICSESSION.FabricDispatcher.ApplicationMap.AddCompletedUpgradeDomain(appName, wstring(status.Name));
        }
    }

    FABRIC_APPLICATION_UPGRADE_STATE upgradeState = upgradeProgress->get_UpgradeState();
    if(upgradeState == FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED)
    {
        TestSession::FailTestIfNot(
            FABRICSESSION.FabricDispatcher.ApplicationMap.IsUpgrading(appName), 
            "UpgradeState cannot be FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED when application is not upgrading");

        TestSession::WriteInfo(TraceSource, "Upgrade for {0} complete.", appName);

        // Different behaviors depending on EnableDefaultServicesUpgrade
        // True - Target manifest reflects new default services status
        // False - Do not support changing required services but adding new ones is supported. Removing does nothing.
        bool const isDefaultServicesUpgradeEnabled = Management::ManagementConfig::GetConfig().EnableDefaultServicesUpgrade;

        StringCollection defaultServices;
        ApplicationBuilder::GetDefaultServices(appTypeName, targetVersion, defaultServices);

        StringCollection defaultServiceNames;
        wstring slash(L"/");
        for (auto iter = defaultServices.begin(); iter != defaultServices.end(); iter++)
        { 
            wstring applicationName = appName;
            if(!Common::StringUtility::EndsWith(applicationName, slash))
            {
                applicationName.append(slash);
            }

            defaultServiceNames.push_back(applicationName.append(*iter));
        }

        vector<PartitionedServiceDescriptor> serviceDescriptors;
        TestFabricClientSPtr->GetPartitionedServiceDescriptors(defaultServiceNames, serviceDescriptors);
        for (auto iter = serviceDescriptors.begin(); iter != serviceDescriptors.end(); iter++)
        { 
            PartitionedServiceDescriptor serviceDescriptor = *iter;
            
            if (isDefaultServicesUpgradeEnabled || !FABRICSESSION.FabricDispatcher.ServiceMap.IsCreatedService(serviceDescriptor.Service.Name))
            {
                FABRICSESSION.FabricDispatcher.ServiceMap.AddPartitionedServiceDescriptor(serviceDescriptor.Service.Name, move(serviceDescriptor), true);
            }
        }

        if (isDefaultServicesUpgradeEnabled)
        {
            StringCollection currentDefaultServices;
            ApplicationBuilder::TryGetDefaultServices(
                previousAppBuilderName,
                currentDefaultServices);

            for (auto iter = currentDefaultServices.begin(); iter != currentDefaultServices.end(); ++iter)
            {
                auto findIter = find(defaultServices.begin(), defaultServices.end(), *iter);
                if (findIter == defaultServices.end())
                {
                    wstring applicationName = appName;
                    if(!Common::StringUtility::EndsWith(applicationName, slash))
                    {
                        applicationName.append(slash);
                    }

                    FABRICSESSION.FabricDispatcher.ServiceMap.MarkServiceAsDeleted(applicationName.append(*iter));
                }
            }
        }

        FABRICSESSION.FabricDispatcher.ApplicationMap.MarkUpgradeComplete(appName);

        isUpgradeComplete = true;
    }
    else if(upgradeState == FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_COMPLETED && 
        FABRICSESSION.FabricDispatcher.ApplicationMap.IsRollingback(appName))
    {
        TestSession::WriteInfo(TraceSource, "Rollback for {0} complete.", appName);

        FABRICSESSION.FabricDispatcher.ApplicationMap.MarkRollbackComplete(appName);

        isUpgradeComplete = true;
    }

    if (previousProgress.GetRawPointer() == NULL || previousProgress->get_UpgradeState() == upgradeProgress->get_UpgradeState())
    {
        PrintUpgradeChanges(previousProgress, upgradeProgress);
    }

    if (!isUpgradeComplete)
    {
        auto root = FABRICSESSION.CreateComponentRoot();
        auto nextContext = ApplicationUpgradeContext::Create(*context, upgradeProgress);
        Threadpool::Post([this, root, appName, appTypeName, targetVersion, nextContext, previousAppBuilderName]()
        {
            CheckUpgradeAppComplete(appName, appTypeName, targetVersion, previousAppBuilderName, nextContext);
        }, TimeSpan::FromMilliseconds(4000));
    } 
    else if (!expectedHealthEvaluation.empty())
    {
        auto progressDescription = upgradeProgress->get_UpgradeProgress();
        TestSession::FailTestIf(progressDescription == NULL, "get_UpgradeProgress() is NULL");
        
        TestSession::FailTestIfNot(
            TestFabricClientHealth::VerifyExpectedHealthEvaluations(progressDescription->UnhealthyEvaluations, expectedHealthEvaluation),
            "HealthEvaluation check failed, expected {0}",
            expectedHealthEvaluation);
    }
    else if (expectedFailureReason != UpgradeFailureReason::None)
    {
        FABRIC_APPLICATION_UPGRADE_PROGRESS const * progress = upgradeProgress->get_UpgradeProgress();
        TestSession::FailTestIf(progress == NULL, "IFabricApplicationUpgradeProgressResult3->get_UpgradeProgress() returned NULL");
        TestSession::FailTestIf(progress->Reserved == NULL, "FABRIC_APPLICATION_UPGRADE_PROGRESS->Reserved is NULL");

        FABRIC_APPLICATION_UPGRADE_PROGRESS_EX1 const * progressEx1 = (FABRIC_APPLICATION_UPGRADE_PROGRESS_EX1*)progress->Reserved;

        auto failureReason = UpgradeFailureReason::FromPublicApi(progressEx1->FailureReason);
        TestSession::FailTestIf(
            failureReason != expectedFailureReason, 
            "UpgradeFailureReason mismatch: expected={0} actual={1}", 
            expectedFailureReason,
            failureReason);

        DateTime startTime(progressEx1->StartTimestampUtc);
        TestSession::FailTestIf(startTime == DateTime::Zero, "StartTimestampUtc not set");

        DateTime failureTime(progressEx1->FailureTimestampUtc);
        TestSession::FailTestIf(failureTime == DateTime::Zero, "FailureTimestampUtc not set");

        if (!expectedFailureProgress.IsEmpty)
        {
            TestSession::FailTestIf(progressEx1->UpgradeDomainProgressAtFailure == NULL, "FABRIC_APPLICATION_UPGRADE_PROGRESS_EX1->UpgradeDomainProgressAtFailure is NULL");

            UpgradeDomainProgress failureProgress;
            auto error = failureProgress.FromPublicApi(*progressEx1->UpgradeDomainProgressAtFailure);
            TestSession::FailTestIf(!error.IsSuccess(), "UpgradeDomainProgress::FromPublicApi failed with {0}", error);

            TestSession::FailTestIf(
                !failureProgress.Equals(expectedFailureProgress, true), // ignoreDynamicContent=true
                "UpgradeDomainProgressAtFailure mismatch: expected={0} actual={1}", 
                expectedFailureProgress, 
                failureProgress);
        }
    }
}

void TestFabricClientUpgrade::CheckUpgradeFabricComplete(    
    wstring const& codeVersion, 
    wstring const& configVersion,
    FabricUpgradeContextSPtr const & upgradeContext)
{
    if(FABRICSESSION.FabricDispatcher.UpgradeFabricData.HasUpgradeBeenInterrupted())
    {
        TestSession::WriteInfo(TraceSource, "Upgrade for Fabric interupted. Exiting CheckUpgradeFabricComplete loop");
        return;
    }

    HRESULT hr;
    bool isUpgradeComplete = false;

    auto expectedHealthEvaluation = upgradeContext->GetExpectedHealthEvaluation();
    auto expectedFailureReason = upgradeContext->GetExpectedFailureReason();
    auto expectedFailureProgress = upgradeContext->GetExpectedFailureProgress();

    ComPointer<IFabricUpgradeProgressResult3> previousProgress = upgradeContext->GetPreviousUpgradeProgress();
    ComPointer<IFabricUpgradeProgressResult3> upgradeProgress;

    UpgradeFabricStatusImpl(S_OK, FABRIC_E_TIMEOUT, upgradeProgress);

    ULONG count;
    FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const * buffer;

    hr = upgradeProgress->GetUpgradeDomains(&count, &buffer);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "upgradeProgress->GetUpgradeDomains failed with {0}", hr);

    FABRICSESSION.FabricDispatcher.UpgradeFabricData.SetRollingUpgradeMode(
        upgradeProgress->get_RollingUpgradeMode());

    FABRICSESSION.FabricDispatcher.UpgradeFabricData.SetUpgradeState(
        upgradeProgress->get_UpgradeState());

    for (ULONG ix = 0; ix < count; ++ix)
    {
        FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const & status = *(buffer + ix);

        if (status.State == FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED)
        {
            FABRICSESSION.FabricDispatcher.UpgradeFabricData.AddCompletedUpgradeDomain(wstring(status.Name));            
        }
    }

    if (previousProgress.GetRawPointer() == NULL || previousProgress->get_UpgradeState() == upgradeProgress->get_UpgradeState())
    {
        PrintUpgradeChanges(previousProgress, upgradeProgress);
    }

    FABRIC_UPGRADE_STATE upgradeState = upgradeProgress->get_UpgradeState();
    if(upgradeState == FABRIC_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED)
    {           
        TestSession::FailTestIfNot(
            FABRICSESSION.FabricDispatcher.UpgradeFabricData.IsUpgrading(), 
            "UpgradeState cannot be FABRIC_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED when application is not upgrading");

        TestSession::WriteInfo(TraceSource, "Upgrade for Fabric complete.");       
        FABRICSESSION.FabricDispatcher.UpgradeFabricData.MarkUpgradeComplete();
        isUpgradeComplete = true;
    }
    else if(upgradeState == FABRIC_UPGRADE_STATE_ROLLING_BACK_COMPLETED && 
        FABRICSESSION.FabricDispatcher.UpgradeFabricData.IsRollingback())
    {
        TestSession::WriteInfo(TraceSource, "Rollback for Fabric complete.");
        FABRICSESSION.FabricDispatcher.UpgradeFabricData.MarkRollbackComplete();
        isUpgradeComplete = true;
    }

    if (!isUpgradeComplete)
    {
        auto root = FABRICSESSION.CreateComponentRoot();
        Threadpool::Post([this, root, codeVersion, configVersion, upgradeContext, upgradeProgress]()
        {
            CheckUpgradeFabricComplete(
                codeVersion, 
                configVersion, 
                FabricUpgradeContext::Create(
                    *upgradeContext,
                    upgradeProgress));
        }, TimeSpan::FromMilliseconds(4000));
    }
    else if (!expectedHealthEvaluation.empty())
    {
        auto progressDescription = upgradeProgress->get_UpgradeProgress();
        TestSession::FailTestIf(progressDescription == NULL, "get_UpgradeProgress() is NULL");
        TestSession::FailTestIfNot(
            TestFabricClientHealth::VerifyExpectedHealthEvaluations(progressDescription->UnhealthyEvaluations, expectedHealthEvaluation),
            "HealthEvaluation check failed, expected {0}",
            expectedHealthEvaluation);
    }
    else if (expectedFailureReason != UpgradeFailureReason::None)
    {
        FABRIC_UPGRADE_PROGRESS const * progress = upgradeProgress->get_UpgradeProgress();
        TestSession::FailTestIf(progress == NULL, "IFabricUpgradeProgressResult3->get_UpgradeProgress() returned NULL");
        TestSession::FailTestIf(progress->Reserved == NULL, "FABRIC_UPGRADE_PROGRESS->Reserved is NULL");

        FABRIC_UPGRADE_PROGRESS_EX1 const * progressEx1 = (FABRIC_UPGRADE_PROGRESS_EX1*)progress->Reserved;

        auto failureReason = UpgradeFailureReason::FromPublicApi(progressEx1->FailureReason);
        TestSession::FailTestIf(
            failureReason != expectedFailureReason, 
            "UpgradeFailureReason mismatch: expected={0} actual={1}", 
            expectedFailureReason,
            failureReason);

        DateTime startTime(progressEx1->StartTimestampUtc);
        TestSession::FailTestIf(startTime == DateTime::Zero, "StartTimestampUtc not set");

        DateTime failureTime(progressEx1->FailureTimestampUtc);
        TestSession::FailTestIf(failureTime == DateTime::Zero, "FailureTimestampUtc not set");

        if (!expectedFailureProgress.IsEmpty)
        {
            TestSession::FailTestIf(progressEx1->UpgradeDomainProgressAtFailure == NULL, "FABRIC_UPGRADE_PROGRESS_EX1->UpgradeDomainProgressAtFailure is NULL");

            UpgradeDomainProgress failureProgress;
            auto error = failureProgress.FromPublicApi(*progressEx1->UpgradeDomainProgressAtFailure);
            TestSession::FailTestIf(!error.IsSuccess(), "UpgradeDomainProgress::FromPublicApi failed with {0}", error);

            TestSession::FailTestIf(
                !failureProgress.Equals(expectedFailureProgress, true), // ignoreDynamicContent=true
                "UpgradeDomainProgressAtFailure mismatch: expected={0} actual={1}", 
                expectedFailureProgress, 
                failureProgress);
        }
    }
}

bool TestFabricClientUpgrade::UpgradeAppStatus(StringCollection const & params)
{
    CHECK_PARAMS(1, 999,FabricTestCommands::UpgradeAppStatusCommand);
        
    wstring appName = params[0];
    CommandLineParser parser(params, 1);

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    bool printDetails = parser.GetBool(L"details");

    ComPointer<IFabricApplicationUpgradeProgressResult3> upgradeProgress;
    UpgradeAppStatusImpl(appName, expectedError, upgradeProgress);

    return PrintUpgradeProgress(printDetails, upgradeProgress);
}

bool TestFabricClientUpgrade::UpgradeFabricStatus(StringCollection const & params)
{
    CommandLineParser parser(params, 0);

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    bool printDetails = parser.GetBool(L"details");

    ComPointer<IFabricUpgradeProgressResult3> upgradeProgress;
    UpgradeFabricStatusImpl(expectedError, FABRIC_E_TIMEOUT, upgradeProgress);

    return PrintUpgradeProgress(printDetails, upgradeProgress);
}

bool TestFabricClientUpgrade::MoveNextApplicationUpgradeDomain(StringCollection const & params)
{
    CHECK_PARAMS(1, 4, FabricTestCommands::UpgradeAppMoveNext);
        
    wstring appName = params[0];
    CommandLineParser parser(params, 1);

    bool useOverloadApi = parser.GetBool(L"overload");           

    wstring ud;
    parser.TryGetString(L"ud", ud, L"");

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    if (useOverloadApi)
    {
        if (ud.empty())
        {
            MoveNextApplicationUpgradeDomainImpl2(appName, expectedError);
        }
        else
        {
            MoveNextApplicationUpgradeDomainImpl2(appName.c_str(), ud.c_str(), expectedError);
        }
    }
    else
    {
        MoveNextApplicationUpgradeDomainImpl(appName, expectedError);
    }

    return true;
}

bool TestFabricClientUpgrade::CreateComposeDeployment(StringCollection const & params)
{
    if (params.size() > 6)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::CreateComposeCommand);
        return false;
    }

    wstring deploymentName = params[0];

    CommandLineParser parser(params, 1);
    bool IsOverrideIB = parser.GetBool(L"isOverrideIB");
    bool isSkipSetIB = parser.GetBool(L"isSkipSetIB");

    if (!isSkipSetIB)
    {
        StringCollection mockImageBuilderProperties;
        wstring incomingBuildPath = Path::Combine(ApplicationBuilder::ApplicationPackageFolderInImageStore, params[1]);

        mockImageBuilderProperties.push_back(NamingUri(params[0]).ToString()); //appName
        mockImageBuilderProperties.push_back(incomingBuildPath);// appBuildPath
        mockImageBuilderProperties.push_back(params[2]);// appTypeName
        mockImageBuilderProperties.push_back(params[3]);// appTypeVersion
        if (IsOverrideIB)
        {
            FABRICSESSION.FabricDispatcher.EraseMockImageBuilderProperties(mockImageBuilderProperties[0]);
        }
        if (!FABRICSESSION.FabricDispatcher.SetMockImageBuilderProperties(mockImageBuilderProperties))
        {
            TestSession::WriteError(
                TraceSource,
                "Could not set mock ImageBuilder properties");
            return false;
        }
    }

    bool isVerify = parser.GetBool(L"verify");
    bool isAsyncVerify = parser.GetBool(L"asyncverify");

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    ScopedHeap heap;
    FABRIC_COMPOSE_DEPLOYMENT_DESCRIPTION composeDescription = { 0 };
    composeDescription.DeploymentName = heap.AddString(deploymentName);
    wstring pathToTemplate = Path::Combine(Environment::GetExecutablePath(), L"docker-compose.yml");
    composeDescription.ComposeFilePath = heap.AddString(pathToTemplate);

    this->CreateComposeDeploymentImpl(composeDescription, expectedError);

    if (expectedError == S_OK)
    {
        TestSession::WriteNoise(TraceSource, "Compose Deployment '{0}' has been created with {1}.", deploymentName, expectedError);

        shared_ptr<Management::ClusterManager::ClusterManagerReplica> cm = FABRICSESSION.FabricDispatcher.GetCM();
        TestSession::FailTestIfNot(cm != nullptr, "Could not get CM from FabricDispatcher.");

        if (isVerify)
        {
            int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
            Management::ClusterManager::ServiceModelApplicationId appId;
            wstring appTypeName;
            wstring appTypeVersion;
            bool found = false;
            while (remainingRetries-- >= 0 && !found)
            {
                auto errorCode = cm->Test_GetComposeDeployment(deploymentName, appId, appTypeName, appTypeVersion);
                if (errorCode.IsSuccess())
                {
                    vector<StoreDataComposeDeploymentInstance> composeDeploymentInstances;
                    errorCode = cm->Test_GetComposeDeploymentInstanceData(deploymentName, composeDeploymentInstances);
                    if (errorCode.IsSuccess())
                    {
                        for (auto it = composeDeploymentInstances.begin(); it != composeDeploymentInstances.end(); ++it)
                        {
                            if ((*it).Version.Value == appTypeVersion)
                            {
                                found = true;
                                break;
                            }
                        }
                    }
                }
                Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
            }

            wstring applicationName = this->GetApplicationNameFromDeploymentName(deploymentName);

            TestSession::FailTestIf(remainingRetries < 0, "Could not get compose deployment, all retries are exhausted");
            FABRICSESSION.FabricDispatcher.ApplicationMap.AddApplication(applicationName, appTypeName, ApplicationBuilder::GetApplicationBuilderName(appTypeName, appTypeVersion));
            FABRICSESSION.FabricDispatcher.ApplicationMap.UpdateApplicationId(applicationName, appId);

            StringCollection defaultServices;
            ApplicationBuilder::GetDefaultServices(appTypeName, appTypeVersion, defaultServices);
            StringCollection defaultServiceNames;
            wstring appNameWithSlash = applicationName;
            wstring slash(L"/");
            if (!Common::StringUtility::EndsWith(appNameWithSlash, slash))
            {
                appNameWithSlash.append(slash);
            }

            for (auto iter = defaultServices.begin(); iter != defaultServices.end(); iter++)
            {
                defaultServiceNames.push_back(appNameWithSlash + (*iter));
            }

            vector<PartitionedServiceDescriptor> serviceDescriptors;
            TestFabricClientSPtr->GetPartitionedServiceDescriptors(defaultServiceNames, serviceDescriptors);

            for (auto && serviceDescriptor : serviceDescriptors)
            {
                TestSession::FailTestIf(
                    FABRICSESSION.FabricDispatcher.ServiceMap.IsCreatedService(serviceDescriptor.Service.Name),
                    "Service with name {0} already exists", serviceDescriptor.Service.Name);

                FABRICSESSION.FabricDispatcher.ServiceMap.AddPartitionedServiceDescriptor(serviceDescriptor.Service.Name, move(serviceDescriptor));
            }
        }
        else if (isAsyncVerify)
        {
            Threadpool::Post([=]() {
                int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
                Management::ClusterManager::ServiceModelApplicationId appId;
                wstring appTypeName;
                wstring appTypeVersion;
                while (remainingRetries-- >= 0)
                {
                    auto errorCode = cm->Test_GetComposeDeployment(deploymentName, appId, appTypeName, appTypeVersion);
                    if (errorCode.IsSuccess())
                    {
                        break;
                    }
                    Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
                }

                wstring applicationName = this->GetApplicationNameFromDeploymentName(deploymentName);
                TestSession::FailTestIf(remainingRetries < 0, "Could not get compose application, all retries are exhausted");
                FABRICSESSION.FabricDispatcher.ApplicationMap.AddApplication(applicationName, appTypeName, ApplicationBuilder::GetApplicationBuilderName(appTypeName, appTypeVersion));
                FABRICSESSION.FabricDispatcher.ApplicationMap.UpdateApplicationId(applicationName, appId);

                StringCollection defaultServices;
                ApplicationBuilder::GetDefaultServices(appTypeName, appTypeVersion, defaultServices);
                StringCollection defaultServiceNames;
                wstring appNameWithSlash = applicationName;
                wstring slash(L"/");
                if (!Common::StringUtility::EndsWith(appNameWithSlash, slash))
                {
                    appNameWithSlash.append(slash);
                }

                for (auto iter = defaultServices.begin(); iter != defaultServices.end(); iter++)
                {
                    defaultServiceNames.push_back(appNameWithSlash + (*iter));
                }

                vector<PartitionedServiceDescriptor> serviceDescriptors;
                TestFabricClientSPtr->GetPartitionedServiceDescriptors(defaultServiceNames, serviceDescriptors);

                for (auto && serviceDescriptor : serviceDescriptors)
                {
                    TestSession::FailTestIf(
                        FABRICSESSION.FabricDispatcher.ServiceMap.IsCreatedService(serviceDescriptor.Service.Name),
                        "Service with name {0} already exists", serviceDescriptor.Service.Name);

                    FABRICSESSION.FabricDispatcher.ServiceMap.AddPartitionedServiceDescriptor(serviceDescriptor.Service.Name, move(serviceDescriptor));
                }
            });
        }
    }
    return true;
}

bool TestFabricClientUpgrade::DeleteComposeDeployment(StringCollection const & params)
{
    if (params.size() < 1 || params.size() > 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeleteComposeCommand);
        return false;
    }

    wstring deploymentName = params[0];

    CommandLineParser parser(params, 1);

    bool isVerify = parser.GetBool(L"verify");
    bool isNoApp = parser.GetBool(L"noapp");

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    DeleteComposeDeploymentImpl(deploymentName, expectedError);

    if (expectedError == S_OK)
    {
        if (isVerify)
        {
            shared_ptr<Management::ClusterManager::ClusterManagerReplica> cm = FABRICSESSION.FabricDispatcher.GetCM();
            TestSession::FailTestIfNot(cm != nullptr, "Could not get CM from FabricDispatcher.");

            Management::ClusterManager::ServiceModelApplicationId appId;
            wstring appTypeName;
            wstring appTypeVersion;

            int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
            while (remainingRetries -- >= 0)
            {
                auto errorCode = cm->Test_GetComposeDeployment(deploymentName, appId, appTypeName, appTypeVersion);
                if (errorCode.IsError(ErrorCodeValue::ComposeDeploymentNotFound))
                {
                    vector<StoreDataComposeDeploymentInstance> composeDeploymentInstances;
                    errorCode = cm->Test_GetComposeDeploymentInstanceData(deploymentName, composeDeploymentInstances);
                    TestSession::FailTestIfNot(
                        errorCode.IsSuccess(),
                        "Got unrecognized error when validating compose deployment deletion");

                    TestSession::FailTestIfNot(
                        composeDeploymentInstances.empty(),
                        "Found valid deployment instance objects after deleting compose deployment");

                    break;
                }
                Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
            }
            TestSession::FailTestIf(remainingRetries < 0, "Could not delete compose deployment, all retries are exhausted");
        }

        wstring applicationName = this->GetApplicationNameFromDeploymentName(deploymentName);
        if (!isNoApp)
        {
            FABRICSESSION.FabricDispatcher.ApplicationMap.DeleteApplication(applicationName);
            FABRICSESSION.FabricDispatcher.ServiceMap.MarkAppAsDeleted(applicationName);
        }

        FABRICSESSION.FabricDispatcher.EraseMockImageBuilderProperties(applicationName);
    }

    return true;
}

bool TestFabricClientUpgrade::UpgradeComposeDeployment(StringCollection const & params)
{
    CHECK_PARAMS(1, 999, FabricTestCommands::UpgradeComposeCommand);

    wstring deploymentName = params[0];
    wstring appName = this->GetApplicationNameFromDeploymentName(deploymentName);

    FABRIC_APPLICATION_UPGRADE_KIND upgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
 
    CommandLineParser parser(params, 2);
    wstring targetVersion;

    StringCollection mockImageBuilderProperties;
    wstring incomingBuildPath = Path::Combine(ApplicationBuilder::ApplicationPackageFolderInImageStore, params[1]);
    targetVersion = params[3];

    mockImageBuilderProperties.push_back(appName); //appName
    mockImageBuilderProperties.push_back(incomingBuildPath);// appBuildPath
    mockImageBuilderProperties.push_back(params[2]);// appTypeName
    mockImageBuilderProperties.push_back(targetVersion);// appTypeVersion

    // Erase current mock IB property so that new version could be inserted.
    if (!FABRICSESSION.FabricDispatcher.EraseMockImageBuilderProperties(appName))
    {
        TestSession::WriteError(
                TraceSource,
                "Could not erase mock ImageBuilder properties");
        return false;
    }

    if (!FABRICSESSION.FabricDispatcher.SetMockImageBuilderProperties(mockImageBuilderProperties))
    {
        TestSession::WriteError(
                TraceSource,
                "Could not set mock ImageBuilder properties");
        return false;
    }

    vector<wstring> errorStrings;
    parser.GetVector(L"errors", errorStrings, L"Success,ApplicationAlreadyInTargetVersion");
    vector<HRESULT> expectedErrors;
    for (auto it = errorStrings.begin(); it != errorStrings.end(); ++it)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(*it);
        expectedErrors.push_back(ErrorCode(error).ToHResult());
    }

    bool startUpgradeCheck = !parser.GetBool(L"expectUpgradeNotStart") && (find(expectedErrors.begin(), expectedErrors.end(), S_OK) != expectedErrors.end());

    wstring errorString;
    parser.TryGetString(L"upgradecheckerror", errorString, L"Success");
    ErrorCodeValue::Enum upgradeCheckError = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);

    ScopedHeap heap;
    FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION upgradeDescription = { 0 };
    upgradeDescription.DeploymentName = heap.AddString(deploymentName);

    auto pathList = heap.AddItem<FABRIC_STRING_LIST>();
    pathList->Count = static_cast<ULONG>(1);
    auto paths = heap.AddArray<LPCWSTR>(1);

    wstring pathToTemplate = Path::Combine(Environment::GetExecutablePath(), L"docker-compose.yml");
    paths[0] = heap.AddString(pathToTemplate);

    pathList->Items = paths.GetRawArray();
    upgradeDescription.ComposeFilePaths = *pathList;

    int64 timeoutInSeconds;
    parser.TryGetInt64(L"timeout", timeoutInSeconds, FabricTestSessionConfig::GetConfig().NamingOperationTimeout.TotalSeconds());
    TimeSpan timeout = TimeSpan::FromSeconds(static_cast<double>(timeoutInSeconds));
    upgradeDescription.UpgradeKind = upgradeKind;

    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION policyDescription;
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1 policyDescriptionEx1 = {0};
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY monitoringPolicy;
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1 monitoringPolicyEx1 = {0};
    FABRIC_APPLICATION_HEALTH_POLICY healthPolicy = {0};

    memset( &policyDescription, 0, sizeof(policyDescription) );
    memset( &monitoringPolicy, 0, sizeof(monitoringPolicy) );

    policyDescription.Reserved = &policyDescriptionEx1;
    policyDescriptionEx1.MonitoringPolicy = &monitoringPolicy;
    monitoringPolicy.Reserved = &monitoringPolicyEx1;

    this->GetRollingUpgradePolicyDescriptionForUpgrade(
        parser,
        policyDescription);

    wstring healthPolicyStr;
    parser.TryGetString(L"jsonpolicy", healthPolicyStr, L"");

    if (healthPolicyStr.empty())
    {
        policyDescriptionEx1.HealthPolicy = nullptr;
    }
    else
    {
        ApplicationHealthPolicy parsedHealthPolicy;
        ApplicationHealthPolicy::FromString(healthPolicyStr, parsedHealthPolicy);

        parsedHealthPolicy.ToPublicApi(heap, healthPolicy);

        policyDescriptionEx1.HealthPolicy = &healthPolicy;
    }

    upgradeDescription.UpgradePolicyDescription = &policyDescription;

    FABRICSESSION.FabricDispatcher.ApplicationMap.InterruptUpgrade(appName);

    wstring appTypeName = FABRICSESSION.FabricDispatcher.ApplicationMap.GetAppId(appName).ApplicationTypeName;
    wstring previousAppBuilderName = FABRICSESSION.FabricDispatcher.ApplicationMap.GetCurrentApplicationBuilderName(appName);
    
    bool forceRestart = (policyDescription.ForceRestart == TRUE);

    if (startUpgradeCheck)
    {
        FABRICSESSION.FabricDispatcher.ApplicationMap.StartUpgradeApplication(
            appName, 
            ApplicationBuilder::GetApplicationBuilderName(appTypeName, targetVersion),
            forceRestart);
    }

    UpgradeComposeDeploymentImpl(upgradeDescription, timeout, expectedErrors);

    bool isAsyncVerify = parser.GetBool(L"asyncverify");
    if (isAsyncVerify)
    {
        Threadpool::Post([=]() {
            shared_ptr<Management::ClusterManager::ClusterManagerReplica> cm = FABRICSESSION.FabricDispatcher.GetCM();
            TestSession::FailTestIfNot(cm != nullptr, "Could not get CM from FabricDispatcher.");
            int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
            while (remainingRetries-- >= 0)
            {
                ComposeDeploymentUpgradeState::Enum state;
                auto error = cm->Test_GetComposeDeploymentUpgradeState(
                    deploymentName,
                    state);
                if (error.IsSuccess() && state != ComposeDeploymentUpgradeState::Enum::ProvisioningTarget)
                {
                    break;
                }
                else
                {
                    TestSession::WriteInfo(
                        TraceSource,
                        "Provisioning Target not completed, state {0}, error {1}",
                        state,
                        error);
                }

                Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
            }

            if (startUpgradeCheck)
            {
                CheckUpgradeAppComplete(
                    appName,
                    appTypeName,
                    targetVersion,
                    previousAppBuilderName,
                    ApplicationUpgradeContext::Create(ErrorCode(upgradeCheckError).ToHResult()));
            }
        });
    }
    else
    {
        shared_ptr<Management::ClusterManager::ClusterManagerReplica> cm = FABRICSESSION.FabricDispatcher.GetCM();
        TestSession::FailTestIfNot(cm != nullptr, "Could not get CM from FabricDispatcher.");
        int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
        while (remainingRetries-- >= 0)
        {
            ComposeDeploymentUpgradeState::Enum state;
            auto error = cm->Test_GetComposeDeploymentUpgradeState(
                deploymentName,
                state);
            if (error.IsSuccess() && state != ComposeDeploymentUpgradeState::Enum::ProvisioningTarget)
            {
                break;
            }
            else
            {
                TestSession::WriteInfo(
                    TraceSource,
                    "Provisioning Target not completed, state {0}, error {1}",
                    state,
                    error);
            }

            Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
        }

        if (startUpgradeCheck)
        {
            CheckUpgradeAppComplete(
                appName,
                appTypeName,
                targetVersion,
                previousAppBuilderName,
                ApplicationUpgradeContext::Create(ErrorCode(upgradeCheckError).ToHResult()));
        }
    }
    return true;
}

bool TestFabricClientUpgrade::RollbackComposeDeployment(StringCollection const & params)
{
    if (params.size() < 1 || params.size() > 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RollbackComposeCommand);
        return false;
    }

    wstring deploymentName = params[0];
    CommandLineParser parser(params);

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    RollbackComposeDeploymentImpl(deploymentName, expectedError);

    return true;
}

bool TestFabricClientUpgrade::MoveNextFabricUpgradeDomain(StringCollection const & params)
{ 
    CHECK_MAX_PARAMS(3, FabricTestCommands::UpgradeFabricMoveNext);
        
    CommandLineParser parser(params, 0);

    bool useOverloadApi = parser.GetBool(L"overload");           

    wstring ud;
    parser.TryGetString(L"ud", ud, L"");

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    if (useOverloadApi)
    {
        if (ud.empty())
        {
            MoveNextFabricUpgradeDomainImpl2(expectedError);
        }
        else
        {
            MoveNextFabricUpgradeDomainImpl2(ud.c_str(), expectedError);
        }
    }
    else
    {
        MoveNextFabricUpgradeDomainImpl(expectedError);
    }

    return true;
}

bool TestFabricClientUpgrade::PrintUpgradeProgress(bool printDetails, ComPointer<IFabricApplicationUpgradeProgressResult3> const& upgradeProgress)
{
    if (!upgradeProgress)
    {
        TestSession::WriteInfo(
            TraceSource, 
            "Upgrade Status (NULL)\n");
        return true;
    }

    FABRIC_URI appNameResult = upgradeProgress->get_ApplicationName();
    NamingUri namingUri;
    HRESULT hr = S_OK;
    if (!NamingUri::TryParse(wstring(appNameResult), namingUri))
    {
        TestSession::WriteError(
            TraceSource, 
            "Could not parse '{0}' as a NamingUri",
            wstring(appNameResult));

        hr = FABRIC_E_INVALID_NAME_URI;
    }
    
    LPCWSTR appTypeName = NULL;
    LPCWSTR targetVersion = NULL;
    FABRIC_APPLICATION_UPGRADE_STATE upgradeState(FABRIC_APPLICATION_UPGRADE_STATE_INVALID);
    if (SUCCEEDED(hr))
    {
        appTypeName = upgradeProgress->get_ApplicationTypeName();
        targetVersion = upgradeProgress->get_TargetApplicationTypeVersion();
        upgradeState = upgradeProgress->get_UpgradeState();
    }

    FABRIC_ROLLING_UPGRADE_MODE rollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_INVALID;
    LPCWSTR nextUpgradeDomain = NULL;
    if (SUCCEEDED(hr))
    {
        rollingUpgradeMode = upgradeProgress->get_RollingUpgradeMode();
        nextUpgradeDomain = upgradeProgress->get_NextUpgradeDomain();
    }

    ULONG domainCount = 0;
    FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const * descriptions = NULL;
    if (SUCCEEDED(hr))
    {
        hr = upgradeProgress->GetUpgradeDomains(&domainCount, &descriptions);
    }

    if (SUCCEEDED(hr))
    {
        wstring applicationUpgradeState(L"Invalid");

        switch (upgradeState)
        {
            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED:
                applicationUpgradeState = L"RollforwardCompleted";
                break;

            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS:
                applicationUpgradeState = L"RollforwardInProgress";
                break;

            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_PENDING:
                applicationUpgradeState = L"RollforwardPending";
                break;

            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_COMPLETED:
                applicationUpgradeState = L"RollbackCompleted";
                break;

            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS:
                applicationUpgradeState = L"RollbackInProgress";
                break;

            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_PENDING:
                applicationUpgradeState = L"RollbackPending";
                break;

            case FABRIC_APPLICATION_UPGRADE_STATE_FAILED:
                applicationUpgradeState = L"Failed";
                break;
        }

        vector<wstring> completedDomains;
        vector<wstring> pendingDomains;
        vector<wstring> inProgressDomains;
        vector<wstring> invalidDomains;

        for (ULONG ix = 0; ix < domainCount; ++ix)
        {
            FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const & description = descriptions[ix];

            switch (description.State)
            {
                case FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED:
                    completedDomains.push_back(wstring(description.Name));
                    break;

                case FABRIC_UPGRADE_DOMAIN_STATE_PENDING:
                    pendingDomains.push_back(wstring(description.Name));
                    break;

                case FABRIC_UPGRADE_DOMAIN_STATE_IN_PROGRESS:
                    inProgressDomains.push_back(wstring(description.Name));
                    break;

                default:
                    invalidDomains.push_back(wstring(description.Name));
            }
        }

        wstring upgradeMode(L"Invalid");
        switch (rollingUpgradeMode)
        {
            case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO:
                upgradeMode = L"UnmonitoredAuto";
                break;

            case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
                upgradeMode = L"UnmonitoredManual";
                break;

            case FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
                upgradeMode = L"Monitored";
                break;
        }

        wstring nextDomain = (nextUpgradeDomain == NULL ? L"None" : wstring(nextUpgradeDomain));

        auto progressDescription = upgradeProgress->get_UpgradeProgress();

        auto upgradeDuration = TimeSpan::Zero;
        auto udDuration = TimeSpan::Zero;

        auto startTime = DateTime::Zero;
        auto failureTime = DateTime::Zero;
        auto failureReason = UpgradeFailureReason::None;
        UpgradeDomainProgress failureProgress;

        if (progressDescription != NULL)
        {
            upgradeDuration = TimeSpan::FromSeconds(progressDescription->UpgradeDurationInSeconds);
            udDuration = TimeSpan::FromSeconds(progressDescription->CurrentUpgradeDomainDurationInSeconds);

            if (progressDescription->Reserved != NULL)
            {
                auto const * progressDescriptionEx1 = static_cast<FABRIC_APPLICATION_UPGRADE_PROGRESS_EX1 const *>(progressDescription->Reserved);

                startTime = DateTime(progressDescriptionEx1->StartTimestampUtc);
                failureTime = DateTime(progressDescriptionEx1->FailureTimestampUtc);
                failureReason = UpgradeFailureReason::FromPublicApi(progressDescriptionEx1->FailureReason);

                if (progressDescriptionEx1->UpgradeDomainProgressAtFailure != NULL)
                {
                    failureProgress.FromPublicApi(*progressDescriptionEx1->UpgradeDomainProgressAtFailure);
                }
            }
        }

        TestSession::WriteInfo(
            TraceSource, 
            "Upgrade Status ({0}) ({1}): [{2}:{3} ({4})] Duration[UD={5} Total={6}]\n",
            namingUri,
            upgradeMode,
            wstring(appTypeName),
            wstring(targetVersion),
            applicationUpgradeState,
            udDuration,
            upgradeDuration);

        TestSession::WriteInfo(
            TraceSource, 
            "Upgrade Status Ex1: start={0} failure={1} reason={2} progressAtFailure={3}",
            startTime,
            failureTime,
            failureReason,
            failureProgress);

        TestSession::WriteInfo(
            TraceSource, 
            "COMPLETED = {0} \nIN PROGRESS = {1} \nPENDING = {2} \nINVALID = {3}\nNEXT = {4}\n",
            completedDomains,
            inProgressDomains,
            pendingDomains,
            invalidDomains,
            nextDomain);

        TestSession::FailTestIf(progressDescription == NULL, "get_UpgradeProgress() is NULL");

        vector<FABRIC_HEALTH_EVALUATION_KIND> possibleHealthEvaluationReasons;
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_EVENT);
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS);
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_SERVICES);
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS);
        
        TestFabricClientHealth::PrintHealthEvaluations(
            progressDescription->UnhealthyEvaluations,
            possibleHealthEvaluationReasons);

        PrintUpgradeDomainProgress(progressDescription->CurrentUpgradeDomainProgress);

        if (printDetails)
        {
            FABRIC_APPLICATION_UPGRADE_DESCRIPTION const * upgradeDescription = progressDescription->UpgradeDescription;

            if (upgradeDescription == NULL)
            {
                TestSession::WriteInfo(
                    TraceSource, 
                    "Upgrade Description: NULL");
            }
            else
            {
                Management::ClusterManager::ApplicationUpgradeDescription internalUpgradeDescription;
                auto error = internalUpgradeDescription.FromPublicApi(*upgradeDescription);

                TestSession::FailTestIfNot(
                    error.IsSuccess(), 
                    "ApplicationUpgradeDescription::FromPublicApi error={0}",
                    error);

                TestSession::WriteInfo(
                    TraceSource, 
                    "Upgrade Description: {0}",
                    internalUpgradeDescription);
            }
        }
    }
    else
    {
        TestSession::WriteError(
            TraceSource, 
            "Could not read upgrade status due to HR = {0}",
            hr);
    }

    return SUCCEEDED(hr);
}

void TestFabricClientUpgrade::PrintUpgradeDomainProgress(FABRIC_UPGRADE_DOMAIN_PROGRESS const* upgradeDomainProgress)
{
    if (upgradeDomainProgress != NULL)
    {
        TestSession::WriteInfo(TraceSource, "CURRENT UPGRADE DOMAIN PROGRESS:");

        TestSession::WriteInfo(TraceSource, "UpgradeDomainName: {0}", wformatString(upgradeDomainProgress->UpgradeDomainName));

        for (ULONG ix = 0; ix < upgradeDomainProgress->NodeProgressList->Count; ++ix)
        {
            auto const& nodeUpgradeProgress = upgradeDomainProgress->NodeProgressList->Items[ix];

            auto nodeName = wformatString(nodeUpgradeProgress.NodeName);

            auto upgradePhase = NodeUpgradePhase::Invalid;
            switch (nodeUpgradeProgress.UpgradePhase)
            {
            case FABRIC_NODE_UPGRADE_PHASE_PRE_UPGRADE_SAFETY_CHECK:
                upgradePhase = NodeUpgradePhase::PreUpgradeSafetyCheck;
                break;
            case FABRIC_NODE_UPGRADE_PHASE_UPGRADING:
                upgradePhase = NodeUpgradePhase::Upgrading;
                break;
            case FABRIC_NODE_UPGRADE_PHASE_POST_UPGRADE_SAFETY_CHECK:
                upgradePhase = NodeUpgradePhase::PostUpgradeSafetyCheck;
                break;
            default:
                break;
            }

            auto safetyCheckKind = UpgradeSafetyCheckKind::Invalid;
            if (upgradePhase != NodeUpgradePhase::Upgrading)
            {
                switch (nodeUpgradeProgress.PendingSafetyChecks->Items[0].Kind)
                {
                case FABRIC_UPGRADE_SEED_NODE_SAFETY_CHECK_KIND_ENSURE_QUORUM:
                    safetyCheckKind = UpgradeSafetyCheckKind::EnsureSeedNodeQuorum;
                    break;
                case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_ENSURE_QUORUM:
                    safetyCheckKind = UpgradeSafetyCheckKind::EnsurePartitionQuorum;
                    break;
                case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_SWAP:
                    safetyCheckKind = UpgradeSafetyCheckKind::WaitForPrimarySwap;
                    break;
                case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_PLACEMENT:
                    safetyCheckKind = UpgradeSafetyCheckKind::WaitForPrimaryPlacement;
                    break;
                case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_INBUILD_REPLICA:
                    safetyCheckKind = UpgradeSafetyCheckKind::WaitForInBuildReplica;
                    break;
                case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RECONFIGURATION:
                    safetyCheckKind = UpgradeSafetyCheckKind::WaitForReconfiguration;
                    break;
                case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_ENSURE_AVAILABILITY:
                    safetyCheckKind = UpgradeSafetyCheckKind::EnsureAvailability;
                    break;
                case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RESOURCE_AVAILABILITY:
                    safetyCheckKind = UpgradeSafetyCheckKind::WaitForResourceAvailability;
                    break;
                default:
                    break;
                }
            }

            if (upgradePhase == NodeUpgradePhase::Upgrading)
            {
                TestSession::WriteInfo(TraceSource, "NodeName={0}, {1}", nodeName, upgradePhase);
            }
            else if (safetyCheckKind == UpgradeSafetyCheckKind::EnsureSeedNodeQuorum)
            {
                TestSession::WriteInfo(TraceSource, "NodeName={0}, {1}, SafetyCheck={2}", nodeName, upgradePhase, safetyCheckKind);
            }
            else
            {
                auto partitionSafetyCheck = static_cast<FABRIC_UPGRADE_PARTITION_SAFETY_CHECK*>(nodeUpgradeProgress.PendingSafetyChecks->Items[0].Value);
                auto partitionId = Guid(partitionSafetyCheck->PartitionId);

                TestSession::WriteInfo(TraceSource, "NodeName={0}, {1}, SafetyCheck={2}, PartitionId={3}", nodeName, upgradePhase, safetyCheckKind, partitionId);
            }
        }
    }
}

bool TestFabricClientUpgrade::PrintUpgradeProgress(bool printDetails, ComPointer<IFabricUpgradeProgressResult3> const& upgradeProgress)
{
    if (!upgradeProgress)
    {
        TestSession::WriteInfo(
            TraceSource, 
            "Upgrade Status (NULL)\n");
        return true;
    }    
    
    LPCWSTR codeVersion = NULL;
    LPCWSTR configVersion = NULL;
    FABRIC_UPGRADE_STATE upgradeState(FABRIC_UPGRADE_STATE_INVALID);
    FABRIC_ROLLING_UPGRADE_MODE rollingUpgradeMode(FABRIC_ROLLING_UPGRADE_MODE_INVALID);
    LPCWSTR nextUpgradeDomain = NULL;
    
    codeVersion    = upgradeProgress->get_TargetCodeVersion();
    configVersion = upgradeProgress->get_TargetConfigVersion();
    upgradeState  = upgradeProgress->get_UpgradeState();   
    rollingUpgradeMode = upgradeProgress->get_RollingUpgradeMode();
    nextUpgradeDomain = upgradeProgress->get_NextUpgradeDomain();

    ULONG domainCount = 0;
    FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const * descriptions = NULL;

    HRESULT hr = S_OK;
    hr = upgradeProgress->GetUpgradeDomains(&domainCount, &descriptions);
    
    if (SUCCEEDED(hr))
    {
        wstring fabricUpgradeState(L"Invalid");

        switch (upgradeState)
        {
            case FABRIC_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED:
                fabricUpgradeState = L"RollforwardCompleted";
                break;

            case FABRIC_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS:
                fabricUpgradeState = L"RollforwardInProgress";
                break;

            case FABRIC_UPGRADE_STATE_ROLLING_FORWARD_PENDING:
                fabricUpgradeState = L"RollforwardPending";
                break;

            case FABRIC_UPGRADE_STATE_ROLLING_BACK_COMPLETED:
                fabricUpgradeState = L"RollbackCompleted";
                break;

            case FABRIC_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS:
                fabricUpgradeState = L"RollbackInProgress";
                break;

            case FABRIC_UPGRADE_STATE_ROLLING_BACK_PENDING:
                fabricUpgradeState = L"RollbackPending";
                break;

        }

        vector<wstring> completedDomains;
        vector<wstring> pendingDomains;
        vector<wstring> inProgressDomains;
        vector<wstring> invalidDomains;

        for (ULONG ix = 0; ix < domainCount; ++ix)
        {
            FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const & description = descriptions[ix];

            switch (description.State)
            {
                case FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED:
                    completedDomains.push_back(wstring(description.Name));
                    break;

                case FABRIC_UPGRADE_DOMAIN_STATE_PENDING:
                    pendingDomains.push_back(wstring(description.Name));
                    break;

                case FABRIC_UPGRADE_DOMAIN_STATE_IN_PROGRESS:
                    inProgressDomains.push_back(wstring(description.Name));
                    break;

                default:
                    invalidDomains.push_back(wstring(description.Name));
            }
        }

        wstring upgradeMode(L"Invalid");
        switch (rollingUpgradeMode)
        {
            case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO:
                upgradeMode = L"UnmonitoredAuto";
                break;

            case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
                upgradeMode = L"UnmonitoredManual";
                break;

            case FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
                upgradeMode = L"Monitored";
                break;
        }

        wstring nextDomain = (nextUpgradeDomain == NULL ? L"None" : wstring(nextUpgradeDomain));

        auto progressDescription = upgradeProgress->get_UpgradeProgress();

        auto upgradeDuration = TimeSpan::Zero;
        auto udDuration = TimeSpan::Zero;

        auto startTime = DateTime::Zero;
        auto failureTime = DateTime::Zero;
        auto failureReason = UpgradeFailureReason::None;
        UpgradeDomainProgress failureProgress;

        if (progressDescription != NULL)
        {
            upgradeDuration = TimeSpan::FromSeconds(progressDescription->UpgradeDurationInSeconds);
            udDuration = TimeSpan::FromSeconds(progressDescription->CurrentUpgradeDomainDurationInSeconds);

            if (progressDescription->Reserved != NULL)
            {
                auto const * progressDescriptionEx1 = static_cast<FABRIC_UPGRADE_PROGRESS_EX1 const *>(progressDescription->Reserved);

                startTime = DateTime(progressDescriptionEx1->StartTimestampUtc);
                failureTime = DateTime(progressDescriptionEx1->FailureTimestampUtc);
                failureReason = UpgradeFailureReason::FromPublicApi(progressDescriptionEx1->FailureReason);

                if (progressDescriptionEx1->UpgradeDomainProgressAtFailure != NULL)
                {
                    failureProgress.FromPublicApi(*progressDescriptionEx1->UpgradeDomainProgressAtFailure);
                }
            }
        }

        TestSession::WriteInfo(
            TraceSource, 
            "Upgrade Fabric Status ({0}): [{1}:{2} ({3})] Duration[UD={4} total={5}]",
            upgradeMode,
            wstring(codeVersion),
            wstring(configVersion),
            fabricUpgradeState,
            udDuration,
            upgradeDuration);

        TestSession::WriteInfo(
            TraceSource, 
            "Upgrade Status Ex1: start={0} failure={1} reason={2} progressAtFailure={3}",
            startTime,
            failureTime,
            failureReason,
            failureProgress);

        TestSession::WriteInfo(
            TraceSource, 
            "COMPLETED = {0} \nIN PROGRESS = {1} \nPENDING = {2} \nINVALID = {3}\nNEXT = {4}\n",
            completedDomains,
            inProgressDomains,
            pendingDomains,
            invalidDomains,
            nextDomain);

        TestSession::FailTestIf(progressDescription == NULL, "get_UpgradeProgress() is NULL");

        vector<FABRIC_HEALTH_EVALUATION_KIND> possibleHealthEvaluationReasons;
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_EVENT);
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS);
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS);
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_NODES);
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES);
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION);
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK);
        possibleHealthEvaluationReasons.push_back(FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK);

        TestFabricClientHealth::PrintHealthEvaluations(
            progressDescription->UnhealthyEvaluations,
            possibleHealthEvaluationReasons);
        
        PrintUpgradeDomainProgress(progressDescription->CurrentUpgradeDomainProgress);

        if (printDetails)
        {
            FABRIC_UPGRADE_DESCRIPTION const * upgradeDescription = progressDescription->UpgradeDescription;

            if (upgradeDescription == NULL)
            {
                TestSession::WriteInfo(
                    TraceSource, 
                    "Upgrade Description: NULL");
            }
            else
            {
                Management::ClusterManager::FabricUpgradeDescription internalUpgradeDescription;
                auto error = internalUpgradeDescription.FromPublicApi(*upgradeDescription);

                TestSession::FailTestIfNot(
                    error.IsSuccess(), 
                    "FabricUpgradeDescription::FromPublicApi error={0}",
                    error);

                TestSession::WriteInfo(
                    TraceSource, 
                    "Upgrade Description: {0}",
                    internalUpgradeDescription);
            }
        }
    }
    else
    {
        TestSession::WriteError(
            TraceSource, 
            "Could not read upgrade status due to HR = {0}",
            hr);
    }

    return SUCCEEDED(hr);
}

bool TestFabricClientUpgrade::PrintUpgradeChanges(
    ComPointer<IFabricApplicationUpgradeProgressResult3> const& previousProgress,
    ComPointer<IFabricApplicationUpgradeProgressResult3> const& upgradeProgress)
{
    PrintUpgradeProgress(false, upgradeProgress);

    vector<wstring> changedDomains;

    ULONG changedCount = 0;
    FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const * changedDomainsArray = NULL;
    HRESULT hr = upgradeProgress->GetChangedUpgradeDomains(previousProgress.GetRawPointer(), &changedCount, &changedDomainsArray);

    if (SUCCEEDED(hr))
    {
        for (ULONG ix = 0; ix < changedCount; ++ix)
        {
            FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const & description = changedDomainsArray[ix];

            wstring info(description.Name);
            info.append(L":");

            switch (description.State)
            {
                case FABRIC_UPGRADE_DOMAIN_STATE_PENDING:
                    info.append(L"Pending");
                    break;

                case FABRIC_UPGRADE_DOMAIN_STATE_IN_PROGRESS:
                    info.append(L"InProgress");
                    break;

                case FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED:
                    info.append(L"Completed");
                    break;

                default:
                    info.append(L"Invalid");
            }

            changedDomains.push_back(info);
        }

        TestSession::WriteInfo(
            TraceSource, 
            "Upgrade Application Changes = {0}", 
            changedDomains);
    }
    else
    {
        TestSession::WriteError(
            TraceSource, 
            "Could not read upgrade application changes due to HR = {0}",
            hr);
    }

    return SUCCEEDED(hr);
}

bool TestFabricClientUpgrade::PrintUpgradeChanges(
    ComPointer<IFabricUpgradeProgressResult3> const& previousProgress,
    ComPointer<IFabricUpgradeProgressResult3> const& upgradeProgress)
{
    PrintUpgradeProgress(false, upgradeProgress);

    vector<wstring> changedDomains;

    ULONG changedCount = 0;
FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const * changedDomainsArray = NULL;
HRESULT hr = upgradeProgress->GetChangedUpgradeDomains(previousProgress.GetRawPointer(), &changedCount, &changedDomainsArray);

if (SUCCEEDED(hr))
{
    for (ULONG ix = 0; ix < changedCount; ++ix)
    {
        FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION const & description = changedDomainsArray[ix];

        wstring info(description.Name);
        info.append(L":");

        switch (description.State)
        {
        case FABRIC_UPGRADE_DOMAIN_STATE_PENDING:
            info.append(L"Pending");
            break;

        case FABRIC_UPGRADE_DOMAIN_STATE_IN_PROGRESS:
            info.append(L"InProgress");
            break;

        case FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED:
            info.append(L"Completed");
            break;

        default:
            info.append(L"Invalid");
        }

        changedDomains.push_back(info);
    }

    TestSession::WriteInfo(
        TraceSource,
        "Upgrade Fabric Changes = {0}",
        changedDomains);
}
else
{
    TestSession::WriteError(
        TraceSource,
        "Could not read upgrade fabric changes due to HR = {0}",
        hr);
}

return SUCCEEDED(hr);
}

void TestFabricClientUpgrade::ProvisionApplicationTypeImpl(
    wstring const& applicationBuildPath,
    bool isAsync,
    ApplicationPackageCleanupPolicy::Enum applicationPackageCleanupPolicy,
    HRESULT expectedError)
{
    ScopedHeap heap;

    auto description = heap.AddItem<FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION>();
    description->BuildPath = applicationBuildPath.c_str();
    description->Async = (isAsync ? TRUE : FALSE);
    auto desc1 = heap.AddItem<FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_EX1>();
    memset(desc1.GetRawPointer(), 0, sizeof(FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_EX1));
    desc1->ApplicationPackageCleanupPolicy = Management::ClusterManager::ApplicationPackageCleanupPolicy::ToPublicApi(applicationPackageCleanupPolicy);
    description->Reserved = desc1.GetRawPointer();


    Random r;
    auto managementClient = CreateFabricApplicationClient10(FABRICSESSION.FabricDispatcher.Federation);
    if (r.Next(5) == 0)
    {
        // Use older versions of provision with description
        TestFabricClient::PerformFabricClientOperation(
            L"ProvisionApplicationType",
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginProvisionApplicationType2(
                description.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
            [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndProvisionApplicationType2(context.GetRawPointer());
        },
            expectedError,
            FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS);
    }
    else
    {
        // Use newer versions, with hierarchy
        FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_BASE descriptionBase;
        descriptionBase.Kind = FABRIC_PROVISION_APPLICATION_TYPE_KIND_IMAGE_STORE_PATH;
        descriptionBase.Value = description.GetRawPointer();

        // There is a possibility that application package could be deleted by previous provision for retry scenario
        HRESULT acceptableErrorOnRetry = FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS;
        if (applicationPackageCleanupPolicy == ApplicationPackageCleanupPolicy::Enum::Automatic ||
            ((applicationPackageCleanupPolicy == ApplicationPackageCleanupPolicy::Enum::Default || 
                applicationPackageCleanupPolicy == ApplicationPackageCleanupPolicy::Enum::Invalid) &&
                Management::ManagementConfig::GetConfig().CleanupApplicationPackageOnProvisionSuccess))
        {
            acceptableErrorOnRetry = FABRIC_E_DIRECTORY_NOT_FOUND;
        }

        TestFabricClient::PerformFabricClientOperation(
            L"ProvisionApplicationType",
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
            {
                return managementClient->BeginProvisionApplicationType3(
                    &descriptionBase,
                    timeout,
                    callback.GetRawPointer(),
                    context.InitializationAddress());
            },
            [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
            {
                return managementClient->EndProvisionApplicationType3(context.GetRawPointer());
            },
            expectedError,
            acceptableErrorOnRetry);
    }    
}

void TestFabricClientUpgrade::ProvisionApplicationTypeImpl(
    wstring const& downloadPath,
    wstring const& appTypeName,
    wstring const& appTypeVersion,
    bool isAsync,
    HRESULT expectedError)
{
    ScopedHeap heap;

    auto description = heap.AddItem<FABRIC_EXTERNAL_STORE_PROVISION_APPLICATION_TYPE_DESCRIPTION>();
    description->ApplicationPackageDownloadUri = downloadPath.c_str();
    description->ApplicationTypeName = appTypeName.c_str();
    description->ApplicationTypeVersion = appTypeVersion.c_str();
    description->Async = (isAsync ? TRUE : FALSE);

    auto managementClient = CreateFabricApplicationClient10(FABRICSESSION.FabricDispatcher.Federation);
    
    FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_BASE descriptionBase;
    descriptionBase.Kind = FABRIC_PROVISION_APPLICATION_TYPE_KIND_EXTERNAL_STORE;
    descriptionBase.Value = description.GetRawPointer();

    TestFabricClient::PerformFabricClientOperation(
        L"ProvisionApplicationType",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginProvisionApplicationType3(
                &descriptionBase,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndProvisionApplicationType3(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS);
}

void TestFabricClientUpgrade::UnprovisionApplicationTypeImpl(
    wstring const& typeName,
    wstring const& typeVersion,
    bool isAsync,
    HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "UnprovisionApplication called for type {0}, version {1}", typeName, typeVersion);

    auto managementClient = CreateFabricApplicationClient10(FABRICSESSION.FabricDispatcher.Federation);

    FABRIC_UNPROVISION_APPLICATION_TYPE_DESCRIPTION description = { 0 };
    description.ApplicationTypeName = typeName.c_str();
    description.ApplicationTypeVersion = typeVersion.c_str();

    description.Async = (isAsync ? TRUE : FALSE);

    TestFabricClient::PerformFabricClientOperation(
        L"UnprovisionApplicationType",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginUnprovisionApplicationType2(
                &description,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndUnprovisionApplicationType2(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_APPLICATION_TYPE_NOT_FOUND);
}

void TestFabricClientUpgrade::CreateApplicationImpl(
    FABRIC_APPLICATION_DESCRIPTION const& applicationeDescription,
    HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "CreateApplication called for name {0}",
        applicationeDescription.ApplicationName, applicationeDescription.ApplicationTypeName, applicationeDescription.ApplicationTypeVersion);

    ComPointer<IFabricApplicationManagementClient> managementClient = CreateFabricApplicationClient(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"CreateApplication",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginCreateApplication(
                &applicationeDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndCreateApplication(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_APPLICATION_ALREADY_EXISTS);
}

void TestFabricClientUpgrade::CreateComposeDeploymentImpl(
    FABRIC_COMPOSE_DEPLOYMENT_DESCRIPTION const & composeDescription,
    HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "CreateComposeDeployment called for name {0}", composeDescription.DeploymentName);

    ComPointer<IInternalFabricApplicationManagementClient> composeClient = CreateInternalFabricApplicationClient(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"CreateComposeDeployment",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return composeClient->BeginCreateComposeDeployment(
                &composeDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return composeClient->EndCreateComposeDeployment(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_COMPOSE_DEPLOYMENT_ALREADY_EXISTS);
}

void TestFabricClientUpgrade::DeleteComposeDeploymentImpl(
    wstring const & deploymentName,
    HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "DeleteComposeDeployment called for name {0}", deploymentName);

    ComPointer<IInternalFabricApplicationManagementClient> composeClient = CreateInternalFabricApplicationClient(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"DeleteComposeDeployment",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            FABRIC_DELETE_COMPOSE_DEPLOYMENT_DESCRIPTION description;
            description.DeploymentName = deploymentName.c_str();

            return composeClient->BeginDeleteComposeDeployment(
                &description,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return composeClient->EndDeleteComposeDeployment(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_COMPOSE_DEPLOYMENT_NOT_FOUND);
}

void TestFabricClientUpgrade::UpdateApplicationImpl(
    FABRIC_APPLICATION_UPDATE_DESCRIPTION const& applicationUpdateDescription,
    HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "UpdateApplication called for name {0}", applicationUpdateDescription.ApplicationName);

    auto managementClient = CreateFabricApplicationClient6(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"UpdateApplication",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginUpdateApplication(
                &applicationUpdateDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndUpdateApplication(context.GetRawPointer());
        },
        expectedError);
}

void TestFabricClientUpgrade::DeleteApplicationImpl(NamingUri const& name, bool const isForce, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "DeleteApplication called for name {0}", name);

    ComPointer<IFabricApplicationManagementClient7> managementClient = CreateFabricApplicationClient7(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"DeleteApplication",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            FABRIC_DELETE_APPLICATION_DESCRIPTION description;
            auto tempName = name.Name;
            description.ApplicationName = tempName.c_str();
            description.ForceDelete = isForce;
            return managementClient->BeginDeleteApplication2(
                &description,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndDeleteApplication(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_APPLICATION_NOT_FOUND);
}

void TestFabricClientUpgrade::UpgradeComposeDeploymentImpl(
    FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION & description,
    TimeSpan const timeout,
    vector<HRESULT> expectedErrors)
{
    TestSession::WriteNoise(TraceSource, "UpgradeComposeDeployment called with expected errors {0}", expectedErrors);

    vector<HRESULT> allowedErrorsOnRetry;
    allowedErrorsOnRetry.push_back(FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS);   // previous upgrade is still running
    allowedErrorsOnRetry.push_back(FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION);  // previous upgrade is complete

    vector<HRESULT> retryableErrors;
    retryableErrors.push_back(FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS);   // may get this for an interrupting request

    ComPointer<IInternalFabricApplicationManagementClient> composeClient = CreateInternalFabricApplicationClient(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"UpgradeComposeDeployment",
        timeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return composeClient->BeginUpgradeComposeDeployment(
                &description,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return composeClient->EndUpgradeComposeDeployment(context.GetRawPointer());
        },
        expectedErrors,
        allowedErrorsOnRetry,
        retryableErrors);
}

void TestFabricClientUpgrade::UpgradeApplicationImpl(
    FABRIC_APPLICATION_UPGRADE_DESCRIPTION & upgradeDescription, 
    TimeSpan const inTimeout, 
    vector<HRESULT> expectedErrors)
{
    TestSession::WriteNoise(TraceSource, "UpgradeApplication called with expected errors {0}", expectedErrors);

    vector<HRESULT> allowedErrorsOnRetry;
    allowedErrorsOnRetry.push_back(FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS);   // previous upgrade is still running
    allowedErrorsOnRetry.push_back(FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION);  // previous upgrade is complete

    vector<HRESULT> retryableErrors;
    retryableErrors.push_back(FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS);   // may get this for an interrupting request

    ComPointer<IFabricApplicationManagementClient> managementClient = CreateFabricApplicationClient(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"UpgradeApplication",
        inTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginUpgradeApplication(
                &upgradeDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndUpgradeApplication(context.GetRawPointer());
        },
        expectedErrors,
        allowedErrorsOnRetry,
        retryableErrors);
}

void TestFabricClientUpgrade::UpdateApplicationUpgradeImpl(
    FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION & updateDescription, 
    HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "UpdateApplicationUpgrade called with expected error {0}: flags={1:0X}", expectedError, updateDescription.UpdateFlags);

    vector<HRESULT> expectedErrors;
    expectedErrors.push_back(expectedError);

    ComPointer<IFabricApplicationManagementClient3> managementClient = CreateFabricApplicationClient3(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"UpdateApplicationUpgrade",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginUpdateApplicationUpgrade(
                &updateDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndUpdateApplicationUpgrade(context.GetRawPointer());
        },
        expectedErrors,
        vector<HRESULT>(),
        vector<HRESULT>());
}

void TestFabricClientUpgrade::RollbackApplicationUpgradeImpl(
    NamingUri const & appUri,
    HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "RollbackApplicationUpgrade {0} called with expected error {1}", appUri, expectedError);

    vector<HRESULT> expectedErrors;
    expectedErrors.push_back(expectedError);

    auto managementClient = CreateFabricApplicationClient5(FABRICSESSION.FabricDispatcher.Federation);
    auto appName = appUri.ToString();

    TestFabricClient::PerformFabricClientOperation(
        L"RollbackApplicationUpgrade",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginRollbackApplicationUpgrade(
                appName.c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndRollbackApplicationUpgrade(context.GetRawPointer());
        },
        expectedErrors,
        vector<HRESULT>(),
        vector<HRESULT>());
}

void TestFabricClientUpgrade::RollbackComposeDeploymentImpl(
    wstring const & deploymentName,
    HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "RollbackComposeDeployment {0} called with expected error {1}", deploymentName, expectedError);

    vector<HRESULT> expectedErrors;
    expectedErrors.push_back(expectedError);

    ComPointer<IInternalFabricApplicationManagementClient2> composeClient = CreateInternalFabricApplicationClient2(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"RollbackComposeDeployment",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            FABRIC_COMPOSE_DEPLOYMENT_ROLLBACK_DESCRIPTION description;
            description.DeploymentName = deploymentName.c_str();
            return composeClient->BeginRollbackComposeDeployment(
                &description,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return composeClient->EndRollbackComposeDeployment(context.GetRawPointer());
        },
        expectedErrors,
        vector<HRESULT>(),
        vector<HRESULT>());
}

void TestFabricClientUpgrade::UpgradeFabricImpl(FABRIC_UPGRADE_DESCRIPTION & upgradeFabricDescription, TimeSpan const inTimeout, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "UpgradeFabric called with expected error {0}", expectedError);

    vector<HRESULT> expectedErrors;
    expectedErrors.push_back(expectedError);

    vector<HRESULT> allowedErrorsOnRetry;
    allowedErrorsOnRetry.push_back(FABRIC_E_FABRIC_UPGRADE_IN_PROGRESS);   // previous upgrade is still running   
    allowedErrorsOnRetry.push_back(FABRIC_E_FABRIC_UPGRADE_VALIDATION_ERROR);  // previous upgrade is complete   

    vector<HRESULT> retryableErrors;
    retryableErrors.push_back(S_OK);

    auto managementClient = CreateFabricClusterClient2(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"UpgradeFabric",
        inTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginUpgradeFabric(
                &upgradeFabricDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndUpgradeFabric(context.GetRawPointer());
        },
        expectedErrors,
        allowedErrorsOnRetry,
        retryableErrors);
}

void TestFabricClientUpgrade::UpdateFabricUpgradeImpl(
    FABRIC_UPGRADE_UPDATE_DESCRIPTION & updateDescription, 
    HRESULT expectedError)
{
    TestSession::WriteInfo(TraceSource, "UpdateFabricUpgrade called with expected error {0}: flags={1:0X}", expectedError, updateDescription.UpdateFlags);

    vector<HRESULT> expectedErrors;
    expectedErrors.push_back(expectedError);

    ComPointer<IFabricClusterManagementClient3> managementClient = CreateFabricClusterClient3(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"UpdateFabricUpgrade",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginUpdateFabricUpgrade(
                &updateDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndUpdateFabricUpgrade(context.GetRawPointer());
        },
        expectedErrors,
        vector<HRESULT>(),
        vector<HRESULT>());
}

void TestFabricClientUpgrade::RollbackFabricUpgradeImpl(
    HRESULT expectedError)
{
    TestSession::WriteInfo(TraceSource, "RollbackFabricUpgrade called with expected error {0}", expectedError);

    vector<HRESULT> expectedErrors;
    expectedErrors.push_back(expectedError);

    auto managementClient = CreateFabricClusterClient4(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"RollbackFabricUpgrade",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginRollbackFabricUpgrade(
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndRollbackFabricUpgrade(context.GetRawPointer());
        },
        expectedErrors,
        vector<HRESULT>(),
        vector<HRESULT>());
}

void TestFabricClientUpgrade::UpgradeAppStatusImpl(
    wstring const & appName, 
    HRESULT expectedError, 
    __out Common::ComPointer<IFabricApplicationUpgradeProgressResult3> & upgradeProgressResult)
{
    vector<HRESULT> expectedErrors;
    expectedErrors.push_back(expectedError);

    this->UpgradeAppStatusImpl(appName, expectedErrors, upgradeProgressResult);
}

HRESULT TestFabricClientUpgrade::UpgradeAppStatusImpl(
    wstring const & appName, 
    vector<HRESULT> expectedErrors, 
    __out Common::ComPointer<IFabricApplicationUpgradeProgressResult3> & upgradeProgressResult)
{
    TestSession::WriteNoise(TraceSource, "UpgradeProgress called with expected error {0}", expectedErrors);

    ComPointer<IFabricApplicationManagementClient3> managementClient = CreateFabricApplicationClient3(FABRICSESSION.FabricDispatcher.Federation);

    return TestFabricClient::PerformFabricClientOperation(
        L"ApplicationUpgradeProgress",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginGetApplicationUpgradeProgress(
                appName.c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            ComPointer<IFabricApplicationUpgradeProgressResult2> result2;
            HRESULT hr = managementClient->EndGetApplicationUpgradeProgress(context.GetRawPointer(), result2.InitializationAddress());

            if (SUCCEEDED(hr))
            {
                hr = result2->QueryInterface(
                    IID_IFabricApplicationUpgradeProgressResult3, 
                    (void **)upgradeProgressResult.InitializationAddress());

                TestSession::FailTestIfNot(SUCCEEDED(hr), "Failed to QI for IID_IFabricApplicationUpgradeProgressResult3: hr = {0}", hr);
            }

            return hr;
        },
        expectedErrors,
        vector<HRESULT>(),
        vector<HRESULT>());
}

void TestFabricClientUpgrade::UpgradeFabricStatusImpl(
    HRESULT expectedError, 
    HRESULT retryableError,
    __out Common::ComPointer<IFabricUpgradeProgressResult3> & upgradeProgressResult)
{
    TestSession::WriteNoise(TraceSource, "UpgradeFabricProgress called with expected error {0}", expectedError);

    auto managementClient = CreateFabricClusterClient3(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"FabricUpgradeProgress",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginGetFabricUpgradeProgress(
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            ComPointer<IFabricUpgradeProgressResult2> result2;
            HRESULT hr = managementClient->EndGetFabricUpgradeProgress(context.GetRawPointer(), result2.InitializationAddress());

            if (SUCCEEDED(hr))
            {
                hr = result2->QueryInterface(
                    IID_IFabricUpgradeProgressResult3, 
                    (void **)upgradeProgressResult.InitializationAddress());

                TestSession::FailTestIfNot(SUCCEEDED(hr), "Failed to QI for IID_IFabricUpgradeProgressResult3: hr = {0}", hr);
            }

            return hr;
        },
        expectedError,
        expectedError,
        retryableError);
}

void TestFabricClientUpgrade::MoveNextApplicationUpgradeDomainImpl(
    wstring const & appName,
    HRESULT expectedError)
{
    ComPointer<IFabricApplicationUpgradeProgressResult3> upgradeProgress;
    UpgradeAppStatusImpl(appName, S_OK, upgradeProgress);

    ComPointer<IFabricApplicationManagementClient> managementClient = CreateFabricApplicationClient(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"MoveNextApplicationUpgradeDomain",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginMoveNextApplicationUpgradeDomain(
                upgradeProgress.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndMoveNextApplicationUpgradeDomain(context.GetRawPointer());
        },
        expectedError);
}

void TestFabricClientUpgrade::MoveNextApplicationUpgradeDomainImpl2(
    wstring const & appName,
    HRESULT expectedError)
{
    ComPointer<IFabricApplicationUpgradeProgressResult3> upgradeProgress;
    UpgradeAppStatusImpl(appName, S_OK, upgradeProgress);

    MoveNextApplicationUpgradeDomainImpl2(
        upgradeProgress->get_ApplicationName(),
        upgradeProgress->get_NextUpgradeDomain(),
        expectedError);
}

void TestFabricClientUpgrade::MoveNextApplicationUpgradeDomainImpl2(
    FABRIC_URI appName,
    LPCWSTR ud,
    HRESULT expectedError)
{
    ComPointer<IFabricApplicationManagementClient2> managementClient = CreateFabricApplicationClient2(FABRICSESSION.FabricDispatcher.Federation);
    
    TestFabricClient::PerformFabricClientOperation(
        L"MoveNextApplicationUpgradeDomain2",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginMoveNextApplicationUpgradeDomain2(
                appName,
                ud,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndMoveNextApplicationUpgradeDomain2(context.GetRawPointer());
        },
        expectedError);
}

void TestFabricClientUpgrade::MoveNextFabricUpgradeDomainImpl(HRESULT expectedError)
{
    ComPointer<IFabricUpgradeProgressResult3> upgradeProgress;
    UpgradeFabricStatusImpl(S_OK, FABRIC_E_TIMEOUT, upgradeProgress);

    auto managementClient = CreateFabricClusterClient2(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"MoveNextFabricUpgradeDomain",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginMoveNextFabricUpgradeDomain(
                upgradeProgress.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndMoveNextFabricUpgradeDomain(context.GetRawPointer());
        },
        expectedError);
}

void TestFabricClientUpgrade::MoveNextFabricUpgradeDomainImpl2(HRESULT expectedError)
{
    ComPointer<IFabricUpgradeProgressResult3> upgradeProgress;
    UpgradeFabricStatusImpl(S_OK, FABRIC_E_TIMEOUT, upgradeProgress);

    MoveNextFabricUpgradeDomainImpl2(
        upgradeProgress->get_NextUpgradeDomain(),
        expectedError);
}

void TestFabricClientUpgrade::MoveNextFabricUpgradeDomainImpl2(LPCWSTR ud, HRESULT expectedError)
{
    ComPointer<IFabricClusterManagementClient2> managementClient = CreateFabricClusterClient2(FABRICSESSION.FabricDispatcher.Federation);

    TestFabricClient::PerformFabricClientOperation(
        L"MoveNextFabricUpgradeDomain2",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->BeginMoveNextFabricUpgradeDomain2(
                ud,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return managementClient->EndMoveNextFabricUpgradeDomain2(context.GetRawPointer());
        },
        expectedError);
}    

UpgradeFailureReason::Enum TestFabricClientUpgrade::ParseUpgradeFailureReason(__in CommandLineParser & parser)
{
    UpgradeFailureReason::Enum failureReason = UpgradeFailureReason::None;
    wstring failureReasonString;
    if (parser.TryGetString(L"failurereason", failureReasonString, L"None"))
    {
        if (StringUtility::AreEqualCaseInsensitive(failureReasonString, L"Interrupted"))
        {
            failureReason = UpgradeFailureReason::Interrupted;
        }
        else if (StringUtility::AreEqualCaseInsensitive(failureReasonString, L"HealthCheck"))
        {
            failureReason = UpgradeFailureReason::HealthCheck;
        }
        else if (StringUtility::AreEqualCaseInsensitive(failureReasonString, L"UpgradeDomainTimeout"))
        {
            failureReason = UpgradeFailureReason::UpgradeDomainTimeout;
        }
        else if (StringUtility::AreEqualCaseInsensitive(failureReasonString, L"OverallUpgradeTimeout"))
        {
            failureReason = UpgradeFailureReason::OverallUpgradeTimeout;
        }
        else
        {
            TestSession::WriteError(TraceSource, "Invalid UpgradeFailureReason: '{0}'", failureReasonString);
        }
    }

    return failureReason;
}
