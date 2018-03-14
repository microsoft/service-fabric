// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

KeyValueStoreQueryResult::KeyValueStoreQueryResult()
    : ReplicaStatusQueryResult(FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE)
    , dbRowCountEstimate_(0)
    , dbLogicalSizeEstimate_(0)
    , copyNotificationCurrentFilter_()
    , copyNotificationCurrentProgress_(0)
    , statusDetails_()
{
}

KeyValueStoreQueryResult::KeyValueStoreQueryResult(
    size_t dbRowCountEstimate,
    size_t dbLogicalSizeEstimate,
    shared_ptr<wstring> copyNotificationCurrentFilter,
    size_t copyNotificationCurrentProgress,
    wstring const & statusDetails)
    : ReplicaStatusQueryResult(FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE)
    , dbRowCountEstimate_(dbRowCountEstimate)
    , dbLogicalSizeEstimate_(dbLogicalSizeEstimate)
    , copyNotificationCurrentFilter_(copyNotificationCurrentFilter)
    , copyNotificationCurrentProgress_(copyNotificationCurrentProgress)
    , statusDetails_(statusDetails)
{
}

KeyValueStoreQueryResult::KeyValueStoreQueryResult(KeyValueStoreQueryResult && other)
    : ReplicaStatusQueryResult(move(other))
    , dbRowCountEstimate_(move(other.dbRowCountEstimate_))
    , dbLogicalSizeEstimate_(move(other.dbLogicalSizeEstimate_))
    , copyNotificationCurrentFilter_(move(other.copyNotificationCurrentFilter_))
    , copyNotificationCurrentProgress_(move(other.copyNotificationCurrentProgress_))
    , statusDetails_(move(other.statusDetails_))
{
}

KeyValueStoreQueryResult const & KeyValueStoreQueryResult::operator=(KeyValueStoreQueryResult && other)
{
    if (this != &other)
    {
        dbRowCountEstimate_ = move(other.dbRowCountEstimate_);
        dbLogicalSizeEstimate_ = move(other.dbLogicalSizeEstimate_);
        copyNotificationCurrentFilter_ = move(other.copyNotificationCurrentFilter_);
        copyNotificationCurrentProgress_ = move(other.copyNotificationCurrentProgress_);
        statusDetails_ = move(other.statusDetails_);

        ReplicaStatusQueryResult::operator=(move(other));
    }

    return *this;
}

void KeyValueStoreQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_REPLICA_STATUS_QUERY_RESULT & publicResult) const 
{
    auto kvsPublicResult = heap.AddItem<FABRIC_KEY_VALUE_STORE_STATUS_QUERY_RESULT>();

    kvsPublicResult->DatabaseRowCountEstimate = dbRowCountEstimate_;
    kvsPublicResult->DatabaseLogicalSizeEstimate = dbLogicalSizeEstimate_;

    if (copyNotificationCurrentFilter_)
    {
        kvsPublicResult->CopyNotificationCurrentKeyFilter = heap.AddString(*copyNotificationCurrentFilter_);
        kvsPublicResult->CopyNotificationCurrentProgress = copyNotificationCurrentProgress_;
    }

    kvsPublicResult->StatusDetails = heap.AddString(statusDetails_);

    publicResult.Kind = FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE;
    publicResult.Value = kvsPublicResult.GetRawPointer();
}

ErrorCode KeyValueStoreQueryResult::FromPublicApi(
    __in FABRIC_REPLICA_STATUS_QUERY_RESULT const & publicResult)
{
    if (publicResult.Value == nullptr ||
        publicResult.Kind != FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    auto kvsPublicResult = reinterpret_cast<FABRIC_KEY_VALUE_STORE_STATUS_QUERY_RESULT*>(publicResult.Value);
    dbRowCountEstimate_ = kvsPublicResult->DatabaseRowCountEstimate;
    dbLogicalSizeEstimate_ = kvsPublicResult->DatabaseLogicalSizeEstimate;

    if (kvsPublicResult->CopyNotificationCurrentKeyFilter != nullptr)
    {
        copyNotificationCurrentFilter_ = make_shared<wstring>();
        auto hr = StringUtility::LpcwstrToWstring(kvsPublicResult->CopyNotificationCurrentKeyFilter, true, *copyNotificationCurrentFilter_);
        if (!SUCCEEDED(hr)) { return ErrorCode::FromHResult(hr); }

        copyNotificationCurrentProgress_ = kvsPublicResult->CopyNotificationCurrentProgress;
    }

    auto hr = StringUtility::LpcwstrToWstring(kvsPublicResult->StatusDetails, true, statusDetails_);
    if (!SUCCEEDED(hr)) { return ErrorCode::FromHResult(hr); }

    return ErrorCodeValue::Success;
}

void KeyValueStoreQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring KeyValueStoreQueryResult::ToString() const
{
    return wformatString("DbRowCount={0} DbLogicalSize={1} CopyNotificationFilter={2} CopyNotificationProgress={3} Status={4}",
        dbRowCountEstimate_,
        dbLogicalSizeEstimate_,
        copyNotificationCurrentFilter_ ? *copyNotificationCurrentFilter_ : L"null",
        copyNotificationCurrentProgress_,
        statusDetails_);
}

