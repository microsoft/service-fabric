
//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("GetChaosEventsDescription");

GetChaosEventsDescription::GetChaosEventsDescription()
: filterSPtr_()
, queryPagingDescriptionSPtr_()
, clientType_()
{
}

GetChaosEventsDescription::GetChaosEventsDescription(
    std::shared_ptr<ChaosEventsFilter> & filterSPtr,
    std::shared_ptr<ServiceModel::QueryPagingDescription> & queryPagingDescriptionSPtr,
    std::wstring & clientType)
: filterSPtr_(filterSPtr)
, queryPagingDescriptionSPtr_(queryPagingDescriptionSPtr)
, clientType_(clientType)
{
}

Common::ErrorCode GetChaosEventsDescription::FromPublicApi(
    FABRIC_CHAOS_EVENTS_SEGMENT_DESCRIPTION const & publicDescription)
{
    if (publicDescription.Filter != nullptr
        && DateTime(publicDescription.Filter->StartTimeUtc) > DateTime::Zero
        && (publicDescription.PagingDescription != nullptr
        && publicDescription.PagingDescription->ContinuationToken != nullptr))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, L"Only one of Filter or PagingDescription.ContinuationToken can be specified.");
    }
    else if (publicDescription.Filter == nullptr
             && (publicDescription.PagingDescription == nullptr
                 || publicDescription.PagingDescription->ContinuationToken == nullptr))
    {
        return ErrorCode(ErrorCodeValue::ArgumentNull, L"One of Filter or PagingDescription.ContinuationToken must be non-null.");
    }

    // ChaosEventsFilter
    ChaosEventsFilter filter;
    auto error = filter.FromPublicApi(*publicDescription.Filter);

    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "GetChaosEventsDescription::FromPublicAPI - filter.FromPublicApi failed, error: {0}.", error);
        return error;
    }

    filterSPtr_ = make_shared<ChaosEventsFilter>(filter.StartTimeUtc, filter.EndTimeUtc);

    // QueryPagingDescription
    ServiceModel::QueryPagingDescription pagingDescription;
    if (publicDescription.PagingDescription != nullptr)
    {
        error = pagingDescription.FromPublicApi(*(publicDescription.PagingDescription));
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceComponent, "GetChaosEventsDescription::FromPublicAPI queryPagingDescription.FromPublicApi failed, error: {0}", error);
            return error;
        }

        queryPagingDescriptionSPtr_ = make_shared<ServiceModel::QueryPagingDescription>(move(pagingDescription));
    }

    // Reserved
    if (publicDescription.Reserved != nullptr)
    {
        auto descriptionClientType = reinterpret_cast<FABRIC_CHAOS_CLIENT_TYPE*>(publicDescription.Reserved);

        if (descriptionClientType->ClientType != nullptr)
        {
            error = StringUtility::LpcwstrToWstring2(
                descriptionClientType->ClientType,
                true, /*accept nullptr*/
                0, /*IntPtr.Zero from managed translates into "\0", a string of length 0*/
                ParameterValidator::MaxStringSize,
                clientType_);

            if (!error.IsSuccess())
            {
                Trace.WriteWarning(TraceComponent, "GetChaosEventsDescription::FromPublicApi Failed to parse ClientType: {0}", error);
                return error;
            }
        }
    }
    else // native
    {
        clientType_ = L"native";
    }

    return ErrorCodeValue::Success;
}

void GetChaosEventsDescription::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENTS_SEGMENT_DESCRIPTION & result) const
{
    auto publicEventsSegmentFilterPtr = heap.AddItem<FABRIC_CHAOS_EVENTS_SEGMENT_FILTER>();
    filterSPtr_->ToPublicApi(heap, *publicEventsSegmentFilterPtr);
    result.Filter = publicEventsSegmentFilterPtr.GetRawPointer();

    auto publicQueryPagingDescription = heap.AddItem<FABRIC_QUERY_PAGING_DESCRIPTION>();

    queryPagingDescriptionSPtr_->ToPublicApi(heap, *publicQueryPagingDescription);

    result.PagingDescription  = publicQueryPagingDescription.GetRawPointer();

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