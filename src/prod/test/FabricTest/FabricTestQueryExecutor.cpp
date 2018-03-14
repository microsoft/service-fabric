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
using namespace Api;

const StringLiteral TraceSource("FabricTest.FabricTestQueryExecutor");

map<wstring, shared_ptr<IQueryHandler>> FabricTestQueryExecutor::queryHandlers_;

FabricTestQueryExecutor::FabricTestQueryExecutor()
{
}

HRESULT FabricTestQueryExecutor::ExecuteQuery(Query::QueryNames::Enum queryName, ServiceModel::QueryArgumentMap const & queryArgs, __out ComPointer<IInternalFabricQueryResult2> & result)
{
    auto queryClient = CreateFabricQueryClient(FABRICSESSION.FabricDispatcher.Federation);
    ComPointer<IInternalFabricQueryClient2> internalQueryClient(queryClient, IID_IInternalFabricQueryClient2);

    wstring query = wformatString(queryName);

    ScopedHeap heap;
    FABRIC_STRING_PAIR_LIST args;
    queryArgs.ToPublicApi(heap, args);

    VerifyResultTable expectQueryResult;
    HRESULT hr = TestFabricClient::PerformFabricClientOperation(
        L"Query." + query,
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        TestSession::WriteInfo(TraceSource, "Executing Query {0}", query);
        return internalQueryClient->BeginInternalQuery(
            query.c_str(),
            &args,
            timeout,
            callback.GetRawPointer(),
            context.InitializationAddress());
    },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return internalQueryClient->EndInternalQuery2(context.GetRawPointer(), result.InitializationAddress());
    },
        S_OK,
        S_OK,
        S_OK,
        false);

    if (SUCCEEDED(hr))
    {
        FABRIC_QUERY_RESULT const * queryResult = result->get_Result();
        TestSession::FailTestIf(
            (!queryResult || queryResult->Value == NULL || queryResult->Kind == FABRIC_QUERY_RESULT_KIND_INVALID),
            "Query result null or invalid.");
        TestSession::WriteNoise(TraceSource, "Query succeeded.");
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "Query returned {0}", hr);
    }

    return hr;
}

bool FabricTestQueryExecutor::ExecuteQuery(StringCollection const & params)
{
    CommandLineParser parser(params);

    int count = 0;
    parser.TryGetInt(L"count", count, 1);

    map<wstring, int> queryWeights;
    if(parser.IsMap(L"queries"))
    {
        parser.GetMap(L"queries", queryWeights);
    }
    else
    {
        vector<wstring> queries;
        parser.GetVector(L"queries", queries);
        for(auto queriesIter = queries.begin(); queriesIter != queries.end(); queriesIter++) 
        {
            queryWeights[*queriesIter] = 1;
        }
    }

    if(queryWeights.size() == 0)
    {
        // Queries not provided as input from command
        auto & queryWeightsFromConfig = FabricTestSessionConfig::GetConfig().QueryWeights;
        for(auto queryHandlerIter = queryHandlers_.begin(); queryHandlerIter != queryHandlers_.end(); queryHandlerIter++) 
        {
            auto queryWeightIter = queryWeightsFromConfig.find(queryHandlerIter->first);
            if(queryWeightIter != queryWeightsFromConfig.end())
            {
                queryWeights[queryHandlerIter->first] = queryWeightIter->second;
            }
            else
            {
                queryWeights[queryHandlerIter->first] = 1;
            }
        }
    }

    map<wstring, int> queryCounts;
    for(auto queryHandlerIter = queryWeights.begin(); queryHandlerIter != queryWeights.end(); queryHandlerIter++) 
    {
        queryCounts[queryHandlerIter->first] = 0;
    }

    int sumOfWeight = 0;
    for(auto queryHandlerIter = queryWeights.begin(); queryHandlerIter != queryWeights.end(); queryHandlerIter++) 
    {
        sumOfWeight += queryHandlerIter->second;
    }

    auto queryClient = CreateFabricQueryClient(FABRICSESSION.FabricDispatcher.Federation);

    while (count > 0)
    {
        wstring queryHandlerName;
        // select any random query

        int randomNumber = random_.Next(min(sumOfWeight, 100));
        for(auto queryHandlerIter = queryWeights.begin(); queryHandlerIter != queryWeights.end(); queryHandlerIter++) 
        {
            if(randomNumber < queryHandlerIter->second)
            {
                queryHandlerName = queryHandlerIter->first;
                break;
            }
            else
            {
                randomNumber -= queryHandlerIter->second;
            }
        }

        if (queryHandlers_.find(queryHandlerName) != queryHandlers_.end())
        {
            TestSession::WriteInfo(TraceSource, "running {0}", queryHandlerName);
            bool result = queryHandlers_[queryHandlerName]->Verify(queryClient, queryHandlerName);

            TestSession::FailTestIfNot(result, "VerifyQueryResult for {0} failed. Check logs", queryHandlerName);
        }

        count--;

        queryCounts[queryHandlerName]++;
    }

    TestSession::WriteInfo(TraceSource, "Queries run");
    for(auto queryHandlerIter = queryCounts.begin(); queryHandlerIter != queryCounts.end(); queryHandlerIter++) 
    {
        TestSession::WriteInfo(TraceSource, "{0} -- Count {1}", queryHandlerIter->first, queryHandlerIter->second);
    }

    return true;
}

ComPointer<IFabricQueryClient7> FabricTestQueryExecutor::CreateFabricQueryClient(__in FabricTestFederation & testFederation)
{
    ComPointer<IFabricQueryClient7> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface(
        IID_IFabricQueryClient7, 
        (void **)result.InitializationAddress());

    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricQueryClient7");

    return result;
}
