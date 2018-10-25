// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ApplicationQueryResult)

ApplicationQueryResult::ApplicationQueryResult()
    : applicationName_()
    , applicationTypeName_()
    , applicationTypeVersion_()
    , applicationStatus_(ApplicationStatus::Invalid)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , applicationParameters_()
    , applicationDefinitionKind_(FABRIC_APPLICATION_DEFINITION_KIND_INVALID)
    , upgradeTypeVersion_()
    , upgradeParameters_()
{
}

ApplicationQueryResult::ApplicationQueryResult(
    Common::Uri const & applicationName, 
    std::wstring const & applicationTypeName, 
    std::wstring const & applicationTypeVersion,
    ApplicationStatus::Enum applicationStatus,
    std::map<std::wstring, std::wstring> const & applicationParameters,
    FABRIC_APPLICATION_DEFINITION_KIND const applicationDefinitionKind,
    std::wstring const & upgradeTypeVersion, 
    std::map<std::wstring, std::wstring> const & upgradeParameters)
    : applicationName_(applicationName)
    , applicationTypeName_(applicationTypeName)
    , applicationTypeVersion_(applicationTypeVersion)
    , applicationStatus_(applicationStatus)
    , applicationParameters_(applicationParameters)
    , applicationDefinitionKind_(applicationDefinitionKind)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , upgradeTypeVersion_(upgradeTypeVersion)
    , upgradeParameters_(upgradeParameters)
{
}

void ApplicationQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_APPLICATION_QUERY_RESULT_ITEM & publicApplicationQueryResult) const 
{
    publicApplicationQueryResult.ApplicationName = heap.AddString(applicationName_.ToString());
    publicApplicationQueryResult.ApplicationTypeName = heap.AddString(applicationTypeName_);
    publicApplicationQueryResult.ApplicationTypeVersion = heap.AddString(applicationTypeVersion_);
    publicApplicationQueryResult.Status = ApplicationStatus::ConvertToPublicApplicationStatus(applicationStatus_);
    auto applicationParameters = heap.AddItem<::FABRIC_APPLICATION_PARAMETER_LIST>();
    applicationParameters->Count = static_cast<ULONG>(applicationParameters_.size());
    auto applicationParametersArray = heap.AddArray<FABRIC_APPLICATION_PARAMETER>(applicationParameters->Count);

    int i = 0;
    for (auto itParameterItem = applicationParameters_.begin(); itParameterItem != applicationParameters_.end(); ++itParameterItem)
    {
        applicationParametersArray[i].Name = heap.AddString(itParameterItem->first);
        applicationParametersArray[i].Value = heap.AddString(itParameterItem->second);
        i++;
    }
    applicationParameters->Items = applicationParametersArray.GetRawArray();
    publicApplicationQueryResult.ApplicationParameters = applicationParameters.GetRawPointer();
    publicApplicationQueryResult.HealthState = healthState_;

    // Ex1 is a deprecated struct
    auto queryResultEx1 = heap.AddItem<FABRIC_APPLICATION_QUERY_RESULT_ITEM_EX1>();
    if (applicationStatus_ == ApplicationStatus::Upgrading)
    {
        queryResultEx1->UpgradeTypeVersion = heap.AddString(upgradeTypeVersion_);

        auto upgradeParametersEx1 = heap.AddItem<FABRIC_APPLICATION_PARAMETER_LIST>();
        upgradeParametersEx1->Count = static_cast<ULONG>(upgradeParameters_.size());

        auto upgradeParametersArrayEx1 = heap.AddArray<FABRIC_APPLICATION_PARAMETER>(upgradeParameters_.size());

        i = 0;
        for (auto it = upgradeParameters_.begin(); it != upgradeParameters_.end(); ++it)
        {
            upgradeParametersArrayEx1[i].Name = heap.AddString(it->first);
            upgradeParametersArrayEx1[i].Value = heap.AddString(it->second);
            i++;
        }

        upgradeParametersEx1->Items = upgradeParametersArrayEx1.GetRawArray();

        queryResultEx1->UpgradeParameters = upgradeParametersEx1.GetRawPointer();
    }

    auto queryResultEx2 = heap.AddItem<FABRIC_APPLICATION_QUERY_RESULT_ITEM_EX2>();
    queryResultEx2->ApplicationDefinitionKind = applicationDefinitionKind_;

    queryResultEx1->Reserved = queryResultEx2.GetRawPointer();
    publicApplicationQueryResult.Reserved = queryResultEx1.GetRawPointer();
}

ErrorCode ApplicationQueryResult::FromPublicApi(__in FABRIC_APPLICATION_QUERY_RESULT_ITEM const& publicApplicationQueryResult)
{
    if (!Uri::TryParse(publicApplicationQueryResult.ApplicationName, applicationName_))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    auto hr = StringUtility::LpcwstrToWstring(publicApplicationQueryResult.ApplicationTypeName, false, applicationTypeName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    hr = StringUtility::LpcwstrToWstring(publicApplicationQueryResult.ApplicationTypeVersion, false, applicationTypeVersion_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    applicationStatus_ = ApplicationStatus::ConvertToServiceModelApplicationStatus(publicApplicationQueryResult.Status);

    if (publicApplicationQueryResult.ApplicationParameters != NULL)
    {
        for(ULONG i = 0; i < publicApplicationQueryResult.ApplicationParameters->Count; i++)
        {
            wstring name;
            hr = StringUtility::LpcwstrToWstring(publicApplicationQueryResult.ApplicationParameters->Items[i].Name, false, name);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

            wstring value;
            hr = StringUtility::LpcwstrToWstring(publicApplicationQueryResult.ApplicationParameters->Items[i].Value, false, value);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

            applicationParameters_.insert(make_pair(name, value));
        }
    }

    healthState_ = publicApplicationQueryResult.HealthState;

    if (publicApplicationQueryResult.Reserved != nullptr)
    {
        auto queryResultEx1 = reinterpret_cast<FABRIC_APPLICATION_QUERY_RESULT_ITEM_EX1*>(publicApplicationQueryResult.Reserved);

        if (applicationStatus_ == ApplicationStatus::Upgrading)
        {
            // May have returned empty upgrade values during original query. Allow these on round-trip.
            //
            hr = StringUtility::LpcwstrToWstring(queryResultEx1->UpgradeTypeVersion, true, upgradeTypeVersion_);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

            for (ULONG i = 0; i < queryResultEx1->UpgradeParameters->Count; ++i)
            {
                wstring paramName;
                hr = StringUtility::LpcwstrToWstring(queryResultEx1->UpgradeParameters[i].Items->Name, false, paramName);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

                wstring paramValue;
                hr = StringUtility::LpcwstrToWstring(queryResultEx1->UpgradeParameters[i].Items->Value, true, paramValue);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

                upgradeParameters_.insert(make_pair(paramName, paramValue));
            }
        }

        if (queryResultEx1->Reserved != nullptr)
        {
            auto queryResultEx2 = reinterpret_cast<FABRIC_APPLICATION_QUERY_RESULT_ITEM_EX2 *>(queryResultEx1->Reserved);
            applicationDefinitionKind_ = queryResultEx2->ApplicationDefinitionKind;
        }
    }

    return ErrorCode::Success();
}

void ApplicationQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring ApplicationQueryResult::ToString() const
{
    wstring applicationParameters = L"";
    for (auto itApplicationParameter = applicationParameters_.begin(); itApplicationParameter != applicationParameters_.end(); ++ itApplicationParameter)
    {
        applicationParameters.append(wformatString("{0}={1};", itApplicationParameter->first, itApplicationParameter->second));
    }

    wstring upgradeParameters = L"";
    for (auto it = upgradeParameters_.begin(); it != upgradeParameters_.end(); ++it)
    {
        upgradeParameters.append(wformatString("{0}={1};", it->first, it->second));
    }

    return wformatString("ApplicationName = '{0}', ApplicationTypeName = '{1}', ApplicationTypeVersion = '{2}', ApplicationStatus = '{3}', ApplicationDefinitionKind = '{4}' HealthState = '{5}' ApplicationParameters = '{6}' UpgradeTypeVersion = '{7}' UpgradeParameters = '{8}'",
        applicationName_,
        applicationTypeName_,
        applicationTypeVersion_,
        ApplicationStatus::ToString(applicationStatus_),
        Management::ClusterManager::ApplicationDefinitionKind::Enum(applicationDefinitionKind_),
        healthState_,
        applicationParameters,
        upgradeTypeVersion_,
        upgradeParameters);
}

std::wstring ApplicationQueryResult::CreateContinuationToken() const
{
    return PagingStatus::CreateContinuationToken<Uri>(applicationName_);
}
