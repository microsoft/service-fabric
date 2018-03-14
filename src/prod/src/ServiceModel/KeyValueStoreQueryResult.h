// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class KeyValueStoreQueryResult : public ReplicaStatusQueryResult
    {
    public:
        KeyValueStoreQueryResult();

        KeyValueStoreQueryResult(
            size_t dbRowCountEstimate,
            size_t dbLogicalSizeEstimate,
            std::shared_ptr<std::wstring> copyNotificationCurrentFilter,
            size_t copyNotificationCurrentProgress,
            std::wstring const &);

        KeyValueStoreQueryResult(KeyValueStoreQueryResult && other);

        KeyValueStoreQueryResult const & operator = (KeyValueStoreQueryResult && other);
        
        virtual void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_REPLICA_STATUS_QUERY_RESULT & publicResult) const override;
        
        virtual Common::ErrorCode FromPublicApi(
            __in FABRIC_REPLICA_STATUS_QUERY_RESULT const & publicResult) override;

        virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const override;
        virtual std::wstring ToString() const override;

        FABRIC_FIELDS_05(
            dbRowCountEstimate_, 
            dbLogicalSizeEstimate_, 
            copyNotificationCurrentFilter_, 
            copyNotificationCurrentProgress_, 
            statusDetails_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::DatabaseRowCountEstimate, dbRowCountEstimate_)
            SERIALIZABLE_PROPERTY(Constants::DatabaseLogicalSizeEstimate, dbLogicalSizeEstimate_)
            SERIALIZABLE_PROPERTY_IF(Constants::CopyNotificationCurrentKeyFilter, copyNotificationCurrentFilter_, copyNotificationCurrentFilter_.get() != nullptr)
            SERIALIZABLE_PROPERTY_IF(Constants::CopyNotificationCurrentProgress, copyNotificationCurrentProgress_, copyNotificationCurrentFilter_.get() != nullptr)
            SERIALIZABLE_PROPERTY(Constants::StatusDetails, statusDetails_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    protected:
        size_t dbRowCountEstimate_;
        size_t dbLogicalSizeEstimate_;
        std::shared_ptr<std::wstring> copyNotificationCurrentFilter_;
        size_t copyNotificationCurrentProgress_;
        std::wstring statusDetails_;
    };
}
