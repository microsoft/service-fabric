// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class HealthEvent
        : public HealthInformation
    {
    public:
        HealthEvent();

        HealthEvent(
            std::wstring const & sourceId,
            std::wstring const & property,
            Common::TimeSpan const & timeToLive,
            FABRIC_HEALTH_STATE state,
            std::wstring const & description,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            Common::DateTime const & sourceUtcTimestamp,
            Common::DateTime const & lastModifiedUtcTimestamp,
            bool isExpired,
            bool removeWhenExpired,
            Common::DateTime const & lastOkTransitionAt,
            Common::DateTime const & lastWarningTransitionAt,
            Common::DateTime const & lastErrorTransitionAt);

        HealthEvent(HealthEvent const & other) = default;
        HealthEvent & operator = (HealthEvent const & other) = default;

        HealthEvent(HealthEvent && other) = default;
        HealthEvent & operator = (HealthEvent && other) = default;

        __declspec(property(get=get_LastModifiedUtcTimestamp)) Common::DateTime const & LastModifiedUtcTimestamp;
        Common::DateTime const & get_LastModifiedUtcTimestamp() const { return lastModifiedUtcTimestamp_; }

        __declspec(property(get=get_LastOkTransitionAt)) Common::DateTime const & LastOkTransitionAt;
        Common::DateTime const & get_LastOkTransitionAt() const { return lastOkTransitionAt_; }

        __declspec(property(get=get_LastWarningTransitionAt)) Common::DateTime const & LastWarningTransitionAt;
        Common::DateTime const & get_LastWarningTransitionAt() const { return lastWarningTransitionAt_; }

        __declspec(property(get=get_LastErrorTransitionAt)) Common::DateTime const & LastErrorTransitionAt;
        Common::DateTime const & get_LastErrorTransitionAt() const { return lastErrorTransitionAt_; }

        __declspec(property(get=get_IsExpired)) bool IsExpired;
        bool get_IsExpired() const { return isExpired_; }

        __declspec(property(get=get_RemoveWhenExpired)) bool RemoveWhenExpired;
        bool get_RemoveWhenExpired() const { return removeWhenExpired_; }

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;
        std::wstring ToString() const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_HEALTH_EVENT & healthEventInformation) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVENT const & healthInformation);

        FABRIC_FIELDS_13(sourceId_, property_, timeToLive_, state_, description_, sequenceNumber_, sourceUtcTimestamp_, lastModifiedUtcTimestamp_, isExpired_, removeWhenExpired_, lastOkTransitionAt_, lastWarningTransitionAt_, lastErrorTransitionAt_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::SourceUtcTimestamp, sourceUtcTimestamp_)
            SERIALIZABLE_PROPERTY(Constants::LastModifiedUtcTimestamp, lastModifiedUtcTimestamp_)
            SERIALIZABLE_PROPERTY(Constants::IsExpired, isExpired_)
            SERIALIZABLE_PROPERTY(Constants::LastOkTransitionAt, lastOkTransitionAt_)
            SERIALIZABLE_PROPERTY(Constants::LastWarningTransitionAt, lastWarningTransitionAt_)
            SERIALIZABLE_PROPERTY(Constants::LastErrorTransitionAt, lastErrorTransitionAt_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(lastModifiedUtcTimestamp_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(lastOkTransitionAt_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(lastWarningTransitionAt_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(lastErrorTransitionAt_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        Common::DateTime lastModifiedUtcTimestamp_;
        bool isExpired_;
        Common::DateTime lastOkTransitionAt_;
        Common::DateTime lastWarningTransitionAt_;
        Common::DateTime lastErrorTransitionAt_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::HealthEvent);
