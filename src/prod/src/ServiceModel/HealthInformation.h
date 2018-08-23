// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class HealthInformation
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
    {
        DEFAULT_COPY_CONSTRUCTOR(HealthInformation)
        DEFAULT_COPY_ASSIGNMENT(HealthInformation)
        
    public:
        static Common::GlobalWString DeleteProperty;

        HealthInformation();

        HealthInformation(
            std::wstring const & sourceId,
            std::wstring const & property,
            Common::TimeSpan const & timeToLive,
            FABRIC_HEALTH_STATE state,
            std::wstring const & description,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            Common::DateTime const & sourceUtcTimestamp,
            bool removeWhenExpired);
        
        HealthInformation(
            std::wstring && sourceId,
            std::wstring && property,
            Common::TimeSpan const & timeToLive,
            FABRIC_HEALTH_STATE state,
            std::wstring const & description,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            Common::DateTime const & sourceUtcTimestamp,
            bool removeWhenExpired);

        virtual ~HealthInformation();

        HealthInformation(HealthInformation && other) = default;
        HealthInformation & operator = (HealthInformation && other) = default;

        __declspec(property(get=get_SourceId)) std::wstring const& SourceId;
        std::wstring const& get_SourceId() const { return sourceId_; }

        __declspec(property(get=get_Property,put=set_Property)) std::wstring & Property;
        std::wstring const& get_Property() const { return property_; }
        void set_Property(std::wstring const& value) { property_ = value; }

        __declspec(property(get=get_TimeToLive)) Common::TimeSpan const& TimeToLive;
        Common::TimeSpan const& get_TimeToLive() const { return timeToLive_; }

        __declspec(property(get=get_State)) FABRIC_HEALTH_STATE State;
        FABRIC_HEALTH_STATE get_State() const { return state_; }
        
        __declspec(property(get=get_Description,put=set_Description)) std::wstring const& Description;
        std::wstring const& get_Description() const { return description_; }
        void set_Description(std::wstring const& value) { description_ = value; }
        
        __declspec(property(get=get_SequenceNumber)) FABRIC_SEQUENCE_NUMBER SequenceNumber;
        FABRIC_SEQUENCE_NUMBER get_SequenceNumber() const { return sequenceNumber_; }
        
        __declspec(property(get=get_SourceUtcTimestamp,put=set_SourceUtcTimestamp)) Common::DateTime & SourceUtcTimestamp;
        Common::DateTime const& get_SourceUtcTimestamp() const { return sourceUtcTimestamp_; }
        void set_SourceUtcTimestamp(Common::DateTime const& value) { sourceUtcTimestamp_ = value; }

        __declspec(property(get=get_IsForDelete)) bool IsForDelete;
        bool get_IsForDelete() const { return property_ == DeleteProperty; }

        __declspec(property(get=get_RemoveWhenExpired)) bool RemoveWhenExpired;
        bool get_RemoveWhenExpired() const { return removeWhenExpired_; }

        // Called by HM to validate reports received
        Common::ErrorCode ValidateReport() const;

        // Called by public API layer to make sure user reports are correct:
        // If dissallowSystemReport is set, the health info should not use reserved source and property.
        // Also called from REST to validate reports.
        Common::ErrorCode TryAccept(bool disallowSystemReport);

        Common::ErrorCode ToCommonPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_HEALTH_INFORMATION & healthInformation) const;
        
        Common::ErrorCode FromCommonPublicApi(
            FABRIC_HEALTH_INFORMATION const & healthInformation);

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::SourceId, sourceId_)
            SERIALIZABLE_PROPERTY(Constants::Property, property_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, state_)
            SERIALIZABLE_PROPERTY(Constants::TimeToLiveInMs, timeToLive_)// TODO Update publicapi
            SERIALIZABLE_PROPERTY(Constants::Description, description_)
            SERIALIZABLE_PROPERTY(Constants::SequenceNumber, sequenceNumber_)
            SERIALIZABLE_PROPERTY(Constants::RemoveWhenExpired, removeWhenExpired_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        //timeToLive_ is initialized with MaxValue, so it has no dynamic overhead
        BEGIN_BASE_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(sourceId_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(property_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(state_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(description_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(sourceUtcTimestamp_)
        END_DYNAMIC_SIZE_ESTIMATION()

    protected:
        Common::ErrorCode ValidateNonSystemReport() const;

        // Validates common fields for both system and user reports
        Common::ErrorCode TryAcceptUserOrSystemReport();

    protected:
        // Expose all fields to derived classes, so they can be include in serialization
        std::wstring sourceId_;
        std::wstring property_;
        Common::TimeSpan timeToLive_;
        FABRIC_HEALTH_STATE state_;
        std::wstring description_;
        FABRIC_SEQUENCE_NUMBER sequenceNumber_;
        bool removeWhenExpired_;

        // Generated internally when the report is accepted
        Common::DateTime sourceUtcTimestamp_;
    };
}
