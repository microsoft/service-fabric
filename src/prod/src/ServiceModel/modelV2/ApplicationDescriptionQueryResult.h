// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ModelV2
    {
        class ApplicationDescriptionQueryResult
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
            , public IPageContinuationToken
        {
        public:
            ApplicationDescriptionQueryResult() = default;
            DEFAULT_MOVE_CONSTRUCTOR(ApplicationDescriptionQueryResult);
            DEFAULT_MOVE_ASSIGNMENT(ApplicationDescriptionQueryResult);

            ApplicationDescriptionQueryResult(
                SingleInstanceDeploymentQueryResult &&deploymentResult,
                std::wstring const &description,
                std::vector<std::wstring> &&serviceNames);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"name", applicationName_)
                SERIALIZABLE_PROPERTY(L"properties", properties_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationUri_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(deploymentName_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(properties_)
            END_DYNAMIC_SIZE_ESTIMATION()

            __declspec(property(get=get_ApplicationName)) std::wstring const &ApplicationName;
            std::wstring const& get_ApplicationName() const { return applicationName_; }

            __declspec(property(get=get_healthState, put=set_healthState)) FABRIC_HEALTH_STATE HealthState;
            FABRIC_HEALTH_STATE get_healthState() const { return properties_.HealthState; }
            void set_healthState(FABRIC_HEALTH_STATE const &healthState) { properties_.HealthState = healthState; }

            __declspec(property(get=get_ApplicationUri)) Common::NamingUri const & ApplicationUri;
            Common::NamingUri const & get_ApplicationUri() const { return applicationUri_; }

            __declspec(property(get=get_UnhealthyEvaluation, put=put_UnhealthyEvaluation)) std::wstring & UnhealthyEvaluation;
            std::wstring & get_UnhealthyEvaluation() { return properties_.UnhealthyEvaluation; }
            void put_UnhealthyEvaluation(std::wstring && value) { properties_.UnhealthyEvaluation = std::move(value); }

            FABRIC_FIELDS_04(applicationName_, applicationUri_, deploymentName_, properties_);

            std::wstring CreateContinuationToken() const;

            private:

            class ApplicationProperties
                : public Serialization::FabricSerializable
                , public Common::IFabricJsonSerializable
                , public Common::ISizeEstimator
            {
                public:
                    ApplicationProperties() = default;
                    DEFAULT_COPY_CONSTRUCTOR(ApplicationProperties);
                    DEFAULT_COPY_ASSIGNMENT(ApplicationProperties);
                    DEFAULT_MOVE_CONSTRUCTOR(ApplicationProperties);
                    DEFAULT_MOVE_ASSIGNMENT(ApplicationProperties);

                    ApplicationProperties(
                        std::wstring const &description,
                        std::vector<std::wstring> &&serviceNames,
                        std::wstring const &statusDetails,
                        ServiceModel::SingleInstanceDeploymentStatus::Enum status);

                    BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                        SERIALIZABLE_PROPERTY(L"description", description_)
                        SERIALIZABLE_PROPERTY_IF(L"serviceNames", serviceNames_, (serviceNames_.size() != 0))
                        SERIALIZABLE_PROPERTY_ENUM(L"healthState", HealthState)
                        SERIALIZABLE_PROPERTY_IF(Constants::statusDetails, statusDetails_, !statusDetails_.empty())
                        SERIALIZABLE_PROPERTY_ENUM(Constants::status, status_)
                        SERIALIZABLE_PROPERTY_IF(Constants::unhealthyEvaluation, UnhealthyEvaluation, !UnhealthyEvaluation.empty())
                    END_JSON_SERIALIZABLE_PROPERTIES()
    
                    BEGIN_DYNAMIC_SIZE_ESTIMATION()
                        DYNAMIC_SIZE_ESTIMATION_MEMBER(description_)
                        DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceNames_)
                        DYNAMIC_SIZE_ESTIMATION_MEMBER(HealthState)
                        DYNAMIC_SIZE_ESTIMATION_MEMBER(statusDetails_)
                        DYNAMIC_SIZE_ESTIMATION_MEMBER(status_)
                        DYNAMIC_SIZE_ESTIMATION_MEMBER(UnhealthyEvaluation)
                    END_DYNAMIC_SIZE_ESTIMATION()

                    FABRIC_FIELDS_06(
                        description_,
                        serviceNames_,
                        HealthState,
                        statusDetails_,
                        status_,
                        UnhealthyEvaluation);

                public:
                    FABRIC_HEALTH_STATE HealthState = FABRIC_HEALTH_STATE_UNKNOWN;
                    std::wstring UnhealthyEvaluation;

                private:
                    std::wstring description_;
                    std::vector<std::wstring> serviceNames_;
                    std::wstring statusDetails_;
                    ServiceModel::SingleInstanceDeploymentStatus::Enum status_;
            };

            Common::NamingUri applicationUri_;
            std::wstring applicationName_;
            std::wstring deploymentName_;
            ApplicationProperties properties_;
        };

        QUERY_JSON_LIST(ApplicationDescriptionQueryResultList, ApplicationDescriptionQueryResult)
    }
}
