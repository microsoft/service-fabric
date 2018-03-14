// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ApplicationTypeQueryResult)

StringLiteral const TraceSource("ApplicationTypeQueryResult");

ApplicationTypeQueryResult::ApplicationTypeQueryResult()
    : applicationTypeName_()
    , applicationTypeVersion_()
    , parameterList_()
    , status_(ApplicationTypeStatus::Invalid)
    , statusDetails_()
    , applicationTypeDefinitionKind_(FABRIC_APPLICATION_TYPE_DEFINITION_KIND_INVALID)
{
}

ApplicationTypeQueryResult::ApplicationTypeQueryResult(
    wstring const & applicationTypeName,
    wstring const & applicationTypeVersion,
    map<wstring, wstring> const & defaultParameters,
    ApplicationTypeStatus::Enum status,
    wstring const & statusDetails)
    : applicationTypeName_(applicationTypeName)
    , applicationTypeVersion_(applicationTypeVersion)
    , parameterList_(defaultParameters)
    , status_(status)
    , statusDetails_(statusDetails)
    , applicationTypeDefinitionKind_(FABRIC_APPLICATION_TYPE_DEFINITION_KIND_INVALID)
{
}

ApplicationTypeQueryResult::ApplicationTypeQueryResult(
    wstring const & applicationTypeName, 
    wstring const & applicationTypeVersion,
    map<wstring, wstring> const & defaultParameters,
    ApplicationTypeStatus::Enum status,
    wstring const & statusDetails,
    FABRIC_APPLICATION_TYPE_DEFINITION_KIND const applicationTypeDefinitionKind)
    : applicationTypeName_(applicationTypeName)
    , applicationTypeVersion_(applicationTypeVersion)
    , parameterList_(defaultParameters)
    , status_(status)
    , statusDetails_(statusDetails)
    , applicationTypeDefinitionKind_(applicationTypeDefinitionKind)
{
}

void ApplicationTypeQueryResult::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM & publicResult) const
{
    // ApplicationTypeName
    publicResult.ApplicationTypeName = heap.AddString(applicationTypeName_);

    // ApplicationTypeVersion
    publicResult.ApplicationTypeVersion = heap.AddString(applicationTypeVersion_);

    // Default parameters
    auto applicationParameters = heap.AddItem<FABRIC_APPLICATION_PARAMETER_LIST>();
    applicationParameters->Count = static_cast<ULONG>(parameterList_.size());
    auto applicationParametersArray = heap.AddArray<FABRIC_APPLICATION_PARAMETER>(parameterList_.size());

    int i = 0;
    for (auto itr = parameterList_.begin(); itr != parameterList_.end(); ++itr)
    {
        applicationParametersArray[i].Name = heap.AddString(itr->first);
        applicationParametersArray[i].Value = heap.AddString(itr->second);
        i++;
    }

    applicationParameters->Items = applicationParametersArray.GetRawArray();
    publicResult.DefaultParameters = applicationParameters.GetRawPointer();

    auto ex1 = heap.AddItem<FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_EX1>();
    ex1->Status = ApplicationTypeStatus::ToPublicApi(status_);

    if (!statusDetails_.empty())
    {
        ex1->StatusDetails = heap.AddString(statusDetails_);
    }

    auto ex2 = heap.AddItem<FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_EX2>();
    ex2->ApplicationTypeDefinitionKind = applicationTypeDefinitionKind_;

    ex1->Reserved = ex2.GetRawPointer();
    publicResult.Reserved = ex1.GetRawPointer();
}

void ApplicationTypeQueryResult::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ApplicationTypeQueryResult::ToString() const
{
    wstring applicationParameters = L"";
    for (auto itApplicationParameter = parameterList_.begin(); itApplicationParameter != parameterList_.end(); ++ itApplicationParameter)
    {
        applicationParameters.append(wformatString("{0}={1};", itApplicationParameter->first, itApplicationParameter->second));
    }

    return wformatString(
        "name='{0}' version='{1}' definitionkind='{2}' parameters='{3}' status={4} details='{5}'", 
        applicationTypeName_, 
        applicationTypeVersion_, 
        Management::ClusterManager::ApplicationTypeDefinitionKind::Enum(applicationTypeDefinitionKind_),
        applicationParameters,
        status_,
        statusDetails_);
}

// Continuation Token should be returned as a string representation of a Json object of type ApplicationTypeQueryContinuationToken (URL encoded)
std::wstring ApplicationTypeQueryResult::CreateContinuationToken() const
{
    auto tokenObject = ApplicationTypeQueryContinuationToken(applicationTypeName_, applicationTypeVersion_);
    wstring jsonContinuationToken;
    auto error = JsonHelper::Serialize<ApplicationTypeQueryContinuationToken>(tokenObject, jsonContinuationToken);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceSource, "Unable to create the continuation token '{0}' because serializing as json object failed.", jsonContinuationToken);

        return Constants::ContinuationTokenCreationFailedMessage;
    }

    wstring urlEncodedContinuationToken;
    error = Common::NamingUri::UrlEscapeString(jsonContinuationToken, urlEncodedContinuationToken);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceSource, "Unable to create the continuation token '{0}' because UrlEscapeString failed.", jsonContinuationToken);

        return Constants::ContinuationTokenCreationFailedMessage;
    }

    return urlEncodedContinuationToken;
}

