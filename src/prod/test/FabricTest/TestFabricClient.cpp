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
using namespace Management::InfrastructureService;
using namespace Management::ClusterManager;
using namespace Api;
using namespace Transport;
using namespace Client;

#define CALL_VERSIONED_API( currentVersionClient, Function, BackwardsCompatibleInterface, ... ) \
    if (!FabricTestDispatcher::UseBackwardsCompatibleClients)                                   \
    {                                                                                           \
        return currentVersionClient->Function(__VA_ARGS__);                                     \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        TestSession::WriteNoise(                                                                \
            TraceSource,                                                                        \
            "Using backwards compatible interface: {0}::{1}",                                   \
            #BackwardsCompatibleInterface,                                                      \
            #Function);                                                                         \
        ComPointer<BackwardsCompatibleInterface> backwardsCompatibleClient;                     \
        HRESULT hr = currentVersionClient->QueryInterface(                                      \
            IID_##BackwardsCompatibleInterface,                                                 \
            backwardsCompatibleClient.VoidInitializationAddress());                             \
        if (!SUCCEEDED(hr)) { return hr; }                                                      \
        return backwardsCompatibleClient->Function(__VA_ARGS__);                                \
    }                                                                                           \

const StringLiteral TraceSource("FabricTest.FabricClient");
wstring const TestFabricClient::EmptyValue = L"";

extern bool IsTrue(wstring const & input);

ComPointer<IFabricPropertyManagementClient> CreateFabricPropertyClient(__in FabricTestFederation &);

ComPointer<IFabricPropertyManagementClient2> CreateFabricPropertyClient2(__in FabricTestFederation &);

ComPointer<IFabricServiceManagementClient> CreateFabricServiceClient(__in FabricTestFederation &);

ComPointer<IFabricServiceManagementClient2> CreateFabricServiceClient2(__in FabricTestFederation &);

ComPointer<IFabricServiceManagementClient3> CreateFabricServiceClient3(__in FabricTestFederation &);
ComPointer<IFabricServiceManagementClient4> CreateFabricServiceClient4(__in FabricTestFederation &);
ComPointer<IFabricServiceManagementClient5> CreateFabricServiceClient5(__in FabricTestFederation &);

ComPointer<IFabricServiceGroupManagementClient> CreateFabricServiceGroupClient(__in FabricTestFederation &);

ComPointer<IFabricServiceGroupManagementClient2> CreateFabricServiceGroupClient2(__in FabricTestFederation &);

ComPointer<IFabricApplicationManagementClient4> CreateFabricApplicationClient4(__in FabricTestFederation &);

ComPointer<IFabricQueryClient10> CreateFabricQueryClient10(__in FabricTestFederation &);

ComPointer<IInternalFabricServiceManagementClient2> CreateInternalFabricServiceClient2(__in FabricTestFederation &);

// Create latest version of API by default. The caller should QI to an older version of the API
// if needed for the specific test.
//
ComPointer<IFabricClusterManagementClient6> CreateFabricClusterClient(__in FabricTestFederation &);

HRESULT TryConvertComResult(
    ComPointer<IFabricResolvedServicePartitionResult> const & comResult,
    __out ResolvedServicePartition & cResult);

DWORD GetComTimeout(TimeSpan);

vector<BYTE> Serialize(wstring const & input);

wstring Deserialize(LPBYTE bytes, SIZE_T byteCount);

bool IsRetryableError(HRESULT hr);

#pragma endregion

TestFabricClient::TestFabricClient()
    : testNamingEntries_(),
    clientThreads_(0),
    clientThreadRange_(0),
    threadDoneEvent_(),
    random_(),
    maxClientOperationInterval_(0),
    closed_(false)
{
}

bool TestFabricClient::CreateName(StringCollection const & params)
{
    CHECK_PARAMS(1, 3, FabricTestCommands::CreateNameCommand);

    NamingUri name;
    if (!NamingUri::TryParse(params[0], name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    vector<HRESULT> expectedErrors(1, S_OK);
    if (params.size() >= 2)
    {
        expectedErrors.clear();
        vector<wstring> errorCodeStrings;
        StringUtility::Split<wstring>(params[1], errorCodeStrings, L",", true);
        for (auto const & errorCodeString : errorCodeStrings)
        {
            ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorCodeString);
            expectedErrors.push_back(ErrorCode(error).ToHResult());
        }
    }

    CommandLineParser parser(params, 1);

    wstring clientName;
    parser.TryGetString(L"client", clientName, L"");

    CreateName(name, clientName, expectedErrors);
    return true;
}

bool TestFabricClient::DeleteName(StringCollection const & params)
{
    CHECK_PARAMS(1, 3, FabricTestCommands::DeleteNameCommand);

    NamingUri name;
    if (!NamingUri::TryParse(params[0], name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    HRESULT expectedError = S_OK;
    if (params.size() >= 2)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[1]);
        expectedError = ErrorCode(error).ToHResult();
    }

    CommandLineParser parser(params, 1);

    wstring clientName;
    parser.TryGetString(L"client", clientName, L"");

    DeleteName(name, clientName, expectedError);
    return true;
}

bool TestFabricClient::NameExists(StringCollection const & params)
{
    CHECK_PARAMS(2, 4, FabricTestCommands::NameExistsCommand);

    NamingUri name;
    if (!NamingUri::TryParse(params[0], name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    bool expectedExists = IsTrue(params[1]);

    HRESULT expectedError = S_OK;
    if (params.size() >= 3)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[2]);
        expectedError = ErrorCode(error).ToHResult();
    }

    CommandLineParser parser(params, 1);

    wstring clientName;
    parser.TryGetString(L"client", clientName, L"");

    NameExists(name, expectedExists, clientName, expectedError);
    return true;
}

bool TestFabricClient::PutProperty(StringCollection const & params)
{
    if(params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::PutPropertyCommand);
        return false;
    }

    NamingUri namingUri;
    if(!NamingUri::TryParse(params[0], namingUri))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    wstring propertyName = params[1];
    wstring data = params[2];

    CommandLineParser parser(params, 1);

    std::wstring error;
    parser.TryGetString(L"error", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    PutProperty(namingUri, propertyName, data, expectedError);
    return true;
}

bool TestFabricClient::PutCustomProperty(StringCollection const & params)
{
    if(params.size() < 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::PutCustomPropertyCommand);
        return false;
    }

    NamingUri namingUri;
    if(!NamingUri::TryParse(params[0], namingUri))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    wstring propertyName = params[1];
    wstring value = params[2];
    wstring customTypeId = params[3];

    CommandLineParser parser(params, 1);

    std::wstring error;
    parser.TryGetString(L"error", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    vector<BYTE> bytes = Serialize(value);

    TestSession::WriteNoise(
        TraceSource,
        "PutCustomProperty called for name {0}, expected error {1}, propertyName {2}, value {3}, customTypeId '{4}'",
        namingUri,
        expectedError,
        propertyName,
        value,
        customTypeId);

    FABRIC_OPERATION_DATA_BUFFER dataBuffer = {0};
    dataBuffer.BufferSize = static_cast<ULONG>(bytes.size());
    dataBuffer.Buffer = bytes.data();

    FABRIC_PUT_CUSTOM_PROPERTY_OPERATION customPutOperation = {0};
    customPutOperation.PropertyName = propertyName.c_str();
    customPutOperation.PropertyTypeId = FABRIC_PROPERTY_TYPE_BINARY;
    customPutOperation.PropertyValue = reinterpret_cast<void*>(&dataBuffer);
    customPutOperation.PropertyCustomTypeId = customTypeId.c_str();

    if (!PutCustomProperty(namingUri, customPutOperation, expectedError))
    {
        return false;
    }

    FABRICSESSION.FabricDispatcher.NamingState.AddProperty(
        namingUri,
        propertyName,
        FABRIC_PROPERTY_TYPE_BINARY,
        bytes,
        customTypeId);
    return true;
}

bool TestFabricClient::SubmitPropertyBatch(StringCollection const & params)
{
    CHECK_PARAMS(2, 999, FabricTestCommands::SubmitPropertyBatchCommand);

    NamingUri namingUri;
    if(!NamingUri::TryParse(params[0], namingUri))
    {
        TestSession::WriteError(
            TraceSource,
            "SubmitBatch: Could not parse name {0}.", params[0]);
        return false;
    }

    CommandLineParser parser(params, 1);
    std::wstring batchError;
    parser.TryGetString(L"error", batchError, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(batchError);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    ULONG expectedFailedIndex;
    parser.TryGetULONG(L"failedindex", expectedFailedIndex, static_cast<ULONG>(-1));

    StringCollection batchOperations;
    if (!parser.GetVector(L"ops", batchOperations))
    {
        TestSession::WriteError(
            TraceSource,
            "SubmitBatch: At least one operation must be specified: {0}.", params);
        return false;
    }

    StringCollection expectedGetProperties;
    parser.GetVector(L"result", expectedGetProperties);

    wstring clientName;
    parser.TryGetString(L"client", clientName, L"");

    return SubmitPropertyBatch(namingUri, clientName, batchOperations, expectedGetProperties, expectedFailedIndex, expectedError);
}

bool TestFabricClient::DeleteProperty(StringCollection const & params)
{
    if(params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeletePropertyCommand);
        return false;
    }

    NamingUri namingUri;
    if(!NamingUri::TryParse(params[0], namingUri))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    wstring propertyName = params[1];

    CommandLineParser parser(params, 1);

    std::wstring error;
    parser.TryGetString(L"error", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    DeleteProperty(namingUri, propertyName, expectedError);
    return true;
}

bool TestFabricClient::GetProperty(StringCollection const & params)
{
    if(params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::GetPropertyCommand);
        return false;
    }

    NamingUri namingUri;
    if(!NamingUri::TryParse(params[0], namingUri))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    wstring propertyName = params[1];

    wstring expectedData;
    if (params.size() > 2)
    {
        expectedData = params[2];
    }

    CommandLineParser parser(params, 1);

    wstring error;
    parser.TryGetString(L"error", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    wstring expectedCustomTypeId;
    parser.TryGetString(L"customTypeId", expectedCustomTypeId, L"");

    wstring seqVarName;
    parser.TryGetString(L"seqvar", seqVarName, L"");

    GetProperty(namingUri, propertyName, expectedData, expectedCustomTypeId, seqVarName, expectedError);

    return true;
}

bool TestFabricClient::GetMetadata(StringCollection const & params)
{
    if(params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::GetMetadataCommand);
        return false;
    }

    NamingUri namingUri;
    if(!NamingUri::TryParse(params[0], namingUri))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    wstring propertyName = params[1];

    CommandLineParser parser(params, 1);

    std::wstring error;
    parser.TryGetString(L"error", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    std::wstring expectedCustomTypeId;
    parser.TryGetString(L"customTypeId", expectedCustomTypeId, L"");

    ComPointer<IFabricPropertyMetadataResult> propertyResult;
    GetMetadata(namingUri, propertyName, expectedError, propertyResult);

    if (SUCCEEDED(expectedError))
    {
        CheckMetadata(propertyResult->get_Metadata(), propertyName, expectedCustomTypeId);
    }

    return true;
}

bool TestFabricClient::EnumerateProperties(StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::EnumeratePropertiesCommand);
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

    bool includeValues = parser.GetBool(L"includeValues");

    int maxResults;
    parser.TryGetInt(L"maxResults", maxResults, numeric_limits<int>::max());

    bool bestEffort = parser.GetBool(L"bestEffort");
    bool moreData = parser.GetBool(L"moreData");

    FABRIC_ENUMERATION_STATUS status = (bestEffort ? FABRIC_ENUMERATION_BEST_EFFORT_MASK : FABRIC_ENUMERATION_CONSISTENT_MASK);
    status = static_cast<FABRIC_ENUMERATION_STATUS>(status & (moreData ? FABRIC_ENUMERATION_MORE_DATA_MASK : FABRIC_ENUMERATION_FINISHED_MASK));

    std::wstring error;
    parser.TryGetString(L"error", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    EnumerateProperties(
        name,
        includeValues,
        maxResults,
        status,
        expectedError);

    return true;
}

bool TestFabricClient::VerifyPropertyEnumeration(StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyPropertyEnumerationCommand);
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

    bool includeValues = parser.GetBool(L"includeValues");

    VerifyPropertyEnumeration(name, includeValues);

    return true;
}

bool TestFabricClient::EnumerateNames(StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::EnumerateNamesCommand);
        return false;
    }

    NamingUri parentName;
    if (params.size() > 0)
    {
        if (!NamingUri::TryParse(params[0], parentName))
        {
            TestSession::WriteError(
                TraceSource,
                "Could not parse name {0}.", params[0]);
            return false;
        }
    }

    CommandLineParser parser(params, 1);

    bool doRecursive = parser.GetBool(L"recursive");

    int expectedCount;
    parser.TryGetInt(L"expectedCount", expectedCount, -1);

    StringCollection expectedNames;
    parser.GetVector(L"expectedNames", expectedNames);

    bool checkExpectedNames = parser.GetBool(L"verifyExpectedNames");

    std::wstring error;
    parser.TryGetString(L"error", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    int maxResults;
    parser.TryGetInt(L"maxResults", maxResults, numeric_limits<int>::max());

    bool bestEffort = parser.GetBool(L"bestEffort");
    bool moreData = parser.GetBool(L"moreData");

    FABRIC_ENUMERATION_STATUS expectedStatus = (bestEffort ? FABRIC_ENUMERATION_BEST_EFFORT_MASK : FABRIC_ENUMERATION_CONSISTENT_MASK);
    expectedStatus = static_cast<FABRIC_ENUMERATION_STATUS>(expectedStatus & (moreData ? FABRIC_ENUMERATION_MORE_DATA_MASK : FABRIC_ENUMERATION_FINISHED_MASK));

    wstring clientName;
    parser.TryGetString(L"client", clientName, L"");

    int nameCount = EnumerateNames(
        parentName,
        doRecursive,
        clientName,
        maxResults,
        checkExpectedNames,
        expectedNames,
        true,
        expectedStatus,
        expectedError);

    TestSession::WriteInfo(TraceSource, "Total names: {0}", nameCount);
    if (expectedCount >= 0)
    {
        TestSession::FailTestIfNot(expectedCount == nameCount, "Enumerated {0} subnames. Expected {1}", nameCount, expectedCount);
    }

    return true;
}


bool TestFabricClient::CreateService(StringCollection const & params)
{
    if (params.size() < 5)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::CreateServiceCommand);
        return false;
    }

    NamingUri name;
    wstring nameString;
    if (!NamingUri::TryParse(params[0], name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    ScopedHeap heap;

    nameString = name.ToString();

    wstring type = params[1];
    bool isStateful = IsTrue(params[2]);
    int partitionCount = Common::Int32_Parse(params[3]);
    int instanceCount = Common::Int32_Parse(params[4]);

    if (instanceCount == -1 && isStateful)
    {
        TestSession::WriteError(
            TraceSource,
            "Invalid -1 replica count for stateful service.");
        return false;
    }

    int targetReplicaSetSize = instanceCount;

    CommandLineParser parser(params, 5);

    wstring clientName;
    parser.TryGetString(L"client", clientName, L"");
    int fabricTimePersistInterval;
    if( parser.TryGetInt(L"fabrictimepersistinterval",fabricTimePersistInterval,0) )
    {
        Store::StoreConfig::GetConfig().FabricTimePersistInterval=TimeSpan::FromMilliseconds(fabricTimePersistInterval);
    }

    ComPointer<IFabricServiceManagementClient> client;
    FABRICSESSION.FabricDispatcher.TryGetNamedFabricClient(clientName, IID_IFabricServiceManagementClient, client);

    wstring applicationNameString;
    parser.TryGetString(L"appname", applicationNameString, L"");
    if(!applicationNameString.empty())
    {
        NamingUri applicationName;
        if (!NamingUri::TryParse(applicationNameString, applicationName))
        {
            TestSession::WriteError(
                TraceSource,
                "Could not parse name {0}.", params[0]);
            return false;
        }
    }

    bool hasPersistedState = parser.GetBool(L"persist") && isStateful;

    int replicaRestartWaitDurationSeconds;
    parser.TryGetInt(L"replicarestartwaitduration", replicaRestartWaitDurationSeconds, -1);

    int quorumLossWaitDurationSeconds;
    parser.TryGetInt(L"quorumlosswaitduration", quorumLossWaitDurationSeconds, -1);

    int standByReplicaKeepDurationSeconds;
    parser.TryGetInt(L"standbyreplicakeepduration", standByReplicaKeepDurationSeconds, -1);

    vector<ServiceCorrelationDescription> serviceCorrelations;
    StringCollection correlationStrings;
    parser.GetVector(L"servicecorrelations", correlationStrings);
    if(correlationStrings.size() > 0 && !GetCorrelations(correlationStrings, serviceCorrelations))
    {
        return false;
    }

    vector<ServiceModel::ServicePlacementPolicyDescription> placementPolicies;
    StringCollection policiesStrings;
    parser.GetVector(L"placementPolicies", policiesStrings);
    if(policiesStrings.size() > 0 && !GetPlacementPolicies(policiesStrings, placementPolicies))
    {
        return false;
    }

    wstring placementConstraints;
    parser.TryGetString(L"constraint", placementConstraints, FabricTestDispatcher::DefaultPlacementConstraints);
    placementConstraints.erase(std::remove(placementConstraints.begin(), placementConstraints.end(), L'\\'), placementConstraints.end());

    vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies;
    wstring scalingPolicy;
    parser.TryGetString(L"scalingPolicy", scalingPolicy, L"");
    if (!scalingPolicy.empty() && !GetServiceScalingPolicy(scalingPolicy, scalingPolicies))
    {
        return false;
    }

    bool overrideConstraints = parser.GetBool(L"xconstraint");

    int scaleoutCount;
    parser.TryGetInt(L"scalecount", scaleoutCount, FabricTestDispatcher::DefaultScaleoutCount);

    int minReplicaSetSize;
    parser.TryGetInt(L"minreplicasetsize", minReplicaSetSize, 1);

    vector<Reliability::ServiceLoadMetricDescription> metrics;
    StringCollection metricStrings;
    parser.GetVector(L"metrics", metricStrings);
    if(!metricStrings.empty() && !GetServiceMetrics(metricStrings, metrics, isStateful))
    {
        return false;
    }

    bool isDefaultMoveCostSpecified;
    ULONG defaultMoveCost;
    isDefaultMoveCostSpecified = parser.TryGetULONG(L"defaultmovecost", defaultMoveCost, 0);

    vector<wstring> errorStrings;
    parser.GetVector(L"errors", errorStrings, L"Success");
    vector<HRESULT> expectedErrors;
    for (auto iter = errorStrings.begin(); iter != errorStrings.end(); ++iter)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(*iter);
        expectedErrors.push_back(ErrorCode(error).ToHResult());
    }

    FABRIC_PARTITION_SCHEME partitionScheme = partitionCount == 0 ? FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON : FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE;
    // normalize partition count like the managed layer does
    partitionCount = partitionCount == 0 ? 1 : partitionCount;

    StringCollection partitionNameStrings;
    parser.GetVector(L"partitionnames", partitionNameStrings);
    vector<LPCWSTR> partitionNames;
    if (partitionNameStrings.size() > 0)
    {
        if (partitionNameStrings.size() != static_cast<size_t>(partitionCount))
        {
            return false;
        }
        partitionScheme = FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED;
        for (size_t i = 0; i < partitionNameStrings.size(); i++)
        {
            partitionNames.push_back(&(partitionNameStrings[i][0]));
        }
    }

    int64 lowRange;
    parser.TryGetInt64(L"lowRange", lowRange, FabricTestSessionConfig::GetConfig().ServiceDescriptorLowRange);

    int64 highRange;
    parser.TryGetInt64(L"highRange", highRange, FabricTestSessionConfig::GetConfig().ServiceDescriptorHighRange);

    bool forceUpdatePsd = parser.GetBool(L"forceUpdatePsd");

    // Processing initialization data. If provided then it will be used while creating FABRIC_STATEFUL/STATELESS_SERVICE_DESCRIPTION.
    wstring initDataStr;

    // Parsing initialization data
    parser.TryGetString(L"initdata", initDataStr, L"");

    // serialization of initData
    vector<byte> serializedInitData(0);
    StringBody strBody(initDataStr);
    FabricSerializer::Serialize(&strBody, serializedInitData);
    TestSession::WriteNoise(TraceSource, "initialization-data: provided: {0}, serialized vector size:{1}", initDataStr, serializedInitData.size());

    // servicePackageActivationMode
    wstring servicePackageActivationModeStr;
    parser.TryGetString(L"servicePackageActivationMode", servicePackageActivationModeStr, L"SharedProcess");

    ServiceModel::ServicePackageActivationMode::Enum activationMode;
    auto error = ServiceModel::ServicePackageActivationMode::EnumFromString(servicePackageActivationModeStr, activationMode);
    if (!error.IsSuccess())
    {
        TestSession::WriteError(TraceSource, "Invalid ServicePackageActivationMode specified = {0}", servicePackageActivationModeStr);
        return false;
    }

    // serviceDnsName
    wstring serviceDnsName;
    parser.TryGetString(L"serviceDnsName", serviceDnsName);

    FABRIC_SERVICE_DESCRIPTION serviceDescription;

    FABRIC_STATEFUL_SERVICE_DESCRIPTION statefulDescription = {0};
    FABRIC_STATELESS_SERVICE_DESCRIPTION statelessDescription = {0};

    FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1 statefulEx1 = {0};
    FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1 statelessEx1 = {0};

    FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2 statefulEx2 = { 0 };
    FABRIC_STATELESS_SERVICE_DESCRIPTION_EX2 statelessEx2 = { 0 };

    FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX3 statefulEx3 = { FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE_SHARED_PROCESS };
    FABRIC_STATELESS_SERVICE_DESCRIPTION_EX3 statelessEx3 = { FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE_SHARED_PROCESS };

    FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX4 statefulEx4 = { 0 };
    FABRIC_STATELESS_SERVICE_DESCRIPTION_EX4 statelessEx4 = { 0 };

    FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS failoverSettings = {0};
    FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_EX1 failoverSettingsEx1 = {0};

    union
    {
        FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION uniformInt64;
        FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION named;
    } partitionDescription;
    vector<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION> metricDescriptions;
    metricDescriptions.resize(metrics.size());
    for (int i = 0; i < static_cast<int>(metrics.size()); i++)
    {
        metricDescriptions[i].Name = metrics[i].Name.c_str();
        metricDescriptions[i].Weight = metrics[i].Weight;
        metricDescriptions[i].PrimaryDefaultLoad = metrics[i].PrimaryDefaultLoad;
        metricDescriptions[i].SecondaryDefaultLoad = metrics[i].SecondaryDefaultLoad;
    }

    vector<FABRIC_SERVICE_CORRELATION_DESCRIPTION> serviceCorrelationDescriptions;
    serviceCorrelationDescriptions.resize(serviceCorrelations.size());
    for (int i = 0; i < static_cast<int>(serviceCorrelations.size()); i++)
    {
        serviceCorrelationDescriptions[i].ServiceName = serviceCorrelations[i].ServiceName.c_str();
        serviceCorrelationDescriptions[i].Scheme = serviceCorrelations[i].Scheme;
    }

    FABRIC_SERVICE_PLACEMENT_POLICY_LIST pList;
    vector<FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION> policyDescriptions;
    policyDescriptions.resize(placementPolicies.size());

    vector<FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN_DESCRIPTION> invalidDomain;
    invalidDomain.resize(placementPolicies.size());
    vector<FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DESCRIPTION> requiredDomain;
    requiredDomain.resize(placementPolicies.size());
    vector<FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN_DESCRIPTION> preferredPrimaryDomain;
    preferredPrimaryDomain.resize(placementPolicies.size());
    vector<FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION_DESCRIPTION> requiredDist;
    requiredDist.resize(placementPolicies.size());
    vector<FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE_DESCRIPTION> nonPartially;
    nonPartially.resize(placementPolicies.size());

    int invalidDomainIdx(0);
    int requiredDomainIdx(0);
    int preferredPrimaryDomainIdx(0);
    int requiredDistIdx(0);
    int nonPartiallyIdx(0);

    for (int i = 0; i < static_cast<int>(placementPolicies.size()); i++)
    {
        policyDescriptions[i].Type = placementPolicies[i].Type;
        switch (placementPolicies[i].Type)
        {
        case FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN:
            {
                invalidDomain[invalidDomainIdx].InvalidFaultDomain = placementPolicies[i].DomainName.c_str();
                policyDescriptions[i].Value = &invalidDomain[invalidDomainIdx];
                invalidDomainIdx++;
                break;
            }
        case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN:
            {
                requiredDomain[requiredDomainIdx].RequiredFaultDomain = placementPolicies[i].DomainName.c_str();
                policyDescriptions[i].Value = &requiredDomain[requiredDomainIdx];
                requiredDomainIdx++;
                break;
            }
        case FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN:
            {
                preferredPrimaryDomain[preferredPrimaryDomainIdx].PreferredPrimaryFaultDomain = placementPolicies[i].DomainName.c_str();
                policyDescriptions[i].Value = &preferredPrimaryDomain[preferredPrimaryDomainIdx];
                preferredPrimaryDomainIdx++;
                break;
            }
        case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION:
            {
                policyDescriptions[i].Value = &requiredDist[requiredDistIdx];
                requiredDistIdx++;
                break;
            }
        case FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE:
            {
                policyDescriptions[i].Value = &nonPartially[nonPartiallyIdx];
                nonPartiallyIdx++;
                break;
            }
        default:
            policyDescriptions[i].Type = FABRIC_PLACEMENT_POLICY_INVALID;
        }
    }

    pList.PolicyCount = static_cast<ULONG>(policyDescriptions.size());
    pList.Policies = policyDescriptions.data();


    if (isStateful)
    {
        serviceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        serviceDescription.Value = &statefulDescription;
        statefulDescription.ServiceName = nameString.c_str();
        statefulDescription.ApplicationName = applicationNameString.c_str();
        statefulDescription.ServiceTypeName = type.c_str();
        statefulDescription.InitializationData =(BYTE*)(serializedInitData.data());
        statefulDescription.InitializationDataSize = static_cast<ULONG>(serializedInitData.size());
        statefulDescription.PartitionScheme = partitionScheme;
        statefulDescription.PartitionSchemeDescription = &partitionDescription;
        statefulDescription.TargetReplicaSetSize = targetReplicaSetSize;
        statefulDescription.MinReplicaSetSize = minReplicaSetSize;
        statefulDescription.PlacementConstraints = (overrideConstraints ? NULL : placementConstraints.c_str());
        statefulDescription.CorrelationCount = static_cast<ULONG>(serviceCorrelationDescriptions.size());
        statefulDescription.Correlations = serviceCorrelationDescriptions.data();
        statefulDescription.MetricCount = static_cast<ULONG>(metrics.size());
        statefulDescription.Metrics = metricDescriptions.data();
        statefulDescription.HasPersistedState = hasPersistedState;
        statefulDescription.Reserved = &statefulEx1;

        statefulEx1.PolicyList = (pList.PolicyCount ? &pList : NULL);

        failoverSettings.Reserved = &failoverSettingsEx1;
        statefulEx1.FailoverSettings = &failoverSettings;

        if (replicaRestartWaitDurationSeconds != -1)
        {
            failoverSettings.Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION;
            failoverSettings.ReplicaRestartWaitDurationSeconds = replicaRestartWaitDurationSeconds;
        }

        if (quorumLossWaitDurationSeconds != -1)
        {
            failoverSettings.Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION;
            failoverSettings.QuorumLossWaitDurationSeconds = quorumLossWaitDurationSeconds;
        }

        if (standByReplicaKeepDurationSeconds != -1)
        {
            failoverSettings.Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION;
            failoverSettingsEx1.StandByReplicaKeepDurationSeconds = standByReplicaKeepDurationSeconds;
        }

        statefulEx1.Reserved = &statefulEx2;
        statefulEx2.IsDefaultMoveCostSpecified = isDefaultMoveCostSpecified;
        statefulEx2.DefaultMoveCost = static_cast<FABRIC_MOVE_COST>(defaultMoveCost);

        statefulEx2.Reserved = &statefulEx3;
        statefulEx3.ServicePackageActivationMode = ServiceModel::ServicePackageActivationMode::ToPublicApi(activationMode);
        statefulEx3.ServiceDnsName = serviceDnsName.c_str();

        statefulEx3.Reserved = &statefulEx4;
        if (scalingPolicies.size() == 1)
        {
            statefulEx4.ScalingPolicyCount = 1;
            auto nativeScalingPolicies = heap.AddArray<FABRIC_SERVICE_SCALING_POLICY>(1);
            scalingPolicies[0].ToPublicApi(heap, nativeScalingPolicies[0]);
            statefulEx4.ServiceScalingPolicies = nativeScalingPolicies.GetRawArray();
        }
        else
        {
            statefulEx4.ScalingPolicyCount = 0;
            statefulEx4.ServiceScalingPolicies = NULL;
        }
    }
    else
    {
        serviceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        serviceDescription.Value = &statelessDescription;
        statelessDescription.ServiceName = nameString.c_str();
        statelessDescription.ApplicationName = applicationNameString.c_str();
        statelessDescription.ServiceTypeName = type.c_str();
        statelessDescription.InitializationData = (BYTE*)(serializedInitData.data());
        statelessDescription.InitializationDataSize = static_cast<ULONG>(serializedInitData.size());
        statelessDescription.PartitionScheme = partitionScheme;
        statelessDescription.PartitionSchemeDescription = &partitionDescription;
        statelessDescription.InstanceCount = targetReplicaSetSize;
        statelessDescription.PlacementConstraints = (overrideConstraints ? NULL : placementConstraints.c_str());
        statelessDescription.CorrelationCount = static_cast<ULONG>(serviceCorrelationDescriptions.size());
        statelessDescription.Correlations = serviceCorrelationDescriptions.data();
        statelessDescription.MetricCount = static_cast<ULONG>(metrics.size());
        statelessDescription.Metrics = metricDescriptions.data();

        statelessDescription.Reserved = &statelessEx1;
        statelessEx1.PolicyList = (pList.PolicyCount ? &pList : NULL);

        statelessEx1.Reserved = &statelessEx2;
        statelessEx2.IsDefaultMoveCostSpecified = isDefaultMoveCostSpecified;
        statelessEx2.DefaultMoveCost = static_cast<FABRIC_MOVE_COST>(defaultMoveCost);
        
        statelessEx2.Reserved = &statelessEx3;
        statelessEx3.ServicePackageActivationMode = ServiceModel::ServicePackageActivationMode::ToPublicApi(activationMode);
        statelessEx3.ServiceDnsName = serviceDnsName.c_str();

        statelessEx3.Reserved = &statelessEx4;
        if (scalingPolicies.size() == 1)
        {
            statelessEx4.ScalingPolicyCount = 1;
            auto nativeScalingPolicies = heap.AddArray<FABRIC_SERVICE_SCALING_POLICY>(1);
            scalingPolicies[0].ToPublicApi(heap, nativeScalingPolicies[0]);
            statelessEx4.ServiceScalingPolicies = nativeScalingPolicies.GetRawArray();
        }
        else
        {
            statelessEx4.ScalingPolicyCount = 0;
            statelessEx4.ServiceScalingPolicies = NULL;
        }
    }

    switch (partitionScheme)
    {
    case FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
        partitionDescription.uniformInt64.PartitionCount = partitionCount;
        partitionDescription.uniformInt64.LowKey = lowRange;
        partitionDescription.uniformInt64.HighKey = highRange;
        break;
    case FABRIC_PARTITION_SCHEME_NAMED:
        partitionDescription.named.PartitionCount = partitionCount;
        partitionDescription.named.Names = partitionNames.data();
        break;
    }

    CreateService(serviceDescription, expectedErrors, client);

    if(find(expectedErrors.begin(), expectedErrors.end(), S_OK) != expectedErrors.end() || forceUpdatePsd)
    {
        PartitionedServiceDescriptor serviceDescriptor;

        if (client)
        {
            GetPartitionedServiceDescriptor(nameString, client, serviceDescriptor);
        }
        else
        {
            GetPartitionedServiceDescriptor(nameString, serviceDescriptor);
        }

        FABRICSESSION.FabricDispatcher.ServiceMap.AddPartitionedServiceDescriptor(nameString, move(serviceDescriptor), forceUpdatePsd);
    }

    return true;
}

bool TestFabricClient::UpdateService(StringCollection const & params)
{
    if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::UpdateServiceCommand);
        return false;
    }

    NamingUri serviceNameUri;
    if (!NamingUri::TryParse(params[0], serviceNameUri))
    {
        TestSession::WriteError(TraceSource, "Could not parse service name {0}.", params[0]);
        return false;
    }

    wstring serviceName = serviceNameUri.ToString();
    wstring internalServiceName = ServiceModel::SystemServiceApplicationNameHelper::GetInternalServiceName(serviceName);

    auto serviceDescription = FABRICSESSION.FabricDispatcher.ServiceMap.GetServiceDescriptor(internalServiceName).Service;

    if (params[1] != L"Stateless" && params[1] != L"Stateful")
    {
        TestSession::WriteError(TraceSource, "Could not parse service kind {0}.", params[1]);
        return false;
    }

    bool isStateful = (params[1] == L"Stateful");

    CommandLineParser parser(params, 2);

    ScopedHeap heap;

    FABRIC_SERVICE_UPDATE_DESCRIPTION serviceUpdateDescription;
    FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION statefulUpdateDescription = {0};
    FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX1 statefulUpdateDescriptionEx1 = {0};
    FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX2 statefulUpdateDescriptionEx2 = {0};
    FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX3 statefulUpdateDescriptionEx3 = {0};
    FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX4 statefulUpdateDescriptionEx4 = {static_cast<FABRIC_MOVE_COST>(0), 0};
    FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION statelessUpdateDescription = {0};
    FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_EX1 statelessUpdateDescriptionEx1 = { 0 };
    FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_EX2 statelessUpdateDescriptionEx2 = { static_cast<FABRIC_MOVE_COST>(0), 0  };
    FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_EX3 statelessUpdateDescriptionEx3 = { FABRIC_SERVICE_PARTITION_KIND_INVALID, 0, 0, 0, 0 };

    vector<ServiceCorrelationDescription> serviceCorrelations;
    vector<FABRIC_SERVICE_CORRELATION_DESCRIPTION> serviceCorrelationDescriptions;
    vector<ServiceModel::ServicePlacementPolicyDescription> placementPolicies;
    wstring placementConstraints;
    vector<Reliability::ServiceLoadMetricDescription> metrics;
    vector<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION> metricDescriptions;
    FABRIC_SERVICE_PLACEMENT_POLICY_LIST pList;
    vector<FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION> policyDescriptions;
    vector<FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN_DESCRIPTION> invalidDomain;
    vector<FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DESCRIPTION> requiredDomain;
    vector<FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN_DESCRIPTION> preferredPrimaryDomain;
    vector<FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION_DESCRIPTION> requiredDist;
    vector<FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE_DESCRIPTION> nonPartially;

    wstring delimiter = L",";

    wstring addedNamesString;
    vector<wstring> addedNames;
    parser.TryGetString(L"addednames", addedNamesString, L""); 
    StringUtility::Split(addedNamesString, addedNames, delimiter);

    wstring removedNamesString;
    vector<wstring> removedNames;
    parser.TryGetString(L"removednames", removedNamesString, L""); 
    StringUtility::Split(removedNamesString, removedNames, delimiter);

    vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies;

    if (isStateful)
    {
        serviceUpdateDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        serviceUpdateDescription.Value = &statefulUpdateDescription;
        statefulUpdateDescription.Reserved = &statefulUpdateDescriptionEx1;
        statefulUpdateDescriptionEx1.Reserved = &statefulUpdateDescriptionEx2;
        statefulUpdateDescriptionEx2.Reserved = &statefulUpdateDescriptionEx3;
        statefulUpdateDescriptionEx3.Reserved = &statefulUpdateDescriptionEx4;

        int targetReplicaSetSize;
        if (parser.TryGetInt(L"TargetReplicaSetSize", targetReplicaSetSize, -1))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_TARGET_REPLICA_SET_SIZE;
            statefulUpdateDescription.TargetReplicaSetSize = targetReplicaSetSize;

            serviceDescription.TargetReplicaSetSize = targetReplicaSetSize;
        }

        int replicaRestartWaitDurationSeconds;
        if (parser.TryGetInt(L"ReplicaRestartWaitDuration", replicaRestartWaitDurationSeconds, -1))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_REPLICA_RESTART_WAIT_DURATION;
            statefulUpdateDescription.ReplicaRestartWaitDurationSeconds = replicaRestartWaitDurationSeconds;

            serviceDescription.ReplicaRestartWaitDuration = TimeSpan::FromSeconds(replicaRestartWaitDurationSeconds);
        }

        int quorumLossWaitDurationSeconds;
        if (parser.TryGetInt(L"QuorumLossWaitDuration", quorumLossWaitDurationSeconds, -1))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_QUORUM_LOSS_WAIT_DURATION;
            statefulUpdateDescription.QuorumLossWaitDurationSeconds = quorumLossWaitDurationSeconds;

            serviceDescription.QuorumLossWaitDuration = TimeSpan::FromSeconds(quorumLossWaitDurationSeconds);
        }

        int standByReplicaKeepDurationSeconds;
        if (parser.TryGetInt(L"StandByReplicaKeepDuration", standByReplicaKeepDurationSeconds, -1))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_STANDBY_REPLICA_KEEP_DURATION;
            statefulUpdateDescriptionEx1.StandByReplicaKeepDurationSeconds = standByReplicaKeepDurationSeconds;

            serviceDescription.StandByReplicaKeepDuration = TimeSpan::FromSeconds(standByReplicaKeepDurationSeconds);
        }

        int minReplicaSetSize;
        if (parser.TryGetInt(L"MinReplicaSetSize", minReplicaSetSize, -1))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_MIN_REPLICA_SET_SIZE;
            statefulUpdateDescriptionEx2.MinReplicaSetSize = minReplicaSetSize;

            serviceDescription.MinReplicaSetSize = minReplicaSetSize;
        }

        if (parser.TryGetString(L"constraint", placementConstraints, FabricTestDispatcher::DefaultPlacementConstraints))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_PLACEMENT_CONSTRAINTS;
            statefulUpdateDescriptionEx3.PlacementConstraints = placementConstraints.c_str();
            serviceDescription.PlacementConstraints = move(wstring(placementConstraints));
        }

        wstring metricString;
        StringCollection metricStrings;
        if (parser.TryGetString(L"metrics", metricString))
        {
            if (!metricString.empty())
            {
                StringUtility::Split<wstring>(metricString, metricStrings, L",");
                if (!GetServiceMetrics(metricStrings, metrics, isStateful))
                {
                    return false;
                }
            }
            metricDescriptions.resize(metrics.size());
            for (int i = 0; i < static_cast<int>(metrics.size()); i++)
            {
                metricDescriptions[i].Name = metrics[i].Name.c_str();
                metricDescriptions[i].Weight = metrics[i].Weight;
                metricDescriptions[i].PrimaryDefaultLoad = metrics[i].PrimaryDefaultLoad;
                metricDescriptions[i].SecondaryDefaultLoad = metrics[i].SecondaryDefaultLoad;
            }
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_METRICS;

            statefulUpdateDescriptionEx3.MetricCount = static_cast<ULONG>(metrics.size());
            statefulUpdateDescriptionEx3.Metrics = metricDescriptions.data();
            serviceDescription.Metrics = move(metrics);
        }

        ULONG defaultMoveCost;
        if (parser.TryGetULONG(L"defaultmovecost", defaultMoveCost, 0))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_MOVE_COST;
            statefulUpdateDescriptionEx4.DefaultMoveCost = static_cast<FABRIC_MOVE_COST>(defaultMoveCost);
            serviceDescription.DefaultMoveCost = static_cast<FABRIC_MOVE_COST>(defaultMoveCost);
        }

        wstring correlationString;
        StringCollection correlationStrings;
        if (parser.TryGetString(L"servicecorrelations", correlationString))
        {
            if (!correlationString.empty())
            {
                StringUtility::Split<wstring>(correlationString, correlationStrings, L",");
                if (!GetCorrelations(correlationStrings, serviceCorrelations))
                {
                    return false;
                }
            }
            serviceCorrelationDescriptions.resize(serviceCorrelations.size());
            for (int i = 0; i < static_cast<int>(serviceCorrelations.size()); i++)
            {
                serviceCorrelationDescriptions[i].ServiceName = serviceCorrelations[i].ServiceName.c_str();
                serviceCorrelationDescriptions[i].Scheme = serviceCorrelations[i].Scheme;
            }
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_CORRELATIONS;


            statefulUpdateDescriptionEx3.CorrelationCount = static_cast<ULONG>(serviceCorrelationDescriptions.size());
            statefulUpdateDescriptionEx3.Correlations = serviceCorrelationDescriptions.data();
            serviceDescription.Correlations = move(serviceCorrelations);
        }

        wstring policiesString;
        StringCollection policiesStrings;
        if (parser.TryGetString(L"placementPolicies", policiesString))
        {
            if (!policiesString.empty())
            {
                StringUtility::Split<wstring>(policiesString, policiesStrings, L",");

                if (!GetPlacementPolicies(policiesStrings, placementPolicies))
                {
                    return false;
                }
            }

            policyDescriptions.resize(placementPolicies.size());
            pList.PolicyCount = static_cast<ULONG>(policyDescriptions.size());
            pList.Policies = policyDescriptions.data();
            policyDescriptions.resize(placementPolicies.size());
            requiredDomain.resize(placementPolicies.size());
            preferredPrimaryDomain.resize(placementPolicies.size());
            invalidDomain.resize(placementPolicies.size());
            requiredDist.resize(placementPolicies.size());
            nonPartially.resize(placementPolicies.size());

            int invalidDomainIdx(0);
            int requiredDomainIdx(0);
            int preferredPrimaryDomainIdx(0);
            int requiredDistIdx(0);
            int nonPartiallyIdx(0);

            for (int i = 0; i < static_cast<int>(placementPolicies.size()); i++)
            {
                policyDescriptions[i].Type = placementPolicies[i].Type;
                switch (placementPolicies[i].Type)
                {
                    case FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN:
                    {
                        invalidDomain[invalidDomainIdx].InvalidFaultDomain = placementPolicies[i].DomainName.c_str();
                        policyDescriptions[i].Value = &invalidDomain[invalidDomainIdx];
                        invalidDomainIdx++;
                        break;
                    }
                    case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN:
                    {
                        requiredDomain[requiredDomainIdx].RequiredFaultDomain = placementPolicies[i].DomainName.c_str();
                        policyDescriptions[i].Value = &requiredDomain[requiredDomainIdx];
                        requiredDomainIdx++;
                        break;
                    }
                    case FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN:
                    {
                         preferredPrimaryDomain[preferredPrimaryDomainIdx].PreferredPrimaryFaultDomain = placementPolicies[i].DomainName.c_str();
                         policyDescriptions[i].Value = &preferredPrimaryDomain[preferredPrimaryDomainIdx];
                         preferredPrimaryDomainIdx++;
                         break;
                    }
                    case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION:
                    {
                         policyDescriptions[i].Value = &requiredDist[requiredDistIdx];
                         requiredDistIdx++;
                         break;
                    }
                    case FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE:
                    {
                        policyDescriptions[i].Value = &nonPartially[nonPartiallyIdx];
                        nonPartiallyIdx++;
                        break;
                    }
                    default:
                        policyDescriptions[i].Type = FABRIC_PLACEMENT_POLICY_INVALID;
                }
            }
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_POLICY_LIST;
            statefulUpdateDescriptionEx3.PolicyList = &pList;
            serviceDescription.PlacementPolicies = move(placementPolicies);

        }

        auto ex5 = heap.AddItem<FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX5>();
        statefulUpdateDescriptionEx4.Reserved = ex5.GetRawPointer();

        if (!addedNames.empty() || !removedNames.empty())
        {
            auto repartitionDescription = heap.AddItem<FABRIC_NAMED_REPARTITION_DESCRIPTION>();

            auto addedNamesArray = heap.AddArray<LPCWSTR>(addedNames.size());
            for (auto ix=0; ix<addedNames.size(); ++ix)
            {
                addedNamesArray[ix] = heap.AddString(addedNames[ix]);
            }
            repartitionDescription->NamesToAddCount = static_cast<ULONG>(addedNames.size());
            repartitionDescription->NamesToAdd = addedNamesArray.GetRawArray();

            auto removedNamesArray = heap.AddArray<LPCWSTR>(removedNames.size());
            for (auto ix=0; ix<removedNames.size(); ++ix)
            {
                removedNamesArray[ix] = heap.AddString(removedNames[ix]);
            }
            repartitionDescription->NamesToRemoveCount = static_cast<ULONG>(removedNames.size());
            repartitionDescription->NamesToRemove = removedNamesArray.GetRawArray();

            ex5->RepartitionKind = FABRIC_SERVICE_PARTITION_KIND_NAMED;
            ex5->RepartitionDescription = repartitionDescription.GetRawPointer();
        }

        wstring scalingPolicy;
        parser.TryGetString(L"scalingPolicy", scalingPolicy, L"");
        if (!scalingPolicy.empty())
        {
            if (!GetServiceScalingPolicy(scalingPolicy, scalingPolicies))
            {
                return false;
            }
        }

        if (scalingPolicies.size() == 1)
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_SCALING_POLICY;
            ex5->ScalingPolicyCount = 1;
            auto nativeScalingPolicies = heap.AddArray<FABRIC_SERVICE_SCALING_POLICY>(1);
            scalingPolicies[0].ToPublicApi(heap, nativeScalingPolicies[0]);
            ex5->ServiceScalingPolicies = nativeScalingPolicies.GetRawArray();
            serviceDescription.ScalingPolicies = scalingPolicies;
        }
    }
    else
    {

        serviceUpdateDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        serviceUpdateDescription.Value = &statelessUpdateDescription;
        statelessUpdateDescription.Reserved = &statelessUpdateDescriptionEx1;
        statelessUpdateDescriptionEx1.Reserved = &statelessUpdateDescriptionEx2;
        statelessUpdateDescriptionEx2.Reserved = &statelessUpdateDescriptionEx3;

        int instanceCount;
        if (parser.TryGetInt(L"InstanceCount", instanceCount, -1))
        {
            statelessUpdateDescription.Flags |= FABRIC_STATELESS_SERVICE_INSTANCE_COUNT;
            statelessUpdateDescription.InstanceCount = instanceCount;

            serviceDescription.TargetReplicaSetSize = instanceCount;
        }

        if (parser.TryGetString(L"constraint", placementConstraints, FabricTestDispatcher::DefaultPlacementConstraints))
        {
            statelessUpdateDescription.Flags |= FABRIC_STATELESS_SERVICE_PLACEMENT_CONSTRAINTS;
            statelessUpdateDescriptionEx1.PlacementConstraints = placementConstraints.c_str();
            serviceDescription.PlacementConstraints = move(wstring(placementConstraints));
        }

        wstring metricString;
        StringCollection metricStrings;
        if (parser.TryGetString(L"metrics", metricString))
        {
            if (!metricString.empty())
            {
                StringUtility::Split<wstring>(metricString, metricStrings, L",");
                if (!GetServiceMetrics(metricStrings, metrics, isStateful))
                {
                    return false;
                }
            }
            metricDescriptions.resize(metrics.size());
            for (int i = 0; i < static_cast<int>(metrics.size()); i++)
            {
                metricDescriptions[i].Name = metrics[i].Name.c_str();
                metricDescriptions[i].Weight = metrics[i].Weight;
                metricDescriptions[i].PrimaryDefaultLoad = metrics[i].PrimaryDefaultLoad;
                metricDescriptions[i].SecondaryDefaultLoad = metrics[i].SecondaryDefaultLoad;
            }
            statelessUpdateDescription.Flags |= FABRIC_STATELESS_SERVICE_METRICS;

            statelessUpdateDescriptionEx1.MetricCount = static_cast<ULONG>(metrics.size());
            statelessUpdateDescriptionEx1.Metrics = metricDescriptions.data();
            serviceDescription.Metrics = move(metrics);
        }

        ULONG defaultMoveCost;
        if (parser.TryGetULONG(L"defaultmovecost", defaultMoveCost, 0))
        {
            statelessUpdateDescription.Flags |= FABRIC_STATELESS_SERVICE_MOVE_COST;
            statelessUpdateDescriptionEx2.DefaultMoveCost = static_cast<FABRIC_MOVE_COST>(defaultMoveCost);
            serviceDescription.DefaultMoveCost = static_cast<FABRIC_MOVE_COST>(defaultMoveCost);
        }

        wstring correlationString;
        StringCollection correlationStrings;
        if (parser.TryGetString(L"servicecorrelations", correlationString))
        {
            if (!correlationString.empty())
            {
                StringUtility::Split<wstring>(correlationString, correlationStrings, L",");
                if (!GetCorrelations(correlationStrings, serviceCorrelations))
                {
                    return false;
                }
            }
            serviceCorrelationDescriptions.resize(serviceCorrelations.size());
            for (int i = 0; i < static_cast<int>(serviceCorrelations.size()); i++)
            {
                serviceCorrelationDescriptions[i].ServiceName = serviceCorrelations[i].ServiceName.c_str();
                serviceCorrelationDescriptions[i].Scheme = serviceCorrelations[i].Scheme;
            }
            statelessUpdateDescription.Flags |= FABRIC_STATELESS_SERVICE_CORRELATIONS;

            statelessUpdateDescriptionEx1.CorrelationCount = static_cast<ULONG>(serviceCorrelationDescriptions.size());
            statelessUpdateDescriptionEx1.Correlations = serviceCorrelationDescriptions.data();
            serviceDescription.Correlations = move(serviceCorrelations);
        }

        wstring policiesString;
        StringCollection policiesStrings;
        if (parser.TryGetString(L"placementPolicies", policiesString))
        {
            if (!policiesString.empty())
            {
                StringUtility::Split<wstring>(policiesString, policiesStrings, L",");

                if (!GetPlacementPolicies(policiesStrings, placementPolicies))
                {
                    return false;
                }
            }

            policyDescriptions.resize(placementPolicies.size());
            requiredDomain.resize(placementPolicies.size());
            preferredPrimaryDomain.resize(placementPolicies.size());
            invalidDomain.resize(placementPolicies.size());
            requiredDist.resize(placementPolicies.size());
            nonPartially.resize(placementPolicies.size());

            int invalidDomainIdx(0);
            int requiredDomainIdx(0);
            int preferredPrimaryDomainIdx(0);
            int requiredDistIdx(0);
            int nonPartiallyIdx(0);

            for (int i = 0; i < static_cast<int>(placementPolicies.size()); i++)
            {
                policyDescriptions[i].Type = placementPolicies[i].Type;
                switch (placementPolicies[i].Type)
                {
                    case FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN:
                    {
                       invalidDomain[invalidDomainIdx].InvalidFaultDomain = placementPolicies[i].DomainName.c_str();
                       policyDescriptions[i].Value = &invalidDomain[invalidDomainIdx];
                       invalidDomainIdx++;
                       break;
                    }
                    case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN:
                    {
                        requiredDomain[requiredDomainIdx].RequiredFaultDomain = placementPolicies[i].DomainName.c_str();
                        policyDescriptions[i].Value = &requiredDomain[requiredDomainIdx];
                        requiredDomainIdx++;
                        break;
                    }
                    case FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN:
                    {
                         preferredPrimaryDomain[preferredPrimaryDomainIdx].PreferredPrimaryFaultDomain = placementPolicies[i].DomainName.c_str();
                         policyDescriptions[i].Value = &preferredPrimaryDomain[preferredPrimaryDomainIdx];
                         preferredPrimaryDomainIdx++;
                         break;
                    }
                    case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION:
                    {
                         policyDescriptions[i].Value = &requiredDist[requiredDistIdx];
                         requiredDistIdx++;
                         break;
                    }
                    case FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE:
                    {
                        policyDescriptions[i].Value = &nonPartially[nonPartiallyIdx];
                        nonPartiallyIdx++;
                        break;
                    }
                    default:
                        policyDescriptions[i].Type = FABRIC_PLACEMENT_POLICY_INVALID;
                }
            }

            pList.PolicyCount = static_cast<ULONG>(policyDescriptions.size());
            pList.Policies = policyDescriptions.data();

            statelessUpdateDescription.Flags |= FABRIC_STATELESS_SERVICE_POLICY_LIST;
            statelessUpdateDescriptionEx1.PolicyList = &pList;
            serviceDescription.PlacementPolicies = move(placementPolicies);
        }

        if (!addedNames.empty() || !removedNames.empty())
        {
            auto repartitionDescription = heap.AddItem<FABRIC_NAMED_REPARTITION_DESCRIPTION>();

            auto addedNamesArray = heap.AddArray<LPCWSTR>(addedNames.size());
            for (auto ix = 0; ix < addedNames.size(); ++ix)
            {
                addedNamesArray[ix] = heap.AddString(addedNames[ix]);
            }
            repartitionDescription->NamesToAddCount = static_cast<ULONG>(addedNames.size());
            repartitionDescription->NamesToAdd = addedNamesArray.GetRawArray();

            auto removedNamesArray = heap.AddArray<LPCWSTR>(removedNames.size());
            for (auto ix = 0; ix < removedNames.size(); ++ix)
            {
                removedNamesArray[ix] = heap.AddString(removedNames[ix]);
            }
            repartitionDescription->NamesToRemoveCount = static_cast<ULONG>(removedNames.size());
            repartitionDescription->NamesToRemove = removedNamesArray.GetRawArray();

            statelessUpdateDescriptionEx3.RepartitionKind = FABRIC_SERVICE_PARTITION_KIND_NAMED;
            statelessUpdateDescriptionEx3.RepartitionDescription = repartitionDescription.GetRawPointer();
        }

        wstring scalingPolicy;
        parser.TryGetString(L"scalingPolicy", scalingPolicy, L"");
        if (!scalingPolicy.empty())
        {
            if (!GetServiceScalingPolicy(scalingPolicy, scalingPolicies))
            {
                return false;
            }
        }

        if (scalingPolicies.size() == 1)
        {
            statelessUpdateDescription.Flags |= FABRIC_STATELESS_SERVICE_SCALING_POLICY;
            statelessUpdateDescriptionEx3.ScalingPolicyCount = 1;
            auto nativeScalingPolicies = heap.AddArray<FABRIC_SERVICE_SCALING_POLICY>(1);
            scalingPolicies[0].ToPublicApi(heap, nativeScalingPolicies[0]);
            statelessUpdateDescriptionEx3.ServiceScalingPolicies = nativeScalingPolicies.GetRawArray();
            serviceDescription.ScalingPolicies = scalingPolicies;
        }
    }

    auto serviceClient = CreateFabricServiceClient2(FABRICSESSION.FabricDispatcher.Federation);

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    vector<HRESULT> expectedErrors;
    expectedErrors.push_back(expectedError);

    UpdateService(serviceName, serviceUpdateDescription, expectedErrors, serviceClient);

    if (expectedError == S_OK)
    {
        if (ServiceModel::SystemServiceApplicationNameHelper::IsInternalServiceName(internalServiceName))
        {
            FABRICSESSION.FabricDispatcher.ServiceMap.UpdatePartitionedServiceDescriptor(internalServiceName, serviceDescription);
        }
        else
        {
            auto existingPsd = FABRICSESSION.FabricDispatcher.ServiceMap.GetServiceDescriptor(internalServiceName);

            ServiceUpdateDescription internalUpdateDescription;
            auto internalError = internalUpdateDescription.FromPublicApi(serviceUpdateDescription);
            TestSession::FailTestIfNot(internalError.IsSuccess(), "ServiceUpdateDescription.FromPublicApi() failed: error={0}", internalError);

            bool isUpdated = false;
            vector<Reliability::ConsistencyUnitDescription> unusedAdded;
            vector<Reliability::ConsistencyUnitDescription> unusedRemoved;
            internalError = internalUpdateDescription.TryUpdateServiceDescription(existingPsd, isUpdated, unusedAdded, unusedRemoved);
            TestSession::FailTestIfNot(internalError.IsSuccess(), "TryUpdateServiceDescription() failed: error={0}", internalError);
            TestSession::FailTestIfNot(isUpdated, "TryUpdateServiceDescription() did not update PSD");

            FABRICSESSION.FabricDispatcher.ServiceMap.UpdatePartitionedServiceDescriptor(internalServiceName, move(existingPsd));
        }
    }

    return true;
}

bool TestFabricClient::CreateServiceFromTemplate(Common::StringCollection const & params)
{
     if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::CreateServiceFromTemplateCommand);
        return false;
     }

     NamingUri serviceName;
     wstring nameString = params[0];
     if (!NamingUri::TryParse(nameString, serviceName))
     {
         TestSession::WriteError(
             TraceSource,
             "Could not parse name {0}.", nameString);
         return false;
     }

     wstring type = params[1];

     NamingUri applicationName;
     wstring applicationNameString = params[2];
     if (!NamingUri::TryParse(applicationNameString, applicationName))
     {
         TestSession::WriteError(
             TraceSource,
             "Could not parse name {0}.", applicationNameString);
         return false;
     }

     HRESULT expectedError = S_OK;
     if (params.size() == 4)
     {
         ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[3]);
         expectedError = ErrorCode(error).ToHResult();
     }

     PartitionedServiceDescriptor description;
     CreateServiceFromTemplate(serviceName, type, applicationName, expectedError, description);

     if(expectedError == S_OK)
     {
         FABRICSESSION.FabricDispatcher.ServiceMap.AddPartitionedServiceDescriptor(nameString, move(description));
     }

     return true;
}

bool TestFabricClient::GetServiceDescription(StringCollection const & params, ComPointer<IFabricServiceManagementClient> const & client)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::GetServiceDescriptionCommand);
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

    HRESULT expectedError = S_OK;
    if (params.size() > 1 && params[1] != L"verify")
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[1]);
        expectedError = ErrorCode(error).ToHResult();
    }

    CommandLineParser parser(params, 1);

    StringCollection partitionNames;
    parser.GetVector(L"partitionnames", partitionNames);

    ComPointer<IFabricServiceDescriptionResult> result;
    GetServiceDescription(name, expectedError, client, result);

    if (SUCCEEDED(expectedError))
    {
        FABRIC_SERVICE_DESCRIPTION const * retrivedDescription = result->get_Description();

        bool isInServiceGroup = !name.Fragment.empty();

        PartitionedServiceDescriptor compare;

        if (!isInServiceGroup)
        {
            compare = FABRICSESSION.FabricDispatcher.ServiceMap.GetServiceDescriptor(name.Name);
        }
        else
        {
            // if the name is a service group member name, get the service description for the service group and pull the member service description from there
            PartitionedServiceDescriptor partitionedServiceDescriptor = FABRICSESSION.FabricDispatcher.ServiceMap.GetServiceDescriptor(NamingUri(name.Path).Name);
            ServiceGroupDescriptor serviceGroupDescriptor;
            ErrorCode error = ServiceGroupDescriptor::Create(partitionedServiceDescriptor, serviceGroupDescriptor);

            if (error.IsSuccess())
            {
                error = serviceGroupDescriptor.GetServiceDescriptorForMember(name, compare);
            }

            TestSession::FailTestIfNot(
                error.IsSuccess(),
                "GetServiceDescriptorForMember failed with {0}",
                error.ToHResult());
        }

        bool isSystemService = ServiceModel::SystemServiceApplicationNameHelper::IsSystemServiceName(name.Name);

        if (retrivedDescription->Kind == FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS)
        {
            FABRIC_STATELESS_SERVICE_DESCRIPTION * returnVal = (FABRIC_STATELESS_SERVICE_DESCRIPTION *)retrivedDescription->Value;

            TestSession::FailTestIfNot(compare.Service.Name == returnVal->ServiceName, "ServiceName - Stored: {0}. Returned: {1}", compare.Service.Name, returnVal->ServiceName);
            TestSession::FailTestIfNot(compare.Service.Type.ServiceTypeName == returnVal->ServiceTypeName, "ServiceType - Stored: {0}. Returned: {1}", compare.Service.Type, returnVal->ServiceTypeName);
            TestSession::FailTestIfNot(compare.Service.InitializationData.size() == returnVal->InitializationDataSize, "InitializationData - Stored: {0}. Returned: {1}", compare.Service.InitializationData.size(), returnVal->InitializationDataSize);
            TestSession::FailTestIfNot(compare.PartitionScheme == returnVal->PartitionScheme, "PartitionScheme - Stored: {0}. Returned: {1}", compare.PartitionScheme, returnVal->PartitionScheme);
            TestSession::FailTestIfNot(compare.Service.TargetReplicaSetSize == returnVal->InstanceCount, "InstanceCount - Stored: {0}. Returned: {1}", compare.Service.TargetReplicaSetSize, returnVal->InstanceCount);
            TestSession::FailTestIfNot(compare.Service.PlacementConstraints == returnVal->PlacementConstraints, "PlacementConstraints - Stored: {0}. Returned: {1}", compare.Service.PlacementConstraints, returnVal->PlacementConstraints);
            TestSession::FailTestIfNot(compare.Service.Correlations.size() == returnVal->CorrelationCount, "CorrelationCount - Stored: {0}. Returned: {1}", compare.Service.Correlations.size(), returnVal->CorrelationCount);
            TestSession::FailTestIfNot(compare.Service.Metrics.size() == returnVal->MetricCount, "MetricCount - Stored: {0}. Returned: {1}", compare.Service.Metrics.size(), returnVal->MetricCount);

            if (!partitionNames.empty())
            {
                TestSession::FailTestIfNot(returnVal->PartitionScheme == FABRIC_PARTITION_SCHEME_NAMED, "PartitionScheme - Expected: {0}. Returned: {1}", FABRIC_PARTITION_SCHEME_NAMED, returnVal->ServiceName);

                auto * partitionDescription = static_cast<FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION*>(returnVal->PartitionSchemeDescription);

                TestSession::FailTestIfNot(partitionDescription->PartitionCount == partitionNames.size(), "PartitionCount - Expected: {0}. Returned: {1}", partitionNames.size(), partitionDescription->PartitionCount);

                vector<wstring> returnedNames;
                for (auto ix=0; ix<partitionDescription->PartitionCount; ++ix)
                {
                    returnedNames.push_back(partitionDescription->Names[ix]);
                }

                sort(partitionNames.begin(), partitionNames.end());
                sort(returnedNames.begin(), returnedNames.end());

                for (auto ix=0; ix<partitionNames.size(); ++ix)
                {
                    TestSession::FailTestIfNot(partitionNames[ix] == returnedNames[ix], "PartitionName - Expected: {0}. Returned: {1} Index: {2}", partitionNames[ix], returnedNames[ix], ix);
                }
            }

            FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1 * ex1 = (FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1 *)returnVal->Reserved;

            std::size_t policyCount = ex1->PolicyList == nullptr ? 0 : ex1->PolicyList->PolicyCount;
            TestSession::FailTestIfNot(compare.Service.PlacementPolicies.size() == policyCount, "PlacementPolicy - Stored: {0}. Returned: {1}", compare.Service.PlacementPolicies.size(), policyCount);

            FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2 * ex2 = (FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2 *)ex1->Reserved;

            TestSession::FailTestIfNot(static_cast<FABRIC_MOVE_COST>(compare.Service.DefaultMoveCost) == ex2->DefaultMoveCost, "DefaultMoveCost - Stored: {0}. Returned: {1}", static_cast<FABRIC_MOVE_COST>(compare.Service.DefaultMoveCost), ex2->DefaultMoveCost);
        }
        else if (retrivedDescription->Kind == FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL)
        {
            FABRIC_STATEFUL_SERVICE_DESCRIPTION * returnVal = (FABRIC_STATEFUL_SERVICE_DESCRIPTION *)retrivedDescription->Value;

            TestSession::FailTestIfNot(compare.Service.Name == ServiceModel::SystemServiceApplicationNameHelper::GetInternalServiceName(returnVal->ServiceName), "ServiceName - Stored: {0}. Returned: {1}", compare.Service.Name, returnVal->ServiceName);
            TestSession::FailTestIfNot(isSystemService || compare.Service.Type.ServiceTypeName == returnVal->ServiceTypeName, "ServiceType - Stored: {0}. Returned: {1}", compare.Service.Type.ServiceTypeName, returnVal->ServiceTypeName);
            TestSession::FailTestIfNot(compare.Service.InitializationData.size() == returnVal->InitializationDataSize, "InitializationData - Stored: {0}. Returned: {1}", compare.Service.InitializationData.size(), returnVal->InitializationDataSize);
            TestSession::FailTestIfNot(compare.PartitionScheme == returnVal->PartitionScheme, "PartitionScheme - Stored: {0}. Returned: {1}", compare.PartitionScheme, returnVal->PartitionScheme);
            TestSession::FailTestIfNot(compare.Service.TargetReplicaSetSize == returnVal->TargetReplicaSetSize, "TargetReplicaSetSize - Stored: {0}. Returned: {1}", compare.Service.TargetReplicaSetSize, returnVal->TargetReplicaSetSize);
            TestSession::FailTestIfNot(compare.Service.MinReplicaSetSize == returnVal->MinReplicaSetSize, "MinReplicaSetSize - Stored: {0}. Returned: {1}", compare.Service.MinReplicaSetSize, returnVal->MinReplicaSetSize);
            TestSession::FailTestIfNot(compare.Service.PlacementConstraints == returnVal->PlacementConstraints, "PlacementConstraints - Stored: {0}. Returned: {1}", compare.Service.PlacementConstraints, returnVal->PlacementConstraints);
            TestSession::FailTestIfNot(compare.Service.Correlations.size() == returnVal->CorrelationCount, "CorrelationCount - Stored: {0}. Returned: {1}", compare.Service.Correlations.size(), returnVal->CorrelationCount);
            TestSession::FailTestIfNot(compare.Service.Metrics.size() == returnVal->MetricCount, "MetricCount - Stored: {0}. Returned: {1}", compare.Service.Metrics.size(), returnVal->MetricCount);

            if (!partitionNames.empty())
            {
                TestSession::FailTestIfNot(returnVal->PartitionScheme == FABRIC_PARTITION_SCHEME_NAMED, "PartitionScheme - Expected: {0}. Returned: {1}", FABRIC_PARTITION_SCHEME_NAMED, returnVal->ServiceName);

                auto * partitionDescription = static_cast<FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION*>(returnVal->PartitionSchemeDescription);

                TestSession::FailTestIfNot(partitionDescription->PartitionCount == partitionNames.size(), "PartitionCount - Expected: {0}. Returned: {1}", partitionNames.size(), partitionDescription->PartitionCount);

                vector<wstring> returnedNames;
                for (auto ix=0; ix<partitionDescription->PartitionCount; ++ix)
                {
                    returnedNames.push_back(partitionDescription->Names[ix]);
                }

                sort(partitionNames.begin(), partitionNames.end());
                sort(returnedNames.begin(), returnedNames.end());

                for (auto ix=0; ix<partitionNames.size(); ++ix)
                {
                    TestSession::FailTestIfNot(partitionNames[ix] == returnedNames[ix], "PartitionName - Expected: {0}. Returned: {1} Index: {2}", partitionNames[ix], returnedNames[ix], ix);
                }
            }

            FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1 * ex1 = (FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1 *)returnVal->Reserved;

            std::size_t policyCount = ex1->PolicyList == nullptr ? 0 : ex1->PolicyList->PolicyCount;
            TestSession::FailTestIfNot(compare.Service.PlacementPolicies.size() == policyCount, "PlacementPolicy - Stored: {0}. Returned: {1}", compare.Service.PlacementPolicies.size(), policyCount);

            int64 replicaRestartWaitDurationSeconds;
            if (parser.TryGetInt64(L"replicarestartwaitduration", replicaRestartWaitDurationSeconds, -1))
            {
                TestSession::FailTestIfNot(replicaRestartWaitDurationSeconds == static_cast<int64>(ex1->FailoverSettings->ReplicaRestartWaitDurationSeconds),
                    "ReplicaRestartWaitDuration: Expected={0}, Actual={1}", replicaRestartWaitDurationSeconds, ex1->FailoverSettings->ReplicaRestartWaitDurationSeconds);
            }

            int64 quorumLossWaitDurationSeconds;
            if (parser.TryGetInt64(L"quorumlosswaitduration", quorumLossWaitDurationSeconds, -1))
            {
                TestSession::FailTestIfNot(quorumLossWaitDurationSeconds == static_cast<int64>(ex1->FailoverSettings->QuorumLossWaitDurationSeconds),
                    "QuorumLossWaitDuration: Expected={0}, Actual={1}", quorumLossWaitDurationSeconds, ex1->FailoverSettings->QuorumLossWaitDurationSeconds);
            }

            FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_EX1 * settingsEx1 = (FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_EX1 *)ex1->FailoverSettings->Reserved;

            int64 standByReplicaKeepDurationSeconds;
            if (parser.TryGetInt64(L"standbyreplicakeepduration", standByReplicaKeepDurationSeconds, -1))
            {
                TestSession::FailTestIfNot(standByReplicaKeepDurationSeconds == static_cast<int64>(settingsEx1->StandByReplicaKeepDurationSeconds),
                    "StandByReplicaKeepDuration: Expected={0}, Actual={1}", standByReplicaKeepDurationSeconds, settingsEx1->StandByReplicaKeepDurationSeconds);
            }

            FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2 * ex2 = (FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2 *)ex1->Reserved;

            TestSession::FailTestIfNot(static_cast<FABRIC_MOVE_COST>(compare.Service.DefaultMoveCost) == ex2->DefaultMoveCost, "DefaultMoveCost - Stored: {0}. Returned: {1}", static_cast<FABRIC_MOVE_COST>(compare.Service.DefaultMoveCost), ex2->DefaultMoveCost);
        }
        else
        {
            TestSession::FailTest("Unrecognized DescriptionType: {0}", retrivedDescription->Kind);
        }
    }

    return true;
}

bool TestFabricClient::GetServiceGroupDescription(StringCollection const & params)
{
    if (params.size() < 1 || params.size() > 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::GetServiceDescriptionCommand);
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

    HRESULT expectedError = S_OK;
    if (params.size() == 2)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[1]);
        expectedError = ErrorCode(error).ToHResult();
    }

    ComPointer<IFabricServiceGroupDescriptionResult> result;
    GetServiceGroupDescription(name, expectedError, result);

    if (SUCCEEDED(expectedError))
    {
        FABRIC_SERVICE_GROUP_DESCRIPTION const * retrievedDescription = result->get_Description();

        if (!name.Fragment.empty())
        {
            name = NamingUri(name.Path);
        }

        PartitionedServiceDescriptor compare = FABRICSESSION.FabricDispatcher.ServiceMap.GetServiceDescriptor(NamingUri(name.Path).Name);
        ServiceGroupDescriptor serviceGroupDescriptor;
        ErrorCode error = ServiceGroupDescriptor::Create(compare, serviceGroupDescriptor);
        TestSession::FailTestIfNot(error.IsSuccess(), "Failed to create service group descriptor due to {0}", error);

        // sanity check service settings
        if (retrievedDescription->Description->Kind == FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS)
        {
            FABRIC_STATELESS_SERVICE_DESCRIPTION * returnVal = (FABRIC_STATELESS_SERVICE_DESCRIPTION *)retrievedDescription->Description->Value;

            TestSession::FailTestIfNot(compare.Service.Name == returnVal->ServiceName, "ServiceName - Stored: {0}. Returned: {1}", compare.Service.Name, returnVal->ServiceName);
            TestSession::FailTestIfNot(compare.Service.Type.ServiceTypeName == returnVal->ServiceTypeName, "ServiceType - Stored: {0}. Returned: {1}", compare.Service.Type, returnVal->ServiceTypeName);
            TestSession::FailTestIfNot(compare.PartitionScheme == returnVal->PartitionScheme, "PartitionScheme - Stored: {0}. Returned: {1}", compare.PartitionScheme, returnVal->PartitionScheme);
            TestSession::FailTestIfNot(compare.Service.TargetReplicaSetSize == returnVal->InstanceCount, "InstanceCount - Stored: {0}. Returned: {1}", compare.Service.TargetReplicaSetSize, returnVal->InstanceCount);
            TestSession::FailTestIfNot(compare.Service.PlacementConstraints == returnVal->PlacementConstraints, "PlacementConstraints - Stored: {0}. Returned: {1}", compare.Service.PlacementConstraints, returnVal->PlacementConstraints);
            TestSession::FailTestIfNot(compare.Service.Correlations.size() == returnVal->CorrelationCount, "CorrelationCount - Stored: {0}. Returned: {1}", compare.Service.Correlations.size(), returnVal->CorrelationCount);
            TestSession::FailTestIfNot(compare.Service.Metrics.size() == returnVal->MetricCount, "MetricCount - Stored: {0}. Returned: {1}", compare.Service.Metrics.size(), returnVal->MetricCount);
        }
        else if (retrievedDescription->Description->Kind == FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL)
        {
            FABRIC_STATEFUL_SERVICE_DESCRIPTION * returnVal = (FABRIC_STATEFUL_SERVICE_DESCRIPTION *)retrievedDescription->Description->Value;

            TestSession::FailTestIfNot(compare.Service.Name == returnVal->ServiceName, "ServiceName - Stored: {0}. Returned: {1}", compare.Service.Name, returnVal->ServiceName);
            TestSession::FailTestIfNot(compare.Service.Type.ServiceTypeName == returnVal->ServiceTypeName, "ServiceType - Stored: {0}. Returned: {1}", compare.Service.Type.ServiceTypeName, returnVal->ServiceTypeName);
            TestSession::FailTestIfNot(compare.PartitionScheme == returnVal->PartitionScheme, "PartitionScheme - Stored: {0}. Returned: {1}", compare.PartitionScheme, returnVal->PartitionScheme);
            TestSession::FailTestIfNot(compare.Service.TargetReplicaSetSize == returnVal->TargetReplicaSetSize, "TargetReplicaSetSize - Stored: {0}. Returned: {1}", compare.Service.TargetReplicaSetSize, returnVal->TargetReplicaSetSize);
            TestSession::FailTestIfNot(compare.Service.MinReplicaSetSize == returnVal->MinReplicaSetSize, "MinWritableReplicaSMinReplicaSetSizeetSize - Stored: {0}. Returned: {1}", compare.Service.MinReplicaSetSize, returnVal->MinReplicaSetSize);
            TestSession::FailTestIfNot(compare.Service.PlacementConstraints == returnVal->PlacementConstraints, "PlacementConstraints - Stored: {0}. Returned: {1}", compare.Service.PlacementConstraints, returnVal->PlacementConstraints);
            TestSession::FailTestIfNot(compare.Service.Correlations.size() == returnVal->CorrelationCount, "CorrelationCount - Stored: {0}. Returned: {1}", compare.Service.Correlations.size(), returnVal->CorrelationCount);
            TestSession::FailTestIfNot(compare.Service.Metrics.size() == returnVal->MetricCount, "MetricCount - Stored: {0}. Returned: {1}", compare.Service.Metrics.size(), returnVal->MetricCount);
        }
        else
        {
            TestSession::FailTest("Unrecognized DescriptionType: {0}", retrievedDescription->Description->Kind);
        }

        ServiceModel::CServiceGroupDescription groupDescription;
        vector<byte> storeDescription(compare.Service.InitializationData);
        FabricSerializer::Deserialize(groupDescription, storeDescription);

        if (retrievedDescription->Description->Kind == FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS)
        {
            FABRIC_STATELESS_SERVICE_DESCRIPTION * returnVal = (FABRIC_STATELESS_SERVICE_DESCRIPTION *)retrievedDescription->Description->Value;
            TestSession::FailTestIfNot(groupDescription.ServiceGroupInitializationData.size() == returnVal->InitializationDataSize, "InitializationData - Stored: {0}. Returned: {1}", groupDescription.ServiceGroupInitializationData.size(), returnVal->InitializationDataSize);
        }
        else if (retrievedDescription->Description->Kind == FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL)
        {
            FABRIC_STATEFUL_SERVICE_DESCRIPTION * returnVal = (FABRIC_STATEFUL_SERVICE_DESCRIPTION *)retrievedDescription->Description->Value;
            TestSession::FailTestIfNot(groupDescription.ServiceGroupInitializationData.size() == returnVal->InitializationDataSize, "InitializationData - Stored: {0}. Returned: {1}", groupDescription.ServiceGroupInitializationData.size(), returnVal->InitializationDataSize);
        }

        TestSession::FailTestIfNot(groupDescription.ServiceGroupMemberData.size() == retrievedDescription->MemberCount, "MemberCount: Stored {0}. Returned {1}", groupDescription.ServiceGroupMemberData.size(), retrievedDescription->MemberCount);

        for (unsigned int i = 0; i < groupDescription.ServiceGroupMemberData.size(); ++i)
        {
            TestSession::FailTestIfNot(groupDescription.ServiceGroupMemberData[i].ServiceName == retrievedDescription->MemberDescriptions[i].ServiceName, "ServiceName: Stored {0}. Returned {1}", groupDescription.ServiceGroupMemberData[i].ServiceName, retrievedDescription->MemberDescriptions[i].ServiceName);
            TestSession::FailTestIfNot(groupDescription.ServiceGroupMemberData[i].ServiceType == retrievedDescription->MemberDescriptions[i].ServiceType, "ServiceType: Stored {0}. Returned {1}", groupDescription.ServiceGroupMemberData[i].ServiceType, retrievedDescription->MemberDescriptions[i].ServiceType);
            TestSession::FailTestIfNot(groupDescription.ServiceGroupMemberData[i].ServiceGroupMemberInitializationData.size() == retrievedDescription->MemberDescriptions[i].InitializationDataSize, "ServiceGroupMemberInitializationDataSize: Stored {0}. Returned {1}", groupDescription.ServiceGroupMemberData[i].ServiceGroupMemberInitializationData.size(), retrievedDescription->MemberDescriptions[i].InitializationDataSize);
        }
    }

    return true;
}

bool TestFabricClient::DeactivateNode(StringCollection const& params, bool & updateNode, NodeId & nodeToUpdate)
{
    updateNode = false;

    if (params.size() < 2 || params.size() > 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeactivateNodeCommand);
        return false;
    }

    wstring nodeName = L"nodeid:" + params[0];

    if (!NodeIdGenerator::GenerateFromString(nodeName, nodeToUpdate).IsSuccess())
    {
        return false;
    }

    FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent;
    if (params[1] == L"Pause")
    {
        deactivationIntent = FABRIC_NODE_DEACTIVATION_INTENT_PAUSE;
    }
    else if (params[1] == L"Restart")
    {
        deactivationIntent = FABRIC_NODE_DEACTIVATION_INTENT_RESTART;
    }
    else if (params[1] == L"RemoveData")
    {
        deactivationIntent = FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_DATA;
    }
    else if (params[1] == L"RemoveNode")
    {
        deactivationIntent = FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_NODE;
    }
    else
    {
        TestSession::FailTest("Invalid FABRIC_NODE_DEACTIVATION_INTENT");
    }

    HRESULT expectedError = S_OK;

    if (params.size() == 2)
    {
        DeactivateNode(nodeName, deactivationIntent, expectedError);
    }
    else
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[2]);
        expectedError = ErrorCode(error).ToHResult();

        DeactivateNode(nodeName, deactivationIntent, expectedError);
    }

    if (deactivationIntent == FABRIC_NODE_DEACTIVATION_INTENT_RESTART)
    {
        updateNode = true;
    }

    return true;
}

bool TestFabricClient::ActivateNode(StringCollection const& params, NodeId & nodeToUpdate)
{
    if (params.size() != 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ActivateNodeCommand);
        return false;
    }

    wstring nodeName = L"nodeid:" + params[0];

    if (!NodeIdGenerator::GenerateFromString(nodeName, nodeToUpdate).IsSuccess())
    {
        return false;
    }

    ActivateNode(nodeName);

    return true;
}

bool TestFabricClient::NodeStateRemoved(StringCollection const& params)
{
    if ((params.size() > 2) || (params.size() < 1))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::NodeStateRemovedCommand);
        return false;
    }

    wstring nodeHostName = L"nodeid:" + params[0];
    wstring nodeStoreDirectory = FABRICSESSION.FabricDispatcher.GetWorkingDirectory(params[0]);

    HRESULT expectedError = S_OK;
    if (params.size() > 1)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[1]);
        expectedError = ErrorCode(error).ToHResult();
    }

    int maxRetryCount = 10;

    int retryCount = 0;
    bool deleteSuccessful = !Directory::Exists(nodeStoreDirectory) || ErrorCode::FromHResult(expectedError).IsError(ErrorCodeValue::NodeIsUp);

    while (!deleteSuccessful && retryCount < maxRetryCount)
    {
        auto error = Directory::Delete(nodeStoreDirectory, true);
        if(error.IsSuccess())
        {
            deleteSuccessful = true;
        }
        else
        {
            retryCount++;
            Sleep(3000);
        }
    }

    TestSession::FailTestIfNot(deleteSuccessful, "Cannot delete directory {0}", nodeStoreDirectory);

    NodeStateRemoved(nodeHostName, expectedError);

    return true;
}

bool TestFabricClient::RecoverPartitions(StringCollection const& params)
{
    if (params.size() > 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RecoverPartitionsCommand);
        return false;
    }

    HRESULT expectedError = S_OK;
    if (params.size() == 1)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[0]);
        expectedError = ErrorCode(error).ToHResult();
    }

    RecoverPartitions(expectedError);

    return true;
}

bool TestFabricClient::RecoverPartition(StringCollection const& params)
{
    if (params.size() < 1 || params.size() > 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RecoverPartitionCommand);
        return false;
    }

    FABRIC_PARTITION_ID partitionId = Guid(params[0]).AsGUID();

    HRESULT expectedError = S_OK;
    if (params.size() == 2)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[1]);
        expectedError = ErrorCode(error).ToHResult();
    }

    RecoverPartition(partitionId, expectedError);

    return true;
}

bool TestFabricClient::RecoverServicePartitions(StringCollection const& params)
{
    if (params.size() < 1 || params.size() > 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RecoverServicePartitionsCommand);
        return false;
    }

    wstring const& serviceName = params[0];

    HRESULT expectedError = S_OK;
    if (params.size() == 2)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[1]);
        expectedError = ErrorCode(error).ToHResult();
    }

    RecoverServicePartitions(serviceName, expectedError);

    return true;
}

bool TestFabricClient::RecoverSystemPartitions(StringCollection const& params)
{
    if (params.size() > 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RecoverSystemPartitionsCommand);
        return false;
    }

    HRESULT expectedError = S_OK;
    if (params.size() == 1)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[0]);
        expectedError = ErrorCode(error).ToHResult();
    }

    RecoverSystemPartitions(expectedError);

    return true;
}

bool TestFabricClient::ReportFault(StringCollection const & params)
{
    if (params.size() < 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientReportFaultCommand);
        return false;
    }

    // Parse NodeName
    NodeId nodeId = FederationTestCommon::CommonDispatcher::ParseNodeId(params[0]);
    wstring nodeName = Federation::NodeIdGenerator::GenerateNodeName(nodeId);

    // partition id
    FABRIC_PARTITION_ID partitionId = Guid(params[1]).AsGUID();

    FABRIC_REPLICA_ID replicaId = 0;
    int64 val;
    if (!TryParseInt64(params[2], val))
    {
        return false;
    }

    replicaId = static_cast<FABRIC_REPLICA_ID>(val);

    // try to parse the fault type
    ::FABRIC_FAULT_TYPE faultType =
        params[3].compare(L"permanent") == 0 ? ::FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_PERMANENT : ::FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_TRANSIENT;

    // Parse optional parameters
    bool isForce = false;
    HRESULT expectedError = S_OK;

    size_t index = 4;
    if (params.size() > index)
    {
        // Parse isForce
        if (params[index].compare(L"true") == 0)
        {
            isForce = true;
            index++;
        }
        else if (params[index].compare(L"false") == 0)
        {
            isForce = false;
            index++;
        }
    }

    if (params.size() > index)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[index]);
        expectedError = ErrorCode(error).ToHResult();
        index++;
    }

    // make the call
    ReportFault(nodeName, partitionId, replicaId, faultType, isForce, expectedError);

    return true;
}

bool TestFabricClient::ResetPartitionLoad(StringCollection const& params)
{
    if (params.size() < 1 || params.size() > 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ResetPartitionLoadCommand);
        return false;
    }

    FABRIC_PARTITION_ID partitionId = Guid(params[0]).AsGUID();

    HRESULT expectedError = S_OK;
    if (params.size() == 2)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[1]);
        expectedError = ErrorCode(error).ToHResult();
    }

    ResetPartitionLoad(partitionId, expectedError);

    return true;
}

bool TestFabricClient::ToggleVerboseServicePlacementHealthReporting(StringCollection const& params)
{
    if (params.size() < 1 || params.size() > 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ToggleVerboseServicePlacementHealthReportingCommand);
        return false;
    }

    std::wstring enableParam = StringUtility::ToWString(params[0]);
    StringUtility::ToLower(enableParam);

    if (enableParam.compare(StringUtility::ToWString("true")) && enableParam.compare(StringUtility::ToWString("false")))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ToggleVerboseServicePlacementHealthReportingCommand);
        return false;
    }

    bool enabled = !(enableParam.compare(StringUtility::ToWString("true")));

    HRESULT expectedError = S_OK;
    if (params.size() == 2)
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[1]);
        expectedError = ErrorCode(error).ToHResult();
    }

    ToggleVerboseServicePlacementHealthReporting(enabled, expectedError);

    return true;
}

bool TestFabricClient::MovePrimaryReplicaFromClient(StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientReportFaultCommand);
        return false;
    }

    auto capitalize = [](std::wstring str) -> std::wstring
    {
        StringUtility::ToUpper(str);
        return str;
    };



    // Parse NodeName
    NodeId nodeId;
    wstring nodeName;
    bool ignoreConstraints = false;

    if (capitalize(params[0]) == L"RANDOM")
    {
        nodeName = L"";
    }
    else
    {
        nodeId = FederationTestCommon::CommonDispatcher::ParseNodeId(params[0]);
        nodeName = Federation::NodeIdGenerator::GenerateNodeName(nodeId);
    }

    // partition id
    FABRIC_PARTITION_ID partitionId = Guid(params[1]).AsGUID();

    HRESULT expectedError = S_OK;

    if (params.size() ==  3)
    {
    wstring constraintsParam = StringUtility::ToWString(params[2]);
        StringUtility::ToLower(constraintsParam);

        ignoreConstraints = !constraintsParam.compare(StringUtility::ToWString("force"));

        if (!ignoreConstraints)
        {
            ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[2]);
            expectedError = ErrorCode(error).ToHResult();
        }

    }

    if (params.size() > 3)
    {
        wstring constraintsParam = StringUtility::ToWString(params[2]);
        StringUtility::ToLower(constraintsParam);

        ignoreConstraints = !constraintsParam.compare(StringUtility::ToWString("force"));

        if (constraintsParam.compare(StringUtility::ToWString("force")))
        {
            FABRICSESSION.PrintHelp(FabricTestCommands::MovePrimaryFromClientCommand);
            return false;
        }

        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[3]);
        expectedError = ErrorCode(error).ToHResult();
    }

    // make the call
    MovePrimaryReplicaFromClient(nodeName, partitionId, ignoreConstraints, expectedError);
    return true;
}

void TestFabricClient::MovePrimaryReplicaFromClient(std::wstring const & nodeName, FABRIC_PARTITION_ID partitionId, bool ignoreConstraints, HRESULT expectedError)
{
    Guid forTrace(partitionId);
    TestSession::WriteNoise(TraceSource, "Move primary called for Node = {0}. Partition = {1}.", nodeName, forTrace);
    auto serviceManagementClient = CreateInternalFabricServiceClient2(FABRICSESSION.FabricDispatcher.Federation);
    this->PerformFabricClientOperation(
        L"MovePrimary",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        FABRIC_MOVE_PRIMARY_REPLICA_DESCRIPTION desc = { 0 };
        FABRIC_MOVE_PRIMARY_REPLICA_DESCRIPTION_EX1 descExt = { 0 };

        desc.NodeName = nodeName.c_str();
        desc.PartitionId = partitionId;

        descExt.IgnoreConstraints = ignoreConstraints;

        desc.Reserved = &descExt;

        return serviceManagementClient->BeginMovePrimary(&desc, timeout, callback.GetRawPointer(), context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return serviceManagementClient->EndMovePrimary(context.GetRawPointer());
    },
        expectedError);
}

bool TestFabricClient::MoveSecondaryReplicaFromClient(StringCollection const & params)
{
    FABRICSESSION.FabricDispatcher.VerifyAll(true);
    if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientReportFaultCommand);
        return false;
    }

    auto capitalize = [](std::wstring str) -> std::wstring
    {
        StringUtility::ToUpper(str);
        return str;
    };

    bool ignoreConstraints = false;
    NodeId newNodeId;
    wstring newNodeName;



    // Parse NodeName
    NodeId currentNodeId = FederationTestCommon::CommonDispatcher::ParseNodeId(params[0]);
    wstring currentNodeName = Federation::NodeIdGenerator::GenerateNodeName(currentNodeId);

    if (capitalize(params[1]) == L"RANDOM")
    {
        newNodeName = L"";
    }
    else
    {
        newNodeId = FederationTestCommon::CommonDispatcher::ParseNodeId(params[1]);
        newNodeName = Federation::NodeIdGenerator::GenerateNodeName(newNodeId);
    }



    // partition id
    FABRIC_PARTITION_ID partitionId = Guid(params[2]).AsGUID();

    HRESULT expectedError = S_OK;

    if (params.size() ==  4)
    {
        wstring constraintsParam = StringUtility::ToWString(params[3]);
        StringUtility::ToLower(constraintsParam);

        ignoreConstraints = !constraintsParam.compare(StringUtility::ToWString("force"));

        if (!ignoreConstraints)
        {
            ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[3]);
            expectedError = ErrorCode(error).ToHResult();
        }

    }

    if (params.size() > 4)
    {
        wstring constraintsParam = StringUtility::ToWString(params[3]);
        StringUtility::ToLower(constraintsParam);

        ignoreConstraints = !constraintsParam.compare(StringUtility::ToWString("force"));

        if (constraintsParam.compare(StringUtility::ToWString("force")))
        {
            FABRICSESSION.PrintHelp(FabricTestCommands::MoveSecondaryFromClientCommand);
            return false;
        }

        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(params[4]);
        expectedError = ErrorCode(error).ToHResult();
    }

    // make the call
    MoveSecondaryReplicaFromClient(currentNodeName, newNodeName, partitionId, ignoreConstraints, expectedError);
    return true;
}

void TestFabricClient::MoveSecondaryReplicaFromClient(std::wstring const & currentNodeName, std::wstring const & newNodeName, FABRIC_PARTITION_ID partitionId, bool ignoreConstraints, HRESULT expectedError)
{
    Guid forTrace(partitionId);
    TestSession::WriteNoise(TraceSource, "Move secondary called from Node= {0} to Node= {1} for Partition= {2}.", currentNodeName, newNodeName, forTrace);
    auto serviceManagementClient = CreateInternalFabricServiceClient2(FABRICSESSION.FabricDispatcher.Federation);
    FABRICSESSION.FabricDispatcher.VerifyAll(true);
    this->PerformFabricClientOperation(
        L"MovePrimary",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        FABRIC_MOVE_SECONDARY_REPLICA_DESCRIPTION desc = { 0 };
        FABRIC_MOVE_SECONDARY_REPLICA_DESCRIPTION_EX1 descExt = { 0 };
        desc.CurrentSecondaryNodeName = currentNodeName.c_str();
        desc.NewSecondaryNodeName = newNodeName.c_str();
        desc.PartitionId = partitionId;

        descExt.IgnoreConstraints = ignoreConstraints;

        desc.Reserved = &descExt;

        return serviceManagementClient->BeginMoveSecondary(&desc, timeout, callback.GetRawPointer(), context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return serviceManagementClient->EndMoveSecondary(context.GetRawPointer());
    },
        expectedError);
}

bool TestFabricClient::CreateServiceGroup(StringCollection const & params)
{
    if (params.size() < 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::CreateServiceGroupCommand);
        return false;
    }

    NamingUri name;
    wstring nameString;
    if (!NamingUri::TryParse(params[0], name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }
    nameString = name.ToString();

    wstring type = params[1];
    bool isStateful = IsTrue(params[2]);
    int partitionCount = Common::Int32_Parse(params[3]);
    int instanceCount = Common::Int32_Parse(params[4]);
    int targetReplicaSetSize = instanceCount;

    int serviceNumber = Common::Int32_Parse(params[5]);


    map<wstring, wstring> initData;

    size_t current = static_cast<size_t>(6+serviceNumber*2);
    while (current < params.size())
    {
        if (params.at(current) == L"init")
        {
            current++;
            TestSession::FailTestIf(current >= params.size(), "Incomplete initialization data");

            wstring appliesToService = params.at(current);
            wstring data = L"";

            current++;
            TestSession::FailTestIf(current >= params.size(), "Incomplete initialization data");
            while (current < params.size() && params.at(current) != L"endinit")
            {
                if (data.empty())
                {
                    data = params.at(current);
                }
                else
                {
                    data = data + L" " + params.at(current);
                }
                current++;
            }

            TestSession::FailTestIf(current > params.size(), "Missing endinit");

            initData.insert(make_pair(appliesToService, data));
            current++;
        }
        else
        {
            break;
        }
    }

    CommandLineParser parser(params, current);

    bool hasPersistedState = parser.GetBool(L"persist") && isStateful;

    int replicaRestartWaitDurationSeconds;
    parser.TryGetInt(L"replicarestartwaitduration", replicaRestartWaitDurationSeconds, -1);

    int quorumLossWaitDurationSeconds;
    parser.TryGetInt(L"quorumlosswaitduration", quorumLossWaitDurationSeconds, -1);

    int standByReplicaKeepDurationSeconds;
    parser.TryGetInt(L"standbyreplicakeepduration", standByReplicaKeepDurationSeconds, -1);

    vector<ServiceCorrelationDescription> serviceCorrelations;
    StringCollection correlationStrings;
    parser.GetVector(L"servicecorrelations", correlationStrings);
    if(correlationStrings.size() > 0 && !GetCorrelations(correlationStrings, serviceCorrelations))
    {
        return false;
    }

    wstring placementConstraints;
    parser.TryGetString(L"constraint", placementConstraints, FabricTestDispatcher::DefaultPlacementConstraints);
    int scaleoutCount;
    parser.TryGetInt(L"scalecount", scaleoutCount, FabricTestDispatcher::DefaultScaleoutCount);
    int minReplicaSetSize;
    parser.TryGetInt(L"minreplicasetsize", minReplicaSetSize, 1);

    vector<Reliability::ServiceLoadMetricDescription> metrics;
    StringCollection metricStrings;
    parser.GetVector(L"metrics", metricStrings);
    if(!metricStrings.empty() && !GetServiceMetrics(metricStrings, metrics, isStateful))
    {
        return false;
    }

    bool isDefaultMoveCostSpecified;
    ULONG defaultMoveCost;
    isDefaultMoveCostSpecified = parser.TryGetULONG(L"defaultmovecost", defaultMoveCost, 0);

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    FABRIC_PARTITION_SCHEME partitionScheme = partitionCount == 0 ? FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON : FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE;
    // normalize partition count like the managed layer does
    partitionCount = partitionCount == 0 ? 1 : partitionCount;

    StringCollection partitionNameStrings;
    parser.GetVector(L"partitionnames", partitionNameStrings);
    vector<LPCWSTR> partitionNames;
    if (partitionNameStrings.size() > 0)
    {
        if (partitionNameStrings.size() != static_cast<size_t>(partitionCount))
        {
            return false;
        }
        partitionScheme = FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED;
        for (size_t i = 0; i < partitionNameStrings.size(); i++)
        {
            partitionNames.push_back(&(partitionNameStrings[i][0]));
        }
    }

    // servicePackageActivationMode
    wstring servicePackageActivationModeStr;
    parser.TryGetString(L"servicePackageActivationMode", servicePackageActivationModeStr, L"SharedProcess");

    ServiceModel::ServicePackageActivationMode::Enum activationMode;
    auto err = ServiceModel::ServicePackageActivationMode::EnumFromString(servicePackageActivationModeStr, activationMode);
    if (!err.IsSuccess())
    {
        TestSession::WriteError(TraceSource, "Invalid ServicePackageActivationMode specified = {0}", servicePackageActivationModeStr);
        return false;
    }

    FABRIC_SERVICE_GROUP_DESCRIPTION serviceGroupDescription = {0};
    FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION serviceGroupMember[100];

    FABRIC_SERVICE_DESCRIPTION serviceDescription;

    FABRIC_STATEFUL_SERVICE_DESCRIPTION statefulDescription = {0};
    FABRIC_STATELESS_SERVICE_DESCRIPTION statelessDescription = {0};

    FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1 statefulEx1 = { 0 };
    FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1 statelessEx1 = { 0 };

    FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2 statefulEx2 = { 0 };
    FABRIC_STATELESS_SERVICE_DESCRIPTION_EX2 statelessEx2 = { 0 };

    FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX3 statefulEx3 = { FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE_SHARED_PROCESS };
    FABRIC_STATELESS_SERVICE_DESCRIPTION_EX3 statelessEx3 = { FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE_SHARED_PROCESS };

    FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS failoverSettings = {0};
    FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_EX1 failoverSettingsEx1 = {0};

    union
    {
        FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION uniformInt64;
        FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION named;
    } partitionDescription;
    vector<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION> metricDescriptions;
    metricDescriptions.resize(metrics.size());
    for (int i = 0; i < static_cast<int>(metrics.size()); i++)
    {
        metricDescriptions[i].Name = metrics[i].Name.c_str();
        if (metrics[i].Weight == 0.0)
        {
            metricDescriptions[i].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO;
        }
        else if (metrics[i].Weight <= 0.01)
        {
            metricDescriptions[i].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW;
        }
        else if (metrics[i].Weight <= 0.1)
        {
            metricDescriptions[i].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM;
        }
        else
        {
            metricDescriptions[i].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH;
        }
        metricDescriptions[i].PrimaryDefaultLoad = metrics[i].PrimaryDefaultLoad;
        metricDescriptions[i].SecondaryDefaultLoad = metrics[i].SecondaryDefaultLoad;
    }

    vector<FABRIC_SERVICE_CORRELATION_DESCRIPTION> serviceCorrelationDescriptions;
    serviceCorrelationDescriptions.resize(serviceCorrelations.size());
    for (int i = 0; i < static_cast<int>(serviceCorrelations.size()); i++)
    {
        serviceCorrelationDescriptions[i].ServiceName = serviceCorrelations[i].ServiceName.c_str();
        serviceCorrelationDescriptions[i].Scheme = serviceCorrelations[i].Scheme;
    }

    memset(serviceGroupMember, 0, sizeof(serviceGroupMember));
    serviceGroupDescription.Description = &serviceDescription;
    serviceGroupDescription.MemberCount = (ULONG)serviceNumber;
    serviceGroupDescription.MemberDescriptions = serviceGroupMember;
    for (int i=0; i<serviceNumber; i++)
    {
        serviceGroupMember[i].ServiceName = params[6+i*2].c_str();
        serviceGroupMember[i].ServiceType = params[6+i*2+1].c_str();
        serviceGroupMember[i].InitializationDataSize = NULL;

        auto initMember = initData.find(params[6+i*2]);
        if (initMember != end(initData))
        {
            serviceGroupMember[i].InitializationData = reinterpret_cast<const byte*>(initMember->second.c_str());
            serviceGroupMember[i].InitializationDataSize = (static_cast<ULONG>(initMember->second.size()) + 1) * sizeof(wchar_t);
        }
    }
    if (isStateful)
    {
        serviceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        serviceDescription.Value = &statefulDescription;
        statefulDescription.ServiceName = nameString.c_str();
        statefulDescription.ServiceTypeName = type.c_str();
        statefulDescription.InitializationData = NULL;
        statefulDescription.InitializationDataSize = 0;
        statefulDescription.PartitionScheme = partitionScheme;
        statefulDescription.PartitionSchemeDescription = &partitionDescription;
        statefulDescription.TargetReplicaSetSize = targetReplicaSetSize;
        statefulDescription.MinReplicaSetSize = minReplicaSetSize;
        statefulDescription.PlacementConstraints = placementConstraints.c_str();
        statefulDescription.CorrelationCount = static_cast<ULONG>(serviceCorrelationDescriptions.size());
        statefulDescription.Correlations = serviceCorrelationDescriptions.data();
        statefulDescription.MetricCount = static_cast<ULONG>(metrics.size());
        statefulDescription.Metrics = metricDescriptions.data();
        statefulDescription.HasPersistedState = hasPersistedState;
        statefulDescription.Reserved = &statefulEx1;

        failoverSettings.Reserved = &failoverSettingsEx1;
        statefulEx1.FailoverSettings = &failoverSettings;

        if (replicaRestartWaitDurationSeconds != -1)
        {
            failoverSettings.Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION;
            failoverSettings.ReplicaRestartWaitDurationSeconds = replicaRestartWaitDurationSeconds;
        }

        if (quorumLossWaitDurationSeconds != -1)
        {
            failoverSettings.Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION;
            failoverSettings.QuorumLossWaitDurationSeconds = quorumLossWaitDurationSeconds;
        }

        if (standByReplicaKeepDurationSeconds != -1)
        {
            failoverSettings.Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION;
            failoverSettingsEx1.StandByReplicaKeepDurationSeconds = standByReplicaKeepDurationSeconds;
        }

        statefulEx1.Reserved = &statefulEx2;
        statefulEx2.IsDefaultMoveCostSpecified = isDefaultMoveCostSpecified;
        statefulEx2.DefaultMoveCost = static_cast<FABRIC_MOVE_COST>(defaultMoveCost);

        statefulEx2.Reserved = &statefulEx3;
        statefulEx3.ServicePackageActivationMode = ServiceModel::ServicePackageActivationMode::ToPublicApi(activationMode);
    }
    else
    {
        serviceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        serviceDescription.Value = &statelessDescription;
        statelessDescription.ServiceName = nameString.c_str();
        statelessDescription.ServiceTypeName = type.c_str();
        statelessDescription.InitializationData = NULL;
        statelessDescription.InitializationDataSize = 0;
        statelessDescription.PartitionScheme = partitionScheme;
        statelessDescription.PartitionSchemeDescription = &partitionDescription;
        statelessDescription.InstanceCount = targetReplicaSetSize;
        statelessDescription.PlacementConstraints = placementConstraints.c_str();
        statelessDescription.CorrelationCount = static_cast<ULONG>(serviceCorrelationDescriptions.size());
        statelessDescription.Correlations = serviceCorrelationDescriptions.data();
        statelessDescription.MetricCount = static_cast<ULONG>(metrics.size());
        statelessDescription.Metrics = metricDescriptions.data();

        statelessDescription.Reserved = &statelessEx1;
        statelessEx1.PolicyList = NULL;

        statelessEx1.Reserved = &statelessEx2;
        statelessEx2.IsDefaultMoveCostSpecified = isDefaultMoveCostSpecified;
        statelessEx2.DefaultMoveCost = static_cast<FABRIC_MOVE_COST>(defaultMoveCost);

        statelessEx2.Reserved = &statefulEx3;
        statelessEx3.ServicePackageActivationMode = ServiceModel::ServicePackageActivationMode::ToPublicApi(activationMode);
    }

    switch (partitionScheme)
    {
    case FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
        partitionDescription.uniformInt64.PartitionCount = partitionCount;
        partitionDescription.uniformInt64.LowKey = FabricTestSessionConfig::GetConfig().ServiceDescriptorLowRange;
        partitionDescription.uniformInt64.HighKey = FabricTestSessionConfig::GetConfig().ServiceDescriptorHighRange;
        break;
    case FABRIC_PARTITION_SCHEME_NAMED:
        partitionDescription.named.PartitionCount = partitionCount;
        partitionDescription.named.Names = partitionNames.data();
        break;
    }

    CreateServiceGroup(serviceGroupDescription, expectedError);

    bool forceUpdatePsd = parser.GetBool(L"forceUpdatePsd");

    if(expectedError == S_OK || forceUpdatePsd)
    {
        PartitionedServiceDescriptor serviceDescriptor;
        GetPartitionedServiceDescriptor(nameString, serviceDescriptor);
        FABRICSESSION.FabricDispatcher.ServiceMap.AddPartitionedServiceDescriptor(nameString, move(serviceDescriptor));
    }

    return true;
}

bool TestFabricClient::UpdateServiceGroup(StringCollection const & params)
{
    if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::UpdateServiceGroupCommand);
        return false;
    }

    NamingUri serviceGroupNameUri;
    if (!NamingUri::TryParse(params[0], serviceGroupNameUri))
    {
        TestSession::WriteError(TraceSource, "Could not parse service group name {0}.", params[0]);
        return false;
    }

    wstring serviceGroupName = serviceGroupNameUri.ToString();

    if (params[1] != L"Stateless" && params[1] != L"Stateful")
    {
        TestSession::WriteError(TraceSource, "Could not parse service kind {0}.", params[1]);
        return false;
    }

    bool isStateful = (params[1] == L"Stateful");

    CommandLineParser parser(params, 2);

    FABRIC_SERVICE_GROUP_UPDATE_DESCRIPTION serviceGroupUpdateDescription = {0};
    FABRIC_SERVICE_UPDATE_DESCRIPTION serviceUpdateDescription;
    FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION statefulUpdateDescription = {0};
    FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX1 statefulUpdateDescriptionEx1 = {0};
    FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX2 statefulUpdateDescriptionEx2 = {0};
    FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION statelessUpdateDescription = {0};

    if (isStateful)
    {
        serviceUpdateDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        serviceUpdateDescription.Value = &statefulUpdateDescription;
        statefulUpdateDescription.Reserved = &statefulUpdateDescriptionEx1;
        statefulUpdateDescriptionEx1.Reserved = &statefulUpdateDescriptionEx2;

        int targetReplicaSetSize;
        if (parser.TryGetInt(L"TargetReplicaSetSize", targetReplicaSetSize, -1))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_TARGET_REPLICA_SET_SIZE;
            statefulUpdateDescription.TargetReplicaSetSize = targetReplicaSetSize;
        }

        int replicaRestartWaitDurationSeconds;
        if (parser.TryGetInt(L"ReplicaRestartWaitDuration", replicaRestartWaitDurationSeconds, -1))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_REPLICA_RESTART_WAIT_DURATION;
            statefulUpdateDescription.ReplicaRestartWaitDurationSeconds = replicaRestartWaitDurationSeconds;
        }

        int quorumLossWaitDurationSeconds;
        if (parser.TryGetInt(L"QuorumLossWaitDuration", quorumLossWaitDurationSeconds, -1))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_QUORUM_LOSS_WAIT_DURATION;
            statefulUpdateDescription.QuorumLossWaitDurationSeconds = quorumLossWaitDurationSeconds;
        }

        int standByReplicaKeepDurationSeconds;
        if (parser.TryGetInt(L"StandByReplicaKeepDuration", standByReplicaKeepDurationSeconds, -1))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_STANDBY_REPLICA_KEEP_DURATION;
            statefulUpdateDescriptionEx1.StandByReplicaKeepDurationSeconds = standByReplicaKeepDurationSeconds;
        }

        int minReplicaSetSize;
        if (parser.TryGetInt(L"MinReplicaSetSize", minReplicaSetSize, -1))
        {
            statefulUpdateDescription.Flags |= FABRIC_STATEFUL_SERVICE_MIN_REPLICA_SET_SIZE;
            statefulUpdateDescriptionEx2.MinReplicaSetSize = minReplicaSetSize;
        }
    }
    else
    {
        serviceUpdateDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        serviceUpdateDescription.Value = &statelessUpdateDescription;

        int instanceCount;
        if (parser.TryGetInt(L"InstanceCount", instanceCount, -1))
        {
            statelessUpdateDescription.Flags |= FABRIC_STATELESS_SERVICE_INSTANCE_COUNT;
            statelessUpdateDescription.InstanceCount = instanceCount;
        }
    }

    serviceGroupUpdateDescription.Description = &serviceUpdateDescription;

    vector<HRESULT> expectedErrors;
    expectedErrors.push_back(S_OK);

    UpdateServiceGroup(serviceGroupName, serviceGroupUpdateDescription, expectedErrors);

    PartitionedServiceDescriptor serviceDescriptor;
    GetPartitionedServiceDescriptor(serviceGroupName, serviceDescriptor);

    FABRICSESSION.FabricDispatcher.ServiceMap.UpdatePartitionedServiceDescriptor(serviceGroupName, serviceDescriptor.Service);

    return true;
}

bool TestFabricClient::DeployServicePackages(StringCollection const & params)
{
    if (params.size() < 5 || params.size() > 8)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeployServicePackageCommand);
        return false;
    }

    CommandLineParser parser(params);
    wstring applicationTypeName;
    wstring applicationTypeVersion;
    wstring serviceManifestName;
    wstring nodeName;
    wstring nodeId;
    if (!parser.TryGetString(FabricTestConstants::ApplicationTypeName, applicationTypeName))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeployServicePackageCommand);
        return false;
    }
    if (!parser.TryGetString(FabricTestConstants::ApplicationTypeVersion, applicationTypeVersion))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeployServicePackageCommand);
        return false;
    }
    if (!parser.TryGetString(FabricTestConstants::ServiceManifestName, serviceManifestName))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeployServicePackageCommand);
        return false;
    }
    if (!parser.TryGetString(FabricTestConstants::NodeId, nodeId))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeployServicePackageCommand);
        return false;
    }
    nodeName = wformatString("nodeid:{0}", nodeId);

    wstring sharedPackageNames;
    parser.TryGetString(FabricTestConstants::SharedPackageNames, sharedPackageNames);
    wstring sharingScope;
    parser.TryGetString(FabricTestConstants::SharingScope, sharingScope);

    FABRIC_PACKAGE_SHARING_POLICY scopePolicy;

    if (StringUtility::AreEqualCaseInsensitive(sharingScope, L"All"))
    {
        scopePolicy.Scope = FABRIC_PACKAGE_SHARING_POLICY_SCOPE_ALL;
    }
    else if (StringUtility::AreEqualCaseInsensitive(sharingScope, L"Code"))
    {
        scopePolicy.Scope = FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CODE;
    }
    else if (StringUtility::AreEqualCaseInsensitive(sharingScope, L"Config"))
    {
        scopePolicy.Scope = FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CONFIG;
    }
    else if (StringUtility::AreEqualCaseInsensitive(sharingScope, L"Data"))
    {
        scopePolicy.Scope = FABRIC_PACKAGE_SHARING_POLICY_SCOPE_DATA;
    }
    else
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeployServicePackageCommand);
        return false;
    }

    ScopedHeap heap;

    FABRIC_PACKAGE_SHARING_POLICY_LIST policies;
    size_t size = 0;
    vector<wstring> packageNames;
    wstring delimiter = L"|";
    StringUtility::Split(sharedPackageNames, packageNames, delimiter);
    size += packageNames.size();
    if (scopePolicy.Scope != FABRIC_PACKAGE_SHARING_POLICY_SCOPE_NONE)
    {
        size++;
    }

    auto policyArray = heap.AddArray<FABRIC_PACKAGE_SHARING_POLICY>(size);
    policies.Count = (ULONG)policyArray.GetCount();
    policies.Items = policyArray.GetRawArray();

    int count = 0;
    for (auto iter = packageNames.begin(); iter != packageNames.end(); ++iter)
    {
        policyArray[count++].PackageName = (*iter).c_str();
    }
    if (scopePolicy.Scope != FABRIC_PACKAGE_SHARING_POLICY_SCOPE_NONE)
    {
        policyArray[count].Scope = scopePolicy.Scope;
    }

    wstring cacheResults;
    parser.TryGetString(FabricTestConstants::VerifyInCache, cacheResults);
    vector<wstring> expectedInCache;
    StringUtility::Split(cacheResults, expectedInCache, delimiter);

    wstring sharedResults;
    vector<wstring> expectedShared;
    parser.TryGetString(FabricTestConstants::VerifyShared, sharedResults);
    StringUtility::Split(sharedResults, expectedShared, delimiter);

    DeployServicePackageToNode(
        applicationTypeName,
        applicationTypeVersion,
        serviceManifestName,
        nodeName,
        nodeId,
        policies,
        expectedInCache,
        expectedShared);

    return true;
}

bool TestFabricClient::VerifyDeployedCodePackageCount(StringCollection const & params)
{
    if (params.size() < 4 || params.size() > 5)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyDeployedCodePackageCountCommand);
        return false;
    }

    CommandLineParser parser(params);
    ULONG expectedCount;
    wstring nodeIdList;
    wstring applicationName;
    wstring serviceManifestName;
    wstring servicePackageActivationId;

    if (!parser.TryGetULONG(FabricTestConstants::ExpectedCount, expectedCount))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyDeployedCodePackageCountCommand);
        return false;
    }
    if (!parser.TryGetString(FabricTestConstants::NodeIdList, nodeIdList))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyDeployedCodePackageCountCommand);
        return false;
    }
    if (!parser.TryGetString(FabricTestConstants::ApplicationName, applicationName))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyDeployedCodePackageCountCommand);
        return false;
    }

    parser.TryGetString(FabricTestConstants::ServiceManifestName, serviceManifestName);

    vector<wstring> nodeIds;
    StringUtility::Split(nodeIdList, nodeIds, wstring(L","));

    if (nodeIds.empty())
    {
        TestSession::FailTestIf(nodeIds.empty(), "VerifyDeployedCodePackageCount: No nodeIds specified.");
    }

    vector<wstring> nodeNames;
    for (auto const & nodeId : nodeIds)
    {
        nodeNames.push_back(wformatString("nodeid:{0}", nodeId));
    }

    ULONG codePackageCount = 0;
    for (auto const & nodeName : nodeNames)
    {
        auto count = VerifyDeployedCodePackageCount(nodeName, applicationName, serviceManifestName);

        TestSession::WriteNoise(
            TraceSource,
            "VerifyDeployedCodePackageCount: NodeName={0}, ApplicationName={1}, ServiceManifestName={2}, CodePackageCount={3}",
            nodeName,
            applicationName,
            serviceManifestName,
            count);

        codePackageCount += count;
    }

    TestSession::FailTestIfNot(
        codePackageCount == expectedCount,
        "VerifyDeployedCodePackageCount mismatch '{0}' != '{1}'.",
        codePackageCount,
        expectedCount);

    return true;
}

ULONG TestFabricClient::VerifyDeployedCodePackageCount(
    wstring const & nodeName,
    wstring const & applicationName,
    wstring const & serviceManifestName)
{
    auto appClient = CreateFabricQueryClient10(FABRICSESSION.FabricDispatcher.Federation);

    vector<HRESULT> retryableErrors;
    retryableErrors.push_back(S_OK);

    FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_DESCRIPTION queryDesc = {};
    queryDesc.NodeName = nodeName.c_str();
    queryDesc.ApplicationName = applicationName.c_str();
    queryDesc.ServiceManifestNameFilter = serviceManifestName.c_str();

    ComPointer<IFabricGetDeployedCodePackageListResult> resultCPtr;

    this->PerformFabricClientOperation(
        L"BeginGetDeployedServicePackageList",
        FabricTestSessionConfig::GetConfig().VerifyTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return appClient->BeginGetDeployedCodePackageList(
                &queryDesc,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return appClient->EndGetDeployedCodePackageList(context.GetRawPointer(), resultCPtr.InitializationAddress());
        });

    auto result = resultCPtr->get_DeployedCodePackageList();

    TestSession::FailTestIf(result == nullptr, "VerifyDeployedCodePackageCount: Query returned nullptr.");

    return result->Count;
}

void TestFabricClient::CreateName(NamingUri const& name, wstring const& clientName, HRESULT expectedError)
{
    vector<HRESULT> expectedErrors(1, expectedError);
    CreateName(name, clientName, expectedErrors);
}

void TestFabricClient::CreateName(NamingUri const& name, wstring const& clientName, vector<HRESULT> expectedErrors)
{
    TestSession::WriteNoise(TraceSource, "CreateName called for name {0}, expected error {1}", name, expectedErrors);

    ComPointer<IFabricPropertyManagementClient> nameClient;
    if (!FABRICSESSION.FabricDispatcher.TryGetNamedFabricClient(clientName, IID_IFabricPropertyManagementClient, nameClient))
    {
        nameClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);
    }

    this->PerformFabricClientOperation(
        L"CreateName",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return nameClient->BeginCreateName(
                name.ToString().c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return nameClient->EndCreateName(context.GetRawPointer());
        },
        expectedErrors,
        FABRIC_E_NAME_ALREADY_EXISTS);
}

void TestFabricClient::DeleteName(NamingUri const& name, wstring const& clientName, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "DeleteName called for name {0}, expected error {1}", name, expectedError);

    ComPointer<IFabricPropertyManagementClient> nameClient;
    if (!FABRICSESSION.FabricDispatcher.TryGetNamedFabricClient(clientName, IID_IFabricPropertyManagementClient, nameClient))
    {
        nameClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);
    }

    this->PerformFabricClientOperation(
        L"DeleteName",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return nameClient->BeginDeleteName(
                name.ToString().c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return nameClient->EndDeleteName(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_NAME_DOES_NOT_EXIST);
}

int TestFabricClient::EnumerateNames(
    NamingUri const& parentName,
    bool doRecursive,
    wstring const & clientName,
    size_t maxResults,
    bool checkExpectedNames,
    __in StringCollection & expectedNames,
    bool checkStatus,
    FABRIC_ENUMERATION_STATUS expectedStatus,
    HRESULT expectedError)
{
    TestSession::WriteNoise(
        TraceSource,
        "EnumerateNames called for name {0}, maxResults {1}, expected status {2}, expected error {3}",
        parentName,
        maxResults,
        static_cast<int>(expectedStatus),
        expectedError);

    ComPointer<IFabricNameEnumerationResult> currentResultCPtr = FABRICSESSION.FabricDispatcher.NamingState.GetNameEnumerationToken(parentName);

    ComPointer<IFabricPropertyManagementClient> nameClient;
    if (!FABRICSESSION.FabricDispatcher.TryGetNamedFabricClient(clientName, IID_IFabricPropertyManagementClient, nameClient))
    {
        nameClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);
    }

    HRESULT hr = S_OK;
    bool done = true;
    int nameCount = 0;
    StringCollection enumeratedNames;

    do
    {
        ComPointer<IFabricNameEnumerationResult> previousResultCPtr = currentResultCPtr;
        currentResultCPtr.SetNoAddRef(NULL);

        hr = this->PerformFabricClientOperation(
            L"EnumerateSubnames",
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
            {
                return nameClient->BeginEnumerateSubNames(
                    parentName.ToString().c_str(),
                    previousResultCPtr.GetRawPointer(), // Pass enumerator from previous enumeration to continue
                    (doRecursive ? TRUE : FALSE),
                    timeout,
                    callback.GetRawPointer(),
                    context.InitializationAddress());
            },
            [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
            {
                return nameClient->EndEnumerateSubNames(context.GetRawPointer(), currentResultCPtr.InitializationAddress());
            },
            expectedError);

        if (SUCCEEDED(hr))
        {
            ULONG count = 0;
            FABRIC_URI const * array;
            hr = currentResultCPtr->GetNames(&count, &array);

            TestSession::FailTestIfNot(SUCCEEDED(hr), "GetStrings() failed with hr = {0}", hr);

            TestSession::WriteInfo(TraceSource, "Batch count: {0}", count);
            for (ULONG ix = 0; ix < count; ++ix)
            {
                wstring subname = wstring(array[ix]);
                TestSession::WriteInfo(TraceSource, "'{0}'", subname);

                enumeratedNames.push_back(subname);
            }
            done = (count == 0 || count >= maxResults);
            nameCount += count;

            FABRICSESSION.FabricDispatcher.NamingState.SetNameEnumerationToken(parentName, currentResultCPtr);
        }
        else
        {
            done = true;
        }

    } while (!done);

    // Only verify the final enumeration status
    //
    if (SUCCEEDED(hr))
    {
        if (checkStatus)
        {
            FABRIC_ENUMERATION_STATUS status = currentResultCPtr->get_EnumerationStatus();

            TestSession::FailTestIfNot(
                status == expectedStatus,
                "FABRIC_ENUMERATION_STATUS mismatch: expected {0} actual {1}",
                static_cast<int>(expectedStatus),
                static_cast<int>(status));

            if ((status & FABRIC_ENUMERATION_FINISHED_MASK) && (expectedStatus & FABRIC_ENUMERATION_FINISHED_MASK))
            {
                FABRICSESSION.FabricDispatcher.NamingState.ClearNameEnumerationResults(parentName);
            }
        }

        if (checkExpectedNames)
        {
            sort(expectedNames.begin(), expectedNames.end());
            sort(enumeratedNames.begin(), enumeratedNames.end());

            TestSession::FailTestIfNot(
                expectedNames.size() == enumeratedNames.size(),
                "Enumeration mismatch: expected = {0} enumerated = {1}",
                expectedNames,
                enumeratedNames);

            for (size_t ix = 0; ix < expectedNames.size(); ++ix)
            {
                TestSession::FailTestIfNot(
                    expectedNames[ix] == enumeratedNames[ix],
                    "Enumeration mismatch: expected = {0} enumerated = {1}",
                    expectedNames,
                    enumeratedNames);
            }
        }
    }

    return nameCount;
}

void TestFabricClient::EnumerateProperties(
    NamingUri const & name,
    bool includeValues,
    size_t maxResults,
    FABRIC_ENUMERATION_STATUS expectedStatus,
    HRESULT expectedError)
{
    TestSession::WriteNoise(
        TraceSource,
        "EnumerateProperties(name = {0} includeValues = {1} maxresults = {2} expected status = {3} expected error = {4})",
        name,
        includeValues,
        maxResults,
        static_cast<int>(expectedStatus),
        expectedError);

    auto propertyClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);
    ComPointer<IFabricPropertyEnumerationResult> currentResultCPtr = FABRICSESSION.FabricDispatcher.NamingState.GetPropertyEnumerationToken(name);

    HRESULT hr = S_OK;
    size_t totalCount = 0;
    bool done = false;

    do
    {
        ComPointer<IFabricPropertyEnumerationResult> previousResultCPtr = currentResultCPtr;
        currentResultCPtr.SetNoAddRef(NULL);

        hr = this->PerformFabricClientOperation(
            L"EnumerateProperties",
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
            {
                return propertyClient->BeginEnumerateProperties(
                    name.ToString().c_str(),
                    includeValues,
                    previousResultCPtr.GetRawPointer(),
                    timeout,
                    callback.GetRawPointer(),
                    context.InitializationAddress());
            },
            [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
            {
                return propertyClient->EndEnumerateProperties(context.GetRawPointer(), currentResultCPtr.InitializationAddress());
            },
            expectedError);

        if (SUCCEEDED(hr))
        {
            ULONG count = currentResultCPtr->get_PropertyCount();

            if (count > 0)
            {
                TestSession::WriteInfo(TraceSource, "Property enumeration result count for {0}: {1}", name, count);
                for (ULONG ix = 0; ix < count; ++ix)
                {
                    ComPointer<IFabricPropertyValueResult> propertyCPtr;
                    hr = currentResultCPtr->GetProperty(ix, propertyCPtr.InitializationAddress());

                    TestSession::FailTestIfNot(SUCCEEDED(hr), "GetProperty() failed with hr = {0}: index = {1}", hr, ix);

                    FABRIC_NAMED_PROPERTY const * property = propertyCPtr->get_Property();

                    vector<byte> bytes;
                    if (includeValues)
                    {
                        bytes.insert(bytes.begin(), property->Value, property->Value + property->Metadata->ValueSize);
                    }

                    wstring customTypeId;
                    if (property->Metadata->Reserved != NULL)
                    {
                        auto extended = reinterpret_cast<FABRIC_NAMED_PROPERTY_METADATA_EX1*>(property->Metadata->Reserved);
                        customTypeId = wstring(extended->CustomTypeId);
                    }

                    FABRICSESSION.FabricDispatcher.NamingState.AddPropertyEnumerationResult(
                        name,
                        property->Metadata->PropertyName,
                        property->Metadata->TypeId,
                        bytes,
                        customTypeId);

                    TestSession::WriteInfo(
                        TraceSource,
                        "{0} ({1}) {2} bytes ({3})",
                        property->Metadata->PropertyName,
                        static_cast<int>(property->Metadata->TypeId),
                        bytes.size(),
                        customTypeId);
                }

                totalCount += count;
                done = (totalCount >= maxResults);
            }
            else
            {
                done = true;
            }

            FABRICSESSION.FabricDispatcher.NamingState.SetPropertyEnumerationToken(name, currentResultCPtr);
        }
        else
        {
            done = true;
        }
    } while (!done);

    // Only verify the final enumeration status
    //
    if (SUCCEEDED(hr))
    {
        FABRIC_ENUMERATION_STATUS status = currentResultCPtr->get_EnumerationStatus();

        TestSession::FailTestIfNot(
            status == expectedStatus,
            "FABRIC_ENUMERATION_STATUS mismatch: expected {0} actual {1}",
            static_cast<int>(expectedStatus),
            static_cast<int>(status));

    }
}

void TestFabricClient::VerifyPropertyEnumeration(NamingUri const & name, bool includeValues)
{
    bool success = FABRICSESSION.FabricDispatcher.NamingState.VerifyPropertyEnumerationResults(name, includeValues);

    TestSession::FailTestIfNot(success, "VerifyPropertyEnumeration failed");

    FABRICSESSION.FabricDispatcher.NamingState.ClearPropertyEnumerationResults(name);
}

void TestFabricClient::NameExists(NamingUri const& name, bool expectedExists, wstring const& clientName, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "NameExists called for name {0}, expected error {1}, expected exists {2}", name, expectedError, expectedExists);
    bool exists = NameExistsInternal(name, clientName, expectedError);
    if (expectedError == S_OK)
    {
        TestSession::FailTestIfNot(expectedExists == exists, "NameExists check failed. Expected {0}.", expectedExists);
    }
}

bool TestFabricClient::NameExistsInternal(NamingUri const& name, wstring const& clientName, HRESULT expectedError)
{
    BOOLEAN exists = FALSE;

    ComPointer<IFabricPropertyManagementClient> nameClient;
    if (!FABRICSESSION.FabricDispatcher.TryGetNamedFabricClient(clientName, IID_IFabricPropertyManagementClient, nameClient))
    {
        nameClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);
    }

    this->PerformFabricClientOperation(
        L"NameExists",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return nameClient->BeginNameExists(
                name.ToString().c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return nameClient->EndNameExists(context.GetRawPointer(), &exists);
        },
        expectedError);

    return (exists == TRUE);
}

void TestFabricClient::PutProperty(
    NamingUri const& name,
    std::wstring const& propertyName,
    std::wstring const& value,
    HRESULT expectedError,
    HRESULT retryableError)
{
    TestSession::WriteNoise(TraceSource, "PutProperty called for name {0}, expected error {1}, propertyName {2}, value {3}", name, expectedError, propertyName, value);

    vector<BYTE> bytes = Serialize(value);

    auto propertyClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);

    HRESULT hr = this->PerformFabricClientOperation(
        L"PutProperty",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return propertyClient->BeginPutPropertyBinary(
                name.Name.c_str(),
                propertyName.c_str(),
                static_cast<int>(bytes.size()),
                bytes.data(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return propertyClient->EndPutPropertyBinary(context.GetRawPointer());
        },
        expectedError,
        S_OK,
        retryableError);

    if (SUCCEEDED(hr))
    {
        FABRICSESSION.FabricDispatcher.NamingState.AddProperty(
            name,
            propertyName,
            FABRIC_PROPERTY_TYPE_BINARY,
            bytes,
            L"");
    }
}

bool TestFabricClient::PutCustomProperty(
    NamingUri const& name,
    FABRIC_PUT_CUSTOM_PROPERTY_OPERATION const & operation,
    HRESULT expectedError,
    HRESULT retryableError)
{
    auto propertyClient = CreateFabricPropertyClient2(FABRICSESSION.FabricDispatcher.Federation);

    HRESULT hr = this->PerformFabricClientOperation(
            L"PutCustomProperty",
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
            {
                return propertyClient->BeginPutCustomPropertyOperation(
                    name.Name.c_str(),
                    &operation,
                    timeout,
                    callback.GetRawPointer(),
                    context.InitializationAddress());
            },
            [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
            {
                return propertyClient->EndPutCustomPropertyOperation(context.GetRawPointer());
            },
            expectedError,
            S_OK,
            retryableError);

    return SUCCEEDED(hr);
}

bool TestFabricClient::SubmitPropertyBatch(
    Common::NamingUri const& name,
    wstring const& clientName,
    StringCollection const& batchOperations,
    StringCollection const& expectedGetProperties,
    ULONG expectedFailedIndex,
    HRESULT expectedError)
{
    TestSession::WriteNoise(
        TraceSource,
        "SubmitPropertyBatch called for name {0}, {1} operations, expected result {2}, expected error {3}, expected failed index {4}",
        name,
        batchOperations,
        expectedGetProperties,
        expectedError,
        expectedFailedIndex);

    vector<FABRIC_PROPERTY_BATCH_OPERATION> operations;

    // Keep the data for Put operations alive
    vector<NamedPropertyData> putOperations;
    putOperations.reserve(batchOperations.size());

    // Keep the data for the other check value operations alive
    vector<vector<byte>> checkValueBytes;
    checkValueBytes.reserve(batchOperations.size());

    ULONG index = 0;

    // keep the properties alive
    vector<unique_ptr<FABRIC_GET_PROPERTY_OPERATION>> nativeGetOps;
    vector<unique_ptr<FABRIC_PUT_PROPERTY_OPERATION>> nativePutOps;
    vector<unique_ptr<FABRIC_PUT_CUSTOM_PROPERTY_OPERATION>> nativePutCustomOps;
    vector<unique_ptr<FABRIC_DELETE_PROPERTY_OPERATION>> nativeDeleteOps;
    vector<unique_ptr<FABRIC_CHECK_SEQUENCE_PROPERTY_OPERATION>> nativeCheckSequenceOps;
    vector<unique_ptr<FABRIC_CHECK_EXISTS_PROPERTY_OPERATION>> nativeCheckExistsOps;
    vector<unique_ptr<FABRIC_CHECK_VALUE_PROPERTY_OPERATION>> nativeCheckValueOps;

    vector<unique_ptr<FABRIC_OPERATION_DATA_BUFFER>> nativePutOpBuffer;

    // allocate capacity upfront - don't want the vector to resize since
    // we reference pointers to the property names in the vector
    //
    vector<wstring> propNames;
    propNames.reserve(batchOperations.size());

    for (auto it = batchOperations.begin(); it != batchOperations.end(); ++it)
    {
        FABRIC_PROPERTY_BATCH_OPERATION nativeOp;
        vector<wstring> splitItems;
        StringUtility::Split<wstring>(*it, splitItems, L":");

        if (splitItems.size() <= 1)
        {
            TestSession::WriteError(
                TraceSource,
                "Could not parse operation {0}.", *it);
            return false;
        }

        propNames.push_back(splitItems[1]);

        wstring const & propName = propNames.back();

        if (splitItems[0] == L"get" || splitItems[0] == L"getmetadata")
        {
            TestSession::WriteNoise(
                TraceSource,
                "Operation {0}: GetProperty {1}.", splitItems[0], propName);
            nativeOp.Kind = FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET;
            auto getOp = make_unique<FABRIC_GET_PROPERTY_OPERATION>();
            getOp->PropertyName = propName.c_str();
            getOp->IncludeValue = (splitItems[0] == L"get");
            getOp->Reserved = NULL;
            nativeOp.Value = reinterpret_cast<void*>(getOp.get());
            nativeGetOps.push_back(move(getOp));
        }
        else if (splitItems[0] == L"put")
        {
            wstring const & propValue = splitItems[2];
            vector<BYTE> bytes = Serialize(propValue);

            wstring customTypeId;
            if (splitItems.size() > 3)
            {
                if (splitItems.size() != 4)
                {
                    TestSession::WriteError(
                        TraceSource,
                        "Could not parse Put operation {0}.", *it);
                    return false;
                }

                customTypeId = splitItems[3];
                if (customTypeId.empty())
                {
                    TestSession::WriteError(
                        TraceSource,
                        "CustomTypeId can't be empty: {0}.", *it);
                    return false;
                }
            }

            TestSession::WriteNoise(
                TraceSource,
                "Operation {0}: PutProperty {1} with value {2}, customTypeId '{3}'.", *it, propName, propValue, customTypeId);

            auto dataBuffer = make_unique<FABRIC_OPERATION_DATA_BUFFER>();
            dataBuffer->BufferSize = static_cast<ULONG>(bytes.size());
            NamedPropertyData putOpData = { propName, FABRIC_PROPERTY_TYPE_BINARY, move(bytes), customTypeId };

            putOperations.push_back(move(putOpData));
            dataBuffer->Buffer = &(putOperations.back().Bytes[0]);
            nativePutOpBuffer.push_back(move(dataBuffer));

            if (splitItems.size() > 3)
            {
                nativeOp.Kind = FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT_CUSTOM;
                auto putCustomOp = make_unique<FABRIC_PUT_CUSTOM_PROPERTY_OPERATION>();
                putCustomOp->PropertyName = propName.c_str();
                putCustomOp->PropertyTypeId = FABRIC_PROPERTY_TYPE_BINARY;
                putCustomOp->PropertyCustomTypeId =  putOperations.back().CustomTypeId.c_str();
                putCustomOp->PropertyValue = reinterpret_cast<void*>(nativePutOpBuffer.back().get());
                putCustomOp->Reserved = NULL;

                nativeOp.Value = reinterpret_cast<void*>(putCustomOp.get());
                nativePutCustomOps.push_back(move(putCustomOp));
            }
            else
            {
                nativeOp.Kind = FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT;
                auto putOp = make_unique<FABRIC_PUT_PROPERTY_OPERATION>();
                putOp->PropertyName = propName.c_str();
                putOp->PropertyTypeId = FABRIC_PROPERTY_TYPE_BINARY;
                putOp->PropertyValue = reinterpret_cast<void*>(nativePutOpBuffer.back().get());
                putOp->Reserved = NULL;

                nativeOp.Value = reinterpret_cast<void*>(putOp.get());
                nativePutOps.push_back(move(putOp));
            }
        }
        else if (splitItems[0] == L"delete")
        {
            TestSession::WriteNoise(
                TraceSource,
                "DeleteProperty {0}.", propName);
            nativeOp.Kind = FABRIC_PROPERTY_BATCH_OPERATION_KIND_DELETE;
            auto op = make_unique<FABRIC_DELETE_PROPERTY_OPERATION>();
            op->PropertyName = propName.c_str();
            op->Reserved = NULL;
            nativeOp.Value = reinterpret_cast<void*>(op.get());
            nativeDeleteOps.push_back(move(op));
        }
        else if (splitItems[0] == L"checksequence")
        {
            if (splitItems.size() != 3)
            {
                TestSession::WriteError(
                    TraceSource,
                    "Could not parse CheckSequenceProperty {0}.", *it);
                return false;
            }

            LONGLONG propSequence = 0;
            auto propSequenceString = splitItems[2];

            if (StringUtility::StartsWith(propSequenceString, L"$"))
            {
                propSequenceString = propSequenceString.substr(1);

                uint64 seqNumResult;
                TestSession::FailTestIfNot(
                    TryLoadSequenceNumber(propSequenceString, seqNumResult),
                    "Could not load sequence number from '{0}'", 
                    propSequenceString);

                propSequence = static_cast<LONGLONG>(seqNumResult);
            }
            else
            {
                TestSession::FailTestIfNot(
                    StringUtility::TryFromWString(propSequenceString, propSequence), 
                    "Could not parse SequenceNumber {0} ({1})", 
                    propSequenceString,
                    *it);
            }

            TestSession::WriteNoise(
                TraceSource,
                "CheckSequenceProperty {0}, #{1}.", propName, propSequence);
            nativeOp.Kind = FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE;
            auto op = make_unique<FABRIC_CHECK_SEQUENCE_PROPERTY_OPERATION>();
            op->PropertyName = propName.c_str();
            op->SequenceNumber = propSequence;
            op->Reserved = NULL;
            nativeOp.Value = reinterpret_cast<void*>(op.get());
            nativeCheckSequenceOps.push_back(move(op));
        }
        else if (splitItems[0] == L"checkexists")
        {
            if (splitItems.size() != 3)
            {
                TestSession::WriteError(
                    TraceSource,
                    "Could not parse CheckExistsProperty {0}.", *it);
                return false;
            }

            bool exists = (splitItems[2] == L"true");
            TestSession::WriteNoise(
                TraceSource,
                "CheckExistsProperty {0}={1}.", propName, exists);
            nativeOp.Kind = FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS;
            auto op = make_unique<FABRIC_CHECK_EXISTS_PROPERTY_OPERATION>();
            op->PropertyName = propName.c_str();
            op->ExistenceCheck = exists;
            op->Reserved = NULL;
            nativeOp.Value = reinterpret_cast<void*>(op.get());
            nativeCheckExistsOps.push_back(move(op));
        }
        else if (splitItems[0] == L"checkvalue")
        {
            if (splitItems.size() != 3)
            {
                TestSession::WriteError(TraceSource, "Could not parse CheckValueProperty {0}.", *it);
                return false;
            }

            wstring const & propValue = splitItems[2];
            vector<BYTE> bytes = Serialize(propValue);

            TestSession::WriteNoise(
                TraceSource,
                "Operation {0}: CheckValue {1} with value {2}.", *it, propName, propValue);

            auto dataBuffer = make_unique<FABRIC_OPERATION_DATA_BUFFER>();
            dataBuffer->BufferSize = static_cast<ULONG>(bytes.size());
            checkValueBytes.push_back(move(bytes));
            dataBuffer->Buffer = &(checkValueBytes.back()[0]);
            nativePutOpBuffer.push_back(move(dataBuffer));

            nativeOp.Kind = FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_VALUE;
            auto checkValueOp = make_unique<FABRIC_CHECK_VALUE_PROPERTY_OPERATION>();
            checkValueOp->PropertyName = propName.c_str();
            checkValueOp->PropertyTypeId = FABRIC_PROPERTY_TYPE_BINARY;
            checkValueOp->PropertyValue = reinterpret_cast<void*>(nativePutOpBuffer.back().get());
            checkValueOp->Reserved = NULL;

            nativeOp.Value = reinterpret_cast<void*>(checkValueOp.get());
            nativeCheckValueOps.push_back(move(checkValueOp));
        }
        else
        {
            TestSession::WriteError(
                TraceSource,
                "Unknown operation {0}.", *it);
            return false;
        }

        operations.push_back(nativeOp);
        ++index;
    }

    ComPointer<IFabricPropertyManagementClient> propertyClient;
    if (!FABRICSESSION.FabricDispatcher.TryGetNamedFabricClient(clientName, IID_IFabricPropertyManagementClient, propertyClient))
    {
        propertyClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);
    }

    ULONG failedOperationIndex;
    ComPointer<IFabricPropertyBatchResult> result;

    HRESULT hr = this->PerformFabricClientOperation(
        L"SubmitPropertyBatch",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return propertyClient->BeginSubmitPropertyBatch(
                name.Name.c_str(),
                static_cast<ULONG>(operations.size()),
                operations.data(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return propertyClient->EndSubmitPropertyBatch(context.GetRawPointer(), &failedOperationIndex, result.InitializationAddress());
        },
        expectedError);

    if (SUCCEEDED(hr))
    {
        TestSession::WriteInfo(TraceSource, "SubmitPropertyBatch is successful: failed index = {0}, expected {1}", failedOperationIndex, expectedFailedIndex);
        if (expectedFailedIndex != static_cast<ULONG>(-1))
        {
            TestSession::FailTestIfNot(failedOperationIndex == expectedFailedIndex, "Failed operation index mismatch. '{0}' != '{1}'", expectedFailedIndex, failedOperationIndex);
        }

        FABRICSESSION.FabricDispatcher.NamingState.AddProperties(name, putOperations);

        for (auto getOp : expectedGetProperties)
        {
            vector<wstring> propItems;
            StringUtility::Split<wstring>(getOp, propItems, L":");

            if (propItems.size() < 2 && propItems.size() > 4)
            {
                TestSession::WriteError(
                    TraceSource,
                    "Could not parse expected get result.", getOp);
                return false;
            }

            ULONG index1;
            TestSession::FailTestIfNot(
                StringUtility::TryFromWString(propItems[0], index1), "Could not parse index for {0} ({1})", propItems[0], getOp);

            ComPointer<IFabricPropertyValueResult> propValueResult;
            HRESULT hrGet = result->GetProperty(
                index1,
                propValueResult.InitializationAddress());

            if (propItems.size() == 2)
            {
                // Expected error
                ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(propItems[1]);
                HRESULT errorHResult = ErrorCode(error).ToHResult();
                if (errorHResult != hrGet)
                {
                    TestSession::WriteError(
                        TraceSource,
                        "Batch result: index {0}: expected failure {1} ({2}), instead got {3}.", index1, errorHResult, propItems[1], hrGet);
                    return false;
                }
            }
            else
            {
                TestSession::FailTestIfNot(SUCCEEDED(hrGet), "Get property at index {0} failed with {1}", index1, hrGet);
                wstring expectedPropertyName = propItems[1];
                wstring expectedValue = propItems[2];
                wstring expectedCustomTypeId;
                if (propItems.size() == 4)
                {
                    expectedCustomTypeId = propItems[3];
                }

                int64 expectedSize;
                if (Common::TryParseInt64(expectedValue, expectedSize))
                {
                    CheckMetadata(
                        propValueResult->get_Property()->Metadata,
                        expectedPropertyName,
                        expectedCustomTypeId,
                        static_cast<ULONG>(expectedSize));
                }
                else
                {
                    CheckProperty(
                        propValueResult,
                        expectedPropertyName,
                        expectedValue,
                        expectedCustomTypeId);
                }
            }
        }
    }

    return true;
}

void TestFabricClient::DeleteProperty(NamingUri const& name, std::wstring const& propertyName, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "DeleteProperty called for name {0}, expected error {1}, propertyName {2}", name, expectedError, propertyName);

    auto propertyClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);

    HRESULT hr = this->PerformFabricClientOperation(
        L"DeleteProperty",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return propertyClient->BeginDeleteProperty(
                name.Name.c_str(),
                propertyName.c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return propertyClient->EndDeleteProperty(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_PROPERTY_DOES_NOT_EXIST);

    if (SUCCEEDED(hr))
    {
        FABRICSESSION.FabricDispatcher.NamingState.RemoveProperty(name, propertyName);
    }
}

void TestFabricClient::GetProperty(
    NamingUri const& name,
    wstring const& propertyName,
    wstring const& expectedValue,
    wstring const& expectedCustomTypeId,
    wstring const& seqVarName,
    HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "GetProperty called for name {0}, expected error {1}, propertyName {2}, expectedValue {3}", name, expectedError, propertyName, expectedValue);
    TestSession::FailTestIf(IsRetryableError(expectedError), "Expected error for GetProperty cannot be retryable");

    ComPointer<IFabricPropertyValueResult> propertyResult;
    auto propertyClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);

    HRESULT hr = this->PerformFabricClientOperation(
        L"GetProperty",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return propertyClient->BeginGetProperty(
                name.Name.c_str(),
                propertyName.c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return propertyClient->EndGetProperty(context.GetRawPointer(), propertyResult.InitializationAddress());
        },
        expectedError);

    if (SUCCEEDED(hr))
    {
        if (!seqVarName.empty())
        {
            SaveSequenceNumber(seqVarName, propertyResult->get_Property()->Metadata->SequenceNumber);
        }

        CheckProperty(
            propertyResult,
            propertyName,
            expectedValue,
            expectedCustomTypeId);
    }
}

bool TestFabricClient::DnsNameExists(Common::StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DnsNameExistsCommand);
        return false;
    }

    CommandLineParser parser(params, 1);

    wstring error;
    parser.TryGetString(L"error", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    GetDnsNamePropertyValue(/*propertyName*/ params[0], /*expectedValue*/ params[1], expectedError);

    return true;
}

void TestFabricClient::GetDnsNamePropertyValue(
    wstring const& propertyName,
    wstring const& expectedValue,
    HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "GetDnsNameProperty called for expected propertyName {0}, expectedValue {1}, error {2},  ", propertyName, expectedValue, expectedError);
    TestSession::FailTestIf(IsRetryableError(expectedError), "Expected error for GetProperty cannot be retryable");

    ComPointer<IFabricPropertyValueResult> propertyResult;
    auto propertyClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);
    
    NamingUri name;
    NamingUri::TryParse(L"fabric:/System/DnsService", name);

    HRESULT hr = this->PerformFabricClientOperation(
        L"GetProperty",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return propertyClient->BeginGetProperty(
            name.Name.c_str(),
            propertyName.c_str(),
            timeout,
            callback.GetRawPointer(),
            context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return propertyClient->EndGetProperty(context.GetRawPointer(), propertyResult.InitializationAddress());
    },
        expectedError);

    if (SUCCEEDED(hr))
    {
        CheckDnsProperty(
            propertyResult,
            expectedValue);
    }
}

void TestFabricClient::CheckDnsProperty(
    Common::ComPointer<IFabricPropertyValueResult> const& propertyResult,
    std::wstring const& expectedValue)
{
    FABRIC_NAMED_PROPERTY const * fabricProperty = propertyResult->get_Property();
    
    vector<byte> bytes;
    bytes.insert(bytes.begin(), fabricProperty->Value, fabricProperty->Value + fabricProperty->Metadata->ValueSize);

    StringBody strBody(L"");
    auto error = FabricSerializer::Deserialize(strBody, bytes);

    TestSession::FailTestIfNot(
        error.IsSuccess(),
        "Failed to deserialize DNS property value: error={0}",
        error);

    std::wstring actualValue = strBody.String;

    TestSession::FailTestIfNot(
        expectedValue.size() == actualValue.size(),
        "Data size mismatch. '{0}' != '{1}'",
        expectedValue.size(),
        actualValue.size());

    bool matched = true;
    size_t matchIndex = 0;
    for (matchIndex = 0; matchIndex < expectedValue.size(); ++matchIndex)
    {
        if (expectedValue[matchIndex] != actualValue[matchIndex])
        {
            matched = false;
            break;
        }
    }

    TestSession::FailTestIfNot(matched, "Data mismatch. '{0}' != '{1}' (index={2})", expectedValue, actualValue, matchIndex);
}

void TestFabricClient::SaveSequenceNumber(std::wstring const & varName, uint64 value)
{
    propertySequenceNumberVariables_[varName] = value;

    TestSession::WriteInfo(
        TraceSource, 
        "Saved sequence number variable: var='{0}' seq='{1}'",
        varName,
        value);
}

bool TestFabricClient::TryLoadSequenceNumber(std::wstring const & varName, __out uint64 & value)
{
    auto findIt = propertySequenceNumberVariables_.find(varName);
    if (findIt == propertySequenceNumberVariables_.end())
    {
        return false;
    }
    else
    {
        value = findIt->second;

        return true;
    }
}

void TestFabricClient::SavePartitionId(std::wstring const & serviceName, std::wstring const & partitionName, Common::Guid const & partitionId)
{
    auto key = wformatString("{0}+{1}", serviceName, partitionName);
    namedPartitionIdMap_[key] = partitionId;

    TestSession::WriteInfo(TraceSource, "Saved partition: service={0} partition={1} cuid={2}", serviceName, partitionName, partitionId);
}

void TestFabricClient::VerifySavedPartitiondId(std::wstring const & serviceName, std::wstring const & partitionName, Common::Guid const & toVerify)
{
    auto key = wformatString("{0}+{1}", serviceName, partitionName);
    auto it = namedPartitionIdMap_.find(key);
    
    TestSession::FailTestIf(
        it == namedPartitionIdMap_.end(), 
        "Previous PartitionId not saved for service={0} partition={1}",
        serviceName,
        partitionName);

    auto partitionId = it->second;

    TestSession::FailTestIf(
        partitionId != toVerify,
        "Saved PartitionId does not match: service={0} partition={1} saved={2} expected={3}",
        serviceName,
        partitionName,
        partitionId,
        toVerify);

    TestSession::WriteInfo(TraceSource, "Verified partition: service={0} partition={1} cuid={2}", serviceName, partitionName, partitionId);
}

void TestFabricClient::CheckMetadata(
    FABRIC_NAMED_PROPERTY_METADATA const * metadata,
    wstring const& propertyName,
    wstring const& expectedCustomTypeId,
    ULONG expectedValueSize)
{
    auto propertyNameResult = std::wstring(metadata->PropertyName);

    TestSession::FailTestIfNot(
        propertyName == propertyNameResult,
        "Requested property '{0}'. Got property '{1}'.",
        propertyName,
        propertyNameResult);

    if (expectedValueSize != static_cast<ULONG>(-1))
    {
        ULONG valueSize = metadata->ValueSize;
        TestSession::FailTestIfNot(
            valueSize == expectedValueSize,
            "Data size mismatch. '{0}' != '{1}'",
            valueSize,
            expectedValueSize);
    }

    wstring customTypeId;
    if (metadata->Reserved != NULL)
    {
        auto extended = reinterpret_cast<FABRIC_NAMED_PROPERTY_METADATA_EX1*>(metadata->Reserved);
        customTypeId = wstring(extended->CustomTypeId);
    }

    TestSession::FailTestIfNot(expectedCustomTypeId == customTypeId, "Custom type id mismatch. '{0}' != '{1}'", expectedCustomTypeId, customTypeId);
}

void TestFabricClient::CheckProperty(
    ComPointer<IFabricPropertyValueResult> const& propertyResult,
    wstring const& propertyName,
    wstring const& expectedValue,
    wstring const& expectedCustomTypeId)
{
    CheckMetadata(propertyResult->get_Property()->Metadata, propertyName, expectedCustomTypeId);

    FABRIC_NAMED_PROPERTY const * namedProperty = propertyResult->get_Property();

    auto expectedData = Serialize(expectedValue);
    auto actualSize = namedProperty->Metadata->ValueSize;

    TestSession::FailTestIfNot(
        expectedData.size() == actualSize,
        "Data size mismatch. '{0}' != '{1}'",
        expectedData.size(),
        actualSize);

    bool matched = true;
    size_t matchIndex = 0;
    for (matchIndex=0; matchIndex<expectedData.size(); ++matchIndex)
    {
        if (expectedData[matchIndex] != namedProperty->Value[matchIndex])
        {
            matched = false;
            break;
        }
    }

    auto actualValue = Deserialize(namedProperty->Value, namedProperty->Metadata->ValueSize);
    TestSession::FailTestIfNot(matched, "Data mismatch. '{0}' != '{1}' (index={2})", expectedValue, actualValue, matchIndex);
}

void TestFabricClient::GetMetadata(
    NamingUri const& name,
    std::wstring const& propertyName,
    HRESULT expectedError,
    __out ComPointer<IFabricPropertyMetadataResult> & propertyResult)
{
    TestSession::WriteNoise(TraceSource, "GetMetadata called for name {0}, expected error {1}, propertyName {2}", name, expectedError, propertyName);
    auto propertyClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"GetMetadata",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return propertyClient->BeginGetPropertyMetadata(
                name.Name.c_str(),
                propertyName.c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return propertyClient->EndGetPropertyMetadata(context.GetRawPointer(), propertyResult.InitializationAddress());
        },
        expectedError);
}

void TestFabricClient::CreateServiceFromTemplate(NamingUri const& serviceName, wstring const& type, NamingUri const& applicationName, HRESULT expectedError, PartitionedServiceDescriptor & psd)
{
    TestSession::WriteNoise(TraceSource, "CreateServiceFromTemplate called with expected error {0}", expectedError);

    ComPointer<IFabricServiceManagementClient> serviceClient = CreateFabricServiceClient(FABRICSESSION.FabricDispatcher.Federation);

    HRESULT hr = this->PerformFabricClientOperation(
        L"CreateServiceFromTemplate",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceClient->BeginCreateServiceFromTemplate(
                applicationName.Name.c_str(),
                serviceName.Name.c_str(),
                type.c_str(),
                0,
                NULL,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceClient->EndCreateServiceFromTemplate(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_SERVICE_ALREADY_EXISTS);

    if(SUCCEEDED(hr))
    {
        vector<wstring> defaultServices;
        defaultServices.push_back(serviceName.Name);
        vector<PartitionedServiceDescriptor> serviceDescriptors;
        GetPartitionedServiceDescriptors(defaultServices, serviceDescriptors);
        psd = serviceDescriptors[0];
    }
}

void TestFabricClient::CreateService(
    FABRIC_SERVICE_DESCRIPTION const& serviceDescription,
    vector<HRESULT> const& expectedErrors,
    ComPointer<IFabricServiceManagementClient> const & client)
{
    TestSession::WriteNoise(TraceSource, "CreateService called");

    ComPointer<IFabricServiceManagementClient> serviceClient = client;
    if (!serviceClient)
    {
        serviceClient = CreateFabricServiceClient(FABRICSESSION.FabricDispatcher.Federation);
    }

    vector<HRESULT> allowedErrorsOnRetry;
    allowedErrorsOnRetry.push_back(FABRIC_E_SERVICE_ALREADY_EXISTS);

    vector<HRESULT> retryableErrors;
    retryableErrors.push_back(S_OK);
    //In some rare cases service creation can drop in time when plb didn't reach to process all pending Node updates which leads to test failures
    //This change will not affect tests which expect this error to occur because the underlying function first check expected errors and if it is expected error it won't retry
    retryableErrors.push_back(FABRIC_E_INSUFFICIENT_CLUSTER_CAPACITY);

    this->PerformFabricClientOperation(
        L"CreateService",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            HRESULT innerHr = serviceClient->BeginCreateService(
                &serviceDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());

            return innerHr;
        },
            [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceClient->EndCreateService(context.GetRawPointer());
        },
        expectedErrors,
        allowedErrorsOnRetry,
        retryableErrors);
}

void TestFabricClient::UpdateService(
    wstring const& serviceName,
    FABRIC_SERVICE_UPDATE_DESCRIPTION const& serviceUpdateDescription,
    vector<HRESULT> const& expectedErrors,
    ComPointer<IFabricServiceManagementClient2> const & client)
{
    TestSession::WriteNoise(TraceSource, "UpdateService called");

    ComPointer<IFabricServiceManagementClient2> serviceClient = client ? client : CreateFabricServiceClient2(FABRICSESSION.FabricDispatcher.Federation);

    vector<HRESULT> allowedErrorsOnRetry;
    vector<HRESULT> retryableErrors;

    this->PerformFabricClientOperation(
        L"UpdateService",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            HRESULT innerHr = serviceClient->BeginUpdateService(
                serviceName.c_str(),
                &serviceUpdateDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());

            return innerHr;
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceClient->EndUpdateService(context.GetRawPointer());
        },
        expectedErrors,
        allowedErrorsOnRetry,
        retryableErrors);
}

void TestFabricClient::CreateServiceGroup(FABRIC_SERVICE_GROUP_DESCRIPTION const& serviceGroupDescription, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "CreateServiceGroup called with expected error {0}", expectedError);

    ComPointer<IFabricServiceGroupManagementClient> serviceClient = CreateFabricServiceGroupClient(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"CreateServiceGroup",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            HRESULT innerHr = serviceClient->BeginCreateServiceGroup(
                &serviceGroupDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());

            return innerHr;
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceClient->EndCreateServiceGroup(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_SERVICE_ALREADY_EXISTS);
}

void TestFabricClient::UpdateServiceGroup(
    wstring const& serviceGroupName,
    FABRIC_SERVICE_GROUP_UPDATE_DESCRIPTION const& serviceGroupUpdateDescription,
    vector<HRESULT> const& expectedErrors)
{
    TestSession::WriteNoise(TraceSource, "UpdateServiceGroup called");

    ComPointer<IFabricServiceGroupManagementClient2> serviceGroupClient = CreateFabricServiceGroupClient2(FABRICSESSION.FabricDispatcher.Federation);

    vector<HRESULT> allowedErrorsOnRetry;
    vector<HRESULT> retryableErrors;

    this->PerformFabricClientOperation(
        L"UpdateServiceGroup",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            HRESULT innerHr = serviceGroupClient->BeginUpdateServiceGroup(
                serviceGroupName.c_str(),
                &serviceGroupUpdateDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());

            return innerHr;
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceGroupClient->EndUpdateServiceGroup(context.GetRawPointer());
        },
        expectedErrors,
        allowedErrorsOnRetry,
        retryableErrors);
}

void TestFabricClient::GetPartitionedServiceDescriptor(
    wstring const& serviceName,
    __out PartitionedServiceDescriptor & psd)
{
    this->GetPartitionedServiceDescriptor(serviceName, FabricTestSessionConfig::GetConfig().NamingOperationTimeout, psd);
}

void TestFabricClient::GetPartitionedServiceDescriptor(
    wstring const& serviceName,
    TimeSpan const timeout,
    __out PartitionedServiceDescriptor & psd)
{
    this->GetPartitionedServiceDescriptor(serviceName, L"", timeout, psd);
}

void TestFabricClient::GetPartitionedServiceDescriptor(
    wstring const& serviceName,
    ComPointer<IFabricServiceManagementClient> const & serviceClient, 
    __out PartitionedServiceDescriptor & psd)
{
    ComPointer<IFabricTestClient> testClient;
    HRESULT hr = serviceClient->QueryInterface(IID_IFabricTestClient, testClient.VoidInitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "QI(IID_IFabricTestClient) failed with {0}", hr);

    ComPointer<IFabricStringResult> stringResult;
    hr = testClient->GetCurrentGatewayAddress(stringResult.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "GetCurrentGatewayAddress() failed with {0}", hr);

    LPCWSTR stringPtr = stringResult->get_String();
    TestSession::FailTestIf(stringPtr == NULL, "Current gateway address is NULL");

    this->GetPartitionedServiceDescriptor(
        serviceName, 
        wstring(stringPtr), 
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        psd);
}

void TestFabricClient::GetPartitionedServiceDescriptor(
    wstring const& serviceName,
    wstring const& gatewayAddress,
    TimeSpan const operationTimeout,
    __out PartitionedServiceDescriptor & psd)
{
    vector<wstring> serviceNames;
    vector<PartitionedServiceDescriptor> psds;

    serviceNames.push_back(serviceName);

    GetPartitionedServiceDescriptors(serviceNames, gatewayAddress, operationTimeout, psds);
    TestSession::FailTestIfNot(psds.size() == 1, "psds should have size 1");
    psd = psds[0];
}

bool TestFabricClient::DeleteServiceGroup(wstring const& serviceName, vector<HRESULT> const& expectedErrors)
{
    TestSession::WriteNoise(TraceSource, "DeleteServiceGroup called for name {0}", serviceName);

    ComPointer<IFabricServiceGroupManagementClient> serviceGroupClient = CreateFabricServiceGroupClient(FABRICSESSION.FabricDispatcher.Federation);

    vector<HRESULT> allowedErrorsOnRetry;
    allowedErrorsOnRetry.push_back(FABRIC_E_SERVICE_DOES_NOT_EXIST);

    vector<HRESULT> retryableErrors;
    retryableErrors.push_back(S_OK);

    this->PerformFabricClientOperation(
        L"DeleteServiceGroup",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceGroupClient->BeginDeleteServiceGroup(
                serviceName.c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceGroupClient->EndDeleteServiceGroup(context.GetRawPointer());
        },
        expectedErrors,
        allowedErrorsOnRetry,
        retryableErrors);

    return true;
}

bool TestFabricClient::DeleteService(wstring const& serviceName, bool const isForce, vector<HRESULT> const& expectedErrors, Common::ComPointer<IFabricServiceManagementClient5> const & client)
{
    TestSession::WriteNoise(TraceSource, "DeleteService called for name {0}", serviceName);

    ComPointer<IFabricServiceManagementClient5> serviceClient = client;
    if (!serviceClient)
    {
        serviceClient = CreateFabricServiceClient5(FABRICSESSION.FabricDispatcher.Federation);
    }

    vector<HRESULT> allowedErrorsOnRetry;
    allowedErrorsOnRetry.push_back(FABRIC_E_SERVICE_DOES_NOT_EXIST);

    vector<HRESULT> retryableErrors;
    retryableErrors.push_back(S_OK);

    this->PerformFabricClientOperation(
        L"DeleteService",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            FABRIC_DELETE_SERVICE_DESCRIPTION description;
            description.ServiceName = serviceName.c_str();
            description.ForceDelete = isForce;
            return serviceClient->BeginDeleteService2(
                &description,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
            [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceClient->EndDeleteService(context.GetRawPointer());
        },
        expectedErrors,
        allowedErrorsOnRetry,
        retryableErrors);

    return true;
}

void TestFabricClient::GetServiceDescription(
    NamingUri const& name,
    HRESULT expectedError,
    ComPointer<IFabricServiceManagementClient> const & client,
    ComPointer<IFabricServiceDescriptionResult> & result)
{
    TestSession::WriteNoise(TraceSource, "GetServiceDescription called for name {0} with expected error = {1}", name, expectedError);

    ComPointer<IFabricServiceManagementClient> serviceClient = client ? client : CreateFabricServiceClient(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"GetServiceDescription",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceClient->BeginGetServiceDescription(
                name.ToString().c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceClient->EndGetServiceDescription(context.GetRawPointer(), result.InitializationAddress());
        },
        expectedError);
}

void TestFabricClient::GetServiceGroupDescription(NamingUri const& name, HRESULT expectedError, ComPointer<IFabricServiceGroupDescriptionResult> & result)
{
    TestSession::WriteNoise(TraceSource, "GetServiceGroupDescription called for name {0} with expected error = {1}", name, expectedError);

    ComPointer<IFabricServiceGroupManagementClient> serviceClient = CreateFabricServiceGroupClient(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"GetServiceGroupDescription",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceClient->BeginGetServiceGroupDescription(
                name.ToString().c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return serviceClient->EndGetServiceGroupDescription(context.GetRawPointer(), result.InitializationAddress());
        },
        expectedError);
}

void TestFabricClient::DeactivateNode(wstring const& nodeName, FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "DeactivateNode called for nodeId '{0}', expected error {1}.", nodeName, expectedError);

    ComPointer<IFabricClusterManagementClient6> clusterClient = CreateFabricClusterClient(FABRICSESSION.FabricDispatcher.Federation);

    PerformFabricClientOperation(
        L"DeactivateNode",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return clusterClient->BeginDeactivateNode(
                nodeName.c_str(),
                deactivationIntent,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return clusterClient->EndDeactivateNode(context.GetRawPointer());
        },
        expectedError,
        S_OK,
        E_FAIL);
}

void TestFabricClient::ActivateNode(wstring const& nodeName)
{
    TestSession::WriteNoise(TraceSource, "ActivateNode called for nodeId '{0}'.", nodeName);

    ComPointer<IFabricClusterManagementClient6> clusterClient = CreateFabricClusterClient(FABRICSESSION.FabricDispatcher.Federation);

    PerformFabricClientOperation(
        L"ActivateNode",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return clusterClient->BeginActivateNode(
                nodeName.c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return clusterClient->EndActivateNode(context.GetRawPointer());
        },
        S_OK,
        S_OK,
        E_FAIL);
}

void TestFabricClient::NodeStateRemoved(wstring const& nodeHostName, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "NodeStateRemoved called for name '{0}', expected error {1}.", nodeHostName, expectedError);

    auto clusterClient = CreateFabricClusterClient(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"NodeStateRemoved",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            CALL_VERSIONED_API(
                clusterClient,
                BeginNodeStateRemoved,
                IFabricClusterManagementClient,
                nodeHostName.c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            CALL_VERSIONED_API(
                clusterClient,
                EndNodeStateRemoved,
                IFabricClusterManagementClient,
                context.GetRawPointer());
        },
        expectedError,
        S_OK,
        E_FAIL);
}

void TestFabricClient::RecoverPartitions(HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "RecoverPartitions called, expected error {0}.", expectedError);

    auto clusterClient = CreateFabricClusterClient(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"RecoverPartitions",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        CALL_VERSIONED_API(
            clusterClient,
            BeginRecoverPartitions,
            IFabricClusterManagementClient,
            timeout,
            callback.GetRawPointer(),
            context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        CALL_VERSIONED_API(
            clusterClient,
            EndRecoverPartitions,
            IFabricClusterManagementClient,
            context.GetRawPointer());
    },
        expectedError,
        S_OK,
        E_FAIL);
}

void TestFabricClient::ReportFault(std::wstring const & nodeName, FABRIC_PARTITION_ID partitionId, FABRIC_REPLICA_ID replicaId, FABRIC_FAULT_TYPE faultType, bool isForce, HRESULT expectedError)
{
    Guid forTrace(partitionId);
    TestSession::WriteNoise(TraceSource, "ReportFault called. Node = {0}. Partition = {1}. Replica = {2}. FaultType = {3}", nodeName, forTrace, replicaId, faultType);

    auto serviceManagementClient = CreateFabricServiceClient3(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"ReportFault",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        if (faultType == FABRIC_FAULT_TYPE_TRANSIENT)
        {
            FABRIC_RESTART_REPLICA_DESCRIPTION desc = { 0 };
            desc.NodeName = nodeName.c_str();
            desc.PartitionId = partitionId;
            desc.ReplicaOrInstanceId = replicaId;

            return serviceManagementClient->BeginRestartReplica(
                &desc,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        }
        else
        {
            FABRIC_REMOVE_REPLICA_DESCRIPTION desc = { 0 };
            desc.NodeName = nodeName.c_str();
            desc.PartitionId = partitionId;
            desc.ReplicaOrInstanceId = replicaId;

            FABRIC_REMOVE_REPLICA_DESCRIPTION_EX1 desc1 = { 0 };
            desc1.ForceRemove = isForce;

            desc.Reserved = &desc1;

            return serviceManagementClient->BeginRemoveReplica(
                &desc,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        }
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        if (faultType == FABRIC_FAULT_TYPE_TRANSIENT)
        {
            return serviceManagementClient->EndRestartReplica(context.GetRawPointer());
        }
        else
        {
            return serviceManagementClient->EndRemoveReplica(context.GetRawPointer());
        }
    },
        expectedError);
}

void TestFabricClient::RecoverPartition(FABRIC_PARTITION_ID partitionId, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "RecoverPartition called, expected error {0}.", expectedError);

    auto clusterClient = CreateFabricClusterClient(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"RecoverPartition",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return clusterClient->BeginRecoverPartition(
                partitionId,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return clusterClient->EndRecoverPartition(context.GetRawPointer());
        },
        expectedError,
        S_OK,
        E_FAIL);
}

void TestFabricClient::RecoverServicePartitions(wstring const& serviceName, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "RecoverServicePartitions called, expected error {0}.", expectedError);

    auto clusterClient = CreateFabricClusterClient(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"RecoverServicePartitions",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return clusterClient->BeginRecoverServicePartitions(
                serviceName.c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return clusterClient->EndRecoverServicePartitions(context.GetRawPointer());
        },
        expectedError,
        S_OK,
        E_FAIL);
}

void TestFabricClient::RecoverSystemPartitions(HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "RecoverSystemPartitions called, expected error {0}.", expectedError);

    auto clusterClient = CreateFabricClusterClient(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"RecoverSystemPartitions",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return clusterClient->BeginRecoverSystemPartitions(
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return clusterClient->EndRecoverSystemPartitions(context.GetRawPointer());
        },
        expectedError,
        S_OK,
        E_FAIL);
}

void TestFabricClient::ResetPartitionLoad(FABRIC_PARTITION_ID partitionId, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "ResetPartitionLoad called, expected error {0}.", expectedError);

    auto clusterClient = CreateFabricClusterClient(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"ResetPartitionLoad",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return clusterClient->BeginResetPartitionLoad(
            partitionId,
            timeout,
            callback.GetRawPointer(),
            context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return clusterClient->EndResetPartitionLoad(context.GetRawPointer());
    },
        expectedError,
        S_OK,
        E_FAIL);
}

void TestFabricClient::ToggleVerboseServicePlacementHealthReporting(bool enabled, HRESULT expectedError)
{
    TestSession::WriteNoise(TraceSource, "ToggleVerboseServicePlacementHealthReporting called, expected error {0}.", expectedError);

    auto clusterClient = CreateFabricClusterClient(FABRICSESSION.FabricDispatcher.Federation);

    this->PerformFabricClientOperation(
        L"ToggleVerboseServicePlacementHealthReporting",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return clusterClient->BeginToggleVerboseServicePlacementHealthReporting(
            enabled,
            timeout,
            callback.GetRawPointer(),
            context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return clusterClient->EndToggleVerboseServicePlacementHealthReporting(context.GetRawPointer());
    },
        expectedError,
        S_OK,
        E_FAIL);
}

bool TestFabricClient::RegisterServiceNotificationFilter(Common::StringCollection const & params)
{
    auto & fabricDispatcher = FABRICSESSION.FabricDispatcher;

    CommandLineParser parser(params, 0);

    wstring clientName;
    parser.TryGetString(L"clientname", clientName, L"ServiceNotificationClient");

    wstring uri;
    parser.TryGetString(L"uri", uri, L"");

    bool prefix = parser.GetBool(L"prefix");

    bool primaryOnly = parser.GetBool(L"primary");

    FABRIC_SERVICE_NOTIFICATION_FILTER_DESCRIPTION description = {0};
    description.Name = uri.c_str();

    ULONG flags = 0;
    flags |= (prefix ? FABRIC_SERVICE_NOTIFICATION_FILTER_FLAGS_NAME_PREFIX : 0);
    flags |= (primaryOnly ? FABRIC_SERVICE_NOTIFICATION_FILTER_FLAGS_PRIMARY_ONLY : 0);

    description.Flags = static_cast<FABRIC_SERVICE_NOTIFICATION_FILTER_FLAGS>(flags);

    ComPointer<IFabricServiceManagementClient4> client;
    bool success = fabricDispatcher.TryGetNamedFabricClient(clientName, IID_IFabricServiceManagementClient4, client);
    TestSession::FailTestIfNot(success, "Client {0} does not exist", clientName);

    LONGLONG filterId = 0;

    this->PerformFabricClientOperation(
        L"RegisterServiceNotificationFilter",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return client->BeginRegisterServiceNotificationFilter(
                &description,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return client->EndRegisterServiceNotificationFilter(context.GetRawPointer(), &filterId);
        },
        S_OK);

    TestSession::WriteInfo(
        TraceSource,
        "Registered filter at {0}: id={1}",
        uri,
        filterId);

    return true;
}

bool TestFabricClient::UnregisterServiceNotificationFilter(Common::StringCollection const & params)
{
    auto & fabricDispatcher = FABRICSESSION.FabricDispatcher;

    CommandLineParser parser(params, 0);

    wstring clientName;
    parser.TryGetString(L"clientname", clientName, L"ServiceNotificationClient");

    int64 filterId;
    parser.TryGetInt64(L"filterid", filterId);

    ComPointer<IFabricServiceManagementClient4> client;
    bool success = fabricDispatcher.TryGetNamedFabricClient(clientName, IID_IFabricServiceManagementClient4, client);
    TestSession::FailTestIfNot(success, "Client {0} does not exist", clientName);

    this->PerformFabricClientOperation(
        L"UnregisterServiceNotificationFilter",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return client->BeginUnregisterServiceNotificationFilter(
                filterId,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return client->EndUnregisterServiceNotificationFilter(context.GetRawPointer());
        },
        S_OK);

    TestSession::WriteInfo(
        TraceSource,
        "Unregistered filter id={0}",
        filterId);

    return true;
}

ITestStoreServiceSPtr TestFabricClient::ResolveService(
    wstring const& serviceName,
    HRESULT expectedError,
    ServiceLocationChangeClientSPtr const & client,
    wstring const & cacheItemName,
    bool skipVerifyLocation,
    HRESULT expectedCompareVersionError)
{
    return ResolveService(
        serviceName,
        FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE,
        NULL,
        Guid::Empty(),
        expectedError,
        true /*isCommand*/,
        client,
        cacheItemName,
        false /*allowAllError*/,
        skipVerifyLocation,
        expectedCompareVersionError);
}

ITestStoreServiceSPtr TestFabricClient::ResolveService(
    wstring const& serviceName,
    __int64 key,
    HRESULT expectedError,
    ServiceLocationChangeClientSPtr const & client,
    wstring const & cacheItemName,
    bool skipVerifyLocation,
    HRESULT expectedCompareVersionError)
{
    return ResolveService(
        serviceName,
        FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64,
        reinterpret_cast<void const *>(&key),
        Guid::Empty(),
        expectedError,
        true /*isCommand*/,
        client,
        cacheItemName,
        false /*allowAllError*/,
        skipVerifyLocation,
        expectedCompareVersionError);
}

ITestStoreServiceSPtr TestFabricClient::ResolveService(
    wstring const& serviceName,
    wstring const& key,
    HRESULT expectedError,
    ServiceLocationChangeClientSPtr const & client,
    wstring const & cacheItemName,
    bool skipVerifyLocation,
    bool savePartitionId,
    bool verifyPartitionId,
    HRESULT expectedCompareVersionError)
{
    return ResolveService(
        serviceName,
        FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING,
        reinterpret_cast<void const *>(key.c_str()),
        Guid::Empty(),
        expectedError,
        true /*isCommand*/,
        client,
        cacheItemName,
        false /*allowAllError*/,
        skipVerifyLocation,
        expectedCompareVersionError,
        savePartitionId,
        verifyPartitionId);
}

wstring TestFabricClient::GetKeyValue(FABRIC_PARTITION_KEY_TYPE keyType, void const * key)
{
    switch (keyType)
    {
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE:
            return L"N/A";

        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64:
            return wformatString("{0}", *reinterpret_cast<__int64 const *>(key));

        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING:
            return wstring(reinterpret_cast<wchar_t const *>(key));
    }

    return L"Invalid";
}

ITestStoreServiceSPtr TestFabricClient::ResolveService(
    std::wstring const& serviceName,
    FABRIC_PARTITION_KEY_TYPE keyType,
    void const * key,
    Guid const& fuId,
    HRESULT expectedError,
    bool isCommand,
    ServiceLocationChangeClientSPtr const & client,
    wstring const & cacheItemName,
    bool allowAllError,
    bool skipVerifyLocation,
    HRESULT expectedCompareVersionError,
    bool savePartitionId,
    bool verifyPartitionId)
{
    TestSession::WriteNoise(TraceSource, "ResolveService called for service name {0} and failoverUnit {1}", serviceName, fuId);
    int retryCount = 0;
    TimeoutHelper helper(FabricTestSessionConfig::GetConfig().NamingResolveRetryTimeout);
    ITestStoreServiceSPtr storeService;
    bool doRetry = false;
    bool verifyResult = false;
    HRESULT beginResolveHr = E_FAIL;
    HRESULT endResolveHr = E_FAIL;
    bool storeServiceFound = false;

    wstring keyValue = GetKeyValue(keyType, key);

    ComPointer<IFabricServiceManagementClient> serviceClient =
        client ? client->Client : CreateFabricServiceClient(FABRICSESSION.FabricDispatcher.Federation);
    ComPointer<IFabricResolvedServicePartitionResult> previous;

    do
    {
        if (client && cacheItemName != L"")
        {
            ServiceLocationChangeCacheInfoSPtr cacheInfo;
            if (!client->GetCacheInfo(cacheItemName, cacheInfo))
            {
                TestSession::WriteNoise(TraceSource, "ResolveService: Cache with name: {0} does not exist. Resolve without refresh", cacheItemName);
            }
            else
            {
                TestSession::WriteNoise(TraceSource, "ResolveService: Using previous entry in cache {0}", cacheItemName);
                previous = cacheInfo->CacheItem;
            }
        }

        auto resetEvent = make_shared<ManualResetEvent>(false);
        auto callback = make_com<ComAsyncOperationCallbackTestHelper>([resetEvent](IFabricAsyncOperationContext *)
        {
            resetEvent->Set();
        });

        ComPointer<IFabricAsyncOperationContext> context;
        beginResolveHr = serviceClient->BeginResolveServicePartition(
            serviceName.c_str(),
            keyType,
            key,
            previous.GetRawPointer(),
            GetComTimeout(FabricTestSessionConfig::GetConfig().NamingOperationTimeout),
            callback.GetRawPointer(),
            context.InitializationAddress());
        if (!SUCCEEDED(beginResolveHr))
        {
            TestSession::WriteInfo(TraceSource, "BeginResolveServicePartition failed: error={0} allowAllError={1}", beginResolveHr, allowAllError);

            if (allowAllError)
            {
                break;
            }
            else
            {
                TestSession::FailTest("BeginResolveServicePartition failed. hresult = {0}", beginResolveHr);
            }
        }

        TimeSpan timeToWait = FabricTestSessionConfig::GetConfig().NamingOperationTimeout + FabricTestSessionConfig::GetConfig().NamingOperationTimeoutBuffer;
        resetEvent->WaitOne(); // infinite

        ComPointer<IFabricResolvedServicePartitionResult> resolvedServiceResult;
        endResolveHr = serviceClient->EndResolveServicePartition(context.GetRawPointer(), resolvedServiceResult.InitializationAddress());

        if (SUCCEEDED(beginResolveHr) && SUCCEEDED(endResolveHr))
        {
            ResolvedServicePartition resolvedServiceLocations;
            HRESULT hr = TryConvertComResult(resolvedServiceResult, resolvedServiceLocations);
            TestSession::FailTestIfNot(SUCCEEDED(hr), "Converting COM service results failed. hresult = {0}", hr);

            if (isCommand)
            {
                TestSession::WriteInfo(
                    TraceSource,
                    "Resolve name for service {0} returned {1} result(s).", serviceName, resolvedServiceLocations.GetEndpointCount());

                if (savePartitionId)
                {
                    this->SavePartitionId(serviceName, keyValue, resolvedServiceLocations.Locations.ConsistencyUnitId.Guid);
                }

                if (verifyPartitionId)
                {
                    this->VerifySavedPartitiondId(serviceName, keyValue, resolvedServiceLocations.Locations.ConsistencyUnitId.Guid);
                }

            }
            else if (resolvedServiceLocations.Locations.ConsistencyUnitId.Guid != fuId)
            {
                TestSession::WriteWarning(
                    TraceSource,
                    "Partition did not match for resolve on service {0}. Expected {1}, Actual {2}",
                    serviceName,
                    fuId.ToString(),
                    resolvedServiceLocations.Locations.ConsistencyUnitId.Guid);
            }

            if ((expectedError != S_OK))
            {
                if (allowAllError)
                {
                    break;
                }
                else
                {
                    TestSession::WriteWarning(
                        TraceSource,
                        "EndResolveServicePartition expected {0} but completed with {1}",
                        expectedError,
                        endResolveHr);
                }
            }
            else
            {
                storeServiceFound = false;
                if (!isCommand && resolvedServiceLocations.IsStateful && resolvedServiceLocations.Locations.ServiceReplicaSet.IsPrimaryLocationValid)
                {
                    TestSession::WriteNoise(
                        TraceSource,
                        "Looking for IStoreService at location '{0}'.", resolvedServiceLocations.Locations.ServiceReplicaSet.PrimaryLocation);

                    storeServiceFound = FABRICSESSION.FabricDispatcher.Federation.TryFindStoreService(
                        resolvedServiceLocations.Locations.ServiceReplicaSet.PrimaryLocation,
                        storeService);
                }

                if (!skipVerifyLocation)
                {
                    verifyResult = FABRICSESSION.FabricDispatcher.VerifyResolveResult(resolvedServiceLocations, serviceName);
                }
                else
                {
                    TestSession::WriteNoise(
                        TraceSource,
                        "Resolve {0}: Skip verifyResult.",
                        serviceName);
                    verifyResult = true;
                }

                if (client && cacheItemName != L"")
                {
                    client->AddCacheItem(cacheItemName, serviceName, keyType, key, resolvedServiceResult, isCommand, expectedCompareVersionError);
                }
                else
                {
                    // Remember the previous result to refresh it on retry
                    previous = resolvedServiceResult;
                }
            }
        }
        else
        {
            TestSession::WriteNoise(
                TraceSource,
                "BeginResolveService for service {0} completed with {1} and EndResolveService with {2}.",
                serviceName,
                beginResolveHr,
                endResolveHr);

            if (expectedError != S_OK && !allowAllError && !IsRetryableError(endResolveHr))
            {
                if (endResolveHr != expectedError)
                {
                    if (expectedError == (HRESULT)FABRIC_E_SERVICE_DOES_NOT_EXIST &&
                        endResolveHr == (HRESULT)FABRIC_E_SERVICE_OFFLINE)
                    {
                        // Retryable error for resolve service only
                        TestSession::WriteNoise(
                            TraceSource,
                            "Retry resolve service {0} on {1}={2} (expected {3}={4}).",
                            serviceName,
                            endResolveHr,
                            ErrorCode::FromHResult(endResolveHr),
                            expectedError,
                            ErrorCode::FromHResult(expectedError));
                    }
                    else
                    {
                        TestSession::FailTest(
                            "EndResolveServicePartition expected {0}={1} but completed with {2}={3}",
                            expectedError,
                            ErrorCode::FromHResult(expectedError),
                            endResolveHr,
                            ErrorCode::FromHResult(endResolveHr));
                    }
                }
                else
                {
                    // Returned error code matches expected one, done
                    break;
                }
            }
        }

        ++retryCount;

        bool failed = (!verifyResult || (!isCommand && !storeServiceFound));

        doRetry = failed && (!helper.IsExpired);

        TestSession::WriteNoise(
            TraceSource,
            "Resolve name for service {0}:{1} completed with Begin HR {2}, End HR {3}, verifyResult {4}, storeServiceFound {5} for iteration {6}. DoRetry={7}.",
            serviceName,
            keyValue,
            beginResolveHr,
            endResolveHr,
            verifyResult,
            storeServiceFound,
            retryCount,
            doRetry);

        if (!isCommand && failed)
        {
            storeService.reset();
            if (FABRICSESSION.FabricDispatcher.IsQuorumLostFU(fuId) || FABRICSESSION.FabricDispatcher.IsRebuildLostFU(fuId))
            {
                TestSession::WriteNoise(TraceSource, "Quorum or Rebuild lost at {0}:{1} for key {2} so skipping ResolveService", serviceName, fuId, keyValue);
                return nullptr;
            }

            if (endResolveHr == ErrorCode(ErrorCodeValue::GatewayUnreachable).ToHResult() ||
                endResolveHr == ErrorCode(ErrorCodeValue::Timeout).ToHResult())
            {
                TestSession::WriteNoise(TraceSource, "Resolve {0}:{1}: Timeout or communication error: Check whether the naming client has any active gateways, skip resolve", serviceName, keyValue);
                return nullptr;
            }
        }

        if (doRetry)
        {
            Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().NamingOperationRetryDelay.TotalMilliseconds()));
        }
    } while (doRetry);

    if ((expectedError == S_OK) && !allowAllError)
    {
        TestSession::FailTestIfNot(SUCCEEDED(endResolveHr), "EndResolveServicePartition failed. hresult = {0}", endResolveHr);
        TestSession::FailTestIfNot(verifyResult, "Verification of resolve result failed");
        TestSession::FailTestIf(!isCommand && !storeServiceFound, "Store for service {0}, key {1} not found after {2} iterations", serviceName, keyValue, retryCount);
    }
    else if ((expectedError != S_OK) && !allowAllError && (SUCCEEDED(beginResolveHr) && SUCCEEDED(endResolveHr)))
    {
        TestSession::FailTest(
            "EndResolveServicePartition expected {0} but completed with {1}",
            expectedError,
            endResolveHr);
    }
    return storeService;
}

bool TestFabricClient::PrefixResolveService(
    CommandLineParser const & parser, 
    IClientFactoryPtr const & clientFactory)
{
    IServiceManagementClientPtr client;
    auto error = clientFactory->CreateServiceManagementClient(client);
    TestSession::FailTestIfNot(error.IsSuccess(), "CreateServiceManagementClient failed: {0}", error);

    wstring prefixString;
    if (!parser.TryGetString(L"prefix", prefixString, L""))
    {
        TestSession::WriteError(TraceSource, "Missing 'prefix' parameter");
        return false;
    }

    vector<wstring> prefixTokens;
    StringUtility::Split<wstring>(prefixString, prefixTokens, L";");

    vector<NamingUri> prefixNames;
    for (auto const & prefixNameString : prefixTokens)
    {
        NamingUri prefixName;
        if (!NamingUri::TryParse(prefixNameString, prefixName))
        {
            TestSession::WriteError(TraceSource, "Invalid prefix: '{0}'", prefixNameString);
            return false;
        }

        prefixNames.push_back(prefixName);
    }

    wstring expectedMatch;
    parser.TryGetString(L"match", expectedMatch, L"");

    wstring expectedErrorString;
    parser.TryGetString(L"error", expectedErrorString, L"Success");

    if (expectedMatch.empty() && expectedErrorString.empty())
    {
        TestSession::WriteError(TraceSource, "Missing either 'match' or 'error' parameter");
        return false;
    }

    auto expectedErrorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(expectedErrorString);

    ServiceResolutionRequestData requestData;

    int partitionKey = 0;
    if (parser.TryGetInt(L"partitionKey", partitionKey))
    {
        requestData = ServiceResolutionRequestData(PartitionKey(partitionKey));
    }

    IResolvedServicePartitionResultPtr previousResult;

    bool bypassCache = parser.GetBool(L"bypass");

    auto timeout = FabricTestSessionConfig::GetConfig().NamingOperationTimeout;

    for (auto retry=0; retry<10; ++retry)
    {
        ManualResetEvent event(false);
        Common::atomic_long pendingCount(static_cast<LONG>(prefixNames.size()));
        Common::atomic_long failCount(0);
        Common::atomic_long retryCount(0);

        for (auto const & prefixName : prefixNames)
        {
            client->BeginPrefixResolveServicePartition(
                prefixName,
                requestData,
                previousResult,
                bypassCache,
                timeout,
                [&client, &event, &pendingCount, &failCount, &retryCount, &expectedMatch, &expectedErrorValue]
                (AsyncOperationSPtr const & operation) 
                { 
                    ResolvedServicePartitionSPtr rsp;
                    auto error = client->EndPrefixResolveServicePartition(operation, rsp);

                    bool success = true;
                    if (error.IsError(expectedErrorValue))
                    {
                        if (error.IsSuccess())
                        {
                            if (rsp)
                            {
                                if (rsp->Locations.ServiceName != expectedMatch)
                                {
                                    TestSession::WriteError(
                                        TraceSource, 
                                        "Prefix mismatch: expected={0} actual={1}", 
                                        expectedMatch, 
                                        rsp->Locations.ServiceName);

                                    success = false;
                                }
                            }
                            else
                            {
                                TestSession::WriteError(TraceSource, "RSP result is null");
                                success = false;
                            }
                        }
                    }
                    else if (IsRetryableError(error.ToHResult()))
                    {
                        TestSession::WriteInfo(TraceSource, "Retryable: {0}", error);
                        ++retryCount;
                    }
                    else
                    {
                        TestSession::WriteError(
                            TraceSource, 
                            "ErrorCode mismatch: expected={0} actual={1}", 
                            expectedErrorValue, 
                            error);
                        
                        success = false;
                    }

                    if (!success)
                    {
                        ++failCount;
                    }

                    if (--pendingCount == 0)
                    {
                        event.Set(); 
                    }
                },
                AsyncOperationSPtr());
        } // for prefixNames

        auto wait = event.WaitOne(timeout);

        if (wait && retryCount.load() == 0 && failCount.load() == 0)
        {
            return true;
        }
        else if (failCount.load() > 0)
        {
            TestSession::WriteInfo(
                TraceSource, 
                "Failed: total={0} fail={1} retry={2}", 
                prefixNames.size(), 
                failCount.load(),
                retryCount.load());

            return false;
        }

        TestSession::WriteInfo(
            TraceSource, 
            "Retrying: total={0} fail={1} retry={2}", 
            prefixNames.size(), 
            failCount.load(),
            retryCount.load());

        Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().NamingOperationRetryDelay.TotalMilliseconds()));

    } // retry

    return false;
}

ITestStoreServiceSPtr TestFabricClient::ResolveServiceWithEvents(std::wstring const& serviceName, FABRIC_PARTITION_KEY_TYPE keyType, void const * key, Guid const& fuId, HRESULT expectedError, bool isCommand)
{
    int retryCount = 0;
    ITestStoreServiceSPtr storeService;
    bool storeServiceFound = false;
    bool verifyResult = false;
    bool doRetry = false;
    TimeoutHelper helper(FabricTestSessionConfig::GetConfig().NamingOperationRetryTimeout);
    ResolvedServicePartition resolvedServiceLocations;
    ComPointer<IFabricServiceManagementClient> serviceClient = CreateFabricServiceClient(FABRICSESSION.FabricDispatcher.Federation);

    wstring keyValue = GetKeyValue(keyType, key);

    do
    {
        ComPointer<IFabricResolvedServicePartitionResult> resolvedServiceResult;
        HRESULT failure;
        LONGLONG handlerId;
        ManualResetEvent resetEvent(false);
        auto callback = make_com<ComTestServiceChangeHandlerImpl>(
            [&resetEvent, &handlerId, &resolvedServiceResult, &failure](IFabricServiceManagementClient *, LONGLONG, IFabricResolvedServicePartitionResult *rsp, HRESULT error)
            {
                failure = error;
                if (rsp != NULL)
                {
                    resolvedServiceResult.SetAndAddRef(rsp);
                }
                resetEvent.Set();
            });

        LONGLONG callbackHandler = 0;
        HRESULT hrRegister = serviceClient->RegisterServicePartitionResolutionChangeHandler(
            serviceName.c_str(),
            keyType,
            key,
            callback.GetRawPointer(),
            &callbackHandler);
        TestSession::FailTestIfNot(SUCCEEDED(hrRegister), "RegisterServicePartitionResolutionChangeHandler failed.  hresult = {0}", hrRegister);

        TimeSpan timeToWait = FabricTestSessionConfig::GetConfig().NamingOperationTimeout + FabricTestSessionConfig::GetConfig().NamingOperationTimeoutBuffer;
        TestSession::FailTestIfNot(resetEvent.WaitOne(timeToWait),
            "ServiceChange callback did not complete in {0} seconds although a timeout of {1} was provided",
            timeToWait,
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout);

        serviceClient->UnregisterServicePartitionResolutionChangeHandler(callbackHandler);

        if (resolvedServiceResult)
        {
            // Address changed
            HRESULT hr = TryConvertComResult(resolvedServiceResult, resolvedServiceLocations);
            TestSession::FailTestIfNot(SUCCEEDED(hr), "Converting COM service results failed. hresult = {0}", hr);

            if (isCommand)
            {
                TestSession::WriteInfo(
                    TraceSource,
                    "Resolve name for service {0} returned {1} result(s).", serviceName, resolvedServiceLocations.GetEndpointCount());
            }

            storeServiceFound = false;

            if (!isCommand && resolvedServiceLocations.IsStateful && resolvedServiceLocations.Locations.ServiceReplicaSet.IsPrimaryLocationValid)
            {
                TestSession::WriteNoise(
                    TraceSource,
                    "Looking for IStoreService at location '{0}'.", resolvedServiceLocations.Locations.ServiceReplicaSet.PrimaryLocation);

                storeServiceFound = FABRICSESSION.FabricDispatcher.Federation.TryFindStoreService(
                    resolvedServiceLocations.Locations.ServiceReplicaSet.PrimaryLocation,
                    storeService);
            }

            verifyResult = FABRICSESSION.FabricDispatcher.VerifyResolveResult(resolvedServiceLocations, serviceName);
            doRetry =
                (!verifyResult || (!isCommand && !storeServiceFound)) &&
                (!helper.IsExpired);
        }
        else
        {
            verifyResult = (expectedError == failure);
            doRetry = (!verifyResult) && (!helper.IsExpired);
            TestSession::WriteNoise(TraceSource,
                "{0}: Address dection failed with {1}, expected {2}.", serviceName, failure, expectedError);
        }

        ++retryCount;

        if (doRetry)
        {
            TestSession::WriteNoise(
                TraceSource,
                "Resolve name for service {0} completed, verifyResult {1}, storeServiceFound {2} for iteration {3}.",
                serviceName,
                verifyResult,
                storeServiceFound,
                retryCount);

            if(!isCommand)
            {
                storeService.reset();
                if(FABRICSESSION.FabricDispatcher.IsQuorumLostFU(fuId) || FABRICSESSION.FabricDispatcher.IsRebuildLostFU(fuId))
                {
                    TestSession::WriteNoise(TraceSource, "Quorum or Rebuild lost at {0}:{1} for key {2} so skipping ResolveService", serviceName, fuId, keyValue);
                    return nullptr;
                }
            }

            Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().NamingOperationRetryDelay.TotalMilliseconds()));
        }

    } while (doRetry);

    TestSession::WriteNoise(TraceSource,
        "Resolve name for service {0} completed, verifyResult {1}.", serviceName, verifyResult);

    if (expectedError == S_OK)
    {
        TestSession::FailTestIfNot(verifyResult, "Verification of resolve result failed");
        TestSession::FailTestIf(!isCommand && !storeServiceFound, "Store for service {0}, key {1} not found after {2} iterations", serviceName, keyValue, retryCount);
    }
    else
    {
        TestSession::FailTestIfNot(verifyResult, "Expected resolve failure {0} not encountered", expectedError);
    }

    return storeService;
}

void TestFabricClient::GetPartitionedServiceDescriptors(
    StringCollection const& serviceNames,
    __out vector<PartitionedServiceDescriptor> & serviceDescriptors)
{
    this->GetPartitionedServiceDescriptors(serviceNames, L"", serviceDescriptors);
}

void TestFabricClient::GetPartitionedServiceDescriptors(
    StringCollection const& serviceNames,
    wstring const & gatewayAddress,
    __out vector<PartitionedServiceDescriptor> & serviceDescriptors)
{
    this->GetPartitionedServiceDescriptors(
        serviceNames,
        gatewayAddress,
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        serviceDescriptors);
}

void TestFabricClient::GetPartitionedServiceDescriptors(
    StringCollection const& serviceNames,
    wstring const & gatewayAddress,
    TimeSpan const operationTimeout,
    __out vector<PartitionedServiceDescriptor> & serviceDescriptors)
{
    vector<wstring> connectionStrings;

    if (gatewayAddress.empty())
    {
        FABRICSESSION.FabricDispatcher.Federation.GetEntreeServiceAddress(connectionStrings);
    }
    else
    {
        connectionStrings.push_back(gatewayAddress);
    }

    shared_ptr<FabricClientImpl> fabricClientSPtr = make_shared<FabricClientImpl>(move(connectionStrings));
    fabricClientSPtr->SetSecurity(SecuritySettings(FABRICSESSION.FabricDispatcher.FabricClientSecurity));

    for (auto iter = serviceNames.begin(); iter != serviceNames.end(); iter++)
    {
        bool doRetry = false;
        int retryCount = 0;
        ErrorCode error;
        TimeoutHelper helper(FabricTestSessionConfig::GetConfig().NamingOperationRetryTimeout);
        PartitionedServiceDescriptor result;
        do
        {
            NamingUri name;
            TestSession::FailTestIfNot(NamingUri::TryParse(*iter, name), "Could not parse {0}", *iter);
            auto waiter = make_shared<AsyncOperationWaiter>();
            auto operation = fabricClientSPtr->BeginInternalGetServiceDescription(
                name,
                FabricActivityHeader(Guid::NewGuid()),
                operationTimeout,
                [this, waiter] (AsyncOperationSPtr const &)
                {
                    waiter->Set();
                },
                AsyncOperationSPtr());

            waiter->WaitOne();

            error = fabricClientSPtr->EndInternalGetServiceDescription(operation, result);
            doRetry = (error.ReadValue() != ErrorCodeValue::Success && !helper.IsExpired);
            TestSession::WriteNoise(
                TraceSource,
                "EndGetServiceDescription {0} completed with error={1}, iteration={2}, doRetry={3}",
                name.ToString(),
                error,
                retryCount,
                doRetry);

            if(doRetry)
            {
                Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().NamingOperationRetryDelay.TotalMilliseconds()));
                retryCount++;
            }

        } while(doRetry);

        TestSession::FailTestIfNot(error.IsSuccess(), "fabricClientSPtr->EndGetServiceDescription failed with error {0}", error);
        serviceDescriptors.push_back(result);
    }

    // Try to close the client if we performed any client operation.
    if (!serviceNames.empty())
    {
        TestSession::FailTestIfNot(fabricClientSPtr->Close().IsSuccess(), "fabricClientSPtr->Close failed");
    }
}

HRESULT TestFabricClient::PerformFabricClientOperation(
    std::wstring const & operationName,
    TimeSpan const timeout,
    BeginFabricClientOperationCallback const & beginCallback,
    EndFabricClientOperationCallback const & endCallback,
    HRESULT expectedError,
    HRESULT allowedErrorOnRetry,
    HRESULT retryableError,
    bool failOnError,
    std::wstring const & expectedErrorMessageFragment)
{
    vector<HRESULT> expectedErrors;
    expectedErrors.push_back(expectedError);
    return PerformFabricClientOperation(
        operationName,
        timeout,
        beginCallback,
        endCallback,
        expectedErrors,
        allowedErrorOnRetry,
        retryableError,
        failOnError,
        expectedErrorMessageFragment);
}

HRESULT TestFabricClient::PerformFabricClientOperation(
    std::wstring const & operationName,
    Common::TimeSpan const timeout,
    BeginFabricClientOperationCallback const & beginCallback,
    EndFabricClientOperationCallback const & endCallback,
    vector<HRESULT> expectedErrors,
    HRESULT allowedErrorOnRetry,
    HRESULT retryableError,
    bool failOnError,
    std::wstring const & expectedErrorMessageFragment)
{
    vector<HRESULT> allowedErrorsOnRetry;
    allowedErrorsOnRetry.push_back(allowedErrorOnRetry);

    vector<HRESULT> retryableErrors;
    retryableErrors.push_back(retryableError);

    return PerformFabricClientOperation(
        operationName,
        timeout,
        beginCallback,
        endCallback,
        expectedErrors,
        allowedErrorsOnRetry,
        retryableErrors,
        failOnError,
        expectedErrorMessageFragment);
}

HRESULT TestFabricClient::FabricGetLastErrorMessageString(wstring & message)
{
    message.clear();

    ComPointer<IFabricStringResult> stringCPtr;
    auto innerHr = ::FabricGetLastErrorMessage(stringCPtr.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(innerHr), "FabricGetLastErrorMessage() failed: hr = {0}", innerHr);

    innerHr = StringUtility::LpcwstrToWstring(stringCPtr->get_String(), true, message);
    TestSession::FailTestIfNot(SUCCEEDED(innerHr), "FabricGetLastErrorMessage(), LPCWSTR->wstring failed: hr = {0}", innerHr);

    return innerHr;
}

bool TestFabricClient::CheckExpectedErrorMessage(
    std::wstring const & operationName,
    bool beginMethod,
    HRESULT hr,
    std::wstring const & expectedErrorMessageFragment)
{
    wstring errorDetails;

    FabricGetLastErrorMessageString(errorDetails);
    bool success = true;

    if (!errorDetails.empty())
    {
        TestSession::WriteInfo(
            TraceSource,
            "{0}{1} Error Details for {2} ({3})= '{4}'",
            beginMethod ? L"Begin" : L"End",
            operationName,
            hr,
            ErrorCode::FromHResult(hr),
            errorDetails);

        if (!expectedErrorMessageFragment.empty())
        {
            // Check that error message matches
            success = StringUtility::Contains<wstring>(errorDetails, expectedErrorMessageFragment);
            if (!success)
            {
                TestSession::WriteWarning(
                    TraceSource, 
                    "{0}{1} Error Details for {2} ({3})= '{4}', expectedErrorMessageFragment={5}",
                    beginMethod ? L"Begin" : L"End",
                    operationName,
                    hr,
                    ErrorCode::FromHResult(hr),
                    errorDetails,
                    expectedErrorMessageFragment);
            }
        }       
    }
    else
    {
        switch ((uint)hr)
        {
        case FABRIC_E_IMAGEBUILDER_VALIDATION_ERROR:
        case FABRIC_E_IMAGEBUILDER_RESERVED_DIRECTORY_ERROR:
        case FABRIC_E_APPLICATION_UPGRADE_VALIDATION_ERROR:
        case FABRIC_E_HEALTH_ENTITY_NOT_FOUND:
            TestSession::FailTest("Always expect error details for errors of type: hr = {0} (error={1})", hr, ErrorCode::FromHResult(hr));
        }

        if (!expectedErrorMessageFragment.empty())
        {
            TestSession::WriteWarning(
                TraceSource,
                "{0}{1} hr={2} (error={3}) returned no error message, expected {4}",
                beginMethod ? L"Begin" : L"End",
                operationName,
                hr,
                ErrorCode::FromHResult(hr),
                expectedErrorMessageFragment);
            success = false;
        }
    }

    return success;
}

HRESULT TestFabricClient::PerformFabricClientOperation(
    std::wstring const & operationName,
    TimeSpan const timeout,
    BeginFabricClientOperationCallback const & beginOperationCallback,
    EndFabricClientOperationCallback const & endOperationCallback,
    vector<HRESULT> const & expectedErrors,
    vector<HRESULT> const & allowedErrorsOnRetry,
    vector<HRESULT> const & retryableErrors,
    bool failOnError,
    std::wstring const & expectedErrorMessageFragment)
{
    TestSession::WriteNoise(
        TraceSource,
        "Performing FabricClient::{0}()",
        operationName);

    TimeoutHelper timeoutHelper(FabricTestSessionConfig::GetConfig().NamingOperationRetryTimeout);

    HRESULT hr = S_OK;
    
    bool success;
    bool doRetry = false;

    do
    {
        success = true;

        DWORD comTimeout = GetComTimeout(timeout);
        ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();
        ComPointer<IFabricAsyncOperationContext> asyncOperation;

        hr = beginOperationCallback(comTimeout, waiter, asyncOperation);

        if (FAILED(hr))
        {
            success = (find(expectedErrors.begin(), expectedErrors.end(), hr) != expectedErrors.end());
            TestSession::FailTestIfNot(success, "Begin{0} failed: hr = {1} ({3}), expected {2}", operationName, hr, expectedErrors, ErrorCode::FromHResult(hr));
            
            success = CheckExpectedErrorMessage(operationName, true, hr, expectedErrorMessageFragment);
            TestSession::FailTestIfNot(success, "Begin{0} failed: hr = {1} ({2}), error message not respected", operationName, hr, ErrorCode::FromHResult(hr));
            
            return hr;
        }

        TimeSpan waitTimeout = timeout + FabricTestSessionConfig::GetConfig().NamingOperationTimeoutBuffer;
        bool waitSuccess = waiter->WaitOne(waitTimeout);

        TestSession::FailTestIfNot(
            waitSuccess,
            "{0} failed to complete in {1}: timeout = {2}",
            operationName,
            waitTimeout,
            timeout);

        hr = endOperationCallback(asyncOperation);

        if (FAILED(hr))
        {
            success = CheckExpectedErrorMessage(operationName, false, hr, expectedErrorMessageFragment);
        }

        if (success)
        {
            success = (find(expectedErrors.begin(), expectedErrors.end(), hr) != expectedErrors.end());
        }

        if (!success && hr == FABRIC_E_GATEWAY_NOT_REACHABLE)
        {
            success = (find(expectedErrors.begin(), expectedErrors.end(), FABRIC_E_TIMEOUT) != expectedErrors.end());

            if (success)
            {
                TestSession::WriteInfo(TraceSource, "{0} expected FABRIC_E_TIMEOUT, handling FABRIC_E_GATEWAY_NOT_REACHABLE as success.", operationName);
            }
        }

        if (!success)
        {
            TestSession::FailTestIf(SUCCEEDED(hr) && !expectedErrors.empty(), "{0} completed with unexpected success: expected = {1}", operationName, expectedErrors);

            if (doRetry)
            {
                success = (find(allowedErrorsOnRetry.begin(), allowedErrorsOnRetry.end(), hr) != allowedErrorsOnRetry.end());

                if (success)
                {
                    TestSession::WriteNoise(TraceSource, "{0} succeeded on retry due to {1}", operationName, hr);
                }
            }
        }

        if (!success)
        {
            bool isRetryable = IsRetryableError(hr) ||
                (find(retryableErrors.begin(), retryableErrors.end(), hr) != retryableErrors.end());

            if (isRetryable)
            {
                if (!timeoutHelper.IsExpired)
                {
                    doRetry = true;
                    Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().NamingOperationRetryDelay.TotalMilliseconds()));
                    TestSession::WriteNoise(TraceSource, "Retrying {0} due to {1} ...", operationName, hr);
                }
                else
                {
                    doRetry = false;
                    TestSession::WriteNoise(TraceSource, "Cannot retry {0} on {1} due to timeout ...", operationName, hr);
                }
            }
            else
            {
                doRetry = false;
                TestSession::WriteNoise(TraceSource, "Cannot retry {0} due to {1} ...", operationName, hr);
            }
        }

    } while (!success && doRetry);

    if (failOnError)
    {
        TestSession::FailTestIfNot(success, "{0} completed with {1} ({3}): expected = {2}", operationName, hr, expectedErrors, ErrorCode::FromHResult(hr));
    }

    return hr;
}

void TestFabricClient::StartTestFabricClient(ULONG clientThreads, ULONG nameCount, ULONG propertiesPerName, int64 clientOperationInterval)
{
    if(nameCount < clientThreads)
    {
        TestSession::WriteWarning(TraceSource, "NameCount is less than client thread count. Setting ClientThreadCount equal to NameCount");
        clientThreads = nameCount;
    }

    clientThreads_ = clientThreads;
    maxClientOperationInterval_ = clientOperationInterval;
    clientThreadRange_ = static_cast<ULONG>(ceil(((1.0 * nameCount) / clientThreads)));
    wstring  testNamePrefix(L"fabric:/TestName");
    for(auto i = 0; i < clientThreads_; i++)
    {
        wstring name = testNamePrefix + StringUtility::ToWString(i);
        NamingUri namingUri;
        NamingUri::TryParse(name, namingUri);
        testNamingEntries_.push_back(make_shared<TestNamingEntry>(namingUri, propertiesPerName));
    }

    for(auto i = 0; i < clientThreads_; i++)
    {
        Threadpool::Post([this, i]()
        {
            DoWork(i);
        });
    }
}

void TestFabricClient::StopTestFabricClient()
{
    //TODO: Make timeout value confiurable if required
    closed_ = true;
    bool result = threadDoneEvent_.WaitOne(TimeSpan::FromSeconds(200));
    TestSession::FailTestIfNot(result, "TestFabricClient did not finish in time");
    TestSession::WriteInfo(TraceSource, "TestFabricClient done");
}

void TestFabricClient::VerifyNodeLoad(wstring nodeName, wstring metricName, int64 expectedNodeLoadValue, double expectedDoubleLoad)
{
    ComPointer<IFabricQueryClient3> queryClient3;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(FABRICSESSION.FabricDispatcher.Federation)->QueryInterface(
        IID_IFabricQueryClient3,
        (void **)queryClient3.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricQueryClient2");
    uint maxTry = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    for (uint i = 0; i < maxTry; ++i)
    {
        bool toAssert = (i == maxTry - 1);
        FABRIC_NODE_LOAD_INFORMATION_QUERY_DESCRIPTION nodeLoadQueryDescription;
        nodeLoadQueryDescription.NodeName = nodeName.c_str();
        nodeLoadQueryDescription.Reserved = NULL;

        ComPointer<IFabricGetNodeLoadInformationResult> result;

        this->PerformFabricClientOperation(
            L"GetNodeLoadInformation",
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return queryClient3->BeginGetNodeLoadInformation(&nodeLoadQueryDescription, timeout, callback.GetRawPointer(), context.InitializationAddress());
        },
            [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return queryClient3->EndGetNodeLoadInformation(context.GetRawPointer(), result.InitializationAddress());
        },
            S_OK);

        ServiceModel::NodeLoadInformationQueryResult queryResult;
        queryResult.FromPublicApi(*(result->get_NodeLoadInformation()));

        std::vector<ServiceModel::NodeLoadMetricInformation> nodeLoads = queryResult.NodeLoadMetricInformation;

        bool metricFound = false;

        for (auto iter = nodeLoads.begin(); iter != nodeLoads.end(); ++iter)
        {
            if (metricName == iter->Name)
            {
                metricFound = true;
                bool nodeLoadExpected = expectedNodeLoadValue == iter->NodeLoad;

                if (expectedDoubleLoad >= 0)
                {
                    nodeLoadExpected = nodeLoadExpected && (expectedDoubleLoad == iter->CurrentNodeLoad);
                }

                if (nodeLoadExpected)
                {
                    return;
                }

                if (toAssert){
                    TestSession::FailTestIfNot(nodeLoadExpected,
                        "NodeLoad expect {0} ({1}), real: {2} ({3})",
                        expectedNodeLoadValue,
                        expectedDoubleLoad,
                        iter->NodeLoad,
                        iter->CurrentNodeLoad);
                }
            }
        }

        if (toAssert)
        {
            TestSession::FailTestIfNot(metricFound, "NodeLoad metric {0} not present in load query result.", metricName);
        }

        Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().ApiDelayInterval.TotalMilliseconds()));
    }
}

bool TestFabricClient::GetNodeLoadForResources(wstring nodeName, double & cpuUsage, double & memoryUsage)
{
    ComPointer<IFabricQueryClient3> queryClient3;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(FABRICSESSION.FabricDispatcher.Federation)->QueryInterface(
        IID_IFabricQueryClient3,
        (void **)queryClient3.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricQueryClient2");

    FABRIC_NODE_LOAD_INFORMATION_QUERY_DESCRIPTION nodeLoadQueryDescription;
    nodeLoadQueryDescription.NodeName = nodeName.c_str();
    nodeLoadQueryDescription.Reserved = NULL;

    ComPointer<IFabricGetNodeLoadInformationResult> result;

    this->PerformFabricClientOperation(
        L"GetNodeLoadInformation",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return queryClient3->BeginGetNodeLoadInformation(&nodeLoadQueryDescription, timeout, callback.GetRawPointer(), context.InitializationAddress());
        },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return queryClient3->EndGetNodeLoadInformation(context.GetRawPointer(), result.InitializationAddress());
        },
        S_OK);

    ServiceModel::NodeLoadInformationQueryResult queryResult;
    queryResult.FromPublicApi(*(result->get_NodeLoadInformation()));

    auto nodeLoads = queryResult.NodeLoadMetricInformation;

    cpuUsage = 0.0;
    memoryUsage = 0;
    for (auto iter = nodeLoads.begin(); iter != nodeLoads.end(); ++iter)
    {
        if (iter->Name == ServiceModel::Constants::SystemMetricNameCpuCores)
        {
            cpuUsage = iter->CurrentNodeLoad;
        }
        else if (iter->Name == ServiceModel::Constants::SystemMetricNameMemoryInMB)
        {
            memoryUsage = iter->CurrentNodeLoad;
        }
    }

    return true;
}

void TestFabricClient::VerifyClusterLoad(wstring metricName,
    int64 expectedClusterLoad,
    int64 expectedMaxNodeLoadValue,
    int64 expectedMinNodeLoadValue,
    double expectedDeviation,
    double expectedDoubleLoad,
    double expectedDoubleMaxNodeLoad,
    double expectedDoubleMinNodeLoad)
{
    ComPointer<IFabricQueryClient6> queryClient;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(FABRICSESSION.FabricDispatcher.Federation)->QueryInterface(
        IID_IFabricQueryClient6,
        (void **)queryClient.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricQueryClient6");
    uint maxTry = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    for (uint i = 0; i < maxTry; ++i)
    {
        bool toAssert = (i == maxTry-1);
        ComPointer<IFabricGetClusterLoadInformationResult> result;

        this->PerformFabricClientOperation(
            L"GetClusterLoadInformation",
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return queryClient->BeginGetClusterLoadInformation(timeout, callback.GetRawPointer(), context.InitializationAddress());
        },
            [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return queryClient->EndGetClusterLoadInformation(context.GetRawPointer(), result.InitializationAddress());
        },
            S_OK);

        FABRIC_CLUSTER_LOAD_INFORMATION const * clusterLoadInformation = result->get_ClusterLoadInformation();
        ServiceModel::ClusterLoadInformationQueryResult queryResult;
        queryResult.FromPublicApi(*(clusterLoadInformation));
        std::vector<ServiceModel::LoadMetricInformation> & allMetrics = queryResult.LoadMetric;

        for (auto iter = allMetrics.begin(); iter != allMetrics.end(); ++iter)
        {
            if (metricName == iter->Name)
            {
                bool clusterLoadExpected = expectedClusterLoad == iter->ClusterLoad;
                bool minNodeLoadExpected = expectedMinNodeLoadValue == iter->MinNodeLoadValue;
                bool maxNodeLoadExpected = expectedMaxNodeLoadValue == iter->MaxNodeLoadValue;
                bool deviationExpected = expectedDeviation == -1 || fabs(expectedDeviation - iter->DeviationAfter) < 0.01;

                bool doubleLoadExpected = expectedDoubleLoad < 0.0 || fabs(expectedDoubleLoad - iter->CurrentClusterLoad) < 0.001;
                bool doubleMaxNodeLoadExpected = expectedDoubleMaxNodeLoad < 0.0 || fabs(expectedDoubleMaxNodeLoad - iter->MaximumNodeLoad) < 0.001;
                bool doubleMinNodeLoadExpected = expectedDoubleMinNodeLoad < 0.0 || fabs(expectedDoubleMinNodeLoad - iter->MinimumNodeLoad) < 0.001;

                if (   clusterLoadExpected && minNodeLoadExpected && maxNodeLoadExpected && deviationExpected
                    && doubleLoadExpected && doubleMaxNodeLoadExpected && doubleMinNodeLoadExpected)
                {
                    return;
                }

                if (toAssert){
                    TestSession::FailTestIfNot(clusterLoadExpected, "ClusterLoad expect {0}, real: {1}", expectedClusterLoad, iter->ClusterLoad);
                    TestSession::FailTestIfNot(minNodeLoadExpected, "MinNodeLoad expect {0}, real: {1}", expectedMinNodeLoadValue, iter->MinNodeLoadValue);
                    TestSession::FailTestIfNot(maxNodeLoadExpected, "MaxNodeLoad expect {0}, real: {1}", expectedMaxNodeLoadValue, iter->MaxNodeLoadValue);
                    TestSession::FailTestIfNot(deviationExpected, "Deviation expect {0}, real: {1}", expectedDeviation, iter->DeviationAfter);
                    TestSession::FailTestIfNot(doubleLoadExpected, "CurrentClusterLoad expect {0}, real: {1}", expectedDoubleLoad, iter->CurrentClusterLoad);
                    TestSession::FailTestIfNot(doubleMaxNodeLoadExpected, "MaximumNodeLoad expect {0}, real: {1}", expectedDoubleMaxNodeLoad, iter->MaximumNodeLoad);
                    TestSession::FailTestIfNot(doubleMinNodeLoadExpected, "MinimumNodeLoad expect {0}, real: {1}", expectedDoubleMinNodeLoad, iter->MinimumNodeLoad);
                }
            }
        }

        Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().ApiDelayInterval.TotalMilliseconds()));
    }
}

void TestFabricClient::VerifyPartitionLoad(FABRIC_PARTITION_ID partitionId, wstring metricName, int64 expectedPrimaryLoad, int64 expectedSecondaryLoad)
{
    ComPointer<IFabricQueryClient6> queryClient;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(FABRICSESSION.FabricDispatcher.Federation)->QueryInterface(
        IID_IFabricQueryClient6,
        (void **)queryClient.InitializationAddress());

    bool expectedMetricNotExist = expectedPrimaryLoad == -1 && expectedSecondaryLoad == -1;

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricQueryClient6");
    uint maxTry = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    for (uint i = 0; i < maxTry; ++i)
    {
        bool toAssert = (i == maxTry - 1);
        ComPointer<IFabricGetPartitionLoadInformationResult> result;
        FABRIC_PARTITION_LOAD_INFORMATION_QUERY_DESCRIPTION partitionLoadQueryDescription;
        partitionLoadQueryDescription.PartitionId = partitionId;
        partitionLoadQueryDescription.Reserved = NULL;

        this->PerformFabricClientOperation(
            L"GetPartitionLoadInformation",
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return queryClient->BeginGetPartitionLoadInformation(&partitionLoadQueryDescription, timeout, callback.GetRawPointer(), context.InitializationAddress());
        },
            [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return queryClient->EndGetPartitionLoadInformation(context.GetRawPointer(), result.InitializationAddress());
        },
            S_OK);

        FABRIC_PARTITION_LOAD_INFORMATION const * partitionLoadInformation = result->get_PartitionLoadInformation();
        ServiceModel::PartitionLoadInformationQueryResult queryResult;
        queryResult.FromPublicApi(*(partitionLoadInformation));
        std::vector<ServiceModel::LoadMetricReport> const & primaryLoadMetricReport = queryResult.PrimaryLoadMetricReports;
        std::vector<ServiceModel::LoadMetricReport> const & secondaryLoadMetricReport = queryResult.SecondaryLoadMetricReports;

        bool primaryloadFound = false;
        bool primaryLoadMatch = false;
        for (ServiceModel::LoadMetricReport metricReport : primaryLoadMetricReport)
        {
            if (metricReport.Name == metricName)
            {
                primaryloadFound = true;
                primaryLoadMatch = metricReport.Value == expectedPrimaryLoad;
                if (toAssert)
                {
                    TestSession::FailTestIfNot(primaryLoadMatch, "PartitionPrimaryLoad for metric {0}, expect {1}, real: {2}", metricName, expectedPrimaryLoad, metricReport.Value);
                }
            }
        }

        if (toAssert && !expectedMetricNotExist)
        {
            TestSession::FailTestIfNot(primaryloadFound, "Cannot find metirc for the primary load {0}", metricName);
        }

        bool secondaryloadFound = false;
        bool secondaryLoadMatch = false;

        for (ServiceModel::LoadMetricReport metricReport : secondaryLoadMetricReport)
        {
            if (metricReport.Name == metricName)
            {
                secondaryloadFound = true;
                secondaryLoadMatch = metricReport.Value == expectedSecondaryLoad;
                if (toAssert)
                {
                    TestSession::FailTestIfNot(secondaryLoadMatch, "PartitionSecondaryLoad for metrics{0}, expect {1}, real: {2}", metricName, expectedSecondaryLoad, metricReport.Value);
                }
            }
        }

        if (toAssert && !expectedMetricNotExist)
        {
            TestSession::FailTestIfNot(secondaryloadFound, "Cannot find metirc for the secondary load  {0}", metricName);
        }

        if (primaryLoadMatch && secondaryLoadMatch && !expectedMetricNotExist)
        {
            return;
        }

        if (expectedMetricNotExist)
        {
            if (primaryloadFound || secondaryloadFound)
            {
                TestSession::FailTest("Expect metric not exists but found in partition load. Metric name: {0}", metricName);
            }
            else
            {
                break;
            }
        }

        Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().ApiDelayInterval.TotalMilliseconds()));
    }
}

void TestFabricClient::VerifyUnplacedReason(std::wstring serviceName, std::wstring reason)
{
    ComPointer<IFabricQueryClient6> queryClient;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(FABRICSESSION.FabricDispatcher.Federation)->QueryInterface(
        IID_IFabricQueryClient6,
        (void **)queryClient.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricQueryClient6");
    ComPointer<IFabricGetUnplacedReplicaInformationResult> result;

    FABRIC_UNPLACED_REPLICA_INFORMATION_QUERY_DESCRIPTION queryDescription;
    queryDescription.ServiceName = serviceName.c_str();
    queryDescription.PartitionId = Guid::Empty().AsGUID();
    queryDescription.OnlyQueryPrimaries = false;
    queryDescription.Reserved = NULL;

    FABRIC_UNPLACED_REPLICA_INFORMATION_QUERY_DESCRIPTION * queryDescriptionPtr = &queryDescription;

    this->PerformFabricClientOperation(
        L"GetUnplacedReplicaInformation",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return queryClient->BeginGetUnplacedReplicaInformation(queryDescriptionPtr, timeout, callback.GetRawPointer(), context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return queryClient->EndGetUnplacedReplicaInformation(context.GetRawPointer(), result.InitializationAddress());
    },
        S_OK);

    FABRIC_UNPLACED_REPLICA_INFORMATION const * unplacedReplicaInformation = result->get_UnplacedReplicaInformation();
    ServiceModel::UnplacedReplicaInformationQueryResult queryResult;
    queryResult.FromPublicApi(*(unplacedReplicaInformation));
    std::vector<std::wstring> & allReasons = queryResult.UnplacedReplicaDetails;

    bool reasonPresent = false;

    for (auto iter = allReasons.begin(); iter != allReasons.end(); ++iter)
    {
        if (StringUtility::Contains(*iter, reason))
        {
            reasonPresent = true;
        }
    }

    TestSession::FailTestIfNot(reasonPresent, "Reason expected not found: {0}", reason);
}

void TestFabricClient::GetApplicationLoadInformation(std::wstring applicationName, ServiceModel::ApplicationLoadInformationQueryResult & queryResult)
{
    ComPointer<IFabricQueryClient7> queryClient7;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(FABRICSESSION.FabricDispatcher.Federation)->QueryInterface(
        IID_IFabricQueryClient7,
        (void **)queryClient7.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricQueryClient7");


    FABRIC_APPLICATION_LOAD_INFORMATION_QUERY_DESCRIPTION applicationLoadQueryDescription;
    applicationLoadQueryDescription.ApplicationName = applicationName.c_str();
    applicationLoadQueryDescription.Reserved = NULL;

    ComPointer<IFabricGetApplicationLoadInformationResult> result;

    this->PerformFabricClientOperation(
        L"GetApplicationLoadInformation",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return queryClient7->BeginGetApplicationLoadInformation(&applicationLoadQueryDescription, timeout, callback.GetRawPointer(), context.InitializationAddress());
        },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return queryClient7->EndGetApplicationLoadInformation(context.GetRawPointer(), result.InitializationAddress());
        },
        S_OK);

    queryResult.FromPublicApi(*(result->get_ApplicationLoadInformation()));
}

//Currently Naming does not expose sequence numbers or conditional writes etc thus disabling optimistic concurrency
//hence all access to a specific name is single threaded and we dont require locks
//Revisit locking for this class once access to a name is multithreaded.
void TestFabricClient::DoWork(ULONG threadId)
{
    TestSession::WriteNoise(TraceSource, "TestFabricClient thread {0} invoked", threadId);

    int minIndex = threadId * clientThreadRange_;
    int maxIndex = (threadId + 1) * clientThreadRange_;
    int testNamingEntriesSize = static_cast<int>(testNamingEntries_.size());
    int nameIndex = random_.Next(minIndex, min(maxIndex, testNamingEntriesSize));
    TestNamingEntrySPtr const& testNamingEntrySPtr = testNamingEntries_[nameIndex];

    //10% chance we delete the property
    double deletePropertyRatio = 0.1;
    //If the property is not to be deleted then we put 75% of the times
    double putPropertyRatio = 0.75;

    if(testNamingEntrySPtr->IsCreated)
    {
        if(random_.NextDouble() < deletePropertyRatio)
        {
            //If property exists then Delete or confirm property does not exist
            wstring propertyName = testNamingEntrySPtr->GetRandomPropertyName(random_);
            if(testNamingEntrySPtr->GetValue(propertyName) == TestFabricClient::EmptyValue)
            {
                DeleteProperty(testNamingEntrySPtr->Name, propertyName, FABRIC_E_PROPERTY_DOES_NOT_EXIST);
            }
            else
            {
                DeleteProperty(testNamingEntrySPtr->Name, propertyName, S_OK);
                testNamingEntrySPtr->SetProperty(propertyName, TestFabricClient::EmptyValue);
            }
        }
        else
        {
            wstring propertyName = testNamingEntrySPtr->GetRandomPropertyName(random_);
            if(random_.NextDouble() < putPropertyRatio)
            {
                //Put new property value into naming store
                wstring propertyValue = Guid::NewGuid().ToString();
                PutProperty(testNamingEntrySPtr->Name, propertyName,  propertyValue, S_OK);
                testNamingEntrySPtr->SetProperty(propertyName, propertyValue);
            }
            else
            {
                //If property exists then Get or confirm property does not exist
                wstring customTypeId;
                if(testNamingEntrySPtr->GetValue(propertyName) == TestFabricClient::EmptyValue)
                {
                    GetProperty(testNamingEntrySPtr->Name, propertyName, TestFabricClient::EmptyValue, customTypeId, L"", FABRIC_E_PROPERTY_DOES_NOT_EXIST);
                }
                else
                {
                    GetProperty(testNamingEntrySPtr->Name, propertyName, testNamingEntrySPtr->GetValue(propertyName), L"", customTypeId, S_OK);
                }
            }
        }
    }
    else //Name is deleted so we Create it
    {
        CreateName(testNamingEntrySPtr->Name, L"", S_OK);
        testNamingEntrySPtr->IsCreated = true;
    }

    if(!closed_)
    {
        Threadpool::Post([this, threadId]()
        {
            DoWork(threadId);
        }, TimeSpan::FromMilliseconds(random_.NextDouble() * maxClientOperationInterval_));
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "TestFabricClient thread {0} done", threadId);
        SignalThreadDone();
    }
}

void TestFabricClient::SignalThreadDone()
{
    if(InterlockedDecrement(&clientThreads_) == 0)
    {
        threadDoneEvent_.Set();
    }
}

bool TestFabricClient::ShowRepairs(StringCollection const & params)
{
    CommandLineParser parser(params);

    auto repairClient = CreateRepairClient();

    FABRIC_REPAIR_SCOPE_IDENTIFIER scope = { FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER, NULL };

    FABRIC_REPAIR_TASK_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.Scope = &scope;

    wstring idFilter;
    if (parser.TryGetString(L"id", idFilter))
    {
        queryDescription.TaskIdFilter = idFilter.c_str();
    }

    int stateMask;
    if (parser.TryGetInt(L"state", stateMask))
    {
        queryDescription.StateFilter = static_cast<FABRIC_REPAIR_TASK_STATE_FILTER>(stateMask);
    }

    wstring executorFilter;
    if (parser.TryGetString(L"executor", executorFilter))
    {
        queryDescription.ExecutorFilter = executorFilter.c_str();
    }

    int timeout = 0;
    parser.TryGetInt(L"timeout", timeout);
    TimeoutHelper timeoutHelper(TimeSpan::FromSeconds(timeout));

    for (;;)
    {
        auto getRepairListResult = GetRepairTaskList(L"ShowRepairs", repairClient, queryDescription);

        auto listPtr = getRepairListResult->get_Tasks();
        TestSession::WriteInfo(TraceSource, "GetRepairList returned {0} results", listPtr->Count);

        int expectedResultCount;
        if (parser.TryGetInt(L"expectedcount", expectedResultCount))
        {
            int actualResultCount = (int) listPtr->Count;

            if (expectedResultCount != actualResultCount)
            {
                if (timeoutHelper.IsExpired)
                {
                    TestSession::FailTest(
                        "Actual result count ({0}) did not match expected result count ({1}); timeout = {2}",
                        actualResultCount,
                        expectedResultCount,
                        timeoutHelper.OriginalTimeout);
                }
                else
                {
                    TestSession::WriteWarning(TraceSource,
                        "Actual result count ({0}) did not match expected result count ({1}); remaining time = {2}",
                        actualResultCount,
                        expectedResultCount,
                        timeoutHelper.GetRemainingTime());

                    Sleep(2000);
                    continue;
                }
            }
        }

        for (ULONG ix = 0; ix < listPtr->Count; ++ix)
        {
            Management::RepairManager::RepairTask internalTask;
            internalTask.FromPublicApi(listPtr->Items[ix]);
            TestSession::WriteInfo(TraceSource, "{0}", internalTask);
        }
        TestSession::WriteInfo(TraceSource, "End of GetRepairList results");

        return true;
    }
}

void SetRepairTaskFields(CommandLineParser & parser, ScopedHeap & heap, FABRIC_REPAIR_TASK & task)
{
    auto scope = heap.AddItem<FABRIC_REPAIR_SCOPE_IDENTIFIER>();
    scope->Kind = FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER;
    scope->Value = NULL;
    task.Scope = scope.GetRawPointer();

    wstring id;
    if (parser.TryGetString(L"id", id))
    {
        task.TaskId = heap.AddString(id);
    }

    int64 version;
    if (parser.TryGetInt64(L"version", version))
    {
        task.Version = version;
    }

    wstring description;
    if (parser.TryGetString(L"description", description))
    {
        task.Description = heap.AddString(description);
    }

    int state;
    if (parser.TryGetInt(L"state", state))
    {
        task.State = static_cast<FABRIC_REPAIR_TASK_STATE>(state);
    }

    int flags;
    if (parser.TryGetInt(L"flags", flags))
    {
        task.Flags = flags;
    }

    wstring action;
    if (parser.TryGetString(L"action", action))
    {
        task.Action = heap.AddString(action);
    }

    wstring targets;
    if (parser.TryGetString(L"targets", targets))
    {
        // TODO also support other (invalid) kinds for negative testing

        vector<wstring> targetList;
        StringUtility::Split(targets, targetList, wstring(L","));

        auto targetListPtr = heap.AddItem<FABRIC_STRING_LIST>();
        ServiceModel::StringList::ToPublicAPI(heap, targetList, targetListPtr);

        auto targetDescription = heap.AddItem<FABRIC_REPAIR_TARGET_DESCRIPTION>();
        targetDescription->Kind = FABRIC_REPAIR_TARGET_KIND_NODE;
        targetDescription->Value = targetListPtr.GetRawPointer();

        task.Target = targetDescription.GetRawPointer();
    }

    wstring executor;
    if (parser.TryGetString(L"executor", executor))
    {
        if (task.ExecutorState == NULL)
        {
            task.ExecutorState = heap.AddItem<FABRIC_REPAIR_EXECUTOR_STATE>().GetRawPointer();
        }
        task.ExecutorState->Executor = heap.AddString(executor);
    }

    wstring executorData;
    if (parser.TryGetString(L"executordata", executorData))
    {
        if (task.ExecutorState == NULL)
        {
            task.ExecutorState = heap.AddItem<FABRIC_REPAIR_EXECUTOR_STATE>().GetRawPointer();
        }
        task.ExecutorState->ExecutorData = heap.AddString(executorData);
    }

    wstring impactString;
    if (parser.TryGetString(L"impact", impactString))
    {
        vector<wstring> impactTokens;
        StringUtility::Split(impactString, impactTokens, wstring(L"[]"));

        task.Impact = heap.AddItem<FABRIC_REPAIR_IMPACT_DESCRIPTION>().GetRawPointer();
        task.Impact->Kind = static_cast<FABRIC_REPAIR_IMPACT_KIND>(Common::Int32_Parse(impactTokens.at(0)));

        if (impactTokens.size() > 1)
        {
            switch (task.Impact->Kind)
            {
            case FABRIC_REPAIR_IMPACT_KIND_NODE:
                {
                    auto impactList = heap.AddItem<FABRIC_REPAIR_NODE_IMPACT_LIST>();
                    task.Impact->Value = impactList.GetRawPointer();

                    vector<wstring> nodeImpactListTokens;
                    StringUtility::Split(impactTokens.at(1), nodeImpactListTokens, wstring(L","));

                    auto impactArray = heap.AddArray<FABRIC_REPAIR_NODE_IMPACT>(nodeImpactListTokens.size());
                    impactList->Count = static_cast<ULONG>(impactArray.GetCount());
                    impactList->Items = impactArray.GetRawArray();

                    for (auto ix = 0; ix < nodeImpactListTokens.size(); ++ix)
                    {
                        vector<wstring> nodeImpactTokens;
                        StringUtility::Split(nodeImpactListTokens.at(ix), nodeImpactTokens, wstring(L"="));

                        impactArray[ix].NodeName = heap.AddString(nodeImpactTokens[0]);
                        impactArray[ix].ImpactLevel = static_cast<FABRIC_REPAIR_NODE_IMPACT_LEVEL>(
                            Common::Int32_Parse(nodeImpactTokens.at(1)));
                    }
                }
                break;
            }
        }
    }

    int resultStatus;
    if (parser.TryGetInt(L"result", resultStatus))
    {
        if (task.Result == NULL)
        {
            task.Result = heap.AddItem<FABRIC_REPAIR_RESULT_DESCRIPTION>().GetRawPointer();
        }
        task.Result->ResultStatus = static_cast<FABRIC_REPAIR_TASK_RESULT>(resultStatus);
    }

    int resultCode;
    if (parser.TryGetInt(L"resultcode", resultCode))
    {
        if (task.Result == NULL)
        {
            task.Result = heap.AddItem<FABRIC_REPAIR_RESULT_DESCRIPTION>().GetRawPointer();
        }
        task.Result->ResultCode = static_cast<HRESULT>(resultCode);
    }

    wstring resultDetails;
    if (parser.TryGetString(L"resultdetail", resultDetails))
    {
        if (task.Result == NULL)
        {
            task.Result = heap.AddItem<FABRIC_REPAIR_RESULT_DESCRIPTION>().GetRawPointer();
        }
        task.Result->ResultDetails = heap.AddString(resultDetails);
    }
}

bool TestFabricClient::CreateRepair(StringCollection const & params)
{
    ScopedHeap heap;
    CommandLineParser parser(params);

    HRESULT expectedError = GetExpectedHResult(parser, L"error");

    auto repairClient = CreateRepairClient();

    FABRIC_REPAIR_TASK newTask = { 0 };
    newTask.Description = L"TestDescription";
    newTask.State = FABRIC_REPAIR_TASK_STATE_CREATED;
    newTask.Action = L"TestAction";

    SetRepairTaskFields(parser, heap, newTask);

    FABRIC_SEQUENCE_NUMBER commitVersion = 0;

    auto hr = this->PerformFabricClientOperation(
        L"CreateRepair",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->BeginCreateRepairTask(
                &newTask,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->EndCreateRepairTask(context.GetRawPointer(), &commitVersion);
        },
        expectedError,
        S_OK,
        FABRIC_E_SERVICE_TYPE_NOT_REGISTERED);

    if (SUCCEEDED(hr))
    {
        TestSession::WriteInfo(TraceSource, "CreateRepairRequest: committed version = {0}", commitVersion);
    }

    return true;
}

bool TestFabricClient::CancelRepair(StringCollection const & params)
{
    CommandLineParser parser(params);

    wstring repairTaskId;
    if (!parser.TryGetString(L"id", repairTaskId))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::CancelRepairCommand);
        return false;
    }

    int64 expectedVersion = 0;
    parser.TryGetInt64(L"version", expectedVersion);

    bool requestAbort = parser.GetBool(L"abort");

    HRESULT expectedError = GetExpectedHResult(parser, L"error");

    auto repairClient = CreateRepairClient();

    FABRIC_SEQUENCE_NUMBER commitVersion = 0;

    FABRIC_REPAIR_SCOPE_IDENTIFIER scope = { FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER, NULL };

    FABRIC_REPAIR_CANCEL_DESCRIPTION requestDescription = { 0 };
    requestDescription.Scope = &scope;
    requestDescription.RepairTaskId = repairTaskId.c_str();
    requestDescription.Version = expectedVersion;
    requestDescription.RequestAbort = requestAbort;

    auto hr = this->PerformFabricClientOperation(
        L"CancelRepair",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->BeginCancelRepairTask(
                &requestDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->EndCancelRepairTask(context.GetRawPointer(), &commitVersion);
        },
        expectedError,
        S_OK,
        FABRIC_E_SERVICE_TYPE_NOT_REGISTERED);

    if (SUCCEEDED(hr))
    {
        TestSession::WriteInfo(TraceSource, "CancelRepairRequest: committed version = {0}", commitVersion);
    }

    return true;
}

bool TestFabricClient::ForceApproveRepair(StringCollection const & params)
{
    CommandLineParser parser(params);

    wstring repairTaskId;
    if (!parser.TryGetString(L"id", repairTaskId))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ForceApproveRepairCommand);
        return false;
    }

    int64 expectedVersion = 0;
    parser.TryGetInt64(L"version", expectedVersion);

    HRESULT expectedError = GetExpectedHResult(parser, L"error");

    auto repairClient = CreateRepairClient();

    FABRIC_SEQUENCE_NUMBER commitVersion = 0;

    FABRIC_REPAIR_SCOPE_IDENTIFIER scope = { FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER, NULL };

    FABRIC_REPAIR_APPROVE_DESCRIPTION requestDescription = { 0 };
    requestDescription.Scope = &scope;
    requestDescription.RepairTaskId = repairTaskId.c_str();
    requestDescription.Version = expectedVersion;

    auto hr = this->PerformFabricClientOperation(
        L"ForceApproveRepair",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->BeginForceApproveRepairTask(
                &requestDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->EndForceApproveRepairTask(context.GetRawPointer(), &commitVersion);
        },
        expectedError,
        S_OK,
        FABRIC_E_SERVICE_TYPE_NOT_REGISTERED);

    if (SUCCEEDED(hr))
    {
        TestSession::WriteInfo(TraceSource, "ForceApproveRepair: committed version = {0}", commitVersion);
    }

    return true;
}

bool TestFabricClient::DeleteRepair(StringCollection const & params)
{
    CommandLineParser parser(params);

    wstring repairTaskId;
    if (!parser.TryGetString(L"id", repairTaskId))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeleteRepairCommand);
        return false;
    }

    int64 expectedVersion = 0;
    parser.TryGetInt64(L"version", expectedVersion);

    HRESULT expectedError = GetExpectedHResult(parser, L"error");

    auto repairClient = CreateRepairClient();

    FABRIC_REPAIR_SCOPE_IDENTIFIER scope = { FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER, NULL };

    FABRIC_REPAIR_DELETE_DESCRIPTION requestDescription = { 0 };
    requestDescription.Scope = &scope;
    requestDescription.RepairTaskId = repairTaskId.c_str();
    requestDescription.Version = expectedVersion;

    this->PerformFabricClientOperation(
        L"DeleteRepair",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->BeginDeleteRepairTask(
                &requestDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->EndDeleteRepairTask(context.GetRawPointer());
        },
        expectedError,
        S_OK,
        FABRIC_E_SERVICE_TYPE_NOT_REGISTERED);

    return true;
}

ComPointer<IFabricGetRepairTaskListResult> TestFabricClient::GetRepairTaskList(
    __in std::wstring const & operationName, 
    __in ComPointer<IFabricRepairManagementClient2> const & repairClient,
    __in FABRIC_REPAIR_TASK_QUERY_DESCRIPTION const & queryDescription) const
{
    ComPointer<IFabricGetRepairTaskListResult> repairListPtr;

    this->PerformFabricClientOperation(
        operationName,
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->BeginGetRepairTaskList(
                &queryDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
            [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->EndGetRepairTaskList(context.GetRawPointer(), repairListPtr.InitializationAddress());
        });

    return repairListPtr;
}

bool TestFabricClient::UpdateRepair(StringCollection const & params)
{
    ScopedHeap heap;
    CommandLineParser parser(params);

    wstring repairTaskId;
    if (!parser.TryGetString(L"id", repairTaskId))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::UpdateRepairCommand);
        return false;
    }

    HRESULT expectedError = GetExpectedHResult(parser, L"error");

    auto repairClient = CreateRepairClient();

    FABRIC_REPAIR_SCOPE_IDENTIFIER scope = { FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER, NULL };

    FABRIC_REPAIR_TASK_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.Scope = &scope;
    queryDescription.TaskIdFilter = repairTaskId.c_str();
    queryDescription.StateFilter = FABRIC_REPAIR_TASK_STATE_FILTER_ALL;

    auto repairListPtr = GetRepairTaskList(L"UpdateRepair_Get", repairClient, queryDescription);

    TestSession::FailTestIf(
        repairListPtr->get_Tasks()->Count != 1,
        "Expected one result, received {0}",
        repairListPtr->get_Tasks()->Count);

    FABRIC_REPAIR_TASK & task = repairListPtr->get_Tasks()->Items[0];

    SetRepairTaskFields(parser, heap, task);

    FABRIC_SEQUENCE_NUMBER commitVersion = 0;

    auto hr = this->PerformFabricClientOperation(
        L"UpdateRepair_Update",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->BeginUpdateRepairExecutionState(
                &task,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->EndUpdateRepairExecutionState(context.GetRawPointer(), &commitVersion);
        },
        expectedError,
        S_OK,
        FABRIC_E_SERVICE_TYPE_NOT_REGISTERED);

    if (SUCCEEDED(hr))
    {
        TestSession::WriteInfo(TraceSource, "UpdateRepairExecutionState: committed version = {0}", commitVersion);
    }

    return true;
}

// TODO, there is still duplication of this method in this file. Remove them
HRESULT TestFabricClient::GetExpectedHResult(
    __in CommandLineParser const & parser, 
    __in std::wstring const & name, 
    __in std::wstring const & defaultValue) const
{
    wstring expectedErrorString;
    parser.TryGetString(name, expectedErrorString, defaultValue);
    ErrorCodeValue::Enum expectedErrorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(expectedErrorString);
    HRESULT expectedError = ErrorCode(expectedErrorValue).ToHResult();

    return expectedError;
}

bool TestFabricClient::UpdateRepairHealthPolicy(StringCollection const & params)
{
    ScopedHeap heap;
    CommandLineParser parser(params);

    wstring repairTaskId;
    if (!parser.TryGetString(L"id", repairTaskId))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::UpdateRepairHealthPolicyCommand);
        return false;
    }

    bool performPreparingHealthCheck = parser.GetBool(L"performpreparinghealthcheck");
    bool performRestoringHealthCheck = parser.GetBool(L"performrestoringhealthcheck");

    DWORD flags = FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_SETTINGS_NONE;
    if (!parser.TryGetULONG(L"flags", flags))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::UpdateRepairHealthPolicyCommand);
        return false;
    }

    HRESULT expectedError = GetExpectedHResult(parser, L"error");

    int64 expectedVersion = 0;
    parser.TryGetInt64(L"version", expectedVersion);

    auto repairClient = CreateRepairClient();

    FABRIC_REPAIR_SCOPE_IDENTIFIER scope = { FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER, NULL };

    FABRIC_REPAIR_TASK_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.Scope = &scope;
    queryDescription.TaskIdFilter = repairTaskId.c_str();
    queryDescription.StateFilter = FABRIC_REPAIR_TASK_STATE_FILTER_ALL;

    auto repairListPtr = GetRepairTaskList(L"UpdateRepair_Get", repairClient, queryDescription);

    TestSession::FailTestIf(
        repairListPtr->get_Tasks()->Count != 1,
        "Expected one result, received {0}",
        repairListPtr->get_Tasks()->Count);

    FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_DESCRIPTION healthPolicyUpdateDescription = { 0 };
    healthPolicyUpdateDescription.Scope = &scope;
    healthPolicyUpdateDescription.RepairTaskId = repairTaskId.c_str();
    healthPolicyUpdateDescription.Version = expectedVersion;
    healthPolicyUpdateDescription.PerformPreparingHealthCheck = performPreparingHealthCheck ? TRUE : FALSE;
    healthPolicyUpdateDescription.PerformRestoringHealthCheck = performRestoringHealthCheck ? TRUE : FALSE;
    healthPolicyUpdateDescription.Flags = flags;

    FABRIC_SEQUENCE_NUMBER commitVersion = 0;

    auto hr = this->PerformFabricClientOperation(
        L"UpdateRepair_UpdateHealthPolicy",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->BeginUpdateRepairTaskHealthPolicy(
                &healthPolicyUpdateDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return repairClient->EndUpdateRepairTaskHealthPolicy(context.GetRawPointer(), &commitVersion);
        },
        expectedError);

    if (SUCCEEDED(hr))
    {
        TestSession::WriteInfo(TraceSource, "EndUpdateRepairTaskHealthPolicy: committed version = {0}", commitVersion);
    }

    return true;
}

/// <summary>
/// Compares int properties. Currently only works for repair task related properties since
/// it expects certain keys passed in from the command line.
/// Returning false indicates that the comparison was not done. 
/// If comparison was done but values didn't match, this method fails the test, so the false
/// return doesn't count.
/// Returning true indicates that the comparison was done and values match.
/// </summary>
bool TestFabricClient::CompareRepairIntProperty(
    __in CommandLineParser const & parser, 
    __in wstring const & parsedPropertyName,
    __in wstring const & propertyName, 
    __in int actualValue) const
{
    if (!StringUtility::AreEqualCaseInsensitive(parsedPropertyName, propertyName)) { return false; }

    int expectedValue;
    if (parser.TryGetInt(L"expectedpropertyvalue", expectedValue))
    {
        wstring message = wformatString("Property name: {0}, expected value: {1}, actual value: {2}", propertyName, expectedValue, actualValue);

        if (expectedValue == actualValue)
        {
            TestSession::WriteInfo(TraceSource, "{0}", message);
            return true;
        }

        TestSession::FailTest("{0}", message);
    }

    return false;
}

bool TestFabricClient::CompareRepairBoolProperty(
    __in CommandLineParser const & parser, 
    __in wstring const & parsedPropertyName,
    __in wstring const & propertyName, 
    __in bool actualValue) const
{
    if (!StringUtility::AreEqualCaseInsensitive(parsedPropertyName, propertyName)) { return false; }

    bool expectedValue = parser.GetBool(L"expectedpropertyvalue");

    wstring message = wformatString("Property name: {0}, expected value: {1}, actual value: {2}", propertyName, expectedValue, actualValue);

    if (expectedValue == actualValue) 
    { 
        TestSession::WriteInfo(TraceSource, "{0}", message);
        return true; 
    }

    TestSession::FailTest("{0}", message);
}

/// <summary>
/// Gets details about a particular repair task.
/// GetRepair id={repair_task_Id} [propertyname={name} expectedpropertyvalue={value}]
/// Currently, only the following properties are supported for query: 
/// - State
/// - PreparingHealthCheckstate
/// - RestoringHealthCheckState
/// - PerformPreparingHealthCheck
/// - PerformRestoringHealthCheck
/// </summary>
/// <example>
/// GetRepair id=RepairA
/// GetRepair id=RepairA propertyname=state expectedpropertyvalue=1
/// </example>
/// <remarks>
/// - Based on need, we could add additional properties
/// - Also, we can only get 1 property at a time now. In future, consider 
///   GetRepair2 with a comma separated propertyNames and expectedPropertyValues
/// </remarks>
bool TestFabricClient::GetRepair(StringCollection const & params)
{
    ScopedHeap heap;
    CommandLineParser parser(params);

    wstring repairTaskId;
    if (!parser.TryGetString(L"id", repairTaskId))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::GetRepairCommand);
        return false;
    }

    auto repairClient = CreateRepairClient();

    FABRIC_REPAIR_SCOPE_IDENTIFIER scope = { FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER, NULL };

    FABRIC_REPAIR_TASK_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.Scope = &scope;
    queryDescription.TaskIdFilter = repairTaskId.c_str();
    queryDescription.StateFilter = FABRIC_REPAIR_TASK_STATE_FILTER_ALL;

    auto repairListPtr = GetRepairTaskList(L"GetRepair", repairClient, queryDescription);

    TestSession::FailTestIf(
        repairListPtr->get_Tasks()->Count != 1,
        "Expected one result, received {0}",
        repairListPtr->get_Tasks()->Count);

    Management::RepairManager::RepairTask internalTask;
    internalTask.FromPublicApi(repairListPtr->get_Tasks()->Items[0]);

    wstring propertyName;
    if (!parser.TryGetString(L"propertyname", propertyName))
    {
        TestSession::WriteInfo(TraceSource, "{0}", internalTask);
        return true;
    }
    
    // if propertyname exists, then expectedpropertyvalue has to exist
    wstring expectedValue;
    if (!parser.TryGetString(L"expectedpropertyvalue", expectedValue))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::GetRepairCommand);
        return false;
    }

    bool compared = CompareRepairIntProperty(parser, propertyName, L"State", internalTask.State);
    if (compared) { return true; }

    compared = CompareRepairIntProperty(parser, propertyName, L"PreparingHealthCheckState", internalTask.PreparingHealthCheckState);
    if (compared) { return true; }

    compared = CompareRepairIntProperty(parser, propertyName, L"RestoringHealthCheckState", internalTask.RestoringHealthCheckState);
    if (compared) { return true; }

    compared = CompareRepairBoolProperty(parser, propertyName, L"PerformPreparingHealthCheck", internalTask.PerformPreparingHealthCheck);
    if (compared) { return true; }

    compared = CompareRepairBoolProperty(parser, propertyName, L"PerformRestoringHealthCheck", internalTask.PerformRestoringHealthCheck);
    if (compared) { return true; }

    return false;
}

ComPointer<IFabricRepairManagementClient2> TestFabricClient::CreateRepairClient()
{
    ComPointer<IFabricRepairManagementClient2> repairClient;
    FabricTestFederation & testFederation = FABRICSESSION.FabricDispatcher.Federation;
    auto comFabricClient = TestFabricClient::FabricCreateClient(testFederation);

    HRESULT hr = comFabricClient->QueryInterface(IID_IFabricRepairManagementClient2, (void**)repairClient.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricRepairManagementClient2");

    return repairClient;
}

bool TestFabricClient::InfrastructureCommand(StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::InfrastructureCommand);
        return false;
    }

    wstring command = params[0];
    vector<wstring> commandTokens;
    StringUtility::Split<wstring>(command, commandTokens, L",");

    CommandLineParser parser(params, 1);

    std::wstring error;
    parser.TryGetString(L"error", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();

    parser.TryGetString(L"queryerror", error, L"Success");
    errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedQueryError = ErrorCode(errorValue).ToHResult();

    std::wstring targetServiceName;
    parser.TryGetString(L"target", targetServiceName);

    bool useClientApi = parser.GetBool(L"clientapi");

    BOOLEAN clientExpectComplete = parser.GetBool(L"clienttimeout") ? FALSE : TRUE;

    if (useClientApi)
    {
        InfrastructureCommandCM(commandTokens, expectedError, clientExpectComplete);
    }
    else
    {
        InfrastructureCommandIS(targetServiceName, commandTokens, expectedError, expectedQueryError);
    }

    return true;
}

void TestFabricClient::InfrastructureCommandCM(
    StringCollection const & commandTokens, 
    HRESULT expectedError,
    BOOLEAN expectedComplete)
{
    FabricTestFederation & testFederation = FABRICSESSION.FabricDispatcher.Federation;
    auto comFabricClient = TestFabricClient::FabricCreateClient(testFederation);

    ComPointer<IInternalFabricClusterManagementClient> client;
    HRESULT hr = comFabricClient->QueryInterface(IID_IInternalFabricClusterManagementClient, (void**)client.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IInternalFabricClusterManagementClient");

    vector<InfrastructureTaskDescription> startTaskDescriptions;
    vector<TaskInstance> finishTaskInstances;

    for (auto it = commandTokens.begin(); it != commandTokens.end(); ++it)
    {
        vector<wstring> taskTokens;
        StringUtility::Split<wstring>(*it, taskTokens, L":");

        TestSession::FailTestIf(taskTokens.empty(), "Could not parse command infra '{0}': no tokens", *it);

        if (taskTokens[0] == L"start")
        {
            TestSession::FailTestIf(taskTokens.size() < 5, "Could not parse command infra '{0}': token count", *it);
            TestSession::FailTestIf(taskTokens.size() % 2 != 1, "Could not parse command infra '{0}': mismatched task list", *it);

            int64 instanceId;
            if (!Common::TryParseInt64(taskTokens[2], instanceId))
            {
                TestSession::FailTest("Could not parse command infra '{0}': invalid instance", *it);
            }

            vector<NodeTaskDescription> nodeTasks;
            for (size_t ix = 3; ix < taskTokens.size(); ix += 2)
            {
                NodeTask::Enum nodeTask;
                if (taskTokens[ix+1] == L"restart")
                {
                    nodeTask = NodeTask::Restart;
                }
                else if (taskTokens[ix+1] == L"relocate")
                {
                    nodeTask = NodeTask::Relocate;
                }
                else if (taskTokens[ix+1] == L"remove")
                {
                    nodeTask = NodeTask::Remove;
                }
                else
                {
                    TestSession::FailTest("Could not parse command infra '{0}': node task type", *it);
                }

                nodeTasks.push_back(NodeTaskDescription(taskTokens[ix], nodeTask));
            }

            startTaskDescriptions.push_back(InfrastructureTaskDescription(
                taskTokens[1],
                instanceId,
                move(nodeTasks)));
        }
        else if (taskTokens[0] == L"finish")
        {
            TestSession::FailTestIf(taskTokens.size() < 3, "Could not parse command infra '{0}': token count", *it);

            int64 instanceId;
            if (!Common::TryParseInt64(taskTokens[2], instanceId))
            {
                TestSession::FailTest("Could not parse command infra '{0}': invalid instance", *it);
            }

            finishTaskInstances.push_back(TaskInstance(taskTokens[1], instanceId));
        }
        else
        {
            TestSession::FailTest("Could not parse command infra '{0}': invalid task type", *it);
        }
    }  // parse commands

    AutoResetEvent commandsEvent(false);
    Common::atomic_long pendingCommands(static_cast<LONG>(startTaskDescriptions.size() + finishTaskInstances.size()));

    ScopedHeap heap;
    for (auto it = startTaskDescriptions.begin(); it != startTaskDescriptions.end(); ++it)
    {
        FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION description = {0};

        it->ToPublicApi(heap, description);

        Threadpool::Post(
            [&, description]()
            {
                BOOLEAN isComplete = FALSE;

                do
                {
                    this->PerformFabricClientOperation(
                        L"StartInfrastructureTask",
                        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
                        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
                        {
                            return client->BeginStartInfrastructureTask(
                                &description,
                                timeout,
                                callback.GetRawPointer(),
                                context.InitializationAddress());
                        },
                        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
                        {
                            return client->EndStartInfrastructureTask(context.GetRawPointer(), &isComplete);
                        },
                        expectedError);

                    if (isComplete == FALSE)
                    {
                        Sleep(1000);
                    }
                } while (isComplete != expectedComplete);

                if (--pendingCommands == 0)
                {
                    commandsEvent.Set();
                }
            });
    } // for start tasks

    for (auto it = finishTaskInstances.begin(); it != finishTaskInstances.end(); ++it)
    {
        TaskInstance taskInstance = *it;

        Threadpool::Post(
            [&, taskInstance]()
            {
                BOOLEAN isComplete = FALSE;

                while (isComplete == FALSE)
                {
                    this->PerformFabricClientOperation(
                        L"FinishInfrastructureTask",
                        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
                        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
                        {
                            return client->BeginFinishInfrastructureTask(
                                taskInstance.Id.c_str(),
                                taskInstance.Instance,
                                timeout,
                                callback.GetRawPointer(),
                                context.InitializationAddress());
                        },
                        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
                        {
                            return client->EndFinishInfrastructureTask(context.GetRawPointer(), &isComplete);
                        },
                        expectedError);

                    if (isComplete == FALSE)
                    {
                        Sleep(1000);
                    }
                }

                if (--pendingCommands == 0)
                {
                    commandsEvent.Set();
                }
            });
    } // for finish tasks

    TimeSpan timeout = FabricTestSessionConfig::GetConfig().NamingOperationTimeout;
    if (FabricTestSessionConfig::GetConfig().NamingOperationRetryTimeout > timeout)
    {
        timeout = FabricTestSessionConfig::GetConfig().NamingOperationRetryTimeout;
    }

    // double the client timeouts as buffer
    timeout = (timeout + timeout);

    bool success = commandsEvent.WaitOne(timeout);

    TestSession::FailTestIfNot(success, "Timed out waiting for infrastructure tasks to complete: timeout = {0}", timeout);
}

void TestFabricClient::InfrastructureCommandIS(
    wstring const & targetServiceName,
    StringCollection const & commandTokens,
    HRESULT expectedError,
    HRESULT expectedQueryError)
{
    FabricTestFederation & testFederation = FABRICSESSION.FabricDispatcher.Federation;
    auto comFabricClient = TestFabricClient::FabricCreateClient(testFederation);

    ComPointer<IFabricInfrastructureServiceClient> client;
    HRESULT hr = comFabricClient->QueryInterface(IID_IFabricInfrastructureServiceClient, (void**)client.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricInfrastructureServiceClient");

    AutoResetEvent commandsEvent(false);
    Common::atomic_long pendingCommands(2 * static_cast<LONG>(commandTokens.size()));

    FABRIC_URI serviceUri = NULL;
    if (!targetServiceName.empty())
    {
        serviceUri = targetServiceName.c_str();
    }

    for (auto it = commandTokens.begin(); it != commandTokens.end(); ++it)
    {
        wstring commandToken = *it;

        Threadpool::Post(
            [&, commandToken]()
            {
                this->PerformFabricClientOperation(
                    L"InvokeInfrastructureCommand",
                    FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
                    [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
                    {
                        return client->BeginInvokeInfrastructureCommand(
                            serviceUri,
                            commandToken.c_str(),
                            timeout,
                            callback.GetRawPointer(),
                            context.InitializationAddress());
                    },
                    [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
                    {
                        ComPointer<IFabricStringResult> stringResult;
                        auto hrRunCommand = client->EndInvokeInfrastructureCommand(context.GetRawPointer(), stringResult.InitializationAddress());

                        if (SUCCEEDED(hrRunCommand))
                        {
                            TestSession::WriteInfo(TraceSource, "InvokeInfrastructureCommand reply: {0}", stringResult->get_String());

                            wstring expectedResult(wformatString(
                                "[TestInfrastructureCoordinator reply for RunCommandAsync(True,'{0}')]",
                                commandToken));
                            wstring actualResult(stringResult->get_String());

                            TestSession::FailTestIfNot(
                                expectedResult == actualResult,
                                "InvokeInfrastructureCommand returned unexpected result; expected = '{0}', actual = '{1}'",
                                expectedResult,
                                actualResult);
                        }

                        return hrRunCommand;
                    },
                    expectedError);

                if (--pendingCommands == 0)
                {
                    commandsEvent.Set();
                }
            });

        // Test the query command too. The test coordinator will just echo back the command.
        Threadpool::Post(
            [&, commandToken]()
            {
                this->PerformFabricClientOperation(
                    L"InvokeInfrastructureQuery",
                    FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
                    [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
                    {
                        return client->BeginInvokeInfrastructureQuery(
                            serviceUri,
                            commandToken.c_str(),
                            timeout,
                            callback.GetRawPointer(),
                            context.InitializationAddress());
                    },
                    [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
                    {
                        ComPointer<IFabricStringResult> stringResult;
                        auto hrRunCommand = client->EndInvokeInfrastructureQuery(context.GetRawPointer(), stringResult.InitializationAddress());

                        if (SUCCEEDED(hrRunCommand))
                        {
                            TestSession::WriteInfo(TraceSource, "InvokeInfrastructureQuery reply: {0}", stringResult->get_String());

                            wstring expectedResult(wformatString(
                                "[TestInfrastructureCoordinator reply for RunCommandAsync(False,'{0}')]",
                                commandToken));
                            wstring actualResult(stringResult->get_String());

                            TestSession::FailTestIfNot(
                                expectedResult == actualResult,
                                "InvokeInfrastructureQuery returned unexpected result; expected = '{0}', actual = '{1}'",
                                expectedResult,
                                actualResult);
                        }

                        return hrRunCommand;
                    },
                    expectedQueryError);

                if (--pendingCommands == 0)
                {
                    commandsEvent.Set();
                }
            });
    }

    // PerformClientOperation will fail on timeout
    commandsEvent.WaitOne();
}

bool TestFabricClient::CreateNetwork(StringCollection const & params)
{
    CommandLineParser parser(params, 0);

    wstring networkName;
    wstring networkAddressPrefix;
    if (!parser.TryGetString(L"networkName", networkName) || !parser.TryGetString(L"networkAddressPrefix", networkAddressPrefix))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::CreateNetworkCommand);
        return false;
    }

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    // Currently we only support local network. It would be extended when we support federated.
    ScopedHeap heap;
    FABRIC_LOCAL_NETWORK_CONFIGURATION_DESCRIPTION localNetworkConfiguration = { 0 };
    localNetworkConfiguration.NetworkAddressPrefix = heap.AddString(networkAddressPrefix);
    
    FABRIC_LOCAL_NETWORK_DESCRIPTION localNetworkDescription = { 0 };
    localNetworkDescription.NetworkConfiguration = &localNetworkConfiguration;

    FABRIC_NETWORK_DESCRIPTION networkDescription;
    networkDescription.NetworkType = FABRIC_NETWORK_TYPE_LOCAL;
    networkDescription.Value = reinterpret_cast<void*>(&localNetworkDescription);

    TestSession::WriteNoise(TraceSource, "CreateNetwork called with networkName = {0}, networkAddressPrefix = {1}, expectedError = {2}", networkName, networkAddressPrefix, expectedError);

    ComPointer<IFabricNetworkManagementClient> networkClient = this->CreateNetworkClient();

    this->PerformFabricClientOperation(
        L"CreateNetwork",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return networkClient->BeginCreateNetwork(
                networkName.c_str(),
                &networkDescription,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return networkClient->EndCreateNetwork(context.GetRawPointer());
        },
        expectedError,
        FABRIC_E_NAME_ALREADY_EXISTS);

    return true;
}

bool TestFabricClient::DeleteNetwork(StringCollection const & params)
{
    CommandLineParser parser(params, 0);

    wstring networkName;
    if (!parser.TryGetString(L"networkName", networkName))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeleteNetworkCommand);
        return false;
    }

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);
    HRESULT expectedError = ErrorCode(error).ToHResult();

    ScopedHeap heap;
    FABRIC_DELETE_NETWORK_DESCRIPTION deleteNetworkDescription = { 0 };
    deleteNetworkDescription.NetworkName = heap.AddString(networkName);

    TestSession::WriteNoise(TraceSource, "DeleteNetwork called with networkName = {0}, expectedError = {1}", networkName, expectedError);

    ComPointer<IFabricNetworkManagementClient> networkClient = this->CreateNetworkClient();

    this->PerformFabricClientOperation(
        L"DeleteNetwork",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return networkClient->BeginDeleteNetwork(
            &deleteNetworkDescription,
            timeout,
            callback.GetRawPointer(),
            context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return networkClient->EndDeleteNetwork(context.GetRawPointer());
    },
        expectedError,
        FABRIC_E_NETWORK_NOT_FOUND);

    return true;
}

bool TestFabricClient::GetNetwork(StringCollection const & params)
{
    CommandLineParser parser(params, 0);

    wstring networkName;
    if (!parser.TryGetString(L"networkName", networkName))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::GetNetworkCommand);
        return false;
    }

    bool networkExists = parser.GetBool(L"networkExists");
    int expectedResultCount = networkExists ? 1 : 0;

    ScopedHeap heap;
    FABRIC_NETWORK_QUERY_DESCRIPTION networkQueryDescription = { 0 };
    networkQueryDescription.NetworkNameFilter = heap.AddString(networkName);

    TestSession::WriteNoise(TraceSource, "GetNetwork called with networkNameFilter = {0}, networkExists = {1}", networkName, networkExists);

    ComPointer<IFabricNetworkManagementClient> networkClient = this->CreateNetworkClient();

    auto resultPtr = GetNetworkList(L"GetNetwork", networkClient, networkQueryDescription);

    TestSession::FailTestIf(
        (int)resultPtr->get_NetworkList()->Count != expectedResultCount,
        "Expected {0} network in query result, returned {1}",
        expectedResultCount,
        resultPtr->get_NetworkList()->Count);

    // TODO: add options to verify the result items.

    return true;
}

bool TestFabricClient::ShowNetworks(StringCollection const & params)
{
    CommandLineParser parser(params, 0);

    int expectedResultCount;
    parser.TryGetInt(L"expectedCount", expectedResultCount, -1);

    ScopedHeap heap;
    FABRIC_NETWORK_QUERY_DESCRIPTION networkQueryDescription = { 0 };    

    TestSession::WriteNoise(TraceSource, "ShowNetworks called with expectedResultCount = {0}", expectedResultCount);

    ComPointer<IFabricNetworkManagementClient> networkClient = this->CreateNetworkClient();

    auto resultPtr = GetNetworkList(L"ShowNetworks", networkClient, networkQueryDescription);

    if (expectedResultCount != -1)
    {
        TestSession::FailTestIf(
            (int)resultPtr->get_NetworkList()->Count != expectedResultCount,
            "Expected {0} networks in query result, returned {1}",
            expectedResultCount,
            resultPtr->get_NetworkList()->Count);
    }

    for (ULONG i = 0; i < resultPtr->get_NetworkList()->Count; ++i)
    {
        ServiceModel::NetworkInformation networkResultItem;
        networkResultItem.FromPublicApi(resultPtr->get_NetworkList()->Items[i]);
        TestSession::WriteInfo(TraceSource, "[{0}] : {1}", i, networkResultItem);
    }       

    return true;
}

ComPointer<IFabricNetworkManagementClient> TestFabricClient::CreateNetworkClient()
{
    ComPointer<IFabricNetworkManagementClient> networkClient;
    FabricTestFederation & testFederation = FABRICSESSION.FabricDispatcher.Federation;
    auto comFabricClient = TestFabricClient::FabricCreateComFabricClient(testFederation);

    HRESULT hr = comFabricClient->QueryInterface(IID_IFabricNetworkManagementClient, (void**)networkClient.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricNetworkManagementClient");

    return networkClient;    
}

ComPointer<IFabricGetNetworkListResult> TestFabricClient::GetNetworkList(
    __in wstring const & operationName,
    __in ComPointer<IFabricNetworkManagementClient> const & networkClient,
    __in FABRIC_NETWORK_QUERY_DESCRIPTION const & queryDescription) const
{
    ComPointer<IFabricGetNetworkListResult> resultPtr;

    this->PerformFabricClientOperation(
        operationName,
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return networkClient->BeginGetNetworkList(
            &queryDescription,
            timeout,
            callback.GetRawPointer(),
            context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return networkClient->EndGetNetworkList(context.GetRawPointer(), resultPtr.InitializationAddress());
    });

    return resultPtr;
}

void TestFabricClient::DeployServicePackageToNode(
    wstring const & applicationTypeName,
    wstring const & applicationTypeVersion,
    wstring const & serviceManifestName,
    wstring const & nodeName,
    wstring const & nodeId,
    FABRIC_PACKAGE_SHARING_POLICY_LIST const & sharingPolicies,
    vector<wstring> const & expectedInCache,
    vector<wstring> const & expectedShared)
{
    TestSession::WriteNoise(TraceSource, "DeployServicePackageToNode called");

    ComPointer<IFabricApplicationManagementClient4> appClient = CreateFabricApplicationClient4(FABRICSESSION.FabricDispatcher.Federation);

    vector<HRESULT> retryableErrors;
    retryableErrors.push_back(S_OK);

        this->PerformFabricClientOperation(
            L"DeployServicePackageToNode",
            FabricTestSessionConfig::GetConfig().PredeploymentTimeout,
            [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return appClient->BeginDeployServicePackageToNode(
                applicationTypeName.c_str(),
                applicationTypeVersion.c_str(),
                serviceManifestName.c_str(),
                &sharingPolicies,
                nodeName.c_str(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
            [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            HRESULT hr = appClient->EndDeployServicePackageToNode(context.GetRawPointer());
            if (hr != S_OK) { return hr; }
            if (!ValidatePredeployedBinaries(applicationTypeName, serviceManifestName, nodeId, expectedInCache, expectedShared))
            {
                return ErrorCode(ErrorCodeValue::OperationFailed).ToHResult();
            }
            return S_OK;
        },
        retryableErrors);
}

bool TestFabricClient::ValidatePredeployedBinaries(
    wstring const & applicationTypeName,
    wstring const & serviceManifestName,
    wstring const & nodeId,
    vector<wstring> const & expectedInCache,
    vector<wstring> const & expectedShared)
{
    wstring nodeworkingDir = Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestNodesDirectory);
    Path::CombineInPlace(nodeworkingDir, nodeId);
#if !defined(PLATFORM_UNIX)
    wstring storePath = Path::Combine(nodeworkingDir, L"ImageCache\\Store\\");
#else
    wstring storePath = Path::Combine(nodeworkingDir, L"ImageCache/Store/");
#endif
    Path::CombineInPlace(storePath, applicationTypeName);

#if !defined(PLATFORM_UNIX)
    wstring sharedPath = Path::Combine(nodeworkingDir, L"Applications\\__shared\\store");
#else
    wstring sharedPath = Path::Combine(nodeworkingDir, L"Applications/__shared/Store");
#endif
    Path::CombineInPlace(sharedPath, applicationTypeName);

    if (!Directory::Exists(storePath))
    {
        TestSession::WriteWarning(TraceSource, "Path does not exists {0}", nodeworkingDir);
        return false;
    }
    for (auto iter = expectedInCache.begin(); iter != expectedInCache.end(); ++iter)
    {
        wstring packageName = wformatString("{0}.{1}", serviceManifestName, *iter);
        wstring packagePath = Path::Combine(storePath, packageName);
        if (!Directory::Exists(packagePath))
        {
            TestSession::WriteWarning(TraceSource, "Package Path does not exists {0}", packagePath);
            return false;
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "Package Path  exists {0}", packagePath);
        }
    }
    for (auto iter = expectedShared.begin(); iter != expectedShared.end(); ++iter)
    {
        wstring packageName = wformatString("{0}.{1}", serviceManifestName, *iter);
        wstring packagePath = Path::Combine(sharedPath, packageName);
        if (!Directory::Exists(packagePath))
        {
            TestSession::WriteWarning(TraceSource, "Shared Package Path does not exists {0}", packagePath);
            return false;
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "Shared Package Path  exists {0}", packagePath);
        }
    }

    return true;
}

bool TestFabricClient::AddBehavior(Common::StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::AddBehaviorCommand);
        return true;
    }

    wstring name = params[0];
    wstring nodeName = wformatString("nodeid:{0}", params[1]);	// source

    FABRIC_UNRELIABLETRANSPORT_BEHAVIOR behavior;

    wstring matchAll = L"*";											 //   <----------------- I couldn't find a simple way of converting UnreliableTransportSpecification::MatchAll
    wstring destinationNode = matchAll;

    behavior.Destination = ((params.size() < 3 || StringUtility::AreEqualCaseInsensitive(params[2], L"*") ) ? matchAll : destinationNode = wformatString("nodeid:{0}", params[2])).c_str();
    behavior.Action = (params.size() > 3 ? params[3] : matchAll).c_str();
    behavior.ProbabilityToApply = (params.size() > 4 ? Common::Double_Parse(params[4]) : 1.0);
    behavior.DelayInSeconds = (params.size() > 5 && !StringUtility::AreEqualCaseInsensitive(params[5], L"Max") ? Common::Double_Parse(params[5]) : 922337203685.478);     //   <----------------- Is there a better way to do this? TimeSpan MaxValue is in Seconds
    behavior.DelaySpanInSeconds = (params.size() > 6 ? Common::Double_Parse(params[6]) : 0.0);
    behavior.Priority = (params.size() > 7 ? Common::Int32_Parse(params[7]) : 0);
    behavior.ApplyCount = (params.size() > 8 ? Common::Int32_Parse(params[8]) : -1);

    // parse filters if specified. filters can be specified as filters=PartitionID:Guid,ReplicaId:Guid
    CommandLineParser parser(params, 4);
    Common::StringMap filtersMap;
    parser.GetMap(L"filters", filtersMap);

    ScopedHeap heap;
    auto filtersCollection = heap.AddItem<FABRIC_STRING_PAIR_LIST>();
    auto filters = heap.AddArray<FABRIC_STRING_PAIR>(filtersMap.size());

    filtersCollection->Count = static_cast<ULONG>(filters.GetCount());
    filtersCollection->Items = filters.GetRawArray();

    size_t index = 0;
    for (auto it = filtersMap.begin(); it != filtersMap.end(); it++)
    {
        filters[index].Name = heap.AddString(it->first);
        filters[index].Value = heap.AddString(it->second);
        ++index;
    }

    behavior.Filters = filtersCollection.GetRawPointer();

    FabricTestFederation & testFederation = FABRICSESSION.FabricDispatcher.Federation;
    auto comFabricClient = TestFabricClient::FabricCreateClient(testFederation);

    ComPointer<IInternalFabricClusterManagementClient> client;
    HRESULT hr = comFabricClient->QueryInterface(IID_IInternalFabricClusterManagementClient, (void**)client.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IInternalFabricClusterManagementClient");

    this->PerformFabricClientOperation(
        L"AddBehavior2",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return client->BeginAddUnreliableTransportBehavior(
            nodeName.c_str(),
            name.c_str(),
            &behavior,
            timeout,
            callback.GetRawPointer(),
            context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return client->EndAddUnreliableTransportBehavior(context.GetRawPointer());
    });

    return true;
}

bool TestFabricClient::RemoveBehavior(Common::StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RemoveBehaviorCommand);
        return true;
    }

    wstring name = params[0];
    wstring nodeName = wformatString("nodeid:{0}", params[1]);	// source

    FabricTestFederation & testFederation = FABRICSESSION.FabricDispatcher.Federation;
    auto comFabricClient = TestFabricClient::FabricCreateClient(testFederation);

    ComPointer<IInternalFabricClusterManagementClient> client;
    HRESULT hr = comFabricClient->QueryInterface(IID_IInternalFabricClusterManagementClient, (void**)client.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IInternalFabricClusterManagementClient");

    this->PerformFabricClientOperation(
        L"RemoveBehavior2",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return client->BeginRemoveUnreliableTransportBehavior(
            nodeName.c_str(),
            name.c_str(),
            timeout,
            callback.GetRawPointer(),
            context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return client->EndRemoveUnreliableTransportBehavior(context.GetRawPointer());
    });

    return true;
}

bool TestFabricClient::CheckUnreliableTransportIsDisabled(Common::StringCollection const & params)
{
    CommandLineParser parser(params, 0);
    wstring expectedStr;

    if (params.size() < 1 || !parser.TryGetString(L"expected", expectedStr))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::CheckUnreliableTransportIsDisabledCommand);
        return false;
    }

    bool expected = parser.GetBool(L"expected");
    bool isDisabled = UnreliableTransportConfig::GetConfig().IsDisabled();

    TestSession::FailTestIf(expected != isDisabled, "Expecting Unreliable Transport IsDisabled={0} but IsDisabled={1}", expected, isDisabled);

    return true;
}

bool TestFabricClient::CheckTransportBehaviorlist(Common::StringCollection const & params)
{
    if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::CheckTransportBehaviorlist);
        return true;
    }

    CommandLineParser parser(params);
    
    int timeout = 0;
    TestSession::FailTestIf(!parser.TryGetInt(L"timeOut", timeout), "CheckTransportBehaviorlist expected timeout int param");

    wstring node;
    TestSession::FailTestIf(!parser.TryGetString(L"nodeId", node), "CheckTransportBehaviorlist expected nodeid string param");
    wstring nodeName = wformatString("nodeid:{0}", node);	// source

    FabricTestFederation & testFederation = FABRICSESSION.FabricDispatcher.Federation;
    auto comFabricClient = TestFabricClient::FabricCreateClient(testFederation);

    ComPointer<IInternalFabricClusterManagementClient> client;
    HRESULT hr = comFabricClient->QueryInterface(IID_IInternalFabricClusterManagementClient, (void**) client.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IInternalFabricClusterManagementClient");

    ComPointer<IFabricStringListResult> getTransportBehaviorListResult;
    this->PerformFabricClientOperation(
        L"CheckTransportBehaviorlist2",
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return client->BeginGetTransportBehaviorList(
            nodeName.c_str(),
            timeout,
            callback.GetRawPointer(),
            context.InitializationAddress());
    },
        [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return client->EndGetTransportBehaviorList(context.GetRawPointer(), getTransportBehaviorListResult.InitializationAddress());
    });

    Sleep(timeout);

    int expectedResultCount;
    TestSession::FailTestIf(!parser.TryGetInt(L"expectedCount", expectedResultCount), "CheckTransportBehaviorlist expected expectedCount int param");
    ULONG itemCount;
    LPCWSTR *items;
    hr = getTransportBehaviorListResult->GetStrings(&itemCount, (const LPCWSTR**) &items);

    TestSession::WriteInfo(TraceSource, "GetTransportBehaviorList returned {0} results", itemCount);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "CheckTransportBehaviorlist() failed with {0}", hr);
    TestSession::FailTestIf(static_cast<ULONG>(expectedResultCount) != itemCount, "TransportBehaviorList ExpectedCount={0} but ContainsCount={1}", expectedResultCount, itemCount);
    wstring behavior = items[0];
    behavior.erase(std::remove_if(behavior.begin(), behavior.end(), ::isspace), behavior.end());

    wstring expectedBehavior;
    parser.TryGetString(L"expectedBehavior", expectedBehavior);
    TestSession::FailTestIf(expectedBehavior != behavior, "TransportBehaviorList ExpectedCount={0} but ContainsCount={1}", expectedBehavior, behavior);
    return true;
}

bool TestFabricClient::PerformanceTest(Common::StringCollection const & params)
{
    CommandLineParser parser(params, 0);

    int64 variation = 0;
    if (!Common::TryParseInt64(params[0], variation))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::PerformanceTestCommand);
        return false;
    }

    switch (variation)
    {
        case 0:
            return this->PerformanceTestVariation0(params);

        case 1:
            return this->PerformanceTestVariation1(params);

        case 2:
            return this->PerformanceTestVariation2(params);

        case 3:
            return this->PerformanceTestVariation3(params);

        default:
            FABRICSESSION.PrintHelp(FabricTestCommands::PerformanceTestCommand);
            return false;
    }
}

std::vector<NamingUri> TestFabricClient::GetPerformanceTestNames(int count, __in CommandLineParser & parser)
{
    NamingUri unused;
    return GetPerformanceTestNames(count, parser, unused);
}

std::vector<NamingUri> TestFabricClient::GetPerformanceTestNames(int count, __in CommandLineParser & parser, __out NamingUri & baseName)
{
    wstring nameBase;
    parser.TryGetString(L"name", nameBase, L"fabric:/perftest");

    wstring subname;
    parser.TryGetString(L"subname", subname, L"");

    int depthCount;
    parser.TryGetInt(L"depthCount", depthCount, 1);

    bool reverseList = parser.GetBool(L"reverse");

    bool shuffleList = parser.GetBool(L"shuffle");

    int duplicateCount;
    parser.TryGetInt(L"duplicates", duplicateCount, 0);

    NamingUri name;
    bool parsed = NamingUri::TryParse(nameBase, name);
    TestSession::FailTestIfNot(parsed, "Could not parse name '{0}'", nameBase);

    bool differentRoot = parser.GetBool(L"differentroot");

    vector<NamingUri> fullNames;

    for (int ix = 0; ix < count / (duplicateCount + 1); ++ix)
    {
        wstring root;
        NamingUri nameBaseUri = name;
        if (differentRoot)
        {
            root = wformatString("{0}{1}", nameBase, ix);
            TestSession::FailTestIfNot(NamingUri::TryParse(root, nameBaseUri), "Could not parse name '{0}'", root);
        }

        if (subname.empty())
        {
            fullNames.push_back(move(nameBaseUri));
        }
        else
        {
            wstring path;
            StringWriter(path).Write("{0}{1}", subname, ix);
            NamingUri fullName = NamingUri::Combine(nameBaseUri, path);

            for (int jx = 1; jx < depthCount; ++jx)
            {
                fullName = NamingUri::Combine(fullName, path);
            }

            for (int kx = 0; kx <= duplicateCount; ++kx)
            {
                fullNames.push_back(fullName);
            }
        }
    }

    baseName = name;

    if (reverseList)
    {
        reverse(fullNames.begin(), fullNames.end());
    }

    if (shuffleList)
    {
        random_shuffle(fullNames.begin(), fullNames.end());
    }

    return fullNames;
}

vector<PartitionedServiceDescriptor> TestFabricClient::GetPerformanceTestServiceDescriptions(
    int serviceCount,
    __in CommandLineParser & parser,
    wstring const & appName,
    wstring const & serviceType,
    std::vector<NamingUri> const & serviceNames)
{
    int partitionCount;
    parser.TryGetInt(L"partitionCount", partitionCount, 1);

    int replicaCount;
    parser.TryGetInt(L"replicaCount", replicaCount, 1);

    vector<PartitionedServiceDescriptor> psds;
    for (int ix = 0; ix < serviceCount; ++ix)
    {
        NamingUri const & serviceName = serviceNames[ix];

        ServiceDescription serviceDescription(
            serviceName.ToString(),
            0,              // instance
            0,              // updateVersion
            partitionCount,
            replicaCount,   // targetReplicaSetSize
            1,              // minReplicaSetSize
            false,          // isStateful
            false,          // hasPersistedState
            TimeSpan::Zero, // replicaRestartWaitDuration
            TimeSpan::MaxValue, // quorumLossWaitDuration
            TimeSpan::MinValue, // standByReplicaKeepDuration
            ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(), serviceType),
            vector<ServiceCorrelationDescription>(),
            L"",            // placement constraints
            0,              // scaleoutCount
            vector<ServiceLoadMetricDescription>(),
            0,              // defaultMoveCost
            vector<byte>(), // initializationData
            appName);       // applicationName

        vector<ConsistencyUnitDescription> cuids;
        for (int jx = 0; jx < partitionCount; ++jx)
        {
            cuids.push_back(ConsistencyUnitDescription());
        }

        PartitionedServiceDescriptor psd;
        ErrorCode error = PartitionedServiceDescriptor::Create(serviceDescription, cuids, psd);

        psds.push_back(move(psd));
    }

    return psds;
}

bool TestFabricClient::PerformanceTestVariation0(StringCollection const & params)
{
    CommandLineParser parser(params, 1);

    int nameCount;
    parser.TryGetInt(L"nameCount", nameCount, 100);

    bool deleteNames = parser.GetBool(L"delete");

    vector<NamingUri> fullNames = GetPerformanceTestNames(nameCount, parser);

    Common::atomic_long failures(0);
    Common::atomic_long pendingCount(nameCount);
    Stopwatch stopwatch;
    ManualResetEvent doneEvent(false);

    wstring clientName;
    parser.TryGetString(L"client", clientName, L"");

    ComPointer<IFabricPropertyManagementClient> propertyClient;
    if (!FABRICSESSION.FabricDispatcher.TryGetNamedFabricClient(clientName, IID_IFabricPropertyManagementClient, propertyClient))
    {
        propertyClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);
    }

    stopwatch.Start();

    for (int ix = 0; ix < nameCount; ++ix)
    {
        NamingUri fullName = fullNames[ix];

        auto callback = make_com<ComCallbackWrapper>([&](__in IFabricAsyncOperationContext * context)
            {
                HRESULT hr = S_OK;
                if (deleteNames)
                {
                    hr = propertyClient->EndDeleteName(context);
                }
                else
                {
                    hr = propertyClient->EndCreateName(context);
                }

                if (!SUCCEEDED(hr))
                {
                    ++failures;
                }

                if (--pendingCount == 0)
                {
                    doneEvent.Set();
                }
            });

        ComPointer<IFabricAsyncOperationContext> context;

        HRESULT hr = S_OK;
        if (deleteNames)
        {
           hr = propertyClient->BeginDeleteName(
                fullName.ToString().c_str(),
                static_cast<DWORD>(FabricTestSessionConfig::GetConfig().NamingOperationTimeout.TotalMilliseconds()),
                callback.GetRawPointer(),
                context.InitializationAddress());
        }
        else
        {
           hr = propertyClient->BeginCreateName(
                fullName.ToString().c_str(),
                static_cast<DWORD>(FabricTestSessionConfig::GetConfig().NamingOperationTimeout.TotalMilliseconds()),
                callback.GetRawPointer(),
                context.InitializationAddress());
        }

        if (!SUCCEEDED(hr))
        {
            ++failures;

            if (--pendingCount == 0)
            {
                doneEvent.Set();
            }
        }
    }

    doneEvent.WaitOne();

    stopwatch.Stop();

    double throughput = static_cast<double>(nameCount) / stopwatch.ElapsedMilliseconds;

    wstring resultMessage;
    StringWriter(resultMessage).Write(
        "PerformanceTestVariation0 results: count = {0} failures = {1} elapsed = {2} write/sec = {3}",
        nameCount,
        failures.load(),
        stopwatch.ElapsedMilliseconds,
        throughput * 1000);

    if (failures.load() == 0)
    {
        TestSession::WriteInfo(TraceSource, "{0}", resultMessage);
    }
    else
    {
        TestSession::WriteError(TraceSource, "{0}", resultMessage);
    }

    return true;
}

bool TestFabricClient::PerformanceTestVariation1(StringCollection const & params)
{
    CommandLineParser parser(params, 1);

    int propertiesCount;
    parser.TryGetInt(L"propertiesCount", propertiesCount, 100);

    int propertiesSize;
    parser.TryGetInt(L"propertiesSize", propertiesSize, 128);

    bool deleteProperties = parser.GetBool(L"delete");

    vector<NamingUri> fullNames = GetPerformanceTestNames(propertiesCount, parser);

    vector<BYTE> bytes;
    for (int64 ix = 0; ix < propertiesSize; ++ix)
    {
        bytes.push_back(static_cast<byte>(ix));
    }

    Common::atomic_long failures(0);
    Common::atomic_long pendingProperties(propertiesCount);
    Stopwatch stopwatch;
    ManualResetEvent doneEvent(false);

    wstring clientName;
    parser.TryGetString(L"client", clientName, L"");

    ComPointer<IFabricPropertyManagementClient> propertyClient;
    if (!FABRICSESSION.FabricDispatcher.TryGetNamedFabricClient(clientName, IID_IFabricPropertyManagementClient, propertyClient))
    {
        propertyClient = CreateFabricPropertyClient(FABRICSESSION.FabricDispatcher.Federation);
    }

    stopwatch.Start();

    for (int ix = 0; ix < propertiesCount; ++ix)
    {
        NamingUri const & fullName = fullNames[ix];

        wstring propertyName;
        StringWriter(propertyName).Write("myProperty_{0}", ix);

        auto callback = make_com<ComCallbackWrapper>([&, fullName, propertyName](__in IFabricAsyncOperationContext * context)
            {
                HRESULT hr = S_OK;
                if (deleteProperties)
                {
                    hr = propertyClient->EndDeleteProperty(context);
                }
                else
                {
                    hr = propertyClient->EndPutPropertyBinary(context);
                }

                if (SUCCEEDED(hr))
                {
                    FABRICSESSION.FabricDispatcher.NamingState.AddProperty(
                        fullName,
                        propertyName,
                        FABRIC_PROPERTY_TYPE_BINARY,
                        bytes,
                        L"" /*customTypeId*/);
                }
                else
                {
                    ++failures;
                }

                if (--pendingProperties == 0)
                {
                    doneEvent.Set();
                }
            });

        ComPointer<IFabricAsyncOperationContext> context;

        HRESULT hr = S_OK;
        if (deleteProperties)
        {
           hr = propertyClient->BeginDeleteProperty(
                fullName.ToString().c_str(),
                propertyName.c_str(),
                static_cast<DWORD>(FabricTestSessionConfig::GetConfig().NamingOperationTimeout.TotalMilliseconds()),
                callback.GetRawPointer(),
                context.InitializationAddress());
        }
        else
        {
           hr = propertyClient->BeginPutPropertyBinary(
                fullName.ToString().c_str(),
                propertyName.c_str(),
                static_cast<int>(bytes.size()),
                bytes.data(),
                static_cast<DWORD>(FabricTestSessionConfig::GetConfig().NamingOperationTimeout.TotalMilliseconds()),
                callback.GetRawPointer(),
                context.InitializationAddress());
        }

        if (!SUCCEEDED(hr))
        {
            ++failures;

            if (--pendingProperties == 0)
            {
                doneEvent.Set();
            }
        }
    }

    doneEvent.WaitOne();

    stopwatch.Stop();

    double throughput = static_cast<double>(propertiesCount) / stopwatch.ElapsedMilliseconds;

    wstring resultMessage;
    StringWriter(resultMessage).Write(
        "PerformanceTestVariation1 results: count = {0} failures = {1} size = {2} elapsed = {3} write/sec = {4}",
        propertiesCount,
        failures.load(),
        propertiesSize,
        stopwatch.ElapsedMilliseconds,
        throughput * 1000);

    if (failures.load() == 0)
    {
        TestSession::WriteInfo(TraceSource, "{0}", resultMessage);
    }
    else
    {
        TestSession::WriteError(TraceSource, "{0}", resultMessage);
    }

    return true;
}

bool TestFabricClient::PerformanceTestVariation2(StringCollection const & params)
{
    CommandLineParser parser(params, 1);

    int serviceCount;
    parser.TryGetInt(L"serviceCount", serviceCount, 1000);

    bool deleteServices = parser.GetBool(L"delete");

    vector<NamingUri> fullNames = GetPerformanceTestNames(serviceCount, parser);
    vector<PartitionedServiceDescriptor> psds = GetPerformanceTestServiceDescriptions(serviceCount, parser, L"", L"CalculatorServiceType", fullNames);

    Common::atomic_long failures(0);
    Common::atomic_long pendingServices(serviceCount);
    Stopwatch stopwatch;
    ManualResetEvent doneEvent(false);

    vector<wstring> connectionStrings;
    connectionStrings.push_back(FABRICSESSION.FabricDispatcher.Federation.GetEntreeServiceAddress());

    shared_ptr<FabricClientImpl> fabricClientSPtr = make_shared<FabricClientImpl>(std::move(connectionStrings));
    fabricClientSPtr->SetSecurity(SecuritySettings(FABRICSESSION.FabricDispatcher.FabricClientSecurity));

    stopwatch.Start();

    for (int ix = 0; ix < serviceCount; ++ix)
    {
        if (deleteServices)
        {
            NamingUri const & serviceName = fullNames[ix];

            fabricClientSPtr->BeginInternalDeleteService(
                ServiceModel::DeleteServiceDescription(serviceName),
                ActivityId(Guid::NewGuid()),
                FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
                [&, serviceName](AsyncOperationSPtr const & operation)
                {
                    bool unused;
                    ErrorCode error = fabricClientSPtr->EndInternalDeleteService(operation, unused);

                    if (error.IsSuccess())
                    {
                        FABRICSESSION.FabricDispatcher.ServiceMap.MarkServiceAsDeleted(serviceName.ToString());
                    }
                    else
                    {
                        ++failures;
                    }

                    if (--pendingServices == 0)
                    {
                        doneEvent.Set();
                    }
                },
                fabricClientSPtr->CreateAsyncOperationRoot());
        }
        else
        {
            PartitionedServiceDescriptor const & psd = psds[ix];

            fabricClientSPtr->BeginInternalCreateService(
                psd,
                ServiceModel::ServicePackageVersion(),
                0,
                FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
                [&, psd](AsyncOperationSPtr const & operation)
                {
                    ErrorCode error = fabricClientSPtr->EndInternalCreateService(operation);

                    if (error.IsSuccess())
                    {
                        PartitionedServiceDescriptor copyPsd = psd;

                        FABRICSESSION.FabricDispatcher.ServiceMap.AddPartitionedServiceDescriptor(
                            psd.Service.Name,
                            move(copyPsd),
                            true);
                    }
                    else
                    {
                        ++failures;
                    }

                    if (--pendingServices == 0)
                    {
                        doneEvent.Set();
                    }
                },
                fabricClientSPtr->CreateAsyncOperationRoot());
        }
    }

    doneEvent.WaitOne();

    TestSession::FailTestIfNot(fabricClientSPtr->Close().IsSuccess(), "fabricClientSPtr->Close failed");

    stopwatch.Stop();

    double throughput = static_cast<double>(serviceCount) / stopwatch.ElapsedMilliseconds;

    wstring resultMessage;
    StringWriter(resultMessage).Write(
        "PerformanceTestVariation2 results: count = {0} failures = {1} elapsed = {2} req/sec = {3}",
        serviceCount,
        failures.load(),
        stopwatch.ElapsedMilliseconds,
        throughput * 1000);

    if (failures.load() == 0)
    {
        TestSession::WriteInfo(TraceSource, "{0}", resultMessage);
    }
    else
    {
        TestSession::WriteError(TraceSource, "{0}", resultMessage);
    }

    return true;
}

bool TestFabricClient::PerformanceTestVariation3(StringCollection const & params)
{
    CommandLineParser parser(params, 1);

    int serviceCount;
    parser.TryGetInt(L"serviceCount", serviceCount, 1000);

    wstring serviceType;
    parser.TryGetString(L"serviceType", serviceType, L"UndefinedServiceType");

    NamingUri appName;
    vector<NamingUri> fullNames = GetPerformanceTestNames(serviceCount, parser, appName);

    wstring appId;
    parser.TryGetString(L"appId", appId, L"UndefinedAppId");

    wstring servicePackage;
    parser.TryGetString(L"servicePackage", servicePackage, L"UndefinedServicePackage");

    bool overridePlacementConstraints = parser.GetBool(L"xconstraint");

    vector<PartitionedServiceDescriptor> psds = GetPerformanceTestServiceDescriptions(serviceCount, parser, appName.ToString(), serviceType, fullNames);

    ScopedHeap heap;
    vector<FABRIC_SERVICE_DESCRIPTION> serviceDescriptions;
    serviceDescriptions.resize(psds.size());
    for (size_t ix = 0; ix < psds.size(); ++ix)
    {
        psds[ix].ToPublicApi(heap, serviceDescriptions[ix]);

        if (overridePlacementConstraints)
        {
            reinterpret_cast<FABRIC_STATELESS_SERVICE_DESCRIPTION*>(serviceDescriptions[ix].Value)->PlacementConstraints = NULL;
        }
    }

    Common::atomic_long failures(0);
    Common::atomic_long pendingServices(serviceCount);
    Stopwatch stopwatch;
    ManualResetEvent doneEvent(false);

    ComPointer<IFabricServiceManagementClient> serviceClient = CreateFabricServiceClient(FABRICSESSION.FabricDispatcher.Federation);

    stopwatch.Start();

    for (size_t ix = 0; ix < psds.size(); ++ix)
    {
        PartitionedServiceDescriptor psd = psds[ix];

        auto callback = make_com<ComCallbackWrapper>([&, psd](__in IFabricAsyncOperationContext * context)
            {
                HRESULT hr = serviceClient->EndCreateService(context);

                if (SUCCEEDED(hr))
                {
                    PartitionedServiceDescriptor copyPsd = psd;

                    const_cast<ServiceModel::ServiceTypeIdentifier &>(copyPsd.Service.Type) =
                        ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(appId, servicePackage), serviceType),

                    FABRICSESSION.FabricDispatcher.ServiceMap.AddPartitionedServiceDescriptor(
                        psd.Service.Name,
                        move(copyPsd),
                        true);
                }
                else
                {
                    ++failures;
                }

                if (--pendingServices == 0)
                {
                    doneEvent.Set();
                }
            });

        ComPointer<IFabricAsyncOperationContext> context;

        HRESULT hr = serviceClient->BeginCreateService(
            &serviceDescriptions[ix],
            static_cast<DWORD>(FabricTestSessionConfig::GetConfig().NamingOperationTimeout.TotalMilliseconds()),
            callback.GetRawPointer(),
            context.InitializationAddress());

        if (!SUCCEEDED(hr))
        {
            ++failures;

            if (--pendingServices == 0)
            {
                doneEvent.Set();
            }
        }
    }

    doneEvent.WaitOne();

    stopwatch.Stop();

    double createThroughput = static_cast<double>(serviceCount) / stopwatch.ElapsedMilliseconds;

    wstring resultMessage;
    StringWriter(resultMessage).Write(
        "PerformanceTestVariation3 results (CreateService): count = {0} failures = {1} elapsed = {2} req/sec = {3}",
        serviceCount,
        failures.load(),
        stopwatch.ElapsedMilliseconds,
        createThroughput * 1000);

    if (failures.load() == 0)
    {
        TestSession::WriteInfo(TraceSource, "{0}", resultMessage);
    }
    else
    {
        TestSession::WriteError(TraceSource, "{0}", resultMessage);
    }

    bool deleteServices = parser.GetBool(L"delete");

    if (deleteServices)
    {
        pendingServices.store(static_cast<LONG>(psds.size()));
        failures.store(0);
        stopwatch.Reset();
        doneEvent.Reset();

        stopwatch.Start();

        for (size_t ix = 0; ix < psds.size(); ++ix)
        {
            PartitionedServiceDescriptor psd = psds[ix];

            auto callback = make_com<ComCallbackWrapper>([&, psd](__in IFabricAsyncOperationContext * context)
                {
                    HRESULT hr = serviceClient->EndDeleteService(context);

                    if (SUCCEEDED(hr))
                    {
                        FABRICSESSION.FabricDispatcher.ServiceMap.MarkServiceAsDeleted(psd.Service.Name);
                    }
                    else
                    {
                        ++failures;
                    }

                    if (--pendingServices == 0)
                    {
                        doneEvent.Set();
                    }
                });

            ComPointer<IFabricAsyncOperationContext> context;

            HRESULT hr = serviceClient->BeginDeleteService(
                psd.Service.Name.c_str(),
                static_cast<DWORD>(FabricTestSessionConfig::GetConfig().NamingOperationTimeout.TotalMilliseconds()),
                callback.GetRawPointer(),
                context.InitializationAddress());

            if (!SUCCEEDED(hr))
            {
                ++failures;

                if (--pendingServices == 0)
                {
                    doneEvent.Set();
                }
            }
        }

        doneEvent.WaitOne();

        stopwatch.Stop();

        double deleteThroughput = static_cast<double>(serviceCount) / stopwatch.ElapsedMilliseconds;

        resultMessage.clear();
        StringWriter(resultMessage).Write(
            "PerformanceTestVariation3 results (DeleteService): count = {0} failures = {1} elapsed = {2} req/sec = {3}",
            serviceCount,
            failures.load(),
            stopwatch.ElapsedMilliseconds,
            deleteThroughput * 1000);

        if (failures.load() == 0)
        {
            TestSession::WriteInfo(TraceSource, "{0}", resultMessage);
        }
        else
        {
            TestSession::WriteError(TraceSource, "{0}", resultMessage);
        }
    }

    return true;
}

bool TestFabricClient::GetServiceMetrics(StringCollection const& metricCollection, vector<ServiceLoadMetricDescription> & metrics, bool isStateful)
{
    UNREFERENCED_PARAMETER(isStateful);
    size_t metricCount = metricCollection.size();
    if (metricCount < 4 || (metricCount % 4) != 0)
    {
        TestSession::WriteError(
            TraceSource,
            "Incorrect number of arguments provided to service metrics.");
        return false;
    }

    for(size_t i = 0; i < metricCount; i += 4)
    {
        wstring name = metricCollection[i];

        auto weightEnum = ServiceModel::WeightType::GetWeightType(metricCollection[i+1]);

        FABRIC_SERVICE_LOAD_METRIC_WEIGHT weight = ServiceModel::WeightType::ToPublicApi(weightEnum);

        int64 primaryDefaultLoad = Common::Int64_Parse(metricCollection[i+2]);
        if (primaryDefaultLoad < 0 || primaryDefaultLoad > UINT_MAX)
        {
            TestSession::WriteError(
                TraceSource,
                "Invalid primary default load {0}", metricCollection[i+2]);
            return false;
        }

        int64 secondaryDefaultLoad = Common::Int64_Parse(metricCollection[i+3]);
        if (secondaryDefaultLoad < 0 || secondaryDefaultLoad > UINT_MAX)
        {
            TestSession::WriteError(
                TraceSource,
                "Invalid secondary default load {0}", metricCollection[i+3]);
            return false;
        }

        metrics.push_back(ServiceLoadMetricDescription(name, weight, static_cast<uint>(primaryDefaultLoad), static_cast<uint>(secondaryDefaultLoad)));
    }

    return true;
}

bool TestFabricClient::GetCorrelations(StringCollection const& correlationCollection, vector<ServiceCorrelationDescription> & serviceCorrelations)
{
    size_t count = correlationCollection.size();
    if (count < 2 || (count % 2) != 0)
    {
        TestSession::WriteError(
            TraceSource,
            "Incorrect number of arguments provided to service correlations.");
        return false;
    }

    for(size_t i = 0; i < count; i += 2)
    {
        FABRIC_SERVICE_CORRELATION_SCHEME scheme;
        if (correlationCollection[i+1] == L"affinity")
        {
            scheme = FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY;
        }
        else if (correlationCollection[i+1] == L"alignedAffinity")
        {
            scheme = FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY;
        }
        else if (correlationCollection[i+1] == L"nonAlignedAffinity")
        {
            scheme = FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY;
        }
        else
        {
            TestSession::WriteError(
                TraceSource,
                "Invalid scheme {0}", correlationCollection[i+1]);
            return false;
        }

        serviceCorrelations.push_back(ServiceCorrelationDescription(correlationCollection[i], scheme));
    }

    return true;
}


bool TestFabricClient::GetPlacementPolicies(StringCollection const& policiesCollection, vector<ServiceModel::ServicePlacementPolicyDescription> & placementPolicies)
{
    size_t count = policiesCollection.size();
    if (count < 2 || (count % 2) != 0)
    {
        TestSession::WriteError(
            TraceSource,
            "Incorrect number of arguments provided to service placement policies.");
        return false;
    }

    for(size_t i = 0; i < count; i += 2)
    {
        FABRIC_PLACEMENT_POLICY_TYPE type;
        if (policiesCollection[i+1] == L"invalidDomain")
        {
            type = FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN;
        }
        else if (policiesCollection[i+1] == L"requiredDomain")
        {
            type = FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN;
        }
        else if (policiesCollection[i+1] == L"preferredPrimaryDomain")
        {
            type = FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN;
        }
        else if (policiesCollection[i+1] == L"requiredDomainDistribution")
        {
            type = FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION;
        }
        else if (policiesCollection[i+1] == L"nonPartiallyPlaceService")
        {
            type = FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE;
        }
        else
        {
            TestSession::WriteError(
                TraceSource,
                "Invalid placement policy type {0}", policiesCollection[i+1]);
            return false;
        }

        placementPolicies.push_back(ServiceModel::ServicePlacementPolicyDescription(policiesCollection[i], type));
    }

    return true;
}

bool TestFabricClient::GetServiceScalingPolicy(std::wstring const & scalingCollection, vector<Reliability::ServiceScalingPolicyDescription> & scalingPolicy)
{
    std::vector<wstring> scalingElements;
    StringUtility::Split<wstring>(scalingCollection, scalingElements, L",", true);

    std::vector<wstring> scalingKind;
    StringUtility::Split<wstring>(scalingElements[0], scalingKind, L":", true);

    if (scalingKind.size() != 2)
    {
        TestSession::WriteError(
            TraceSource,
            "Bad scaling policy kind");
        return false;
    }

    // Used for trigger
    wstring metricName;
    double minLoad = -1.0;
    double maxLoad = -1.0;
    int scaleInterval = -1;
    bool useOnlyPrimaryLoad = false;
    // used for mechanism
    int minCount = -2;
    int maxCount = -2;
    int increment = -2;

    for (int i = 1; i < scalingElements.size(); ++i)
    {
        std::vector<wstring> scalingData;
        StringUtility::Split<wstring>(scalingElements[i], scalingData, L":", true);
        if (StringUtility::AreEqualCaseInsensitive(scalingData[0], L"metricname"))
        {
            metricName = scalingData[1];
        }
        else if (StringUtility::AreEqualCaseInsensitive(scalingData[0], L"minCount"))
        {
            int64 minInstanceCount;
            if (Common::TryParseInt64(scalingData[1], minInstanceCount))
            {
                minCount = (int)minInstanceCount;
            }
            else
            {
                TestSession::WriteError(
                    TraceSource,
                    "Bad min count");
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(scalingData[0], L"maxcount"))
        {
            int64 maxInstanceCount;
            if (Common::TryParseInt64(scalingData[1], maxInstanceCount))
            {
                maxCount = (int)maxInstanceCount;
            }
            else
            {
                TestSession::WriteError(
                    TraceSource,
                    "Bad max count");
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(scalingData[0], L"metriclow"))
        {
            double metricLow;
            if (Common::TryParseDouble(scalingData[1], metricLow))
            {
                minLoad = metricLow;
            }
            else
            {
                TestSession::WriteError(
                    TraceSource,
                    "Bad metric limit low");
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(scalingData[0], L"metrichigh"))
        {
            double metricHigh;
            if (Common::TryParseDouble(scalingData[1], metricHigh))
            {
                maxLoad = metricHigh;
            }
            else
            {
                TestSession::WriteError(
                    TraceSource,
                    "Bad metric limit high");
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(scalingData[0], L"scaleinterval"))
        {
            uint64 parsedScaleInterval;
            if (Common::TryParseUInt64(scalingData[1], parsedScaleInterval))
            {
                scaleInterval = (int)parsedScaleInterval;
            }
            else
            {
                TestSession::WriteError(
                    TraceSource,
                    "Bad scale interval");
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(scalingData[0], L"scaleincrement"))
        {
            int64 scaleIncrement;
            if (Common::TryParseInt64(scalingData[1], scaleIncrement))
            {
                increment = (int)scaleIncrement;
            }
            else
            {
                TestSession::WriteError(
                    TraceSource,
                    "Bad scale interval");
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(scalingData[0], L"useonlyprimaryload"))
        {
            if (scalingData[1].compare( L"true") == 0)
            {
                useOnlyPrimaryLoad = true;
            }
            else
            {
                useOnlyPrimaryLoad = false;
            }
        }
        else
        {
            TestSession::WriteError(
                TraceSource,
                "Bad argument for average load scaling");
            return false;
        }
    }


    if (StringUtility::AreEqualCaseInsensitive(scalingKind[1], L"instance"))
    {
        Reliability::ScalingMechanismSPtr mechanism = make_shared<Reliability::InstanceCountScalingMechanism>(
            minCount,
            maxCount,
            increment);
        Reliability::ScalingTriggerSPtr trigger = make_shared<Reliability::AveragePartitionLoadScalingTrigger>(
            metricName,
            minLoad,
            maxLoad,
            scaleInterval);
        scalingPolicy.push_back(Reliability::ServiceScalingPolicyDescription(trigger, mechanism));

        return true;
    }
    else if (StringUtility::AreEqualCaseInsensitive(scalingKind[1], L"partition"))
    {
        Reliability::ScalingMechanismSPtr mechanism = make_shared<Reliability::AddRemoveIncrementalNamedPartitionScalingMechanism>(
            minCount,
            maxCount,
            increment);
        Reliability::ScalingTriggerSPtr trigger = make_shared<Reliability::AverageServiceLoadScalingTrigger>(
            metricName,
            minLoad,
            maxLoad,
            scaleInterval, 
            useOnlyPrimaryLoad);
        scalingPolicy.push_back(Reliability::ServiceScalingPolicyDescription(trigger, mechanism));

        return true;
    }
    else
    {
        TestSession::WriteError(
            TraceSource,
            "Bad scaling policy kind");
        return false;
    }

    return true;
}

// COM helper implementations

IClientFactoryPtr TestFabricClient::FabricCreateNativeClientFactory(__in FabricTestFederation &testFederation)
{
    vector<wstring> addresses;
    testFederation.GetEntreeServiceAddress(addresses);

    vector<LPCWSTR> connectionStrings;
    for (auto it = addresses.begin(); it != addresses.end(); ++it)
    {
        connectionStrings.push_back(it->c_str());
    }

    IClientFactoryPtr factoryPtr;
    auto error = Client::ClientFactory::CreateClientFactory(
        static_cast<USHORT>(connectionStrings.size()),
        connectionStrings.data(),
        factoryPtr);

    TestSession::FailTestIfNot(error.IsSuccess(), "Could not create fabric client factory to {0}. error = {1}", connectionStrings[0], error);

    //
    // Set security creds before giving out the factory pointer
    //
    auto credentials = FABRICSESSION.FabricDispatcher.FabricClientSecurity;
    IClientSettingsPtr settingsClient;
    error = factoryPtr->CreateSettingsClient(settingsClient);
    ASSERT_IFNOT(error.IsSuccess(), "Could not create settings client. error = {0}", error);

    error = settingsClient->SetSecurity(move(credentials));
    TestSession::FailTestIfNot(error.IsSuccess(), "SetSecurityCredentials failed with error = {0}", error);

    return factoryPtr;
}

ComPointer<Api::ComFabricClient> TestFabricClient::FabricCreateComFabricClient(__in FabricTestFederation &testFederation)
{
    vector<wstring> addresses;
    testFederation.GetEntreeServiceAddress(addresses);

    vector<LPCWSTR> connectionStrings;
    for (auto it = addresses.begin(); it != addresses.end(); ++it)
    {
        connectionStrings.push_back(it->c_str());
    }

    // TODO: Temporary workaround
    if (connectionStrings.size() == 0)
    {
        connectionStrings.push_back(L"127.0.0.1:9999");
    }

    IClientFactoryPtr factoryPtr;
    auto error = Client::ClientFactory::CreateClientFactory(
        static_cast<USHORT>(connectionStrings.size()),
        connectionStrings.data(),
        factoryPtr);

    TestSession::FailTestIfNot(error.IsSuccess(), "Could not create fabric client factory to {0}. error = {1}", connectionStrings[0], error);

    IClientSettingsPtr internalSettingsClient;
    error = factoryPtr->CreateSettingsClient(internalSettingsClient);
    TestSession::FailTestIfNot(error.IsSuccess(), "Could not create settings internal client. error = {1}", error);

    //
    // Set security creds before giving out the comfactory pointer
    //
    Transport::SecuritySettings settings = FABRICSESSION.FabricDispatcher.FabricClientSecurity;

    error = internalSettingsClient->SetSecurity(move(settings));
    TestSession::FailTestIfNot(error.IsSuccess(), "SetSecurityCredentials failed with hr = {0}", error);

    ComPointer<Api::ComFabricClient> result;
    auto hr = Api::ComFabricClient::CreateComFabricClient(factoryPtr, result);
    TestSession::FailTestIfNot(!FAILED(hr), "CreateComFabricClient failed", error);

    return result;
}

ComPointer<IFabricClientSettings2> TestFabricClient::FabricCreateClient(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricClientSettings2> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricClientSettings2,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricClientSettings2");

    return result;
}

ComPointer<IFabricPropertyManagementClient> CreateFabricPropertyClient(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricPropertyManagementClient> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricPropertyManagementClient,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricPropertyManagementClient");

    return result;
}

ComPointer<IFabricPropertyManagementClient2> CreateFabricPropertyClient2(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricPropertyManagementClient2> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricPropertyManagementClient2,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricPropertyManagementClient2");

    return result;
}

ComPointer<IFabricServiceManagementClient> CreateFabricServiceClient(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricServiceManagementClient> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricServiceManagementClient,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricServiceManagementClient");

    return result;
}

ComPointer<IFabricServiceManagementClient2> CreateFabricServiceClient2(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricServiceManagementClient2> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricServiceManagementClient2,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricServiceManagementClient2");

    return result;
}

ComPointer<IFabricServiceManagementClient3> CreateFabricServiceClient3(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricServiceManagementClient3> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricServiceManagementClient3,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricServiceManagementClient3");

    return result;
}

ComPointer<IFabricServiceManagementClient4> CreateFabricServiceClient4(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricServiceManagementClient4> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricServiceManagementClient4,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricServiceManagementClient4");

    return result;
}

ComPointer<IFabricServiceManagementClient5> CreateFabricServiceClient5(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricServiceManagementClient5> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricServiceManagementClient5,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricServiceManagementClient5");

    return result;
}

ComPointer<IFabricServiceGroupManagementClient> CreateFabricServiceGroupClient(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricServiceGroupManagementClient> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricServiceGroupManagementClient,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricServiceGroupManagementClient");

    return result;
}

ComPointer<IFabricServiceGroupManagementClient2> CreateFabricServiceGroupClient2(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricServiceGroupManagementClient2> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricServiceGroupManagementClient2,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricServiceGroupManagementClient2");

    return result;
}

ComPointer<IFabricApplicationManagementClient> CreateFabricApplicationClient(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricApplicationManagementClient> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricApplicationManagementClient,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricApplicationManagementClient");

    return result;
}

ComPointer<IFabricApplicationManagementClient4> CreateFabricApplicationClient4(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricApplicationManagementClient4> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricApplicationManagementClient4,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricApplicationManagementClient4");

    return result;
}

ComPointer<IFabricQueryClient10> CreateFabricQueryClient10(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricQueryClient10> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricQueryClient10,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricQueryClient10");

    return result;
}

ComPointer<IInternalFabricServiceManagementClient2> CreateInternalFabricServiceClient2(__in FabricTestFederation & testFederation)
{
    ComPointer<IInternalFabricServiceManagementClient2> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IInternalFabricServiceManagementClient2,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IInternalFabricServiceManagementClient2");

    return result;
}

ComPointer<IFabricClusterManagementClient6> CreateFabricClusterClient(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricClusterManagementClient6> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricClusterManagementClient6,
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricClusterManagementClient6");

    return result;
}

HRESULT TryConvertComResult(
    ComPointer<IFabricResolvedServicePartitionResult> const & comResult,
    __out ResolvedServicePartition & cResult)
{
    FABRIC_RESOLVED_SERVICE_PARTITION const * partition = comResult->get_Partition();

    ULONG count = partition->EndpointCount;

    bool hasPrimary = false;
    bool isStateful = false;
    wstring primary;
    vector<wstring> replicas;

    TestSession::WriteNoise(TraceSource, "Batch count: {0}", count);
    for (ULONG ix = 0; ix < count; ++ix)
    {
        if (partition->Endpoints[ix].Role == FABRIC_SERVICE_ENDPOINT_ROLE::FABRIC_SERVICE_ROLE_STATEFUL_PRIMARY)
        {
            primary = partition->Endpoints[ix].Address;
            isStateful = true;
            hasPrimary = true;
        }
        else
        {
            if (partition->Endpoints[ix].Role == FABRIC_SERVICE_ENDPOINT_ROLE::FABRIC_SERVICE_ROLE_STATEFUL_SECONDARY)
            {
                isStateful = true;
            }

            replicas.push_back(partition->Endpoints[ix].Address);
        }
    }

    cResult = ResolvedServicePartition(
        false, // isServiceGroup
        Reliability::ServiceTableEntry(
            Reliability::ConsistencyUnitId(Common::Guid(Common::ServiceInfoUtility::GetPartitionId(&(partition->Info)))),
            L"fabric:/fabrictest", // this data is lost in the translation
            Reliability::ServiceReplicaSet(isStateful, hasPrimary, move(primary), move(replicas), -1)),
        partition->Info.Kind == FABRIC_SERVICE_PARTITION_KIND::FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE ?
            PartitionInfo(
                (reinterpret_cast<FABRIC_INT64_RANGE_PARTITION_INFORMATION*>(partition->Info.Value))->LowKey,
                (reinterpret_cast<FABRIC_INT64_RANGE_PARTITION_INFORMATION*>(partition->Info.Value))->HighKey) :
            PartitionInfo(),
        GenerationNumber(),
        -1,
        nullptr);

    return S_OK;
}

DWORD GetComTimeout(TimeSpan cTimeout)
{
    DWORD comTimeout;
    HRESULT hr = Int64ToDWord(cTimeout.TotalPositiveMilliseconds(), &comTimeout);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "Could not convert timeout from int64 to DWORD. hr = {0}", hr);

    return comTimeout;
}

vector<BYTE> Serialize(wstring const & input)
{
    int64 count;
    if (Common::TryParseInt64(input, count))
    {
        vector<BYTE> bytes;
        for (int64 ix = 0; ix < count; ++ix)
        {
            bytes.push_back(static_cast<byte>(ix));
        }
        return bytes;
    }
    else
    {
#if !defined(PLATFORM_UNIX)
        vector<BYTE> bytes(1024);
#else
        vector<BYTE> bytes(1024, 0);
#endif
        WideCharToMultiByte(
            CP_ACP,
            0,
            input.c_str(),
            static_cast<int>(input.size()),
            reinterpret_cast<LPSTR>(bytes.data()),
            1024,
            NULL,
            NULL);

        bytes.resize(input.size() * sizeof(wchar_t));
        return bytes;
    }
}

wstring Deserialize(LPBYTE bytes, SIZE_T byteCount)
{
#if !defined(PLATFORM_UNIX)
    WCHAR * buffer = new WCHAR[1024];
#else
    WCHAR * buffer = new WCHAR[1024]();
#endif
    MultiByteToWideChar(
        CP_ACP,
        0,
        reinterpret_cast<LPSTR>(bytes),
        static_cast<int>(byteCount),
        buffer,
        1024);

    wstring result(buffer);
    delete[] buffer;

    return result;
}

bool IsRetryableError(HRESULT hr)
{
    switch(hr)
    {
    case (HRESULT) FABRIC_E_IMAGEBUILDER_TIMEOUT:
    case (HRESULT) FABRIC_E_TIMEOUT:
    case E_ABORT:
    case (HRESULT) FABRIC_E_GATEWAY_NOT_REACHABLE:
    case (HRESULT) FABRIC_E_WRITE_CONFLICT:
        return true;
    default:
        return false;
    }
}
