// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedServicePackageAggregatedHealthState
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
    public:
        DeployedServicePackageAggregatedHealthState();

        DeployedServicePackageAggregatedHealthState(
            std::wstring const & applicationName,
            std::wstring const & serviceManifestName,
            std::wstring const & servicePackageActivationId,
            std::wstring const & nodeName,
            FABRIC_HEALTH_STATE aggregatedHealthState);

        DeployedServicePackageAggregatedHealthState(DeployedServicePackageAggregatedHealthState const & other) = default;
        DeployedServicePackageAggregatedHealthState & operator = (DeployedServicePackageAggregatedHealthState const & other) = default;

        DeployedServicePackageAggregatedHealthState(DeployedServicePackageAggregatedHealthState && other) = default;
        DeployedServicePackageAggregatedHealthState & operator = (DeployedServicePackageAggregatedHealthState && other) = default;

        ~DeployedServicePackageAggregatedHealthState();

        __declspec(property(get=get_ApplicationName)) std::wstring const& ApplicationName;
        std::wstring const& get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_ServiceManifestName)) std::wstring const& ServiceManifestName;
        std::wstring const& get_ServiceManifestName() const { return serviceManifestName_; }

        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }

        __declspec(property(get=get_NodeName)) std::wstring const& NodeName;
        std::wstring const& get_NodeName() const { return nodeName_; }
            
        __declspec(property(get=get_AggregatedHealthState)) FABRIC_HEALTH_STATE AggregatedHealthState;
        FABRIC_HEALTH_STATE get_AggregatedHealthState() const { return aggregatedHealthState_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE & publicDeployedServicePackageAggregatedHealthState) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE const & publicDeployedServicePackageAggregatedHealthState);
        
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

		static std::wstring CreateContinuationString(
			std::wstring const & serviceManifestName,
			std::wstring const & servicePackageActivationId);

        FABRIC_FIELDS_05(applicationName_, serviceManifestName_, nodeName_, aggregatedHealthState_, servicePackageActivationId_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestName, serviceManifestName_)
            SERIALIZABLE_PROPERTY(Constants::NodeName, nodeName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::AggregatedHealthState, aggregatedHealthState_)
            SERIALIZABLE_PROPERTY(Constants::ServicePackageActivationId, servicePackageActivationId_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceManifestName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(aggregatedHealthState_)
			DYNAMIC_SIZE_ESTIMATION_MEMBER(servicePackageActivationId_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationName_;
        std::wstring serviceManifestName_;
        std::wstring servicePackageActivationId_;
        std::wstring nodeName_;
        FABRIC_HEALTH_STATE aggregatedHealthState_;
    };    
}
