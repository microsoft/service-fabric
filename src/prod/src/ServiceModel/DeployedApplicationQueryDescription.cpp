// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedApplicationQueryDescription");

DeployedApplicationQueryDescription::DeployedApplicationQueryDescription()
    : nodeName_()
    , applicationName_()
    , includeHealthState_(false)
    , queryPagingDescription_()
{
}

Common::ErrorCode ServiceModel::DeployedApplicationQueryDescription::FromPublicApi(FABRIC_PAGED_DEPLOYED_APPLICATION_QUERY_DESCRIPTION const & deployedApplicationQueryDescription)
{
    TRY_PARSE_PUBLIC_STRING(deployedApplicationQueryDescription.NodeName, nodeName_);

    if (deployedApplicationQueryDescription.ApplicationNameFilter != NULL)
    {
        auto error = NamingUri::TryParse(
            deployedApplicationQueryDescription.ApplicationNameFilter,
            "ApplicationName", // This value following ApplicationHealthQueryDescription
            applicationName_);

        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "DeployedApplicationQueryDescription::FromPublicApi error: {0}", error);
            return error;
        }
    }

    includeHealthState_ = deployedApplicationQueryDescription.IncludeHealthState ? true : false;

    // QueryPagingDescription
    if (deployedApplicationQueryDescription.PagingDescription != NULL)
    {
        QueryPagingDescription pagingDescription;

        // deployedApplicationQueryDescription.PagingDescription needs the * in front because PagingDescription is stored as a unique pointer
        auto error = pagingDescription.FromPublicApi(*(deployedApplicationQueryDescription.PagingDescription));
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "PagingDescription::FromPublicApi error: {0}", error);
            return error;
        }

        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return ErrorCode::Success();
}

Common::ErrorCode DeployedApplicationQueryDescription::GetDescriptionFromQueryArgumentMap(
    QueryArgumentMap const & argMap)
{
    wstring nodeId;
    bool nodeIdProvided = argMap.TryGetValue(Query::QueryResourceProperties::Node::Name, nodeId);
    if (nodeIdProvided)
    {
        nodeName_ = move(nodeId);
    }
    else
    {
        // Fail because this is a required parameter
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_QUERY_RC(Query_Missing_Required_Argument),
                "GetDeployedApplication",
                "NodeName")
        );
    }

    // We don't worry about this string being blank because it is initialized and we check for empty later.
    wstring applicationNameArgument;
    bool appNameProvided = argMap.TryGetValue(Query::QueryResourceProperties::Application::ApplicationName, applicationNameArgument);
    if (appNameProvided)
    {
        wstring trimmed; // The passed in "fabric:/applicationName" minus the "fabric:/" so that we don't end up with two "fabric:"s
        NamingUri::FabricNameToId(applicationNameArgument, trimmed);
        applicationName_ = NamingUri(trimmed);
    }

    bool includeHealthState;
    auto error = argMap.GetBool(Query::QueryResourceProperties::QueryMetadata::IncludeHealthState, includeHealthState);
    if (error.IsSuccess())
    {
        includeHealthState_ = includeHealthState;
    }
    else if (error.IsError(ErrorCodeValue::InvalidArgument))
    {
        wstring includeHealthStateString;
        argMap.TryGetValue(Query::QueryResourceProperties::QueryMetadata::IncludeHealthState, includeHealthStateString);

        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_COMMON_RC(Invalid_String_Boolean_Value),
                "IncludeHealthState",
                includeHealthStateString));
    }

    // This checks for the validity of the PagingQueryDescription as well as retrieving the data.
    QueryPagingDescription pagingDescription;
    error = pagingDescription.GetFromArgumentMap(argMap);
    if (error.IsSuccess())
    {
        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return error;
}

void DeployedApplicationQueryDescription::GetQueryArgumentMap(__out QueryArgumentMap & argMap) const
{
    // required parameter
    argMap.Insert(
        Query::QueryResourceProperties::Node::Name,
        nodeName_);

    wstring applicationNameString = move(GetApplicationNameString());
    if (!applicationNameString.empty())
    {
        argMap.Insert(
            Query::QueryResourceProperties::Application::ApplicationName,
            applicationNameString);
    }

    // always has a value
    argMap.Insert(
        Query::QueryResourceProperties::QueryMetadata::IncludeHealthState,
        wformatString(includeHealthState_));

    if (queryPagingDescription_ != nullptr)
    {
        queryPagingDescription_->SetQueryArguments(argMap);
    }
}

wstring DeployedApplicationQueryDescription::GetApplicationNameString() const
{
    if (!applicationName_.IsRootNamingUri)
    {
        return applicationName_.ToString();
    }
    else
    {
        return wstring();
    }
}

bool DeployedApplicationQueryDescription::IsExclusiveFilterHelper(bool const isValid, bool & hasFilterSet)
{
    if (!isValid) { return true; }

    if (hasFilterSet)
    {
        return false;
    }
    else
    {
        hasFilterSet = true;
        return true;
    }
}
