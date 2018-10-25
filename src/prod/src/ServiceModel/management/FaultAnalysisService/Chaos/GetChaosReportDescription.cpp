// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("GetChaosReportDescription");

GetChaosReportDescription::GetChaosReportDescription()
: filterSPtr_()
, continuationToken_()
, clientType_()
{
}

GetChaosReportDescription::GetChaosReportDescription(
std::shared_ptr<ChaosReportFilter> & filter,
std::wstring & token,
std::wstring & clientType)
: filterSPtr_(filter)
, continuationToken_(token)
, clientType_(clientType)
{
}

GetChaosReportDescription::GetChaosReportDescription(GetChaosReportDescription const & other)
: filterSPtr_(other.filterSPtr_)
, continuationToken_(other.continuationToken_)
, clientType_(other.clientType_)
{
}

GetChaosReportDescription::GetChaosReportDescription(GetChaosReportDescription && other)
: filterSPtr_(move(other.filterSPtr_))
, continuationToken_(move(other.continuationToken_))
, clientType_(move(other.clientType_))
{
}

// This never gets called when client is REST
Common::ErrorCode GetChaosReportDescription::FromPublicApi(
    FABRIC_GET_CHAOS_REPORT_DESCRIPTION const & publicDescription)
{
    ChaosReportFilter filter;
    auto error = filter.FromPublicApi(*publicDescription.Filter);

    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "filter.FromPublicApi failed, error: {0}.", error);
        return error;
    }

    filterSPtr_ = make_shared<ChaosReportFilter>(filter);

    HRESULT hr = StringUtility::LpcwstrToWstring(
        publicDescription.ContinuationToken,
        true, /*accept nullptr*/
        0, /*IntPtr.Zero from managed translates into "\0", a string of length 0*/
        ParameterValidator::MaxStringSize,
        continuationToken_);

    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "GetChaosReportDescription::FromPublicApi/Failed to parse ContinuationToken: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    if (publicDescription.Reserved != nullptr) // managed
    {
        auto descriptionClientType = reinterpret_cast<FABRIC_CHAOS_CLIENT_TYPE*>(publicDescription.Reserved);

        if (descriptionClientType->ClientType != nullptr)
        {
            hr = StringUtility::LpcwstrToWstring(
                descriptionClientType->ClientType,
                true, /*accept nullptr*/
                0, /*IntPtr.Zero from managed translates into "\0", a string of length 0*/
                ParameterValidator::MaxStringSize,
                clientType_);

            if (FAILED(hr))
            {
                Trace.WriteWarning(TraceComponent, "GetChaosReportDescription::FromPublicApi/Failed to parse ClientType: {0}", hr);
                return ErrorCode::FromHResult(hr);
            }
        }
    }
    else // native
    {
        clientType_ = L"native";
    }

    return ErrorCodeValue::Success;
}

void GetChaosReportDescription::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_GET_CHAOS_REPORT_DESCRIPTION & result) const
{
    auto publicReportFilterPtr = heap.AddItem<FABRIC_CHAOS_REPORT_FILTER>();
    filterSPtr_->ToPublicApi(heap, *publicReportFilterPtr);
    result.Filter = publicReportFilterPtr.GetRawPointer();

    result.ContinuationToken = heap.AddString(continuationToken_);

    auto clientTypeEx1 = heap.AddItem<FABRIC_CHAOS_CLIENT_TYPE>();

    if (!clientType_.empty())
    {
        clientTypeEx1->ClientType = heap.AddString(clientType_);
    }
    else
    {
        clientTypeEx1->ClientType = nullptr;
    }

    result.Reserved = clientTypeEx1.GetRawPointer();
}
