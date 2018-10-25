// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServiceAggregatedHealthState
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
    public:
        ServiceAggregatedHealthState();

        ServiceAggregatedHealthState(
            std::wstring const & serviceName,
            FABRIC_HEALTH_STATE aggregatedHealthState);

        ServiceAggregatedHealthState(
            std::wstring && serviceName,
            FABRIC_HEALTH_STATE aggregatedHealthState);

        ServiceAggregatedHealthState(ServiceAggregatedHealthState const & other) = default;
        ServiceAggregatedHealthState & operator = (ServiceAggregatedHealthState const & other) = default;

        ServiceAggregatedHealthState(ServiceAggregatedHealthState && other) = default;
        ServiceAggregatedHealthState & operator = (ServiceAggregatedHealthState && other) = default;

        ~ServiceAggregatedHealthState();

        __declspec(property(get=get_ServiceName)) std::wstring const& ServiceName;
        std::wstring const& get_ServiceName() const { return serviceName_; }

         __declspec(property(get=get_AggregatedHealthState)) FABRIC_HEALTH_STATE AggregatedHealthState;
        FABRIC_HEALTH_STATE get_AggregatedHealthState() const { return aggregatedHealthState_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SERVICE_HEALTH_STATE & publicServiceAggregatedHealthState) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_SERVICE_HEALTH_STATE const & publicServiceAggregatedHealthState);
        
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

        FABRIC_FIELDS_02(serviceName_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ServiceName, serviceName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::AggregatedHealthState, aggregatedHealthState_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceName_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(aggregatedHealthState_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring serviceName_;
        FABRIC_HEALTH_STATE aggregatedHealthState_;
    };    
}
