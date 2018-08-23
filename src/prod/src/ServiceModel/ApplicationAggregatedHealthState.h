// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ApplicationAggregatedHealthState
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
    public:
        ApplicationAggregatedHealthState();

        ApplicationAggregatedHealthState(
            std::wstring const & applicationName,
            FABRIC_HEALTH_STATE aggregatedHealthState);

        ApplicationAggregatedHealthState(ApplicationAggregatedHealthState const & other) = default;
        ApplicationAggregatedHealthState & operator = (ApplicationAggregatedHealthState const & other) = default;

        ApplicationAggregatedHealthState(ApplicationAggregatedHealthState && other) = default;
        ApplicationAggregatedHealthState & operator = (ApplicationAggregatedHealthState && other) = default;

        virtual ~ApplicationAggregatedHealthState();

        __declspec(property(get=get_ApplicationName)) std::wstring const& ApplicationName;
        std::wstring const& get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_AggregatedHealthState)) FABRIC_HEALTH_STATE AggregatedHealthState;
        FABRIC_HEALTH_STATE get_AggregatedHealthState() const { return aggregatedHealthState_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_APPLICATION_HEALTH_STATE & publicApplicationAggregatedHealthState) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_APPLICATION_HEALTH_STATE const & publicApplicationAggregatedHealthState);
        
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        
        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

        FABRIC_FIELDS_02(applicationName_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, applicationName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::AggregatedHealthState, aggregatedHealthState_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(aggregatedHealthState_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationName_;
        FABRIC_HEALTH_STATE aggregatedHealthState_;
    };

    using ApplicationAggregatedHealthStateList = std::vector<ApplicationAggregatedHealthState>;
}
