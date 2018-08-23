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

StringLiteral const TraceComponent("ChaosSchedule");

ChaosSchedule::ChaosSchedule()
    : startDate_()
    , expiryDate_()
    , chaosParametersMap_()
    , jobs_()
{
}

ErrorCode ChaosSchedule::FromPublicApi(
    FABRIC_CHAOS_SCHEDULE const & publicSchedule)
{
    startDate_ = DateTime(publicSchedule.StartDate);
    expiryDate_ = DateTime(publicSchedule.ExpiryDate);
    ErrorCode error = ErrorCodeValue::Success;
    for (size_t i = 0; i < publicSchedule.ChaosParametersMap->Count; ++i)
    {
        auto publicParametersMapItem = publicSchedule.ChaosParametersMap->Items[i];

        if (publicParametersMapItem.Name == nullptr || publicParametersMapItem.Parameters == nullptr)
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().ChaosContextNullKeyOrValue));
        }

        // Key
        wstring key;
        error = StringUtility::LpcwstrToWstring2(
            publicParametersMapItem.Name,
            true,
            0,
            ParameterValidator::MaxStringSize,
            key);
        if (!error.IsSuccess())
        {
            Trace.WriteError(TraceComponent, "ChaosSchedule.ChaosParametersMap::FromPublicApi/Failed to parse key: {0}", error);
            return error;
        }

        // Value
        ChaosParameters chaosParameters;
        chaosParameters.FromPublicApi(*(publicParametersMapItem.Parameters));
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "ChaosSchedule.ChaosParametersMap::FromPublicApi/Failed at chaosParameters.FromPublicApi");
            return error;
        }

        chaosParametersMap_.insert(make_pair(key, move(chaosParameters)));
    }

    error = PublicApiHelper::FromPublicApiList<ChaosScheduleJob, FABRIC_CHAOS_SCHEDULE_JOB_LIST>(
        publicSchedule.Jobs,
        jobs_);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "ChaosSchedule::FromPublicApi/Failed to retrieve the chaos schedule jobs list: {0}", error);
        return error;
    }

    return ErrorCodeValue::Success;
}

ErrorCode ChaosSchedule::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_SCHEDULE & result) const
{
    result.StartDate = startDate_.AsFileTime;
    result.ExpiryDate = expiryDate_.AsFileTime;

    auto publicChaosParametersMap = heap.AddItem<FABRIC_CHAOS_SCHEDULE_CHAOS_PARAMETERS_MAP>();
    publicChaosParametersMap->Items = 0;

    if (chaosParametersMap_.size() > 0)
    {
        auto publicParametersArray = heap.AddArray<FABRIC_CHAOS_SCHEDULE_CHAOS_PARAMETERS_MAP_ITEM>(chaosParametersMap_.size());

        size_t i = 0;
        for (auto itr = chaosParametersMap_.begin(); itr != chaosParametersMap_.end(); itr++)
        {
            publicParametersArray[i].Name = heap.AddString(itr->first);

            auto publicChaosParametersPtr = heap.AddItem<FABRIC_CHAOS_PARAMETERS>();
            itr->second.ToPublicApi(heap, *publicChaosParametersPtr);
            publicParametersArray[i].Parameters = publicChaosParametersPtr.GetRawPointer();

            ++i;
        }

        publicChaosParametersMap->Count = static_cast<ULONG>(chaosParametersMap_.size());
        publicChaosParametersMap->Items = publicParametersArray.GetRawArray();
    }

    result.ChaosParametersMap = publicChaosParametersMap.GetRawPointer();

    auto publicJobsList = heap.AddItem<FABRIC_CHAOS_SCHEDULE_JOB_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<ChaosScheduleJob, FABRIC_CHAOS_SCHEDULE_JOB, FABRIC_CHAOS_SCHEDULE_JOB_LIST>(
        heap,
        jobs_,
        *publicJobsList);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "ChaosSchedule::ToPublicApi/Failed to publish the list of Chaos schedule jobs with error: {0}", error);
        return error;
    }

    result.Jobs = publicJobsList.GetRawPointer();

    return ErrorCodeValue::Success;
}