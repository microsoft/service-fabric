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
using namespace ServiceModel;
using namespace Reliability::FailoverManagerComponent;
using namespace Management::HealthManager;
using namespace Management;
using namespace Api;
using namespace Query;
using namespace Client;
using namespace Transport;
using namespace ClientServerTransport;
using namespace ReliabilityTestApi;
using namespace ReliabilityTestApi::FailoverManagerComponentTestApi;

const StringLiteral TraceSource("FabricTest.FabricClientHealth");

TestFabricClientHealth::TestFabricClientHealth(FabricTestDispatcher & dispatcher)
    : dispatcher_(dispatcher)
    , healthTable_(make_shared<TestHealthTable>())
{
}

void AddReport(__in vector<vector<HealthReport>> & reports, EntityHealthInformationSPtr && entityInfo, AttributeList && attributes)
{
    wstring property;
    Random r;
    auto authorityReports = HealthReport::GetAuthoritySources(entityInfo->Kind);
    TestSession::FailTestIf(authorityReports.empty(), "Authority reports for entity {0} can't be empty", entityInfo);

    auto const & sourceId = authorityReports[0];
    if (r.Next(2) == 0)
    {
        // Generate report with critical priority for some of the entities (the ones that have authority reports with State property)
        property = L"State";
    }
    else
    {
        // Generate report with Higher priority
        property = L"HMLoadTestProperty";
    }

    HealthReport report(
        move(entityInfo),
        sourceId,
        property,
        TimeSpan::MaxValue,
        FABRIC_HEALTH_STATE_OK,
        L"Report from HMLoadTest",
        Common::SequenceNumber::GetNext(),
        false,
        move(attributes));

    if (reports.empty() || (reports.back().size() == static_cast<size_t>(ClientConfig::GetConfig().MaxNumberOfHealthReports)))
    {
        // Create new bucket, or the reports will be dropped at the health client because max number of queued reports is reached
        reports.push_back(vector<HealthReport>());
    }

    reports.back().push_back(move(report));
}

// Takes a batch of reports and breaks them into chunks that are then reported in parallel on the internal health client.
// Both ReportHealth and InternalReportHealth are used for coverage.
void TestFabricClientHealth::ParallelReportHealthBatch(vector<HealthReport> && reports)
{
    Random r;
    vector<vector<HealthReport>> chunks;
    size_t reportCount = reports.size();
    int remainingCount = static_cast<int>(reportCount);
    int i = 0;
    while (remainingCount > 0)
    {
        // Force the last batch to have 1 report by generating less than remainingCount
        int nextCount = (remainingCount == 1) ? 1 : r.Next(1, remainingCount);

        vector<HealthReport> batch;
        for (int j = 0; j < nextCount; ++j, ++i)
        {
            batch.push_back(move(reports[i]));
        }

        chunks.push_back(move(batch));
        remainingCount -= nextCount;
    }

    size_t chunkCount = chunks.size();
    wstring trace;
    StringWriter writer(trace);
    writer.Write("Break batch of {0} reports in {1} chunks added in parallel :", reportCount, chunkCount);
    for (auto const & chunk : chunks)
    {
        writer.Write(" {0}", chunk.size());
    }

    TestSession::WriteInfo(TraceSource, "{0}", trace);

    // Post all add reports to thread pool and then wait for them to be done
    Common::atomic_long parallelBatches(static_cast<long>(chunkCount));
    ManualResetEvent allDone(false);
    for (size_t idx = 0; idx < chunkCount; ++idx)
    {
        Threadpool::Post([&chunks, &parallelBatches, &allDone, this, idx]()
        {
            auto localChunk = move(chunks[idx]);
            for (int retry = 0; retry < FabricTestSessionConfig::GetConfig().QueryOperationRetryCount; ++retry)
            {
                ErrorCode error;
                if (localChunk.size() == 1)
                {
                    // When there is one report, use the overload for 1 report
                    error = this->internalHealthClient_->ReportHealth(move(localChunk[0]), nullptr);
                }
                else
                {
                    error = this->internalHealthClient_->InternalReportHealth(move(localChunk), nullptr);
                }

                if (error.IsSuccess())
                {
                    long previousValue = --parallelBatches;
                    if (previousValue == 0)
                    {
                        allDone.Set();
                    }

                    return;
                }
                else if (error.IsError(ErrorCodeValue::HealthMaxReportsReached))
                {
                    if (retry >= FabricTestSessionConfig::GetConfig().QueryOperationRetryCount - 1)
                    {
                        TestSession::FailTest("Report batch {0} failed with error {1} and retry was exhausted", idx, error);
                    }
                    else
                    {
                        // When multiple reports are passed in to internal report health, some may be accepted. Those must not be passed in again.
                        auto temp = move(localChunk);
                        for (auto && ck : temp)
                        {
                            if (ck.EntityInformation)
                            {
                                localChunk.push_back(move(ck));
                            }
                        }

                        // Sleep and retry
                        TestSession::WriteInfo(TraceSource, "Report batch {0} with {1} items failed with error {2}, remaining {3}, sleep and retry", idx, temp.size(), error, localChunk.size());
                        Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
                        continue;
                    }
                }
                else
                {
                    TestSession::FailTest("Report batch {0} failed with unexpected error {1}", idx, error);
                }
            }
        });
    }

    allDone.WaitOne();
    TestSession::WriteInfo(TraceSource, "Batch of {0} reports in {1} chunks added", reportCount, chunkCount);
}

bool TestFabricClientHealth::HMLoadTest(Common::StringCollection const & params)
{
    // Creates services and performs queries on HM to test how HM is doing under load
    CommandLineParser parser(params, 0);

    int serviceCount;
    parser.TryGetInt(L"serviceCount", serviceCount, 1000);
    TestSession::FailTestIf(serviceCount <= 0, "Service count must be positive, not {0}", serviceCount);

    int partitionCount;
    parser.TryGetInt(L"partitionCount", partitionCount, 3);
    TestSession::FailTestIf(partitionCount <= 0, "Partition count must be positive, not {0}", partitionCount);

    int replicaCount;
    parser.TryGetInt(L"replicaCount", replicaCount, 3);
    TestSession::FailTestIf(replicaCount <= 0, "replica count must be positive, not {0}", replicaCount);

    // Report through internal health client
    CreateInternalFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);

    NamingUri appUri;
    vector<NamingUri> serviceNames = TestFabricClient::GetPerformanceTestNames(serviceCount, parser, appUri);
    wstring appName = appUri.ToString();

    vector<vector<HealthReport>> reports;

    vector<StringCollection> queryArgs;
    wstring expectedHealthStateParam(L"expectedhealthstate=ok");
    wstring retryServiceTooBusy(L"retryableerror=ServiceTooBusy");

    TestSession::WriteInfo(TraceSource, "Create app reports for {0}", appName);
    AttributeList appAttribs;
    appAttribs.AddAttribute(HealthAttributeNames::ApplicationHealthPolicy, ApplicationHealthPolicy::Default->ToString());
    appAttribs.AddAttribute(HealthAttributeNames::ApplicationTypeName, appName + L"Type");
    AddReport(reports, EntityHealthInformation::CreateApplicationEntityHealthInformation(appName, 1), move(appAttribs));
    StringCollection appParams;
    appParams.push_back(L"application");
    appParams.push_back(wformatString("appname={0}", appName));
    appParams.push_back(expectedHealthStateParam);
    appParams.push_back(retryServiceTooBusy);

    queryArgs.push_back(move(appParams));

    TestSession::WriteInfo(
        TraceSource,
        "Create reports for {0} services ({1}, ...), {2} partitions and {3} replicas",
        serviceCount,
        serviceNames[0].ToString(),
        partitionCount * serviceCount,
        partitionCount * serviceCount * replicaCount);

    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    for (size_t ix = 0; ix < serviceNames.size(); ++ix)
    {
        bool isStateful = ix < (serviceNames.size() / 2);
        wstring serviceName = serviceNames[ix].ToString();

        // Set up service queries
        StringCollection serviceParams;
        serviceParams.push_back(L"service");
        serviceParams.push_back(wformatString("servicename={0}", serviceName));
        serviceParams.push_back(expectedHealthStateParam);
        serviceParams.push_back(retryServiceTooBusy);
        queryArgs.push_back(move(serviceParams));

        AttributeList serviceAttribs;
        serviceAttribs.AddAttribute(HealthAttributeNames::ServiceTypeName, L"ServiceTypeName");
        serviceAttribs.AddAttribute(HealthAttributeNames::ApplicationName, appName);
        AddReport(reports, EntityHealthInformation::CreateServiceEntityHealthInformation(serviceName, 1), move(serviceAttribs));

        // Create partitions for each service
        FABRIC_REPLICA_ID replicaId = 23;
        for (int i = 0; i < partitionCount; ++i)
        {
            Guid partitionId = Guid::NewGuid();

            StringCollection partitionParams;
            partitionParams.push_back(L"partition");
            partitionParams.push_back(wformatString("partitionguid={0}", partitionId));
            partitionParams.push_back(expectedHealthStateParam);
            partitionParams.push_back(retryServiceTooBusy);
            queryArgs.push_back(move(partitionParams));

            AttributeList partitionAttribs;
            partitionAttribs.AddAttribute(HealthAttributeNames::ServiceName, serviceName);
            AddReport(reports, EntityHealthInformation::CreatePartitionEntityHealthInformation(partitionId), move(partitionAttribs));

            // Create replicas for each partition
            for (int j = 0; j < replicaCount; ++j)
            {
                StringCollection replicaParams;
                replicaParams.push_back(L"replica");
                replicaParams.push_back(wformatString("partitionguid={0}", partitionId));
                replicaParams.push_back(wformatString("replica.id={0}", replicaId));
                replicaParams.push_back(expectedHealthStateParam);
                replicaParams.push_back(retryServiceTooBusy);
                queryArgs.push_back(move(replicaParams));

                AttributeList replicaAttribs;
                replicaAttribs.AddAttribute(HealthAttributeNames::NodeId, wformatString(nodeId));
                replicaAttribs.AddAttribute(HealthAttributeNames::NodeInstanceId, wformatString(nodeInstanceId));

                auto entityInfo = isStateful
                    ? EntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(partitionId, replicaId, 1)
                    : EntityHealthInformation::CreateStatelessInstanceEntityHealthInformation(partitionId, replicaId);

                AddReport(reports, move(entityInfo), move(replicaAttribs));
                ++replicaId;
            }
        }
    }

    Stopwatch totalDurationStopwatch;

    int maxParallelQueries = 50;
    Common::atomic_long failedQueries(0);
    Common::atomic_long successfulQueries(0);
    ManualResetEvent queryBatchDone(false);
    int queryIndex = 0;

    StringCollection checkClientParams;
    checkClientParams.push_back(L"reportcount=0");

    totalDurationStopwatch.Start();
    for (size_t i = 0; i < reports.size(); ++i)
    {
        size_t currentBatchSize = reports[i].size();
        TestSession::WriteInfo(TraceSource, "Add batch {0} with {1} reports", i, currentBatchSize);
        ParallelReportHealthBatch(move(reports[i]));

        // Send queries for the current processed reports in parallel with the reports
        size_t lastCurrentQueryIndex = queryIndex + currentBatchSize;
        for (; queryIndex < lastCurrentQueryIndex; ++queryIndex)
        {
            TestSession::FailTestIf(queryIndex >= queryArgs.size(), "Query index {0} >= queryArgs.size {1}", queryIndex, queryArgs.size());
            Threadpool::Post([&, queryIndex]()
            {
                if (QueryHealth(queryArgs[queryIndex]))
                {
                    ++successfulQueries;
                }
                else
                {
                    ++failedQueries;
                }

                if (successfulQueries.load() + failedQueries.load() == queryArgs.size())
                {
                    queryBatchDone.Set();
                }
            });

            if (queryIndex % maxParallelQueries == 0)
            {
                Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
            }
        }

        // Wait for the client to drain reports before sending next batch
        TestSession::FailTestIfNot(CheckHealthClient(checkClientParams), "CheckHealthClient failed for batch {0}", i);
    }

    TestSession::FailTestIf(queryIndex != queryArgs.size(), "Query index {0} != queryArgs.size {1}", queryIndex, queryArgs.size());

    queryBatchDone.WaitOne();
    totalDurationStopwatch.Stop();

    TestSession::WriteInfo(
        TraceSource,
        "TotalEntities={0}, max parallel query={1}, query ok/failed {2}/{3}, total duration={4} msec",
        queryArgs.size(),
        maxParallelQueries,
        successfulQueries.load(),
        failedQueries.load(),
        totalDurationStopwatch.ElapsedMilliseconds);

    TestSession::FailTestIfNot(failedQueries.load() == 0, "Not all entities were created/queried successfully");
    TestSession::FailTestIfNot(successfulQueries.load() == queryArgs.size(), "successful queries count {0} doesn't match expected count {1}", successfulQueries.load(), queryArgs.size());

    return true;
}

bool TestFabricClientHealth::HealthPreInitialize(StringCollection const & params)
{
    if (params.size() < 2)
    {
        return false;
    }

    wstring healthKindString = params[0];
    FABRIC_HEALTH_REPORT_KIND kind = GetHealthKind(healthKindString);

    if (kind == FABRIC_HEALTH_REPORT_KIND_INVALID)
    {
        return false;
    }

    CommandLineParser parser(params, 1);

    wstring sourceId;
    bool sourceIdExists = parser.TryGetString(L"sourceid", sourceId);

    if (!sourceIdExists || sourceId.empty())
    {
        TestSession::WriteError(TraceSource, "Invalid sourceid");
        return false;
    }

    CreateInternalFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);

    int64 instance;
    parser.TryGetInt64(L"instance", instance, 0);
    auto error = internalHealthClient_->HealthPreInitialize(sourceId, kind, instance);

    TestSession::FailTestIfNot(error.IsSuccess(), "HealthPreInitialize failed with unexpected error = {0}", error);
    return true;
}

bool TestFabricClientHealth::RunWatchDog(StringCollection const & params)
{
    if (params[0] == L"pause")
    {
        healthTable_->Pause();
        return true;
    }
    else if (params[0] == L"resume")
    {
        healthTable_->Resume();
        return true;
    }

    auto it = watchdogs_.find(params[1]);
    if (params[0] == L"stop")
    {
        if (it == watchdogs_.end())
        {
            TestSession::WriteError(TraceSource, "Watchdog {0} not found", params[1]);
        }
        else
        {
            it->second->Stop();
            watchdogs_.erase(it);
        }
    }
    else if (params[0] == L"start")
    {
        if (it == watchdogs_.end())
        {
            CommandLineParser parser(params, 2);
            double interval;
            parser.TryGetDouble(L"interval", interval, 1.0);
            TestWatchDogSPtr watchDog = make_shared<TestWatchDog>(dispatcher_, params[1], TimeSpan::FromSeconds(interval));
            watchDog->Start(watchDog);
            watchdogs_[params[1]] = move(watchDog);
        }
        else
        {
            TestSession::WriteError(TraceSource, "Watchdog {0} already exists", params[1]);
        }
    }

    return true;
}

bool TestFabricClientHealth::HealthPostInitialize(StringCollection const & params)
{
    if (params.size() < 2)
    {
        return false;
    }

    wstring healthKindString = params[0];
    FABRIC_HEALTH_REPORT_KIND kind = GetHealthKind(healthKindString);

    if (kind == FABRIC_HEALTH_REPORT_KIND_INVALID)
    {
        return false;
    }

    CommandLineParser parser(params, 1);

    wstring sourceId;
    bool sourceIdExists = parser.TryGetString(L"sourceid", sourceId);
    FABRIC_SEQUENCE_NUMBER sequenceNumber, invalidateSequenceNumber;
    parser.TryGetInt64(L"sequencenumber", sequenceNumber, 0);
    parser.TryGetInt64(L"invalidatesequencenumber", invalidateSequenceNumber, FABRIC_INVALID_SEQUENCE_NUMBER);

    if (!sourceIdExists || sourceId.empty())
    {
        TestSession::WriteError(TraceSource, "Invalid sourceid");
        return false;
    }

    CreateInternalFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);

    auto error = internalHealthClient_->HealthPostInitialize(sourceId, kind, sequenceNumber, invalidateSequenceNumber);

    TestSession::FailTestIfNot(error.IsSuccess(), "HealthPostInitialize failed with unexpected error = {0}", error);
    return true;
}

bool TestFabricClientHealth::HealthGetProgress(StringCollection const & params)
{
    if (params.size() < 2)
    {
        return false;
    }

    wstring healthKindString = params[0];
    FABRIC_HEALTH_REPORT_KIND kind = GetHealthKind(healthKindString);

    if (kind == FABRIC_HEALTH_REPORT_KIND_INVALID)
    {
        return false;
    }

    CommandLineParser parser(params, 1);

    wstring sourceId;
    bool sourceIdExists = parser.TryGetString(L"sourceid", sourceId);

    if (!sourceIdExists || sourceId.empty())
    {
        TestSession::WriteError(TraceSource, "Invalid sourceid");
        return false;
    }

    FABRIC_SEQUENCE_NUMBER expectedProgress;
    bool expectedProgressExists = parser.TryGetInt64(L"expectedprogress", expectedProgress);

    CreateInternalFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);

    int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    do
    {
        --remainingRetries;
        FABRIC_SEQUENCE_NUMBER actualProgress;
        ErrorCode error;

        if (sourceId == ServiceModel::Constants::HealthReportFMSource)
        {
            auto fm = this->dispatcher_.GetFM();
            if (!fm)
            {
                error = ErrorCodeValue::FMNotReadyForUse;
            }

            error = fm->HealthClient->HealthGetProgress(sourceId, kind, actualProgress);
        }
        else
        {
            error = internalHealthClient_->HealthGetProgress(sourceId, kind, actualProgress);
        }

        if (!error.IsSuccess())
        {
            TestSession::WriteInfo(TraceSource, "HealthGetProgress failed with error = {0}", error);
        }
        else if (!expectedProgressExists)
        {
            TestSession::WriteInfo(TraceSource, "HealthGetProgress result: {0}", actualProgress);
            return true;
        }
        else
        {
            if (actualProgress != expectedProgress)
            {
                TestSession::WriteInfo(TraceSource, "HealthGetProgress actualprogress {0} does not match expectedprogress {1}", actualProgress, expectedProgress);
            }
            else
            {
                return true;
            }
        }

        Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
    } while (remainingRetries >= 0);

    TestSession::FailTest("HealthGetProgress failed");
}

bool TestFabricClientHealth::HealthSkipSequence(StringCollection const & params)
{
    if (params.size() < 2)
    {
        return false;
    }

    wstring healthKindString = params[0];
    FABRIC_HEALTH_REPORT_KIND kind = GetHealthKind(healthKindString);

    if (kind == FABRIC_HEALTH_REPORT_KIND_INVALID)
    {
        return false;
    }

    CommandLineParser parser(params, 1);

    wstring sourceId;
    bool sourceIdExists = parser.TryGetString(L"sourceid", sourceId);
    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    parser.TryGetInt64(L"sequencenumber", sequenceNumber, 0);

    if (!sourceIdExists || sourceId.empty())
    {
        TestSession::WriteError(TraceSource, "Invalid sourceid");
        return false;
    }

    CreateInternalFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);

    auto error = internalHealthClient_->HealthSkipSequence(sourceId, kind, sequenceNumber);

    TestSession::FailTestIfNot(error.IsSuccess(), "HealthSkipSequence failed with unexpected error = {0}", error);
    return true;
}

bool TestFabricClientHealth::ResetHealthClient(StringCollection const & params)
{
    UNREFERENCED_PARAMETER(params);
    if (healthClient_.GetRawPointer() != nullptr)
    {
        healthClient_.Release();
    }

    if (internalHealthClient_.get() != NULL)
    {
        IHealthClientPtr cleanup;
        cleanup = move(internalHealthClient_);
    }

    return true;
}

bool TestFabricClientHealth::CheckHResult(HRESULT hr, vector<HRESULT> const & expectedErrors)
{
    auto actualError = ErrorCode::FromHResult(hr);
    for (auto it = expectedErrors.begin(); it != expectedErrors.end(); ++it)
    {
        if (hr == *it)
        {
            TestSession::WriteWarning(TraceSource, "Operation failed with expected error: {0}", actualError);
            return true;
        }
    }

    TestSession::WriteWarning(TraceSource, "Operation failed with unexpected error: {0}", actualError);
    return false;
}

FABRIC_HEALTH_REPORT_KIND TestFabricClientHealth::GetHealthKind(wstring & healthKindString)
{
    if (StringUtility::AreEqualCaseInsensitive(healthKindString, L"node"))
    {
        return FABRIC_HEALTH_REPORT_KIND_NODE;
    }
    else if (StringUtility::AreEqualCaseInsensitive(healthKindString, L"cluster"))
    {
        return FABRIC_HEALTH_REPORT_KIND_CLUSTER;
    }
    else if (StringUtility::AreEqualCaseInsensitive(healthKindString, L"partition"))
    {
        return FABRIC_HEALTH_REPORT_KIND_PARTITION;
    }
    else if (StringUtility::AreEqualCaseInsensitive(healthKindString, L"replica"))
    {
        return FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA;
    }
    else if (StringUtility::AreEqualCaseInsensitive(healthKindString, L"instance"))
    {
        return FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE;
    }
    if (StringUtility::AreEqualCaseInsensitive(healthKindString, L"deployedapplication"))
    {
        return FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION;
    }
    if (StringUtility::AreEqualCaseInsensitive(healthKindString, L"deployedservicepackage"))
    {
        return FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE;
    }
    if (StringUtility::AreEqualCaseInsensitive(healthKindString, L"application"))
    {
        return FABRIC_HEALTH_REPORT_KIND_APPLICATION;
    }
    if (StringUtility::AreEqualCaseInsensitive(healthKindString, L"service"))
    {
        return FABRIC_HEALTH_REPORT_KIND_SERVICE;
    }
    TestSession::WriteError(TraceSource, "Unknown type {0}", healthKindString);
    return FABRIC_HEALTH_REPORT_KIND_INVALID;
}

std::wstring TestFabricClientHealth::GetEntityHealthBaseDetails(EntityHealthBase const & entityBase)
{
    wstring trace;
    StringWriter writer(trace);

    writer.WriteLine("{0}", entityBase);

    writer.WriteLine("Events:");
    for (auto it = entityBase.Events.begin(); it != entityBase.Events.end(); ++it)
    {
        writer.WriteLine("\t{0}", *it);
    }

    for (auto it = entityBase.UnhealthyEvaluations.begin(); it != entityBase.UnhealthyEvaluations.end(); ++it)
    {
        auto evaluation = it->Evaluation;
        TestSession::FailTestIfNot(evaluation != nullptr, "evaluation should not be null for {0}", trace);
        TestSession::FailTestIf(evaluation->Description == L"", "Reason description should be defined for {0}", trace);
    }

    return trace;
}

bool TestFabricClientHealth::DeleteHealth(StringCollection const & params)
{
    if (params.size() < 2)
    {
        return false;
    }

    wstring entityType = params[0];
    CommandLineParser parser(params, 1);
    ServiceModel::HealthReport healthReport;
    if (StringUtility::AreEqualCaseInsensitive(entityType, L"node"))
    {
        if (!CreateNodeHealthDeleteReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"partition"))
    {
        if (!CreatePartitionHealthDeleteReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"replica"))
    {
        if (!CreateReplicaHealthDeleteReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"instance"))
    {
        if (!CreateInstanceHealthDeleteReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"deployedapplication"))
    {
        if (!CreateDeployedApplicationHealthDeleteReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"deployedservicepackage"))
    {
        if (!CreateDeployedServicePackageHealthDeleteReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"service"))
    {
        if (!CreateServiceHealthDeleteReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"app"))
    {
        if (!CreateApplicationHealthDeleteReport(parser, healthReport))
        {
            return false;
        }
    }
    else
    {
        TestSession::WriteError(TraceSource, "Unknown type {0}", entityType);
        return false;
    }

    CreateInternalFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);

    vector<HealthReport> healthVector;
    healthVector.push_back(move(healthReport));

    auto reportError = ReportHealthThroughInternalClientOrHmPrimary(parser, move(healthVector));

    std::wstring error;
    parser.TryGetString(L"expectederror", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);

    TestSession::FailTestIfNot(errorValue == reportError.ReadValue(), "InternalReportHealth for delete failed with unexpected reportError = {0}", reportError);
    return true;
}

bool TestFabricClientHealth::ReportHealthInternal(StringCollection const & params)
{
    if (params.size() < 2)
    {
        return false;
    }

    wstring entityType = params[0];
    CommandLineParser parser(params, 1);
    ServiceModel::HealthReport healthReport;
    if (StringUtility::AreEqualCaseInsensitive(entityType, L"node"))
    {
        if (!CreateInternalNodeHealthReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"partition"))
    {
        if (!CreateInternalPartitionHealthReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"cluster"))
    {
        if (!CreateInternalClusterHealthReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"replica"))
    {
        if (!CreateInternalReplicaHealthReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"instance"))
    {
        if (!CreateInternalInstanceHealthReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"deployedapplication"))
    {
        if (!CreateInternalDeployedApplicationHealthReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"deployedservicepackage"))
    {
        if (!CreateInternalDeployedServicePackageHealthReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"application"))
    {
        if (!CreateInternalApplicationHealthReport(parser, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"service"))
    {
        if (!CreateInternalServiceHealthReport(parser, healthReport))
        {
            return false;
        }
    }
    else
    {
        TestSession::WriteError(TraceSource, "Unknown type {0}", entityType);
        return false;
    }

    vector<HealthReport> healthVector;
    healthVector.push_back(move(healthReport));

    std::wstring error;
    bool expectedErrorExists = parser.TryGetString(L"expectederror", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);

    int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    ErrorCode reportError(ErrorCodeValue::Success);
    do
    {
        --remainingRetries;
        auto healthVectorCopy = healthVector;

        reportError = ReportHealthThroughInternalClientOrHmPrimary(parser, move(healthVectorCopy));

        if (errorValue == reportError.ReadValue())
        {
            break;
        }

        TestSession::FailTestIf(expectedErrorExists, "InternalReportHealth failed with error {0}, expected {1}", reportError, errorValue);

        TestSession::WriteWarning(TraceSource, "InternalReportHealth returned unexpected error = {0}, expected {1}", reportError, errorValue);
        Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));

    }while (remainingRetries > 0);

    TestSession::FailTestIfNot(errorValue == reportError.ReadValue(), "InternalReportHealth failed with unexpected reportError = {0}", reportError);
    return true;
}

bool TestFabricClientHealth::ReportHealthStress(StringCollection const & params)
{
    int reportCount;
    int interval;
    int reportsPerMessage;
    // Critical reports are counted as part of the total reports.
    // For example, if the test command is "reporthealth stress reports=1 reportsPerMessage=4 criticalreports=2", then this means that
    // a total of 4 reports are generated, out of which 2 are critical reports. Therefore, if the max pending reports is 1,
    // then 1 critical report and 1 non critical report is accepted.
    int criticalReports;
    int iterations;

    CommandLineParser parser(params, 1);
    parser.TryGetInt(L"reports", reportCount, 8192);
    parser.TryGetInt(L"interval", interval, 0);
    parser.TryGetInt(L"reportsPerMessage", reportsPerMessage, 1);
    parser.TryGetInt(L"criticalreports", criticalReports, 0);
    parser.TryGetInt(L"iterations", iterations, 1);

    auto timeout = ClientConfig::GetConfig().HealthOperationTimeout;

    double timeoutSec;
    if (parser.TryGetDouble(L"timeout", timeoutSec))
    {
        timeout = TimeSpan::FromSeconds(timeoutSec);
    }

    Management::HealthManager::HealthManagerReplicaSPtr hmPrimary;
    if (!GetHMPrimaryWithRetries(hmPrimary))
    {
        return false;
    }

    // Nodes must have System.FM report to be considered healthy, as that is the authority source
    wstring sourceId(L"System.FM");
    wstring description(L"stress report");

    for (int iteration = 0; iteration < iterations; ++iteration)
    {
        Common::atomic_long pendingCount(reportCount);
        Common::atomic_long successCount(0);
        ManualResetEvent event(false);

        Stopwatch stopwatch;

        stopwatch.Start();

        for (int i = 0; i < reportCount; i++)
        {
            vector<HealthReport> reports;
            int criticalGeneratedReports = 0;

            for (int j = 0; j < reportsPerMessage; ++j)
            {
                wstring property(L"stress");
                if (criticalGeneratedReports < criticalReports)
                {
                    // Simulate authority report
                    property = L"State";
                    ++criticalGeneratedReports;
                }

                LargeInteger nodeId = LargeInteger::RandomLargeInteger_();
                wstring nodeName = NodeIdGenerator::GenerateNodeName(NodeId(nodeId));

                AttributeList attributes;
                attributes.AddAttribute(*HealthAttributeNames::NodeName, nodeName);

                HealthReport healthReport(
                    EntityHealthInformation::CreateNodeEntityHealthInformation(nodeId, nodeName, DateTime::Now().Ticks),
                    sourceId,
                    property,
                    TimeSpan::MaxValue,
                    FABRIC_HEALTH_STATE_OK,
                    description,
                    Common::SequenceNumber::GetNext(),
                    false,
                    move(attributes));

                reports.push_back(move(healthReport));
            }

            ActivityId activityId;
            auto requestMessage = HealthManagerTcpMessage::GetReportHealth(
                activityId,
                Common::make_unique<ReportHealthMessageBody>(move(reports), std::vector<SequenceStreamInformation>()))->GetTcpMessage();

            hmPrimary->Test_BeginProcessRequest(
                move(requestMessage),
                activityId,
                timeout,
                [&hmPrimary, &event, &pendingCount, &successCount](AsyncOperationSPtr const & operation)
                {
                    Transport::MessageUPtr reply;
                    auto error = hmPrimary->Test_EndProcessRequest(operation, reply);

                    if (error.IsSuccess())
                    {
                        ReportHealthReplyMessageBody body;
                        reply->GetBody(body);
                        auto replyResults = body.MoveEntries();

                        if (replyResults.empty())
                        {
                            error = ErrorCodeValue::Timeout;
                        }
                        else
                        {
                            error = replyResults[0].Error;
                        }
                    }

                    if (error.IsSuccess())
                    {
                        ++successCount;
                    }

                    if (--pendingCount == 0)
                    {
                        event.Set();
                    }
                },
                AsyncOperationSPtr());

            if (interval > 0)
            {
                Sleep(interval);
            }
        }

        TestSession::WriteInfo(
            TraceSource,
            "[{0}] Waiting for {1}*{2} reports...",
            iteration,
            reportCount,
            reportsPerMessage);

        event.WaitOne();

        stopwatch.Stop();

        auto throughput = (static_cast<double>(successCount.load()) / stopwatch.Elapsed.TotalPositiveMilliseconds()) * 1000;
        auto effectiveThroughput = (static_cast<double>(successCount.load() * reportsPerMessage) / stopwatch.Elapsed.TotalPositiveMilliseconds()) * 1000;

        TestSession::WriteInfo(
            TraceSource,
            "[{0}] {1}*{2} reports created, {3} critical per message, {4} successful, {5} reports/sec, {6} effective reports/sec",
            iteration,
            reportCount,
            reportsPerMessage,
            criticalReports,
            successCount.load(),
            throughput,
            effectiveThroughput);

    } // for iterations

    return true;
}

Common::ErrorCode TestFabricClientHealth::ReportHealthThroughInternalClientOrHmPrimary(CommandLineParser & parser, std::vector<ServiceModel::HealthReport> && reports)
{
    if (FabricTestSessionConfig::GetConfig().ReportHealthThroughHMPrimary)
    {
        auto hmPrimary = FABRICSESSION.FabricDispatcher.GetHM();
        if (!hmPrimary)
        {
            TestSession::WriteInfo(TraceSource, "HM not ready");
            return ErrorCode(ErrorCodeValue::NotReady);
        }

        ActivityId activityId;
        Transport::MessageUPtr requestMessage = move(HealthManagerTcpMessage::GetReportHealth(
            activityId,
            Common::make_unique<ReportHealthMessageBody>(move(reports), std::vector<SequenceStreamInformation>()))->GetTcpMessage());

        auto timeout = ClientConfig::GetConfig().HealthOperationTimeout;

        double timeoutSec;
        if (parser.TryGetDouble(L"timeout", timeoutSec))
        {
            timeout = TimeSpan::FromSeconds(timeoutSec);
        }

        ManualResetEvent event(false);

        auto operation = hmPrimary->Test_BeginProcessRequest(
            move(requestMessage),
            activityId,
            timeout,
            [&event](AsyncOperationSPtr const &) { event.Set(); },
            AsyncOperationSPtr());

        Transport::MessageUPtr reply;
        ErrorCode error(ErrorCodeValue::Success);

        if (event.WaitOne(timeout + TimeSpan::FromSeconds(10)))
        {
            error = hmPrimary->Test_EndProcessRequest(operation, reply);
        }
        else
        {
            error = ErrorCodeValue::Timeout;
        }

        if (!error.IsSuccess())
        {
            TestSession::WriteInfo(TraceSource, "{0}: HM Test_ProcessRequest returned {1}", activityId, error);
            return error;
        }

        ReportHealthReplyMessageBody body;
        reply->GetBody(body);
        auto replyResults = body.MoveEntries();
        if (replyResults.empty() || replyResults[0].Error == ErrorCodeValue::NotReady)
        {
            TestSession::WriteInfo(TraceSource, "ReportHealthReplyMessageBody has no reports");
            return ErrorCode(ErrorCodeValue::Timeout);
        }

        if (replyResults[0].Error == ErrorCodeValue::NotReady)
        {
            // There wasn't enough time to set the result before the async operation timed out
            return ErrorCode(ErrorCodeValue::Timeout);
        }

        return replyResults[0].Error;
    }
    else
    {
        // Report through internal health client
        CreateInternalFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);
        bool immediate = parser.GetBool(L"immediate");
        HealthReportSendOptionsUPtr options;
        Random r;
        if (immediate || r.Next() == 0)
        {
            options = make_unique<HealthReportSendOptions>(immediate);
        }

        return internalHealthClient_->InternalReportHealth(move(reports), move(options));
    }
}

bool TestFabricClientHealth::ReportHealth(StringCollection const & params)
{
    if (params.size() < 1)
    {
        return false;
    }

    wstring reportType = params[0];
    if (reportType == L"stress")
    {
        return ReportHealthStress(params);
    }

    if (params.size() < 2)
    {
        return false;
    }

    if (FabricTestSessionConfig::GetConfig().UseInternalHealthClient)
    {
        return ReportHealthInternal(params);
    }

    CommandLineParser parser(params, 1);

    FABRIC_HEALTH_REPORT healthReport;
    ScopedHeap heap;
    if (StringUtility::AreEqualCaseInsensitive(reportType, L"node"))
    {
        if (!CreateNodeHealthReport(parser, heap, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(reportType, L"cluster"))
    {
        if (!CreateClusterHealthReport(parser, heap, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(reportType, L"partition"))
    {
        if (!CreatePartitionHealthReport(parser, heap, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(reportType, L"replica"))
    {
        if (!CreateReplicaHealthReport(parser, heap, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(reportType, L"instance"))
    {
        if (!CreateInstanceHealthReport(parser, heap, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(reportType, L"deployedapplication"))
    {
        if (!CreateDeployedApplicationHealthReport(parser, heap, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(reportType, L"deployedservicepackage"))
    {
        if (!CreateDeployedServicePackageHealthReport(parser, heap, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(reportType, L"application"))
    {
        if (!CreateApplicationHealthReport(parser, heap, healthReport))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(reportType, L"service"))
    {
        if (!CreateServiceHealthReport(parser, heap, healthReport))
        {
            return false;
        }
    }
    else
    {
        TestSession::WriteError(TraceSource, "Unknown type {0}", reportType);
        return false;
    }

    HRESULT hr;
    std::wstring error;
    bool expectedErrorExists = parser.TryGetString(L"expectederror", error, L"Success");

    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();
    CreateFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);

    bool immediate = parser.GetBool(L"immediate");
    Random r;

    int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    do
    {
        --remainingRetries;

        if (immediate || r.Next() == 0)
        {
            FABRIC_HEALTH_REPORT_SEND_OPTIONS sendOptions = { 0 };
            sendOptions.Immediate = immediate ? TRUE : FALSE;
            hr = healthClient_->ReportHealth2(&healthReport, &sendOptions);
        }
        else
        {
            hr = healthClient_->ReportHealth(&healthReport);
        }

        if (expectedError == hr)
        {
            break;
        }

        TestSession::FailTestIf(expectedErrorExists, "ReportHealth failed with unexpected hr = {0}", hr);

        TestSession::WriteWarning(TraceSource,  "ReportHealth returned unexpected hr = {0}", hr);
        Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));

    }while (remainingRetries > 0);

    TestSession::FailTestIfNot(expectedError == hr, "ReportHealth failed with unexpected hr = {0}", hr);

    return true;
}

bool TestFabricClientHealth::ReportHealthIpc(StringCollection const & params)
{
    if (params.size() < 3)
    {
        return false;
    }

    wstring reportType = params[0];
    CommandLineParser parser(params, 1);

    wstring nodeIdString;
    if (!parser.TryGetString(L"nodeid", nodeIdString, wstring()))
    {
        TestSession::WriteInfo(TraceSource, "Could not parse nodeid parameter");
        return false;
    }

    NodeId nodeId = FABRICSESSION.FabricDispatcher.ParseNodeId(nodeIdString);

    wstring serviceName;
    if (!parser.TryGetString(L"servicename", serviceName))
    {
        TestSession::WriteInfo(TraceSource, "Could not parse servicename parameter");
        return false;
    }

    ScopedHeap heap;
    ReferencePointer<FABRIC_HEALTH_INFORMATION> healthInfo = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    if (!CreateCommonHealthInformation(parser, heap, *healthInfo))
    {
        TestSession::WriteError(TraceSource, "Unable to build HealthInfo");
        return false;
    }

    ReferencePointer<FABRIC_HEALTH_REPORT_SEND_OPTIONS> sendOptions;
    bool immediate = parser.GetBool(L"immediate");
    Random r;
    if (immediate || r.Next() == 0)
    {
        sendOptions = heap.AddItem<FABRIC_HEALTH_REPORT_SEND_OPTIONS>();
        sendOptions->Immediate = immediate ? TRUE : FALSE;
    }

    CalculatorServiceSPtr calculatorService;
    ITestStoreServiceSPtr testStoreService;
    bool foundService = false;
    if (FABRICSESSION.FabricDispatcher.Federation.TryFindCalculatorService(serviceName, nodeId, calculatorService))
    {
        foundService = true;
    }
    else if (FABRICSESSION.FabricDispatcher.Federation.TryFindStoreService(serviceName, nodeId, testStoreService))
    {
        foundService = true;
    }

    if (!foundService)
    {
        TestSession::WriteWarning(TraceSource, "Failed to find service {0} on node {1}.", serviceName, nodeId);
        return false;
    }

    ErrorCode error;
    if (StringUtility::AreEqualCaseInsensitive(reportType, L"partition"))
    {
        if (calculatorService)
        {
            error = calculatorService->ReportPartitionHealth(healthInfo.GetRawPointer(), sendOptions.GetRawPointer());
        }
        else
        {
            TestSession::FailTestIf(testStoreService == nullptr, "reporthealthipc on partition: service is null");
            error = testStoreService->ReportPartitionHealth(healthInfo.GetRawPointer(), sendOptions.GetRawPointer());
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(reportType, L"replica"))
    {
        if (calculatorService)
        {
            error = calculatorService->ReportInstanceHealth(healthInfo.GetRawPointer(), sendOptions.GetRawPointer());
        }
        else
        {
            TestSession::FailTestIf(testStoreService == nullptr, "reporthealthipc on replica: service is null");
            error = testStoreService->ReportReplicaHealth(healthInfo.GetRawPointer(), sendOptions.GetRawPointer());
        }
    }
    else
    {
        TestSession::WriteError(TraceSource, "Cannot report health via ipc for entity: {0}", reportType);
        return false;
    }

    return true;
}

bool TestFabricClientHealth::CheckHealthClient(Common::StringCollection const & params)
{
    if (params.size() < 1)
    {
        return false;
    }

    CommandLineParser parser(params, 0);
    int reportCount;
    if (!parser.TryGetInt(L"reportcount", reportCount))
    {
        TestSession::WriteInfo(TraceSource, "Could not parse reportcount parameter");
        return false;
    }

    int expectedServiceTooBusyCount;
    bool checkServiceTooBusy = parser.TryGetInt(L"servicetoobusycount", expectedServiceTooBusyCount);

    CreateInternalFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);
    FabricClientImpl * fcImpl = static_cast<FabricClientImpl*>(internalHealthClient_.get());

    int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    do
    {
        int actualServiceTooBusyCount;
        int actualReportCount = fcImpl->Test_GetReportingComponent().Test_GetCurrentReportCount(actualServiceTooBusyCount);

        bool needsRetry = false;
        if (checkServiceTooBusy)
        {
            if (expectedServiceTooBusyCount == 0)
            {
                if (expectedServiceTooBusyCount != actualServiceTooBusyCount)
                {
                    TestSession::WriteWarning(TraceSource, "actualServiceTooBusyCount does not match: expected {0}, actual {1}", expectedServiceTooBusyCount, actualServiceTooBusyCount);
                    needsRetry = true;
                }
            }
            else if (expectedServiceTooBusyCount > actualServiceTooBusyCount)
            {
                TestSession::WriteWarning(TraceSource, "actualServiceTooBusyCount does not match: expected {0}, actual {1}", expectedServiceTooBusyCount, actualServiceTooBusyCount);
                needsRetry = true;
            }
        }

        if (reportCount != actualReportCount)
        {
            TestSession::WriteWarning(TraceSource, "reportcount does not match: expected {0}, actual {1}", reportCount, actualReportCount);
            needsRetry = true;
        }

        if (!needsRetry)
        {
            return true;
        }

        Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
        --remainingRetries;

    } while (remainingRetries > 0);

    // Retries have been exhausted, the check failed
    return false;
}

bool TestFabricClientHealth::CheckHMEntity(Common::StringCollection const & params)
{
    if (params.size() < 2)
    {
        return false;
    }

    auto const & entityType = params[0];
    CommandLineParser parser(params, 1);

    wstring expectedState;
    wstring expectedEntityState;
    if (!parser.TryGetString(L"state", expectedState) && !parser.TryGetString(L"expectedentitystate", expectedEntityState))
    {
        TestSession::WriteInfo(TraceSource, "Please specify either \"state\" or \"expectedentitystate\" for validation");
        return false;
    }

    // Changing the entity state can depend on the cleanup logic.
    // Since cleanup runs every HealthStoreCleanupInterval and respects grace periods HealthStoreCleanupGraceInterval and HealthStoreEntityWithoutSystemReportKeptInterval,
    // compute max number of retries depending on these.
    int maxRetryCapTimeInSeconds = 300;
    auto maxRetryTimeInSeconds =
        Management::ManagementConfig::GetConfig().HealthStoreCleanupInterval.TotalSeconds() +
        Management::ManagementConfig::GetConfig().HealthStoreCleanupGraceInterval.TotalSeconds() +
        Management::ManagementConfig::GetConfig().HealthStoreEntityWithoutSystemReportKeptInterval.TotalSeconds();
    if (maxRetryTimeInSeconds > maxRetryCapTimeInSeconds)
    {
        maxRetryTimeInSeconds = maxRetryCapTimeInSeconds;
    }

    auto remainingRetries = maxRetryTimeInSeconds / FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalSeconds();
    bool success = false;
    do
    {
        --remainingRetries;

        // Get the HM primary before retrying as it may have moved
        auto hmPrimary = FABRICSESSION.FabricDispatcher.GetHM();
        if (!hmPrimary)
        {
            TestSession::WriteWarning(TraceSource, "CheckHMEntity: HM not ready");
            success = false;
        }
        else if (!hmPrimary->Test_IsOpened())
        {
            TestSession::WriteWarning(TraceSource, "CheckHMEntity: HM created, but acceptRequests is false");
            success = false;
        }
        else if (StringUtility::AreEqualCaseInsensitive(entityType, L"node"))
        {
            if (!CheckNodeEntityState(parser, hmPrimary, success))
            {
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(entityType, L"partition"))
        {
            if (!CheckPartitionEntityState(parser, hmPrimary, success))
            {
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(entityType, L"replica"))
        {
            if (!CheckReplicaEntityState(parser, hmPrimary, success))
            {
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(entityType, L"service"))
        {
            if (!CheckServiceEntityState(parser, hmPrimary, success))
            {
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(entityType, L"application"))
        {
            if (!CheckApplicationEntityState(parser, hmPrimary, success))
            {
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(entityType, L"deployedapplication"))
        {
            if (!CheckDeployedApplicationEntityState(parser, hmPrimary, success))
            {
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(entityType, L"deployedservicepackage"))
        {
            if (!CheckDeployedServicePackageEntityState(parser, hmPrimary, success))
            {
                return false;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(entityType, L"cluster"))
        {
            if (!CheckClusterEntityState(parser, hmPrimary, success))
            {
                return false;
            }
        }
        else
        {
            TestSession::WriteError(TraceSource, "Unknown type {0}", entityType);
            return false;
        }

        if (!success)
        {
            Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
            TestSession::WriteInfo(TraceSource, "CheckHMEntity failed, remaining retries {0}", remainingRetries);
        }

    } while (!success && remainingRetries >= 0);

    if (!success)
    {
        TestSession::WriteWarning(TraceSource, "CheckHMEntity failed and retry time exhausted");
    }

    return success;
}

bool TestFabricClientHealth::CorruptHMEntity(Common::StringCollection const & params)
{
    if (params.size() < 2)
    {
        return false;
    }

    Management::HealthManager::HealthManagerReplicaSPtr hmPrimary;
    if (!GetHMPrimaryWithRetries(hmPrimary))
    {
        return false;
    }

    wstring const & entityType = params[0];
    ActivityId activityId;
    CommandLineParser parser(params, 1);

    HealthEntitySPtr entity;
    if (StringUtility::AreEqualCaseInsensitive(entityType, L"cluster"))
    {
        entity = hmPrimary->EntityManager->Cluster.GetEntity(*Management::HealthManager::Constants::StoreType_ClusterTypeString);
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"node"))
    {
        NodeId nodeId;
        FABRIC_NODE_INSTANCE_ID nodeInstanceId;
        std::wstring nodeName;
        if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
        {
            return false;
        }

        entity = hmPrimary->EntityManager->Nodes.GetEntity(nodeId.IdValue);
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"partition"))
    {
        PartitionHealthId partitionId;
        if (!ParsePartitionId(parser, partitionId))
        {
            return false;
        }

        entity = hmPrimary->EntityManager->Partitions.GetEntity(partitionId);
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"replica"))
    {
        FABRIC_REPLICA_ID replicaId;
        FABRIC_INSTANCE_ID replicaInstanceId;
        Common::Guid partitionId;
        if (!ParseReplicaId(parser, partitionId, replicaId, replicaInstanceId))
        {
            return false;
        }

        entity = hmPrimary->EntityManager->Replicas.GetEntity(ReplicaHealthId(partitionId, replicaId));
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"service"))
    {
        wstring serviceName;
        if (!parser.TryGetString(L"servicename", serviceName))
        {
            return false;
        }

        entity = hmPrimary->EntityManager->Services.GetEntity(ServiceHealthId(serviceName));
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"application"))
    {
        wstring applicationName;
        if (!parser.TryGetString(L"appname", applicationName))
        {
            return false;
        }

        entity = hmPrimary->EntityManager->Applications.GetEntity(ApplicationHealthId(applicationName));
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"deployedapplication"))
    {
        wstring appName;
        if (!parser.TryGetString(L"appname", appName, wstring()))
        {
            return false;
        }

        NodeId nodeId;
        FABRIC_NODE_INSTANCE_ID nodeInstanceId;
        std::wstring nodeName;
        if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
        {
            return false;
        }

        entity = hmPrimary->EntityManager->DeployedApplications.GetEntity(DeployedApplicationHealthId(appName, nodeId.IdValue));
    }
    else if (StringUtility::AreEqualCaseInsensitive(entityType, L"deployedservicepackage"))
    {
        wstring appName;
        if (!parser.TryGetString(L"appname", appName, wstring()))
        {
            return false;
        }

        wstring serviceManifestName;
        if (!parser.TryGetString(L"servicemanifestname", serviceManifestName))
        {
            return false;
        }

        wstring servicePackageActivationId;
        if (!ParseServicePackageActivationId(parser, servicePackageActivationId))
        {
            return false;
        }

        TestSession::WriteInfo(TraceSource, "CorruptHMEntity(): Using ServicePackageActivationId=[{0}] for deployedservicepackage.", servicePackageActivationId);

        NodeId nodeId;
        FABRIC_NODE_INSTANCE_ID nodeInstanceId;
        std::wstring nodeName;
        if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
        {
            return false;
        }

        entity = hmPrimary->EntityManager->DeployedServicePackages.GetEntity(
            DeployedServicePackageHealthId(appName, serviceManifestName, servicePackageActivationId, nodeId.IdValue));
    }
    else
    {
        TestSession::WriteError(TraceSource, "Unknown type {0}", entityType);
        return false;
    }

    bool changeEntityState = parser.GetBool(L"changeentitystate");
    bool changeHasSystemReport = parser.GetBool(L"changeHasSystemReport");
    bool changeSystemErrorCount = parser.GetBool(L"changeSystemErrorCount");

    vector<HealthInformation> addReports;
    wstring eventsString;
    if (parser.TryGetString(L"addevents", eventsString))
    {
        map<wstring, vector<wstring>> addEvents;
        if (!ParseEvents(eventsString, addEvents))
        {
            return false;
        }

        Random r;
        for (auto const & entry : addEvents)
        {
            addReports.push_back(HealthInformation(
                entry.second[0], // source
                entry.second[1], // property
                TimeSpan::MaxValue,
                r.Next(3) == 1 ? FABRIC_HEALTH_STATE_WARNING : FABRIC_HEALTH_STATE_ERROR,
                L"Corrupted report",
                Common::SequenceNumber::GetNext(),
                DateTime::Now(),
                false));
        }
    }

    vector<pair<wstring, wstring>> removeEventKeys;
    if (parser.TryGetString(L"removeevents", eventsString))
    {
        map<wstring, vector<wstring>> removeEvents;
        if (!ParseEvents(eventsString, removeEvents))
        {
            return false;
        }

        for (auto const & entry : removeEvents)
        {
            removeEventKeys.push_back(make_pair(entry.second[0], entry.second[1]));
        }
    }

    vector<tuple<wstring, wstring, FABRIC_HEALTH_STATE>> scrambleEventKeys;
    if (parser.TryGetString(L"scrambleevents", eventsString))
    {
        map<wstring, vector<wstring>> scrambleEvents;
        if (!ParseEvents(eventsString, scrambleEvents))
        {
            return false;
        }

        for (auto const & entry : scrambleEvents)
        {
            scrambleEventKeys.push_back(make_tuple(entry.second[0], entry.second[1], GetHealthState(entry.second[2])));
        }
    }

    HealthEntityState::Enum entityState;
    size_t eventCount;
    if (!entity->Test_CorruptEntity(
        activityId,
        changeEntityState,
        changeHasSystemReport,
        changeSystemErrorCount,
        move(addReports),
        removeEventKeys,
        scrambleEventKeys,
        entityState,
        eventCount))
    {
        TestSession::WriteError(TraceSource, "{0}: Corrupt entity failed", activityId);
        return false;
    }

    int expectedEventCount;
    if (parser.TryGetInt(L"expectedeventcount", expectedEventCount))
    {
        if (static_cast<int>(eventCount) != expectedEventCount)
        {
            TestSession::WriteInfo(TraceSource, "Unexpected event count: expected {0}, actual {1}.", expectedEventCount, eventCount);
            return false;
        }
    }

    wstring expectedEntityStateString;
    if (parser.TryGetString(L"expectedentitystate", expectedEntityStateString))
    {
        wstring actualEntityState = wformatString(entityState);
        if (!StringUtility::AreEqualCaseInsensitive(expectedEntityStateString, actualEntityState))
        {
            TestSession::WriteInfo(TraceSource, "Unexpected entity health state: expected {0}, actual {1}.", expectedEntityStateString, actualEntityState);
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::QueryHealthStateChunk(StringCollection const & params)
{
    CreateFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);

    CommandLineParser parser(params, 0);

    bool requiresValidation = false;

    wstring expectedHealthStateString;
    wstring expectedHealthStatesString;
    wstring expectedApplicationsString;
    requiresValidation |= parser.TryGetString(L"expectedhealthstate", expectedHealthStateString);
    requiresValidation |= parser.TryGetString(L"expectedstates", expectedHealthStatesString);
    requiresValidation |= parser.TryGetString(L"expectedapps", expectedApplicationsString);

    vector<HRESULT> expectedErrors;
    std::wstring error;
    if (parser.TryGetString(L"expectederror", error, L"Success"))
    {
        ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
        expectedErrors.push_back(ErrorCode(errorValue).ToHResult());
    }
    else
    {
        expectedErrors.push_back(S_OK);
    }

    if (!requiresValidation)
    {
        expectedErrors.push_back(FABRIC_E_HEALTH_ENTITY_NOT_FOUND);
    }

    vector<HRESULT> allowedErrorsOnRetry;
    allowedErrorsOnRetry.push_back(S_OK);

    vector<HRESULT> retryableErrors;
    retryableErrors.push_back(FABRIC_E_NOT_READY);
    retryableErrors.push_back(FABRIC_E_HEALTH_ENTITY_NOT_FOUND);

    if (!requiresValidation)
    {
        retryableErrors.push_back(FABRIC_E_SERVICE_TOO_BUSY);
    }
    else
    {
        std::wstring retryableError;
        if (parser.TryGetString(L"retryableerror", retryableError, L"Success"))
        {
            ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(retryableError);
            retryableErrors.push_back(ErrorCode(errorValue).ToHResult());
        }
    }

    CreateFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);

    wstring queryType = params[0];
    bool result;
    if (StringUtility::AreEqualCaseInsensitive(queryType, L"cluster"))
    {
        result = GetClusterHealthChunk(params, requiresValidation, expectedErrors, allowedErrorsOnRetry, retryableErrors);
    }
    else
    {
        TestSession::WriteError(TraceSource, "Unsupported type {0}", queryType);
        return false;
    }

    if (!result)
    {
        TestSession::FailTest("GetHealthStateChunk failed");
    }

    return true;
}

bool TestFabricClientHealth::QueryHealth(StringCollection const & params)
{
    CreateFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);

    if (params.size() == 0)
    {
        return healthTable_->TestQuery(healthClient_);
    }

    bool result;
    wstring const & queryType = params[0];
    if (StringUtility::AreEqualCaseInsensitive(queryType, L"node"))
    {
        result = GetNodeHealth(params);
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"partition"))
    {
        result = GetPartitionHealth(params);
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"replica"))
    {
        result = GetReplicaHealth(params);
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"service"))
    {
        result = GetServiceHealth(params);
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"application"))
    {
        result = GetApplicationHealth(params);
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"deployedapplication"))
    {
        result = GetDeployedApplicationHealth(params);
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"deployedservicepackage"))
    {
        result = GetDeployedServicePackageHealth(params);
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"cluster"))
    {
        result = GetClusterHealth(params);
    }
    else
    {
        TestSession::WriteError(TraceSource, "Unknown type {0}", queryType);
        return false;
    }

    if (!result)
    {
        TestSession::WriteError(TraceSource,"GetHealth failed");
        return false;
    }

    return true;
}

bool CheckQueryHResult(HRESULT hr, vector<HRESULT> const & expectedErrors, bool requiresValidation)
{
    bool isExpected = false;
    for (auto it = expectedErrors.begin(); it != expectedErrors.end(); ++it)
    {
        if (hr == *it)
        {
            isExpected = true;
            break;
        }
    }

    if (FAILED(hr))
    {
        ErrorCode error = ErrorCode::FromHResult(hr);
        if (isExpected)
        {
            TestSession::WriteInfo(TraceSource, "Query returned expected error {0}", error);
        }
        else if (requiresValidation)
        {
            TestSession::WriteWarning(TraceSource, "Query failed with error {0}", error);
            return false;
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "Query returned {0}", error);
        }
    }
    else if (!isExpected)
    {
        TestSession::WriteWarning(TraceSource, "Query returned success. Expected: {0}", expectedErrors);
        return false;
    }

    return true;
}

bool TestFabricClientHealth::GetClusterHealthChunk(StringCollection const & params, bool requiresValidation, vector<HRESULT> const & expectedErrors, vector<HRESULT> const & allowedErrorsOnRetry, vector<HRESULT> const & retryableErrors)
{
    CommandLineParser parser(params, 0);
    ScopedHeap heap;

    auto queryDescription = heap.AddItem<FABRIC_CLUSTER_HEALTH_CHUNK_QUERY_DESCRIPTION>();

    wstring nodeFiltersString;
    if (parser.TryGetString(L"nodefilters", nodeFiltersString))
    {
        NodeHealthStateFilterListWrapper nodeFilters;
        auto error = nodeFilters.FromJsonString(nodeFiltersString, nodeFilters);
        TestSession::FailTestIfNot(error.IsSuccess(), "Failed to parse node filters string {0} with {1}", nodeFiltersString, error);

        auto nodeFiltersPublic = heap.AddItem<FABRIC_NODE_HEALTH_STATE_FILTER_LIST>();
        error = nodeFilters.ToPublicApi(heap, *nodeFiltersPublic);
        queryDescription->NodeFilters = nodeFiltersPublic.GetRawPointer();
    }

    wstring applicationFiltersString;
    if (parser.TryGetString(L"applicationfilters", applicationFiltersString))
    {
        ApplicationHealthStateFilterListWrapper applicationFilters;
        auto error = applicationFilters.FromJsonString(applicationFiltersString, applicationFilters);
        TestSession::FailTestIfNot(error.IsSuccess(), "Failed to parse application filters string {0} with {1}", applicationFiltersString, error);

        auto applicationFiltersPublic = heap.AddItem<FABRIC_APPLICATION_HEALTH_STATE_FILTER_LIST>();
        error = applicationFilters.ToPublicApi(heap, *applicationFiltersPublic);
        queryDescription->ApplicationFilters = applicationFiltersPublic.GetRawPointer();
    }

    std::unique_ptr<ClusterHealthPolicy> healthPolicy;
    if (!ParseClusterHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        auto clusterHealthPolicy = heap.AddItem<FABRIC_CLUSTER_HEALTH_POLICY>();
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        healthPolicy->ToPublicApi(heap, *clusterHealthPolicy);
        queryDescription->ClusterHealthPolicy = clusterHealthPolicy.GetRawPointer();
    }

    // Parse application health policy map passed as a Json string
    ApplicationHealthPolicyMapSPtr applicationHealthPolicies;
    if (!GetApplicationHealthPolicies(parser, applicationHealthPolicies))
    {
        return false;
    }

    if (applicationHealthPolicies)
    {
        auto applicationHealthPolicyMap = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY_MAP>();
        applicationHealthPolicies->ToPublicApi(heap, *applicationHealthPolicyMap);
        queryDescription->ApplicationHealthPolicyMap = applicationHealthPolicyMap.GetRawPointer();
    }

    wstring expectedErrorMessage;
    parser.TryGetString(L"expectederrormessage", expectedErrorMessage);

    CreateFabricHealthClient(FABRICSESSION.FabricDispatcher.Federation);

    bool success = false;
    int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    do
    {
        --remainingRetries;
        ComPointer<IFabricGetClusterHealthChunkResult> clusterHealthChunkResult;
        HRESULT hr = TestFabricClient::PerformFabricClientOperation(
            L"GetClusterHealthChunk",
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->BeginGetClusterHealthChunk(
                queryDescription.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
            [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->EndGetClusterHealthChunk(context.GetRawPointer(), clusterHealthChunkResult.InitializationAddress());
        },
            expectedErrors,
            allowedErrorsOnRetry,
            retryableErrors,
            true /*failOnError*/,
            expectedErrorMessage);

        success = CheckQueryHResult(hr, expectedErrors, requiresValidation);
        if (success && SUCCEEDED(hr))
        {
            success = ValidateClusterHealthChunk(parser, *clusterHealthChunkResult->get_ClusterHealthChunk(), requiresValidation);
        }

        if (!success)
        {
            Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
        }

    } while (!success && remainingRetries >= 0);

    return success;
}

bool TestFabricClientHealth::PerformHealthQueryClientOperation(
    __in CommandLineParser & parser,
    std::wstring const & operationName,
    TestFabricClient::BeginFabricClientOperationCallback const & beginCallback,
    TestFabricClient::EndFabricClientOperationCallback const & endCallback,
    ValidateHealthCallback const & validateCallback)
{
    bool expectEmpty = parser.GetBool(L"expectempty") | parser.GetBool(L"expectedempty");

    bool requiresValidation = expectEmpty;

    wstring expectedHealthStateString;
    wstring expectedHealthDescString;
    wstring expectedHealthStatesString;
    wstring expectedHealthDescStringLength;
    wstring expectedEvaluation;
    wstring expectedStatsString;
    int expectedEventCount;
    requiresValidation |= parser.TryGetString(L"expectedhealthstate", expectedHealthStateString);
    requiresValidation |= parser.TryGetString(L"expecteddesc", expectedHealthDescString);
    requiresValidation |= parser.TryGetString(L"expecteddesclength", expectedHealthDescStringLength);
    requiresValidation |= parser.TryGetString(L"expectedstates", expectedHealthStatesString);
    requiresValidation |= parser.TryGetInt(L"expectedeventcount", expectedEventCount, -1);
    requiresValidation |= parser.TryGetString(L"expectedreason", expectedEvaluation);
    requiresValidation |= parser.TryGetString(L"stats", expectedStatsString);

    vector<HRESULT> expectedErrors;
    std::wstring error;
    if (parser.TryGetString(L"expectederror", error, L"Success"))
    {
        ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
        expectedErrors.push_back(ErrorCode(errorValue).ToHResult());
    }
    else
    {
        expectedErrors.push_back(S_OK);
    }

    if (expectEmpty || (expectedEventCount == 0) || !requiresValidation)
    {
        expectedErrors.push_back(FABRIC_E_HEALTH_ENTITY_NOT_FOUND);
    }

    vector<HRESULT> allowedErrorsOnRetry;
    allowedErrorsOnRetry.push_back(S_OK);

    vector<HRESULT> retryableErrors;
    retryableErrors.push_back(FABRIC_E_NOT_READY);
    if (!expectEmpty)
    {
        retryableErrors.push_back(FABRIC_E_HEALTH_ENTITY_NOT_FOUND);
    }
    else
    {
        retryableErrors.push_back(FABRIC_E_VALUE_TOO_LARGE);
    }

    if (!requiresValidation)
    {
        retryableErrors.push_back(FABRIC_E_SERVICE_TOO_BUSY);
    }
    else
    {
        std::wstring retryableError;
        if (parser.TryGetString(L"retryableerror", retryableError, L"Success"))
        {
            ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(retryableError);
            retryableErrors.push_back(ErrorCode(errorValue).ToHResult());
        }
    }

    wstring expectedErrorMessage;
    parser.TryGetString(L"expectederrormessage", expectedErrorMessage);

    bool success = false;
    int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    do
    {
        --remainingRetries;
        HRESULT hr = TestFabricClient::PerformFabricClientOperation(
            operationName,
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            beginCallback,
            endCallback,
            expectedErrors,
            allowedErrorsOnRetry,
            retryableErrors,
            true /*failOnError*/,
            expectedErrorMessage);

        success = CheckQueryHResult(hr, expectedErrors, requiresValidation);
        if (success && SUCCEEDED(hr))
        {
            success = validateCallback(expectEmpty, requiresValidation);
        }

        if (!success)
        {
            TestSession::WriteInfo(TraceSource, "PerformHealthQueryClientOperation {0}: !success, remainingRetries {1}...", operationName, remainingRetries);
            Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
        }

    } while (!success && remainingRetries >= 0);

    return success;
}

bool TestFabricClientHealth::GetClusterHealth(StringCollection const & params)
{
    CommandLineParser parser(params, 0);
    ScopedHeap heap;

    auto queryDescription = heap.AddItem<FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION>();

    wstring eventsFilterString;
    if (parser.TryGetString(L"eventsfilter", eventsFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(eventsFilterString, filter))
        {
            return false;
        }

        auto eventsFilter = heap.AddItem<FABRIC_HEALTH_EVENTS_FILTER>();
        eventsFilter->HealthStateFilter = filter;
        queryDescription->EventsFilter = eventsFilter.GetRawPointer();
    }

    wstring nodesFilterString;
    if (parser.TryGetString(L"nodesfilter", nodesFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(nodesFilterString, filter))
        {
            return false;
        }

        auto nodesFilter = heap.AddItem<FABRIC_NODE_HEALTH_STATES_FILTER>();
        nodesFilter->HealthStateFilter = filter;
        queryDescription->NodesFilter = nodesFilter.GetRawPointer();
    }

    wstring applicationsFilterString;
    if (parser.TryGetString(L"applicationsfilter", applicationsFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(applicationsFilterString, filter))
        {
            return false;
        }

        auto applicationsFilter = heap.AddItem<FABRIC_APPLICATION_HEALTH_STATES_FILTER>();
        applicationsFilter->HealthStateFilter = filter;
        queryDescription->ApplicationsFilter = applicationsFilter.GetRawPointer();
    }

    std::unique_ptr<ClusterHealthPolicy> healthPolicy;
    if (!ParseClusterHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        auto clusterHealthPolicy = heap.AddItem<FABRIC_CLUSTER_HEALTH_POLICY>();
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        healthPolicy->ToPublicApi(heap, *clusterHealthPolicy);
        queryDescription->HealthPolicy = clusterHealthPolicy.GetRawPointer();
    }

    // Parse application health policy map passed as a Json string
    ApplicationHealthPolicyMapSPtr applicationHealthPolicies;
    if (!GetApplicationHealthPolicies(parser, applicationHealthPolicies))
    {
        return false;
    }

    if (applicationHealthPolicies)
    {
        auto applicationHealthPolicyMap = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY_MAP>();
        applicationHealthPolicies->ToPublicApi(heap, *applicationHealthPolicyMap);
        queryDescription->ApplicationHealthPolicyMap = applicationHealthPolicyMap.GetRawPointer();
    }

    wstring excludeHealthStatsString;
    wstring includeSystemAppStatsString;
    if (parser.TryGetString(L"excludeHealthStats", excludeHealthStatsString) ||
        parser.TryGetString(L"includeSystemAppStats", includeSystemAppStatsString))
    {
        auto statsFilter = heap.AddItem<FABRIC_CLUSTER_HEALTH_STATISTICS_FILTER>();
        if (!excludeHealthStatsString.empty())
        {
            bool excludeHealthStats = parser.GetBool(L"excludeHealthStats");
            statsFilter->ExcludeHealthStatistics = excludeHealthStats ? TRUE : FALSE;
        }

        if (!includeSystemAppStatsString.empty())
        {
            bool includeSystemAppStats = parser.GetBool(L"includeSystemAppStats");
            statsFilter->IncludeSystemApplicationHealthStatistics = includeSystemAppStats ? TRUE : FALSE;
        }

        auto ex1 = heap.AddItem<FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION_EX1>();
        ex1->HealthStatisticsFilter = statsFilter.GetRawPointer();
        queryDescription->Reserved = ex1.GetRawPointer();
    }

    ComPointer<IFabricClusterHealthResult> clusterHealthResult;
    return PerformHealthQueryClientOperation(
        parser,
        L"GetClusterHealth",
        [this, &queryDescription](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->BeginGetClusterHealth2(
                queryDescription.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [this, &clusterHealthResult](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->EndGetClusterHealth2(context.GetRawPointer(), clusterHealthResult.InitializationAddress());
        },
        [this, &parser, &clusterHealthResult](bool expectEmpty, bool requiresValidation)
        {
            bool result = ValidateClusterHealth(parser, *clusterHealthResult->get_ClusterHealth(), expectEmpty, requiresValidation);
            clusterHealthResult.Release();
            return result;
        });
}

bool TestFabricClientHealth::GetNodeHealth(StringCollection const & params)
{
    CommandLineParser parser(params, 0);
    ScopedHeap heap;

    auto queryDescription = heap.AddItem<FABRIC_NODE_HEALTH_QUERY_DESCRIPTION>();

    wstring eventsFilterString;
    if (parser.TryGetString(L"eventsfilter", eventsFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(eventsFilterString, filter))
        {
            return false;
        }

        auto eventsFilter = heap.AddItem<FABRIC_HEALTH_EVENTS_FILTER>();
        eventsFilter->HealthStateFilter = filter;
        queryDescription->EventsFilter = eventsFilter.GetRawPointer();
    }

    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    queryDescription->NodeName = heap.AddString(nodeName);

    std::unique_ptr<ClusterHealthPolicy> healthPolicy;
    if (!ParseClusterHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        auto nodeHealthPolicy = heap.AddItem<FABRIC_CLUSTER_HEALTH_POLICY>();
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        healthPolicy->ToPublicApi(heap, *nodeHealthPolicy);
        queryDescription->HealthPolicy = nodeHealthPolicy.GetRawPointer();
    }

    ComPointer<IFabricNodeHealthResult> nodeHealthResult;
    return PerformHealthQueryClientOperation(
        parser,
        L"GetNodeHealth",
        [this, &queryDescription](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->BeginGetNodeHealth2(
                queryDescription.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [this, &nodeHealthResult](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->EndGetNodeHealth2(context.GetRawPointer(), nodeHealthResult.InitializationAddress());
        },
        [this, &parser, &nodeHealthResult, &nodeName](bool expectEmpty, bool requiresValidation)
        {
            bool result = this->ValidateNodeHealth(parser, *nodeHealthResult->get_NodeHealth(), nodeName, expectEmpty, requiresValidation);
            nodeHealthResult.Release();
            return result;
        });
}

bool TestFabricClientHealth::GetReplicaHealth(StringCollection const & params)
{
    CommandLineParser parser(params, 0);
    ScopedHeap heap;

    auto queryDescription = heap.AddItem<FABRIC_REPLICA_HEALTH_QUERY_DESCRIPTION>();

    wstring eventsFilterString;
    if (parser.TryGetString(L"eventsfilter", eventsFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(eventsFilterString, filter))
        {
            return false;
        }

        auto eventsFilter = heap.AddItem<FABRIC_HEALTH_EVENTS_FILTER>();
        eventsFilter->HealthStateFilter = filter;
        queryDescription->EventsFilter = eventsFilter.GetRawPointer();
    }

    FABRIC_REPLICA_ID replicaId;
    FABRIC_INSTANCE_ID replicaInstanceId;
    Common::Guid partitionGuid;
    if (!ParseReplicaId(parser, partitionGuid, replicaId, replicaInstanceId))
    {
        return false;
    }

    queryDescription->PartitionId = partitionGuid.AsGUID();
    queryDescription->ReplicaOrInstanceId = replicaId;

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        auto replicaHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        healthPolicy->ToPublicApi(heap, *replicaHealthPolicy);
        queryDescription->HealthPolicy = replicaHealthPolicy.GetRawPointer();
    }

    ComPointer<IFabricReplicaHealthResult> replicaHealthResult;
    return PerformHealthQueryClientOperation(
        parser,
        L"GetReplicaHealth",
        [this, &queryDescription](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->BeginGetReplicaHealth2(
                queryDescription.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [this, &replicaHealthResult](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->EndGetReplicaHealth2(context.GetRawPointer(), replicaHealthResult.InitializationAddress());
        },
        [this, &parser, &replicaHealthResult, partitionGuid, replicaId](bool expectEmpty, bool requiresValidation)
        {
            bool result = ValidateReplicaHealth(parser, *replicaHealthResult->get_ReplicaHealth(), partitionGuid, replicaId, expectEmpty, requiresValidation);
            replicaHealthResult.Release();
            return result;
        });
}

bool TestFabricClientHealth::GetPartitionHealth(StringCollection const & params)
{
    CommandLineParser parser(params, 0);
    ScopedHeap heap;
    auto queryDescription = heap.AddItem<FABRIC_PARTITION_HEALTH_QUERY_DESCRIPTION>();

    wstring eventsFilterString;
    if (parser.TryGetString(L"eventsfilter", eventsFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(eventsFilterString, filter))
        {
            return false;
        }

        auto eventsFilter = heap.AddItem<FABRIC_HEALTH_EVENTS_FILTER>();
        eventsFilter->HealthStateFilter = filter;
        queryDescription->EventsFilter = eventsFilter.GetRawPointer();
    }

    wstring replicasFilterString;
    if (parser.TryGetString(L"replicasfilter", replicasFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(replicasFilterString, filter))
        {
            return false;
        }

        auto replicasFilter = heap.AddItem<FABRIC_REPLICA_HEALTH_STATES_FILTER>();
        replicasFilter->HealthStateFilter = filter;
        queryDescription->ReplicasFilter = replicasFilter.GetRawPointer();
    }

    wstring excludeHealthStatsString;
    if (parser.TryGetString(L"excludeHealthStats", excludeHealthStatsString))
    {
        bool excludeHealthStats = parser.GetBool(L"excludeHealthStats");
        auto statsFilter = heap.AddItem<FABRIC_PARTITION_HEALTH_STATISTICS_FILTER>();
        statsFilter->ExcludeHealthStatistics = excludeHealthStats ? TRUE : FALSE;

        auto ex1 = heap.AddItem<FABRIC_PARTITION_HEALTH_QUERY_DESCRIPTION_EX1>();
        ex1->HealthStatisticsFilter = statsFilter.GetRawPointer();
        queryDescription->Reserved = ex1.GetRawPointer();
    }

    Common::Guid partitionGuid;
    if (!ParsePartitionId(parser, partitionGuid))
    {
        return false;
    }

    queryDescription->PartitionId = partitionGuid.AsGUID();

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        auto partitionHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        healthPolicy->ToPublicApi(heap, *partitionHealthPolicy);
        queryDescription->HealthPolicy = partitionHealthPolicy.GetRawPointer();
    }

    ComPointer<IFabricPartitionHealthResult> partitionHealthResult;
    return PerformHealthQueryClientOperation(
        parser,
        L"GetPartitionHealth",
        [this, &queryDescription](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->BeginGetPartitionHealth2(
                queryDescription.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [this, &partitionHealthResult](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->EndGetPartitionHealth2(context.GetRawPointer(), partitionHealthResult.InitializationAddress());
        },
        [this, &parser, &partitionHealthResult, partitionGuid](bool expectEmpty, bool requiresValidation)
        {
            bool result = ValidatePartitionHealth(parser, *partitionHealthResult->get_PartitionHealth(), partitionGuid, expectEmpty, requiresValidation);
            partitionHealthResult.Release();
            return result;
        });
}

bool TestFabricClientHealth::GetServiceHealth(StringCollection const & params)
{
    CommandLineParser parser(params, 0);
    ScopedHeap heap;
    auto queryDescription = heap.AddItem<FABRIC_SERVICE_HEALTH_QUERY_DESCRIPTION>();

    wstring eventsFilterString;
    if (parser.TryGetString(L"eventsfilter", eventsFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(eventsFilterString, filter))
        {
            return false;
        }

        auto eventsFilter = heap.AddItem<FABRIC_HEALTH_EVENTS_FILTER>();
        eventsFilter->HealthStateFilter = filter;
        queryDescription->EventsFilter = eventsFilter.GetRawPointer();
    }

    wstring partitionsFilterString;
    if (parser.TryGetString(L"partitionsfilter", partitionsFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(partitionsFilterString, filter))
        {
            return false;
        }

        auto partitionsFilter = heap.AddItem<FABRIC_PARTITION_HEALTH_STATES_FILTER>();
        partitionsFilter->HealthStateFilter = filter;
        queryDescription->PartitionsFilter = partitionsFilter.GetRawPointer();
    }

    wstring serviceNameString;
    if (!parser.TryGetString(L"servicename", serviceNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse servicename parameter");
        return false;
    }
    queryDescription->ServiceName = heap.AddString(serviceNameString);

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        auto serviceHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        healthPolicy->ToPublicApi(heap, *serviceHealthPolicy);
        queryDescription->HealthPolicy = serviceHealthPolicy.GetRawPointer();
    }

    wstring excludeHealthStatsString;
    if (parser.TryGetString(L"excludeHealthStats", excludeHealthStatsString))
    {
        bool excludeHealthStats = parser.GetBool(L"excludeHealthStats");
        auto statsFilter = heap.AddItem<FABRIC_SERVICE_HEALTH_STATISTICS_FILTER>();
        statsFilter->ExcludeHealthStatistics = excludeHealthStats ? TRUE : FALSE;

        auto ex1 = heap.AddItem<FABRIC_SERVICE_HEALTH_QUERY_DESCRIPTION_EX1>();
        ex1->HealthStatisticsFilter = statsFilter.GetRawPointer();
        queryDescription->Reserved = ex1.GetRawPointer();
    }

    ComPointer<IFabricServiceHealthResult> serviceHealthResult;
    return PerformHealthQueryClientOperation(
        parser,
        L"GetServiceHealth",
        [this, &queryDescription](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->BeginGetServiceHealth2(
                queryDescription.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [this, &serviceHealthResult](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->EndGetServiceHealth2(context.GetRawPointer(), serviceHealthResult.InitializationAddress());
        },
        [this, &parser, &serviceHealthResult, &serviceNameString](bool expectEmpty, bool requiresValidation)
        {
            bool result = ValidateServiceHealth(parser, *serviceHealthResult->get_ServiceHealth(), serviceNameString, expectEmpty, requiresValidation);
            serviceHealthResult.Release();
            return result;
        });
}

bool TestFabricClientHealth::GetDeployedApplicationHealth(StringCollection const & params)
{
    CommandLineParser parser(params, 0);
    ScopedHeap heap;
    auto queryDescription = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_QUERY_DESCRIPTION>();

    wstring eventsFilterString;
    if (parser.TryGetString(L"eventsfilter", eventsFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(eventsFilterString, filter))
        {
            return false;
        }

        auto eventsFilter = heap.AddItem<FABRIC_HEALTH_EVENTS_FILTER>();
        eventsFilter->HealthStateFilter = filter;
        queryDescription->EventsFilter = eventsFilter.GetRawPointer();
    }

    wstring deployedServicePackagesFilterString;
    if (parser.TryGetString(L"deployedservicepackagesfilter", deployedServicePackagesFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(deployedServicePackagesFilterString, filter))
        {
            return false;
        }

        auto deployedServicePackagesFilter = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATES_FILTER>();
        deployedServicePackagesFilter->HealthStateFilter = filter;
        queryDescription->DeployedServicePackagesFilter = deployedServicePackagesFilter.GetRawPointer();
    }

    wstring applicationNameString;
    if (!parser.TryGetString(L"appname", applicationNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    queryDescription->ApplicationName = heap.AddString(applicationNameString);

    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }
    queryDescription->NodeName = heap.AddString(nodeName);

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        auto deployedApplicationHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        healthPolicy->ToPublicApi(heap, *deployedApplicationHealthPolicy);
        queryDescription->HealthPolicy = deployedApplicationHealthPolicy.GetRawPointer();
    }

    wstring excludeHealthStatsString;
    if (parser.TryGetString(L"excludeHealthStats", excludeHealthStatsString))
    {
        bool excludeHealthStats = parser.GetBool(L"excludeHealthStats");
        auto statsFilter = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_STATISTICS_FILTER>();
        statsFilter->ExcludeHealthStatistics = excludeHealthStats ? TRUE : FALSE;

        auto ex1 = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_QUERY_DESCRIPTION_EX1>();
        ex1->HealthStatisticsFilter = statsFilter.GetRawPointer();
        queryDescription->Reserved = ex1.GetRawPointer();
    }

    ComPointer<IFabricDeployedApplicationHealthResult> deployedApplicationHealthResult;
    return PerformHealthQueryClientOperation(
        parser,
        L"GetDeployedApplicationHealth",
        [this, &queryDescription](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->BeginGetDeployedApplicationHealth2(
                queryDescription.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [this, &deployedApplicationHealthResult](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->EndGetDeployedApplicationHealth2(context.GetRawPointer(), deployedApplicationHealthResult.InitializationAddress());
        },
        [this, &parser, &deployedApplicationHealthResult, &applicationNameString, &nodeName](bool expectEmpty, bool requiresValidation)
        {
            bool result = ValidateDeployedApplicationHealth(parser, *deployedApplicationHealthResult->get_DeployedApplicationHealth(), applicationNameString, nodeName, expectEmpty, requiresValidation);
            deployedApplicationHealthResult.Release();
            return result;
        });
}

bool TestFabricClientHealth::GetDeployedServicePackageHealth(StringCollection const & params)
{
    CommandLineParser parser(params, 0);
    ScopedHeap heap;
    auto queryDescription = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION>();

    wstring eventsFilterString;
    if (parser.TryGetString(L"eventsfilter", eventsFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(eventsFilterString, filter))
        {
            return false;
        }

        auto eventsFilter = heap.AddItem<FABRIC_HEALTH_EVENTS_FILTER>();
        eventsFilter->HealthStateFilter = filter;
        queryDescription->EventsFilter = eventsFilter.GetRawPointer();
    }

    wstring applicationNameString;
    if (!parser.TryGetString(L"appname", applicationNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    queryDescription->ApplicationName = heap.AddString(applicationNameString);

    wstring serviceManifestNameString;
    if (!parser.TryGetString(L"servicemanifestname", serviceManifestNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse servicemanifestname parameter");
        return false;
    }

    queryDescription->ServiceManifestName = heap.AddString(serviceManifestNameString);

    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    queryDescription->NodeName = heap.AddString(nodeName);

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        auto deployedServicePackageHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        healthPolicy->ToPublicApi(heap, *deployedServicePackageHealthPolicy);
        queryDescription->HealthPolicy = deployedServicePackageHealthPolicy.GetRawPointer();
    }

    wstring servicePackageActivationId;
    if (!ParseServicePackageActivationId(parser, servicePackageActivationId))
    {
        return false;
    }

    TestSession::WriteInfo(
            TraceSource,
            "GetDeployedServicePackageHealth(): applicationNameString:{0}, serviceManifestNameString={1}, servicePackageActivationId={2}.",
            applicationNameString,
            serviceManifestNameString,
            servicePackageActivationId);

    auto queryDescriptionEx1 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION_EX1>();
    queryDescriptionEx1->ServicePackageActivationId = heap.AddString(servicePackageActivationId);
    queryDescription->Reserved = queryDescriptionEx1.GetRawPointer();

    ComPointer<IFabricDeployedServicePackageHealthResult> deployedServicePackageHealthResult;
    return PerformHealthQueryClientOperation(
        parser,
        L"GetDeployedServicePackageHealth",
        [this, &queryDescription](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->BeginGetDeployedServicePackageHealth2(
                queryDescription.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [this, &deployedServicePackageHealthResult](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->EndGetDeployedServicePackageHealth2(context.GetRawPointer(), deployedServicePackageHealthResult.InitializationAddress());
        },
        [this, &parser, &deployedServicePackageHealthResult, &applicationNameString, &nodeName, &serviceManifestNameString, &servicePackageActivationId](bool expectEmpty, bool requiresValidation)
        {
            bool result = ValidateDeployedServicePackageHealth(
                parser,
                *deployedServicePackageHealthResult->get_DeployedServicePackageHealth(),
                applicationNameString,
                serviceManifestNameString,
                servicePackageActivationId,
                nodeName,
                expectEmpty,
                requiresValidation);

            deployedServicePackageHealthResult.Release();
            return result;
        });
}

bool TestFabricClientHealth::GetApplicationHealth(StringCollection const & params)
{
    CommandLineParser parser(params, 0);
    ScopedHeap heap;
    auto queryDescription = heap.AddItem<FABRIC_APPLICATION_HEALTH_QUERY_DESCRIPTION>();

    wstring eventsFilterString;
    if (parser.TryGetString(L"eventsfilter", eventsFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(eventsFilterString, filter))
        {
            return false;
        }

        auto eventsFilter = heap.AddItem<FABRIC_HEALTH_EVENTS_FILTER>();
        eventsFilter->HealthStateFilter = filter;
        queryDescription->EventsFilter = eventsFilter.GetRawPointer();
    }

    wstring servicesFilterString;
    if (parser.TryGetString(L"servicesfilter", servicesFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(servicesFilterString, filter))
        {
            return false;
        }

        auto servicesFilter = heap.AddItem<FABRIC_SERVICE_HEALTH_STATES_FILTER>();
        servicesFilter->HealthStateFilter = filter;
        queryDescription->ServicesFilter = servicesFilter.GetRawPointer();
    }

    wstring deployedApplicationsFilterString;
    if (parser.TryGetString(L"deployedapplicationsfilter", deployedApplicationsFilterString))
    {
        DWORD filter;
        if (!GetHealthStateFilter(deployedApplicationsFilterString, filter))
        {
            return false;
        }

        auto deployedApplicationsFilter = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_STATES_FILTER>();
        deployedApplicationsFilter->HealthStateFilter = filter;
        queryDescription->DeployedApplicationsFilter = deployedApplicationsFilter.GetRawPointer();
    }

    wstring applicationNameString;
    if (!parser.TryGetString(L"appname", applicationNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }
    queryDescription->ApplicationName = heap.AddString(applicationNameString);

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        auto applicationHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        healthPolicy->ToPublicApi(heap, *applicationHealthPolicy);
        queryDescription->HealthPolicy = applicationHealthPolicy.GetRawPointer();
    }

    wstring excludeHealthStatsString;
    if (parser.TryGetString(L"excludeHealthStats", excludeHealthStatsString))
    {
        bool excludeHealthStats = parser.GetBool(L"excludeHealthStats");
        auto statsFilter = heap.AddItem<FABRIC_APPLICATION_HEALTH_STATISTICS_FILTER>();
        statsFilter->ExcludeHealthStatistics = excludeHealthStats ? TRUE : FALSE;

        auto ex1 = heap.AddItem<FABRIC_APPLICATION_HEALTH_QUERY_DESCRIPTION_EX1>();
        ex1->HealthStatisticsFilter = statsFilter.GetRawPointer();
        queryDescription->Reserved = ex1.GetRawPointer();
    }

    ComPointer<IFabricApplicationHealthResult> applicationHealthResult;
    return PerformHealthQueryClientOperation(
        parser,
        L"GetApplicationHealth",
        [this, &queryDescription](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->BeginGetApplicationHealth2(
                queryDescription.GetRawPointer(),
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
        [this, &applicationHealthResult](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return healthClient_->EndGetApplicationHealth2(context.GetRawPointer(), applicationHealthResult.InitializationAddress());
        },
        [this, &parser, &applicationHealthResult, &applicationNameString](bool expectEmpty, bool requiresValidation)
        {
            bool result = ValidateApplicationHealth(parser, *applicationHealthResult->get_ApplicationHealth(), applicationNameString, expectEmpty, requiresValidation);
            applicationHealthResult.Release();
            return result;
        });
}

bool TestFabricClientHealth::QueryHealthList(StringCollection const & params)
{
    if (params.size() < 1)
    {
        return false;
    }

    wstring queryType = params[0];
    CommandLineParser parser(params, 0);

    QueryArgumentMap queryArgs;
    Query::QueryNames::Enum queryName;

    if (StringUtility::AreEqualCaseInsensitive(queryType, L"nodes"))
    {
        queryName = Query::QueryNames::GetAggregatedNodeHealthList;
        if (!CreateNodeHealthQueryList(parser, queryArgs))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"partitions"))
    {
        queryName = Query::QueryNames::GetAggregatedServicePartitionHealthList;
        if (!CreatePartitionHealthQueryList(parser, queryArgs))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"replicas"))
    {
        queryName = Query::QueryNames::GetAggregatedServicePartitionReplicaHealthList;
        if (!CreateReplicaHealthQueryList(parser, queryArgs))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"services"))
    {
        queryName = Query::QueryNames::GetAggregatedServiceHealthList;
        if (!CreateServiceHealthQueryList(parser, queryArgs))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"applications"))
    {
        queryName = Query::QueryNames::GetAggregatedApplicationHealthList;
        if (!CreateApplicationHealthQueryList(parser, queryArgs))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"deployedapplications"))
    {
        queryName = Query::QueryNames::GetAggregatedDeployedApplicationHealthList;
        if (!CreateDeployedApplicationHealthQueryList(parser, queryArgs))
        {
            return false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryType, L"deployedservicepackages"))
    {
        queryName = Query::QueryNames::GetAggregatedDeployedServicePackageHealthList;
        if (!CreateDeployedServicePackageHealthQueryList(parser, queryArgs))
        {
            return false;
        }
    }
    else
    {
        TestSession::WriteError(TraceSource, "Unknown type {0}", queryType);
        return false;
    }

    wstring continuationToken;
    if (parser.TryGetString(L"continuationtoken", continuationToken))
    {
        queryArgs.Insert(QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);
    }

    auto internalQueryClient = TestFabricClientQuery::CreateInternalFabricQueryClient(FABRICSESSION.FabricDispatcher.Federation);

    int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    bool success = false;
    wstring queryNameString = wformatString(queryName);
    AutoResetEvent waitEvent;
    auto defaultNamingOperationTimeoutBuffer = TimeSpan::FromSeconds(10);

    do
    {
        --remainingRetries;
        auto asyncOperation = internalQueryClient->BeginInternalQuery(
            queryNameString,
            queryArgs,
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&waitEvent](Common::AsyncOperationSPtr const &) { waitEvent.Set(); },
            internalQueryClient.get_Root()->CreateAsyncOperationRoot());

        TimeSpan timeToWait = FabricTestSessionConfig::GetConfig().NamingOperationTimeout + defaultNamingOperationTimeoutBuffer;
        if (!waitEvent.WaitOne(timeToWait))
        {
            TestSession::FailTest(
                "BeginInternalQuery did not complete in {0} secs although a timeout of {1} was provided",
                timeToWait,
                FabricTestSessionConfig::GetConfig().NamingOperationTimeout);
        }

        QueryResult queryResult;
        auto error = internalQueryClient->EndInternalQuery(asyncOperation, queryResult);
        if (!error.IsSuccess())
        {
            TestSession::WriteWarning(TraceSource, "Query failed with error = {0}", error);
            Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
            continue;
        }
        else
        {
            if (!VerifyHealthQueryListResult(parser, queryName, queryResult))
            {
                TestSession::WriteWarning(TraceSource, "Health query list result verification failed");
                Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
                continue;
            }

            success = true;
        }

    } while (!success && remainingRetries >= 0);

    if (!success)
    {
        TestSession::FailTest("QueryHealthList failed");
    }

    return true;
}

Management::HealthManager::EntityKind::Enum GetEntityKind(FABRIC_HEALTH_REPORT_KIND kind)
{
    switch (kind)
    {
    case FABRIC_HEALTH_REPORT_KIND_NODE:
        return EntityKind::Node;
    case FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA:
    case FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE:
        return EntityKind::Replica;
    case FABRIC_HEALTH_REPORT_KIND_PARTITION:
        return EntityKind::Partition;
    case FABRIC_HEALTH_REPORT_KIND_SERVICE:
        return EntityKind::Service;
    case FABRIC_HEALTH_REPORT_KIND_APPLICATION:
        return EntityKind::Application;
    case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION:
        return EntityKind::DeployedApplication;
    case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE:
        return EntityKind::DeployedServicePackage;
    case FABRIC_HEALTH_REPORT_KIND_CLUSTER:
        return EntityKind::Cluster;
    default:
        TestSession::FailTest("{0}: Unsupported entity kind", static_cast<int>(kind));
    }
}

template<class TResultType>
bool CheckAggregatedHealthQueryResult(__in CommandLineParser & parser, FABRIC_HEALTH_REPORT_KIND kind, __in QueryResult & queryResult)
{
    bool requiresValidation = false;
    wstring expectedHealthStateString;
    wstring expectedHealthStatesString;
    wstring expectedContinuationToken;
    wstring expectedPagingStatusString;
    requiresValidation |= parser.TryGetString(L"expectedhealthstate", expectedHealthStateString);
    requiresValidation |= parser.TryGetString(L"expectedstates", expectedHealthStatesString);
    requiresValidation |= parser.TryGetString(L"expectedcontinuationtoken", expectedContinuationToken);
    requiresValidation |= parser.TryGetString(L"expectedpagingstatus", expectedPagingStatusString);

    bool expectEmpty = parser.GetBool(L"expectempty") | parser.GetBool(L"expectedempty");

    vector<TResultType> aggregatedHealthList;
    auto error = queryResult.MoveList<TResultType>(aggregatedHealthList);
    TestSession::FailTestIfNot(error.IsSuccess(), "QueryHealthList: MoveList returned error {0}", error);

    if (expectEmpty)
    {
        if (aggregatedHealthList.size() != 0u)
        {
            TestSession::WriteWarning(TraceSource, "Unexpected health query result count {0}.", aggregatedHealthList.size());
            return false;
        }

        TestSession::WriteInfo(TraceSource, "QueryHealthList: Received empty list...");
        return true;
    }

    wstring resultString;
    StringWriter writer(resultString);
    HealthStateCount counts;
    for (auto it = aggregatedHealthList.begin(); it != aggregatedHealthList.end(); ++it)
    {
        counts.Add(it->AggregatedHealthState);
        writer.WriteLine("{0}", it->ToString());
    }

    HealthStateCountMap results;
    results.insert(make_pair(GetEntityKind(kind), move(counts)));

    bool success = TestFabricClientHealth::VerifyAggregatedHealthStates(parser, results);

    auto pagingStatus = queryResult.MovePagingStatus();
    bool expectedPagingStatus = parser.GetBool(L"expectedpagingstatus");
    if (expectedContinuationToken.empty())
    {
        // Check that no continuation token is received
        if (pagingStatus && !pagingStatus->ContinuationToken.empty())
        {
            if (requiresValidation && !expectedPagingStatus)
            {
                TestSession::WriteWarning(TraceSource, "Unexpected continuation token {0}, expected none.", pagingStatus->ContinuationToken);
                return false;
            }
            else
            {
                writer.WriteLine("ContinuationToken: {0}", pagingStatus->ContinuationToken);
            }
        }
        else if (expectedPagingStatus)
        {
            TestSession::WriteWarning(TraceSource, "Expected paging status not received.");
            return false;
        }
    }
    else
    {
        // Check expected continuation token
        if (!pagingStatus)
        {
            TestSession::WriteWarning(TraceSource, "Expected continuation token {0} not received.", expectedContinuationToken);
            return false;
        }

        if (expectedContinuationToken != pagingStatus->ContinuationToken)
        {
            TestSession::WriteWarning(TraceSource, "Continuation token mismatch: expected {0}, actual {1}.", expectedContinuationToken, pagingStatus->ContinuationToken);
            return false;
        }
    }

    if (!requiresValidation || !success)
    {
        TestSession::WriteInfo(TraceSource, "{0}", resultString);
    }
    else
    {
        TestSession::WriteNoise(TraceSource, "{0}", resultString);
    }

    return success;
}

bool TestFabricClientHealth::VerifyHealthQueryListResult(CommandLineParser & parser, Query::QueryNames::Enum queryName, QueryResult & queryResult)
{
    TestSession::FailTestIf(
        queryResult.ResultKind != FABRIC_QUERY_RESULT_KIND_LIST,
        "Query result null or invalid.");
    bool success;
    switch (queryName)
    {
    case QueryNames::GetAggregatedNodeHealthList:
        success = CheckAggregatedHealthQueryResult<NodeAggregatedHealthState>(parser, FABRIC_HEALTH_REPORT_KIND_NODE, queryResult);
        break;
    case QueryNames::GetAggregatedServicePartitionReplicaHealthList:
        success = CheckAggregatedHealthQueryResult<ReplicaAggregatedHealthState>(parser, FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA, queryResult);
        break;
    case QueryNames::GetAggregatedServicePartitionHealthList:
        success = CheckAggregatedHealthQueryResult<PartitionAggregatedHealthState>(parser, FABRIC_HEALTH_REPORT_KIND_PARTITION, queryResult);
        break;
    case QueryNames::GetAggregatedServiceHealthList:
        success = CheckAggregatedHealthQueryResult<ServiceAggregatedHealthState>(parser, FABRIC_HEALTH_REPORT_KIND_SERVICE, queryResult);
        break;
    case QueryNames::GetAggregatedDeployedServicePackageHealthList:
        success = CheckAggregatedHealthQueryResult<DeployedServicePackageAggregatedHealthState>(parser, FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE, queryResult);
        break;
    case  QueryNames::GetAggregatedDeployedApplicationHealthList:
        success = CheckAggregatedHealthQueryResult<DeployedApplicationAggregatedHealthState>(parser, FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION, queryResult);
        break;
    case QueryNames::GetAggregatedApplicationHealthList:
        success = CheckAggregatedHealthQueryResult<ApplicationAggregatedHealthState>(parser, FABRIC_HEALTH_REPORT_KIND_APPLICATION, queryResult);
        break;
    default:
        TestSession::FailTest("Invalid query name {0}", queryName);
    }

    return success;
}

bool TestFabricClientHealth::CreateNodeHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs)
{
    std::unique_ptr<ClusterHealthPolicy> healthPolicy;
    if (!ParseClusterHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);

        queryArgs.Insert(Query::QueryResourceProperties::Health::ClusterHealthPolicy, healthPolicyString);
    }

    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        queryArgs.Insert(Query::QueryResourceProperties::Node::Name, nodeName);
    }

    // No required parameters
    return true;
}

bool TestFabricClientHealth::CreateReplicaHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs)
{
    Common::Guid partitionGuid;
    if (!ParsePartitionId(parser, partitionGuid))
    {
        return false;
    }

    queryArgs.Insert(Query::QueryResourceProperties::Partition::PartitionId, partitionGuid.ToString());

    FABRIC_REPLICA_ID replicaId;
    FABRIC_INSTANCE_ID replicaInstanceId;
    if (ParseReplicaId(parser, partitionGuid, replicaId, replicaInstanceId))
    {
        queryArgs.Insert(QueryResourceProperties::Replica::ReplicaId, wformatString(replicaId));
    }

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        queryArgs.Insert(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, healthPolicyString);
    }

    return true;
}

bool TestFabricClientHealth::CreatePartitionHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs)
{
    bool found = false;
    wstring serviceNameArg;

    if (parser.TryGetString(L"servicename", serviceNameArg, wstring()))
    {
        queryArgs.Insert(Query::QueryResourceProperties::Service::ServiceName, serviceNameArg);
        found = true;
    }

    Guid partitionGuid;
    if (ParsePartitionId(parser, partitionGuid))
    {
        queryArgs.Insert(QueryResourceProperties::Partition::PartitionId, wformatString(partitionGuid));
        found = true;
    }

    if (!found)
    {
        TestSession::WriteError(TraceSource, "Did not find any of the servicename or partitionId parameters. At least one of them must be present.");
        return false;
    }

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        queryArgs.Insert(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, healthPolicyString);
    }

    return true;
}

bool TestFabricClientHealth::CreateDeployedServicePackageHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs)
{
    wstring appName;
    if (!parser.TryGetString(L"appname", appName, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    queryArgs.Insert(Query::QueryResourceProperties::Node::Name, nodeName);
    queryArgs.Insert(Query::QueryResourceProperties::Application::ApplicationName, appName);

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        queryArgs.Insert(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, healthPolicyString);
    }

    return true;
}

bool TestFabricClientHealth::CreateDeployedApplicationHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs)
{
    wstring appName;
    if (!parser.TryGetString(L"appname", appName, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    queryArgs.Insert(Query::QueryResourceProperties::Application::ApplicationName, appName);

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        queryArgs.Insert(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, healthPolicyString);
    }

    return true;
}

bool TestFabricClientHealth::CreateServiceHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs)
{
    wstring appName;
    if (!parser.TryGetString(L"appname", appName, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    queryArgs.Insert(Query::QueryResourceProperties::Application::ApplicationName, move(appName));

    wstring serviceName;
    if (parser.TryGetString(L"servicename", serviceName, wstring()))
    {
        queryArgs.Insert(Query::QueryResourceProperties::Service::ServiceName, move(serviceName));
    }

    wstring serviceTypeName;
    if (parser.TryGetString(L"servicetypename", serviceTypeName, wstring()))
    {
        queryArgs.Insert(Query::QueryResourceProperties::ServiceType::ServiceTypeName, move(serviceTypeName));
    }

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        queryArgs.Insert(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, healthPolicyString);
    }

    return true;
}

bool TestFabricClientHealth::CreateApplicationHealthQueryList(CommandLineParser & parser, ServiceModel::QueryArgumentMap & queryArgs)
{
    std::unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (!ParseApplicationHealthPolicy(parser, healthPolicy))
    {
        return false;
    }

    if (healthPolicy)
    {
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Use policy {0}", healthPolicyString);
        queryArgs.Insert(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, healthPolicyString);
    }

    wstring appName(L"");
    if (parser.TryGetString(L"appname", appName))
    {
        queryArgs.Insert(Query::QueryResourceProperties::Application::ApplicationName, move(appName));
    }

    wstring appTypeName(L"");
    if (parser.TryGetString(L"apptypename", appTypeName))
    {
        queryArgs.Insert(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, move(appTypeName));
    }

    wstring applicationDefinitionKindFilter(L"");
    if (parser.TryGetString(L"applicationdefinitionkindfilter", applicationDefinitionKindFilter))
    {
        queryArgs.Insert(Query::QueryResourceProperties::Deployment::ApplicationDefinitionKindFilter, move(applicationDefinitionKindFilter));
    }

    // No required parameters
    return true;
}

bool TestFabricClientHealth::GetHealthStateFilter(std::wstring const & healthStateFilterString, __out DWORD & filter)
{
    filter = FABRIC_HEALTH_STATE_FILTER_DEFAULT;
    vector<wstring> tokens;
    StringUtility::Split<wstring>(healthStateFilterString, tokens, L"|");
    for (auto it = tokens.begin(); it != tokens.end(); ++it)
    {
        DWORD filterValueToken = FABRIC_HEALTH_STATE_FILTER_DEFAULT;
        FABRIC_HEALTH_STATE healthState = GetHealthState(*it);
        switch (healthState)
        {
        case FABRIC_HEALTH_STATE_OK: filterValueToken = FABRIC_HEALTH_STATE_FILTER_OK;
            break;
        case FABRIC_HEALTH_STATE_WARNING: filterValueToken = FABRIC_HEALTH_STATE_FILTER_WARNING; break;
        case FABRIC_HEALTH_STATE_ERROR: filterValueToken = FABRIC_HEALTH_STATE_FILTER_ERROR; break;
        case FABRIC_HEALTH_STATE_INVALID:
            if (*it == L"all")
            {
                filterValueToken = FABRIC_HEALTH_STATE_FILTER_ALL;
            }
            else if (*it == L"none")
            {
                filterValueToken = FABRIC_HEALTH_STATE_FILTER_NONE;
            }
            else
            {
                TestSession::WriteInfo(TraceSource, "GetHealthStateFilter token {0} not supported in {1}", *it, healthStateFilterString);
            }
            break;
        default:
            TestSession::WriteInfo(TraceSource, "GetHealthStateFilter token {0} not supported in {1}", *it, healthStateFilterString);
            return false;
        }

        if (filter == FABRIC_HEALTH_STATE_FILTER_DEFAULT)
        {
            filter = filterValueToken;
        }
        else
        {
            filter |= filterValueToken;
        }
    }

    TestSession::WriteNoise(TraceSource, "GetHealthStateFilter: {0} = {1}", healthStateFilterString, filter);
    return true;
}

FABRIC_HEALTH_STATE TestFabricClientHealth::GetHealthState(std::wstring const & healthStateString)
{
    FABRIC_HEALTH_STATE healthState = FABRIC_HEALTH_STATE_INVALID;
    if (StringUtility::AreEqualCaseInsensitive(healthStateString, L"ok"))
    {
        healthState = FABRIC_HEALTH_STATE_OK;
    }
    else if (StringUtility::AreEqualCaseInsensitive(healthStateString, L"warning"))
    {
        healthState = FABRIC_HEALTH_STATE_WARNING;
    }
    else if (StringUtility::AreEqualCaseInsensitive(healthStateString, L"error"))
    {
        healthState = FABRIC_HEALTH_STATE_ERROR;
    }
    else if (StringUtility::AreEqualCaseInsensitive(healthStateString, L"unknown"))
    {
        healthState = FABRIC_HEALTH_STATE_UNKNOWN;
    }

    return healthState;
}

std::wstring TestFabricClientHealth::GetHealthStateString(FABRIC_HEALTH_STATE healthState)
{
    switch(healthState)
    {
    case FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_OK:
        return L"ok";
    case FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_WARNING:
        return L"warning";
    case FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR:
        return L"error";
    case FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_UNKNOWN:
        return L"unknown";
    default:
        return L"invalid";
    }
}

bool TestFabricClientHealth::ParseCommonHealthInformation(CommandLineParser & parser, FABRIC_SEQUENCE_NUMBER & sequenceNumber, std::wstring & sourceId, FABRIC_HEALTH_STATE & healthState, std::wstring & property, int64 & timeToLiveSeconds, std::wstring & description, bool & removeWhenExpired)
{
    parser.TryGetString(L"property", property, L"FabricTest_Property");
    parser.TryGetString(L"sourceid", sourceId, L"FabricTest_Source");

    int64 descriptionLength;
    if (parser.TryGetInt64(L"descriptionlength", descriptionLength))
    {
        // Set description as a string with required length
        description = wstring(descriptionLength, L'a');
        TestSession::WriteInfo(TraceSource, "Generate description based on descriptionLength, with {0} characters", description.size());
    }
    else
    {
        parser.TryGetString(L"description", description, L"FabricTest_Description");
    }

    // Use Auto as default so the system generates a sequence number
    parser.TryGetInt64(L"sequencenumber", sequenceNumber, FABRIC_AUTO_SEQUENCE_NUMBER);

    wstring ttlString;
    if (parser.TryGetString(L"timetoliveseconds", ttlString))
    {
        if (ttlString == L"infinite")
        {
            timeToLiveSeconds = FABRIC_HEALTH_REPORT_INFINITE_TTL;
        }
        else if (!parser.TryGetInt64(L"timetoliveseconds", timeToLiveSeconds, 60 * 60))
        {
            TestSession::WriteInfo(TraceSource, "Invalid TTL specified: {0}. Expected int64 of infinite.", ttlString);
            return false;
        }
    }
    else
    {
        // Use default ttl large enough for events not to expire before validation in general case
        timeToLiveSeconds = 60 * 60;
    }

    wstring healthStateString;
    if (!parser.TryGetString(L"healthstate", healthStateString, L""))
    {
        TestSession::WriteError(TraceSource, "Could not parse healthstate parameter");
        return false;
    }

    healthState = GetHealthState(healthStateString);

    if (healthState == FABRIC_HEALTH_STATE_INVALID)
    {
        TestSession::WriteError(TraceSource, "Unknown healthstate {0}", healthStateString);
        return false;
    }

    removeWhenExpired = parser.GetBool(L"transient");
    return true;
}

bool TestFabricClientHealth::ParsePartitionId(CommandLineParser & parser, __inout Common::Guid & partitionGuid)
{
    wstring partitionGuidString;
    wstring partitionIdString;
    if (parser.TryGetString(L"partitionguid", partitionGuidString, wstring()))
    {
        partitionGuid = Guid(partitionGuidString);
    }
    else
    {
        if (!parser.TryGetString(L"partitionid", partitionIdString, wstring()))
        {
            TestSession::WriteError(TraceSource, "Could not parse partitionid parameter");
            return false;
        }

        auto failoverUnit = FABRICSESSION.FabricDispatcher.GetFMFailoverUnitFromParam(partitionIdString);

        if (failoverUnit == nullptr)
        {
            TestSession::WriteError(TraceSource, "Could not find the partition");
            return false;
        }

        partitionGuid = failoverUnit->Id.Guid;
    }

    TestSession::WriteInfo(TraceSource, "Partition guid: {0}", partitionGuid.ToString());

    return true;
}

bool TestFabricClientHealth::ParseReplicaId(CommandLineParser & parser, __inout Common::Guid & partitionGuid, __inout FABRIC_REPLICA_ID & replicaId, __inout FABRIC_INSTANCE_ID & replicaInstanceId)
{
    wstring replicaIdString;
    if (parser.TryGetString(L"replicaid", replicaIdString, wstring()) ||
        parser.TryGetString(L"instanceid", replicaIdString, wstring()))
    {
        FailoverUnitSnapshotUPtr failoverUnit;
        auto replicaPtr = FABRICSESSION.FabricDispatcher.GetReplicaFromParam(replicaIdString, failoverUnit);

        if (replicaPtr == nullptr)
        {
            TestSession::WriteError(TraceSource, "Could not find the replica");
            return false;
        }

        auto & replica = *replicaPtr;

        partitionGuid = failoverUnit->Id.Guid;
        replicaId = replica.ReplicaId;

        // Replace the instance id with the test passed one
        wstring replicaInstanceIdString;
        if (parser.TryGetString(L"replica.instanceid", replicaInstanceIdString, wstring()))
        {
            if (!TryParseInt64(replicaInstanceIdString, replicaInstanceId))
            {
                return false;
            }
        }
        else
        {
            replicaInstanceId = replica.InstanceId;
        }
    }
    else if (parser.TryGetString(L"replica.id", replicaIdString, wstring()) ||
        parser.TryGetString(L"instance.id", replicaIdString, wstring()))
    {
        if (!TryParseInt64(replicaIdString, replicaId))
        {
            return false;
        }

        if (!ParsePartitionId(parser, partitionGuid))
        {
            return false;
        }

        wstring replicaInstanceIdString;
        if (parser.TryGetString(L"replica.instanceid", replicaInstanceIdString, wstring()))
        {
            if (!TryParseInt64(replicaInstanceIdString, replicaInstanceId))
            {
                return false;
            }
        }
        else
        {
            replicaInstanceId = FABRIC_INVALID_INSTANCE_ID;
        }
    }
    else
    {
        TestSession::WriteError(TraceSource, "Could not parse replicaid or replica.id parameter");
        return false;
    }

    TestSession::WriteInfo(TraceSource, "Partition Id = {0} Replica Id = {1} replicainstanceId = {2}", partitionGuid.ToString(), replicaId, replicaInstanceId);
    return true;
}

bool TestFabricClientHealth::ParseNodeHealthInformation(CommandLineParser & parser, NodeId & nodeId, FABRIC_NODE_INSTANCE_ID & nodeInstanceId, std::wstring & nodeName)
{
    wstring nodeIdString;
    if (!parser.TryGetString(L"nodeid", nodeIdString, wstring()))
    {
        TestSession::WriteInfo(TraceSource, "Could not parse nodeid parameter");
        return false;
    }

    nodeId = FABRICSESSION.FabricDispatcher.ParseNodeId(nodeIdString);
    if (!parser.TryGetString(L"nodename", nodeName))
    {
        nodeName = Federation::NodeIdGenerator::GenerateNodeName(nodeId);
    }

    nodeInstanceId = FABRIC_INVALID_NODE_INSTANCE_ID;
    int64 instance;
    if (parser.TryGetInt64(L"node.instanceid", instance))
    {
        nodeInstanceId = static_cast<FABRIC_NODE_INSTANCE_ID>(instance);
    }
    else
    {
        auto node = FabricTest::FabricTestSession::GetFabricTestSession().FabricDispatcher.Federation.GetNode(nodeId);
        if (node)
        {
            nodeInstanceId = node->Node->Instance.InstanceId;
        }

        if (nodeInstanceId == FABRIC_INVALID_NODE_INSTANCE_ID)
        {
            nodeInstanceId = static_cast<FABRIC_NODE_INSTANCE_ID>(DateTime::Now().Ticks);
        }
    }

    TestSession::WriteInfo(TraceSource, "Node Id = {0} instanceId = {1}", nodeId, nodeInstanceId);

    return true;
}

bool TestFabricClientHealth::CreateCommonHealthInformation(
    CommandLineParser & parser,
    ScopedHeap & heap,
    FABRIC_HEALTH_INFORMATION & healthInformation)
{
    wstring sourceId;
    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    FABRIC_HEALTH_STATE healthState;
    wstring property;
    int64 timeToLiveSeconds;
    wstring description;
    bool removeWhenExpired;
    if (!ParseCommonHealthInformation(parser, sequenceNumber, sourceId, healthState, property, timeToLiveSeconds, description, removeWhenExpired))
    {
        return false;
    }

    healthInformation.Description = heap.AddString(description);
    healthInformation.SourceId = heap.AddString(sourceId);
    healthInformation.Property = heap.AddString(property);
    healthInformation.TimeToLiveSeconds = static_cast<DWORD>(timeToLiveSeconds);
    healthInformation.SequenceNumber = sequenceNumber;
    healthInformation.State = healthState;
    healthInformation.RemoveWhenExpired = removeWhenExpired;

    return true;
}

bool TestFabricClientHealth::CreateClusterHealthReport(CommandLineParser & parser, ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport)
{
    auto clusterHealthReport = heap.AddItem<FABRIC_CLUSTER_HEALTH_REPORT>();

    ReferencePointer<FABRIC_HEALTH_INFORMATION> healthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    bool success = CreateCommonHealthInformation(parser, heap, *healthInformation);
    if (!success)
    {
        return success;
    }

    clusterHealthReport->HealthInformation = healthInformation.GetRawPointer();

    healthReport.Value = clusterHealthReport.GetRawPointer();
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_CLUSTER;

    return success;
}

bool TestFabricClientHealth::CreateReplicaHealthReport(CommandLineParser & parser, ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport)
{
    FABRIC_REPLICA_ID replicaId;
    FABRIC_INSTANCE_ID replicaInstanceId;
    Common::Guid partitionGuid;
    if (!ParseReplicaId(parser, partitionGuid, replicaId, replicaInstanceId))
    {
        return false;
    }

    auto replicaHealthReport = heap.AddItem<FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_REPORT>();
    replicaHealthReport->PartitionId = partitionGuid.AsGUID();
    replicaHealthReport->ReplicaId = replicaId;

    ReferencePointer<FABRIC_HEALTH_INFORMATION> healthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    bool success = CreateCommonHealthInformation(parser, heap, *healthInformation);
    if (!success)
    {
        return success;
    }

    replicaHealthReport->HealthInformation = healthInformation.GetRawPointer();

    healthReport.Value = replicaHealthReport.GetRawPointer();
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA;

    return success;
}

bool TestFabricClientHealth::CreateInstanceHealthReport(CommandLineParser & parser, ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport)
{
    FABRIC_REPLICA_ID replicaId;
    FABRIC_INSTANCE_ID replicaInstanceId;
    Common::Guid partitionGuid;
    if (!ParseReplicaId(parser, partitionGuid, replicaId, replicaInstanceId))
    {
        return false;
    }

    auto replicaHealthReport = heap.AddItem<FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH_REPORT>();
    replicaHealthReport->PartitionId = partitionGuid.AsGUID();
    replicaHealthReport->InstanceId = replicaId;

    ReferencePointer<FABRIC_HEALTH_INFORMATION> healthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    bool success = CreateCommonHealthInformation(parser, heap, *healthInformation);
    if (!success)
    {
        return success;
    }

    replicaHealthReport->HealthInformation = healthInformation.GetRawPointer();

    healthReport.Value = replicaHealthReport.GetRawPointer();
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE;

    return success;
}

bool TestFabricClientHealth::CreatePartitionHealthReport(CommandLineParser & parser, ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport)
{
    Common::Guid partitionGuid;
    if (!ParsePartitionId(parser, partitionGuid))
    {
        return false;
    }

    TestSession::WriteInfo(TraceSource, "Partition Id = {0}", partitionGuid.ToString());

    auto partitionHealthReport = heap.AddItem<FABRIC_PARTITION_HEALTH_REPORT>();
    partitionHealthReport->PartitionId = partitionGuid.AsGUID();

    ReferencePointer<FABRIC_HEALTH_INFORMATION> healthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    bool success = CreateCommonHealthInformation(parser, heap, *healthInformation);
    if (!success)
    {
        return success;
    }

    partitionHealthReport->HealthInformation = healthInformation.GetRawPointer();

    healthReport.Value = partitionHealthReport.GetRawPointer();
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_PARTITION;

    return success;
}

bool TestFabricClientHealth::CreateNodeHealthReport(CommandLineParser & parser, ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport)
{
    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    auto nodeHealthInformation = heap.AddItem<FABRIC_NODE_HEALTH_REPORT>();
    nodeHealthInformation->NodeName = heap.AddString(nodeName);

    ReferencePointer<FABRIC_HEALTH_INFORMATION> healthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    bool success = CreateCommonHealthInformation(parser, heap, *healthInformation);
    if (!success)
    {
        return success;
    }

    nodeHealthInformation->HealthInformation = healthInformation.GetRawPointer();

    healthReport.Value = nodeHealthInformation.GetRawPointer();
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_NODE;

    return success;
}

bool TestFabricClientHealth::CreateDeployedApplicationHealthReport(CommandLineParser & parser, ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport)
{
    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    wstring applicationNameString;
    if (!parser.TryGetString(L"appname", applicationNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    auto applicationName = heap.AddString(applicationNameString);

    auto deployedApplicationHealthInformation = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_REPORT>();
    deployedApplicationHealthInformation->ApplicationName = heap.AddString(applicationName);
    deployedApplicationHealthInformation->NodeName = heap.AddString(nodeName);

    ReferencePointer<FABRIC_HEALTH_INFORMATION> healthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    bool success = CreateCommonHealthInformation(parser, heap, *healthInformation);
    if (!success)
    {
        return success;
    }

    deployedApplicationHealthInformation->HealthInformation = healthInformation.GetRawPointer();

    healthReport.Value = deployedApplicationHealthInformation.GetRawPointer();
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION;

    return success;
}

bool TestFabricClientHealth::CreateDeployedServicePackageHealthReport(CommandLineParser & parser, ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport)
{
    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    wstring applicationNameString;
    if (!parser.TryGetString(L"appname", applicationNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    wstring serviceManifestNameString;
    if (!parser.TryGetString(L"servicemanifestname", serviceManifestNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse servicemanifestname parameter");
        return false;
    }

    auto deployedServicePackageHealthInformation = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_REPORT>();
    deployedServicePackageHealthInformation->ApplicationName = heap.AddString(applicationNameString);
    deployedServicePackageHealthInformation->ServiceManifestName = heap.AddString(serviceManifestNameString);
    deployedServicePackageHealthInformation->NodeName = heap.AddString(nodeName);

    ReferencePointer<FABRIC_HEALTH_INFORMATION> healthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    bool success = CreateCommonHealthInformation(parser, heap, *healthInformation);
    if (!success)
    {
        return success;
    }

    deployedServicePackageHealthInformation->HealthInformation = healthInformation.GetRawPointer();

    healthReport.Value = deployedServicePackageHealthInformation.GetRawPointer();
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE;

    return success;
}

bool TestFabricClientHealth::CreateApplicationHealthReport(CommandLineParser & parser, ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport)
{
    wstring applicationNameString;
    if (!parser.TryGetString(L"appname", applicationNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    auto applicationHealthInformation = heap.AddItem<FABRIC_APPLICATION_HEALTH_REPORT>();
    applicationHealthInformation->ApplicationName = heap.AddString(applicationNameString);

    ReferencePointer<FABRIC_HEALTH_INFORMATION> healthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    bool success = CreateCommonHealthInformation(parser, heap, *healthInformation);
    if (!success)
    {
        return success;
    }

    applicationHealthInformation->HealthInformation = healthInformation.GetRawPointer();

    healthReport.Value = applicationHealthInformation.GetRawPointer();
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_APPLICATION;

    return success;
}

bool TestFabricClientHealth::CreateServiceHealthReport(CommandLineParser & parser, ScopedHeap & heap, FABRIC_HEALTH_REPORT & healthReport)
{
    wstring serviceNameString;
    if (!parser.TryGetString(L"servicename", serviceNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse servicename parameter");
        return false;
    }

    auto serviceHealthInformation = heap.AddItem<FABRIC_SERVICE_HEALTH_REPORT>();
    serviceHealthInformation->ServiceName = heap.AddString(serviceNameString);

    ReferencePointer<FABRIC_HEALTH_INFORMATION> healthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    bool success = CreateCommonHealthInformation(parser, heap, *healthInformation);
    if (!success)
    {
        return success;
    }

    serviceHealthInformation->HealthInformation = healthInformation.GetRawPointer();

    healthReport.Value = serviceHealthInformation.GetRawPointer();
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_SERVICE;

    return success;
}

bool TestFabricClientHealth::CreateApplicationHealthDeleteReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    wstring appNameString;
    if (!parser.TryGetString(L"appname", appNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    FABRIC_INSTANCE_ID instanceId;
    if (!parser.TryGetInt64(L"appinstanceid", instanceId, 1))
    {
        TestSession::WriteError(TraceSource, "Could not parse appinstanceid parameter");
        return false;
    }

    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    parser.TryGetInt64(L"sequencenumber", sequenceNumber, FABRIC_INVALID_SEQUENCE_NUMBER);
    wstring sourceId;
    parser.TryGetString(L"sourceid", sourceId, L"FabricTest_Source");

    healthReport = ServiceModel::HealthReport::CreateHealthInformationForDelete(
        EntityHealthInformation::CreateApplicationEntityHealthInformation(appNameString, instanceId),
        move(sourceId),
        sequenceNumber);

    return true;
}

bool TestFabricClientHealth::CreateServiceHealthDeleteReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    wstring serviceNameString;
    if (!parser.TryGetString(L"servicename", serviceNameString, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse servicename parameter");
        return false;
    }

    FABRIC_INSTANCE_ID instanceId;
    if (!parser.TryGetInt64(L"serviceinstanceid", instanceId, 1))
    {
        TestSession::WriteError(TraceSource, "Could not parse serviceinstanceid parameter");
        return false;
    }

    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    parser.TryGetInt64(L"sequencenumber", sequenceNumber, FABRIC_INVALID_SEQUENCE_NUMBER);
    wstring sourceId;
    parser.TryGetString(L"sourceid", sourceId, L"FabricTest_Source");

    healthReport = ServiceModel::HealthReport::CreateHealthInformationForDelete(
        EntityHealthInformation::CreateServiceEntityHealthInformation(serviceNameString, instanceId),
        move(sourceId),
        sequenceNumber);

    return true;
}

bool TestFabricClientHealth::CreateReplicaHealthDeleteReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    FABRIC_REPLICA_ID replicaId;
    FABRIC_INSTANCE_ID replicaInstanceId;
    Common::Guid partitionGuid;
    if (!ParseReplicaId(parser, partitionGuid, replicaId, replicaInstanceId))
    {
        return false;
    }

    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    parser.TryGetInt64(L"sequencenumber", sequenceNumber, FABRIC_INVALID_SEQUENCE_NUMBER);
    wstring sourceId;
    parser.TryGetString(L"sourceid", sourceId, L"FabricTest_Source");

    healthReport = ServiceModel::HealthReport::CreateHealthInformationForDelete(
        EntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(partitionGuid, replicaId, replicaInstanceId),
        move(sourceId),
        sequenceNumber);

    return true;
}

bool TestFabricClientHealth::CreateInstanceHealthDeleteReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    FABRIC_REPLICA_ID replicaId;
    FABRIC_INSTANCE_ID replicaInstanceId;
    Common::Guid partitionGuid;
    if (!ParseReplicaId(parser, partitionGuid, replicaId, replicaInstanceId))
    {
        return false;
    }

    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    parser.TryGetInt64(L"sequencenumber", sequenceNumber, FABRIC_INVALID_SEQUENCE_NUMBER);
    wstring sourceId;
    parser.TryGetString(L"sourceid", sourceId, L"FabricTest_Source");

    healthReport = ServiceModel::HealthReport::CreateHealthInformationForDelete(
        EntityHealthInformation::CreateStatelessInstanceEntityHealthInformation(partitionGuid, replicaId),
        move(sourceId),
        sequenceNumber);

    return true;
}

bool TestFabricClientHealth::CreatePartitionHealthDeleteReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    Common::Guid partitionGuid;
    if (!ParsePartitionId(parser, partitionGuid))
    {
        return false;
    }

    TestSession::WriteInfo(TraceSource, "Partition Id = {0}", partitionGuid.ToString());

    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    parser.TryGetInt64(L"sequencenumber", sequenceNumber, FABRIC_INVALID_SEQUENCE_NUMBER);
    wstring sourceId;
    parser.TryGetString(L"sourceid", sourceId, L"FabricTest_Source");

    healthReport = ServiceModel::HealthReport::CreateHealthInformationForDelete(
        EntityHealthInformation::CreatePartitionEntityHealthInformation(partitionGuid),
        move(sourceId),
        sequenceNumber);

    return true;
}

bool TestFabricClientHealth::CreateNodeHealthDeleteReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    parser.TryGetInt64(L"sequencenumber", sequenceNumber, FABRIC_INVALID_SEQUENCE_NUMBER);
    wstring sourceId;
    parser.TryGetString(L"sourceid", sourceId, L"FabricTest_Source");

    healthReport = ServiceModel::HealthReport::CreateHealthInformationForDelete(
        EntityHealthInformation::CreateNodeEntityHealthInformation(nodeId.IdValue, nodeName, nodeInstanceId),
        move(sourceId),
        sequenceNumber);

    return true;
}

bool TestFabricClientHealth::CreateDeployedApplicationHealthDeleteReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    wstring appName;
    if (!parser.TryGetString(L"appname", appName, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    FABRIC_INSTANCE_ID appInstanceId;
    parser.TryGetInt64(L"appinstanceid", appInstanceId, FABRIC_INVALID_INSTANCE_ID);

    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    parser.TryGetInt64(L"sequencenumber", sequenceNumber, FABRIC_INVALID_SEQUENCE_NUMBER);
    wstring sourceId;
    parser.TryGetString(L"sourceid", sourceId, L"FabricTest_Source");

    healthReport = ServiceModel::HealthReport::CreateHealthInformationForDelete(
        EntityHealthInformation::CreateDeployedApplicationEntityHealthInformation(appName, nodeId.IdValue, nodeName, appInstanceId),
        move(sourceId),
        sequenceNumber);

    return true;
}

bool TestFabricClientHealth::CreateDeployedServicePackageHealthDeleteReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    wstring serviceManifestName;
    if (!parser.TryGetString(L"servicemanifestname", serviceManifestName, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse servicemanifestname parameter");
        return false;
    }

    wstring appName;
    if (!parser.TryGetString(L"appname", appName, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    FABRIC_INSTANCE_ID servicePackageInstanceId;
    parser.TryGetInt64(L"servicepackageinstanceid", servicePackageInstanceId, FABRIC_INVALID_INSTANCE_ID);

    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    parser.TryGetInt64(L"sequencenumber", sequenceNumber, FABRIC_INVALID_SEQUENCE_NUMBER);
    wstring sourceId;
    parser.TryGetString(L"sourceid", sourceId, L"FabricTest_Source");

    wstring servicePackageActivationId;
    if (!ParseServicePackageActivationId(parser, servicePackageActivationId))
    {
        return false;
    }

    TestSession::WriteInfo(TraceSource, "CreateDeployedServicePackageHealthDeleteReport(): Using servicePackageActivationId=[{0}].", servicePackageActivationId);

    healthReport = ServiceModel::HealthReport::CreateHealthInformationForDelete(
        EntityHealthInformation::CreateDeployedServicePackageEntityHealthInformation(appName, serviceManifestName, servicePackageActivationId, nodeId.IdValue, nodeName, servicePackageInstanceId),
        move(sourceId),
        sequenceNumber);

    return true;
}

// Internal HealthReport - with attributes
bool TestFabricClientHealth::CreateInternalClusterHealthReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    wstring sourceId;
    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    FABRIC_HEALTH_STATE healthState;
    wstring property;
    int64 timeToLiveSeconds;
    wstring description;
    bool removeWhenExpired;
    if (!ParseCommonHealthInformation(parser, sequenceNumber, sourceId, healthState, property, timeToLiveSeconds, description, removeWhenExpired))
    {
        return false;
    }

    AttributeList attributeList;

    healthReport = HealthReport(
        EntityHealthInformation::CreateClusterEntityHealthInformation(),
        sourceId,
        property,
        TimeSpan::FromSeconds(static_cast<double>(timeToLiveSeconds)),
        healthState,
        description,
        sequenceNumber,
        removeWhenExpired,
        move(attributeList));

    return true;
}

bool TestFabricClientHealth::CreateInternalReplicaHealthReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    FABRIC_REPLICA_ID replicaId;
    FABRIC_INSTANCE_ID replicaInstanceId;
    Common::Guid partitionGuid;
    if (!ParseReplicaId(parser, partitionGuid, replicaId, replicaInstanceId))
    {
        return false;
    }

    wstring sourceId;
    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    FABRIC_HEALTH_STATE healthState;
    wstring property;
    int64 timeToLiveSeconds;
    wstring description;
    bool removeWhenExpired;
    if (!ParseCommonHealthInformation(parser, sequenceNumber, sourceId, healthState, property, timeToLiveSeconds, description, removeWhenExpired))
    {
        return false;
    }

    AttributeList attributeList;

    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        attributeList.AddAttribute(HealthAttributeNames::NodeId, wformatString(nodeId));
        attributeList.AddAttribute(HealthAttributeNames::NodeInstanceId, wformatString(nodeInstanceId));
    }

    healthReport = HealthReport(
        EntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(partitionGuid, replicaId,  replicaInstanceId),
        sourceId,
        property,
        TimeSpan::FromSeconds(static_cast<double>(timeToLiveSeconds)),
        healthState,
        description,
        sequenceNumber,
        removeWhenExpired,
        move(attributeList));

    return true;
}

bool TestFabricClientHealth::CreateInternalInstanceHealthReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    FABRIC_REPLICA_ID replicaId;
    FABRIC_INSTANCE_ID replicaInstanceId;
    Common::Guid partitionGuid;
    if (!ParseReplicaId(parser, partitionGuid, replicaId, replicaInstanceId))
    {
        return false;
    }

    wstring sourceId;
    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    FABRIC_HEALTH_STATE healthState;
    wstring property;
    int64 timeToLiveSeconds;
    wstring description;
    bool removeWhenExpired;
    if (!ParseCommonHealthInformation(parser, sequenceNumber, sourceId, healthState, property, timeToLiveSeconds, description, removeWhenExpired))
    {
        return false;
    }

    AttributeList attributeList;

    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        attributeList.AddAttribute(HealthAttributeNames::NodeId, wformatString(nodeId));
        attributeList.AddAttribute(HealthAttributeNames::NodeInstanceId, wformatString(nodeInstanceId));
    }

    healthReport = HealthReport(
        EntityHealthInformation::CreateStatelessInstanceEntityHealthInformation(partitionGuid, replicaId),
        sourceId,
        property,
        TimeSpan::FromSeconds(static_cast<double>(timeToLiveSeconds)),
        healthState,
        description,
        sequenceNumber,
        removeWhenExpired,
        move(attributeList));

    return true;
}

bool TestFabricClientHealth::CreateInternalPartitionHealthReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    Common::Guid partitionGuid;
    if (!ParsePartitionId(parser, partitionGuid))
    {
        return false;
    }

    wstring sourceId;
    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    FABRIC_HEALTH_STATE healthState;
    wstring property;
    int64 timeToLiveSeconds;
    wstring description;
    bool removeWhenExpired;
    if (!ParseCommonHealthInformation(parser, sequenceNumber, sourceId, healthState, property, timeToLiveSeconds, description, removeWhenExpired))
    {
        return false;
    }

    AttributeList attributeList;

    wstring serviceName;
    if (parser.TryGetString(L"servicename", serviceName, wstring()))
    {
        attributeList.AddAttribute(HealthAttributeNames::ServiceName, serviceName);
    }

    healthReport = HealthReport(
        EntityHealthInformation::CreatePartitionEntityHealthInformation(partitionGuid),
        sourceId,
        property,
        TimeSpan::FromSeconds(static_cast<double>(timeToLiveSeconds)),
        healthState,
        description,
        sequenceNumber,
        removeWhenExpired,
        move(attributeList));

    return true;
}

bool TestFabricClientHealth::CreateInternalNodeHealthReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    wstring sourceId;
    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    FABRIC_HEALTH_STATE healthState;
    wstring property;
    int64 timeToLiveSeconds;
    wstring description;
    bool removeWhenExpired;
    if (!ParseCommonHealthInformation(parser, sequenceNumber, sourceId, healthState, property, timeToLiveSeconds, description, removeWhenExpired))
    {
        return false;
    }

    TimeSpan ttl = (timeToLiveSeconds == FABRIC_HEALTH_REPORT_INFINITE_TTL) ? TimeSpan::MaxValue : TimeSpan::FromSeconds(static_cast<double>(timeToLiveSeconds));

    AttributeList attributeList;

    wstring ipAddressOrFqdn;
    if (parser.TryGetString(L"ipaddressorfqdn", ipAddressOrFqdn, wstring()))
    {
        attributeList.AddAttribute(HealthAttributeNames::IpAddressOrFqdn, ipAddressOrFqdn);
    }

    wstring upgradeDomain;
    if (parser.TryGetString(L"ud", upgradeDomain))
    {
        attributeList.AddAttribute(HealthAttributeNames::UpgradeDomain, upgradeDomain);
    }

    wstring faultDomain;
    if (parser.TryGetString(L"fd", faultDomain))
    {
        attributeList.AddAttribute(HealthAttributeNames::FaultDomain, faultDomain);
    }

    if (!nodeName.empty())
    {
        attributeList.AddAttribute(*HealthAttributeNames::NodeName, nodeName);
    }

    healthReport = HealthReport(
        EntityHealthInformation::CreateNodeEntityHealthInformation(nodeId.IdValue, nodeName, nodeInstanceId),
        sourceId,
        property,
        ttl,
        healthState,
        description,
        sequenceNumber,
        removeWhenExpired,
        move(attributeList));

    return true;
}

bool TestFabricClientHealth::CreateInternalDeployedServicePackageHealthReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    wstring serviceManifestName;
    if (!parser.TryGetString(L"servicemanifestname", serviceManifestName, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse servicemanifestname parameter");
        return false;
    }

    wstring appName;
    if (!parser.TryGetString(L"appname", appName, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    FABRIC_INSTANCE_ID servicePackageInstanceId;
    parser.TryGetInt64(L"servicepackageinstanceid", servicePackageInstanceId, FABRIC_INVALID_INSTANCE_ID);

    wstring sourceId;
    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    FABRIC_HEALTH_STATE healthState;
    wstring property;
    int64 timeToLiveSeconds;
    wstring description;
    bool removeWhenExpired;
    if (!ParseCommonHealthInformation(parser, sequenceNumber, sourceId, healthState, property, timeToLiveSeconds, description, removeWhenExpired))
    {
        return false;
    }

    AttributeList attributeList;
    if (!nodeName.empty())
    {
        attributeList.AddAttribute(*HealthAttributeNames::NodeName, nodeName);
    }

    if (nodeInstanceId != FABRIC_INVALID_NODE_INSTANCE_ID)
    {
        attributeList.AddAttribute(*HealthAttributeNames::NodeInstanceId, wformatString(nodeInstanceId));
    }

    wstring servicePackageActivationId;
    if (!ParseServicePackageActivationId(parser, servicePackageActivationId))
    {
        return false;
    }

    TestSession::WriteInfo(TraceSource, "CreateInternalDeployedServicePackageHealthReport(): Using servicePackageActivationId=[{0}].", servicePackageActivationId);

    healthReport = HealthReport(
        EntityHealthInformation::CreateDeployedServicePackageEntityHealthInformation(appName, serviceManifestName, servicePackageActivationId, nodeId.IdValue, nodeName, servicePackageInstanceId),
        sourceId,
        property,
        TimeSpan::FromSeconds(static_cast<double>(timeToLiveSeconds)),
        healthState,
        description,
        sequenceNumber,
        removeWhenExpired,
        move(attributeList));

    return true;
}

bool TestFabricClientHealth::CreateInternalDeployedApplicationHealthReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    wstring appName;
    if (!parser.TryGetString(L"appname", appName, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    FABRIC_INSTANCE_ID applicationInstanceId;
    parser.TryGetInt64(L"appinstanceid", applicationInstanceId, FABRIC_INVALID_INSTANCE_ID);

    wstring sourceId;
    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    FABRIC_HEALTH_STATE healthState;
    wstring property;
    int64 timeToLiveSeconds;
    wstring description;
    bool removeWhenExpired;
    if (!ParseCommonHealthInformation(parser, sequenceNumber, sourceId, healthState, property, timeToLiveSeconds, description, removeWhenExpired))
    {
        return false;
    }

    AttributeList attributeList;
    if (!nodeName.empty())
    {
        attributeList.AddAttribute(*HealthAttributeNames::NodeName, nodeName);
    }

    if (nodeInstanceId != FABRIC_INVALID_NODE_INSTANCE_ID)
    {
        attributeList.AddAttribute(*HealthAttributeNames::NodeInstanceId, wformatString(nodeInstanceId));
    }

    healthReport = HealthReport(
        EntityHealthInformation::CreateDeployedApplicationEntityHealthInformation(appName, nodeId.IdValue, nodeName, applicationInstanceId),
        sourceId,
        property,
        TimeSpan::FromSeconds(static_cast<double>(timeToLiveSeconds)),
        healthState,
        description,
        sequenceNumber,
        removeWhenExpired,
        move(attributeList));

    return true;
}

bool TestFabricClientHealth::CreateInternalApplicationHealthReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    wstring appName;
    if (!parser.TryGetString(L"appname", appName, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse appname parameter");
        return false;
    }

    wstring sourceId;
    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    FABRIC_HEALTH_STATE healthState;
    wstring property;
    int64 timeToLiveSeconds;
    wstring description;
    bool removeWhenExpired;
    if (!ParseCommonHealthInformation(parser, sequenceNumber, sourceId, healthState, property, timeToLiveSeconds, description, removeWhenExpired))
    {
        return false;
    }

    FABRIC_INSTANCE_ID applicationInstanceId;
    parser.TryGetInt64(L"appinstanceid", applicationInstanceId, FABRIC_INVALID_INSTANCE_ID);

    AttributeList attributeList;

    wstring appTypeName;
    if (parser.TryGetString(L"apptype", appTypeName, wstring()))
    {
        attributeList.AddAttribute(HealthAttributeNames::ApplicationTypeName, move(appTypeName));
    }

    wstring applicationDefinitionKind;
    if (parser.TryGetString(L"applicationdefinitionkind", applicationDefinitionKind, wstring()))
    {
        attributeList.AddAttribute(HealthAttributeNames::ApplicationDefinitionKind, move(applicationDefinitionKind));
    }

    std::unique_ptr<ApplicationHealthPolicy> healthPolicy = nullptr;
    if (ParseApplicationHealthPolicy(parser, healthPolicy) && (healthPolicy != nullptr))
    {
        wstring healthPolicyString = healthPolicy->ToString();
        TestSession::WriteNoise(TraceSource, "Add app policy {0}", healthPolicyString);
        attributeList.AddAttribute(HealthAttributeNames::ApplicationHealthPolicy, healthPolicyString);
    }

    healthReport = HealthReport(
        EntityHealthInformation::CreateApplicationEntityHealthInformation(appName, applicationInstanceId),
        sourceId,
        property,
        TimeSpan::FromSeconds(static_cast<double>(timeToLiveSeconds)),
        healthState,
        description,
        sequenceNumber,
        removeWhenExpired,
        move(attributeList));

    return true;
}

bool TestFabricClientHealth::CreateInternalServiceHealthReport(CommandLineParser & parser,  __inout ServiceModel::HealthReport & healthReport)
{
    wstring serviceName;
    if (!parser.TryGetString(L"servicename", serviceName, wstring()))
    {
        TestSession::WriteError(TraceSource, "Could not parse servicename parameter");
        return false;
    }

    FABRIC_INSTANCE_ID instanceId;
    parser.TryGetInt64(L"serviceinstanceid", instanceId, 1);

    wstring sourceId;
    FABRIC_SEQUENCE_NUMBER sequenceNumber;
    FABRIC_HEALTH_STATE healthState;
    wstring property;
    int64 timeToLiveSeconds;
    wstring description;
    bool removeWhenExpired;
    if (!ParseCommonHealthInformation(parser, sequenceNumber, sourceId, healthState, property, timeToLiveSeconds, description, removeWhenExpired))
    {
        return false;
    }

    AttributeList attributeList;
    wstring serviceTypeName;
    if (parser.TryGetString(L"servicetypename", serviceTypeName, wstring()))
    {
        attributeList.AddAttribute(HealthAttributeNames::ServiceTypeName, serviceTypeName);
    }

    wstring appName;
    if (parser.TryGetString(L"appname", appName, wstring()))
    {
        attributeList.AddAttribute(HealthAttributeNames::ApplicationName, appName);
    }

    healthReport = HealthReport(
        EntityHealthInformation::CreateServiceEntityHealthInformation(serviceName, instanceId),
        sourceId,
        property,
        TimeSpan::FromSeconds(static_cast<double>(timeToLiveSeconds)),
        healthState,
        description,
        sequenceNumber,
        removeWhenExpired,
        move(attributeList));

    return true;
}

// Parse health policies
bool TestFabricClientHealth::ParseClusterHealthPolicy(CommandLineParser & parser, __inout std::unique_ptr<ClusterHealthPolicy> & clusterHealthPolicy)
{
    wstring healthPolicy;
    if (!parser.TryGetString(L"clusterpolicy", healthPolicy, wstring()))
    {
        if (parser.TryGetString(L"jsonpolicy", healthPolicy))
        {
            clusterHealthPolicy = make_unique<ClusterHealthPolicy>();
            auto error = ClusterHealthPolicy::FromString(healthPolicy, *clusterHealthPolicy);
            TestSession::FailTestIfNot(error.IsSuccess(), "Failed to parse cluster health policy string {0} with {1}", healthPolicy, error);
        }

        return true;
    }

    vector<wstring> tokens;
    StringUtility::Split<wstring>(healthPolicy, tokens, L",");
    if (tokens.size() == 0 || tokens.size() > 4)
    {
        TestSession::WriteError(TraceSource, "Could not parse nodepolicy parameter - too many arguments {0}", healthPolicy);
        return false;
    }

    bool considerWarningAsError = false;

    int maxUnhealthyNodes = 0;
    int maxUnhealthyApplications = 0;
    for (auto it = tokens.begin(); it != tokens.end(); ++it)
    {
        if (StringUtility::AreEqualCaseInsensitive(*it, L"warningaserror"))
        {
            considerWarningAsError = true;
        }
        else if (StringUtility::StartsWithCaseInsensitive<wstring>(*it, L"maxunhealthynodespercent"))
        {
            vector<wstring> innerTokens;
            StringUtility::Split<wstring>(*it, innerTokens, L":");
            if (innerTokens.size() != 2u)
            {
                TestSession::WriteError(TraceSource, "Could not parse maxunhealthynodespercent in nodepolicy parameter - {0}", healthPolicy);
                return false;
            }

            maxUnhealthyNodes = Int32_Parse(innerTokens[1]);
        }
        else if (StringUtility::StartsWithCaseInsensitive<wstring>(*it, L"maxunhealthyappspercent"))
        {
            vector<wstring> innerTokens;
            StringUtility::Split<wstring>(*it, innerTokens, L":");
            if (innerTokens.size() != 2u)
            {
                TestSession::WriteError(TraceSource, "Could not parse maxunhealthyappspercent in nodepolicy parameter - {0}", healthPolicy);
                return false;
            }

            maxUnhealthyApplications = Int32_Parse(innerTokens[1]);
        }
        else
        {
            TestSession::WriteError(TraceSource, "Unrecognized input {0} in {1}", *it, healthPolicy);
            return false;
        }
    }

    BYTE maxPercentUnhealthyNodes = static_cast<BYTE>(maxUnhealthyNodes);
    BYTE maxPercentUnhealthyApplications = static_cast<BYTE>(maxUnhealthyApplications);
    clusterHealthPolicy = make_unique<ClusterHealthPolicy>(considerWarningAsError, maxPercentUnhealthyNodes, maxPercentUnhealthyApplications);
    return true;
}

bool TestFabricClientHealth::ParseApplicationHealthPolicy(CommandLineParser & parser, __inout std::unique_ptr<ApplicationHealthPolicy> & applicationHealthPolicy)
{
    wstring healthPolicy;
    if (!parser.TryGetString(L"apppolicy", healthPolicy, wstring()))
    {
        return true;
    }

    return ParseApplicationHealthPolicy(healthPolicy, applicationHealthPolicy);
}

bool TestFabricClientHealth::ParseApplicationHealthPolicy(std::wstring const & healthPolicy, __inout std::unique_ptr<ApplicationHealthPolicy> & applicationHealthPolicy)
{
    if (healthPolicy == L"default")
    {
        // Use default policy
        applicationHealthPolicy = make_unique<ApplicationHealthPolicy>();
        return true;
    }

    vector<wstring> tokens;
    StringUtility::Split<wstring>(healthPolicy, tokens, L",");
    if (tokens.size() == 0 || tokens.size() > 5)
    {
        TestSession::WriteError(TraceSource, "Could not parse apppolicy parameter - invalid argument count {0}", healthPolicy);
        return false;
    }

    bool considerWarningAsError = false;
    int maxUnhealthyPartitions = 0;
    int maxUnhealthyReplicas = 0;
    int maxUnhealthyServices = 0;
    int maxUnhealthyDeployedApps = 0;
    for (auto it = tokens.begin(); it != tokens.end(); ++it)
    {
        if (StringUtility::AreEqualCaseInsensitive(*it, L"warningaserror"))
        {
            considerWarningAsError = true;
        }
        else if (StringUtility::StartsWithCaseInsensitive<wstring>(*it, L"maxunhealthyreplicaspercent"))
        {
            vector<wstring> innerTokens;
            StringUtility::Split<wstring>(*it, innerTokens, L":");
            if (innerTokens.size() != 2u)
            {
                TestSession::WriteError(TraceSource, "Could not parse maxunhealthyreplicaspercent in apppolicy parameter - {0}", healthPolicy);
                return false;
            }

            maxUnhealthyReplicas = Int32_Parse(innerTokens[1]);
        }
        else if (StringUtility::StartsWithCaseInsensitive<wstring>(*it, L"maxunhealthypartitionspercent"))
        {
            vector<wstring> innerTokens;
            StringUtility::Split<wstring>(*it, innerTokens, L":");
            if (innerTokens.size() != 2u)
            {
                TestSession::WriteError(TraceSource, "Could not parse maxunhealthypartitionspercent in apppolicy parameter - {0}", healthPolicy);
                return false;
            }

            maxUnhealthyPartitions = Int32_Parse(innerTokens[1]);
        }
        else if (StringUtility::StartsWithCaseInsensitive<wstring>(*it, L"maxunhealthyservicespercent"))
        {
            vector<wstring> innerTokens;
            StringUtility::Split<wstring>(*it, innerTokens, L":");
            if (innerTokens.size() != 2u)
            {
                TestSession::WriteError(TraceSource, "Could not parse maxunhealthyservicespercent in apppolicy parameter - {0}", healthPolicy);
                return false;
            }

            maxUnhealthyServices = Int32_Parse(innerTokens[1]);
        }
        else if (StringUtility::StartsWithCaseInsensitive<wstring>(*it, L"maxunhealthydeployedapplicationspercent"))
        {
            vector<wstring> innerTokens;
            StringUtility::Split<wstring>(*it, innerTokens, L":");
            if (innerTokens.size() != 2u)
            {
                TestSession::WriteError(TraceSource, "Could not parse maxunhealthydeployedapplicationspercent in applicationpolicy parameter - {0}", healthPolicy);
                return false;
            }

            maxUnhealthyDeployedApps = Int32_Parse(innerTokens[1]);
        }
        else
        {
            TestSession::WriteError(TraceSource, "Unrecognized input {0} in {1}", *it, healthPolicy);
            return false;
        }
    }

    BYTE maxPercentUnhealthyServices = static_cast<BYTE>(maxUnhealthyServices);
    BYTE maxPercentUnhealthyPartitionsPerService = static_cast<BYTE>(maxUnhealthyPartitions);
    BYTE maxPercentUnhealthyDeployedApplications = static_cast<BYTE>(maxUnhealthyDeployedApps);
    BYTE maxPercentUnhealthyReplicasPerPartition = static_cast<BYTE>(maxUnhealthyReplicas);
    ServiceTypeHealthPolicy defaultServiceType(maxPercentUnhealthyServices, maxPercentUnhealthyPartitionsPerService, maxPercentUnhealthyReplicasPerPartition);

    applicationHealthPolicy = make_unique<ApplicationHealthPolicy>(
        considerWarningAsError, maxPercentUnhealthyDeployedApplications, defaultServiceType, ServiceTypeHealthPolicyMap());

    return true;
}

bool TestFabricClientHealth::ValidateClusterHealthChunk(CommandLineParser & parser, FABRIC_CLUSTER_HEALTH_CHUNK const & publicClusterStateChunk, bool requiresValidation)
{
    ClusterHealthChunk clusterHealthChunk;
    auto error = clusterHealthChunk.FromPublicApi(publicClusterStateChunk);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetClusterHealthChunk: ClusterHealthChunk.FromPublicApi failed with unexpected error = {0}", error);

    bool success = true;

    TestSession::WriteInfo(TraceSource, "GetClusterHealthChunk returned {0}", clusterHealthChunk);

    wstring expectedHealthStateString;
    if (parser.TryGetString(L"expectedhealthstate", expectedHealthStateString))
    {
        FABRIC_HEALTH_STATE expectedHealthState = GetHealthState(expectedHealthStateString);
        if (expectedHealthState != clusterHealthChunk.HealthState)
        {
            TestSession::WriteInfo(
                TraceSource,
                "Aggregated health state does not match expected:{0} actual:{1}",
                expectedHealthStateString,
                GetHealthStateString(clusterHealthChunk.HealthState));

            return false;
        }
    }

    wstring expectedAppsString;
    map<wstring, vector<wstring>> expectedAppChunks;
    if (parser.TryGetString(L"expectedapps", expectedAppsString))
    {
        vector<wstring> appInfos;
        StringUtility::Split<wstring>(expectedAppsString, appInfos, L";");
        for (auto const & appInfo : appInfos)
        {
            vector<wstring> appTokens;
            StringUtility::Split<wstring>(appInfo, appTokens, L",", false /*skipEmptyItems*/);
            TestSession::FailTestIf(appTokens.size() != 3, "Error parsing expectedapps {0}: {1} should contain app name, app type and health state", expectedAppsString, appInfo);
            expectedAppChunks.insert(pair<wstring, vector<wstring>>(appTokens[0], appTokens));
        }
    }

    HealthStateCountMap resultsPerChildrenType;

    HealthStateCount nodeResults;
    for (auto const & node : clusterHealthChunk.NodeHealthStateChunks.Items)
    {
        TestSession::FailTestIf(node.NodeName.empty(), "Node name should be set in node health state chunk");
        nodeResults.Add(node.HealthState);
    }

    ULONG nodeTotal = nodeResults.GetTotal();
    TestSession::FailTestIf(clusterHealthChunk.NodeHealthStateChunks.TotalCount < nodeTotal, "Node TotalCount {0} should be >= included item count {1}", clusterHealthChunk.NodeHealthStateChunks.TotalCount, nodeTotal);
    TestSession::FailTestIfNot(clusterHealthChunk.NodeHealthStateChunks.Count == nodeTotal, "Node Count {0} should be == included item count {1}", clusterHealthChunk.NodeHealthStateChunks.TotalCount, nodeTotal);

    HealthStateCount appResults;
    HealthStateCount serviceResults;
    HealthStateCount partitionResults;
    HealthStateCount replicaResults;
    HealthStateCount deployedAppResults;
    HealthStateCount deployedServicePackageResults;
    size_t verifiedAppChunks = 0;
    for (auto const & app : clusterHealthChunk.ApplicationHealthStateChunks.Items)
    {
        TestSession::FailTestIf(app.ApplicationName.empty(), "Application name should be set in app health state chunk {0}", app);

        auto itApp = expectedAppChunks.find(app.ApplicationName);
        if (itApp != expectedAppChunks.cend())
        {
            if (app.ApplicationTypeName != itApp->second[1])
            {
                TestSession::WriteInfo(TraceSource, "App type name mismatch: received {0}, expected {1}", app, itApp->second[1]);
                return false;
            }

            if (app.HealthState != GetHealthState(itApp->second[2]))
            {
                TestSession::WriteInfo(TraceSource, "App health state mismatch: received {0}, expected {1}", app, itApp->second[2]);
                return false;
            }

            ++verifiedAppChunks;
        }
        else if (!app.ApplicationName.empty() && app.ApplicationName != SystemServiceApplicationNameHelper::SystemServiceApplicationName)
        {
            TestSession::FailTestIf(app.ApplicationTypeName.empty(), "Application type name should be set in app health state chunk {0}", app);
        }

        appResults.Add(app.HealthState);

        size_t serviceCount = 0;
        for (auto const & service : app.ServiceHealthStateChunks.Items)
        {
            TestSession::FailTestIf(service.ServiceName.empty(), "Service name should be set in service health state chunk {0}", service);
            serviceResults.Add(service.HealthState);
            serviceCount++;

            size_t partitionCount = 0;
            for (auto const & partition : service.PartitionHealthStateChunks.Items)
            {
                TestSession::FailTestIf(partition.PartitionId == Guid::Empty(), "Partition id should be set in partition health state chunk {0}", partition);
                partitionResults.Add(partition.HealthState);
                ++partitionCount;

                size_t replicaCount = 0;
                for (auto const & replica : partition.ReplicaHealthStateChunks.Items)
                {
                    TestSession::FailTestIf(replica.ReplicaOrInstanceId == FABRIC_INVALID_REPLICA_ID, "Replica id should be set in replica health state chunk {0}", replica);
                    replicaResults.Add(replica.HealthState);
                    ++replicaCount;
                }

                TestSession::FailTestIfNot(
                    partition.ReplicaHealthStateChunks.Count == replicaCount && partition.ReplicaHealthStateChunks.TotalCount >= replicaCount,
                    "Partition {0}: Count={1}, TotalCount={2}, included items {3}",
                    partition.PartitionId,
                    partition.ReplicaHealthStateChunks.Count,
                    partition.ReplicaHealthStateChunks.TotalCount,
                    replicaCount);
            }

            TestSession::FailTestIfNot(
                service.PartitionHealthStateChunks.Count == partitionCount && service.PartitionHealthStateChunks.TotalCount >= partitionCount,
                "Service {0}: Count={1}, TotalCount={2}, included items {3}",
                service.ServiceName,
                service.PartitionHealthStateChunks.Count,
                service.PartitionHealthStateChunks.TotalCount,
                partitionCount);
        }

        TestSession::FailTestIfNot(
            app.ServiceHealthStateChunks.Count == serviceCount && app.ServiceHealthStateChunks.TotalCount >= serviceCount,
            "App services {0}: Count={1}, TotalCount={2}, included items {3}",
            app.ApplicationName,
            app.ServiceHealthStateChunks.Count,
            app.ServiceHealthStateChunks.TotalCount,
            serviceCount);

        size_t daCount = 0;
        for (auto const & deployedApp : app.DeployedApplicationHealthStateChunks.Items)
        {
            TestSession::FailTestIf(deployedApp.NodeName.empty(), "Node name should be set in deployed app health state chunk {0}", deployedApp);
            deployedAppResults.Add(deployedApp.HealthState);
            daCount++;

            size_t dspCount = 0;
            for (auto const & dsp : deployedApp.DeployedServicePackageHealthStateChunks.Items)
            {
                TestSession::FailTestIf(dsp.ServiceManifestName.empty(), "Service manifest name should be set in deployed service package health state chunk {0}", dsp);
                deployedServicePackageResults.Add(dsp.HealthState);
                ++dspCount;
            }

            if (deployedApp.DeployedServicePackageHealthStateChunks.Count != dspCount || deployedApp.DeployedServicePackageHealthStateChunks.TotalCount < dspCount)
            {
                TestSession::FailTest(
                    "{0}+{1}: Count={2}, TotalCount={3}, included items {4}",
                    app.ApplicationName,
                    deployedApp.NodeName,
                    deployedApp.DeployedServicePackageHealthStateChunks.Count,
                    deployedApp.DeployedServicePackageHealthStateChunks.TotalCount,
                    dspCount);
            }
        }

        TestSession::FailTestIfNot(
            app.DeployedApplicationHealthStateChunks.Count == daCount && app.DeployedApplicationHealthStateChunks.TotalCount >= daCount,
            "App deployed apps {0}: Count={1}, TotalCount={2}, included items {3}",
            app.ApplicationName,
            app.DeployedApplicationHealthStateChunks.Count,
            app.DeployedApplicationHealthStateChunks.TotalCount,
            daCount);
    }

    if (verifiedAppChunks != expectedAppChunks.size())
    {
        TestSession::WriteInfo(TraceSource, "Not all app chunks were verified against expectedApps: verified {0}, expected {1}", verifiedAppChunks, expectedAppsString);
        return false;
    }

    ULONG appTotal = appResults.GetTotal();
    TestSession::FailTestIfNot(clusterHealthChunk.ApplicationHealthStateChunks.TotalCount >= appTotal, "Application TotalCount {0} should be >= included item count {1}", clusterHealthChunk.ApplicationHealthStateChunks.TotalCount, appTotal);
    TestSession::FailTestIfNot(clusterHealthChunk.ApplicationHealthStateChunks.Count == appTotal, "Application Count {0} should be == included item count {1}", clusterHealthChunk.ApplicationHealthStateChunks.TotalCount, appTotal);

    resultsPerChildrenType.insert(make_pair(EntityKind::Node, move(nodeResults)));
    resultsPerChildrenType.insert(make_pair(EntityKind::Application, move(appResults)));
    resultsPerChildrenType.insert(make_pair(EntityKind::Service, move(serviceResults)));
    resultsPerChildrenType.insert(make_pair(EntityKind::Partition, move(partitionResults)));
    resultsPerChildrenType.insert(make_pair(EntityKind::Replica, move(replicaResults)));
    resultsPerChildrenType.insert(make_pair(EntityKind::DeployedApplication, move(deployedAppResults)));
    resultsPerChildrenType.insert(make_pair(EntityKind::DeployedServicePackage, move(deployedServicePackageResults)));

    if (!VerifyAggregatedHealthStates(parser, resultsPerChildrenType))
    {
        success = false;
    }

    if (!success || !requiresValidation)
    {
        wstring trace(L"Cluster: ");
        StringWriter writer(trace);
        writer.WriteLine(GetHealthStateString(clusterHealthChunk.HealthState));

        if (!clusterHealthChunk.ApplicationHealthStateChunks.Items.empty())
        {
            writer.WriteLine("Application Children:");
            for (auto const & app : clusterHealthChunk.ApplicationHealthStateChunks.Items)
            {
                writer.WriteLine("\t{0}", app);
                for (auto const & service : app.ServiceHealthStateChunks.Items)
                {
                    writer.WriteLine("\t\t{0}", service);
                    for (auto const & partition : service.PartitionHealthStateChunks.Items)
                    {
                        writer.WriteLine("\t\t\t{0}", partition);
                        for (auto const & replica : partition.ReplicaHealthStateChunks.Items)
                        {
                            writer.WriteLine("\t\t\t\t{0}", replica);
                        }
                    }
                }

                for (auto const & deployedApp : app.DeployedApplicationHealthStateChunks.Items)
                {
                    writer.WriteLine("\t\t{0}", deployedApp);
                    for (auto const & dsp : deployedApp.DeployedServicePackageHealthStateChunks.Items)
                    {
                        writer.WriteLine("\t\t\t\t{0}", dsp);
                    }
                }
            }
        }

        if (!clusterHealthChunk.NodeHealthStateChunks.Items.empty())
        {
            writer.WriteLine("Node Children:");
            for (auto const & node : clusterHealthChunk.NodeHealthStateChunks.Items)
            {
                writer.WriteLine("\t{0}", node);
            }
        }

        TestSession::WriteInfo(TraceSource, "{0}\n", trace);
    }

    return success;
}

bool TestFabricClientHealth::ValidateClusterHealth(CommandLineParser & parser, FABRIC_CLUSTER_HEALTH const & publicClusterHealth, bool expectEmpty, bool requiresValidation)
{
    ClusterHealth clusterHealth;
    auto error = clusterHealth.FromPublicApi(publicClusterHealth);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetClusterHealth: ClusterHealth.FromPublicApi failed with unexpected error = {0}", error);

    bool success = true;
    wstring expectedHealthStateString;
    if (requiresValidation)
    {
        TestSession::WriteInfo(TraceSource, "GetClusterHealth returned {0}", clusterHealth);

        if (!VerifyEntityEventsAndHealthState(parser, false, clusterHealth.Events, clusterHealth.AggregatedHealthState, clusterHealth.UnhealthyEvaluations, expectEmpty))
        {
            TestSession::WriteWarning(TraceSource,  "GetClusterHealth events and health state verification failed");
            success = false;
        }

        HealthStateCountMap resultsPerChildrenType;
        HealthStateCount appResults;
        for (auto it = clusterHealth.ApplicationsAggregatedHealthStates.begin(); it != clusterHealth.ApplicationsAggregatedHealthStates.end(); ++it)
        {
            appResults.Add(it->AggregatedHealthState);
        }

        resultsPerChildrenType.insert(make_pair(EntityKind::Application, move(appResults)));

        HealthStateCount nodeResults;
        for (auto it = clusterHealth.NodesAggregatedHealthStates.begin(); it != clusterHealth.NodesAggregatedHealthStates.end(); ++it)
        {
            nodeResults.Add(it->AggregatedHealthState);
        }

        resultsPerChildrenType.insert(make_pair(EntityKind::Node, move(nodeResults)));

        if (!VerifyAggregatedHealthStates(parser, resultsPerChildrenType))
        {
            success = false;
        }

        if (!CheckHealthStats(parser, clusterHealth.HealthStats))
        {
            success = false;
        }
    }

    if (!success || !requiresValidation)
    {
        wstring trace = GetEntityHealthBaseDetails(clusterHealth);
        StringWriter writer(trace);

        writer.WriteLine("Application Children:");
        for (auto it = clusterHealth.ApplicationsAggregatedHealthStates.begin(); it != clusterHealth.ApplicationsAggregatedHealthStates.end(); ++it)
        {
            writer.WriteLine("\t{0}", *it);
        }

        writer.WriteLine("Node Children:");
        for (auto it = clusterHealth.NodesAggregatedHealthStates.begin(); it != clusterHealth.NodesAggregatedHealthStates.end(); ++it)
        {
            writer.WriteLine("\t{0}", *it);
        }

        if (clusterHealth.HealthStats)
        {
            writer.WriteLine("\nStats:{0}", *clusterHealth.HealthStats);
        }

        TestSession::WriteInfo(TraceSource, "{0}\n", trace);
    }

    return success;
}

bool TestFabricClientHealth::ValidateNodeHealth(CommandLineParser & parser, FABRIC_NODE_HEALTH const & publicNodeHealth, std::wstring const & nodeName, bool expectEmpty, bool requiresValidation)
{
    NodeHealth nodeHealth;
    auto error = nodeHealth.FromPublicApi(publicNodeHealth);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetNodeHealth: NodeHealth.FromPublicApi failed with unexpected error = {0}", error);

    if (nodeHealth.NodeName.empty())
    {
        TestSession::WriteInfo(TraceSource, "GetNodeHealth returned empty node name: {0}", nodeHealth);
        // FM health report is not yet received, so node name is not populated
        return false;
    }

    TestSession::FailTestIf(nodeHealth.NodeName != nodeName, "GetNodeHealth entity information verification failed: expected nodeName {0}, received {1}", nodeName, nodeHealth.NodeName);

    bool success = true;
    if (requiresValidation)
    {
        TestSession::WriteInfo(TraceSource, "GetNodeHealth {0} returned {1}", nodeHealth.NodeName, nodeHealth);
        if (!VerifyEntityEventsAndHealthState(parser, true, nodeHealth.Events, nodeHealth.AggregatedHealthState, nodeHealth.UnhealthyEvaluations, expectEmpty))
        {
            TestSession::WriteWarning(TraceSource,  "GetNodeHealth events and health state verification failed");
            success = false;
        }
    }

    if (!success || !requiresValidation)
    {
        wstring trace = GetEntityHealthBaseDetails(nodeHealth);
        TestSession::WriteInfo(TraceSource, "{0}\n", trace);
    }

    return success;
}

bool TestFabricClientHealth::ValidateReplicaHealth(CommandLineParser & parser, FABRIC_REPLICA_HEALTH const & publicReplicaHealth, Common::Guid const & partitionGuid, FABRIC_REPLICA_ID replicaId, bool expectEmpty, bool requiresValidation)
{
    ReplicaHealth replicaHealth;
    auto error = replicaHealth.FromPublicApi(publicReplicaHealth);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetReplicaHealth: ReplicaHealth.FromPublicApi failed with unexpected error = {0}", error);
    TestSession::FailTestIf(replicaHealth.ReplicaId != replicaId, "GetReplicaHealth entity information verification failed: expected replicaId {0}, received {1}", replicaId, replicaHealth.ReplicaId);
    TestSession::FailTestIf(replicaHealth.PartitionId != partitionGuid, "GetPartitionHealth entity information verification failed: expected partitionId {0}, received {1}", partitionGuid, replicaHealth.PartitionId);

    bool success = true;
    if (requiresValidation)
    {
        TestSession::WriteNoise(TraceSource, "GetReplicaHealth {0}+{1} returned {2}", replicaHealth.PartitionId, replicaHealth.ReplicaId, replicaHealth);

        if (!VerifyEntityEventsAndHealthState(parser, true, replicaHealth.Events, replicaHealth.AggregatedHealthState, replicaHealth.UnhealthyEvaluations, expectEmpty))
        {
            TestSession::WriteWarning(TraceSource,  "GetReplicaHealth events and health state verification failed");
            success = false;
        }
    }

    if (!success || !requiresValidation)
    {
        wstring trace = GetEntityHealthBaseDetails(replicaHealth);
        TestSession::WriteInfo(TraceSource, "{0}\n", trace);
    }

    return success;
}

bool TestFabricClientHealth::ValidatePartitionHealth(CommandLineParser & parser, FABRIC_PARTITION_HEALTH const & publicPartitionHealth, Common::Guid const & partitionGuid, bool expectEmpty, bool requiresValidation)
{
    PartitionHealth partitionHealth;
    auto error = partitionHealth.FromPublicApi(publicPartitionHealth);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetPartitionHealth: PartitionHealth.FromPublicApi failed with unexpected error = {0}", error);
    TestSession::FailTestIf(partitionHealth.PartitionId != partitionGuid, "GetPartitionHealth entity information verification failed: expected partitionId {0}, received {1}", partitionGuid, partitionHealth.PartitionId);

    bool success = true;
    if (requiresValidation)
    {
        TestSession::WriteNoise(TraceSource, "GetPartitionHealth {0} returned {1}", partitionHealth.PartitionId, partitionHealth);

        if (!VerifyEntityEventsAndHealthState(parser, true, partitionHealth.Events, partitionHealth.AggregatedHealthState, partitionHealth.UnhealthyEvaluations, expectEmpty))
        {
            TestSession::WriteWarning(TraceSource,  "GetPartitionHealth events and health state verification failed");
            success = false;
        }

        HealthStateCount results;
        for (auto it = partitionHealth.ReplicasAggregatedHealthStates.begin(); it != partitionHealth.ReplicasAggregatedHealthStates.end(); ++it)
        {
            results.Add(it->AggregatedHealthState);
        }

        HealthStateCountMap resultsMap;
        resultsMap.insert(make_pair(EntityKind::Replica, move(results)));
        if (!VerifyAggregatedHealthStates(parser, resultsMap))
        {
            success = false;
        }

        if (!CheckHealthStats(parser, partitionHealth.HealthStats))
        {
            success = false;
        }
    }

    if (!success || !requiresValidation)
    {
        wstring trace = GetEntityHealthBaseDetails(partitionHealth);

        StringWriter writer(trace);
        writer.WriteLine("Children:");
        for (auto it = partitionHealth.ReplicasAggregatedHealthStates.begin(); it != partitionHealth.ReplicasAggregatedHealthStates.end(); ++it)
        {
            writer.WriteLine("\t{0}", *it);
        }

        if (partitionHealth.HealthStats)
        {
            writer.WriteLine("\nStats:{0}", *partitionHealth.HealthStats);
        }

        TestSession::WriteInfo(TraceSource, "{0}\n", trace);
    }

    return success;
}

bool TestFabricClientHealth::ValidateDeployedServicePackageHealth(
    CommandLineParser & parser,
    FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH const & publicDeployedServicePackageHealth,
    std::wstring const & appName,
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    std::wstring const & nodeName,
    bool expectEmpty,
    bool requiresValidation)
{
    DeployedServicePackageHealth deployedServicePackageHealth;
    auto error = deployedServicePackageHealth.FromPublicApi(publicDeployedServicePackageHealth);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetDeployedServicePackageHealth: DeployedServicePackageHealth.FromPublicApi failed with unexpected error = {0}", error);
    TestSession::FailTestIf(deployedServicePackageHealth.ApplicationName != appName, "GetDeployedServicePackageHealth entity information verification failed: expected appName {0}, received {1}", appName, deployedServicePackageHealth.ApplicationName);
    TestSession::FailTestIf(deployedServicePackageHealth.ServiceManifestName != serviceManifestName, "GetDeployedServicePackageHealth entity information verification failed: expected deployedServiceManifestName {0}, received {1}", serviceManifestName, deployedServicePackageHealth.ServiceManifestName);

    TestSession::FailTestIf(
        deployedServicePackageHealth.ServicePackageActivationId != servicePackageActivationId,
        "GetDeployedServicePackageHealth entity information verification failed: expected ServicePackageActivationId={0}, received={1}.",
        servicePackageActivationId,
        deployedServicePackageHealth.ServicePackageActivationId);

    if (deployedServicePackageHealth.NodeName.empty())
    {
        TestSession::WriteInfo(TraceSource, "GetDeployedServicePackageHealth returned empty node name: {0}", deployedServicePackageHealth);
        // FM health report is not yet received, so node name is not populated
        return false;
    }

    TestSession::FailTestIf(deployedServicePackageHealth.NodeName != nodeName, "GetDeployedServicePackageHealth entity information verification failed: expected nodeName {0}, received {1}", nodeName, deployedServicePackageHealth.NodeName);

    bool success = true;
    if (requiresValidation)
    {
        TestSession::WriteInfo(TraceSource, "GetDeployedServicePackageHealth on node {0} returned {1}", deployedServicePackageHealth.NodeName, deployedServicePackageHealth);
        if (!VerifyEntityEventsAndHealthState(parser, true, deployedServicePackageHealth.Events, deployedServicePackageHealth.AggregatedHealthState, deployedServicePackageHealth.UnhealthyEvaluations, expectEmpty))
        {
            TestSession::WriteWarning(TraceSource,  "GetDeployedServicePackageHealth events and health state verification failed");
            success = false;
        }
    }

    if (!success || !requiresValidation)
    {
        wstring trace = GetEntityHealthBaseDetails(deployedServicePackageHealth);
        TestSession::WriteInfo(TraceSource, "{0}\n", trace);
    }

    return success;
}

bool TestFabricClientHealth::ValidateDeployedApplicationHealth(CommandLineParser & parser, FABRIC_DEPLOYED_APPLICATION_HEALTH const & publicDeployedApplicationHealth, std::wstring const & appName, std::wstring const & nodeName, bool expectEmpty, bool requiresValidation)
{
    DeployedApplicationHealth deployedApplicationHealth;
    auto error = deployedApplicationHealth.FromPublicApi(publicDeployedApplicationHealth);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetDeployedApplicationHealth: DeployedApplicationHealth.FromPublicApi failed with unexpected error = {0}", error);
    TestSession::FailTestIf(deployedApplicationHealth.ApplicationName != appName, "GetDeployedApplicationHealth entity information verification failed: expected appName {0}, received {1}", appName, deployedApplicationHealth.ApplicationName);
    if (deployedApplicationHealth.NodeName.empty())
    {
        TestSession::WriteInfo(TraceSource, "GetDeployedApplicationHealth returned empty node name: {0}", deployedApplicationHealth);
        return false;
    }

    TestSession::FailTestIf(deployedApplicationHealth.NodeName != nodeName, "GetDeployedApplicationHealth entity information verification failed: expected nodeName {0}, received {1}", nodeName, deployedApplicationHealth.NodeName);

    bool success = true;
    if (requiresValidation)
    {
        TestSession::WriteInfo(TraceSource, "GetDeployedApplicationHealth on node {0} returned {1}", deployedApplicationHealth.NodeName, deployedApplicationHealth);
        if (!VerifyEntityEventsAndHealthState(parser, false, deployedApplicationHealth.Events, deployedApplicationHealth.AggregatedHealthState, deployedApplicationHealth.UnhealthyEvaluations, expectEmpty))
        {
            TestSession::WriteWarning(TraceSource,  "GetDeployedApplicationHealth events and health state verification failed");
            success = false;
        }

        HealthStateCount results;
        for (auto it = deployedApplicationHealth.DeployedServicePackagesAggregatedHealthStates.begin(); it != deployedApplicationHealth.DeployedServicePackagesAggregatedHealthStates.end(); ++it)
        {
            results.Add(it->AggregatedHealthState);
        }

        HealthStateCountMap resultsMap;
        resultsMap.insert(make_pair(EntityKind::DeployedServicePackage, move(results)));
        if (!VerifyAggregatedHealthStates(parser, resultsMap))
        {
            success = false;
        }

        if (!CheckHealthStats(parser, deployedApplicationHealth.HealthStats))
        {
            success = false;
        }
    }

    if (!success || !requiresValidation)
    {
        wstring trace = GetEntityHealthBaseDetails(deployedApplicationHealth);

        StringWriter writer(trace);

        writer.WriteLine("Children:");
        for (auto it = deployedApplicationHealth.DeployedServicePackagesAggregatedHealthStates.begin(); it != deployedApplicationHealth.DeployedServicePackagesAggregatedHealthStates.end(); ++it)
        {
            writer.WriteLine("\t{0}", *it);
        }

        if (deployedApplicationHealth.HealthStats)
        {
            writer.WriteLine("\nStats:{0}", deployedApplicationHealth.HealthStats);
        }

        TestSession::WriteInfo(TraceSource, "{0}\n", trace);
    }

    return success;
}

bool TestFabricClientHealth::ValidateServiceHealth(CommandLineParser & parser, FABRIC_SERVICE_HEALTH const & publicServiceHealth, std::wstring const & serviceName, bool expectEmpty, bool requiresValidation)
{
    ServiceHealth serviceHealth;
    auto error = serviceHealth.FromPublicApi(publicServiceHealth);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetServiceHealth: ServiceHealth.FromPublicApi failed with unexpected error = {0}", error);
    TestSession::FailTestIf(serviceHealth.ServiceName != serviceName, "GetServiceHealth entity information verification failed: expected serviceName {0}, received {1}", serviceName, serviceHealth.ServiceName);

    bool success = true;
    if (requiresValidation)
    {
        TestSession::WriteInfo(TraceSource, "GetServiceHealth {0} returned {1}", serviceHealth.ServiceName, serviceHealth);
        if (!VerifyEntityEventsAndHealthState(parser, false, serviceHealth.Events, serviceHealth.AggregatedHealthState, serviceHealth.UnhealthyEvaluations, expectEmpty))
        {
            TestSession::WriteWarning(TraceSource,  "GetServiceHealth events and health state verification failed");
            success = false;
        }

        HealthStateCount results;
        for (auto it = serviceHealth.PartitionsAggregatedHealthStates.begin(); it != serviceHealth.PartitionsAggregatedHealthStates.end(); ++it)
        {
            results.Add(it->AggregatedHealthState);
        }

        HealthStateCountMap resultsMap;
        resultsMap.insert(make_pair(EntityKind::Partition, move(results)));
        if (!VerifyAggregatedHealthStates(parser, resultsMap))
        {
            success = false;
        }

        if (!CheckHealthStats(parser, serviceHealth.HealthStats))
        {
            success = false;
        }
    }

    if (!success || !requiresValidation)
    {
        wstring trace = GetEntityHealthBaseDetails(serviceHealth);

        StringWriter writer(trace);

        writer.WriteLine("Children:");
        for (auto it = serviceHealth.PartitionsAggregatedHealthStates.begin(); it != serviceHealth.PartitionsAggregatedHealthStates.end(); ++it)
        {
            writer.WriteLine("\t{0}", *it);
        }

        if (serviceHealth.HealthStats)
        {
            writer.WriteLine("\nStats:{0}", serviceHealth.HealthStats);
        }

        TestSession::WriteInfo(TraceSource, "{0}\n", trace);
    }

    return success;
}

bool TestFabricClientHealth::ValidateApplicationHealth(CommandLineParser & parser, FABRIC_APPLICATION_HEALTH const & publicApplicationHealth, std::wstring const & applicationName, bool expectEmpty, bool requiresValidation)
{
    ApplicationHealth applicationHealth;
    auto error = applicationHealth.FromPublicApi(publicApplicationHealth);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetApplicationHealth: ApplicationHealth.FromPublicApi failed with unexpected error = {0}", error);
    TestSession::FailTestIf(applicationHealth.ApplicationName != applicationName, "GetApplicationHealth entity information verification failed: expected applicationName {0}, received {1}", applicationName, applicationHealth.ApplicationName);

    bool success = true;
    if (requiresValidation)
    {
        TestSession::WriteInfo(TraceSource, "GetApplicationHealth for {0} returned {1}", applicationHealth.ApplicationName, applicationHealth);
        if (!VerifyEntityEventsAndHealthState(parser, false, applicationHealth.Events, applicationHealth.AggregatedHealthState, applicationHealth.UnhealthyEvaluations, expectEmpty))
        {
            TestSession::WriteWarning(TraceSource,  "GetApplicationHealth events and health state verification failed");
            success = false;
        }

        HealthStateCountMap resultsMap;
        HealthStateCount serviceResults;
        for (auto it = applicationHealth.ServicesAggregatedHealthStates.begin(); it != applicationHealth.ServicesAggregatedHealthStates.end(); ++it)
        {
            serviceResults.Add(it->AggregatedHealthState);
        }

        resultsMap.insert(make_pair(EntityKind::Service, move(serviceResults)));

        HealthStateCount daResults;
        for (auto it = applicationHealth.DeployedApplicationsAggregatedHealthStates.begin(); it != applicationHealth.DeployedApplicationsAggregatedHealthStates.end(); ++it)
        {
            daResults.Add(it->AggregatedHealthState);
        }

        resultsMap.insert(make_pair(EntityKind::DeployedApplication, move(daResults)));

        if (!VerifyAggregatedHealthStates(parser, resultsMap))
        {
            success = false;
        }

        if (!CheckHealthStats(parser, applicationHealth.HealthStats))
        {
            success = false;
        }
    }

    if (!success || !requiresValidation)
    {
        wstring trace = GetEntityHealthBaseDetails(applicationHealth);

        StringWriter writer(trace);

        writer.WriteLine("Service Children:");
        for (auto it = applicationHealth.ServicesAggregatedHealthStates.begin(); it != applicationHealth.ServicesAggregatedHealthStates.end(); ++it)
        {
            writer.WriteLine("\t{0}", *it);
        }

        writer.WriteLine("DeployedApplication Children:");
        for (auto it = applicationHealth.DeployedApplicationsAggregatedHealthStates.begin(); it != applicationHealth.DeployedApplicationsAggregatedHealthStates.end(); ++it)
        {
            writer.WriteLine("\t{0}", *it);
        }

        if (applicationHealth.HealthStats)
        {
            writer.WriteLine("\nStats:{0}", applicationHealth.HealthStats);
        }

        TestSession::WriteInfo(TraceSource, "{0}\n", trace);
    }

    return success;
}

bool TestFabricClientHealth::ParseEvents(std::wstring const & eventsString, __out map<wstring, StringCollection> & events)
{
    vector<wstring> eventsSegments;
    StringUtility::Split<wstring>(eventsString, eventsSegments, L";");

    for (auto it = eventsSegments.begin(); it != eventsSegments.end(); ++it)
    {
        vector<wstring> expectedEventTokens;
        StringUtility::Split<wstring>(*it, expectedEventTokens, L",");
        if (expectedEventTokens.size() < 3 || expectedEventTokens.size() > 5)
        {
            TestSession::WriteInfo(TraceSource, "Invalid expectedevents format for {0}. Expected 3 to 5 tokens", *it);
            return false;
        }

        wstring const & source = expectedEventTokens[0];
        wstring const & property = expectedEventTokens[1];
        events.insert(make_pair(source + property, move(expectedEventTokens)));
    }

    return true;
}

// Parse strings like key:value;key:value
void ParseKeyValueTestArray(wstring const & input, __out map<wstring, wstring> & values)
{
    vector<wstring> segments;
    StringUtility::Split<wstring>(input, segments, L";");
    for (auto it = segments.begin(); it != segments.end(); ++it)
    {
        size_t index = it->find_first_of(L":");
        if (index != it->npos)
        {
            values[it->substr(0, index)] = it->substr(index + 1);
        }
    }
}

bool TestFabricClientHealth::ParseServicePackageActivationId(CommandLineParser & parser, __inout std::wstring & servicePackageActivationId)
{
    if (parser.TryGetString(L"servicePackageActivationId", servicePackageActivationId))
    {
        ServicePackageActivationContext activationContext;
        auto error = ServicePackageActivationContext::FromString(servicePackageActivationId, activationContext);
        if (!error.IsSuccess())
        {
            TestSession::WriteError(
                TraceSource,
                "Invalid servicePackageActivationId={0} specified. Parsing Error={2}.",
                servicePackageActivationId,
                error);

            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyEntityEventsAndHealthState(
    CommandLineParser & parser,
    bool isEntityPersisted,
    vector<ServiceModel::HealthEvent> const & events,
    FABRIC_HEALTH_STATE resultAggregatedHealthState,
    std::vector<HealthEvaluation> const & unhealthyEvaluations,
    bool isEmptyExpected)
{
    size_t receivedEventsCount = events.size();
    if (isEmptyExpected)
    {
        if (receivedEventsCount != 0)
        {
            TestSession::WriteWarning(TraceSource, "Unexpected health query result count {0}.", events.size());
            return false;
        }

        return true;
    }

    int expectedEventCount = -1;
    if (parser.TryGetInt(L"expectedeventcount", expectedEventCount))
    {
        if (receivedEventsCount != expectedEventCount)
        {
            TestSession::WriteWarning(TraceSource, "Unexpected health query event count: expected {0}, actual {1}.", expectedEventCount, receivedEventsCount);
            return false;
        }
    }

    wstring expectedHealthStateString;
    bool checkHealthState = parser.TryGetString(L"expectedhealthstate", expectedHealthStateString);

    wstring expectedHealthDescString;
    map<wstring, wstring> expectedDescriptions;
    size_t expectedDescriptionsChecked = 0;
    if (parser.TryGetString(L"expecteddesc", expectedHealthDescString))
    {
        ParseKeyValueTestArray(expectedHealthDescString, expectedDescriptions);
        expectedDescriptionsChecked = expectedDescriptions.size();
    }

    wstring expectedHealthDescStringLength;
    map<wstring, wstring> expectedDescriptionsLength;
    size_t expectedDescriptionsLengthChecked = 0;
    if (parser.TryGetString(L"expecteddesclength", expectedHealthDescStringLength))
    {
        ParseKeyValueTestArray(expectedHealthDescStringLength, expectedDescriptionsLength);
        expectedDescriptionsLengthChecked = expectedDescriptionsLength.size();
    }

    map<wstring, vector<wstring>> expectedEvents;
    wstring expectedEventsString;
    if (parser.TryGetString(L"expectedevents", expectedEventsString))
    {
        if (!ParseEvents(expectedEventsString, expectedEvents))
        {
            return false;
        }
    }

    if (expectedHealthDescString.empty() && expectedHealthStateString.empty() && expectedHealthDescStringLength.empty())
    {
        return true;
    }

    if (receivedEventsCount == 0 && isEntityPersisted && expectedEventCount != 0)
    {
        TestSession::WriteInfo(TraceSource, "Invalid health query result count {0}.", receivedEventsCount);
        return false;
    }

    FABRIC_HEALTH_STATE expectedHealthState = FABRIC_HEALTH_STATE_INVALID;
    if (checkHealthState)
    {
        expectedHealthState = GetHealthState(expectedHealthStateString);
        if (expectedHealthState != resultAggregatedHealthState)
        {
            TestSession::WriteInfo(TraceSource, "Aggregated health state does not match expected:{0} actual:{1}",
                expectedHealthStateString, GetHealthStateString(resultAggregatedHealthState));

            return false;
        }
    }

    if (resultAggregatedHealthState == FABRIC_HEALTH_STATE_OK)
    {
        // No evaluation should be generated in this case
        TestSession::FailTestIfNot(unhealthyEvaluations.empty(), "Aggregated health state {0} and {1} evaluation reasons. There should be 0 reasons.", resultAggregatedHealthState, unhealthyEvaluations.size());
    }
    else if (unhealthyEvaluations.empty())
    {
        TestSession::FailTest("Aggregated health state {0}: invalid evaluation reasons count {1}.", resultAggregatedHealthState, unhealthyEvaluations.size());
    }

    for (size_t i = 0; i < receivedEventsCount; ++i)
    {
        if (checkHealthState && (events[i].State > expectedHealthState) && !events[i].IsExpired)
        {
            TestSession::WriteInfo(TraceSource, "Event health state does not match expected aggregated health:{0} actual:{1}, event not expired ({2})",
                expectedHealthStateString, GetHealthStateString(events[i].State), events[i]);

            return false;
        }

        wstring const & eventSource = events[i].SourceId;
        wstring const & eventProperty = events[i].Property;
        wstring const & eventDesc = events[i].Description;

        auto it = expectedDescriptions.find(eventProperty);
        if (it != expectedDescriptions.end())
        {
            --expectedDescriptionsChecked;
            if (!StringUtility::Contains<wstring>(eventDesc, it->second))
            {
                TestSession::WriteInfo(TraceSource, "Health description does not match expected:\"{0}\" actual:\"{1}\"",
                    it->second, eventDesc);
                return false;
            }
        }

        auto it2 = expectedDescriptionsLength.find(eventProperty);
        if (it2 != expectedDescriptionsLength.end())
        {
            --expectedDescriptionsLengthChecked;
            if (wformatString("{0}", eventDesc.size()) != it2->second)
            {
                TestSession::WriteInfo(TraceSource, "Health description length does not match expected: actual \"{0}\", length {1} expected:{2}",
                    eventDesc, eventDesc.size(), it2->second);
                return false;
            }
        }

        vector<pair<FABRIC_HEALTH_STATE, DateTime>> healthStateTransitions;
        healthStateTransitions.push_back(make_pair(FABRIC_HEALTH_STATE_OK, events[i].LastOkTransitionAt));
        healthStateTransitions.push_back(make_pair(FABRIC_HEALTH_STATE_WARNING, events[i].LastWarningTransitionAt));
        healthStateTransitions.push_back(make_pair(FABRIC_HEALTH_STATE_ERROR, events[i].LastErrorTransitionAt));

        // Sort DESC
        sort(healthStateTransitions.begin(), healthStateTransitions.end(), [](pair<FABRIC_HEALTH_STATE, DateTime> const & l, pair<FABRIC_HEALTH_STATE, DateTime> const & r) { return l.second > r.second; });

        auto & currentStateTransitionTime = healthStateTransitions[0].second;
        TestSession::FailTestIf(currentStateTransitionTime == DateTime::Zero, "Validate event {0} failed: Current state transition time should be set", events[i]);

        auto previousState = FABRIC_HEALTH_STATE_INVALID;
        if (healthStateTransitions[1].second != DateTime::Zero)
        {
            previousState = (healthStateTransitions[1].first == events[i].State) ? healthStateTransitions[0].first : healthStateTransitions[1].first;
            TestSession::FailTestIfNot(
                currentStateTransitionTime == healthStateTransitions[1].second,
                "Validate event {0} failed: previous state {1} time should be = current state transition time {2}",
                events[i],
                previousState,
                currentStateTransitionTime);
        }

        auto oldestState = FABRIC_HEALTH_STATE_INVALID;
        if (healthStateTransitions[2].second != DateTime::Zero)
        {
            oldestState = healthStateTransitions[2].first;
            TestSession::FailTestIfNot(
                currentStateTransitionTime > healthStateTransitions[2].second,
                "Validate event {0} failed: Current state transition time {1} should be > Last{2}",
                events[i],
                currentStateTransitionTime,
                oldestState);
        }

        auto itExpectedEvents = expectedEvents.find(eventSource + eventProperty);
        if (itExpectedEvents != expectedEvents.end())
        {
            // Check the event against desired event
            FABRIC_HEALTH_STATE desiredHealthState = GetHealthState(itExpectedEvents->second[2]);
            if (events[i].State != desiredHealthState)
            {
                TestSession::WriteInfo(TraceSource, "Validate event {0} current health state failed. Expected {1}", events[i], expectedEventsString);
                return false;
            }

            if (itExpectedEvents->second.size() > 3)
            {
                // Check against transition times
                FABRIC_HEALTH_STATE previousDesiredHealthState = GetHealthState(itExpectedEvents->second[3]);
                if (previousDesiredHealthState != previousState)
                {
                    TestSession::WriteInfo(TraceSource, "Validate event {0}: previous health state failed. Expected {1} ({2}), actual previous state = {3}", events[i], expectedEventsString, previousDesiredHealthState, previousState);
                    return false;
                }

                if (itExpectedEvents->second.size() > 4)
                {
                    FABRIC_HEALTH_STATE oldestDesiredHealthState = GetHealthState(itExpectedEvents->second[4]);
                    if (oldestDesiredHealthState != oldestState)
                    {
                        TestSession::WriteInfo(TraceSource, "Validate event {0}: oldest health state failed. Expected {1} ({2}), actual oldest state = {3}", events[i], expectedEventsString, oldestDesiredHealthState, oldestState);
                        return false;
                    }
                }
            }

            // Event validation succeeded
            expectedEvents.erase(itExpectedEvents);
        }
    }

    if (expectedDescriptionsChecked > 0)
    {
        TestSession::WriteInfo(TraceSource, "Event description match failed: Some expected properties were not found. Expected: {0}", expectedHealthDescString);
        return false;
    }

    if (expectedDescriptionsLengthChecked > 0)
    {
        TestSession::WriteInfo(TraceSource, "Event description length match failed: Some expected properties were not found. Expected: {0}", expectedHealthDescStringLength);
        return false;
    }

    if (!expectedEvents.empty())
    {
        TestSession::WriteInfo(TraceSource, "{0} events required for validation were not returned. Expected {1}", expectedEvents.size(), expectedEventsString);
        return false;
    }

    wstring expectedEvaluation;
    if (parser.TryGetString(L"expectedreason", expectedEvaluation))
    {
        return VerifyExpectedHealthEvaluations(expectedEvaluation, unhealthyEvaluations);
    }
    else
    {
        PrintUnhealthyEvaluations(unhealthyEvaluations);
    }

    return true;
}

bool TestFabricClientHealth::VerifyExpectedHealthEvaluations(FABRIC_HEALTH_EVALUATION_LIST const * publicHealthEvaluations, std::wstring const & expectedEvaluation)
{
    TestSession::FailTestIf(publicHealthEvaluations == NULL, "ExpectedHealthEvaluation evaluation {0}, but no reasons returned", expectedEvaluation);

    vector<HealthEvaluation> unhealthyEvaluations;
    auto error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicHealthEvaluations,
        unhealthyEvaluations);
    TestSession::FailTestIfNot(
        error.IsSuccess(),
        "UnhealthyEvaluations FromPublicApiList error={0}",
        error);

    return VerifyExpectedHealthEvaluations(expectedEvaluation, unhealthyEvaluations);
}

bool TestFabricClientHealth::VerifyExpectedHealthEvaluations(std::wstring const & expectedEvaluation, vector<HealthEvaluation> const & unhealthyEvaluations)
{
    TestSession::FailTestIf(unhealthyEvaluations.size() != 1, "Evaluation reasons count is not valid: {0}, expected evaluation: {1}", unhealthyEvaluations.size(), expectedEvaluation);

    HealthEvaluationBaseSPtr evaluation = unhealthyEvaluations[0].Evaluation;
    TestSession::FailTestIf(!evaluation, "EvaluationWrapper contains null evaluation evaluation");
    TestSession::FailTestIf(evaluation->Description.empty(), "Description is empty for {0}. Expected {1}", evaluation, expectedEvaluation);

    // Print the concatenated recursive health evaluation description.
    // This is used for Mesh as workaround until we add the proper hierarchy for v2 applications.
    // The description concatenates all hierarchy evaluations, so it must include the description for event,
    // unless it is truncated.
    wstring unhealthyEvalsDescription = HealthEvaluation::GetUnhealthyEvaluationDescription(unhealthyEvaluations);
    TestSession::WriteNoise(TraceSource, "v2Application HealthEvaluationDescription: {0}", unhealthyEvalsDescription);
    
    vector<wstring> tokens;
    StringUtility::Split<wstring>(expectedEvaluation, tokens, L",");
    if (tokens.size() == 0)
    {
        TestSession::WriteError(TraceSource, "Could not parse evaluationreason parameter - too few arguments {0}", expectedEvaluation);
        return false;
    }

    if (tokens[0] == L"event")
    {
        if (!VerifyEventEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"replicas")
    {
        if (!VerifyReplicasEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"nodes")
    {
        if (!VerifyNodesEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"partitions")
    {
        if (!VerifyPartitionsEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"deployedapplications")
    {
        if (!VerifyDeployedApplicationsEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"deployedservicepackages")
    {
        if (!VerifyDeployedServicePackagesEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"applications")
    {
        if (!VerifyApplicationsEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"apptypeapplications")
    {
        if (!VerifyApplicationTypeApplicationsEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"services")
    {
        if (!VerifyServicesEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"systemapp")
    {
        if (!VerifySystemAppEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"deployedapplicationsperud")
    {
        if (!VerifyDeployedApplicationsPerUdEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"nodesperud")
    {
        if (!VerifyNodesPerUdEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"delta")
    {
        if (!VerifyDeltaEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else if (tokens[0] == L"uddelta")
    {
        if (!VerifyDeltaPerUdEvaluation(tokens, evaluation))
        {
            return false;
        }
    }
    else
    {
        TestSession::FailTest("Can't parse evaluationReason {0}: can't identify the type", expectedEvaluation);
    }

    return true;
}

void TestFabricClientHealth::PrintUnhealthyEvaluations(std::vector<HealthEvaluation> const & unhealthyEvaluations)
{
    if (unhealthyEvaluations.empty())
    {
        return;
    }

    for (auto it = unhealthyEvaluations.begin(); it != unhealthyEvaluations.end(); ++it)
    {
        auto & evaluation = it->Evaluation;
        // Check expected reasons
        switch(evaluation->Kind)
        {
        case FABRIC_HEALTH_EVALUATION_KIND_NODES:
            TestSession::FailTestIfNot(VerifyNodesEvaluation(evaluation), "Verify nodes failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_REPLICAS:
            TestSession::FailTestIfNot(VerifyReplicasEvaluation(evaluation), "Verify replicas failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS:
            TestSession::FailTestIfNot(VerifyPartitionsEvaluation(evaluation), "Verify partitions failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_SERVICES:
            TestSession::FailTestIfNot(VerifyServicesEvaluation(evaluation), "Verify services failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS:
            TestSession::FailTestIfNot(VerifyApplicationsEvaluation(evaluation), "Verify apps failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS:
            TestSession::FailTestIfNot(VerifyApplicationTypeApplicationsEvaluation(evaluation), "Verify app type apps failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION:
            TestSession::FailTestIfNot(VerifySystemApplicationEvaluation(evaluation), "Verify system app failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS:
            TestSession::FailTestIfNot(VerifyDeployedApplicationsEvaluation(evaluation), "Verify deployed app failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES:
            TestSession::FailTestIfNot(VerifyDeployedServicePackagesEvaluation(evaluation), "Verify deployed service packages failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_EVENT:
            TestSession::FailTestIfNot(VerifyEventEvaluation(evaluation), "Verify event failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS:
            TestSession::FailTestIfNot(VerifyUDDeployedApplicationsEvaluation(evaluation), "Verify deployed app failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES:
            TestSession::FailTestIfNot(VerifyUDNodesEvaluation(evaluation), "Verify nodes failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK:
            TestSession::FailTestIfNot(VerifyDeltaNodesEvaluation(evaluation), "Verify delta nodes failed for {0}", evaluation);
            break;
        case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK:
            TestSession::FailTestIfNot(VerifyUDDeltaNodesEvaluation(evaluation), "Verify upgrade domain delta nodes failed for {0}", evaluation);
            break;
        default:
            TestSession::FailTest("Unexpected health evaluation evaluation {0}: {1}", evaluation->Kind, evaluation->Description);
            break;
        }
    }
}

void TestFabricClientHealth::PrintHealthEvaluations(FABRIC_HEALTH_EVALUATION_LIST const * publicHealthEvaluations, vector<FABRIC_HEALTH_EVALUATION_KIND> const & possibleReasons)
{
    if (publicHealthEvaluations == NULL)
    {
        return;
    }

    vector<HealthEvaluation> unhealthyEvaluations;
    auto error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicHealthEvaluations,
        unhealthyEvaluations);
    TestSession::FailTestIfNot(
        error.IsSuccess(),
        "UnhealthyEvaluations FromPublicApiList error={0}",
        error);

    return PrintHealthEvaluations(unhealthyEvaluations, possibleReasons);
}

void TestFabricClientHealth::PrintHealthEvaluations(vector<HealthEvaluation> const & unhealthyEvaluations, vector<FABRIC_HEALTH_EVALUATION_KIND> const & possibleReasons)
{
    if (unhealthyEvaluations.empty())
    {
        return;
    }

    TestSession::FailTestIfNot(unhealthyEvaluations.size() == 1, "Evaluation reasons count is not valid: {0}", unhealthyEvaluations.size());

    HealthEvaluationBaseSPtr evaluation = unhealthyEvaluations[0].Evaluation;
    TestSession::FailTestIfNot(evaluation != nullptr, "EvaluationWrapper contains null evaluation evaluation");
    TestSession::FailTestIf(evaluation->Description.empty(), "Description is empty for {0}.", evaluation);

    TestSession::WriteInfo(TraceSource, "HealthEvaluation: {0}", evaluation->Description);

    bool isExpected = false;
    for (auto it = possibleReasons.begin(); it != possibleReasons.end(); ++it)
    {
        if (evaluation->Kind != *it)
        {
            continue;
        }

        isExpected = true;
    }
    TestSession::FailTestIfNot(isExpected, "Unexpected health evaluation kind");

    PrintUnhealthyEvaluations(unhealthyEvaluations);
}

bool TestFabricClientHealth::VerifyEventEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedReason = dynamic_cast<EventHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedReason->Description.empty(), "Description is empty for {0}.", evaluation);
    return true;
}

bool TestFabricClientHealth::VerifyNodesEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<NodesHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "There should be at least one child for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);

    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        if (!VerifyNodeEvaluation(itInner->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyNodeEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_NODE)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_NODE, evaluation->Kind);
        return false;
    }

    auto typedEvaluation = dynamic_cast<NodeHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for child {0}", evaluation->Description);
    TestSession::FailTestIfNot(typedEvaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_WARNING || evaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_ERROR, "Child should be at warning or error not {0}", evaluation->AggregatedHealthState);
    TestSession::FailTestIf(typedEvaluation->NodeName.empty(), "Node name is empty for {0}", evaluation);

    // The children may be trimmed, so the number of children can be 0 or 1.
    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    TestSession::FailTestIf(typedEvaluation->UnhealthyEvaluations.size() != 1, "There should be one event unhealthy for {0}", evaluation);
    return VerifyEventEvaluation(typedEvaluation->UnhealthyEvaluations[0].Evaluation);
}

bool TestFabricClientHealth::VerifyUDNodesEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<UDNodesHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "There should be at least one child for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);
    TestSession::FailTestIf(typedEvaluation->UpgradeDomainName.empty(), "UpgradeDomainName should be set for {0}", evaluation);

    // The children may be trimmed
    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        if (!VerifyNodeEvaluation(itInner->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyUDDeltaNodesEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<UpgradeDomainDeltaNodesCheckHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);
    TestSession::FailTestIf(typedEvaluation->MaxPercentUpgradeDomainDeltaUnhealthyNodes >= 100, "Delta can't be greater than 100 for {0}.", evaluation);
    TestSession::FailTestIf(typedEvaluation->UpgradeDomainName.empty(), "UpgradeDomainName should be set for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->BaselineTotalCount < typedEvaluation->BaselineErrorCount, "baseline error can't be greater than baseline total for {0}.", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "current total count can't be 0 for {0}.", evaluation);

    // The children may be trimmed
    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        if (!VerifyNodeEvaluation(itInner->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyReplicasEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<ReplicasHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "There should be at least one child for {0}", evaluation);

    // The children may be trimmed
    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        if (!VerifyReplicaEvaluation(itInner->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyReplicaEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_REPLICA)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_REPLICA, evaluation->Kind);
        return false;
    }

    auto typedEvaluation = dynamic_cast<ReplicaHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for child {0}", evaluation);
    TestSession::FailTestIfNot(typedEvaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_WARNING || evaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_ERROR, "Child should be at warning or error not {0}", evaluation->AggregatedHealthState);
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);

    // The children may be trimmed
    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    TestSession::FailTestIf(typedEvaluation->UnhealthyEvaluations.size() != 1, "There should be at 1 event unhealthy entry for {0}", evaluation);

    return VerifyEventEvaluation(typedEvaluation->UnhealthyEvaluations[0].Evaluation);
}

bool TestFabricClientHealth::VerifyPartitionsEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<PartitionsHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "There should be at least one child for {0}", evaluation);

    // The children may be trimmed
    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        if (!VerifyPartitionEvaluation(itInner->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyPartitionEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_PARTITION)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_PARTITION, evaluation->Kind);
        return false;
    }

    auto typedEvaluation = dynamic_cast<PartitionHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for child {0}", evaluation);
    TestSession::FailTestIfNot(typedEvaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_WARNING || evaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_ERROR, "Child should be at warning or error not {0}", evaluation->AggregatedHealthState);
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);

    // The children may be trimmed
    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    TestSession::FailTestIf(typedEvaluation->UnhealthyEvaluations.size() != 1, "There should be at 1 event unhealthy entry for {0}", evaluation);

    // Children can be either event or replicas
    auto const & eval = typedEvaluation->UnhealthyEvaluations[0].Evaluation;
    switch(eval->Kind)
    {
    case FABRIC_HEALTH_EVALUATION_KIND_EVENT:
        return VerifyEventEvaluation(eval);
    case FABRIC_HEALTH_EVALUATION_KIND_REPLICAS:
        return VerifyReplicasEvaluation(eval);
    default:
        return false;
    }
}

bool TestFabricClientHealth::VerifyServicesEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<ServicesHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "There should be at least one child for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->ServiceTypeName.empty(), "ServiceTypeName should be set for {0}", evaluation);

    // The children may be trimmed
    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        if (!VerifyServiceEvaluation(itInner->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyServiceEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_SERVICE)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_SERVICE, evaluation->Kind);
        return false;
    }

    auto typedEvaluation = dynamic_cast<ServiceHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for child {0}", evaluation);
    TestSession::FailTestIfNot(typedEvaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_WARNING || evaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_ERROR, "Child should be at warning or error not {0}", evaluation->AggregatedHealthState);
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);
    TestSession::FailTestIf(typedEvaluation->ServiceName.empty(), "Service name shouldn't be empty for {0}", evaluation);

    // The children may be trimmed
    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    TestSession::FailTestIf(typedEvaluation->UnhealthyEvaluations.size() != 1, "There should be at 1 event unhealthy entry for {0}", evaluation);

     // Children can be either event or partitions
    auto const & eval = typedEvaluation->UnhealthyEvaluations[0].Evaluation;
    switch(eval->Kind)
    {
    case FABRIC_HEALTH_EVALUATION_KIND_EVENT:
        return VerifyEventEvaluation(eval);
    case FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS:
        return VerifyPartitionsEvaluation(eval);
    default:
        return false;
    }
}

bool TestFabricClientHealth::VerifyDeployedServicePackagesEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<DeployedServicePackagesHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "There should be at least one child for {0}", evaluation);

    // The children may be trimmed
    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        TestSession::FailTestIfNot(VerifyDeployedServicePackageEvaluation(itInner->Evaluation), "VerifyDeployedServicesPackageEvaluation failed");
    }

    return true;
}

bool TestFabricClientHealth::VerifyDeployedServicePackageEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE, evaluation->Kind);
        return false;
    }

    auto typedEvaluation = dynamic_cast<DeployedServicePackageHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for child {0}", evaluation);
    TestSession::FailTestIfNot(typedEvaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_WARNING || evaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_ERROR, "Child should be at warning or error not {0}", evaluation->AggregatedHealthState);
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);
    TestSession::FailTestIf(typedEvaluation->NodeName.empty(), "Node name is empty for {0}", evaluation);

    // The children may be trimmed
    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    TestSession::FailTestIf(typedEvaluation->UnhealthyEvaluations.size() != 1, "There should be at 1 event unhealthy entry for {0}", evaluation);
    return VerifyEventEvaluation(typedEvaluation->UnhealthyEvaluations[0].Evaluation);
}

bool TestFabricClientHealth::VerifyDeployedApplicationsEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<DeployedApplicationsHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "There should be at least one child for {0}", evaluation);

    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        TestSession::FailTestIfNot(VerifyDeployedApplicationEvaluation(itInner->Evaluation), "VerifyDeployedApplicationEvaluation failed");
    }

    return true;
}

bool TestFabricClientHealth::VerifyUDDeployedApplicationsEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<UDDeployedApplicationsHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "There should be at least one child for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->UpgradeDomainName.empty(), "UpgradeDomainName should be set for {0}", evaluation);

    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        TestSession::FailTestIfNot(VerifyDeployedApplicationEvaluation(itInner->Evaluation), "VerifyDeployedApplicationEvaluation failed");
    }

    return true;
}

bool TestFabricClientHealth::VerifyDeployedApplicationEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION, evaluation->Kind);
        return false;
    }

    auto typedEvaluation = dynamic_cast<DeployedApplicationHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for child {0}", evaluation);
    TestSession::FailTestIfNot(typedEvaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_WARNING || evaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_ERROR, "Child should be at warning or error not {0}", evaluation->AggregatedHealthState);
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);
    TestSession::FailTestIf(typedEvaluation->NodeName.empty(), "Node name shouldn't be empty for {0}", evaluation);

    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    TestSession::FailTestIf(typedEvaluation->UnhealthyEvaluations.size() != 1, "There should be at 1 event unhealthy entry for {0}", evaluation);

    // Children can be either event, services or deployed apps
    auto const & eval = typedEvaluation->UnhealthyEvaluations[0].Evaluation;
    switch(eval->Kind)
    {
    case FABRIC_HEALTH_EVALUATION_KIND_EVENT:
        return VerifyEventEvaluation(eval);
    case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES:
        return VerifyDeployedServicePackagesEvaluation(eval);
    default:
        return false;
    }
}

bool TestFabricClientHealth::VerifySystemApplicationEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<SystemApplicationHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIfNot(typedEvaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_WARNING || evaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_ERROR, "Child should be at warning or error not {0}", evaluation->AggregatedHealthState);
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);

    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    auto const & eval = typedEvaluation->UnhealthyEvaluations[0].Evaluation;
    switch(eval->Kind)
    {
    case FABRIC_HEALTH_EVALUATION_KIND_EVENT:
        return VerifyEventEvaluation(eval);
    case FABRIC_HEALTH_EVALUATION_KIND_SERVICES:
        return VerifyServicesEvaluation(eval);
    case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS:
        return VerifyDeployedApplicationsEvaluation(eval);
    default:
        return false;
    }
}

bool TestFabricClientHealth::VerifyApplicationsEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<ApplicationsHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "There should be at least one child for {0}", evaluation);

    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        TestSession::FailTestIfNot(VerifyApplicationEvaluation(itInner->Evaluation), "VerifyApplicationEvaluation failed");
    }

    return true;
}

bool TestFabricClientHealth::VerifyApplicationTypeApplicationsEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<ApplicationTypeApplicationsHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "There should be at least one child for {0}", evaluation);
    TestSession::FailTestIfNot(typedEvaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_WARNING || evaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_ERROR, "Child should be at warning or error not {0}", evaluation->AggregatedHealthState);
    TestSession::FailTestIf(typedEvaluation->ApplicationTypeName.empty(), "Application type name should be set for {0}", evaluation);

    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        TestSession::FailTestIfNot(VerifyApplicationEvaluation(itInner->Evaluation), "VerifyApplicationEvaluation failed");
    }

    return true;
}

bool TestFabricClientHealth::VerifyApplicationEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_APPLICATION)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_APPLICATION, evaluation->Kind);
        return false;
    }

    auto typedEvaluation = dynamic_cast<ApplicationHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for child {0}", evaluation);
    TestSession::FailTestIfNot(typedEvaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_WARNING || evaluation->AggregatedHealthState == FABRIC_HEALTH_STATE_ERROR, "Child should be at warning or error not {0}", evaluation->AggregatedHealthState);
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);

    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    TestSession::FailTestIf(typedEvaluation->UnhealthyEvaluations.size() != 1, "There should be at 1 event unhealthy entry for {0}", evaluation);

    // Children can be either event, services or deployed apps
    auto const & eval = typedEvaluation->UnhealthyEvaluations[0].Evaluation;
    switch(eval->Kind)
    {
    case FABRIC_HEALTH_EVALUATION_KIND_EVENT:
        return VerifyEventEvaluation(eval);
    case FABRIC_HEALTH_EVALUATION_KIND_SERVICES:
        return VerifyServicesEvaluation(eval);
    case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS:
        return VerifyDeployedApplicationsEvaluation(eval);
    default:
        return false;
    }
}

bool TestFabricClientHealth::VerifyDeltaNodesEvaluation(HealthEvaluationBaseSPtr const & evaluation)
{
    auto typedEvaluation = dynamic_cast<DeltaNodesCheckHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedEvaluation == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedEvaluation->Description.empty(), "Description is empty for {0}.", evaluation);
    TestSession::FailTestIf(typedEvaluation->MaxPercentDeltaUnhealthyNodes >= 100, "Delta can't be greater than 100 for {0}.", evaluation);
    TestSession::FailTestIf(typedEvaluation->BaselineTotalCount < typedEvaluation->BaselineErrorCount, "baseline error can't be greater than baseline total for {0}.", evaluation);
    TestSession::FailTestIf(typedEvaluation->TotalCount == 0, "current total count can't be 0 for {0}.", evaluation);

    if (typedEvaluation->UnhealthyEvaluations.empty())
    {
        TestSession::WriteInfo(TraceSource, "Verify evaluations: children are trimmed: {0}", typedEvaluation->Description);
        return true;
    }

    TestSession::FailTestIf(typedEvaluation->UnhealthyEvaluations.empty(), "There should be at least one unhealthy entry for {0}", evaluation);

    for (auto itInner = typedEvaluation->UnhealthyEvaluations.begin(); itInner != typedEvaluation->UnhealthyEvaluations.end(); ++itInner)
    {
        if (!VerifyNodeEvaluation(itInner->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyChildrenEvaluation(
    std::vector<std::wstring> const & expectedEvaluationTokens,
    std::vector<ServiceModel::HealthEvaluation> const & unhealthyEvaluations,
    FABRIC_HEALTH_EVALUATION_KIND expectedChildrenKind,
    ULONG totalCount,
    BYTE maxUnhealthy,
    std::wstring const & description)
{
    TestSession::FailTestIf(totalCount == 0, "There should be at least one child for {0}", description);

    size_t childrenCount = unhealthyEvaluations.size();

    BYTE invalid = static_cast<BYTE>(-1);
    if (expectedEvaluationTokens.size() > 1)
    {
        int expectedUnhealthy = Int32_Parse(expectedEvaluationTokens[1]);
        if (childrenCount != static_cast<size_t>(expectedUnhealthy))
        {
            TestSession::WriteInfo(TraceSource, "Expected children count doesn't match: expected {0}, actual {1}", expectedUnhealthy, childrenCount);
            return false;
        }

        for (auto it = unhealthyEvaluations.begin(); it != unhealthyEvaluations.end(); ++it)
        {
            TestSession::FailTestIfNot(it->Evaluation != nullptr, "Child evaluation is null. Expected unhealthy: {0}", expectedUnhealthy);
            TestSession::FailTestIfNot(it->Evaluation->Description.size() > 0, "Child evaluation has empty description. Expected unhealthy: {0}", expectedUnhealthy);
            TestSession::FailTestIfNot(it->Evaluation->Kind == expectedChildrenKind, "Child evaluation doesn't have expected type. Expected unhealthy: {0}, child {1}", expectedUnhealthy, it->Evaluation);
        }

        if (expectedEvaluationTokens.size() > 2)
        {
            if (maxUnhealthy == invalid)
            {
                TestSession::FailTest("Invalid evaluationreason, request for maxUnhealthy when maxUnhealthy shouldn't exist");
            }

            int expectedMaxUnhealthy = Int32_Parse(expectedEvaluationTokens[2]);
            if (maxUnhealthy != expectedMaxUnhealthy)
            {
                TestSession::WriteInfo(TraceSource, "Expected maxUnhealthyPercent doesn't match: expected {0}, actual {1}", expectedMaxUnhealthy, maxUnhealthy);
                return false;
            }
        }

        if (expectedEvaluationTokens.size() > 3)
        {
            int expectedTotal = Int32_Parse(expectedEvaluationTokens[3]);
            if (totalCount != static_cast<ULONG>(expectedTotal))
            {
                TestSession::WriteInfo(TraceSource, "Expected total children count doesn't match: expected {0}, actual {1}", expectedTotal, totalCount);
                return false;
            }
        }
    }

    if (maxUnhealthy != invalid)
    {
        wstring maxUnhealthyString = wformatString("{0}", maxUnhealthy);
        if (!StringUtility::Contains<wstring>(description, maxUnhealthyString))
        {
            TestSession::WriteInfo(TraceSource, "Reason doesn't contain the maxUnhealthy percent: \"{0}\", maxUnhealthy {1}", description, maxUnhealthy);
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyNodesEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_NODES)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_NODES, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<NodesHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);

    if (!VerifyChildrenEvaluation(expectedEvaluationTokens, typedReason->UnhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND_NODE, typedReason->TotalCount, typedReason->MaxPercentUnhealthyNodes, evaluation->Description))
    {
        return false;
    }

    // Children of nodes should be of type node
    for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
    {
        if (!VerifyNodeEvaluation(it->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyApplicationsEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<ApplicationsHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);

    if (!VerifyChildrenEvaluation(expectedEvaluationTokens, typedReason->UnhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND_APPLICATION, typedReason->TotalCount, typedReason->MaxPercentUnhealthyApplications, evaluation->Description))
    {
        return false;
    }

    for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
    {
        if (!VerifyApplicationEvaluation(it->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyApplicationTypeApplicationsEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<ApplicationTypeApplicationsHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);

    if (expectedEvaluationTokens.size() > 1)
    {
        if (typedReason->ApplicationTypeName != expectedEvaluationTokens[1])
        {
            TestSession::WriteInfo(TraceSource, "Upgrade domain name mismatch: expected {0}, actual {1}", expectedEvaluationTokens[1], typedReason->ApplicationTypeName);
            return false;
        }

        // Check the rest (common parameters);
        vector<wstring> commonTokens;
        for (size_t i = 1; i < expectedEvaluationTokens.size(); ++i)
        {
            commonTokens.push_back(expectedEvaluationTokens[i]);
        }

        if (!VerifyChildrenEvaluation(commonTokens, typedReason->UnhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND_APPLICATION, typedReason->TotalCount, typedReason->MaxPercentUnhealthyApplications, evaluation->Description))
        {
            return false;
        }
    }
    else if (!VerifyChildrenEvaluation(expectedEvaluationTokens, typedReason->UnhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND_APPLICATION, typedReason->TotalCount, typedReason->MaxPercentUnhealthyApplications, evaluation->Description))
    {
        return false;
    }

    for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
    {
        if (!VerifyApplicationEvaluation(it->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyDeployedApplicationsEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<DeployedApplicationsHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);

    if (!VerifyChildrenEvaluation(expectedEvaluationTokens, typedReason->UnhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION, typedReason->TotalCount, typedReason->MaxPercentUnhealthyDeployedApplications, evaluation->Description))
    {
        return false;
    }

    for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
    {
        if (!VerifyDeployedApplicationEvaluation(it->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyDeployedServicePackagesEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<DeployedServicePackagesHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);

    BYTE invalid = static_cast<BYTE>(-1);
    if (!VerifyChildrenEvaluation(expectedEvaluationTokens, typedReason->UnhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE, typedReason->TotalCount, invalid, evaluation->Description))
    {
        return false;
    }

    for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
    {
        if (!VerifyDeployedServicePackageEvaluation(it->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyReplicasEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_REPLICAS)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_REPLICAS, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<ReplicasHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);

    if (!VerifyChildrenEvaluation(expectedEvaluationTokens, typedReason->UnhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND_REPLICA, typedReason->TotalCount, typedReason->MaxPercentUnhealthyReplicasPerPartition, evaluation->Description))
    {
        return false;
    }

    for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
    {
        if (!VerifyReplicaEvaluation(it->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyPartitionsEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<PartitionsHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);

    if (!VerifyChildrenEvaluation(expectedEvaluationTokens, typedReason->UnhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND_PARTITION, typedReason->TotalCount, typedReason->MaxPercentUnhealthyPartitionsPerService, evaluation->Description))
    {
        return false;
    }

    for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
    {
        if (!VerifyPartitionEvaluation(it->Evaluation))
        {
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyServicesEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_SERVICES)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_SERVICES, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<ServicesHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);

    // Check service type name
    if (expectedEvaluationTokens.size() > 1)
    {
        if (typedReason->ServiceTypeName != expectedEvaluationTokens[1])
        {
            TestSession::WriteInfo(TraceSource, "Service type name mismatch: expected {0}, actual {1}", expectedEvaluationTokens[1], typedReason->ServiceTypeName);
            return false;
        }

        // Check the rest (common parameters);
        vector<wstring> commonTokens;
        for (size_t i = 1; i < expectedEvaluationTokens.size(); ++i)
        {
            commonTokens.push_back(expectedEvaluationTokens[i]);
        }

        if (!VerifyChildrenEvaluation(commonTokens, typedReason->UnhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND_SERVICE, typedReason->TotalCount, typedReason->MaxPercentUnhealthyServices, evaluation->Description))
        {
            return false;
        }

        for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
        {
            if (!VerifyServiceEvaluation(it->Evaluation))
            {
                return false;
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifySystemAppEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (expectedEvaluationTokens.size() != 1)
    {
        TestSession::FailTest("Invalid expectedEvalutionReasons: nothing should be passed in for systemapp");
    }

    if (!VerifySystemApplicationEvaluation(evaluation))
    {
        return false;
    }

    return true;
}

bool TestFabricClientHealth::VerifyNodesPerUdEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<UDNodesHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);

    // Check upgrade domain name
    if (expectedEvaluationTokens.size() > 1)
    {
        if (typedReason->UpgradeDomainName != expectedEvaluationTokens[1])
        {
            TestSession::WriteInfo(TraceSource, "Upgrade domain name mismatch: expected {0}, actual {1}", expectedEvaluationTokens[1], typedReason->UpgradeDomainName);
            return false;
        }

        // Check the rest (common parameters);
        vector<wstring> commonTokens;
        for (size_t i = 1; i < expectedEvaluationTokens.size(); ++i)
        {
            commonTokens.push_back(expectedEvaluationTokens[i]);
        }

        if (!VerifyChildrenEvaluation(commonTokens, typedReason->UnhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND_NODE, typedReason->TotalCount, typedReason->MaxPercentUnhealthyNodes, evaluation->Description))
        {
            return false;
        }

        for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
        {
            if (!VerifyNodeEvaluation(it->Evaluation))
            {
                return false;
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyDeltaEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<DeltaNodesCheckHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);
    TestSession::FailTestIf(typedReason->Description.empty(), "unexpected empty description for {0}", evaluation);

    if (expectedEvaluationTokens.size() != 6)
    {
        TestSession::WriteInfo(TraceSource, "expectedreason: invalid number of tokesn {0}", expectedEvaluationTokens);
        return false;
    }

    ULONG baseError, baseTotal, error, total, delta;
    if (!StringUtility::TryFromWString(expectedEvaluationTokens[1], baseError) ||
        !StringUtility::TryFromWString(expectedEvaluationTokens[2], baseTotal) ||
        !StringUtility::TryFromWString(expectedEvaluationTokens[3], error) ||
        !StringUtility::TryFromWString(expectedEvaluationTokens[4], total) ||
        !StringUtility::TryFromWString(expectedEvaluationTokens[5], delta))
    {
        TestSession::WriteError(TraceSource, "expectedreason: Unable to parse {0}", expectedEvaluationTokens);
        return false;
    }

    if (baseError != typedReason->BaselineErrorCount)
    {
        TestSession::WriteInfo(TraceSource, "BaselineError mismatch: expected {0}, actual {1}", baseError, typedReason->BaselineErrorCount);
        return false;
    }

    if (baseTotal != typedReason->BaselineTotalCount)
    {
        TestSession::WriteInfo(TraceSource, "BaselineTotal mismatch: expected {0}, actual {1}", baseTotal, typedReason->BaselineTotalCount);
        return false;
    }

    if (error != static_cast<ULONG>(typedReason->UnhealthyEvaluations.size()))
    {
        TestSession::WriteInfo(TraceSource, "Error mismatch: expected {0}, actual {1}", error, typedReason->UnhealthyEvaluations.size());
        return false;
    }

    if (total != typedReason->TotalCount)
    {
        TestSession::WriteInfo(TraceSource, "Total mismatch: expected {0}, actual {1}", total, typedReason->TotalCount);
        return false;
    }

    if (delta != typedReason->MaxPercentDeltaUnhealthyNodes)
    {
        TestSession::WriteInfo(TraceSource, "MaxPercentDeltaUnhealthyNodes mismatch: expected {0}, actual {1}", delta, typedReason->MaxPercentDeltaUnhealthyNodes);
        return false;
    }

    if (typedReason->UnhealthyEvaluations.size() != static_cast<size_t>(error))
    {
        TestSession::WriteInfo(TraceSource, "nhealthy evaluations size mismatch: expected {0}, actual {1}", error, typedReason->UnhealthyEvaluations.size());
        return false;
    }

    for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
    {
        if (!VerifyNodeEvaluation(it->Evaluation))
        {
            return false;
        }
    }

    wstring totalString = wformatString("{0}", total);
    if (!StringUtility::Contains<wstring>(typedReason->Description, totalString))
    {
        TestSession::WriteInfo(TraceSource, "Reason doesn't contain the total count: \"{0}\", total {1}", typedReason->Description, total);
        return false;
    }

    wstring baseErrorString = wformatString("{0}", baseError);
    if (!StringUtility::Contains<wstring>(typedReason->Description, baseErrorString))
    {
        TestSession::WriteInfo(TraceSource, "Reason doesn't contain the baseline error count: \"{0}\", baseerror {1}", typedReason->Description, baseError);
        return false;
    }

    wstring baseTotalString = wformatString("{0}", baseTotal);
    if (!StringUtility::Contains<wstring>(typedReason->Description, baseTotalString))
    {
        TestSession::WriteInfo(TraceSource, "Reason doesn't contain the baseline total count: \"{0}\", basetotal {1}", typedReason->Description, baseTotal);
        return false;
    }

    return true;
}

bool TestFabricClientHealth::VerifyDeltaPerUdEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<UpgradeDomainDeltaNodesCheckHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);

    if (expectedEvaluationTokens.size() != 7)
    {
        TestSession::WriteInfo(TraceSource, "expectedreason: invalid number of tokesn {0}", expectedEvaluationTokens);
        return false;
    }

    if (typedReason->UpgradeDomainName != expectedEvaluationTokens[1])
    {
        TestSession::WriteInfo(TraceSource, "Upgrade domain name mismatch: expected {0}, actual {1}", expectedEvaluationTokens[1], typedReason->UpgradeDomainName);
        return false;
    }

    ULONG baseError, baseTotal, error, total, delta;
    if (!StringUtility::TryFromWString(expectedEvaluationTokens[2], baseError) ||
        !StringUtility::TryFromWString(expectedEvaluationTokens[3], baseTotal) ||
        !StringUtility::TryFromWString(expectedEvaluationTokens[4], error) ||
        !StringUtility::TryFromWString(expectedEvaluationTokens[5], total) ||
        !StringUtility::TryFromWString(expectedEvaluationTokens[6], delta))
    {
        TestSession::WriteError(TraceSource, "expectedreason: Unable to parse {0}", expectedEvaluationTokens);
        return false;
    }

    if (baseError != typedReason->BaselineErrorCount)
    {
        TestSession::WriteInfo(TraceSource, "BaselineError mismatch: expected {0}, actual {1}", baseError, typedReason->BaselineErrorCount);
        return false;
    }

    if (baseTotal != typedReason->BaselineTotalCount)
    {
        TestSession::WriteInfo(TraceSource, "BaselineTotal mismatch: expected {0}, actual {1}", baseTotal, typedReason->BaselineTotalCount);
        return false;
    }

    if (error != static_cast<ULONG>(typedReason->UnhealthyEvaluations.size()))
    {
        TestSession::WriteInfo(TraceSource, "Error mismatch: expected {0}, actual {1}", error, typedReason->UnhealthyEvaluations.size());
        return false;
    }

    if (total != typedReason->TotalCount)
    {
        TestSession::WriteInfo(TraceSource, "Total mismatch: expected {0}, actual {1}", total, typedReason->TotalCount);
        return false;
    }

    if (delta != typedReason->MaxPercentUpgradeDomainDeltaUnhealthyNodes)
    {
        TestSession::WriteInfo(TraceSource, "MaxPercentUpgradeDomainDeltaUnhealthyNodes mismatch: expected {0}, actual {1}", delta, typedReason->MaxPercentUpgradeDomainDeltaUnhealthyNodes);
        return false;
    }

    if (typedReason->UnhealthyEvaluations.size() != static_cast<size_t>(error))
    {
        TestSession::WriteInfo(TraceSource, "Healthy evaluations size mismatch: expected {0}, actual {1}", error, typedReason->UnhealthyEvaluations.size());
        return false;
    }

    for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
    {
        if (!VerifyNodeEvaluation(it->Evaluation))
        {
            return false;
        }
    }

    wstring baseErrorString = wformatString("{0}", baseError);
    if (!StringUtility::Contains<wstring>(typedReason->Description, baseErrorString))
    {
        TestSession::WriteInfo(TraceSource, "Reason doesn't contain the baseline error count: \"{0}\", baseerror {1}", typedReason->Description, baseError);
        return false;
    }

    wstring baseTotalString = wformatString("{0}", baseTotal);
    if (!StringUtility::Contains<wstring>(typedReason->Description, baseTotalString))
    {
        TestSession::WriteInfo(TraceSource, "Reason doesn't contain the baseline total count: \"{0}\", basetotal {1}", typedReason->Description, baseTotal);
        return false;
    }

    wstring totalString = wformatString("{0}", total);
    if (!StringUtility::Contains<wstring>(typedReason->Description, totalString))
    {
        TestSession::WriteInfo(TraceSource, "Reason doesn't contain the total count: \"{0}\", total {1}", typedReason->Description, total);
        return false;
    }

    return true;
}

bool TestFabricClientHealth::VerifyDeployedApplicationsPerUdEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS, evaluation->Kind);
        return false;
    }

    auto typedReason = dynamic_cast<UDDeployedApplicationsHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(typedReason == NULL, "Conversion to type returned null for {0}", evaluation);

    // Check service type name
    if (expectedEvaluationTokens.size() > 1)
    {
        if (typedReason->UpgradeDomainName != expectedEvaluationTokens[1])
        {
            TestSession::WriteInfo(TraceSource, "Upgrade domain name mismatch: expected {0}, actual {1}", expectedEvaluationTokens[1], typedReason->UpgradeDomainName);
            return false;
        }

        // Check the rest (common parameters);
        vector<wstring> commonTokens;
        for (size_t i = 1; i < expectedEvaluationTokens.size(); ++i)
        {
            commonTokens.push_back(expectedEvaluationTokens[i]);
        }

        if (!VerifyChildrenEvaluation(commonTokens, typedReason->UnhealthyEvaluations, FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION, typedReason->TotalCount, typedReason->MaxPercentUnhealthyDeployedApplications, evaluation->Description))
        {
            return false;
        }

        for (auto it = typedReason->UnhealthyEvaluations.begin(); it != typedReason->UnhealthyEvaluations.end(); ++it)
        {
            if (!VerifyDeployedApplicationEvaluation(it->Evaluation))
            {
                return false;
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyEventEvaluation(std::vector<std::wstring> const & expectedEvaluationTokens, HealthEvaluationBaseSPtr const & evaluation)
{
    if (evaluation->Kind != FABRIC_HEALTH_EVALUATION_KIND_EVENT)
    {
        TestSession::WriteInfo(TraceSource, "HealthEvaluationBase kind mismatch: Expected {0}, received {1}", FABRIC_HEALTH_EVALUATION_KIND_EVENT, evaluation->Kind);
        return false;
    }

    auto eventReason = dynamic_cast<EventHealthEvaluation *>(evaluation.get());
    TestSession::FailTestIf(eventReason == NULL, "Conversion to type returned null for {0}", evaluation);

    if (expectedEvaluationTokens.size() > 1)
    {
        if (eventReason->UnhealthyEvent.SourceId != expectedEvaluationTokens[1])
        {
            TestSession::WriteInfo(TraceSource, "EventEvaluation: Source mismatch: expected {0}, received {1}", expectedEvaluationTokens[1], eventReason->UnhealthyEvent.SourceId);
            return false;
        }

        if (expectedEvaluationTokens.size() > 2)
        {
            if (eventReason->UnhealthyEvent.Property != expectedEvaluationTokens[2])
            {
                TestSession::WriteInfo(TraceSource, "EventEvaluation: Property mismatch: expected {0}, received {1}", expectedEvaluationTokens[2], eventReason->UnhealthyEvent.Property);
                return false;
            }

            if (expectedEvaluationTokens.size() > 3)
            {
                int64 sn;
                if (!Common::TryParseInt64(expectedEvaluationTokens[3], sn))
                {
                    TestSession::WriteInfo(TraceSource, "EventEvaluation: error reading sn from expected: expected {0}, received {1}", expectedEvaluationTokens[3], eventReason->UnhealthyEvent.SequenceNumber);
                    return false;
                }

                if (eventReason->UnhealthyEvent.SequenceNumber != sn)
                {
                    TestSession::WriteInfo(TraceSource, "EventEvaluation: sequence number mismatch: expected {0}, received {1}", expectedEvaluationTokens[3], eventReason->UnhealthyEvent.SequenceNumber);
                    return false;
                }
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyHealthStateCount(
    std::wstring const & expectedHealthStatesString,
    HealthStateCount const & counts)
{
    // Expected format: ok:<number>,warning:<number>,error:<number>
    vector<wstring> tokens;
    StringUtility::Split<wstring>(expectedHealthStatesString, tokens, L",");
    int expectedTotalCount = -1;
    int expectedOkCount = 0;
    int expectedWarningCount = 0;
    int expectedErrorCount = 0;
    for (auto it = tokens.begin(); it != tokens.end(); ++it)
    {
        vector<wstring> innerTokens;
        StringUtility::Split<wstring>(*it, innerTokens, L":");
        if (innerTokens.size() == 1u)
        {
            expectedTotalCount = Int32_Parse(innerTokens[0]);
            // If total is requested, no per states information will be checked
            TestSession::FailTestIfNot(tokens.size() == 1u, "Invalid health state check command: global count requested with other tokens in {0}", expectedHealthStatesString);
        }
        else if (innerTokens.size() > 2u)
        {
            TestSession::WriteError(TraceSource, "Could not parse expectedHealthStates inner token {0} in {1}", *it, expectedHealthStatesString);
            return false;
        }
        else
        {
            int expectedStateCount = Int32_Parse(innerTokens[1]);
            switch(GetHealthState(innerTokens[0]))
            {
            case FABRIC_HEALTH_STATE_OK: expectedOkCount = expectedStateCount; break;
            case FABRIC_HEALTH_STATE_WARNING: expectedWarningCount = expectedStateCount; break;
            case FABRIC_HEALTH_STATE_ERROR: expectedErrorCount = expectedStateCount; break;
            default: TestSession::FailTest("Unknown health state {0}", innerTokens[0]);
            }
        }
    }

    int receivedOkCount = static_cast<int>(counts.OkCount);
    int receivedWarningCount = static_cast<int>(counts.WarningCount);
    int receivedErrorCount = static_cast<int>(counts.ErrorCount);

    int receivedTotalCount = receivedErrorCount + receivedOkCount + receivedWarningCount;
    if (expectedTotalCount != -1)
    {
        if (expectedTotalCount != receivedTotalCount)
        {
            TestSession::WriteInfo(TraceSource, "Invalid health query total state count: expected {0}, actual {1}/{2}/{3} (total {4}).", expectedTotalCount, receivedOkCount, receivedWarningCount, receivedErrorCount, receivedTotalCount);
            return false;
        }
    }
    else if (expectedOkCount != receivedOkCount ||
        expectedWarningCount != receivedWarningCount ||
        expectedErrorCount != receivedErrorCount)
    {
        TestSession::WriteInfo(TraceSource, "Invalid health query state count: expected {0}/{1}/{2}, actual {3}/{4}/{5}.", expectedOkCount, expectedWarningCount, expectedErrorCount, receivedOkCount, receivedWarningCount, receivedErrorCount);
        return false;
    }

    return true;
}

bool TestFabricClientHealth::VerifyHealthStateCountMap(
    std::wstring const & expectedHealthStatesString,
    HealthStateCountMap const & results)
{
    TestSession::WriteInfo(TraceSource, "VerifyHealthStateCountMap: Received health state map: {0}", results);

    // Parameter format:
    // expectedstates=<type>+
    // Where per type info can be either the total count, total count per states (ok/warning/error) or count and count per states for each type (nodes, apps etc)
    // Example:
    // expectedstates=10
    // expectedstates=ok:4,warning:6
    // expectedstates=nodes-4;apps-ok:5,warning:2,error:1

    vector<wstring> typeInfo;
    StringUtility::Split<wstring>(expectedHealthStatesString, typeInfo, L";");
    for (auto itTypeInfo = typeInfo.begin(); itTypeInfo != typeInfo.end(); ++itTypeInfo)
    {
        vector<wstring> typeTokens;
        StringUtility::Split<wstring>(*itTypeInfo, typeTokens, L"-");
        if (typeTokens.size() > 2)
        {
            TestSession::WriteInfo(TraceSource, "Invalid command format: type info has too many separators for {0}.", *itTypeInfo);
            return false;
        }

        if (typeTokens.size() == 2)
        {
            // First token represents the type
            auto itActualResults = results.end();
            if (typeTokens[0] == L"nodes")
            {
                itActualResults = results.find(EntityKind::Node);
            }
            else if (typeTokens[0] == L"replicas")
            {
                itActualResults = results.find(EntityKind::Replica);
            }
            else if (typeTokens[0] == L"partitions")
            {
                itActualResults = results.find(EntityKind::Partition);
            }
            else if (typeTokens[0] == L"services")
            {
                itActualResults = results.find(EntityKind::Service);
            }
            else if (typeTokens[0] == L"apps")
            {
                itActualResults = results.find(EntityKind::Application);
            }
            else if (typeTokens[0] == L"deployedservicepackages")
            {
                itActualResults = results.find(EntityKind::DeployedServicePackage);
            }
            else if (typeTokens[0] == L"deployedapps")
            {
                itActualResults = results.find(EntityKind::DeployedApplication);
            }
            else
            {
                TestSession::WriteInfo(TraceSource, "Invalid type {0} for {1}", typeTokens[0], *itTypeInfo);
                return false;
            }

            TestSession::FailTestIf(itActualResults == results.end(), "Expected {0} not returned by GetHealth. Expected children: {1}", typeTokens[0], expectedHealthStatesString);
            if (!VerifyHealthStateCount(typeTokens[1], itActualResults->second))
            {
                TestSession::WriteInfo(TraceSource, "Verify HealthStateCount for {0} failed, expected {1}", typeTokens[0], typeTokens[1]);
                return false;
            }
        }
        else
        {
            // Check total count/health states
            HealthStateCount globalResults;
            for (auto itResults = results.begin(); itResults != results.end(); ++itResults)
            {
                globalResults.AppendCount(itResults->second);
            }

            if (!VerifyHealthStateCount(typeTokens[0], globalResults))
            {
                return false;
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyAggregatedHealthStates(CommandLineParser & parser, HealthStateCountMap const & results)
{
    wstring expectedHealthStatesString;
    if (parser.TryGetString(L"expectedchildrenhealthstates", expectedHealthStatesString) ||
        parser.TryGetString(L"expectedhealthstates", expectedHealthStatesString) ||
        parser.TryGetString(L"expectedchildrencount", expectedHealthStatesString))
    {
        TestSession::WriteWarning(TraceSource, "{0} parameter is no longer supported", expectedHealthStatesString);
        return false;
    }

    wstring expectedChildrenString;
    if (!parser.TryGetString(L"expectedstates", expectedChildrenString))
    {
        // No validation needed
        return true;
    }

    return VerifyHealthStateCountMap(expectedChildrenString, results);
}

void TestFabricClientHealth::CreateFabricHealthClient(__in FabricTestFederation & testFederation)
{
    if (healthClient_.GetRawPointer() == nullptr)
    {
        ComPointer<IFabricHealthClient4> result;
        HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
            IID_IFabricHealthClient4,
            (void **)result.InitializationAddress());
        TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricHealthClient");

        healthClient_ = result;
    }
}

void TestFabricClientHealth::CreateInternalFabricHealthClient(__in FabricTestFederation & testFederation)
{
    if (internalHealthClient_.get() == NULL)
    {
        auto factoryPtr = TestFabricClient::FabricCreateNativeClientFactory(testFederation);
        auto error = factoryPtr->CreateHealthClient(internalHealthClient_);
        TestSession::FailTestIfNot(error.IsSuccess(), "InternalClientFactory does not support IHealthClient");
    }
}

bool TestFabricClientHealth::CheckClusterEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success)
{
    auto entity = hmPrimary->EntityManager->Cluster.GetEntity(*Management::HealthManager::Constants::StoreType_ClusterTypeString);
    AttributesStoreDataSPtr baseAttributes;
    success = CheckHealthEntityCommonParameters(parser, entity, baseAttributes);
    return true;
}

bool TestFabricClientHealth::CheckNodeEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success)
{
    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    auto entity = hmPrimary->EntityManager->Nodes.GetEntity(nodeId.IdValue);
    AttributesStoreDataSPtr baseAttributes;
    success = CheckHealthEntityCommonParameters(parser, entity, baseAttributes);
    if (!success)
    {
        return true;
    }

    if (baseAttributes)
    {
        auto attributes = static_cast<NodeAttributesStoreData*>(baseAttributes.get());

        if (attributes->HasSystemReport && nodeName != attributes->NodeName)
        {
            TestSession::WriteInfo(TraceSource, "CheckNodeEntityState: nodeName mismatch: expected '{0}'/ actual '{1}'", nodeName, attributes->NodeName);
            success = false;
            return true;
        }

        if (nodeInstanceId != attributes->NodeInstanceId)
        {
            TestSession::WriteInfo(TraceSource, "CheckNodeEntityState: nodeInstanceId mismatch: expected '{0}'/ actual '{1}'", nodeInstanceId, attributes->InstanceId);
            success = false;
            return true;
        }

        if (!attributes->IsMarkedForDeletion && !attributes->IsCleanedUp && attributes->HasSystemReport)
        {
            wstring ipAddressOrFqdn;
            if (parser.TryGetString(L"ipaddressorfqdn", ipAddressOrFqdn))
            {
                if (!attributes->AttributeSetFlags.IsIpAddressOrFqdnSet() ||
                    attributes->IpAddressOrFqdn != ipAddressOrFqdn)
                {
                    TestSession::WriteInfo(TraceSource, "CheckNodeEntityState: ipaddressorfqdn mismatch: expected '{0}'/ actual '{1}'", ipAddressOrFqdn, attributes->IpAddressOrFqdn);
                    success = false;
                    return true;
                }
            }

            wstring upgradeDomain;
            if (parser.TryGetString(L"ud", upgradeDomain))
            {
                if (!attributes->AttributeSetFlags.IsUpgradeDomainSet() ||
                     attributes->UpgradeDomain != upgradeDomain)
                {
                    TestSession::WriteInfo(TraceSource, "CheckNodeEntityState: ud mismatch expected: '{0}'/ actual '{1}'", upgradeDomain, attributes->UpgradeDomain);
                    success = false;
                    return true;
                }
            }

            wstring faultDomain;
            if (parser.TryGetString(L"fd", faultDomain))
            {
                if (!attributes->AttributeSetFlags.IsFaultDomainSet() ||
                     attributes->FaultDomain != faultDomain)
                {
                    TestSession::WriteInfo(TraceSource, "CheckNodeEntityState: fd mismatch expected: '{0}'/ actual '{1}'", faultDomain, attributes->FaultDomain);
                    success = false;
                    return true;
                }
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::CheckReplicaEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success)
{
    FABRIC_REPLICA_ID replicaId;
    FABRIC_INSTANCE_ID replicaInstanceId;
    Common::Guid partitionGuid;
    if (!ParseReplicaId(parser, partitionGuid, replicaId, replicaInstanceId))
    {
        return false;
    }

    AttributesStoreDataSPtr baseAttributes;
    auto entity = hmPrimary->EntityManager->Replicas.GetEntity(ReplicaHealthId(partitionGuid, replicaId));
    success = CheckHealthEntityCommonParameters(parser, entity, baseAttributes);
    if (!success)
    {
        return true;
    }

    if (baseAttributes)
    {
        auto attributes = static_cast<ReplicaAttributesStoreData*>(baseAttributes.get());

        if (replicaInstanceId != FABRIC_INVALID_INSTANCE_ID && replicaInstanceId != attributes->InstanceId)
        {
            TestSession::WriteInfo(TraceSource, "CheckEntityState: replicaInstanceId mismatch: expected '{0}'/ actual '{1}'", replicaInstanceId, attributes->InstanceId);
            success = false;
            return true;
        }

        if (!attributes->IsMarkedForDeletion && !attributes->IsCleanedUp)
        {
            NodeId nodeId;
            FABRIC_NODE_INSTANCE_ID nodeInstanceId;
            std::wstring nodeName;
            if (ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
            {
                if (!attributes->AttributeSetFlags.IsNodeIdSet() ||
                    attributes->NodeId != nodeId.IdValue)
                {
                    TestSession::WriteInfo(TraceSource, "CheckEntityState: nodeid mismatch expected: '{0}'/ actual '{1}'", nodeId, attributes->NodeId);
                    success = false;
                    return true;
                }

                if (!attributes->AttributeSetFlags.IsNodeInstanceIdSet() ||
                    attributes->NodeInstanceId != nodeInstanceId)
                {
                    TestSession::WriteInfo(TraceSource, "CheckEntityState: nodeInstanceId mismatch expected: '{0}'/ actual '{1}'", nodeInstanceId, attributes->NodeInstanceId);
                    success = false;
                    return true;
                }
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::CheckPartitionEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success)
{
    PartitionHealthId partitionId;
    if (!ParsePartitionId(parser, partitionId))
    {
        return false;
    }

    auto entity = hmPrimary->EntityManager->Partitions.GetEntity(partitionId);
    AttributesStoreDataSPtr baseAttributes;
    success = CheckHealthEntityCommonParameters(parser, entity, baseAttributes);
    if (!success)
    {
        return true;
    }

    if (baseAttributes)
    {
        auto attributes = static_cast<PartitionAttributesStoreData*>(baseAttributes.get());

        if (!attributes->IsMarkedForDeletion && !attributes->IsCleanedUp)
        {
            wstring serviceName;
            if (parser.TryGetString(L"servicename", serviceName, wstring()))
            {
                if (!attributes->AttributeSetFlags.IsServiceNameSet() ||
                    attributes->ServiceName != serviceName)
                {
                    TestSession::WriteInfo(TraceSource, "CheckEntityState: servicename mismatch expected: '{0}'/ actual '{1}'", serviceName, attributes->ServiceName);
                    success = false;
                    return true;
                }
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::CheckServiceEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success)
{
    wstring serviceName;
    if (!parser.TryGetString(L"servicename", serviceName))
    {
        return false;
    }

    auto entity = hmPrimary->EntityManager->Services.GetEntity(ServiceHealthId(serviceName));
    AttributesStoreDataSPtr baseAttributes;
    success = CheckHealthEntityCommonParameters(parser, entity, baseAttributes);
    if (!success)
    {
        return true;
    }

    if (baseAttributes)
    {
        auto attributes = static_cast<ServiceAttributesStoreData*>(baseAttributes.get());

        if (!attributes->IsMarkedForDeletion && !attributes->IsCleanedUp)
        {
            wstring serviceTypeName;
            if (parser.TryGetString(L"servicetypename", serviceTypeName, wstring()))
            {
                if (!attributes->AttributeSetFlags.IsServiceTypeNameSet() ||
                    attributes->ServiceTypeName != serviceTypeName)
                {
                    TestSession::WriteInfo(TraceSource, "CheckEntityState: servicetypename mismatch expected: '{0}'/ actual '{1}'", serviceTypeName, attributes->ServiceTypeName);
                    success = false;
                    return true;
                }
            }

            wstring appName;
            if (parser.TryGetString(L"appname", appName, wstring()))
            {
                if (!attributes->AttributeSetFlags.IsApplicationNameSet() ||
                    attributes->ApplicationName != appName)
                {
                    TestSession::WriteInfo(TraceSource, "CheckEntityState: appName mismatch expected: '{0}'/ actual '{1}'", appName, attributes->ApplicationName);
                    success = false;
                    return true;
                }
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::CheckDeployedServicePackageEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success)
{
    wstring appName;
    if (!parser.TryGetString(L"appname", appName, wstring()))
    {
        return false;
    }

    wstring serviceManifestName;
    if (!parser.TryGetString(L"servicemanifestname", serviceManifestName))
    {
        return false;
    }

    wstring servicePackageActivationId;
    if (!ParseServicePackageActivationId(parser, servicePackageActivationId))
    {
        return false;
    }

    TestSession::WriteInfo(TraceSource, "CheckDeployedServicePackageEntityState(): Using servicePackageActivationId=[{0}].", servicePackageActivationId);

    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    auto entity = hmPrimary->EntityManager->DeployedServicePackages.GetEntity(
        DeployedServicePackageHealthId(appName, serviceManifestName, servicePackageActivationId, nodeId.IdValue));

    AttributesStoreDataSPtr baseAttributes;
    success = CheckHealthEntityCommonParameters(parser, entity, baseAttributes);
    if (!success)
    {
        return true;
    }

    if (baseAttributes)
    {
        auto attributes = static_cast<DeployedServicePackageAttributesStoreData*>(baseAttributes.get());

        FABRIC_INSTANCE_ID servicePackageInstanceId;
        parser.TryGetInt64(L"servicepackageinstanceid", servicePackageInstanceId, FABRIC_INVALID_INSTANCE_ID);

        if (servicePackageInstanceId != FABRIC_INVALID_INSTANCE_ID && servicePackageInstanceId != attributes->InstanceId)
        {
            TestSession::WriteInfo(TraceSource, "CheckEntityState: servicePackageInstanceId mismatch: expected '{0}'/ actual '{1}'", servicePackageInstanceId, attributes->InstanceId);
            success = false;
            return true;
        }

        if (!attributes->IsMarkedForDeletion && !attributes->IsCleanedUp)
        {
            if (nodeInstanceId != FABRIC_INVALID_NODE_INSTANCE_ID &&
               (!attributes->AttributeSetFlags.IsNodeInstanceIdSet() || attributes->NodeInstanceId != nodeInstanceId))
            {
                TestSession::WriteInfo(TraceSource, "CheckEntityState: nodeInstanceId mismatch expected: '{0}'/ actual '{1}'", nodeInstanceId, attributes->NodeInstanceId);
                success = false;
                return true;
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::CheckDeployedApplicationEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success)
{
    wstring appName;
    if (!parser.TryGetString(L"appname", appName, wstring()))
    {
        return false;
    }

    NodeId nodeId;
    FABRIC_NODE_INSTANCE_ID nodeInstanceId;
    std::wstring nodeName;
    if (!ParseNodeHealthInformation(parser, nodeId, nodeInstanceId, nodeName))
    {
        return false;
    }

    auto entity = hmPrimary->EntityManager->DeployedApplications.GetEntity(DeployedApplicationHealthId(appName, nodeId.IdValue));
    AttributesStoreDataSPtr baseAttributes;
    success = CheckHealthEntityCommonParameters(parser, entity, baseAttributes);
    if (!success)
    {
        return true;
    }

    if (baseAttributes)
    {
        auto attributes = static_cast<DeployedApplicationAttributesStoreData*>(baseAttributes.get());

        FABRIC_INSTANCE_ID applicationInstanceId;
        parser.TryGetInt64(L"appinstanceid", applicationInstanceId, FABRIC_INVALID_INSTANCE_ID);

        if (applicationInstanceId != FABRIC_INVALID_INSTANCE_ID && applicationInstanceId != attributes->InstanceId)
        {
            TestSession::WriteInfo(TraceSource, "CheckEntityState: applicationInstanceId mismatch: expected '{0}'/ actual '{1}'", applicationInstanceId, attributes->InstanceId);
            success = false;
            return true;
        }

        if (!attributes->IsMarkedForDeletion && !attributes->IsCleanedUp)
        {
            if (nodeInstanceId != FABRIC_INVALID_NODE_INSTANCE_ID &&
               (!attributes->AttributeSetFlags.IsNodeInstanceIdSet() || attributes->NodeInstanceId != nodeInstanceId))
            {
                TestSession::WriteInfo(TraceSource, "CheckEntityState: nodeInstanceId mismatch: expected '{0}'/ actual '{1}'", nodeInstanceId, attributes->NodeInstanceId);
                success = false;
                return true;
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::CheckApplicationEntityState(CommandLineParser & parser, Management::HealthManager::HealthManagerReplicaSPtr const & hmPrimary, bool & success)
{
    wstring applicationName;
    if (!parser.TryGetString(L"appname", applicationName))
    {
        return false;
    }

    auto entity = hmPrimary->EntityManager->Applications.GetEntity(ApplicationHealthId(applicationName));
    AttributesStoreDataSPtr baseAttributes;
    success = CheckHealthEntityCommonParameters(parser, entity, baseAttributes);
    if (!success)
    {
        return true;
    }

    if (baseAttributes)
    {
        auto attributes = static_cast<ApplicationAttributesStoreData*>(baseAttributes.get());

        if (success && !attributes->IsMarkedForDeletion && !attributes->IsCleanedUp && attributes->HasSystemReport)
        {
            wstring applicationTypeName;
            if (parser.TryGetString(L"apptype", applicationTypeName))
            {
                if (!attributes->AttributeSetFlags.IsApplicationTypeNameSet() ||
                    attributes->ApplicationTypeName != applicationTypeName)
                {
                    TestSession::WriteInfo(TraceSource, "CheckApplicationEntityState: apptypeName mismatch: expected '{0}'/ actual '{1}'", applicationTypeName, attributes->ApplicationTypeName);
                    success = false;
                    return true;
                }
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::CheckHealthEntityCommonParameters(
    __in CommandLineParser & parser,
    HealthEntitySPtr const & entity,
    __inout AttributesStoreDataSPtr & baseAttributes)
{
    if (entity)
    {
        HealthEntityState::Enum entityState;
        vector<HealthEvent> events;
        if (!entity->Test_GetData(entityState, baseAttributes, events))
        {
            return false;
        }

        int expectedEventCount;
        if (parser.TryGetInt(L"expectedeventcount", expectedEventCount))
        {
            if (static_cast<int>(events.size()) != expectedEventCount)
            {
                TestSession::WriteInfo(TraceSource, "Unexpected event count: expected {0}, actual {1}.", expectedEventCount, events.size());
                return false;
            }
        }

        wstring expectedEntityStateString;
        if (parser.TryGetString(L"expectedentitystate", expectedEntityStateString))
        {
            wstring actualEntityState = wformatString(entityState);
            if (!StringUtility::AreEqualCaseInsensitive(expectedEntityStateString, actualEntityState))
            {
                TestSession::WriteInfo(TraceSource, "Unexpected entity health state: expected {0}, actual {1}.", expectedEntityStateString, actualEntityState);
                return false;
            }
        }

        wstring expectedState;
        if (parser.TryGetString(L"state", expectedState))
        {
            return CheckEntityState(expectedState, *baseAttributes);
        }

        return true;
    }
    else
    {
        wstring expectedState;
        if (!parser.TryGetString(L"state", expectedState))
        {
            TestSession::WriteInfo(TraceSource, "Entity not found and \"state\" parameter not provided");
            return false;
        }

        return CheckCleanedUpEntityState(expectedState);
    }
}

bool TestFabricClientHealth::CheckCleanedUpEntityState(std::wstring const & expectedState)
{
    TestSession::WriteNoise(TraceSource, "CheckCleanedUpEntityState: expectedState={0}", expectedState);
    return (expectedState == L"cleanedup") || (expectedState == L"deleted") || (expectedState == L"notfound");
}

bool TestFabricClientHealth::CheckEntityState(std::wstring const & expectedState, AttributesStoreData const & attributes)
{
    TestSession::WriteNoise(TraceSource, "CheckEntityState: {0}, expectedState={1}", attributes, expectedState);
    bool success;
    if (expectedState == L"notfound")
    {
        success = false;
    }
    if (expectedState == L"cleanedup")
    {
        success = attributes.IsCleanedUp;
    }
    else if (expectedState == L"deleted")
    {
        success = attributes.IsCleanedUp || attributes.IsMarkedForDeletion;
    }
    else if (expectedState == L"nosystemreport")
    {
        success = attributes.ExpectSystemReports && (!attributes.HasSystemReport);
    }
    else if (expectedState == L"systemerror")
    {
        success = attributes.HasSystemError;
    }
    else if (expectedState == L"ok")
    {
        success = attributes.HasSystemReport && (!attributes.IsMarkedForDeletion) && (!attributes.IsCleanedUp);
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "CheckEntityState failed: unknown expectedState={0}, actual: {1}", expectedState, attributes);
        success = false;
    }

    if (!success)
    {
        TestSession::WriteInfo(TraceSource, "CheckEntityState failed: expectedState={0}, actual: {1}", expectedState, attributes);
    }

    return success;
}

wstring TestFabricClientHealth::ConvertHealthOutput(wstring const & input)
{
    size_t i = input.find_first_of(L'(');
    if (i != wstring::npos && input.back() == L')')
    {
        return input.substr(i + 1, input.size() - i - 2);
    }

    return input;
}

bool TestFabricClientHealth::ParseUnhealthyState(std::wstring const & input, __inout ULONG & error, __inout ULONG & total)
{
    vector<wstring> unhealthyState;
    StringUtility::Split<wstring>(input, unhealthyState, L"/");
    if (unhealthyState.size() != 2)
    {
        TestSession::WriteError(TraceSource, "Unable to parse unhealthy state {0}: expected error/total format", input);
        return false;
    }

    if (!StringUtility::TryFromWString(unhealthyState[0], error))
    {
        TestSession::WriteError(TraceSource, "Unable to parse error count from {0}", input);
        return false;
    }

    if (!StringUtility::TryFromWString(unhealthyState[1], total))
    {
        TestSession::WriteError(TraceSource, "Unable to parse total count from {0}", input);
        return false;
    }

    return true;
}

bool TestFabricClientHealth::ParseSnapshot(std::wstring const & input, Management::HealthManager::ClusterUpgradeStateSnapshot & snapshot)
{
    vector<wstring> tokens;
    StringUtility::Split<wstring>(input, tokens, L",");
    for (wstring const & entry : tokens)
    {
        vector<wstring> uds;
        StringUtility::Split<wstring>(entry, uds, L":");
        if (uds.size() == 1)
        {
            // global
            ULONG error;
            ULONG total;
            if (!ParseUnhealthyState(uds[0], error, total))
            {
                return false;
            }

            snapshot.SetGlobalState(error, total);
        }
        else if (uds.size() == 2)
        {
            ULONG error;
            ULONG total;
            if (!ParseUnhealthyState(uds[1], error, total))
            {
                return false;
            }

            snapshot.AddUpgradeDomainEntry(uds[0], error, total);
        }
        else
        {
            TestSession::WriteError(TraceSource, "Unable to parse snapshot {0}: {1} is not global or ud entry", input, entry);
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::TakeClusterHealthSnapshot(StringCollection const & params)
{
    CommandLineParser parser(params);

    bool requiresValidation = false;

    wstring snapshotString;
    ClusterUpgradeStateSnapshot expectedSnapshot;
    if (parser.TryGetString(L"expected", snapshotString))
    {
        requiresValidation = true;
        if (!ParseSnapshot(snapshotString, expectedSnapshot))
        {
            return false;
        }
    }

    Management::HealthManager::HealthManagerReplicaSPtr hmPrimary;
    if (!GetHMPrimaryWithRetries(hmPrimary))
    {
        return false;
    }

    ActivityId activityId;
    ClusterUpgradeStateSnapshot actualSnapshot;
    auto error = hmPrimary->GetClusterUpgradeSnapshot(activityId, actualSnapshot);
    TestSession::FailTestIfNot(error.IsSuccess(), "hm->GetClusterUpgradeSnapshot returned {0}", error);

    // Print snapshot
    wstring actualSnapshotString;
    StringWriter writer(actualSnapshotString);
    writer.Write("Global={0}/{1}, Uds=[", actualSnapshot.GlobalUnhealthyState.ErrorCount, actualSnapshot.GlobalUnhealthyState.TotalCount);
    for (auto const & entry : actualSnapshot.UpgradeDomainUnhealthyStates)
    {
        writer.Write("{0}:{1}/{2} ", entry.first, entry.second.ErrorCount, entry.second.TotalCount);
    }
    writer.Write("]");

    TestSession::WriteInfo(TraceSource, "Snapshot: {0}", actualSnapshotString);

    if (requiresValidation)
    {
        if (expectedSnapshot.GlobalUnhealthyState != actualSnapshot.GlobalUnhealthyState)
        {
            TestSession::WriteInfo(
                TraceSource,
                "Snapshot global mismatch: expected {0}/{1}, actual {2}/{3}",
                expectedSnapshot.GlobalUnhealthyState.ErrorCount,
                expectedSnapshot.GlobalUnhealthyState.TotalCount,
                actualSnapshot.GlobalUnhealthyState.ErrorCount,
                actualSnapshot.GlobalUnhealthyState.TotalCount);
            return false;
        }

        if (expectedSnapshot.UpgradeDomainUnhealthyStates.size() != actualSnapshot.UpgradeDomainUnhealthyStates.size())
        {
            TestSession::WriteInfo(TraceSource, "Snapshot ud count mismatch: expected {0}, actual {1}", expectedSnapshot.UpgradeDomainUnhealthyStates.size(), actualSnapshot.UpgradeDomainUnhealthyStates.size());
            return false;
        }

        for (auto const & entry : actualSnapshot.UpgradeDomainUnhealthyStates)
        {
            ULONG err;
            ULONG total;
            if (!expectedSnapshot.TryGetUpgradeDomainEntry(entry.first, err, total).IsSuccess())
            {
                TestSession::WriteInfo(TraceSource, "Returned UD is not expected {0}", entry.first);
                return false;
            }

            UnhealthyState expectedState(err, total);
            if (entry.second != expectedState)
            {
                TestSession::WriteInfo(
                    TraceSource,
                    "Snapshot UD mismatch for {0}: expected {1}/{2}, actual {3}/{4}",
                    entry.first,
                    expectedState.ErrorCount,
                    expectedState.TotalCount,
                    entry.second.ErrorCount,
                    entry.second.TotalCount);
                return false;
            }
        }
    }

    return true;
}

bool TestFabricClientHealth::GetApplicationHealthPolicies(__in CommandLineParser & parser, __inout ApplicationHealthPolicyMapSPtr & applicationHealthPolicies)
{
    wstring jsonApplicationHealthPolicies;
    if (parser.TryGetString(L"apphealthpolicies", jsonApplicationHealthPolicies))
    {
        applicationHealthPolicies = make_shared<ApplicationHealthPolicyMap>();
        auto error = ApplicationHealthPolicyMap::FromString(jsonApplicationHealthPolicies, *applicationHealthPolicies);
        if (!error.IsSuccess())
        {
            TestSession::WriteWarning(TraceSource, "Failed to parse application health policies string {0} with {1}", jsonApplicationHealthPolicies, error);
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::GetHMPrimaryWithRetries(__inout Management::HealthManager::HealthManagerReplicaSPtr & hm)
{
    int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    DWORD retryDelay = static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds());
    do
    {
        --remainingRetries;
        hm = FABRICSESSION.FabricDispatcher.GetHM();
        if (hm)
        {
            return true;
        }

        TestSession::WriteInfo(TraceSource, "GetHMPrimaryWithRetries: HM not ready, remaining retries {0}, sleep {1} msec", remainingRetries, retryDelay);
        Sleep(retryDelay);
    } while (remainingRetries >= 0);

    TestSession::WriteWarning(TraceSource, "GetHMPrimaryWithRetries: Could not get HM and retries are exhausted.");
    return false;
}

bool TestFabricClientHealth::CheckIsClusterHealthy(StringCollection const & params)
{
    CommandLineParser parser(params);

    wstring snapshotString;
    ClusterUpgradeStateSnapshot baseline;
    if (parser.TryGetString(L"baseline", snapshotString))
    {
        if (!ParseSnapshot(snapshotString, baseline))
        {
            return false;
        }
    }

    wstring jsonPolicy;
    shared_ptr<ClusterHealthPolicy> clusterPolicy;
    if (parser.TryGetString(L"jsonpolicy", jsonPolicy))
    {
        clusterPolicy = make_shared<ClusterHealthPolicy>();
        auto error = ClusterHealthPolicy::FromString(jsonPolicy, *clusterPolicy);
        if (!error.IsSuccess())
        {
            TestSession::WriteWarning(TraceSource, "Failed to parse cluster health policy string {0} with {1}", jsonPolicy, error);
            return false;
        }
    }

    wstring jsonUpgradePolicy;
    shared_ptr<ClusterUpgradeHealthPolicy> clusterUpgradePolicy;
    if (parser.TryGetString(L"jsonupgradepolicy", jsonUpgradePolicy))
    {
        clusterUpgradePolicy = make_shared<ClusterUpgradeHealthPolicy>();
        auto error = ClusterUpgradeHealthPolicy::FromString(jsonUpgradePolicy, *clusterUpgradePolicy);
        if (!error.IsSuccess())
        {
            TestSession::WriteWarning(TraceSource, "Failed to parse cluster upgrade health policy string {0} with {1}", jsonUpgradePolicy, error);
            return false;
        }
    }

    vector<wstring> upgradeDomains;
    wstring upgradeDomainList;
    if (!parser.TryGetString(L"upgradedomains", upgradeDomainList))
    {
        TestSession::WriteWarning(TraceSource, "Required parameter upgradedomains is missing");
        return false;
    }

    StringUtility::Split<wstring>(upgradeDomainList, upgradeDomains, L",");

    size_t expectedAppsWithoutAppTypeCount;
    int expectedAppsWithoutAppTypeCountInt;
    parser.TryGetInt(L"expectedappswithoutapptypecount", expectedAppsWithoutAppTypeCountInt, 0);
    expectedAppsWithoutAppTypeCount = static_cast<size_t>(expectedAppsWithoutAppTypeCountInt);

    vector<wstring> expectedAppsWithoutAppType;
    wstring appsWithoutAppTypeList;
    if (parser.TryGetString(L"expectedappswithoutapptype", appsWithoutAppTypeList))
    {
        StringUtility::Split<wstring>(appsWithoutAppTypeList, expectedAppsWithoutAppType, L",");
        if (expectedAppsWithoutAppTypeCount == 0u)
        {
            expectedAppsWithoutAppTypeCount = expectedAppsWithoutAppType.size();
        }
    }

    bool expectedIsHealthy = false;
    wstring expectedHealthyString;
    if (parser.TryGetString(L"expectedhealthy", expectedHealthyString))
    {
        expectedIsHealthy = parser.GetBool(L"expectedhealthy");
    }
    else if (expectedAppsWithoutAppTypeCount == 0)
    {
        TestSession::WriteWarning(TraceSource, "Required parameter expectedhealthy is missing");
        return false;
    }

    ApplicationHealthPolicyMapSPtr applicationHealthPolicies;
    if (!GetApplicationHealthPolicies(parser, applicationHealthPolicies))
    {
        return false;
    }

    Management::HealthManager::HealthManagerReplicaSPtr hmPrimary;
    if (!GetHMPrimaryWithRetries(hmPrimary))
    {
        return false;
    }

    ActivityId activityId;
    bool actualIsHealthy;
    vector<HealthEvaluation> unhealthyEvaluations;
    vector<wstring> actualAppsWithoutAppType;
    auto error = hmPrimary->Test_IsClusterHealthy(
        activityId,
        upgradeDomains,
        clusterPolicy,
        clusterUpgradePolicy,
        applicationHealthPolicies,
        baseline,
        actualIsHealthy,
        unhealthyEvaluations,
        actualAppsWithoutAppType);

    if (expectedAppsWithoutAppTypeCount > 0)
    {
        // Expect to receive error
        TestSession::FailTestIfNot(error.IsError(ErrorCodeValue::ApplicationTypeNotFound), "{0}: Test_IsClusterHealthy returned {1}, but expected {2} apps without app types {3}", activityId, error, expectedAppsWithoutAppTypeCount, expectedAppsWithoutAppType);
        if (actualAppsWithoutAppType.size() != expectedAppsWithoutAppTypeCount)
        {
            TestSession::WriteInfo(TraceSource, "{0}: mismatch for apps without app types count: expected {1}, actual {2}", activityId, expectedAppsWithoutAppTypeCount, actualAppsWithoutAppType.size());
            return false;
        }

        if (!expectedAppsWithoutAppType.empty())
        {
            // Sort the vectors
            sort(expectedAppsWithoutAppType.begin(), expectedAppsWithoutAppType.end());
            sort(actualAppsWithoutAppType.begin(), actualAppsWithoutAppType.end());
            for (size_t i = 0; i < expectedAppsWithoutAppType.size(); ++i)
            {
                if (actualAppsWithoutAppType[i] != expectedAppsWithoutAppType[i])
                {
                    TestSession::WriteInfo(TraceSource, "{0}: mismatch for apps without app types at position {1}: expected {2}, actual {3}", activityId, i, expectedAppsWithoutAppType[i], actualAppsWithoutAppType[i]);
                    return false;
                }
            }
        }

        return true;
    }

    TestSession::FailTestIfNot(error.IsSuccess(), "{0}: Error executing Test_IsClusterHealthy: {1}", activityId, error);

    if (actualIsHealthy)
    {
        if (!unhealthyEvaluations.empty())
        {
            TestSession::FailTest("{0}: IsClusterHealthy returned healthy and {1} evaluation results: {1}", activityId, unhealthyEvaluations.size(), unhealthyEvaluations[0].Evaluation->Description);
        }
    }
    else
    {
        TestSession::FailTestIf(unhealthyEvaluations.empty(), "{0}: IsClusterHealthy returned unhealthy and 0 evaluations", activityId);
    }

    if (expectedIsHealthy != actualIsHealthy)
    {
        TestSession::WriteInfo(TraceSource, "{0}: mismatch for isHealthy result: expected {1}, actual {2}", activityId, expectedIsHealthy, actualIsHealthy);
        return false;
    }

    wstring expectedEvaluation;
    if (parser.TryGetString(L"expectedreason", expectedEvaluation))
    {
        return VerifyExpectedHealthEvaluations(expectedEvaluation, unhealthyEvaluations);
    }
    else
    {
        PrintUnhealthyEvaluations(unhealthyEvaluations);
    }

    return true;
}

bool TestFabricClientHealth::SetHMThrottle(StringCollection const & params)
{
    if (params.size() == 0)
    {
        return false;
    }

    Management::HealthManager::HealthManagerReplicaSPtr hm;
    if (!GetHMPrimaryWithRetries(hm))
    {
        return false;
    }

    CommandLineParser parser(params);
    bool shouldThrottle = parser.GetBool(L"throttle");
    hm->JobQueueManager.Test_SetThrottle(shouldThrottle);
    return true;
}

bool TestFabricClientHealth::CheckHealthStats(__in CommandLineParser & parser, HealthStatisticsUPtr const & healthStats)
{
    wstring excludeHealthStatsString;
    bool excludeHealthStats = parser.GetBool(L"excludeHealthStats");

    if (excludeHealthStats)
    {
        if (healthStats != nullptr)
        {
            TestSession::WriteInfo(TraceSource, "CheckHealthStats: excludeHealthStates=true but received health stats {0}", *healthStats);
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (healthStats == nullptr)
    {
        TestSession::WriteInfo(TraceSource, "CheckHealthStats: excludeHealthStates=false but no health stats received");
        return false;
    }

    wstring expectedHealthStatesString;
    if (!parser.TryGetString(L"stats", expectedHealthStatesString))
    {
        // No need to validate
        return true;
    }

    HealthStateCountMap results;

    if (healthStats)
    {
        results = healthStats->GetHealthCountMap();
    }

    wstring expectedStats;
    if (!parser.TryGetString(L"stats", expectedStats))
    {
        // No validation needed
        return true;
    }

    return VerifyHealthStateCountMap(expectedStats, results);
}

bool TestFabricClientHealth::CheckHM(StringCollection const & params)
{
    if (params.size() == 0)
    {
        return false;
    }

    Management::HealthManager::HealthManagerReplicaSPtr hm;
    if (!GetHMPrimaryWithRetries(hm))
    {
        return false;
    }

    CommandLineParser parser(params);
    int expectedCount = -1;
    parser.TryGetInt(L"expectedcount", expectedCount);

    ErrorCode error;
    size_t actualCount;
    int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    ActivityId activityId;
    do
    {
        --remainingRetries;

        vector<HealthEntitySPtr> entities;
        if (params[0] == L"nodes")
        {
            error = hm->EntityManager->Nodes.GetEntities(activityId, entities);
        }
        else if (params[0] == L"partitions")
        {
            error = hm->EntityManager->Partitions.GetEntities(activityId, entities);
        }
        else if (params[0] == L"replicas")
        {
            error = hm->EntityManager->Replicas.GetEntities(activityId, entities);
        }
        else if (params[0] == L"applications")
        {
            error = hm->EntityManager->Applications.GetEntities(activityId, entities);
        }
        else if (params[0] == L"services")
        {
            error = hm->EntityManager->Services.GetEntities(activityId, entities);
        }
        else if (params[0] == L"deployedapplications")
        {
            error = hm->EntityManager->DeployedApplications.GetEntities(activityId, entities);
        }
        else if (params[0] == L"deployedservicepackages")
        {
            error = hm->EntityManager->DeployedServicePackages.GetEntities(activityId, entities);
        }
        else
        {
            return false;
        }

        actualCount = entities.size();

        if (error.IsSuccess() && expectedCount == static_cast<int>(actualCount))
        {
            return true;
        }

        // Create entitiesId string, because WriteInfo doesn't know how to trace out entities by default.
        wstring entitiesId;
        if (actualCount > 0)
        {
            entitiesId = wformatString(L"\r\nThe list of actual {0} entities by ID:\r\n", entities[0]->EntityKind);
            for (auto const & entity : entities)
            {
                entitiesId.append(wformatString(L"{0}\r\n", entity->EntityIdString));
            }
        }

        TestSession::WriteInfo(TraceSource, "CheckHM failed: expectedCount={0}, actualCount={1}, remaining retries {2}{3}", expectedCount, actualCount, remainingRetries, entitiesId);
        Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
    }
    while (remainingRetries > 0);

    TestSession::WriteWarning(TraceSource, "CheckHM failed: expectedCount={0}, timeout expired", expectedCount);
    return false;
}

bool TestFabricClientHealth::ShowHM(HealthManagerReplicaSPtr const & hm, StringCollection const & params)
{
    if (params.size() == 0)
    {
        return false;
    }

    ErrorCode error;
    ActivityId activityId;
    vector<HealthEntitySPtr> entities;
    if (params[0] == L"nodes")
    {
        error = hm->EntityManager->Nodes.GetEntities(activityId, entities);
    }
    else if (params[0] == L"partitions")
    {
        error = hm->EntityManager->Partitions.GetEntities(activityId, entities);
    }
    else if (params[0] == L"replicas")
    {
        error = hm->EntityManager->Replicas.GetEntities(activityId, entities);
    }
    else if (params[0] == L"applications")
    {
        error = hm->EntityManager->Applications.GetEntities(activityId, entities);
    }
    else if (params[0] == L"services")
    {
        error = hm->EntityManager->Services.GetEntities(activityId, entities);
    }
    else if (params[0] == L"deployedapplications")
    {
        error = hm->EntityManager->DeployedApplications.GetEntities(activityId, entities);
    }
    else if (params[0] == L"deployedservicepackages")
    {
        error = hm->EntityManager->DeployedServicePackages.GetEntities(activityId, entities);
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "ShowHM: Unknown entity type {0}", params[0]);
        return false;
    }

    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "Error getting {0} from HM cache: {1}", params[0], error);
        return false;
    }

    wstring pattern;
    bool countOnly = false;
    if (params.size() > 1)
    {
        if (params[1] == L"count")
        {
            countOnly = true;
        }
        else
        {
            pattern = params[1];
        }
    }

    int count = 0;
    bool showReplicas = (params[0] == L"partitions");

    for (auto const & entity : entities)
    {
        if (pattern.empty() || StringUtility::Contains<wstring>(entity->EntityIdString, pattern))
        {
            HealthEntityState::Enum entityState;
            AttributesStoreDataSPtr baseAttributes;
            vector<HealthEvent> events;
            if (entity->Test_GetData(entityState, baseAttributes, events))
            {
                if (!countOnly)
                {
                    TestSession::WriteInfo(TraceSource, "\n{0}, entityState {1}", TestFabricClientHealth::ConvertHealthOutput(wformatString(*baseAttributes)), entityState);
                    for (auto it = events.begin(); it != events.end(); ++it)
                    {
                        TestSession::WriteInfo(TraceSource, "\t{0}", ConvertHealthOutput(wformatString(*it)));
                    }

                    if (showReplicas)
                    {
                        auto partition = PartitionsCache::GetCastedEntityPtr(entity);
                        TestSession::WriteInfo(TraceSource, "Replicas:");
                        set<ReplicaEntitySPtr> replicas = partition->GetReplicas();
                        for (ReplicaEntitySPtr const & replica : replicas)
                        {
                            TestSession::WriteInfo(TraceSource, "\t{0}", replica->EntityIdString);
                        }
                    }
                }

                ++count;
            }
            else
            {
                TestSession::WriteInfo(TraceSource, "\nNot loaded, entity state {0}", entityState);
            }
        }
    }

    TestSession::WriteInfo(TraceSource, "{0} {1} entities", count, params[0]);

    return true;
}

bool TestFabricClientHealth::VerifyNodeHealthReports(HealthManagerReplicaSPtr const & hm, vector<NodeSnapshot> const & nodes, size_t expectedUpCount)
{
    vector<NodeEntitySPtr> hmNodes;
    if (!healthTable_->Update(hm, hmNodes))
    {
        return false;
    }

    FABRIC_SEQUENCE_NUMBER invalidatedSequence = hm->EntityManager->Nodes.Test_GetInvalidatedLsn(*ServiceModel::Constants::HealthReportFMSource);

    size_t hmUpCount = 0;
    for (NodeEntitySPtr const & node : hmNodes)
    {
        bool isInvalidated = false;

        HealthEntityState::Enum entityState;
        shared_ptr<AttributesStoreData> entityBase;
        vector<HealthEvent> events;
        if (!node->Test_GetData(entityState, entityBase, events))
        {
            TestSession::WriteInfo(TraceSource, "HM get node failed for {0}, entity state {1}", node->EntityId, entityState);
            return false;
        }

        auto entity = dynamic_cast<NodeAttributesStoreData*>(entityBase.get());
        bool found = false;
        for (auto it = events.begin(); it != events.end(); ++it)
        {
            if (it->SourceId == *ServiceModel::Constants::HealthReportFMSource)
            {
                found = true;

                TestSession::FailTestIfNot(
                    (it->Property == *ServiceModel::Constants::HealthStateProperty) && (it->TimeToLive == TimeSpan::MaxValue),
                    "{0}: Unexpected health event: {1}",
                    node->EntityIdString,
                    *it);

                if (it->State == FABRIC_HEALTH_STATE_OK)
                {
                    ++hmUpCount;
                    auto itNode = find_if(nodes.begin(), nodes.end(),
                        [node](NodeSnapshot const & fmNode) { return fmNode.Id == node->EntityId; });
                    if (itNode == nodes.end())
                    {
                        TestSession::WriteInfo(TraceSource, "Up node {0} in HM not found in FM", node->EntityId);
                        return false;
                    }
                    else if (!itNode->IsUp)
                    {
                        TestSession::WriteInfo(TraceSource, "Up node {0} in HM is down in FM", node->EntityId);
                        return false;
                    }
                    else if (itNode->HealthSequence != it->SequenceNumber)
                    {
                        TestSession::WriteInfo(TraceSource, "HM node {0} sequence number {1} does not match FM {2}", node->EntityId, it->SequenceNumber, itNode->HealthSequence);
                        return false;
                    }
                    else if ((itNode->NodeInstance.InstanceId != entity->NodeInstanceId) ||
                        (itNode->NodeName != entity->NodeName) ||
                        (itNode->FaultDomainId != entity->FaultDomain) ||
                        (itNode->ActualUpgradeDomainId != entity->UpgradeDomain) ||
                        (itNode->IpAddressOrFQDN != entity->IpAddressOrFqdn))
                    {
                        TestSession::WriteInfo(TraceSource, "HM node property {0} does not match FM {1}", *entity, *itNode);
                        return false;
                    }
                }

                isInvalidated = (it->SequenceNumber < invalidatedSequence);
            }
        }

        if (!found)
        {
            TestSession::WriteInfo(TraceSource, "Node {0} did not find FM report", node->EntityIdString);
            return false;
        }

        if (!healthTable_->Verify(node, events, isInvalidated))
        {
            return false;
        }
    }

    if (hmUpCount != expectedUpCount)
    {
        TestSession::WriteInfo(TraceSource, "HM up node count {0} does not match FM {1}", hmUpCount, expectedUpCount);
        return false;
    }

    return true;
}

bool TestFabricClientHealth::VerifyParititionHealthReports(HealthManagerReplicaSPtr const & hm, map<FailoverUnitId, FailoverUnitSnapshot> const & failoverUnits)
{
    set<FailoverUnitId> fmFailoverUnits;
    for (auto it = failoverUnits.begin(); it != failoverUnits.end(); ++it)
    {
        if (!it->second.IsOrphaned)
        {
            fmFailoverUnits.insert(it->first);
        }
    }

    vector<PartitionEntitySPtr> hmPartitions;
    if (!healthTable_->Update(hm, hmPartitions))
    {
        return false;
    }

    FABRIC_SEQUENCE_NUMBER invalidatedSequence = hm->EntityManager->Partitions.Test_GetInvalidatedLsn(*ServiceModel::Constants::HealthReportFMSource);

    for (PartitionEntitySPtr const & partition : hmPartitions)
    {
        bool isInvalidated = false;

        HealthEntityState::Enum entityState;
        shared_ptr<AttributesStoreData> entityBase;
        vector<HealthEvent> events;
        if (!partition->Test_GetData(entityState, entityBase, events))
        {
            TestSession::WriteInfo(TraceSource, "HM get partition failed for {0}, entity state {1}", partition->EntityId, entityState);
            return false;
        }

        auto entity = static_cast<PartitionAttributesStoreData*>(entityBase.get());
        bool found = false;
        for (auto it = events.begin(); it != events.end(); ++it)
        {
            if (it->SourceId == *ServiceModel::Constants::HealthReportFMSource)
            {
                found = true;
                isInvalidated = (it->SequenceNumber < invalidatedSequence);

                TestSession::FailTestIf(
                    it->Property != *ServiceModel::Constants::HealthStateProperty ||
                    it->TimeToLive != TimeSpan::MaxValue,
                    "Unexpected health event: {0}", *it);

                auto itFailoverUnit = failoverUnits.find(FailoverUnitId(entity->EntityId));
                if (itFailoverUnit == failoverUnits.end() || itFailoverUnit->second.IsOrphaned)
                {
                    if (dispatcher_.IsRebuildLostFU(entity->EntityId))
                    {
                        continue;
                    }

                    TestSession::WriteInfo(TraceSource, "HM Partition {0}\n{1} not found in FM", *entity, *it);
                    return false;
                }

                fmFailoverUnits.erase(itFailoverUnit->first);

                FailoverUnitSnapshot const * failoverUnit = &(itFailoverUnit->second);
                if (failoverUnit->HealthSequence != it->SequenceNumber)
                {
                    TestSession::WriteInfo(TraceSource, "HM partition {0}\n{1} sequence number does not match FM {2}", *entity, *it, failoverUnit->HealthSequence);
                    return false;
                }
                else if (SystemServiceApplicationNameHelper::GetPublicServiceName(failoverUnit->ServiceName) != entity->ServiceName)
                {
                    TestSession::WriteInfo(TraceSource, "HM partition {0}\n{1} attributes do not match FM {2}", *entity, *it, failoverUnit->GetServiceInfo().ServiceDescription);
                    return false;
                }
                else if (failoverUnit->IsQuorumLost())
                {
                    if (failoverUnit->CurrentHealthState != FailoverUnitHealthState::QuorumLost)
                    {
                        TestSession::WriteInfo(TraceSource, "FM partition state {0} expected to be in quorum loss: {1}", failoverUnit->CurrentHealthState, *failoverUnit);
                        return false;
                    }

                    if (it->State != FABRIC_HEALTH_STATE_ERROR || !StringUtility::Contains(it->Description, HMResource::GetResources().PartitionQuorumLoss))
                    {
                        TestSession::WriteInfo(TraceSource, "HM Partition {0}\n{1} does not match FM quorum loss state", *entity, *it);
                        return false;
                    }
                }
                else if (failoverUnit->IsInReconfiguration)
                {
                    if (failoverUnit->CurrentHealthState != FailoverUnitHealthState::ReconfigurationStuck)
                    {
                        TestSession::WriteInfo(TraceSource, "FM partition state {0} expected to be in reconfiguration stuck: {1}: {1}", failoverUnit->CurrentHealthState, *failoverUnit);
                        return false;
                    }

                    if (it->State != FABRIC_HEALTH_STATE_WARNING || !StringUtility::Contains(it->Description, HMResource::GetResources().PartitionReconfigurationStuck))
                    {
                        TestSession::WriteInfo(TraceSource, "HM Partition {0}\n{1} does not match FM reconfiguration state", *entity, *it);
                        return false;
                    }
                }
                else if ((failoverUnit->IsStateful && static_cast<int>(failoverUnit->GetCurrentConfigurationReplicas().size()) < failoverUnit->TargetReplicaSetSize) ||
                    (!failoverUnit->IsStateful && static_cast<int>(failoverUnit->ReplicaCount) < failoverUnit->TargetReplicaSetSize))
                {
                    if (failoverUnit->CurrentHealthState != FailoverUnitHealthState::PlacementStuck)
                    {
                        TestSession::WriteInfo(TraceSource, "FM partition state {0} expected to be in placement stuck: {1}", failoverUnit->CurrentHealthState, *failoverUnit);
                        return false;
                    }

                    if (it->State != FABRIC_HEALTH_STATE_WARNING || !StringUtility::Contains(it->Description, HMResource::GetResources().PartitionPlacementStuck))
                    {
                        TestSession::WriteInfo(TraceSource, "HM Partition {0}\n{1} does not match FM below replica count state", *entity, *it);
                        return false;
                    }
                }
                else
                {
                    bool inBuild = false;
                    auto replicas = failoverUnit->GetReplicas();
                    for (auto replica = replicas.begin(); replica != replicas.end(); ++replica)
                    {
                        if (replica->IsUp && replica->IsInBuild)
                        {
                            inBuild = true;

                            if (failoverUnit->CurrentHealthState != FailoverUnitHealthState::BuildStuck)
                            {
                                TestSession::WriteInfo(TraceSource, "FM partition state {0} expected to be in placement stuck for replica {1}: {2}", failoverUnit->CurrentHealthState, replica->FederationNodeId, *failoverUnit);
                                return false;
                            }

                            if (it->State != FABRIC_HEALTH_STATE_WARNING || !StringUtility::StartsWith(it->Description, HMResource::GetResources().PartitionPlacementStuck) || !StringUtility::Contains(it->Description, replica->GetNode().NodeName))
                            {
                                TestSession::WriteInfo(TraceSource, "HM Partition {0}\n{1} does not match FM below replica inbuild state", *entity, *it);
                                return false;
                            }
                        }
                    }

                    if (!inBuild)
                    {
                        if (failoverUnit->CurrentHealthState != FailoverUnitHealthState::Healthy)
                        {
                            TestSession::WriteInfo(TraceSource, "FM partition state {0} expected to be healthy: {1}", failoverUnit->CurrentHealthState, *failoverUnit);
                            return false;
                        }

                        if (it->State != FABRIC_HEALTH_STATE_OK || !StringUtility::Contains(it->Description, HMResource::GetResources().PartitionHealthy))
                        {
                            TestSession::WriteInfo(TraceSource, "HM Partition {0}\n{1} does not match FM healthy state", *entity, *it);
                            return false;
                        }
                    }
                }
            }
        }

        if (!found && partition->EntityId != Reliability::Constants::FMServiceGuid)
        {
            TestSession::WriteInfo(TraceSource, "Partition {0} did not find FM report", partition->EntityId);
            return false;
        }

        if (!healthTable_->Verify(partition, events, isInvalidated))
        {
            return false;
        }
    }

    if (fmFailoverUnits.size() > 0)
    {
        TestSession::WriteInfo(TraceSource, "{0} FM partitions not found in HM: {1}", fmFailoverUnits.size(), *fmFailoverUnits.begin());
        return false;
    }

    return true;
}

bool TestFabricClientHealth::VerifyReplicaHealthReports(
    HealthManagerReplicaSPtr const & hm,
    map<FailoverUnitId, FailoverUnitSnapshot> const & failoverUnits)
{
    map<wstring, bool> fmReplicas;
    for (auto it = failoverUnits.begin(); it != failoverUnits.end(); ++it)
    {
        auto replicas = it->second.GetReplicas();
        for (auto replica = replicas.begin(); replica != replicas.end(); ++replica)
        {
            if (replica->IsUp && !replica->IsCreating &&
                (!it->second.IsStateful || (replica->CurrentConfigurationRole != ReplicaRole::None && !replica->IsStandBy)))
            {
                wstring id = wformatString("{0}+{1}", it->second.Id, replica->ReplicaId);
                fmReplicas.insert(make_pair(move(id), it->second.IsQuorumLost()));
            }
        }
    }

    TestSession::WriteNoise(TraceSource, "FM up replicas :{0}", fmReplicas.size());

    vector<ReplicaEntitySPtr> hmReplicas;
    if (!healthTable_->Update(hm, hmReplicas))
    {
        return false;
    }

    for (ReplicaEntitySPtr const & replica : hmReplicas)
    {
        HealthEntityState::Enum entityState;
        shared_ptr<AttributesStoreData> entityBase;
        vector<HealthEvent> events;
        if (!replica->Test_GetData(entityState, entityBase, events))
        {
            TestSession::WriteInfo(TraceSource, "HM get replica failed: {0}, entity state {1}", replica->EntityId, entityState);
            return false;
        }

        auto entity = static_cast<ReplicaAttributesStoreData*>(entityBase.get());
        bool found = false;
        for (auto it = events.begin(); it != events.end(); ++it)
        {
            if (it->SourceId == *ServiceModel::Constants::HealthReportRASource)
            {
                found = true;

                if (it->Property == *ServiceModel::Constants::HealthStateProperty)
                {
                    TestSession::FailTestIf(
                        it->TimeToLive != TimeSpan::MaxValue,
                        "Unexpected health event (TTL not infinite for EventProperty: {0}, EventState: {1}): {2}", it->Property, it->State, *it);
                }

                if (replica->EntityId.PartitionId == Reliability::Constants::FMServiceGuid ||
                    dispatcher_.IsRebuildLostFailoverUnit(replica->EntityId.PartitionId))
                {
                    continue;
                }

                auto itFailoverUnit = failoverUnits.find(FailoverUnitId(entity->EntityId.PartitionId));
                if (itFailoverUnit == failoverUnits.end() || itFailoverUnit->second.IsOrphaned)
                {
                    TestSession::WriteInfo(TraceSource, "Replica {0} in HM not found in FM partition", *entity);
                    return false;
                }

                auto fmReplica = itFailoverUnit->second.GetReplica(entity->NodeId);
                if (!fmReplica)
                {
                    TestSession::WriteInfo(TraceSource, "Replica {0} in HM not found in FM", *entity);
                    return false;
                }

                if (entity->EntityId.ReplicaId != fmReplica->ReplicaId ||
                    entity->InstanceId != (itFailoverUnit->second.IsStateful ? fmReplica->InstanceId : -2) ||
                    entity->NodeInstanceId != fmReplica->GetNode().NodeInstance.InstanceId)
                {
                    TestSession::WriteInfo(TraceSource, "Replica attributes {0} in HM does not match FM {1}", *entity, *fmReplica);
                    return false;
                }

                if (it->State != FABRIC_HEALTH_STATE_OK)
                {
                    TestSession::WriteInfo(TraceSource, "Replica {0} in HM is not healthy", *it);

                    // This is a special check because ClearError health report may not be propertly generated due to bug 7188617.
                    // This will be removed after that bug is fixed.
                    if (it->State == FABRIC_HEALTH_STATE_ERROR &&
                        it->Property == *ServiceModel::Constants::HealthReplicaChangeRoleStatusProperty &&
                        itFailoverUnit->second.IsInReconfiguration)
                    {
                        TestSession::WriteInfo(TraceSource, "Ignore health verification for replica {0} event {1} because ClearError health report may not be generated properly.", replica->EntityIdString, *it);
                    }
                    else if (it->State == FABRIC_HEALTH_STATE_WARNING &&
                            it->Property == *ServiceModel::Constants::ReconfigurationProperty &&
                            itFailoverUnit->second.IsInReconfiguration)
                    {
                        TestSession::WriteInfo(TraceSource, "Ignore health verification for replica {0} event {1} because ReconfigurationStuck warning is expected.", replica->EntityIdString, *it);
                    }
                    else
                    {
                        return false;
                    }
                }

                if (!fmReplica->IsUp)
                {
                    TestSession::WriteInfo(TraceSource, "Replica {0} is down in FM", replica->EntityId);
                    return false;
                }

                fmReplicas.erase(replica->EntityId.ToString());
            }
        }

        if (!found)
        {
            TestSession::WriteInfo(TraceSource, "Replica {0} did not find RA report", replica->EntityId);
            return false;
        }

        if (!healthTable_->Verify(replica, events))
        {
            return false;
        }
    }

    for (auto it = fmReplicas.begin(); it != fmReplicas.end(); ++it)
    {
        if (!it->second)
        {
            TestSession::WriteInfo(TraceSource, "{0} replicas on FM not found in HM: {1}", fmReplicas.size(), it->first);
            return false;
        }
    }

    return true;
}

bool TestFabricClientHealth::VerifyServiceHealthReports(HealthManagerReplicaSPtr const & hm, vector<ServiceSnapshot> const & services)
{
    map<wstring, ServiceSnapshot> fmServices;
    for (auto it = services.begin(); it != services.end(); ++it)
    {
        if (!it->IsDeleted && it->ServiceDescription.Name != Reliability::Constants::TombstoneServiceName)
        {
            fmServices.insert(make_pair(SystemServiceApplicationNameHelper::GetPublicServiceName(it->ServiceDescription.Name), *it));
        }
    }

    vector<ServiceEntitySPtr> hmServices;
    if (!healthTable_->Update(hm, hmServices))
    {
        return false;
    }

    FABRIC_SEQUENCE_NUMBER invalidatedSequence = hm->EntityManager->Services.Test_GetInvalidatedLsn(*ServiceModel::Constants::HealthReportFMSource);

    for (ServiceEntitySPtr const & service : hmServices)
    {
        if (service->EntityId.ServiceName == ServiceModel::SystemServiceApplicationNameHelper::PublicFMServiceName)
        {
            continue;
        }

        bool isInvalidated = false;

        HealthEntityState::Enum entityState;
        shared_ptr<AttributesStoreData> entityBase;
        vector<HealthEvent> events;
        if (!service->Test_GetData(entityState, entityBase, events))
        {
            TestSession::WriteInfo(TraceSource, "HM get service failed: {0}, entity state {1}", service->EntityId, entityState);
            return false;
        }

        auto entity = static_cast<ServiceAttributesStoreData*>(entityBase.get());
        bool found = false;
        for (auto it = events.begin(); it != events.end(); ++it)
        {
            if (it->SourceId == *ServiceModel::Constants::HealthReportFMSource)
            {
                found = true;

                TestSession::FailTestIf(
                    it->Property != *ServiceModel::Constants::HealthStateProperty ||
                    it->TimeToLive != TimeSpan::MaxValue ||
                    it->State != FABRIC_HEALTH_STATE_OK,
                    "Unexpected health event: {0}", *it);

                auto itService = fmServices.find(service->EntityId.ServiceName);
                if (itService == fmServices.end())
                {
                    auto partitions = service->GetPartitions();
                    wstring partitionOutput;

                    for (PartitionEntitySPtr const & partition : partitions)
                    {
                        if (!dispatcher_.IsRebuildLostFailoverUnit(partition->EntityId))
                        {
                            partitionOutput = partition->EntityIdString;
                            break;
                        }
                    }

                    if (partitionOutput.size() > 0)
                    {
                        TestSession::WriteInfo(TraceSource, "Service {0} with {1} partitions {2} in HM not found in FM",
                            *entity, partitions.size(), partitionOutput);
                        return false;
                    }
                }
                else
                {
                    if (itService->second.HealthSequence != it->SequenceNumber)
                    {
                        TestSession::WriteInfo(TraceSource, "HM service {0} sequence number {1} does not match FM {2}", service->EntityId, entity->SequenceNumber, itService->second.HealthSequence);
                        return false;
                    }

                    fmServices.erase(itService);
                }

                isInvalidated = (it->SequenceNumber < invalidatedSequence);
            }
        }

        if (!found)
        {
            TestSession::WriteInfo(TraceSource, "Service {0} did not find FM report", service->EntityId);
            return false;
        }

        if (!healthTable_->Verify(service, events, isInvalidated))
        {
            return false;
        }
    }

    if (fmServices.size() > 0)
    {
        TestSession::WriteInfo(TraceSource, "{0} services on FM not found in HM: {1}", fmServices.size(), *fmServices.begin());
        return false;
    }

    return true;
}

bool TestFabricClientHealth::VerifyApplicationHealthReports(HealthManagerReplicaSPtr const & hm, vector<ServiceSnapshot> const & services)
{
    map<wstring, ApplicationSnapshot> fmApplications;
    for (auto it = services.begin(); it != services.end(); ++it)
    {
        if (it->ServiceDescription.ApplicationName.size() > 0 && !it->IsDeleted && it->ServiceDescription.Name != Reliability::Constants::TombstoneServiceName)
        {
            auto app = it->GetServiceType().GetApplication();
            fmApplications.insert(make_pair(it->ServiceDescription.ApplicationName, app));
        }
    }

    vector<ApplicationEntitySPtr> hmApplications;
    if (!healthTable_->Update(hm, hmApplications))
    {
        return false;
    }

    for (ApplicationEntitySPtr const & application : hmApplications)
    {
        HealthEntityState::Enum entityState;
        shared_ptr<AttributesStoreData> entityBase;
        vector<HealthEvent> events;
        if (!application->Test_GetData(entityState, entityBase, events))
        {
            TestSession::WriteInfo(TraceSource, "HM get application failed: {0}, entity state {1}", application->EntityId, entityState);
            return false;
        }

        auto entity = static_cast<ApplicationAttributesStoreData*>(entityBase.get());
        if (!application->IsAdHocApp() && !application->IsSystemApp)
        {
            auto it = fmApplications.find(application->EntityId.ApplicationName);
            if (it == fmApplications.end())
            {
                auto hmAppServices = application->GetServices();
                if (hmAppServices.empty())
                {
                    // If an application has no services, it will not appear in FM view, skip validation here
                    TestSession::WriteNoise(TraceSource, "Application {0} with 0 services in HM not found in FM", *entity);
                }
                else
                {
                    // Check whether the services are deleted. The CleanupGraceInterval is by default 1 hour, so the entities may not have been cleaned up yet
                    int upChildren = 0;

                    wstring details;
                    StringWriter writer(details);
                    writer.WriteLine("{0} total services, up:", hmAppServices.size());
                    for (auto hmAppServicesIt = hmAppServices.begin(); hmAppServicesIt != hmAppServices.end(); ++hmAppServicesIt)
                    {
                        auto attrib = (*hmAppServicesIt)->GetAttributesCopy();
                        if (!attrib->IsCleanedUp && !attrib->IsMarkedForDeletion)
                        {
                            ++upChildren;

                            wstring partitions;
                            StringWriter partitionsDetail(partitions);
                            auto hmPartitions = (*hmAppServicesIt)->GetPartitions();
                            for (auto itPartitions = hmPartitions.begin(); itPartitions != hmPartitions.end(); ++itPartitions)
                            {
                                partitionsDetail.Write("{0};", (*itPartitions)->EntityIdString);
                            }

                            writer.WriteLine("{0}, {1}, partitions: {2}", *attrib, (*hmAppServicesIt)->State.State, partitions);
                        }
                    }

                    if (upChildren > 0)
                    {
                        TestSession::WriteInfo(TraceSource, "Application {0} in HM not found in FM. {1}", *entity, details);
                        return false;
                    }
                }

                // If the FM application is not found, the CM report may or may not be received on HM.
                // Do not do any validation in this case.
                continue;
            }

            bool found = false;
            for (auto itEvents = events.begin(); itEvents != events.end(); ++itEvents)
            {
                if (itEvents->SourceId == L"System.CM")
                {
                    found = true;

                    TestSession::FailTestIf(
                        itEvents->Property != *ServiceModel::Constants::HealthStateProperty ||
                        itEvents->TimeToLive != TimeSpan::MaxValue,
                        "Unexpected health event: {0}", *it);
                }
            }

            if (!found)
            {
                TestSession::WriteInfo(TraceSource, "Application {0} did not find CM report", application->EntityIdString);
                return false;
            }

            if (entity->AttributeSetFlags.IsApplicationTypeNameSet())
            {
                if (entity->ApplicationTypeName != it->second.AppId.ApplicationTypeName)
                {
                    TestSession::WriteInfo(TraceSource, "Application {0}: HM app type name {1} doesn't match FM {2}", application->EntityIdString, entity->ApplicationTypeName, it->second.AppId.ApplicationTypeName);
                    return false;
                }
            }
            else
            {
                TestSession::WriteInfo(TraceSource, "Application {0}: app type name not set", application->EntityIdString);
                return false;
            }

            fmApplications.erase(it);
        }

        if (!healthTable_->Verify(application, events))
        {
            return false;
        }
    }

    if (fmApplications.size() > 0)
    {
        TestSession::WriteInfo(TraceSource, "{0} applications on FM not found in HM: {1}", fmApplications.size(), fmApplications.begin()->first);
        return false;
    }

    return true;
}

bool TestFabricClientHealth::VerifyDeployedServicePackageAndApplicationHealthReports(HealthManagerReplicaSPtr const & hm, map<DeployedServicePackageHealthId, FailoverUnitId> && packages)
{
    vector<DeployedServicePackageEntitySPtr> hmDeployedServicePackages;
    if (!healthTable_->Update(hm, hmDeployedServicePackages))
    {
        return false;
    }

    TestSession::WriteInfo(TraceSource, "DeployedServicePckageCount - HM={0}, RA={1}.", hmDeployedServicePackages.size(), packages.size());

    map<DeployedApplicationHealthId, FABRIC_NODE_INSTANCE_ID> applications;

    for (DeployedServicePackageEntitySPtr const & package : hmDeployedServicePackages)
    {
        HealthEntityState::Enum entityState;
        shared_ptr<AttributesStoreData> entityBase;
        vector<HealthEvent> events;
        if (!package->Test_GetData(entityState, entityBase, events))
        {
            TestSession::WriteInfo(TraceSource, "HM get deployed service package failed: {0}, entity state {1}", package->EntityId, entityState);
            return false;
        }

        auto entity = static_cast<DeployedServicePackageAttributesStoreData*>(entityBase.get());
        auto it = packages.find(package->EntityId);
        if (it == packages.end())
        {
            TestSession::WriteInfo(TraceSource, "Deployed service package {0} in HM not found on RA", *entity);
            return false;
        }
        else
        {
            bool found = false;
            for (auto const & event : events)
            {
                if (event.SourceId == L"System.Hosting")
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                TestSession::WriteInfo(TraceSource, "Deployed service package {0} did not find Hosting report", *entity);
                return false;
            }

            packages.erase(it);
        }

        if (!healthTable_->Verify(package, events))
        {
            return false;
        }

        DeployedApplicationHealthId appId(package->EntityId.ApplicationName, package->EntityId.NodeId);
        applications[appId] = entity->NodeInstanceId;
    }

    if (packages.size() > 0)
    {
        TestSession::WriteInfo(TraceSource, "{0} deployed service packages on RA not found in HM: {1} with Failover unit {2}", packages.size(), packages.begin()->first, packages.begin()->second);
        return false;
    }

    vector<DeployedApplicationEntitySPtr> hmDeployedApplications;
    if (!healthTable_->Update(hm, hmDeployedApplications))
    {
        return false;
    }

    for (DeployedApplicationEntitySPtr const & application : hmDeployedApplications)
    {
        HealthEntityState::Enum entityState;
        shared_ptr<AttributesStoreData> entityBase;
        vector<HealthEvent> events;
        if (!application->Test_GetData(entityState, entityBase, events))
        {
            TestSession::WriteInfo(TraceSource, "HM get deployed application failed: {0}, entity state {1}", application->EntityId, entityState);
            return false;
        }

        auto entity = static_cast<DeployedApplicationAttributesStoreData*>(entityBase.get());
        auto it = applications.find(application->EntityId);
        if (it == applications.end())
        {
            TestSession::WriteInfo(TraceSource, "Deployed application {0} in HM not found in FM", *entity);
            return false;
        }
        else
        {
            if (entity->NodeInstanceId != it->second)
            {
                TestSession::WriteInfo(TraceSource, "Deployed application {0} in HM node instance does not match FM {1}",
                    *entity, it->second);
                return false;
            }

            bool found = false;
            for (auto const & event : events)
            {
                if (event.SourceId == L"System.Hosting")
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                TestSession::WriteInfo(TraceSource, "Deployed application {0} did not find Hosting report", *entity);
                return false;
            }

            applications.erase(it);
        }

        if (!healthTable_->Verify(application, events))
        {
            return false;
        }
    }

    return true;
}
