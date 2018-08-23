// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;
using namespace ServiceModel::ModelV2;

Common::StringLiteral const TraceType("GetApplicationResourceListQuery");

// We don't override ParallelQueryExecutionTimeout in this specification because here is the only way to get application resource's health info, no need to prioritize.
GetApplicationResourceListQuerySpecification::GetApplicationResourceListQuerySpecification()
    : ParallelQuerySpecification(
        QueryNames::GetApplicationResourceList,
        QueryAddresses::GetGateway(),
        QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false))
{
    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetApplicationResourceList,
            QueryAddresses::GetCM(),
            QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetApplicationUnhealthyEvaluation,
            Query::QueryAddresses::GetHMViaCM(),
            QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
}

ErrorCode GetApplicationResourceListQuerySpecification::OnParallelQueryExecutionComplete(
    ActivityId const & activityId,
    map<QuerySpecificationSPtr, QueryResult> & queryResults,
    __out Transport::MessageUPtr & replyMessage)
{
    WriteInfo(
        TraceType,
        "{0}: GetApplicationResourceListQuery parallel query returned {1} results",
        activityId,
        queryResults.size());
    ErrorCode error;
    auto it = find_if(queryResults.begin(), queryResults.end(), [](auto const & p) { return p.first->QueryName == QueryNames::GetApplicationResourceList; });
    vector<ApplicationDescriptionQueryResult> cmResultList;

    if (it != queryResults.end())
    {
        error = it->second.MoveList<ApplicationDescriptionQueryResult>(cmResultList);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceType,
                "{0}: Error '{1}' while obtaining result from CM during merge",
                activityId,
                error);
            return error;
        }
    }

    it = find_if(queryResults.begin(), queryResults.end(), [](auto const & p) { return p.first->QueryName == QueryNames::GetApplicationUnhealthyEvaluation; });
    vector<ApplicationUnhealthyEvaluation> hmResultList;
    if (it != queryResults.end())
    {
        error = it->second.MoveList<ApplicationUnhealthyEvaluation>(hmResultList);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceType,
                "{0}: Error '{1}' while obtaining result from HM during merge",
                activityId,
                error);
        }
    }

    for (ApplicationDescriptionQueryResult & result : cmResultList)
    {
        wstring applicationUri = NamingUri(result.ApplicationName).ToString();
        auto hmIter = find_if(hmResultList.begin(), hmResultList.end(), [&applicationUri](auto const & app) { return app.ApplicationName == applicationUri; });
        if (hmIter == hmResultList.end())
        {
            WriteInfo(
                TraceType,
                "{0} Application resource {1} has no HM information.",
                activityId,
                result.ApplicationName);
            continue;
        }

        result.HealthState = hmIter->AggregatedHealthState;
        wstring unhealthyEvals = HealthEvaluation::GetUnhealthyEvaluationDescription(hmIter->UnhealthyEvaluations);
        if (!unhealthyEvals.empty())
        {
            WriteNoise(
                TraceType,
                "{0} Application resource {1} has state {2}, unhealthy evaluations: {3}.",
                activityId,
                result.ApplicationName,
                result.HealthState,
                unhealthyEvals);
        }

        result.UnhealthyEvaluation = move(unhealthyEvals);
    }

    replyMessage = make_unique<Transport::Message>(QueryResult(move(cmResultList)));
    return ErrorCode::Success();
}
