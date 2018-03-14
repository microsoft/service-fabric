// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "ServiceModel/ServiceModel.h"
#include "Query.h"
#include "Common/Common.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace Federation;
using namespace Query;

const StringLiteral TraceType("QueryMessageHandlerTest");

class QueryMessageHandlerTest
{
protected:
    static void VerifyNodeQueryResult(
        QueryMessageHandler & handler,
        vector<wstring> const & nodeNames,
        vector<NodeId> const & nodeIds,
        int lastFM,
        int lastCM,
        int lastHM,
        int ctId);

    void VerifyServicePartitionQueryResult(
        QueryMessageHandler & handler,
        vector<Guid> const & partitionIds,
        int lastFM,
        int lastHM,
        int ctId);

    static void VerifyServiceQueryResult(
        QueryMessageHandler & handler,
        vector<NamingUri> const & serviceNames,
        int lastFM,
        int lastCM,
        int lastHM,
        int ctId);
};

BOOST_FIXTURE_TEST_SUITE(QueryMessageHandlerTestSuite,QueryMessageHandlerTest)

BOOST_AUTO_TEST_CASE(SimpleQueryMessageProcessingTest)
{
    shared_ptr<ComponentRoot> dummyComponentRoot = make_shared<ComponentRoot>();
    QueryMessageHandler queryMessageHandler_(*dummyComponentRoot, L"/");
    bool queryCalled = false;

    Common::ActivityId testActivityId(Guid::NewGuid());

    queryMessageHandler_.RegisterQueryHandler(
        [&queryCalled, &testActivityId](Query::QueryNames::Enum queryName, ServiceModel::QueryArgumentMap const & , Common::ActivityId const & activityId, Transport::MessageUPtr & )
        {
            queryCalled =
                (queryName == QueryNames::GetQueries);
            VERIFY_IS_TRUE(activityId.Guid == testActivityId.Guid, L"ActivityId does not match");
            return ErrorCode::Success();
        });

    auto requestMessage = QueryMessage::CreateMessage(QueryNames::GetQueries, QueryType::Simple, QueryArgumentMap(), L"/");
    requestMessage->Headers.Add(Transport::FabricActivityHeader(testActivityId));
    queryMessageHandler_.Open();
    queryMessageHandler_.ProcessQueryMessage(*requestMessage);
    VERIFY_IS_TRUE(queryCalled, L"Query was not called");
    queryMessageHandler_.Close();
}

BOOST_AUTO_TEST_CASE(SimpleQueryForwardMessageTest)
{
    shared_ptr<ComponentRoot> dummyComponentRoot = make_shared<ComponentRoot>();
    QueryMessageHandler queryMessageHandler_(*dummyComponentRoot, L"/");
    bool forwarderCalled = false;
    queryMessageHandler_.RegisterQueryForwardHandler(
        [&forwarderCalled](std::wstring const & addressSegment, std::wstring const & , Transport::MessageUPtr & , _Out_ Transport::MessageUPtr & )
        {
            forwarderCalled =
                (addressSegment == QueryAddresses::CMAddressSegment);
            return ErrorCode::Success();
        });
    Common::ActivityId testActivityId(Guid::NewGuid());
    auto requestMessage = QueryMessage::CreateMessage(QueryNames::GetApplicationList, QueryType::Simple, QueryArgumentMap(), L"/CM");
    requestMessage->Headers.Add(Transport::FabricActivityHeader(testActivityId));
    queryMessageHandler_.Open();
    queryMessageHandler_.ProcessQueryMessage(*requestMessage);
    VERIFY_IS_TRUE(forwarderCalled, L"Forwarder was not called");
    queryMessageHandler_.Close();
}

BOOST_AUTO_TEST_CASE(ParallelQueryTest)
{
    shared_ptr<ComponentRoot> dummyComponentRoot = make_shared<ComponentRoot>();
    QueryMessageHandler queryMessageHandler_(*dummyComponentRoot, L"/");

    bool failFM = false, failHM = false;

    bool forwarderToFMCalled = false;
    bool forwarderToHMCalled = false;
    queryMessageHandler_.RegisterQueryForwardHandler(
        [&failFM, &failHM, &forwarderToFMCalled, &forwarderToHMCalled](std::wstring const & addressSegment, std::wstring const & , Transport::MessageUPtr & , _Out_ Transport::MessageUPtr & result)
        {
            if (addressSegment == QueryAddresses::FMAddressSegment) { forwarderToFMCalled = true; }
            if (addressSegment == QueryAddresses::HMAddressSegment) { forwarderToHMCalled = true; }

            if (failFM || failHM)
            {
                result = make_unique<Transport::Message>(QueryResult(ErrorCodeValue::UnspecifiedError));
            }
            else
            {
                result = make_unique<Transport::Message>(QueryResult(vector<ServiceQueryResult>()));
            }

            return ErrorCode::Success();
        });
    queryMessageHandler_.Open();

    auto testFunc = [&forwarderToFMCalled, &forwarderToHMCalled, &queryMessageHandler_](bool failFM, bool failHM)
    {
        failFM = failFM;
        failHM = failHM;
        forwarderToFMCalled = false;
        forwarderToHMCalled = false;
        Common::ActivityId testActivityId(Guid::NewGuid());
        auto requestMessage = QueryMessage::CreateMessage(QueryNames::GetSystemServicesList, QueryType::Parallel, QueryArgumentMap(), L"/");
        requestMessage->Headers.Add(Transport::FabricActivityHeader(testActivityId));
        queryMessageHandler_.ProcessQueryMessage(*requestMessage);
        VERIFY_IS_TRUE(forwarderToFMCalled && forwarderToHMCalled, L"Parallel query forwarders were not called");
    };

    testFunc(true, true);
    testFunc(true, false);
    testFunc(false, true);
    testFunc(false, false);
    queryMessageHandler_.Close();
}

BOOST_AUTO_TEST_CASE(HealthParallelQueryTest)
{
    shared_ptr<ComponentRoot> dummyComponentRoot = make_shared<ComponentRoot>();
    QueryMessageHandler queryMessageHandler_(*dummyComponentRoot, L"/");
    ServiceReplicaQueryResult replicaResultItem = ServiceReplicaQueryResult::CreateStatelessServiceInstanceQueryResult(
        1,
        FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY,
        L"",
        L"",
        0);

    bool doFailFMQuery = false;
    bool doFailHMQuery = false;

    ErrorCodeValue::Enum fmError = ErrorCodeValue::FMNotReadyForUse;
    ErrorCodeValue::Enum hmError = ErrorCodeValue::NotReady;

    Guid guid = Guid::NewGuid();
    queryMessageHandler_.RegisterQueryForwardHandler(
        [&guid, &replicaResultItem, &doFailFMQuery, &doFailHMQuery, &fmError, &hmError](std::wstring const & addressSegment, std::wstring const & , Transport::MessageUPtr & , _Out_ Transport::MessageUPtr & result)->ErrorCode
        {
            if (addressSegment == QueryAddresses::FMAddressSegment)
            {
                if (doFailFMQuery) { return ErrorCode(fmError); }
                std::vector<ServiceReplicaQueryResult> replicaResults;
                replicaResults.push_back(replicaResultItem);
                result = make_unique<Message>(QueryResult(move(replicaResults)));
                return ErrorCode::Success();
            }
            else if (addressSegment == QueryAddresses::HMAddressSegment)
            {
                if (doFailHMQuery) { return ErrorCode(hmError); }
                std::vector<ReplicaAggregatedHealthState> aggregateHealthResult;
                aggregateHealthResult.push_back(ReplicaAggregatedHealthState(
                    FABRIC_SERVICE_KIND_STATELESS,
                    guid,
                    replicaResultItem.InstanceId,
                    FABRIC_HEALTH_STATE_OK));
                result = make_unique<Message>(QueryResult(move(aggregateHealthResult)));
                return ErrorCode::Success();
            }

            return ErrorCode(ErrorCodeValue::InvalidAddress);
        });
    queryMessageHandler_.Open();

    Common::ActivityId testActivityId(Guid::NewGuid());
    QueryArgumentMap args;
    args.Insert(QueryResourceProperties::Partition::PartitionId, guid.ToString());
    auto requestMessage = QueryMessage::CreateMessage(QueryNames::GetServicePartitionReplicaList, QueryType::Parallel, args, L"/");
    requestMessage->Headers.Add(Transport::FabricActivityHeader(testActivityId));

    doFailFMQuery = false;
    doFailHMQuery = true;
    auto reply = queryMessageHandler_.ProcessQueryMessage(*requestMessage);
    QueryResult result;
    VERIFY_IS_TRUE(reply->GetBody(result));
    vector<ServiceReplicaQueryResult> replicaResultList;
    VERIFY_IS_TRUE(result.MoveList(replicaResultList).IsSuccess());
    VERIFY_IS_TRUE(replicaResultList[0].HealthState == FABRIC_HEALTH_STATE_UNKNOWN);

    doFailFMQuery = true;
    doFailHMQuery = false;
    requestMessage = QueryMessage::CreateMessage(QueryNames::GetServicePartitionReplicaList, QueryType::Parallel, args, L"/");
    requestMessage->Headers.Add(Transport::FabricActivityHeader(testActivityId));
    reply = queryMessageHandler_.ProcessQueryMessage(*requestMessage);
    VERIFY_IS_TRUE(reply->GetBody(result));
    VERIFY_IS_TRUE(result.MoveList(replicaResultList).IsError(fmError));

    doFailFMQuery = true;
    doFailHMQuery = true;
    reply = queryMessageHandler_.ProcessQueryMessage(*requestMessage);
    VERIFY_IS_TRUE(reply->GetBody(result));
    VERIFY_IS_TRUE(result.MoveList(replicaResultList).IsError(fmError) || result.MoveList(replicaResultList).IsError(hmError));

    doFailFMQuery = false;
    doFailHMQuery = false;
    reply = queryMessageHandler_.ProcessQueryMessage(*requestMessage);
    VERIFY_IS_TRUE(reply->GetBody(result));
    VERIFY_IS_TRUE(result.MoveList(replicaResultList).IsSuccess());
    VERIFY_IS_TRUE(replicaResultList[0].HealthState == FABRIC_HEALTH_STATE_OK);

    VERIFY_IS_TRUE(queryMessageHandler_.Close().IsSuccess());
}

BOOST_AUTO_TEST_CASE(ParallelNodeQueryWithContinuationTokensTest)
{
    shared_ptr<ComponentRoot> dummyComponentRoot = make_shared<ComponentRoot>();
    QueryMessageHandler queryMessageHandler(*dummyComponentRoot, L"/");

    bool setFMContinuationToken = false;
    bool setCMContinuationToken = false;
    bool setHMContinuationToken = false;

    int lastFM = 8;
    int lastCM = 10;
    int lastHM = 7;

    int maxNode = 20;

    vector<wstring> nodeNames;
    for (int i = 0; i < maxNode; ++i)
    {
        nodeNames.push_back(wformatString("Node{0}", i));
    }

    vector<NodeId> nodeIds;
    for (int i = 0; i < maxNode; ++i)
    {
        nodeIds.push_back(NodeId(LargeInteger(i, i)));
    }

    queryMessageHandler.RegisterQueryForwardHandler(
        [&](std::wstring const & addressSegment, std::wstring const &, Transport::MessageUPtr &, _Out_ Transport::MessageUPtr & result)->ErrorCode
    {
        if (addressSegment == QueryAddresses::CMAddressSegment)
        {
            // List of nodes from component CM
            ListPager<NodeQueryResult> nodesCM;
            for (int i = 0; i <= lastCM; ++i)
            {
                NodeQueryResult node(
                    nodeNames[i],
                    L"127.0.0.1:2343",
                    L"NodeType",
                    true,
                    L"ud1",
                    L"fd1",
                    nodeIds[i]);
                VERIFY_IS_TRUE(nodesCM.TryAdd(move(node)).IsSuccess());
            }

            if (setCMContinuationToken)
            {
                nodesCM.SetPagingStatus(make_unique<PagingStatus>(PagingStatus::CreateContinuationToken<NodeId>(nodeIds[lastCM])));
            }

            result = make_unique<Message>(QueryResult(move(nodesCM)));
            return ErrorCode::Success();
        }
        else if (addressSegment == QueryAddresses::FMAddressSegment)
        {
            // List of nodes from component FM
            ListPager<NodeQueryResult> nodesFM;
            for (int i = 0; i <= lastFM; ++i)
            {
                NodeQueryResult node(
                    L"", // do not set nodename
                    L"127.0.0.1:2343",
                    L"NodeType",
                    true,
                    L"ud1",
                    L"fd1",
                    nodeIds[i]);
                VERIFY_IS_TRUE(nodesFM.TryAdd(move(node)).IsSuccess());
            }

            if (setFMContinuationToken)
            {
                nodesFM.SetPagingStatus(make_unique<PagingStatus>(PagingStatus::CreateContinuationToken<NodeId>(nodeIds[lastFM])));
            }

            result = make_unique<Message>(QueryResult(move(nodesFM)));
            return ErrorCode::Success();
        }
        else if (addressSegment == QueryAddresses::HMAddressSegment)
        {
            ListPager<NodeAggregatedHealthState> nodesHM;
            for (int i = 0; i <= lastHM; ++i)
            {
                VERIFY_IS_TRUE(nodesHM.TryAdd(NodeAggregatedHealthState(nodeNames[i], nodeIds[i], FABRIC_HEALTH_STATE_WARNING)).IsSuccess());
            }

            if (setHMContinuationToken)
            {
                nodesHM.SetPagingStatus(make_unique<PagingStatus>(PagingStatus::CreateContinuationToken<NodeId>(nodeIds[lastHM])));
            }

            result = make_unique<Message>(QueryResult(move(nodesHM)));
            return ErrorCode::Success();
        }

        return ErrorCode(ErrorCodeValue::InvalidAddress);
    });
    queryMessageHandler.Open();

    //
    // No continuation token
    //
    setFMContinuationToken = false;
    setCMContinuationToken = false;
    setHMContinuationToken = false;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyNodeQueryResult(queryMessageHandler, nodeNames, nodeIds, lastFM, lastCM, lastHM, -1);

    //
    // Continuation token from FM
    //
    setFMContinuationToken = true;
    setCMContinuationToken = false;
    setHMContinuationToken = false;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyNodeQueryResult(queryMessageHandler, nodeNames, nodeIds, lastFM, lastCM, lastHM, lastFM);

    //
    // Continuation token from CM
    //
    setFMContinuationToken = false;
    setCMContinuationToken = true;
    setHMContinuationToken = false;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyNodeQueryResult(queryMessageHandler, nodeNames, nodeIds, lastFM, lastCM, lastHM, lastCM);

    //
    // Continuation token from HM
    //
    setFMContinuationToken = false;
    setCMContinuationToken = false;
    setHMContinuationToken = true;
    lastHM = 5;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyNodeQueryResult(queryMessageHandler, nodeNames, nodeIds, lastFM, lastCM, lastHM, lastHM);

    //
    // Continuation token from FM and CM
    //
    setFMContinuationToken = true;
    setCMContinuationToken = true;
    setHMContinuationToken = false;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyNodeQueryResult(queryMessageHandler, nodeNames, nodeIds, lastFM, lastCM, lastHM, min(lastFM, lastCM));

    //
    // Continuation token from FM, CM and HM
    //
    setFMContinuationToken = true;
    setCMContinuationToken = true;
    setHMContinuationToken = true;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyNodeQueryResult(queryMessageHandler, nodeNames, nodeIds, lastFM, lastCM, lastHM, lastHM);

    VERIFY_IS_TRUE(queryMessageHandler.Close().IsSuccess());
}

BOOST_AUTO_TEST_CASE(ParallelServicePartitionQueryWithContinuationTokensTest)
{
    shared_ptr<ComponentRoot> dummyComponentRoot = make_shared<ComponentRoot>();
    QueryMessageHandler queryMessageHandler(*dummyComponentRoot, L"/");

    bool setFMContinuationToken = false;
    bool setHMContinuationToken = false;
    int lastFM = 8;
    int lastHM = 7;
    int maxServicePartition = 20;

    vector<Guid> partitionIds;
    for (int i = 0; i < maxServicePartition; ++i)
    {
        partitionIds.push_back(Guid::NewGuid());
    }

    sort(partitionIds.begin(), partitionIds.end());

    queryMessageHandler.RegisterQueryForwardHandler(
        [&](std::wstring const & addressSegment, std::wstring const &, Transport::MessageUPtr &, _Out_ Transport::MessageUPtr & result)->ErrorCode
    {
        if (addressSegment == QueryAddresses::FMAddressSegment)
        {
            ListPager<ServicePartitionQueryResult> partitionsFM;
            for (int i = 0; i <= lastFM; ++i)
            {
                ServicePartitionQueryResult partition = ServicePartitionQueryResult::CreateStatelessServicePartitionQueryResult(
                    ServicePartitionInformation(partitionIds[i], 0, 100),
                    7,
                    FABRIC_QUERY_SERVICE_PARTITION_STATUS_IN_QUORUM_LOSS);
                VERIFY_IS_TRUE(partitionsFM.TryAdd(move(partition)).IsSuccess());
            }

            if (setFMContinuationToken)
            {
                partitionsFM.SetPagingStatus(make_unique<PagingStatus>(PagingStatus::CreateContinuationToken<Guid>(partitionIds[lastFM])));
            }

            result = make_unique<Message>(QueryResult(move(partitionsFM)));
            return ErrorCode::Success();
        }
        else if (addressSegment == QueryAddresses::HMAddressSegment)
        {
            ListPager<PartitionAggregatedHealthState> partitionsHM;
            for (int i = 0; i <= lastHM; ++i)
            {
                VERIFY_IS_TRUE(partitionsHM.TryAdd(PartitionAggregatedHealthState(partitionIds[i], FABRIC_HEALTH_STATE_WARNING)).IsSuccess());
            }

            if (setHMContinuationToken)
            {
                partitionsHM.SetPagingStatus(make_unique<PagingStatus>(PagingStatus::CreateContinuationToken<Guid>(partitionIds[lastHM])));
            }

            result = make_unique<Message>(QueryResult(move(partitionsHM)));
            return ErrorCode::Success();
        }

        return ErrorCode(ErrorCodeValue::InvalidAddress);
    });
    queryMessageHandler.Open();

    //
    // No continuation token
    //
    setFMContinuationToken = false;
    setHMContinuationToken = false;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, HM={1}, last partition: FM={2}, HM={3}", setFMContinuationToken, setHMContinuationToken, lastFM, lastHM);
    VerifyServicePartitionQueryResult(queryMessageHandler, partitionIds, lastFM, lastHM, -1);

    //
    // Continuation token from FM
    //
    setFMContinuationToken = true;
    setHMContinuationToken = false;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, HM={1}, last partition: FM={2}, HM={3}", setFMContinuationToken, setHMContinuationToken, lastFM, lastHM);
    VerifyServicePartitionQueryResult(queryMessageHandler, partitionIds, lastFM, lastHM, lastFM);

    //
    // Continuation token from HM
    //
    setFMContinuationToken = false;
    setHMContinuationToken = true;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, HM={1}, last partition: FM={2}, HM={3}", setFMContinuationToken, setHMContinuationToken, lastFM, lastHM);
    VerifyServicePartitionQueryResult(queryMessageHandler, partitionIds, lastFM, lastHM, lastHM);

    //
    // Continuation token from FM and HM
    //
    setFMContinuationToken = true;
    setHMContinuationToken = true;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, HM={1}, last partition: FM={2}, HM={3}", setFMContinuationToken, setHMContinuationToken, lastFM, lastHM);
    VerifyServicePartitionQueryResult(queryMessageHandler, partitionIds, lastFM, lastHM, min(lastFM, lastHM));

    VERIFY_IS_TRUE(queryMessageHandler.Close().IsSuccess());
}

BOOST_AUTO_TEST_CASE(ParallelServiceQueryWithContinuationTokensTest)
{
    shared_ptr<ComponentRoot> dummyComponentRoot = make_shared<ComponentRoot>();
    bool setFMContinuationToken = false;
    bool setCMContinuationToken = false;
    bool setHMContinuationToken = false;

    int lastFM = 8;
    int lastCM = 10;
    int lastHM = 7;

    int maxService = 20;
    vector<NamingUri> serviceNames;
    for (int i = 0; i < maxService; ++i)
    {
        serviceNames.push_back(NamingUri(wformatString("ParallelServiceQueryWithContinuationTokensTest{0}", i)));
    }

    sort(serviceNames.begin(), serviceNames.end());

    QueryMessageHandler queryMessageHandler(*dummyComponentRoot, L"/");
    queryMessageHandler.RegisterQueryForwardHandler(
        [&](std::wstring const & addressSegment, std::wstring const &, Transport::MessageUPtr & requestMessage, _Out_ Transport::MessageUPtr & result)->ErrorCode
    {
        if (addressSegment == QueryAddresses::CMAddressSegment)
        {
            QueryRequestMessageBodyInternal messageBody;
            if (!requestMessage->GetBody(messageBody))
            {
                return ErrorCode(ErrorCodeValue::InvalidMessage);
            }

            if (messageBody.QueryName == QueryNames::GetAggregatedServiceHealthList)
            {
                ListPager<ServiceAggregatedHealthState> servicesHM;
                for (int i = 0; i <= lastHM; ++i)
                {
                    VERIFY_IS_TRUE(servicesHM.TryAdd(ServiceAggregatedHealthState(serviceNames[i].ToString(), FABRIC_HEALTH_STATE_WARNING)).IsSuccess());
                }

                if (setHMContinuationToken)
                {
                    servicesHM.SetPagingStatus(make_unique<PagingStatus>(serviceNames[lastHM].ToString()));
                }

                result = make_unique<Message>(QueryResult(move(servicesHM)));
            }
            else
            {
                // List of services from component CM
                ListPager<ServiceQueryResult> servicesCM;
                for (int i = 0; i <= lastCM; ++i)
                {
                    ServiceQueryResult service = ServiceQueryResult::CreateStatefulServiceQueryResult(
                        serviceNames[i],
                        L"",
                        L"",
                        false,
                        FABRIC_QUERY_SERVICE_STATUS_UNKNOWN);
                    VERIFY_IS_TRUE(servicesCM.TryAdd(move(service)).IsSuccess());
                }

                if (setCMContinuationToken)
                {
                    servicesCM.SetPagingStatus(make_unique<PagingStatus>(PagingStatus::CreateContinuationToken<Uri>(serviceNames[lastCM])));
                }

                result = make_unique<Message>(QueryResult(move(servicesCM)));
            }

            return ErrorCode::Success();
        }
        else if (addressSegment == QueryAddresses::FMAddressSegment)
        {
            // List of services from component FM
            ListPager<ServiceQueryResult> servicesFM;
            for (int i = 0; i <= lastFM; ++i)
            {
                ServiceQueryResult service = ServiceQueryResult::CreateStatefulServiceQueryResult(
                    serviceNames[i],
                    L"ServiceTypeName",
                    L"ServiceManifestName",
                    true,
                    FABRIC_QUERY_SERVICE_STATUS_ACTIVE);
                VERIFY_IS_TRUE(servicesFM.TryAdd(move(service)).IsSuccess());
            }

            if (setFMContinuationToken)
            {
                servicesFM.SetPagingStatus(make_unique<PagingStatus>(PagingStatus::CreateContinuationToken<Uri>(serviceNames[lastFM])));
            }

            result = make_unique<Message>(QueryResult(move(servicesFM)));
            return ErrorCode::Success();
        }

        return ErrorCode(ErrorCodeValue::InvalidAddress);
    });
    queryMessageHandler.Open();

    //
    // No continuation token
    //
    setFMContinuationToken = false;
    setCMContinuationToken = false;
    setHMContinuationToken = false;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyServiceQueryResult(queryMessageHandler, serviceNames, lastFM, lastCM, lastHM, -1);

    //
    // Continuation token from FM
    //
    setFMContinuationToken = true;
    setCMContinuationToken = false;
    setHMContinuationToken = false;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyServiceQueryResult(queryMessageHandler, serviceNames, lastFM, lastCM, lastHM, lastFM);

    //
    // Continuation token from CM
    //
    setFMContinuationToken = false;
    setCMContinuationToken = true;
    setHMContinuationToken = false;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyServiceQueryResult(queryMessageHandler, serviceNames, lastFM, lastCM, lastHM, lastCM);

    //
    // Continuation token from HM
    //
    setFMContinuationToken = false;
    setCMContinuationToken = false;
    setHMContinuationToken = true;
    lastHM = 5;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyServiceQueryResult(queryMessageHandler, serviceNames, lastFM, lastCM, lastHM, lastHM);

    //
    // Continuation token from FM and CM
    //
    setFMContinuationToken = true;
    setCMContinuationToken = true;
    setHMContinuationToken = false;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyServiceQueryResult(queryMessageHandler, serviceNames, lastFM, lastCM, lastHM, min(lastFM, lastCM));

    //
    // Continuation token from FM, CM and HM
    //
    setFMContinuationToken = true;
    setCMContinuationToken = true;
    setHMContinuationToken = true;
    BOOST_WMESSAGE_FORMAT("ContinuationTokens: FM={0}, CM={1}, HM={2}, last node: FM={3}, CM={4}, HM={5}", setFMContinuationToken, setCMContinuationToken, setHMContinuationToken, lastFM, lastCM, lastHM);
    VerifyServiceQueryResult(queryMessageHandler, serviceNames, lastFM, lastCM, lastHM, lastHM);

    VERIFY_IS_TRUE(queryMessageHandler.Close().IsSuccess());
}

BOOST_AUTO_TEST_SUITE_END()

void QueryMessageHandlerTest::VerifyNodeQueryResult(
    QueryMessageHandler & handler,
    vector<wstring> const & nodeNames,
    vector<NodeId> const & nodeIds,
    int lastFM,
    int lastCM,
    int lastHM,
    int ctId)
{
    Common::ActivityId testActivityId(Guid::NewGuid());
    QueryArgumentMap args;

    auto requestMessage = QueryMessage::CreateMessage(QueryNames::GetNodeList, QueryType::Parallel, args, L"/");
    requestMessage->Headers.Add(Transport::FabricActivityHeader(testActivityId.GetNestedActivity()));
    auto reply = handler.ProcessQueryMessage(*requestMessage);
    QueryResult result;
    VERIFY_IS_TRUE(reply->GetBody(result));

    vector<NodeQueryResult> resultList;
    VERIFY_IS_TRUE(result.MoveList(resultList).IsSuccess());
    auto pagingStatus = result.MovePagingStatus();
    int expectedCount = max(lastFM, lastCM) + 1;
    if (ctId != -1)
    {
        VERIFY_IS_FALSE(pagingStatus == nullptr);
        wstring expectedContinuationToken = wformatString("{0}", PagingStatus::CreateContinuationToken<NodeId>(nodeIds[ctId]));
        VERIFY_ARE_EQUAL(expectedContinuationToken, pagingStatus->ContinuationToken);
        expectedCount = ctId + 1;
    }
    else
    {
        VERIFY_IS_TRUE(pagingStatus == nullptr);
    }

    VERIFY_IS_TRUE(resultList.size() == static_cast<size_t>(expectedCount), wformatString("Merge results count: actual {0}, expected {1}", resultList.size(), expectedCount).c_str());
    for (size_t i = 0; i < resultList.size(); ++i)
    {
        VERIFY_IS_TRUE(resultList[i].NodeId == nodeIds[i]);

        if (i <= lastCM)
        {
            VERIFY_IS_TRUE(resultList[i].NodeName == nodeNames[i]);
        }

        if (i <= lastHM)
        {
            VERIFY_IS_TRUE(resultList[i].HealthState == FABRIC_HEALTH_STATE_WARNING);
        }
        else
        {
            VERIFY_IS_TRUE(resultList[i].HealthState == FABRIC_HEALTH_STATE_UNKNOWN);
        }
    }
}

void QueryMessageHandlerTest::VerifyServicePartitionQueryResult(
    QueryMessageHandler & handler,
    vector<Guid> const & partitionIds,
    int lastFM,
    int lastHM,
    int ctId)
{
    Common::ActivityId testActivityId(Guid::NewGuid());
    QueryArgumentMap args;
    args.Insert(Query::QueryResourceProperties::Service::ServiceName, L"fabric:/VerifyServicePartitionQueryResult/Service");

    auto requestMessage = QueryMessage::CreateMessage(QueryNames::GetServicePartitionList, QueryType::Parallel, args, L"/");
    requestMessage->Headers.Add(Transport::FabricActivityHeader(testActivityId.GetNestedActivity()));
    auto reply = handler.ProcessQueryMessage(*requestMessage);
    QueryResult result;
    VERIFY_IS_TRUE(reply->GetBody(result));

    vector<ServicePartitionQueryResult> resultList;
    VERIFY_IS_TRUE(result.MoveList(resultList).IsSuccess());
    auto pagingStatus = result.MovePagingStatus();
    int expectedCount = lastFM + 1;
    if (ctId != -1)
    {
        VERIFY_IS_FALSE(pagingStatus == nullptr);
        wstring expectedContinuationToken = wformatString("{0}", PagingStatus::CreateContinuationToken<Guid>(partitionIds[ctId]));
        VERIFY_ARE_EQUAL(expectedContinuationToken, pagingStatus->ContinuationToken);
        expectedCount = ctId + 1;
    }
    else
    {
        VERIFY_IS_TRUE(pagingStatus == nullptr);
    }

    VERIFY_IS_TRUE(resultList.size() == static_cast<size_t>(expectedCount), wformatString("Merge results count: actual {0}, expected {1}", resultList.size(), expectedCount).c_str());
    for (size_t i = 0; i < resultList.size(); ++i)
    {
        VERIFY_IS_TRUE(resultList[i].PartitionId == partitionIds[i]);

        if (i <= lastHM)
        {
            VERIFY_IS_TRUE(resultList[i].HealthState == FABRIC_HEALTH_STATE_WARNING);
        }
        else
        {
            VERIFY_IS_TRUE(resultList[i].HealthState == FABRIC_HEALTH_STATE_UNKNOWN);
        }
    }
}

void QueryMessageHandlerTest::VerifyServiceQueryResult(
    QueryMessageHandler & handler,
    vector<NamingUri> const & serviceNames,
    int lastFM,
    int lastCM,
    int lastHM,
    int ctId)
{
    Common::ActivityId testActivityId(Guid::NewGuid());
    QueryArgumentMap args;
    args.Insert(QueryResourceProperties::Application::ApplicationName, L"fabric:/VerifyServiceQueryResult/AppName");

    auto requestMessage = QueryMessage::CreateMessage(QueryNames::GetApplicationServiceList, QueryType::Parallel, args, L"/");
    requestMessage->Headers.Add(Transport::FabricActivityHeader(testActivityId.GetNestedActivity()));
    auto reply = handler.ProcessQueryMessage(*requestMessage);
    QueryResult result;
    VERIFY_IS_TRUE(reply->GetBody(result));

    vector<ServiceQueryResult> resultList;
    VERIFY_IS_TRUE(result.MoveList(resultList).IsSuccess());
    auto pagingStatus = result.MovePagingStatus();
    int expectedCount = max(lastFM, lastCM) + 1;
    if (ctId != -1)
    {
        VERIFY_IS_FALSE(pagingStatus == nullptr);
        wstring expectedContinuationToken = wformatString("{0}", PagingStatus::CreateContinuationToken<Uri>(serviceNames[ctId]));
        VERIFY_IS_TRUE(pagingStatus->ContinuationToken == expectedContinuationToken);
        expectedCount = ctId + 1;
    }
    else
    {
        VERIFY_IS_TRUE(pagingStatus == nullptr);
    }


    VERIFY_IS_TRUE(resultList.size() == static_cast<size_t>(expectedCount), wformatString("Merge results count: actual {0}, expected {1}", resultList.size(), expectedCount).c_str());
    for (size_t i = 0; i < resultList.size(); ++i)
    {
        VERIFY_IS_TRUE(resultList[i].ServiceName == serviceNames[i]);

        if (i <= lastFM)
        {
            // set by FM only
            VERIFY_IS_TRUE(resultList[i].ServiceStatus == FABRIC_QUERY_SERVICE_STATUS_ACTIVE);
        }
        else
        {
            VERIFY_IS_TRUE(resultList[i].ServiceStatus == FABRIC_QUERY_SERVICE_STATUS_UNKNOWN);
        }

        if (i <= lastHM)
        {
            VERIFY_IS_TRUE(resultList[i].HealthState == FABRIC_HEALTH_STATE_WARNING);
        }
        else
        {
            VERIFY_IS_TRUE(resultList[i].HealthState == FABRIC_HEALTH_STATE_UNKNOWN);
        }
    }
}
