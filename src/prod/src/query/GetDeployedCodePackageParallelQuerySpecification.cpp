//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

Common::StringLiteral const DeployedCodePackageParallelQueryTraceType("DeployedCodePackageParallelQuery");

GetDeployedCodePackageParallelQuerySpecification::GetDeployedCodePackageParallelQuerySpecification(
    wstring const & applicationNameString,
    vector<wstring> const & nodeNames)
    : ParallelQuerySpecification(
        QueryNames::Enum::GetDeployedCodePackageListByApplication,
        Query::QueryAddresses::GetGateway(),
        QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
        QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceManifestName, false),
        QueryArgument(Query::QueryResourceProperties::CodePackage::CodePackageName, false))
    , queryArgumentMaps_()
{
    for (auto const & nodeName : nodeNames)
    {
        QueryArgumentMap argMap;
        argMap.Insert(
            Query::QueryResourceProperties::Application::ApplicationName,
            applicationNameString);
        argMap.Insert(
            Query::QueryResourceProperties::Node::Name,
            nodeName);

        QuerySpecification spec = *(QuerySpecificationStore::Get().GetSpecification(
            QueryNames::Enum::GetCodePackageListDeployedOnNode,
            argMap));
        // QuerySpecificationId wouldn't be used anymore, use it to identify node name
        spec.QuerySpecificationId = nodeName;

        ParallelQuerySpecifications.push_back(make_shared<QuerySpecification>(spec));
        queryArgumentMaps_.push_back(argMap);
    }
}

ErrorCode GetDeployedCodePackageParallelQuerySpecification::OnParallelQueryExecutionComplete(
    ActivityId const & activityId,
    map<QuerySpecificationSPtr, QueryResult> & queryResults,
    __out Transport::MessageUPtr & replyMessage)
{
    WriteInfo(
        DeployedCodePackageParallelQueryTraceType,
        "{0}: deployed code package parallel query returned {1} results",
        activityId,
        queryResults.size());

    vector<DeployedCodePackageQueryResult> resultList;
    for (auto itResult = queryResults.begin(); itResult != queryResults.end(); ++itResult)
    {
        ErrorCode error = this->MergeDeployedCodePackageResults(resultList, itResult->second);
        if (!error.IsSuccess())
        {
            WriteInfo(
                DeployedCodePackageParallelQueryTraceType,
                "{0} error {1} while merging entity info from sub-query, AddressString {2}",
                activityId,
                error,
                itResult->first->AddressString);
            return error;
        }
    }

    // TODO: Consider not return message since it is not used.
    replyMessage = make_unique<Transport::Message>(QueryResult(move(resultList)));
    return ErrorCode::Success();
}

// Ignore queryProcessingError. This is just the last step of CGS instance sequential query.
ErrorCode GetDeployedCodePackageParallelQuerySpecification::MergeDeployedCodePackageResults(
    __inout vector<DeployedCodePackageQueryResult> & mergedQueryResult,
    __in QueryResult & results)
{
    if (mergedQueryResult.size() == 0)
    {
        results.MoveList(mergedQueryResult);
    }
    else
    {
        vector<DeployedCodePackageQueryResult> entityQueryResult;
        auto error = results.MoveList(entityQueryResult);
        if (error.IsSuccess())
        {
            mergedQueryResult.insert(
                mergedQueryResult.end(),
                make_move_iterator(entityQueryResult.begin()),
                make_move_iterator(entityQueryResult.end()));
        }
    }
    return ErrorCode::Success();
}

QueryArgumentMap GetDeployedCodePackageParallelQuerySpecification::GetQueryArgumentMap(
    size_t index,
    QueryArgumentMap const &)
{
    if (index >= queryArgumentMaps_.size())
    {
        Assert::CodingError("index {0} excceeds the query argument maps size {1}", index, queryArgumentMaps_.size());
    }

    return queryArgumentMaps_[index];
}
