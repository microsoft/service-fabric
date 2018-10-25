// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedApplicationAggregatedHealthState
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
    public:
        DeployedApplicationAggregatedHealthState();

        DeployedApplicationAggregatedHealthState(
            std::wstring const & applicationName,
            std::wstring const & nodeName,     
            FABRIC_HEALTH_STATE aggregatedHealthState);

        DeployedApplicationAggregatedHealthState(DeployedApplicationAggregatedHealthState const & other) = default;
        DeployedApplicationAggregatedHealthState & operator = (DeployedApplicationAggregatedHealthState const & other) = default;

        DeployedApplicationAggregatedHealthState(DeployedApplicationAggregatedHealthState && other) = default;
        DeployedApplicationAggregatedHealthState & operator = (DeployedApplicationAggregatedHealthState && other) = default;

        ~DeployedApplicationAggregatedHealthState();

        __declspec(property(get=get_ApplicationName)) std::wstring const& ApplicationName;
        std::wstring const& get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_NodeName)) std::wstring const& NodeName;
        std::wstring const& get_NodeName() const { return nodeName_; }
                    
        __declspec(property(get=get_AggregatedHealthState)) FABRIC_HEALTH_STATE AggregatedHealthState;
        FABRIC_HEALTH_STATE get_AggregatedHealthState() const { return aggregatedHealthState_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE & publicDeployedApplicationAggregatedHealthState) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE const & publicDeployedApplicationAggregatedHealthState);
        
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

        FABRIC_FIELDS_03(applicationName_, nodeName_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::NodeName, nodeName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::AggregatedHealthState, aggregatedHealthState_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(aggregatedHealthState_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationName_;
        std::wstring nodeName_;
        FABRIC_HEALTH_STATE aggregatedHealthState_;
    };    
}
