// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

const StringLiteral TraceComponent("KeyValueStoreQueryResult");

KeyValueStoreQueryResult::KeyValueStoreQueryResult()
    : ReplicaStatusQueryResult(FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE)
    , dbRowCountEstimate_(0)
    , dbLogicalSizeEstimate_(0)
    , copyNotificationCurrentFilter_()
    , copyNotificationCurrentProgress_(0)
    , statusDetails_()
    , providerKind_(Store::ProviderKind::Unknown)
    , migrationDetails_()
{
}

KeyValueStoreQueryResult::KeyValueStoreQueryResult(
    size_t dbRowCountEstimate,
    size_t dbLogicalSizeEstimate,
    shared_ptr<wstring> copyNotificationCurrentFilter,
    size_t copyNotificationCurrentProgress,
    wstring const & statusDetails,
    Store::ProviderKind::Enum providerKind,
    shared_ptr<Store::MigrationQueryResult> && migrationDetails)
    : ReplicaStatusQueryResult(FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE)
    , dbRowCountEstimate_(dbRowCountEstimate)
    , dbLogicalSizeEstimate_(dbLogicalSizeEstimate)
    , copyNotificationCurrentFilter_(copyNotificationCurrentFilter)
    , copyNotificationCurrentProgress_(copyNotificationCurrentProgress)
    , statusDetails_(statusDetails)
    , providerKind_(providerKind)
    , migrationDetails_(move(migrationDetails))
{
}

KeyValueStoreQueryResult::KeyValueStoreQueryResult(KeyValueStoreQueryResult && other)
    : ReplicaStatusQueryResult(move(other))
    , dbRowCountEstimate_(move(other.dbRowCountEstimate_))
    , dbLogicalSizeEstimate_(move(other.dbLogicalSizeEstimate_))
    , copyNotificationCurrentFilter_(move(other.copyNotificationCurrentFilter_))
    , copyNotificationCurrentProgress_(move(other.copyNotificationCurrentProgress_))
    , statusDetails_(move(other.statusDetails_))
    , providerKind_(move(other.providerKind_))
    , migrationDetails_(move(other.migrationDetails_))
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
        providerKind_ = move(other.providerKind_);
        migrationDetails_ = move(other.migrationDetails_);

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

    auto ex1 = heap.AddItem<FABRIC_KEY_VALUE_STORE_STATUS_QUERY_RESULT_EX1>();

    ex1->ProviderKind = Store::ProviderKind::ToPublic(providerKind_);

    if (migrationDetails_.get() != nullptr)
    {
        auto publicMigration = heap.AddItem<FABRIC_KEY_VALUE_STORE_MIGRATION_QUERY_RESULT>();
        migrationDetails_->ToPublicApi(heap, *publicMigration);
        ex1->MigrationStatus = publicMigration.GetRawPointer();
    }

    kvsPublicResult->Reserved = ex1.GetRawPointer();

    publicResult.Kind = FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE;
    publicResult.Value = kvsPublicResult.GetRawPointer();
}

ErrorCode KeyValueStoreQueryResult::FromPublicApi(
    __in FABRIC_REPLICA_STATUS_QUERY_RESULT const & publicResult)
{
    if (publicResult.Value == nullptr)
    {
        Trace.WriteWarning(TraceComponent, "FromPublicApi: null value");

        return ErrorCodeValue::InvalidArgument;
    }
    else if (publicResult.Kind != FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE)
    {
        Trace.WriteWarning(TraceComponent, "FromPublicApi: invalid kind {0}", static_cast<int>(publicResult.Kind));

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

    if (kvsPublicResult->Reserved != nullptr)
    {
        auto ex1 = reinterpret_cast<FABRIC_KEY_VALUE_STORE_STATUS_QUERY_RESULT_EX1*>(kvsPublicResult->Reserved);

        providerKind_ = Store::ProviderKind::FromPublic(ex1->ProviderKind);

        if (ex1->MigrationStatus != nullptr)
        {
            migrationDetails_ = make_shared<Store::MigrationQueryResult>();
            auto error = migrationDetails_->FromPublicApi(*(ex1->MigrationStatus));
            if (!error.IsSuccess()) { return error; }
        }
    }

    return ErrorCodeValue::Success;
}

void KeyValueStoreQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring KeyValueStoreQueryResult::ToString() const
{
    auto toString = wformatString("DbRowCount={0} DbLogicalSize={1} CopyNotificationFilter={2} CopyNotificationProgress={3} Status={4} Provider={5}",
        dbRowCountEstimate_,
        dbLogicalSizeEstimate_,
        copyNotificationCurrentFilter_ ? *copyNotificationCurrentFilter_ : L"null",
        copyNotificationCurrentProgress_,
        statusDetails_,
        providerKind_);

    if (migrationDetails_.get() != nullptr)
    {
        toString.append(wformatString(" Migration={0}", *migrationDetails_));
    }

    return toString;
}

