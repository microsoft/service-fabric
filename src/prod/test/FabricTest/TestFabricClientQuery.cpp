// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace ServiceModel;
using namespace Query;
using namespace Naming;
using namespace TestCommon;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ReliabilityTestApi;
using namespace ReliabilityTestApi::FailoverManagerComponentTestApi;
using namespace Api;

const StringLiteral TraceSource("FabricTest.FabricClientQuery");

wstring const TestFabricClientQuery::NodeName = L"NodeName";
wstring const TestFabricClientQuery::NodeId = L"NodeId";
wstring const TestFabricClientQuery::NodeInstanceId = L"NodeInstanceId";
wstring const TestFabricClientQuery::IpAddressOrFQFN = L"IpAddressOrFQDN";
wstring const TestFabricClientQuery::NodeType = L"NodeType";
wstring const TestFabricClientQuery::CodeVersion = L"CodeVersion";
wstring const TestFabricClientQuery::ConfigVersion = L"ConfigVersion";
wstring const TestFabricClientQuery::NodeStatus = L"NodeStatus";
wstring const TestFabricClientQuery::IsSeedNode = L"IsSeedNode";
wstring const TestFabricClientQuery::NextUpgradeDomain = L"NextUpgradeDomain";
wstring const TestFabricClientQuery::UpgradeDomain = L"UpgradeDomain";
wstring const TestFabricClientQuery::FaultDomain = L"FaultDomain";
wstring const TestFabricClientQuery::DeploymentName = L"DeploymentName";
wstring const TestFabricClientQuery::ApplicationDefinitionKind = L"ApplicationDefinitionKind";
wstring const TestFabricClientQuery::ApplicationTypeName = L"ApplicationTypeName";
wstring const TestFabricClientQuery::ApplicationTypeVersion = L"ApplicationTypeVersion";
wstring const TestFabricClientQuery::TargetApplicationTypeVersion = L"TargetApplicationTypeVersion";
wstring const TestFabricClientQuery::ApplicationName = L"ApplicationName";
wstring const TestFabricClientQuery::ApplicationStatus = L"ApplicationStatus";
wstring const TestFabricClientQuery::ApplicationParameters = L"ApplicationParameters";
wstring const TestFabricClientQuery::DefaultParameters = L"DefaultParameters";
wstring const TestFabricClientQuery::ApplicationTypeDefinitionKind = L"ApplicationTypeDefinitionKind";
wstring const TestFabricClientQuery::GetApplicationTypeList = L"GetApplicationTypeList";
wstring const TestFabricClientQuery::GetApplicationTypePagedList = L"GetApplicationTypePagedList";
wstring const TestFabricClientQuery::Status = L"Status";
wstring const TestFabricClientQuery::StatusDetails = L"StatusDetails";
wstring const TestFabricClientQuery::Unprovisioning = L"Unprovisioning";
wstring const TestFabricClientQuery::ServiceName = L"ServiceName";
wstring const TestFabricClientQuery::ServiceManifestName = L"ServiceManifestName";
wstring const TestFabricClientQuery::ServiceManifestVersion = L"ServiceManifestVersion";
wstring const TestFabricClientQuery::ServiceStatus = L"ServiceStatus";
wstring const TestFabricClientQuery::ServiceType = L"Type";
wstring const TestFabricClientQuery::ServiceTypeName = L"ServiceTypeName";
wstring const TestFabricClientQuery::CodePackageName = L"CodePackageName";
wstring const TestFabricClientQuery::CodePackageVersion = L"CodePackageVersion";
wstring const TestFabricClientQuery::HasPersistedState = L"HasPersistedState";
wstring const TestFabricClientQuery::PartitionId = L"PartitionId";
wstring const TestFabricClientQuery::PartitionKind = L"PartitionKind";
wstring const TestFabricClientQuery::PartitionName = L"PartitionName";
wstring const TestFabricClientQuery::PartitionLowKey = L"PartitionLowKey";
wstring const TestFabricClientQuery::PartitionHighKey = L"PartitionHighKey";
wstring const TestFabricClientQuery::TargetReplicaSetSize = L"TargetReplicaSetSize";
wstring const TestFabricClientQuery::MinReplicaSetSize = L"MinReplicaSetSize";
wstring const TestFabricClientQuery::PartitionStatus = L"PartitionStatus";
wstring const TestFabricClientQuery::InstanceCount = L"InstanceCount";
wstring const TestFabricClientQuery::ReplicaId = L"ReplicaId";
wstring const TestFabricClientQuery::ReplicaRole = L"ReplicaRole";
wstring const TestFabricClientQuery::ReplicaStatus = L"ReplicaStatus";
wstring const TestFabricClientQuery::ReplicaState = L"ReplicaState";
wstring const TestFabricClientQuery::ReplicaAddress = L"ReplicaAddress";
wstring const TestFabricClientQuery::InstanceId = L"InstanceId";
wstring const TestFabricClientQuery::InstanceStatus = L"InstanceStatus";
wstring const TestFabricClientQuery::InstanceState = L"InstanceState";
wstring const TestFabricClientQuery::UseImplicitHost = L"UseImplicitHost";
wstring const TestFabricClientQuery::DeploymentStatus = L"DeploymentStatus";

wstring const TestFabricClientQuery::EntryPointStatus = L"EntryPointStatus";
wstring const TestFabricClientQuery::RunFrequencyInterval = L"RunFrequencyInterval";
wstring const TestFabricClientQuery::HealthState = L"HealthState";
wstring const TestFabricClientQuery::ContinuationToken = L"ContinuationToken";
wstring const TestFabricClientQuery::ActivationId = L"ActivationId";
wstring const TestFabricClientQuery::SkipValidationToken = L"SkipValidationToken";

wstring const TestFabricClientQuery::NodeStatusFilterDefaultString = make_global<wstring>(L"default");
wstring const TestFabricClientQuery::NodeStatusFilterAllString = make_global<wstring>(L"all");
wstring const TestFabricClientQuery::NodeStatusFilterUpString = make_global<wstring>(L"up");
wstring const TestFabricClientQuery::NodeStatusFilterDownString = make_global<wstring>(L"down");
wstring const TestFabricClientQuery::NodeStatusFilterEnablingString = make_global<wstring>(L"enabling");
wstring const TestFabricClientQuery::NodeStatusFilterDisablingString = make_global<wstring>(L"disabling");
wstring const TestFabricClientQuery::NodeStatusFilterDisabledString = make_global<wstring>(L"disabled");
wstring const TestFabricClientQuery::NodeStatusFilterUnknownString = make_global<wstring>(L"unknown");
wstring const TestFabricClientQuery::NodeStatusFilterRemovedString = make_global<wstring>(L"removed");

wstring const TestFabricClientQuery::NodeStatusFilterDefault = make_global<wstring>(L"0"); // value of FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT
wstring const TestFabricClientQuery::NodeStatusFilterAll = make_global<wstring>(L"0xFFFF");
wstring const TestFabricClientQuery::NodeStatusFilterUp = make_global<wstring>(L"0x0001");
wstring const TestFabricClientQuery::NodeStatusFilterDown = make_global<wstring>(L"0x0002");
wstring const TestFabricClientQuery::NodeStatusFilterEnabling = make_global<wstring>(L"0x0004");
wstring const TestFabricClientQuery::NodeStatusFilterDisabling = make_global<wstring>(L"0x0008");
wstring const TestFabricClientQuery::NodeStatusFilterDisabled = make_global<wstring>(L"0x0010");
wstring const TestFabricClientQuery::NodeStatusFilterUnknown = make_global<wstring>(L"0x0020");
wstring const TestFabricClientQuery::NodeStatusFilterRemoved = make_global<wstring>(L"0x0040");

bool TestFabricClientQuery::Query(Common::StringCollection const & params)
{
    auto queryClient = CreateFabricQueryClient(FABRICSESSION.FabricDispatcher.Federation);
    return Query(params, queryClient);
}

bool TestFabricClientQuery::Query(StringCollection const & params, Common::ComPointer<IFabricQueryClient> const & queryClient)
{
    ScopedHeap heap;
    bool isVerifyFromFMArg = false;
    wstring queryName;
    vector<FABRIC_STRING_PAIR> queryArgsVector;
    VerifyResultTable expectQueryResult;
    HRESULT expectedError = S_OK;
    FABRIC_STRING_PAIR_LIST queryArgs;
    size_t paramIndex = 0;
    bool isVerifyArg = false;
    bool ignoreResultVerification = false;
    bool expectEmpty = false;
    bool requiresEmptyValidation = false;
    queryName = params[paramIndex];

    while (++paramIndex < params.size())
    {
        if (params[paramIndex] == FabricTestCommands::VerifyCommand)
        {
            isVerifyArg = true;
            continue;
        }
        else if (params[paramIndex] == FabricTestCommands::VerifyFromFMCommand)
        {
            isVerifyFromFMArg = true;
            break;
        }

        if (isVerifyArg)
        {
            if (!ProcessVerifyArg(params[paramIndex], expectQueryResult))
            {
                return false;
            }
        }
        else
        {
            if (!ProcessQueryArg(queryName, params[paramIndex], queryArgsVector, heap, expectedError, expectEmpty, requiresEmptyValidation, ignoreResultVerification))
            {
                return false;
            }
        }
    }

    if (queryArgsVector.size() > 0)
    {
        queryArgs.Items = queryArgsVector.data();
    }
    queryArgs.Count = static_cast<ULONG>(queryArgsVector.size());

    ComPointer<IInternalFabricQueryClient2> internalQueryClient(queryClient, IID_IInternalFabricQueryClient2);

    // Execute query with retries
    int remainingRetries = FabricTestSessionConfig::GetConfig().QueryOperationRetryCount;
    bool success;
    do
    {
        --remainingRetries;

        success = true;
        ComPointer<IInternalFabricQueryResult2> fabricQuery;
        HRESULT hr = TestFabricClient::PerformFabricClientOperation(
            L"Query",
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {

            return internalQueryClient->BeginInternalQuery(
                queryName.c_str(),
                &queryArgs,
                timeout,
                callback.GetRawPointer(),
                context.InitializationAddress());
        },
            [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
        {
            return internalQueryClient->EndInternalQuery2(context.GetRawPointer(), fabricQuery.InitializationAddress());
        },
            expectedError,
            S_OK,
            S_OK,
            !ignoreResultVerification);

        if (ignoreResultVerification)
        {
            return true;
        }

        TestSession::FailTestIfNot(
            (SUCCEEDED(hr) || hr == expectedError),
            "Query failed with hr = {0}",
            hr);

        if (SUCCEEDED(hr))
        {
            StringUtility::ToLower(queryName);
            VerifyResultTable actualQueryResult;

            if (queryName == L"getdeployedservicereplicadetail")
            {
                actualQueryResult = ProcessGetDeployedServiceReplicaDetailQueryResult(fabricQuery->get_Result());
            }
            else if (queryName == L"getcomposedeploymentupgradeprogress")
            {
                actualQueryResult = ProcessGetComposeDeploymentUpgradeProgressResult(fabricQuery->get_Result());
            }
            else
            {
                actualQueryResult = ProcessQueryResult(fabricQuery->get_Result(), fabricQuery->get_PagingStatus());
                if (requiresEmptyValidation)
                {
                    if (!expectEmpty && actualQueryResult.empty())
                    {
                        TestSession::WriteInfo(TraceSource, "Query had no results.");
                        success = false;
                    }
                    else if (expectEmpty && !actualQueryResult.empty())
                    {
                        TestSession::WriteInfo(TraceSource, "Expected empty query results failed.");
                        success = false;
                    }
                }

                if (success && isVerifyFromFMArg)
                {
                    if (!ProcessVerifyFromFMArg(queryName, queryArgsVector, expectQueryResult))
                    {
                        success = false;
                    }
                }
            }

            if (success)
            {
                success = VerifyQueryResult(queryName, expectQueryResult, actualQueryResult, isVerifyFromFMArg);
            }

            if (success)
            {
                break;
            }

            // Check failed, retry if there are retries left
            if (remainingRetries > 0)
            {
                TestSession::WriteInfo(TraceSource, "Query failed, remaining retries {0}. Sleep {1}", remainingRetries, FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay);
                Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelay.TotalMilliseconds()));
            }
            else
            {
                TestSession::WriteInfo(TraceSource, "Query failed and there are no remaining retries");
                return false;
            }
        }
    } while (remainingRetries > 0);

    return true;
}

ComPointer<IFabricQueryClient> TestFabricClientQuery::CreateFabricQueryClient(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricQueryClient> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricQueryClient,
        (void **)result.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricQueryClient");

    return result;
}

Api::IQueryClientPtr TestFabricClientQuery::CreateInternalFabricQueryClient(__in FabricTestFederation & testFederation)
{
    Api::IQueryClientPtr internalQueryClient;
    auto factoryPtr = TestFabricClient::FabricCreateNativeClientFactory(testFederation);
    auto error = factoryPtr->CreateQueryClient(internalQueryClient);
    TestSession::FailTestIfNot(error.IsSuccess(), "InternalClientFactory does not support IQueryClient");
    return internalQueryClient;
}

VerifyResultTable TestFabricClientQuery::ProcessGetComposeDeploymentUpgradeProgressResult(FABRIC_QUERY_RESULT const * queryResult)
{
    TestSession::FailTestIf(
        (!queryResult || queryResult->Value == nullptr || queryResult->Kind != FABRIC_QUERY_RESULT_KIND_ITEM),
        "Query result null or invalid.");
    TestSession::WriteInfo(TraceSource, "Query succeeded.");

    FABRIC_QUERY_RESULT_ITEM* item = reinterpret_cast<FABRIC_QUERY_RESULT_ITEM*>(queryResult->Value);
    TestSession::FailTestIf(
        (item->Kind != FABRIC_QUERY_RESULT_ITEM_KIND_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS),
        "Received unknown item Kind {0}. Only compose deployment upgrade progress is supported.",
        static_cast<LONG>(item->Kind));

    FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS * result = reinterpret_cast<FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS*>(item->Item);

    VerifyResultTable resultTable;

    wstring deploymentName = result->DeploymentName;
    wstring applicationName = result->ApplicationName;
    wstring targetApplicationTypeVersion = result->TargetApplicationTypeVersion;
    wstring nextUpgradeDomain = result->NextUpgradeDomain == nullptr ? L"" : result->NextUpgradeDomain;
    wstring upgradeStatusDetails = result->UpgradeStatusDetails == nullptr ? L"" : result->UpgradeStatusDetails;

    VerifyResultRow row;
    row[DeploymentName] = deploymentName;
    row[ApplicationName] = applicationName;
    row[TargetApplicationTypeVersion] = targetApplicationTypeVersion;
    row[Status] = GetComposeDeploymentUpgradeState(result->UpgradeState);
    row[NextUpgradeDomain] = nextUpgradeDomain;
    row[StatusDetails] = upgradeStatusDetails;

    resultTable[deploymentName] = row;

    return resultTable;
}

VerifyResultTable TestFabricClientQuery::ProcessGetDeployedServiceReplicaDetailQueryResult(FABRIC_QUERY_RESULT const * queryResult)
{
    TestSession::FailTestIf(
        (!queryResult || queryResult->Value == NULL || queryResult->Kind != FABRIC_QUERY_RESULT_KIND_ITEM),
        "Query result null or invalid.");
    TestSession::WriteInfo(TraceSource, "Query succeeded.");

    FABRIC_QUERY_RESULT_ITEM* item = reinterpret_cast<FABRIC_QUERY_RESULT_ITEM*>(queryResult->Value);
    TestSession::FailTestIf(
        (item->Kind != FABRIC_QUERY_RESULT_ITEM_KIND_DEPLOYED_SERVICE_REPLICA_DETAIL),
        "Received unknown item Kind {0}. Only deployed service replica detail is supported.",
        static_cast<LONG>(item->Kind));

    FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM* detailresult = reinterpret_cast<FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM*>(item->Item);
    TestSession::FailTestIf(
        (detailresult->Kind != FABRIC_SERVICE_KIND_STATEFUL),
        "Received unknown service Kind {0}. Only stateful service is supported.",
        static_cast<LONG>(detailresult->Kind));

    VerifyResultTable resultTable;

    FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM* statefulresult = reinterpret_cast<FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM*>(detailresult->Value);

    TestSession::FailTestIf(
        (statefulresult->ReplicatorStatus == nullptr),
        " Replicator status is null");

    VerifyResultRow roleRow;

    // add all the items in the same row to re-use the code written for other verification
    wstring roleKey;
    if (statefulresult->ReplicatorStatus->Role == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        roleKey = L"primary";
            // Add more verification as needed
        FABRIC_PRIMARY_REPLICATOR_STATUS_QUERY_RESULT * primaryResult = reinterpret_cast<FABRIC_PRIMARY_REPLICATOR_STATUS_QUERY_RESULT*>(statefulresult->ReplicatorStatus->Value);

        TestSession::FailTestIf(primaryResult->ReplicationQueueStatus == nullptr, "Empty Primary Queue Status in replicator query");
        TestSession::FailTestIf(primaryResult->RemoteReplicators == nullptr, "Empty Remote Replicators in replicator query");
        TestSession::FailTestIf(primaryResult->ReplicationQueueStatus->QueueUtilizationPercentage > 100, "Primary QueueUtilizationPercentage cannot exceed 100 in replicator query");

        FABRIC_REMOTE_REPLICATOR_STATUS_LIST * remoteReplicatorStatusList = reinterpret_cast<FABRIC_REMOTE_REPLICATOR_STATUS_LIST*>(primaryResult->RemoteReplicators);

        for (size_t i = 0; i < remoteReplicatorStatusList->Count; i++)
        {
            auto remoteReplicator = static_cast<FABRIC_REMOTE_REPLICATOR_STATUS>(remoteReplicatorStatusList->Items[i]);

            TestSession::FailTestIf(remoteReplicator.LastReceivedReplicationSequenceNumber < 0, "LastReceivedReplicationSequenceNumber must be greater than zero");
            TestSession::FailTestIf(remoteReplicator.LastReceivedCopySequenceNumber < 0, "LastReceivedCopySequenceNumber must be greater than zero");

            TestSession::FailTestIf(remoteReplicator.Reserved == nullptr, "remoteReplicator.Reserved must not be null");
            auto remoteReplicatorAcknowledgementStatus = static_cast<FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_STATUS*>(remoteReplicator.Reserved);

            TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->ReplicationStreamAcknowledgementDetails->NotReceivedCount < 0, "NotReceivedReplicationCount must be greater than zero");
            TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->ReplicationStreamAcknowledgementDetails->ReceivedAndNotAppliedCount < 0, "ReceivedAndNotAppliedReplicationCount must be greater than zero");
            TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->ReplicationStreamAcknowledgementDetails->AverageApplyDurationMilliseconds < 0, "AverageReplicationApplyAckDurationTicks must be greater than zero");
            TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->ReplicationStreamAcknowledgementDetails->AverageReceiveDurationMilliseconds < 0, "AverageReplicationReceiveAckDurationTicks must be greater than zero");

            if (remoteReplicator.IsInBuild == TRUE)
            {
                TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->CopyStreamAcknowledgementDetails->NotReceivedCount < 0, "NotReceivedCopyCount must be greater than zero");
                TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->CopyStreamAcknowledgementDetails->ReceivedAndNotAppliedCount < 0, "ReceivedAndNotAppliedCopyCount must be greater than zero");
                TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->CopyStreamAcknowledgementDetails->AverageApplyDurationMilliseconds < 0, "AverageCopyApplyAckDurationTicks must be greater than zero");
                TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->CopyStreamAcknowledgementDetails->AverageReceiveDurationMilliseconds < 0, "AverageCopyReceiveAckDurationTicks must be greater than zero");
            }
        }
    }
    else if (statefulresult->ReplicatorStatus->Role == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
    {
        roleKey = L"secondary";
        FABRIC_SECONDARY_REPLICATOR_STATUS_QUERY_RESULT* secondaryResult = reinterpret_cast<FABRIC_SECONDARY_REPLICATOR_STATUS_QUERY_RESULT*>(statefulresult->ReplicatorStatus->Value);
        roleRow[L"firstrepl"] = wformatString("{0}", secondaryResult->ReplicationQueueStatus->FirstSequenceNumber);
        roleRow[L"lastrepl"] = wformatString("{0}", secondaryResult->ReplicationQueueStatus->LastSequenceNumber);
        // Add more verification as needed
    }
    else if (statefulresult->ReplicatorStatus->Role == FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
    {
        roleKey = L"idle";
        // Add more verification as needed
    }
    else
    {
         TestSession::FailTest("{0} role not supported", statefulresult->ReplicatorStatus->Role);
    }

    roleRow[L"role"] = roleKey;
    resultTable[roleKey] = roleRow;

    return resultTable;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResult(FABRIC_QUERY_RESULT const * queryResult, FABRIC_PAGING_STATUS const * queryPagingStatus)
{
    TestSession::FailTestIf(
        (!queryResult || queryResult->Value == NULL || queryResult->Kind == FABRIC_QUERY_RESULT_KIND_INVALID),
        "Query result null or invalid.");
    TestSession::WriteInfo(TraceSource, "Query succeeded.");

    TestSession::FailTestIf(
        (queryResult->Kind != FABRIC_QUERY_RESULT_KIND_LIST),
        "Received unknown result Kind {0}. Only List is supported.",
        static_cast<LONG>(queryResult->Kind));

    FABRIC_QUERY_RESULT_LIST* resultList = reinterpret_cast<FABRIC_QUERY_RESULT_LIST*>(queryResult->Value);
    TestSession::FailTestIf(
        ((resultList->Kind == FABRIC_QUERY_RESULT_ITEM_KIND_INVALID) || ((resultList->Items == NULL) && (resultList->Count != 0))),
        "Received invalid result list data.");

    if(resultList->Count == 0)
    {
        return VerifyResultTable();
    }

    VerifyResultTable resultTable;
    switch (resultList->Kind)
    {
    case FABRIC_QUERY_RESULT_ITEM_KIND_APPLICATION_TYPE:
        {
            FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM* applicationTypeResult = reinterpret_cast<FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM*>(resultList->Items);
            resultTable = ProcessQueryResultApplicationType(applicationTypeResult, resultList->Count);
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_NODE:
        {
            FABRIC_NODE_QUERY_RESULT_ITEM* nodeResult = reinterpret_cast<FABRIC_NODE_QUERY_RESULT_ITEM*>(resultList->Items);
            resultTable = ProcessQueryResultNode(nodeResult, resultList->Count);
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_APPLICATION:
        {
            FABRIC_APPLICATION_QUERY_RESULT_ITEM* applicationResult = reinterpret_cast<FABRIC_APPLICATION_QUERY_RESULT_ITEM*>(resultList->Items);
            resultTable = ProcessQueryResultApplication(applicationResult, resultList->Count);
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_SERVICE:
        {
            FABRIC_SERVICE_QUERY_RESULT_ITEM* serviceResult = reinterpret_cast<FABRIC_SERVICE_QUERY_RESULT_ITEM*>(resultList->Items);
            resultTable = ProcessQueryResultService(serviceResult, resultList->Count);
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_SERVICE_TYPE:
        {
            FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM* serviceTypeResult = reinterpret_cast<FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM*>(resultList->Items);
            resultTable = ProcessQueryResultServiceType(serviceTypeResult, resultList->Count);
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_SERVICE_PARTITION:
        {
            FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM* partitionResult = reinterpret_cast<FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM*>(resultList->Items);
            TestSession::FailTestIf(
                (resultList->Count == 0),
                "Empty service partition result set.");

            TestSession::FailTestIf(
                (partitionResult[0].Kind == FABRIC_SERVICE_KIND_INVALID),
                "Invalid service partition type.");

            if (partitionResult[0].Kind == FABRIC_SERVICE_KIND_STATEFUL)
            {
                resultTable = ProcessQueryResultStatefulPartition(partitionResult, resultList->Count);
            }
            else
            {
                resultTable = ProcessQueryResultStatelessPartition(partitionResult, resultList->Count);
            }
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_SERVICE_REPLICA:
        {
            FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM* replicaResult = reinterpret_cast<FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM*>(resultList->Items);
            TestSession::FailTestIf(
                (resultList->Count == 0),
                "Empty service replica result set.");

            TestSession::FailTestIf(
                (replicaResult[0].Kind == FABRIC_SERVICE_KIND_INVALID),
                "Invalid service replica type.");

            if (replicaResult[0].Kind == FABRIC_SERVICE_KIND_STATEFUL)
            {
                resultTable = ProcessQueryResultStatefulReplica(replicaResult, resultList->Count);
            }
            else
            {
                resultTable = ProcessQueryResultStatelessInstance(replicaResult, resultList->Count);
            }
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_QUERY_METADATA:
        {
            FABRIC_QUERY_METADATA_QUERY_RESULT_ITEM* queryMetadataResult = reinterpret_cast<FABRIC_QUERY_METADATA_QUERY_RESULT_ITEM*>(resultList->Items);
            return ProcessQueryResultQueryMetadata(queryMetadataResult, resultList->Count);
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_DEPLOYED_APPLICATION:
        {
            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM* deployedApplicationResult = reinterpret_cast<FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM*>(resultList->Items);
            resultTable = ProcessQueryResultDeployedApplication(deployedApplicationResult, resultList->Count);
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_DEPLOYED_SERVICE_MANIFEST:
        {
            FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM* deployedServiceManifestResult = reinterpret_cast<FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM*>(resultList->Items);
            resultTable = ProcessQueryResultDeployedServiceManifest(deployedServiceManifestResult, resultList->Count);
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_DEPLOYED_SERVICE_TYPE:
        {
            FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM* deployedServiceTypeResult = reinterpret_cast<FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM*>(resultList->Items);
            resultTable = ProcessQueryResultDeployedServiceType(deployedServiceTypeResult, resultList->Count);
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_DEPLOYED_CODE_PACKAGE:
        {
            FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM* deployedCodePackageResult = reinterpret_cast<FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM*>(resultList->Items);
            resultTable = ProcessQueryResultDeployedCodePackage(deployedCodePackageResult, resultList->Count);
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_DEPLOYED_SERVICE_REPLICA:
        {
            FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM* deployedServiceReplicaResult = reinterpret_cast<FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM*>(resultList->Items);
            resultTable = ProcessQueryResultDeployedServiceReplica(deployedServiceReplicaResult, resultList->Count);
        }
        break;
    case FABRIC_QUERY_RESULT_ITEM_KIND_COMPOSE_DEPLOYMENT_STATUS:
        {
            FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM* composeDeploymentResult = reinterpret_cast<FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM*>(resultList->Items);
            resultTable = ProcessQueryResultComposeDeployment(composeDeploymentResult, resultList->Count);
        }
        break;
    default:
        TestSession::FailTest("Invalid or unsupported QueryResultListItem");
    }

    wstring continuationToken;
    if (queryPagingStatus != NULL)
    {
        auto hr = StringUtility::LpcwstrToWstring(queryPagingStatus->ContinuationToken, true, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, continuationToken);
        TestSession::FailTestIf(FAILED(hr), "Error getting continuation token from query result");
        if (!continuationToken.empty())
        {
            TestSession::WriteInfo(TraceSource, "Received ContinuationToken \"{0}\"", continuationToken);
            VerifyResultRow actualResultRow;

            auto uniqueContinuationToken = L"CT:" + continuationToken;
            actualResultRow[ContinuationToken] = uniqueContinuationToken; // ContinuationToken is a static string defined in this file.

            // The primary key for things is fabric test is created from the result string. For example, if ContinuationToken=a, then primary key would be a.
            // Often, the primary key for the results would end up sharing a primary key with the continuation token.
            // We adjust this value so that the continuation value can be the same as a primary key for another value.
            // This is the case for GetApplicationList, for example, if maxResults=1, then there would be a continuation token which shares a primary key
            // with one of the returned results, because that primary key is generated from the name of the application, as is the continuation token.
            // DON'T WORRY about this in the .test files. This is internal only - please pass in the values as before.
            resultTable[move(uniqueContinuationToken)] = move(actualResultRow);
        }
    }

    return resultTable;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultQueryMetadata(FABRIC_QUERY_METADATA_QUERY_RESULT_ITEM const * queryMetadataResult, ULONG resultCount)
{
    TestSession::FailTestIf(
        (resultCount == 0),
        "Empty query metadata result set.");

    for (ULONG indexResult = 0 ; indexResult  < resultCount; ++indexResult)
    {
        wstring queryName = queryMetadataResult[indexResult].QueryName;
        TestSession::WriteInfo(TraceSource, "QueryName: {0}.", queryName);
        QuerySpecificationSPtr querySpecification;
        for (int i = QueryNames::FIRST_QUERY_MINUS_ONE + 1; i < QueryNames::LAST_QUERY_PLUS_ONE; ++i)
        {
            if (StringUtility::AreEqualCaseInsensitive(QueryNames::ToString(static_cast<QueryNames::Enum>(i)), queryName))
            {
                querySpecification = QuerySpecificationStore::Get().GetSpecification(static_cast<QueryNames::Enum>(i), QueryArgumentMap());
            }
        }

        TestSession::FailTestIf(
            (querySpecification == nullptr),
            "Query name not found.");

        TestSession::WriteInfo(TraceSource, "RequiredArguments:");
        for (ULONG i = 0; i < queryMetadataResult[indexResult].RequiredArguments->Count; ++i)
        {
            wstring requiredArgument = queryMetadataResult[indexResult].RequiredArguments->Items[i];
            auto findResult = find_if(
                querySpecification->SupportedArguments.begin(),
                querySpecification->SupportedArguments.end(),
                [&requiredArgument](QueryArgument const & argument) { return (StringUtility::AreEqualCaseInsensitive(argument.Name, requiredArgument) && argument.IsRequired); });

            TestSession::FailTestIf(
                (querySpecification->SupportedArguments.end() == findResult),
                "Argument {0} not found",
                requiredArgument);

            TestSession::WriteInfo(TraceSource, "{0}", requiredArgument);
        }

        TestSession::WriteInfo(TraceSource, "OptionalArguments:");
        for (ULONG i = 0; i < queryMetadataResult[indexResult].OptionalArguments->Count; ++i)
        {
            wstring optionalArgument = queryMetadataResult[indexResult].OptionalArguments->Items[i];
            auto findResult = find_if(
                querySpecification->SupportedArguments.begin(),
                querySpecification->SupportedArguments.end(),
                [&optionalArgument](QueryArgument const & argument) { return (StringUtility::AreEqualCaseInsensitive(argument.Name, optionalArgument) && !argument.IsRequired); });

            TestSession::FailTestIf(
                (querySpecification->SupportedArguments.end() == findResult),
                "Argument {0} not found",
                optionalArgument);

            TestSession::WriteInfo(TraceSource, "{0}", optionalArgument);
        }
    }

    return VerifyResultTable();
}

// This command is used when a query command is given the keyword "verify"
// For example: query GetApplicationTypeList verify ApplicationTypeName=FailType
bool TestFabricClientQuery::VerifyQueryResult(wstring const & queryName, VerifyResultTable const & expectQueryResult, VerifyResultTable const & actualQueryResult, bool verifyFromFM)
{
    if (expectQueryResult.empty())
    {
        return true;
    }

    // For the query GetApplicationTypes:
    // If querying for the status of an unprovisioning application, it's possible that the query result will be empty if
    // unprovisioning completes quickly. We don't want to fail the test if that is the case.
    // Therefore, if the actual results are empty, AND all expected results have the status "Unprovisioning", return without error.
    if (actualQueryResult.empty() &&
        ((StringUtility::AreEqualCaseInsensitive(queryName, GetApplicationTypeList) ||
        StringUtility::AreEqualCaseInsensitive(queryName, GetApplicationTypePagedList))))
    {
        // For all the expected results, check what the expected status is
        for (auto expectTableIt = expectQueryResult.begin(); expectTableIt != expectQueryResult.end(); ++expectTableIt)
        {
            // Find the "Status" pair
            auto const & params = expectTableIt->second;
            auto const & statusPair = params.find(Status);

            // If there is an expected status that is NOT Unprovisioning, then fail the test.
            // If Status isn't found, then don't do anything
            if (statusPair == params.end() || statusPair->second != Unprovisioning)
            {
                TestSession::WriteInfo(TraceSource, "Actual query result is empty and status not specified / not Unprovisioning");
                return false;
            }
        }

        return true;
    }
    else
    {
        // For all queries that are not GetApplicationTypes, fail if actual query results are empty.
        if (actualQueryResult.empty())
        {
            TestSession::WriteInfo(TraceSource, "Actual query result is empty");
            return false;
        }
    }

    // check that there are the same number of results in expected and in actual.
    if (verifyFromFM)
    {
        if (expectQueryResult.size() != actualQueryResult.size())
        {
            TestSession::WriteInfo(
                TraceSource,
                "Actual query result row number {0} does not equal expected row number {1}",
                actualQueryResult.size(),
                expectQueryResult.size());
            return false;
        }
    }

    for (auto expectTableIt = expectQueryResult.begin(); expectTableIt != expectQueryResult.end(); ++expectTableIt)
    {
        auto actualTableIt = actualQueryResult.find(expectTableIt->first);

        // If the query is GetApplicationType, and we don't find a corresponding query result in the actual returned results
        if ((StringUtility::AreEqualCaseInsensitive(expectTableIt->first, GetApplicationTypeList) ||
            StringUtility::AreEqualCaseInsensitive(expectTableIt->first, GetApplicationTypePagedList)) &&
            actualTableIt == actualQueryResult.end())
        {
            auto const & params = expectTableIt->second;
            auto const & statusPair = params.find(Status);

            // If we find the "Status" field in the expected results
            // and if the "Status" is "Unprovisioning"
            if (statusPair != params.end() && statusPair->second == Unprovisioning)
            {
                // If the expected result has status "Unprovisioning", don't fail this test
                // Query result will be empty if unprovisioning completes quickly
                continue;
            }
        }

        if (actualTableIt == actualQueryResult.end())
        {
            TestSession::WriteInfo(
                TraceSource,
                "Expected primary key \"{0}\" does not exist in the actual query result",
                expectTableIt->first);
            return false;
        }

        // Check that for each entry, we match all data in each entry
        // For example: For app type Type1, make sure that the type has the correct status
        for (auto expectRowIt = expectTableIt->second.begin(); expectRowIt != expectTableIt->second.end(); ++expectRowIt)
        {
            if (verifyFromFM)
            {
                if (expectTableIt->second.size() != actualTableIt->second.size())
                {
                    TestSession::WriteInfo(
                        TraceSource,
                        "Actual query result column number \"{0}\" is not equal to the expected column number \"{1}\"",
                        actualTableIt->second.size(),
                        expectTableIt->second.size());
                    return false;
                }
            }

            auto actualRowIt = actualTableIt->second.find(expectRowIt->first);
            if (actualRowIt == actualTableIt->second.end())
            {
                TestSession::WriteInfo(
                    TraceSource,
                    "Expected key \"{0}\" does not exist in the actual query result",
                    expectRowIt->first);
                return false;
            }

            wstring const & actualValue = actualRowIt->second;

            if(StringUtility::StartsWith<wstring>(expectRowIt->second, L"~"))
            {
                wstring expectedValue = expectRowIt->second.substr(1);
                if (!StringUtility::Contains<wstring>(actualValue, expectedValue))
                {
                    TestSession::WriteInfo(
                        TraceSource,
                        "Actual query result value {0} does not contain expected value {1} for key {2}",
                        actualValue,
                        expectedValue,
                        expectRowIt->first);
                    return false;
                }
            }
            else if (expectRowIt->second != SkipValidationToken)
            {
                // Have to way to ensure that values are empty
                // If the user inputs null, then we want to make sure that no value was returned
                auto expectedSecond = expectRowIt->second;

                if (expectRowIt->second == L"null")
                {
                    expectedSecond = wstring();
                }

                if (expectedSecond != actualValue)
                {
                    TestSession::WriteInfo(
                        TraceSource,
                        "Actual query result value: {0} does not match expected value: {1} for key: {2}",
                        actualValue,
                        expectedSecond,
                        expectRowIt->first);
                    return false;
                }
            }
        }
    }

    TestSession::WriteInfo(TraceSource, "Verify succeeded.");
    return true;
}

bool TestFabricClientQuery::ProcessQueryArg(wstring const & queryName, wstring const & param, vector<FABRIC_STRING_PAIR> & queryArgsVector, ScopedHeap & heap, HRESULT &expectedError, bool & expectEmpty, bool & requiresEmptyValidation, bool & ignoreResultVerification)
{
    StringCollection paramTokens;
    StringUtility::Split<wstring>(param, paramTokens, FabricTestDispatcher::KeyValueDelimiter);

    if (paramTokens.size() != 2)
    {
        TestSession::WriteError(
            TraceSource,
            "Incorrect query argument {0} key=value expected",
            param);
        return false;
    }

    // Expected error argument
    if (paramTokens[0] == L"error")
    {
        ErrorCodeValue::Enum error = FABRICSESSION.FabricDispatcher.ParseErrorCode(paramTokens[1]);
        expectedError = ErrorCode(error).ToHResult();
        return true;
    }

    if (paramTokens[0] == L"expectempty")
    {
        expectEmpty = StringUtility::AreEqualCaseInsensitive(paramTokens[1], L"true");
        requiresEmptyValidation = true;
        return true;
    }

    if (paramTokens[0] == L"ignoreresultverification")
    {
        ignoreResultVerification = StringUtility::AreEqualCaseInsensitive(paramTokens[1], L"true");
        return true;
    }

    if (queryName.compare(L"getnodelist") == 0)
    {
        UpdateParamTokensForGetNodesQuery(paramTokens);
    }

    FABRIC_STRING_PAIR queryArg;
    queryArg.Name = heap.AddString(paramTokens[0]);
    queryArg.Value = heap.AddString(paramTokens[1]);
    queryArgsVector.push_back(queryArg);
    return true;
}

void TestFabricClientQuery::UpdateParamTokensForGetNodesQuery(StringCollection & paramTokens)
{
    if (paramTokens[0] == L"NodeStatusFilter")
    {
        wstring nodeStatusFilterString = paramTokens[1];
        if (nodeStatusFilterString.compare(NodeStatusFilterAllString) == 0)
        {
            paramTokens[1] = NodeStatusFilterAll;
        }
        else if (nodeStatusFilterString.compare(NodeStatusFilterUpString) == 0)
        {
            paramTokens[1] = NodeStatusFilterUp;
        }
        else if (nodeStatusFilterString.compare(NodeStatusFilterDownString) == 0)
        {
            paramTokens[1] = NodeStatusFilterDown;
        }
        else if (nodeStatusFilterString.compare(NodeStatusFilterEnablingString) == 0)
        {
            paramTokens[1] = NodeStatusFilterEnabling;
        }
        else if (nodeStatusFilterString.compare(NodeStatusFilterDisablingString) == 0)
        {
            paramTokens[1] = NodeStatusFilterDisabling;
        }
        else if (nodeStatusFilterString.compare(NodeStatusFilterDisabledString) == 0)
        {
            paramTokens[1] = NodeStatusFilterDisabled;
        }
        else if (nodeStatusFilterString.compare(NodeStatusFilterUnknownString) == 0)
        {
            paramTokens[1] = NodeStatusFilterUnknown;
        }
        else if (nodeStatusFilterString.compare(NodeStatusFilterRemovedString) == 0)
        {
            paramTokens[1] = NodeStatusFilterRemoved;
        }
        else if (nodeStatusFilterString.compare(NodeStatusFilterDefaultString) == 0)
        {
            paramTokens[1] = NodeStatusFilterDefault;
        }
    }
}

bool TestFabricClientQuery::ProcessVerifyArg(wstring const & param, VerifyResultTable & expectQueryResult)
{
    StringCollection paramTokens;
    StringUtility::Split<wstring>(param, paramTokens, FabricTestDispatcher::ItemDelimiter);

    if (paramTokens.size() < 1)
    {
        TestSession::WriteError(
            TraceSource,
            "Incorrect verify argument {0} key=value,key=value,... expected",
            param);
        return false;
    }

    size_t paramIndex = 0;
    VerifyResultRow keyValues;
    wstring primaryKey = L"";

    // Gets expected results.
    while (paramIndex < paramTokens.size())
    {
        StringCollection keyValuePair;
        bool contains = false;
        StringUtility::Split<wstring>(paramTokens[paramIndex], keyValuePair, FabricTestDispatcher::KeyValueDelimiter);
        if (keyValuePair.size() != 2)
        {
            keyValuePair.clear();
            StringUtility::Split<wstring>(paramTokens[paramIndex], keyValuePair, L"~");
            if (keyValuePair.size() != 2)
            {
                TestSession::WriteError(
                    TraceSource,
                    "Incorrect verify argument {0} key(=/~)value expected",
                    paramTokens[paramIndex]);
                return false;
            }

            contains = true;
        }

        if (keyValues.find(keyValuePair[0]) != keyValues.end())
        {
            TestSession::WriteError(
                TraceSource,
                "Incorrect verify argument key {0} already existed",
                keyValuePair[0]);
            return false;
        }

        wstring key = keyValuePair[0];
        wstring value = (keyValuePair[1] == L"-empty-" ? L"" : keyValuePair[1]);

        if (key == ContinuationToken) // ContinuationToken defined at the top of this file.
        {
            // We adjust this value so that the continuation value can be the same as a primary key for another value
            // For example, if maxResults=1, then there is a good chance the continuation token value will clash with the application name value
            // This is the case for GetApplicationList, for example
            value = (L"CT:" + value);
        }
        
        if(contains)
        {
            value = L"~" + keyValuePair[1];
        }

        keyValues[key] = value;
        paramIndex++;

        if (primaryKey == L"")
        {
            primaryKey = value;
        }
    }

    if (primaryKey != L"")
    {
        if (expectQueryResult.find(primaryKey) != expectQueryResult.end())
        {
            TestSession::WriteError(
                TraceSource,
                "Incorrect verify argument primary key {0} already existed",
                primaryKey);
            return false;
        }
        expectQueryResult[primaryKey] = keyValues;
    }
    return true;
}

bool TestFabricClientQuery::ProcessVerifyFromFMArg(wstring const &queryName, vector<FABRIC_STRING_PAIR> & queryArgsVector, VerifyResultTable & expectQueryResult)
{
    expectQueryResult.clear();
    if (StringUtility::AreEqualCaseInsensitive(queryName, QueryNames::ToString(QueryNames::GetNodeList)))
    {
        auto fm = FABRICSESSION.FabricDispatcher.GetFM();
        auto nodes = fm->SnapshotNodeCache();

        for(auto nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
        {
            VerifyResultRow expectResultRow;

            expectResultRow[NodeName] = wformatString(nodeIt->NodeName);
            expectResultRow[IpAddressOrFQFN] = wformatString(nodeIt->IpAddressOrFQDN);
            expectResultRow[NodeType] = wformatString(nodeIt->NodeType);
            expectResultRow[CodeVersion] = wformatString(nodeIt->VersionInstance.Version.CodeVersion);
            expectResultRow[ConfigVersion] = wformatString(nodeIt->VersionInstance.Version.ConfigVersion);
            expectResultRow[NodeStatus] = wformatString(nodeIt->NodeStatus);
            expectResultRow[IsSeedNode] = wformatString(FABRICSESSION.FabricDispatcher.IsSeedNode(nodeIt->Id));
            expectResultRow[UpgradeDomain] = wformatString(nodeIt->ActualUpgradeDomainId);
            expectResultRow[FaultDomain] = wformatString(nodeIt->FaultDomainId);

            // Special value so validation skips this
            expectResultRow[HealthState] = SkipValidationToken;

            expectQueryResult[expectResultRow[NodeName]] = expectResultRow;
        }
        return true;
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryName, QueryNames::ToString(QueryNames::GetServicePartitionList)))
    {
        wstring desiredServiceName = L"";
        wstring desiredPartitionId = L"";
        for (size_t i = 0; i < queryArgsVector.size(); ++i)
        {
            if (queryArgsVector[i].Name == QueryResourceProperties::Service::ServiceName)
            {
                desiredServiceName = queryArgsVector[i].Value;
            }
            if (queryArgsVector[i].Name == QueryResourceProperties::Partition::PartitionId)
            {
                desiredPartitionId = queryArgsVector[i].Value;
            }
        }

        if (desiredServiceName.empty() && desiredPartitionId.empty())
        {
            TestSession::WriteError(
                TraceSource,
                "Empty service name and partitionId for GetServicePartitionList query");
            return false;
        }

        if (!desiredPartitionId.empty() && desiredServiceName.empty())
        {
            FailoverUnitId desiredFailoverUnitId = FailoverUnitId(Common::Guid(desiredPartitionId));
            auto fm = FABRICSESSION.FabricDispatcher.GetFM();
            auto failoverUnitPtr = fm->FindFTByPartitionId(desiredFailoverUnitId);

            auto const & consistencyUnitDescription = failoverUnitPtr->FailoverUnitDescription.ConsistencyUnitDescription;

            VerifyResultRow expectResultRow;
            if (consistencyUnitDescription.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
            {
                expectResultRow[PartitionId] = wformatString(consistencyUnitDescription.ConsistencyUnitId.Guid);
                expectResultRow[PartitionKind] = wformatString(FabricTestDispatcher::PartitionKindSingleton);
            }
            else if (consistencyUnitDescription.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE)
            {
                expectResultRow[PartitionId] = wformatString(consistencyUnitDescription.ConsistencyUnitId.Guid);
                expectResultRow[PartitionKind] = wformatString(FabricTestDispatcher::PartitionKindInt64Range);
                expectResultRow[PartitionLowKey] = wformatString(static_cast<int64>(consistencyUnitDescription.LowKey));
                expectResultRow[PartitionHighKey] = wformatString(static_cast<int64>(consistencyUnitDescription.HighKey));
            }
            else if (consistencyUnitDescription.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_NAMED)
            {
                expectResultRow[PartitionId] = wformatString(consistencyUnitDescription.ConsistencyUnitId.Guid);
                expectResultRow[PartitionKind] = wformatString(FabricTestDispatcher::PartitionKindName);
                expectResultRow[PartitionName] = wformatString(consistencyUnitDescription.PartitionName);
            }
            else
            {
                Assert::CodingError("Invalid Partition Kind");
            }

            if (failoverUnitPtr->IsStateful)
            {
                expectResultRow[TargetReplicaSetSize] = wformatString(failoverUnitPtr->TargetReplicaSetSize);
                expectResultRow[MinReplicaSetSize] = wformatString(failoverUnitPtr->MinReplicaSetSize);
                expectResultRow[PartitionStatus] = wformatString(failoverUnitPtr->PartitionStatus);
            }
            else
            {
                expectResultRow[InstanceCount] = wformatString(failoverUnitPtr->TargetReplicaSetSize);
            }

            // Special value so validation skips this
            expectResultRow[HealthState] = SkipValidationToken;

            expectQueryResult[expectResultRow[PartitionId]] = expectResultRow;

            return true;
        }
        else
        {

            auto fm = FABRICSESSION.FabricDispatcher.GetFM();
            auto gfum = fm->SnapshotGfum();
            for (auto failoverUnit = gfum.begin(); failoverUnit != gfum.end(); ++failoverUnit)
            {
                if (failoverUnit->ServiceName == desiredServiceName && !failoverUnit->IsOrphaned)
                {
                    auto const & consistencyUnitDescription = failoverUnit->FailoverUnitDescription.ConsistencyUnitDescription;

                    VerifyResultRow expectResultRow;
                    if (consistencyUnitDescription.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
                    {
                        expectResultRow[PartitionId] = wformatString(consistencyUnitDescription.ConsistencyUnitId.Guid);
                        expectResultRow[PartitionKind] = wformatString(FabricTestDispatcher::PartitionKindSingleton);
                    }
                    else if (consistencyUnitDescription.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE)
                    {
                        expectResultRow[PartitionId] = wformatString(consistencyUnitDescription.ConsistencyUnitId.Guid);
                        expectResultRow[PartitionKind] = wformatString(FabricTestDispatcher::PartitionKindInt64Range);
                        expectResultRow[PartitionLowKey] = wformatString(static_cast<int64>(consistencyUnitDescription.LowKey));
                        expectResultRow[PartitionHighKey] = wformatString(static_cast<int64>(consistencyUnitDescription.HighKey));
                    }
                    else if (consistencyUnitDescription.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_NAMED)
                    {
                        expectResultRow[PartitionId] = wformatString(consistencyUnitDescription.ConsistencyUnitId.Guid);
                        expectResultRow[PartitionKind] = wformatString(FabricTestDispatcher::PartitionKindName);
                        expectResultRow[PartitionName] = wformatString(consistencyUnitDescription.PartitionName);
                    }
                    else
                    {
                        Assert::CodingError("Invalid Partition Kind");
                    }

                    if (failoverUnit->IsStateful)
                    {
                        expectResultRow[TargetReplicaSetSize] = wformatString(failoverUnit->TargetReplicaSetSize);
                        expectResultRow[MinReplicaSetSize] = wformatString(failoverUnit->MinReplicaSetSize);
                        expectResultRow[PartitionStatus] = wformatString(failoverUnit->PartitionStatus);
                    }
                    else
                    {
                        expectResultRow[InstanceCount] = wformatString(failoverUnit->TargetReplicaSetSize);
                    }

                    // Special value so validation skips this
                    expectResultRow[HealthState] = SkipValidationToken;

                    expectQueryResult[expectResultRow[PartitionId]] = expectResultRow;
                }
            }

            return true;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryName, QueryNames::ToString(QueryNames::GetServicePartitionReplicaList)))
    {
        wstring desiredPartitionId = L"";
        for (size_t i = 0; i < queryArgsVector.size(); ++i)
        {
            if (queryArgsVector[i].Name == QueryResourceProperties::Partition::PartitionId)
            {
                desiredPartitionId = queryArgsVector[i].Value;
                break;
            }
        }

        if (desiredPartitionId.empty())
        {
            TestSession::WriteError(
                TraceSource,
                "Empty partition id for GetServicePartitionReplicaList query");
            return false;
        }

        FailoverUnitId desiredFailoverUnitId = FailoverUnitId(Common::Guid(desiredPartitionId));
        auto fm = FABRICSESSION.FabricDispatcher.GetFM();
        auto failoverUnitPtr = fm->FindFTByPartitionId(desiredFailoverUnitId);

        auto replicas = failoverUnitPtr->GetReplicas();
        for (auto replica = replicas.begin(); replica != replicas.end(); ++replica)
        {
            VerifyResultRow expectResultRow;
            if (failoverUnitPtr->IsStateful)
            {
                expectResultRow[ReplicaId] = wformatString(replica->ReplicaId);
                expectResultRow[ReplicaRole] = wformatString(FABRICSESSION.FabricDispatcher.GetReplicaRoleString(ToReplicaRole(ReplicaRole::ConvertToPublicReplicaRole(replica->CurrentConfigurationRole))));
                expectResultRow[ReplicaStatus] = wformatString(FABRICSESSION.FabricDispatcher.GetReplicaStatusString(ToReplicaStatus(ReplicaStatus::ConvertToPublicReplicaStatus(replica->ReplicaStatus))));
                expectResultRow[ReplicaAddress] = wformatString(replica->ServiceLocation);
                expectResultRow[NodeName] = wformatString(replica->GetNode().NodeName);
                expectResultRow[HealthState] = SkipValidationToken;

                expectQueryResult[expectResultRow[ReplicaId]] = expectResultRow;
            }
            else
            {
                expectResultRow[InstanceId] = wformatString(replica->InstanceId);
                expectResultRow[InstanceStatus] = wformatString(FABRICSESSION.FabricDispatcher.GetReplicaStatusString(ToReplicaStatus(ReplicaStatus::ConvertToPublicReplicaStatus(replica->ReplicaStatus))));
                expectResultRow[ReplicaAddress] = wformatString(replica->ServiceLocation);
                expectResultRow[NodeName] = wformatString(replica->GetNode().NodeName);
                expectResultRow[HealthState] = SkipValidationToken;

                expectQueryResult[expectResultRow[InstanceId]] = expectResultRow;
            }
        }

        return true;
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryName, QueryNames::ToString(QueryNames::GetApplicationServiceList)))
    {
        wstring desiredApplicationName = L"";
        for (size_t i = 0; i < queryArgsVector.size(); ++i)
        {
            if (queryArgsVector[i].Name == QueryResourceProperties::Application::ApplicationName)
            {
                desiredApplicationName = queryArgsVector[i].Value;
                break;
            }
        }

        auto services = FABRICSESSION.FabricDispatcher.GetFM()->SnapshotServiceCache();

        if (desiredApplicationName.empty())
        {
            for(auto serviceInfo = services.begin(); serviceInfo != services.end(); ++serviceInfo)
            {
                auto serviceType = serviceInfo->GetServiceType();
                if (serviceType.Type.IsAdhocServiceType())
                {
                    VerifyResultRow expectResultRow;
                    expectResultRow[ServiceName] = wformatString(serviceInfo->Name);
                    expectResultRow[ServiceStatus] = GetServiceStatusString(serviceInfo->Status);
                    if (serviceInfo->ServiceDescription.IsStateful)
                    {
                        expectResultRow[ServiceType] = wformatString(FabricTestDispatcher::ServiceTypeStateful);
                        expectResultRow[ServiceTypeName] = wformatString(serviceType.Type.ServiceTypeName);
                        expectResultRow[ServiceManifestVersion] = wformatString(serviceInfo->ServiceDescription.PackageVersionInstance.Version);
                        expectResultRow[HasPersistedState] = wformatString(serviceInfo->ServiceDescription.HasPersistedState);
                    }
                    else
                    {
                        expectResultRow[ServiceType] = wformatString(FabricTestDispatcher::ServiceTypeStateless);
                        expectResultRow[ServiceTypeName] = wformatString(serviceType.Type.ServiceTypeName);
                        expectResultRow[ServiceManifestVersion] = wformatString(serviceInfo->ServiceDescription.PackageVersionInstance.Version);
                    }

                    // Special value so validation skips this
                    expectResultRow[HealthState] = SkipValidationToken;

                    expectQueryResult[expectResultRow[ServiceName]] = expectResultRow;
                }
            }
        }
        else
        {
            for(auto serviceInfo = services.begin(); serviceInfo != services.end(); ++serviceInfo)
            {
                auto serviceType = serviceInfo->GetServiceType();
                if (serviceInfo->ServiceDescription.ApplicationName == desiredApplicationName)
                {
                    VerifyResultRow expectResultRow;
                    expectResultRow[ServiceName] = wformatString(serviceInfo->Name);
                    expectResultRow[ServiceStatus] = GetServiceStatusString(serviceInfo->Status);
                    if (serviceInfo->ServiceDescription.IsStateful)
                    {
                        expectResultRow[ServiceType] = wformatString(FabricTestDispatcher::ServiceTypeStateful);
                        expectResultRow[ServiceTypeName] = wformatString(serviceType.Type.ServiceTypeName);
                        expectResultRow[ServiceManifestVersion] = wformatString(serviceInfo->ServiceDescription.PackageVersionInstance.Version.RolloutVersionValue);
                        expectResultRow[HasPersistedState] = wformatString(serviceInfo->ServiceDescription.HasPersistedState);
                    }
                    else
                    {
                        expectResultRow[ServiceType] = wformatString(FabricTestDispatcher::ServiceTypeStateless);
                        expectResultRow[ServiceTypeName] = wformatString(serviceType.Type.ServiceTypeName);
                        expectResultRow[ServiceManifestVersion] = wformatString(serviceInfo->ServiceDescription.PackageVersionInstance.Version.RolloutVersionValue);
                    }

                    // Special value so validation skips this
                    expectResultRow[HealthState] = SkipValidationToken;

                    expectQueryResult[expectResultRow[ServiceName]] = expectResultRow;
                }
            }
        }

        return true;
    }
    else if (StringUtility::AreEqualCaseInsensitive(queryName, QueryNames::ToString(QueryNames::GetSystemServicesList)))
    {
        vector<ServiceSnapshot> result;

        auto fm = FABRICSESSION.FabricDispatcher.GetFM();
        auto services = fm->SnapshotServiceCache();
        auto serviceManifestVersion = fm->GetCurrentFabricVersionInstance().Version.CodeVersion.ToString();
        for(auto serviceInfo = services.begin(); serviceInfo != services.end(); ++serviceInfo)
        {
            if (serviceInfo->GetServiceType().Type.IsSystemServiceType())
            {
                result.push_back(*serviceInfo);
            }
        }

        services = FABRICSESSION.FabricDispatcher.GetFMM()->SnapshotServiceCache();
        for(auto serviceInfo = services.begin(); serviceInfo != services.end(); ++serviceInfo)
        {
            if (serviceInfo->GetServiceType().Type.IsSystemServiceType())
            {
                result.push_back(*serviceInfo);
            }
        }

        for (auto serviceInfo = result.begin(); serviceInfo != result.end(); ++serviceInfo)
        {
            auto serviceType = serviceInfo->GetServiceType();
            VerifyResultRow expectResultRow;
            expectResultRow[ServiceName] = SystemServiceApplicationNameHelper::GetPublicServiceName(serviceInfo->Name);
            expectResultRow[ServiceStatus] = GetServiceStatusString(serviceInfo->Status);
            if (serviceInfo->ServiceDescription.IsStateful)
            {
                expectResultRow[ServiceType] = wformatString(FabricTestDispatcher::ServiceTypeStateful);
                expectResultRow[ServiceTypeName] = wformatString(serviceType.Type.ServiceTypeName);
                expectResultRow[ServiceManifestVersion] = serviceManifestVersion;
                expectResultRow[HasPersistedState] = wformatString(serviceInfo->ServiceDescription.HasPersistedState);
            }
            else
            {
                expectResultRow[ServiceType] = wformatString(FabricTestDispatcher::ServiceTypeStateless);
                expectResultRow[ServiceTypeName] = wformatString(serviceType.Type.ServiceTypeName);
                expectResultRow[ServiceManifestVersion] = serviceManifestVersion;
            }

            expectResultRow[HealthState] = SkipValidationToken;

            expectQueryResult[expectResultRow[ServiceName]] = expectResultRow;
        }

        return true;
    }

    TestSession::WriteError(
        TraceSource,
        "Invalid or unsupported queryName {0} for verifyAll",
        queryName);
    return false;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultNode(FABRIC_NODE_QUERY_RESULT_ITEM const * nodeResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        if(nodeResult[i].NodeStatus != FABRIC_QUERY_NODE_STATUS_INVALID)
        {
            wstring nodeName(nodeResult[i].NodeName);
            wstring ipAddressOrFQFN(nodeResult[i].IpAddressOrFQDN);
            wstring codeVersion = wstring(nodeResult[i].CodeVersion);
            wstring configVersion = wstring(nodeResult[i].ConfigVersion);
            wstring nodeType(nodeResult[i].NodeType);
            wstring upgradeDomain(nodeResult[i].UpgradeDomain);
            wstring faultDomain = wstring(nodeResult[i].FaultDomain);

            ServiceModel::NodeQueryResult nodeQueryResult;
            TestSession::FailTestIfNot(
                nodeQueryResult.FromPublicApi(nodeResult[i]).IsSuccess(),
                "NodeQueryResult::FromPublic API failed");
            TestSession::WriteInfo(
                TraceSource,
                "{0}",
                nodeQueryResult);

            VerifyResultRow actualResultRow;

            actualResultRow[NodeName] = nodeName;
            actualResultRow[IpAddressOrFQFN] = ipAddressOrFQFN;
            actualResultRow[NodeType] = nodeType;
            actualResultRow[CodeVersion] = codeVersion;
            actualResultRow[ConfigVersion] = configVersion;
            actualResultRow[NodeStatus] = wformatString(nodeResult[i].NodeStatus);
            actualResultRow[IsSeedNode] = nodeResult[i].IsSeedNode==TRUE ? L"true" : L"false";
            actualResultRow[UpgradeDomain] = upgradeDomain;
            actualResultRow[FaultDomain] = faultDomain;
            actualResultRow[HealthState] = TestFabricClientHealth::GetHealthStateString(nodeResult[i].AggregatedHealthState);

            actualQueryResult[nodeName] = actualResultRow;
        }
        else
        {
            TestSession::WriteInfo(
                TraceSource,
                "Invalid node {0} found. Skipping",
                wstring(nodeResult[i].NodeName));
        }
    }

    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultApplicationType(FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM const * applicationTypeResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        wstring applicationTypeName(applicationTypeResult[i].ApplicationTypeName);
        wstring applicationTypeVersion(applicationTypeResult[i].ApplicationTypeVersion);
        map<wstring, wstring> parameterList;
        ApplicationTypeStatus::Enum status(ApplicationTypeStatus::Invalid);
        wstring statusDetails;
        FABRIC_APPLICATION_TYPE_DEFINITION_KIND definitionKind = FABRIC_APPLICATION_TYPE_DEFINITION_KIND_INVALID;

        if (applicationTypeResult[i].Reserved != NULL)
        {
            auto * ex1 = reinterpret_cast<FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_EX1*>(applicationTypeResult[i].Reserved);
            status = ApplicationTypeStatus::FromPublicApi(ex1->Status);
            statusDetails = ex1->StatusDetails == NULL ? L"" : wstring(ex1->StatusDetails);

            if (ex1->Reserved != nullptr)
            {
                auto ex2 = reinterpret_cast<FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_EX2 *>(ex1->Reserved);
                definitionKind = ex2->ApplicationTypeDefinitionKind;
            }
        }

        for(ULONG j = 0; j < applicationTypeResult[i].DefaultParameters->Count; ++j)
        {
            parameterList.insert(make_pair(wstring(applicationTypeResult[i].DefaultParameters[j].Items->Name), wstring(applicationTypeResult[i].DefaultParameters[j].Items->Value)));
        }

        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            ServiceModel::ApplicationTypeQueryResult(
                applicationTypeName,
                applicationTypeVersion,
                parameterList,
                status,
                statusDetails,
                definitionKind));

        wstring defaultParameters = L"";
        for(ULONG j = 0; j < applicationTypeResult[i].DefaultParameters->Count; ++j)
        {
            defaultParameters.append(wformatString("{0}:{1};", wstring(applicationTypeResult[i].DefaultParameters[j].Items->Name), wstring(applicationTypeResult[i].DefaultParameters[j].Items->Value)));
        }

        VerifyResultRow actualResultRow;
        actualResultRow[ApplicationTypeName] = applicationTypeName;
        actualResultRow[ApplicationTypeVersion] = applicationTypeVersion;
        actualResultRow[ApplicationTypeDefinitionKind] = wformatString(Management::ClusterManager::ApplicationTypeDefinitionKind::FromPublicApi(definitionKind));
        actualResultRow[DefaultParameters] = defaultParameters;
        actualResultRow[Status] = wformatString("{0}", status);
        actualResultRow[StatusDetails] = statusDetails;
        actualQueryResult[applicationTypeName] = actualResultRow;
    }

    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultApplication(FABRIC_APPLICATION_QUERY_RESULT_ITEM const * applicationResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        Common::Uri applicationUri;
        wstring applicationName = wstring(applicationResult[i].ApplicationName);

        TestSession::FailTestIfNot(
            Common::Uri::TryParse(wstring(applicationResult[i].ApplicationName), applicationUri),
            "Application URI has invalid format, Uri = {0}",
            applicationName);

        ServiceModel::ApplicationQueryResult applicationQueryResult;
        applicationQueryResult.FromPublicApi(applicationResult[i]);
        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            applicationQueryResult);

        wstring applicationParameters = L"";
        for(ULONG j = 0; j < applicationResult[i].ApplicationParameters->Count; ++j)
        {
            applicationParameters.append(wformatString("{0}:{1};", wstring(applicationResult[i].ApplicationParameters[j].Items->Name), wstring(applicationResult[i].ApplicationParameters[j].Items->Value)));
        }

        VerifyResultRow actualResultRow;
        actualResultRow[ApplicationName] = applicationName;
        actualResultRow[ApplicationTypeName] = wstring(applicationResult[i].ApplicationTypeName);
        actualResultRow[ApplicationTypeVersion] = wstring(applicationResult[i].ApplicationTypeVersion);
        actualResultRow[ApplicationStatus] = ApplicationStatus::ToString(ApplicationStatus::ConvertToServiceModelApplicationStatus(applicationResult[i].Status));
        actualResultRow[ApplicationParameters] = applicationParameters;
        actualResultRow[HealthState] = TestFabricClientHealth::GetHealthStateString(applicationResult[i].HealthState);

        if (applicationResult[i].Reserved != nullptr)
        {
            auto ex1 = reinterpret_cast<FABRIC_APPLICATION_QUERY_RESULT_ITEM_EX1 *>(applicationResult[i].Reserved);

            if (ex1->Reserved != nullptr)
            {
                auto ex2 = reinterpret_cast<FABRIC_APPLICATION_QUERY_RESULT_ITEM_EX2 *>(ex1->Reserved);
                actualResultRow[ApplicationDefinitionKind] = wformatString(Management::ClusterManager::ApplicationDefinitionKind::FromPublicApi(ex2->ApplicationDefinitionKind));
            }
        }

        actualQueryResult[applicationName] = actualResultRow;
    }
    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultComposeDeployment(FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM const * composeDeploymentResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        wstring const & deploymentName = composeDeploymentResult[i].DeploymentName;
        Common::NamingUri applicationUri;
        wstring applicationName = wstring(composeDeploymentResult[i].ApplicationName);
        TestSession::FailTestIfNot(
            NamingUri::TryParse(applicationName, applicationUri),
            "Compose Application Uri has invalid format, Uri = {0}",
            applicationName);

        ServiceModel::ComposeDeploymentStatusQueryResult composeDeploymentQueryResult;
        composeDeploymentQueryResult.FromPublicApi(composeDeploymentResult[i]);
        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            composeDeploymentQueryResult);

        VerifyResultRow actualResultRow;
        actualResultRow[DeploymentName] = deploymentName;
        actualResultRow[ApplicationName] = applicationName;
        actualResultRow[Status] = ComposeDeploymentStatus::ToString(composeDeploymentQueryResult.Status);
        actualResultRow[StatusDetails] = wstring(composeDeploymentQueryResult.StatusDetails);

        actualQueryResult[deploymentName] = actualResultRow;
    }
    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultDeployedApplication(FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM const * applicationResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        ServiceModel::DeployedApplicationQueryResult applicationQueryResult;
        applicationQueryResult.FromPublicApi(applicationResult[i]);
        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            applicationQueryResult);

        Common::Uri applicationUri;
        TestSession::FailTestIfNot(
            Common::Uri::TryParse(applicationQueryResult.ApplicationName, applicationUri),
            "Application URI has invalid format, Uri = {0}",
            applicationQueryResult.ApplicationName);

        VerifyResultRow actualResultRow;
        actualResultRow[ApplicationName] = applicationQueryResult.ApplicationName;
        actualResultRow[ApplicationTypeName] = applicationQueryResult.ApplicationTypeName;
        actualResultRow[DeploymentStatus] = DeploymentStatus::ToString(applicationQueryResult.DeployedApplicationStatus);

        if (applicationResult[i].Reserved != nullptr)
        {
            auto ex1 = reinterpret_cast<FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_EX *>(applicationResult[i].Reserved);

            if (ex1->Reserved != nullptr)
            {
                auto ex2 = reinterpret_cast<FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_EX2 *>(ex1->Reserved);
                actualResultRow[HealthState] = TestFabricClientHealth::GetHealthStateString(ex2->HealthState);
            }
        }

        actualQueryResult[applicationQueryResult.ApplicationName] = actualResultRow;
    }
    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultDeployedServiceManifest(FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM const * serviceManifestResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        ServiceModel::DeployedServiceManifestQueryResult serviceManifestQueryResult;
        serviceManifestQueryResult.FromPublicApi(serviceManifestResult[i]);
        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            serviceManifestQueryResult);

        VerifyResultRow actualResultRow;
        actualResultRow[ServiceManifestName] = serviceManifestQueryResult.ServiceManifestName;
        actualResultRow[ServiceManifestVersion] = serviceManifestQueryResult.ServiceManifestVersion;
        actualResultRow[DeploymentStatus] = DeploymentStatus::ToString(serviceManifestQueryResult.DeployedServicePackageStatus);
        actualQueryResult[serviceManifestResult[i].ServiceManifestName] = actualResultRow;
    }
    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultDeployedServiceType(FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM const * serviceTypeResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        ServiceModel::DeployedServiceTypeQueryResult serviceTypeQueryResult;
        serviceTypeQueryResult.FromPublicApi(serviceTypeResult[i]);
        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            serviceTypeQueryResult);

        VerifyResultRow actualResultRow;

        wstring activationId;
        if (serviceTypeResult[i].Reserved != nullptr)
        {
            auto ex1 = reinterpret_cast<FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM_EX1 *>(serviceTypeResult[i].Reserved);

            auto error = StringUtility::LpcwstrToWstring2(ex1->ServicePackageActivationId, true, activationId);
            if (!error.IsSuccess())
            {
                TestSession::WriteWarning(
                    TraceSource,
                    "Unable to parse servicePackageActivationId in ProcessQueryResultDeployedServiceType");
            }

            actualResultRow[ActivationId] = activationId;
        }

        wstring hasActivationIdIndicator;
        wstring serviceTypeNameWithActivationId;
        if (activationId.empty())
        {
            hasActivationIdIndicator = wstring();
            serviceTypeNameWithActivationId = wstring(serviceTypeResult[i].ServiceTypeName);
        }
        else
        {
            hasActivationIdIndicator = wstring(L"+");
            serviceTypeNameWithActivationId = wstring(serviceTypeResult[i].ServiceTypeName) + hasActivationIdIndicator;
        }

        actualResultRow[ServiceTypeName] = wstring(serviceTypeNameWithActivationId);
        actualResultRow[CodePackageName] = wstring(serviceTypeResult[i].CodePackageName);
        actualResultRow[ServiceManifestName] = wstring(serviceTypeResult[i].ServiceManifestName);

        actualQueryResult[(serviceTypeResult[i].ServiceTypeName + hasActivationIdIndicator)] = actualResultRow;

        TestSession::FailTestIfNot(
            serviceTypeResult[i].Status == FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_REGISTERED,
            "ServiceType registration status invalid: {0}",
            static_cast<ULONG>(serviceTypeResult[i].Status));
    }

    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultDeployedCodePackage(FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM const * codePackageResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        ServiceModel::DeployedCodePackageQueryResult codePackageQueryResult;
        codePackageQueryResult.FromPublicApi(codePackageResult[i]);
        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            codePackageQueryResult);

        VerifyResultRow actualResultRow;
        actualResultRow[CodePackageName] = codePackageQueryResult.CodePackageName;
        actualResultRow[CodePackageVersion] = codePackageQueryResult.CodePackageVersion;
        actualResultRow[ServiceManifestName] = codePackageQueryResult.ServiceManifestName;
        actualResultRow[DeploymentStatus] = DeploymentStatus::ToString(codePackageQueryResult.DeployedCodePackageStatus);
        actualResultRow[RunFrequencyInterval] = wformatString("{0}", codePackageQueryResult.RunFrequencyInterval);

        if(codePackageQueryResult.EntryPoint.EntryPointStatus == EntryPointStatus::Started)
        {
            TestSession::FailTestIf(codePackageQueryResult.EntryPoint.ProcessId < 1, "ProcessId should be greater than 0 when EntryPoint in started");
            TestSession::FailTestIf(codePackageQueryResult.EntryPoint.EntryPointLocation.empty(), "EntryPointLocation cannot be empty when EntryPoint is started");
        }

        actualResultRow[EntryPointStatus] = EntryPointStatus::ToString(codePackageQueryResult.EntryPoint.EntryPointStatus);

        actualQueryResult[codePackageResult[i].CodePackageName] = actualResultRow;
    }

    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultDeployedServiceReplica(FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM const * serviceReplicaResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        if (serviceReplicaResult[i].Kind == FABRIC_SERVICE_KIND_STATEFUL)
        {
            FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM* statefulDeployedReplica = reinterpret_cast<FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM*>(serviceReplicaResult[i].Value);
            
            TestSession::WriteInfo(
                TraceSource,
                "{0}",
                statefulDeployedReplica);

            VerifyResultRow actualResultRow;

            Guid partitionId = Common::Guid(statefulDeployedReplica->PartitionId);
            actualResultRow[PartitionId] = partitionId.ToString();

            wstring serviceName, serviceTypeName, serviceManifestVersion, codePackageName;
            StringUtility::LpcwstrToWstring2(statefulDeployedReplica->ServiceName, false, serviceName);
            StringUtility::LpcwstrToWstring2(statefulDeployedReplica->ServiceTypeName, false, serviceTypeName);
            StringUtility::LpcwstrToWstring2(statefulDeployedReplica->ServiceManifestVersion, true, serviceManifestVersion);
            StringUtility::LpcwstrToWstring2(statefulDeployedReplica->CodePackageName, true, codePackageName);

            actualResultRow[ServiceName] = serviceName;
            actualResultRow[ServiceTypeName] = serviceTypeName;
            actualResultRow[ServiceManifestVersion] = serviceManifestVersion;
            actualResultRow[CodePackageName] = codePackageName;

            actualResultRow[ReplicaId] = wformatString(statefulDeployedReplica->ReplicaId);
            actualResultRow[ReplicaRole] = wformatString(ReplicaRole);
            actualQueryResult[partitionId.ToString()] = actualResultRow;
        }
        else if (serviceReplicaResult[i].Kind == FABRIC_SERVICE_KIND_STATELESS)
        {
            FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM* statelessDeployedInstance = reinterpret_cast<FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM*>(serviceReplicaResult[i].Value);
            
            TestSession::WriteInfo(
                TraceSource,
                "{0}",
                statelessDeployedInstance);

            VerifyResultRow actualResultRow;

            Guid partitionId = Common::Guid(statelessDeployedInstance->PartitionId);
            actualResultRow[PartitionId] = partitionId.ToString();

            wstring serviceName, serviceTypeName, serviceManifestVersion, codePackageName;
            StringUtility::LpcwstrToWstring2(statelessDeployedInstance->ServiceName, false, serviceName);
            StringUtility::LpcwstrToWstring2(statelessDeployedInstance->ServiceTypeName, false, serviceTypeName);
            StringUtility::LpcwstrToWstring2(statelessDeployedInstance->ServiceManifestVersion, true, serviceManifestVersion);
            StringUtility::LpcwstrToWstring2(statelessDeployedInstance->CodePackageName, true, codePackageName);

            actualResultRow[ServiceName] = serviceName;
            actualResultRow[ServiceTypeName] = serviceTypeName;
            actualResultRow[ServiceManifestVersion] = serviceManifestVersion;
            actualResultRow[CodePackageName] = codePackageName;

            actualResultRow[InstanceId] = wformatString(statelessDeployedInstance->InstanceId);
            actualResultRow[ReplicaRole] = wformatString(ReplicaRole);
            actualQueryResult[partitionId.ToString()] = actualResultRow;
        }
    }

    return actualQueryResult;
}

std::wstring TestFabricClientQuery::GetServiceStatusString(FABRIC_QUERY_SERVICE_STATUS serviceStatus)
{
    switch (serviceStatus)
    {
    case FABRIC_QUERY_SERVICE_STATUS_UNKNOWN:
        return L"Unknown";
        break;
    case FABRIC_QUERY_SERVICE_STATUS_ACTIVE:
        return L"Active";
        break;
    case FABRIC_QUERY_SERVICE_STATUS_UPGRADING:
        return L"Upgrading";
        break;
    case FABRIC_QUERY_SERVICE_STATUS_DELETING:
        return L"Deleting";
        break;
    case FABRIC_QUERY_SERVICE_STATUS_CREATING:
        return L"Creating";
        break;
    case FABRIC_QUERY_SERVICE_STATUS_FAILED:
        return L"Failed";
        break;
    default:
        return L"Invalid";
        break;
    }
}

wstring TestFabricClientQuery::GetComposeDeploymentUpgradeState(FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE const state)
{
    switch (state)
    {
    case FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_PROVISIONING_TARGET:
        return L"ProvisioningTarget";
        break;
    case FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS:
        return L"RollingForwardInProgress";
        break;
    case FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_PENDING:
        return L"RollingForwardPending";
        break;
    case FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_UNPROVISIONING_CURRENT:
        return L"UnprovisioningCurrent";
        break;
    case FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED:
        return L"RollingForwardCompleted";
        break;
    case FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS:
        return L"RollingBackInProgress";
        break;
    case FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_UNPROVISIONING_TARGET:
        return L"UnprovisioningTarget";
        break;
    case FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_BACK_COMPLETED:
        return L"RollingBackCompleted";
        break;
    case FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_FAILED:
        return L"Failed";
        break;
    default:
        return L"Invalid";
    }
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultService(FABRIC_SERVICE_QUERY_RESULT_ITEM const * serviceResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        if(serviceResult[i].Kind == FABRIC_SERVICE_KIND_STATEFUL)
        {
            FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM* statefulService = reinterpret_cast<FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM*>(serviceResult[i].Value);
            Common::Uri serviceUri;
            wstring serviceName(statefulService->ServiceName);
            TestSession::FailTestIfNot(
                Common::Uri::TryParse(serviceName, serviceUri),
                "Stateful Service URI has invalid format, Uri = {0}",
                serviceName);
            wstring serviceTypeName(statefulService->ServiceTypeName);
            wstring serviceManifesteVersion(statefulService->ServiceManifestVersion);

            VerifyResultRow actualResultRow;
            actualResultRow[ServiceName] = serviceUri.ToString();
            actualResultRow[ServiceType] = FabricTestDispatcher::ServiceTypeStateful;
            actualResultRow[ServiceTypeName] = serviceTypeName;
            actualResultRow[ServiceManifestVersion] = serviceManifesteVersion;
            actualResultRow[HasPersistedState] = statefulService->HasPersistedState ? L"true" : L"false";
            actualResultRow[HealthState] = TestFabricClientHealth::GetHealthStateString(statefulService->HealthState);
            if (statefulService->Reserved != NULL)
            {
                FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM_EX1 * statefulServiceEx1 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM_EX1*>(statefulService->Reserved);
                actualResultRow[ServiceStatus] = GetServiceStatusString(statefulServiceEx1->ServiceStatus);
            }

            actualQueryResult[serviceUri.ToString()] = actualResultRow;
        }
        else if (serviceResult[i].Kind == FABRIC_SERVICE_KIND_STATELESS)
        {
            FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM* statelessService = reinterpret_cast<FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM*>(serviceResult[i].Value);
            Common::Uri serviceUri;
            wstring serviceName(statelessService->ServiceName);

            TestSession::FailTestIfNot(
                Common::Uri::TryParse(serviceName, serviceUri),
                "Stateless Service URI has invalid format, Uri = {0}",
                serviceName);

            wstring serviceTypeName(statelessService->ServiceTypeName);
            wstring serviceManifesteVersion(statelessService->ServiceManifestVersion);

            VerifyResultRow actualResultRow;
            actualResultRow[ServiceName] = serviceUri.ToString();
            actualResultRow[ServiceType] = FabricTestDispatcher::ServiceTypeStateless;
            actualResultRow[ServiceTypeName] = serviceTypeName;
            actualResultRow[ServiceManifestVersion] = serviceManifesteVersion;
            actualResultRow[HealthState] = TestFabricClientHealth::GetHealthStateString(statelessService->HealthState);
            if (statelessService->Reserved != NULL)
            {
                FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM_EX1 * statelessServiceEx1 = reinterpret_cast<FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM_EX1*>(statelessService->Reserved);
                actualResultRow[ServiceStatus] = GetServiceStatusString(statelessServiceEx1->ServiceStatus);
            }

            actualQueryResult[serviceUri.ToString()] = actualResultRow;
        }
        else
        {
            TestSession::FailTest("Invalid ServiceResult Kind");
        }

        ServiceModel::ServiceQueryResult serviceQueryResult;
        auto error = serviceQueryResult.FromPublicApi(serviceResult[i]);
        TestSession::FailTestIfNot(
            !error.IsSuccess(),
            "ServiceQueryResult.FromPublicApi failed with error code {0}",
            error);

        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            serviceQueryResult);
    }

    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultStatefulPartition(FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM const * partitionResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        TestSession::FailTestIf(
            (partitionResult[i].Kind != FABRIC_SERVICE_KIND_STATEFUL),
            "Invalid service partition type.");
        auto statefulPartition = reinterpret_cast<FABRIC_STATEFUL_SERVICE_PARTITION_QUERY_RESULT_ITEM*>(partitionResult[i].Value);

        ServiceModel::ServicePartitionQueryResult partitionQueryResult;
        TestSession::FailTestIfNot(
            partitionQueryResult.FromPublicApi(partitionResult[i]).IsSuccess(),
            "ServicePartition::FromPublic API failed");
        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            partitionQueryResult);

        VerifyResultRow actualResultRow;

        ToServicePartitionInformation(*statefulPartition->PartitionInformation, actualResultRow);

        actualResultRow[TargetReplicaSetSize] = StringUtility::ToWString(statefulPartition->TargetReplicaSetSize);
        actualResultRow[MinReplicaSetSize] = StringUtility::ToWString(statefulPartition->MinReplicaSetSize);
        actualResultRow[PartitionStatus] = wformatString(statefulPartition->PartitionStatus);
        actualResultRow[HealthState] = TestFabricClientHealth::GetHealthStateString(statefulPartition->HealthState);

        actualQueryResult[actualResultRow[PartitionId]] = actualResultRow;
    }
    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultStatelessPartition(FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM const * partitionResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        TestSession::FailTestIf(
            (partitionResult[i].Kind != FABRIC_SERVICE_KIND_STATELESS),
            "Invalid service partition type.");
        auto statelessPartition = reinterpret_cast<FABRIC_STATELESS_SERVICE_PARTITION_QUERY_RESULT_ITEM*>(partitionResult[i].Value);
        ServiceModel::ServicePartitionQueryResult partitionQueryResult;
        TestSession::FailTestIfNot(
            partitionQueryResult.FromPublicApi(partitionResult[i]).IsSuccess(),
            "ServicePartition::FromPublic API failed");
        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            partitionQueryResult);

        VerifyResultRow actualResultRow;

        ToServicePartitionInformation(*statelessPartition->PartitionInformation, actualResultRow);

        actualResultRow[InstanceCount] = StringUtility::ToWString(statelessPartition->InstanceCount);
        actualResultRow[PartitionStatus] = wformatString(statelessPartition->PartitionStatus);
        actualResultRow[HealthState] = TestFabricClientHealth::GetHealthStateString(statelessPartition->HealthState);

        actualQueryResult[actualResultRow[PartitionId]] = actualResultRow;
    }
    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultStatefulReplica(FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM const * replicaResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        TestSession::FailTestIf(
            (replicaResult[i].Kind != FABRIC_SERVICE_KIND_STATEFUL),
            "Invalid service replica type.");
        auto statefulReplica = reinterpret_cast<FABRIC_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM*>(replicaResult[i].Value);

        ServiceModel::ServiceReplicaQueryResult replicaQueryResult;
        TestSession::FailTestIfNot(
            replicaQueryResult.FromPublicApi(replicaResult[i]).IsSuccess(),
            "ServiceReplica::FromPublic API failed");
        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            replicaQueryResult);

        wstring address(statefulReplica->ReplicaAddress);
        wstring nodeName(statefulReplica->NodeName);

        VerifyResultRow actualResultRow;
        actualResultRow[ReplicaId] = StringUtility::ToWString(statefulReplica->ReplicaId);
        actualResultRow[ReplicaRole] = FABRICSESSION.FabricDispatcher.GetReplicaRoleString(ToReplicaRole((statefulReplica->ReplicaRole)));
        actualResultRow[ReplicaStatus] = FABRICSESSION.FabricDispatcher.GetServiceReplicaStatusString(statefulReplica->ReplicaStatus);
        actualResultRow[ReplicaAddress] = address;
        actualResultRow[NodeName] = nodeName;
        actualResultRow[HealthState] = TestFabricClientHealth::GetHealthStateString(statefulReplica->AggregatedHealthState);

        actualQueryResult[actualResultRow[ReplicaId]] = actualResultRow;
    }

    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultStatelessInstance(FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM const * replicaResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        TestSession::FailTestIf(
            (replicaResult[i].Kind != FABRIC_SERVICE_KIND_STATELESS),
            "Invalid service replica type.");
        auto statelessInstance = reinterpret_cast<FABRIC_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM*>(replicaResult[i].Value);

        ServiceModel::ServiceReplicaQueryResult replicaQueryResult;
        TestSession::FailTestIfNot(
            replicaQueryResult.FromPublicApi(replicaResult[i]).IsSuccess(),
            "ServiceReplica::FromPublic API failed");
        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            replicaQueryResult);

        wstring address(statelessInstance->ReplicaAddress);
        wstring nodeName(statelessInstance->NodeName);

        VerifyResultRow actualResultRow;
        actualResultRow[InstanceId] = StringUtility::ToWString(statelessInstance->InstanceId);
        actualResultRow[InstanceStatus] = FABRICSESSION.FabricDispatcher.GetServiceReplicaStatusString(statelessInstance->ReplicaStatus);
        actualResultRow[ReplicaAddress] = address;
        actualResultRow[NodeName] = nodeName;
        actualResultRow[HealthState] = TestFabricClientHealth::GetHealthStateString(statelessInstance->AggregatedHealthState);
        actualQueryResult[actualResultRow[InstanceId]] = actualResultRow;
    }

    return actualQueryResult;
}

VerifyResultTable TestFabricClientQuery::ProcessQueryResultServiceType(FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM const * serviceTypeResult, ULONG resultCount)
{
    VerifyResultTable actualQueryResult;
    for (ULONG i = 0; i < resultCount; ++i)
    {
        const FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM * currentItem = &(serviceTypeResult[i]);

        wstring serviceManifestVersion(currentItem->ServiceManifestVersion);
        ServiceTypeQueryResult serviceTypeQueryResult;
        auto error = serviceTypeQueryResult.FromPublicApi(serviceTypeResult[i]);
        TestSession::FailTestIf(
            !error.IsSuccess(),
            "ServiceTypeQueryResult::FromPublicApi failed with error {0}",
            error);

        VerifyResultRow actualResultRow;
        actualResultRow[ServiceTypeName] = serviceTypeQueryResult.ServiceTypeDescriptionObj.ServiceTypeName;
        actualResultRow[ServiceManifestVersion] = serviceTypeQueryResult.ServiceManifestVersion;
        actualResultRow[UseImplicitHost] = (serviceTypeQueryResult.ServiceTypeDescriptionObj.UseImplicitHost ? L"true":L"false");
        if (serviceTypeQueryResult.ServiceTypeDescriptionObj.IsStateful)
        {
            actualResultRow[HasPersistedState] = (serviceTypeQueryResult.ServiceTypeDescriptionObj.HasPersistedState ? L"true" : L"false");
        }
        actualQueryResult[serviceTypeQueryResult.ServiceTypeDescriptionObj.ServiceTypeName] = actualResultRow;

        TestSession::WriteInfo(
            TraceSource,
            "{0}",
            serviceTypeQueryResult);
    }

    return actualQueryResult;
}

ServiceModel::ServicePartitionInformation TestFabricClientQuery::ToServicePartitionInformation(FABRIC_SERVICE_PARTITION_INFORMATION const& servicePartitionInformation, VerifyResultRow & actualResultRow)
{
    if (servicePartitionInformation.Kind == FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
    {
        FABRIC_SINGLETON_PARTITION_INFORMATION* servicePartitionInformationPtr = reinterpret_cast<FABRIC_SINGLETON_PARTITION_INFORMATION*>(servicePartitionInformation.Value);
        Guid partionId = Common::Guid(servicePartitionInformationPtr->Id);
        actualResultRow[PartitionId] = partionId.ToString();
        actualResultRow[PartitionKind] = FabricTestDispatcher::PartitionKindSingleton;
        return move(ServiceModel::ServicePartitionInformation(partionId));
    }
    else if (servicePartitionInformation.Kind == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE)
    {
        FABRIC_INT64_RANGE_PARTITION_INFORMATION* servicePartitionInformationPtr = reinterpret_cast<FABRIC_INT64_RANGE_PARTITION_INFORMATION*>(servicePartitionInformation.Value);
        Guid partionId = Common::Guid(servicePartitionInformationPtr->Id);
        actualResultRow[PartitionId] = partionId.ToString();
        actualResultRow[PartitionKind] = FabricTestDispatcher::PartitionKindInt64Range;
        actualResultRow[PartitionLowKey] = StringUtility::ToWString(servicePartitionInformationPtr->LowKey);
        actualResultRow[PartitionHighKey] = StringUtility::ToWString(servicePartitionInformationPtr->HighKey);
        return move(ServiceModel::ServicePartitionInformation(
            partionId,
            static_cast<int64>(servicePartitionInformationPtr->LowKey),
            static_cast<int64>(servicePartitionInformationPtr->HighKey)));
    }
    else if (servicePartitionInformation.Kind == FABRIC_SERVICE_PARTITION_KIND_NAMED)
    {
        FABRIC_NAMED_PARTITION_INFORMATION* servicePartitionInformationPtr = reinterpret_cast<FABRIC_NAMED_PARTITION_INFORMATION*>(servicePartitionInformation.Value);
        Guid partionId = Common::Guid(servicePartitionInformationPtr->Id);
        wstring paritionName(servicePartitionInformationPtr->Name);
        actualResultRow[PartitionId] = partionId.ToString();
        actualResultRow[PartitionKind] = FabricTestDispatcher::PartitionKindName;
        actualResultRow[PartitionName] = paritionName;
        return move(ServiceModel::ServicePartitionInformation(
            partionId,
            paritionName));
    }
    else
    {
        TestSession::FailTest("Invalid PartitionResult Kind");
    }
}

Reliability::ReplicaRole::Enum TestFabricClientQuery::ToReplicaRole(FABRIC_REPLICA_ROLE replicaRole)
{
    switch(replicaRole)
    {
    case FABRIC_REPLICA_ROLE_NONE:
        return ReplicaRole::Enum::None;
    case FABRIC_REPLICA_ROLE_PRIMARY:
        return ReplicaRole::Enum::Primary;
    case FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
        return ReplicaRole::Enum::Idle;
    case FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
        return ReplicaRole::Enum::Secondary;
    default:
        return ReplicaRole::Enum::Unknown;
    }
}

Reliability::ReplicaStatus::Enum TestFabricClientQuery::ToReplicaStatus(FABRIC_REPLICA_STATUS replicaStatus)
{
    switch(replicaStatus)
    {
    case FABRIC_REPLICA_STATUS_DOWN:
        return ReplicaStatus::Enum::Down;
    case FABRIC_REPLICA_STATUS_UP:
        return ReplicaStatus::Enum::Up;
    default:
        return ReplicaStatus::Enum::Invalid;
    }
}
