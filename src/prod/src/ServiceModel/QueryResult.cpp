// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Serialization;
using namespace ServiceModel;

// For serializer
QueryResult::QueryResult()
    : resultKind_(FABRIC_QUERY_RESULT_KIND_INVALID)
    , resultWrapper_()
    , queryProcessingError_(ErrorCodeValue::Success)
    , pagingStatus_()
{
}

QueryResult::QueryResult(Common::ErrorCode const & queryProcessingError)
    : resultKind_(FABRIC_QUERY_RESULT_KIND_INVALID)
    , resultWrapper_()
    , queryProcessingError_(queryProcessingError)
    , pagingStatus_()
{
}

QueryResult::QueryResult(Common::ErrorCode && queryProcessingError)
    : resultKind_(FABRIC_QUERY_RESULT_KIND_INVALID)
    , resultWrapper_()
    , queryProcessingError_(move(queryProcessingError))
    , pagingStatus_()
{
}

QueryResult::QueryResult(Common::ErrorCodeValue::Enum queryProcessingError)
    : resultKind_(FABRIC_QUERY_RESULT_KIND_INVALID)
    , resultWrapper_()
    , queryProcessingError_(queryProcessingError)
    , pagingStatus_()
{
}

QueryResult::QueryResult(QueryResult && other)
    : resultKind_(other.resultKind_)
    , resultWrapper_(move(other.resultWrapper_))
    , queryProcessingError_(move(other.queryProcessingError_))
    , pagingStatus_(move(other.pagingStatus_))
{
}

QueryResult & QueryResult::operator = (QueryResult && other)
{
    if (&other != this)
    {
        resultKind_ = other.resultKind_;
        resultWrapper_ = move(other.resultWrapper_);
        queryProcessingError_ = move(other.queryProcessingError_);
        pagingStatus_ = move(other.pagingStatus_);
    }

    return *this;
}

QueryResult::~QueryResult()
{
}

void QueryResult::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_QUERY_RESULT & queryResult)
{
    queryResult.Kind = resultKind_;
   
    if (resultKind_ == FABRIC_QUERY_RESULT_KIND_ITEM)
    {
        auto item = heap.AddItem<FABRIC_QUERY_RESULT_ITEM>();
        resultWrapper_->ToPublicApi(heap, *item);
        queryResult.Value = item.GetRawPointer();
    }
    else if(resultKind_ == FABRIC_QUERY_RESULT_KIND_LIST)
    {
        auto listItem = heap.AddItem<FABRIC_QUERY_RESULT_LIST>();
        resultWrapper_->ToPublicApi(heap, *listItem);
        queryResult.Value = listItem.GetRawPointer();
    }
    else
    {
        queryResult.Value = NULL;
    }
}
