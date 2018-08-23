//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ContainerServiceQueryResult
        : public ContainerServiceDescription
        , public IPageContinuationToken
        {
        public:
            ContainerServiceQueryResult() = default;

            ContainerServiceQueryResult(std::wstring const &, std::wstring const &,ContainerServiceDescription const &, FABRIC_QUERY_SERVICE_STATUS);

            __declspec(property(get=get_HealthState, put=put_HealthState)) FABRIC_HEALTH_STATE HealthState;
            FABRIC_HEALTH_STATE get_HealthState() const { return Properties.HealthState; }
            void put_HealthState(FABRIC_HEALTH_STATE value) { Properties.HealthState = value; }

            __declspec(property(get=get_Status, put=put_Status)) FABRIC_QUERY_SERVICE_STATUS Status;
            FABRIC_QUERY_SERVICE_STATUS get_Status() const { return Properties.Status; }
            void put_Status(FABRIC_QUERY_SERVICE_STATUS value) { Properties.Status = value; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(ServiceUri)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(ApplicationUri)
            END_DYNAMIC_SIZE_ESTIMATION()

            std::wstring CreateContinuationToken() const;

            FABRIC_FIELDS_02(ServiceUri, ApplicationUri)

            std::wstring ServiceUri;
            std::wstring ApplicationUri;
        };

        QUERY_JSON_LIST(ContainerServiceQueryResultList, ContainerServiceQueryResult)
    }
}
