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
            std::wstring const &,
            Store::ProviderKind::Enum,
            std::shared_ptr<Store::MigrationQueryResult> &&);

        KeyValueStoreQueryResult(KeyValueStoreQueryResult && other);

        KeyValueStoreQueryResult const & operator = (KeyValueStoreQueryResult && other);
        
        __declspec(property(get = get_StatusDetails)) std::wstring const & StatusDetails;
        std::wstring const & get_StatusDetails() const { return statusDetails_; }

        __declspec(property(get = get_ProviderKind)) Store::ProviderKind::Enum ProviderKind;
        Store::ProviderKind::Enum const & get_ProviderKind() const { return providerKind_; }

        __declspec(property(get = get_MigrationDetails)) std::shared_ptr<Store::MigrationQueryResult> const & MigrationDetails;
        std::shared_ptr<Store::MigrationQueryResult> const & get_MigrationDetails() const { return migrationDetails_; }

        virtual void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_REPLICA_STATUS_QUERY_RESULT & publicResult) const override;
        
        virtual Common::ErrorCode FromPublicApi(
            __in FABRIC_REPLICA_STATUS_QUERY_RESULT const & publicResult) override;

        virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const override;
        virtual std::wstring ToString() const override;

        FABRIC_FIELDS_07(
            dbRowCountEstimate_, 
            dbLogicalSizeEstimate_, 
            copyNotificationCurrentFilter_, 
            copyNotificationCurrentProgress_, 
            statusDetails_,
            providerKind_,
            migrationDetails_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::DatabaseRowCountEstimate, dbRowCountEstimate_)
            SERIALIZABLE_PROPERTY(Constants::DatabaseLogicalSizeEstimate, dbLogicalSizeEstimate_)
            SERIALIZABLE_PROPERTY_IF(Constants::CopyNotificationCurrentKeyFilter, copyNotificationCurrentFilter_, copyNotificationCurrentFilter_.get() != nullptr)
            SERIALIZABLE_PROPERTY_IF(Constants::CopyNotificationCurrentProgress, copyNotificationCurrentProgress_, copyNotificationCurrentFilter_.get() != nullptr)
            SERIALIZABLE_PROPERTY(Constants::StatusDetails, statusDetails_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::ProviderKind, providerKind_)
            SERIALIZABLE_PROPERTY_IF(Constants::MigrationStatus, migrationDetails_, migrationDetails_.get() != nullptr)
        END_JSON_SERIALIZABLE_PROPERTIES()

    protected:
        size_t dbRowCountEstimate_;
        size_t dbLogicalSizeEstimate_;
        std::shared_ptr<std::wstring> copyNotificationCurrentFilter_;
        size_t copyNotificationCurrentProgress_;
        std::wstring statusDetails_;
        Store::ProviderKind::Enum providerKind_;
        std::shared_ptr<Store::MigrationQueryResult> migrationDetails_;
    };
}
