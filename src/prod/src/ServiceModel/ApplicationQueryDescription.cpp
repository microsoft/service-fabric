// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace std;
using namespace Common;
using namespace Query;

StringLiteral const TraceComponent("ApplicationQueryDescription");

ApplicationQueryDescription::ApplicationQueryDescription()
    : applicationNameFilter_(NamingUri::RootNamingUri)
    , applicationTypeNameFilter_()
    , applicationDefinitionKindFilter_(0)
    , excludeApplicationParameters_(false)
    , queryPagingDescription_()
{
}

ErrorCode ApplicationQueryDescription::FromPublicApi(FABRIC_APPLICATION_QUERY_DESCRIPTION const & applicationQueryDescription)
{
    ErrorCode err = ErrorCodeValue::Success;
    if (applicationQueryDescription.ApplicationNameFilter != nullptr)
    {
        err = NamingUri::TryParse(applicationQueryDescription.ApplicationNameFilter, "ApplicationNameFilter", applicationNameFilter_);
    }

    if (!err.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "ApplicationQueryDescription::FromPublicApi failed to parse ApplicationNameFilter = {0} error = {1}. errormessage = {2}", applicationQueryDescription.ApplicationNameFilter, err, err.Message);
        return err;
    }

    if (applicationQueryDescription.Reserved != nullptr)
    {
        auto applicationQueryDescriptionEx1 = reinterpret_cast<FABRIC_APPLICATION_QUERY_DESCRIPTION_EX1 *>(applicationQueryDescription.Reserved);
        wstring continuationToken;
        QueryPagingDescription pagingDescription;

        TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(applicationQueryDescriptionEx1->ContinuationToken, continuationToken);
        if (!continuationToken.empty())
        {
            pagingDescription.ContinuationToken = move(continuationToken);
        }

        if (applicationQueryDescriptionEx1->Reserved != nullptr)
        {
            auto applicationQueryDescriptionEx2 = reinterpret_cast<FABRIC_APPLICATION_QUERY_DESCRIPTION_EX2 *>(applicationQueryDescriptionEx1->Reserved);

            TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(applicationQueryDescriptionEx2->ApplicationTypeNameFilter, applicationTypeNameFilter_);

            excludeApplicationParameters_ = applicationQueryDescriptionEx2->ExcludeApplicationParameters ? true : false;

            if (applicationQueryDescriptionEx2->Reserved != nullptr)
            {
                auto applicationQueryDescriptionEx3 = reinterpret_cast<FABRIC_APPLICATION_QUERY_DESCRIPTION_EX3 *>(applicationQueryDescriptionEx2->Reserved);

                applicationDefinitionKindFilter_ = applicationQueryDescriptionEx3->ApplicationDefinitionKindFilter;

                if (applicationQueryDescriptionEx3->Reserved != nullptr)
                {
                    auto applicationQueryDescriptionEx4 = reinterpret_cast<FABRIC_APPLICATION_QUERY_DESCRIPTION_EX4 *>(applicationQueryDescriptionEx3->Reserved);
                    auto maxResults = applicationQueryDescriptionEx4->MaxResults;

                    if (maxResults < 0)
                    {
                        return ErrorCode(
                            ErrorCodeValue::InvalidArgument,
                            wformatString(GET_COMMON_RC(Invalid_Max_Results), maxResults));
                    }

                    pagingDescription.MaxResults = maxResults;
                }
            }
        }

        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return ErrorCodeValue::Success;
}

void ApplicationQueryDescription::GetQueryArgumentMap(__out QueryArgumentMap & argMap) const
{
    if (!applicationNameFilter_.IsRootNamingUri)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Application::ApplicationName,
            applicationNameFilter_.ToString());
    }

    if (!applicationTypeNameFilter_.empty())
    {
        argMap.Insert(
            Query::QueryResourceProperties::ApplicationType::ApplicationTypeName,
            applicationTypeNameFilter_);
    }

    if (applicationDefinitionKindFilter_ != static_cast<DWORD>(FABRIC_APPLICATION_DEFINITION_KIND_FILTER_DEFAULT))
    {
        argMap.Insert(
            Query::QueryResourceProperties::Deployment::ApplicationDefinitionKindFilter,
            wformatString(applicationDefinitionKindFilter_));
    }

    if (excludeApplicationParameters_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::QueryMetadata::ExcludeApplicationParameters,
            wformatString(excludeApplicationParameters_));
    }

    if (queryPagingDescription_ != nullptr)
    {
        queryPagingDescription_->SetQueryArguments(argMap);
    }
}

Common::ErrorCode ApplicationQueryDescription::GetDescriptionFromQueryArgumentMap(
    QueryArgumentMap const & queryArgs)
{
    // Find the ApplicationName argument if it exists.
    wstring applicationNameArg(L"");
    wstring applicationTypeNameArg(L"");
    queryArgs.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameArg);
    queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeName, applicationTypeNameArg);

    wstring applicationDefinitionKindFilterArg;
    DWORD applicationDefinitionKindFilter = static_cast<DWORD>(FABRIC_APPLICATION_DEFINITION_KIND_FILTER_DEFAULT);

    if (queryArgs.TryGetValue(QueryResourceProperties::Deployment::ApplicationDefinitionKindFilter, applicationDefinitionKindFilterArg))
    {
        if (!StringUtility::TryFromWString<DWORD>(applicationDefinitionKindFilterArg, applicationDefinitionKindFilter))
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_CM_RC(Invalid_Application_Definition_Kind_Filter));
        }
    }

    wstring trimmed; // The passed in "fabric:/applicationName" minus the "fabric:/" so that we don't end up with two "fabric:"s
    NamingUri::FabricNameToId(applicationNameArg, trimmed);

    // Filters are exclusive
    bool hasFilterSet = false;
    if (!IsExclusiveFilterHelper(!applicationNameArg.empty(), hasFilterSet)
        || !IsExclusiveFilterHelper(!applicationTypeNameArg.empty(), hasFilterSet)
        || !IsExclusiveFilterHelper(applicationDefinitionKindFilter != FABRIC_APPLICATION_DEFINITION_KIND_FILTER_DEFAULT, hasFilterSet))
    {
        Trace.WriteInfo(
            TraceComponent,
            "At most one of ApplicationName {0}, ApplicationTypeName {1}, or ApplicationDefinitionKindFilter {2} can be specified in GetApplications.",
            applicationNameArg,
            applicationTypeNameArg,
            applicationDefinitionKindFilterArg);

            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_CM_RC(Application_Filter_Dup_Defined)
            );
    }

    // ExcludeApplicationParameters
    wstring excludeApplicationParametersStr;
    bool excludeApplicationParameters = false;
    bool checkExcludeApplicationParameters = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::ExcludeApplicationParameters, excludeApplicationParametersStr);
    if (checkExcludeApplicationParameters)
    {
        StringUtility::ToLower(excludeApplicationParametersStr);
        excludeApplicationParameters = (excludeApplicationParametersStr.compare(L"true") == 0) ? true : false;
    }

    // This checks for the validity of the PagingQueryDescription as well as retrieving the data.
    QueryPagingDescription pagingDescription;
    auto error = pagingDescription.GetFromArgumentMap(queryArgs);
    if (error.IsSuccess())
    {
        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    applicationNameFilter_ = NamingUri(trimmed);
    applicationTypeNameFilter_ = applicationTypeNameArg;
    applicationDefinitionKindFilter_ = applicationDefinitionKindFilter;
    excludeApplicationParameters_ = excludeApplicationParameters;

    return error;
}

bool ApplicationQueryDescription::IsExclusiveFilterHelper(bool const isValid, bool & hasFilterSet)
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

wstring ApplicationQueryDescription::GetApplicationNameString() const
{
    if (!applicationNameFilter_.IsRootNamingUri)
    {
        return applicationNameFilter_.ToString();
    }
    else
    {
        return wstring();
    }
}
