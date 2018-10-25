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

StringLiteral const TraceComponent("ChaosReport");

ChaosReport::ChaosReport()
    : chaosParametersUPtr_()
    , status_()
    , continuationToken_(L"")
    , history_()
{
}

ChaosReport::ChaosReport(ChaosReport && other)
    : chaosParametersUPtr_(move(other.chaosParametersUPtr_))
    , status_(move(other.status_))
    , continuationToken_(move(other.continuationToken_))
    , history_(move(other.history_))
{
}

ErrorCode ChaosReport::FromPublicApi(
    FABRIC_CHAOS_REPORT const & publicReport)
{
    ChaosParameters chaosParameters;
    auto error = chaosParameters.FromPublicApi(*publicReport.ChaosParameters);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "ChaosReport::FromPublicApi/Failed at chaosParameters.FromPublicApi");
        return error;
    }

    chaosParametersUPtr_ = make_unique<ChaosParameters>(move(chaosParameters));

    error = ChaosStatus::FromPublicApi(publicReport.Status, status_);

    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "ChaosStatus::FromPublicApi failed, error: {0}", error);
        return error;
    }

    HRESULT hr = StringUtility::LpcwstrToWstring(
        publicReport.ContinuationToken,
        true, /*accept null*/
        0,
        ParameterValidator::MaxStringSize,
        continuationToken_);

    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "ChaosReport::FromPublicApi/Failed to parse ContinuationToken: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    error = PublicApiHelper::FromPublicApiList<ChaosEvent, FABRIC_CHAOS_EVENT_LIST>(
        publicReport.History,
        history_);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "ChaosReport::FromPublicApi/Failed to retrieve the chaos event list: {0}", error);
        return error;
    }

    return ErrorCodeValue::Success;
}

ErrorCode ChaosReport::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_REPORT & result) const
{
    auto publicChaosParametersPtr = heap.AddItem<FABRIC_CHAOS_PARAMETERS>();
    chaosParametersUPtr_->ToPublicApi(heap, *publicChaosParametersPtr);
    result.ChaosParameters = publicChaosParametersPtr.GetRawPointer();

    result.Status = ChaosStatus::ToPublicApi(status_);

    result.ContinuationToken = heap.AddString(continuationToken_);

    // Start of ToPublic of vector of chaos events
    auto publicChaosEventList = heap.AddItem<FABRIC_CHAOS_EVENT_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<ChaosEvent, FABRIC_CHAOS_EVENT, FABRIC_CHAOS_EVENT_LIST>(
        heap,
        history_,
        *publicChaosEventList);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "ChaosReport::ToPublicApi/Failed to publish the list of Chaos events with error: {0}", error);
        return error;
    }

    result.History = publicChaosEventList.GetRawPointer();

    return ErrorCodeValue::Success;
}
