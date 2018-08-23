// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ContainerServiceProperties
        : public ServicePropertiesBase
        {
        public:
            ContainerServiceProperties() = default;

            DEFAULT_MOVE_ASSIGNMENT(ContainerServiceProperties)
            DEFAULT_MOVE_CONSTRUCTOR(ContainerServiceProperties)
            DEFAULT_COPY_ASSIGNMENT(ContainerServiceProperties)
            DEFAULT_COPY_CONSTRUCTOR(ContainerServiceProperties)

            bool operator==(ContainerServiceProperties const & other) const;
            bool operator!=(ContainerServiceProperties const & other) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(L"description", description_)
                SERIALIZABLE_PROPERTY(L"replicaCount", replicaCount_)
                SERIALIZABLE_PROPERTY_ENUM(L"healthState", HealthState)
                SERIALIZABLE_PROPERTY_ENUM(L"status", Status)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_04(description_, replicaCount_, HealthState, Status);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(description_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(replicaCount_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(HealthState)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(Status)
            END_DYNAMIC_SIZE_ESTIMATION()
            
            Common::ErrorCode TryValidate(std::wstring const &) const override;
            bool CanUpgrade(ContainerServiceProperties const & other) const;

        public:
            FABRIC_HEALTH_STATE HealthState = FABRIC_HEALTH_STATE_UNKNOWN;
            FABRIC_QUERY_SERVICE_STATUS Status = FABRIC_QUERY_SERVICE_STATUS_UNKNOWN;

        private:
            std::wstring description_;
            int replicaCount_ = 1;
        };

        class ContainerServiceDescription
            : public ResourceDescription
        {
        public:
            ContainerServiceDescription()
                : ResourceDescription(*ServiceModel::Constants::ModelV2ReservedDnsNameCharacters)
                , Properties()
            {
            }

            bool operator==(ContainerServiceDescription const &) const;
            bool operator!=(ContainerServiceDescription const &) const;

            __declspec(property(get=get_ServiceName, put=put_ServiceName)) std::wstring const & ServiceName;
            std::wstring const & get_ServiceName() const { return Name; }

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;
            bool CanUpgrade(ContainerServiceDescription const & other) const;
            void WriteTo(__in Common::TextWriter &, Common::FormatOptions const &) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(Constants::properties, Properties)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(Properties)
            END_DYNAMIC_SIZE_ESTIMATION()

            FABRIC_FIELDS_01(Properties)

        public:
            ContainerServiceProperties Properties;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::ContainerServiceDescription);
