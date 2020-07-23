// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class NetworkRef
            : public ModelType
        {
        public:
            NetworkRef() = default;

            bool operator==(NetworkRef const & other) const;

            __declspec(property(get=get_Endpoints)) std::vector<ServiceModel::ModelV2::EndpointRef> const & Endpoints;
            std::vector<ServiceModel::ModelV2::EndpointRef> const & get_Endpoints() const { return EndpointRefs; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::nameCamelCase, Name)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::endpointRefsCamelCase, EndpointRefs)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(Name)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(EndpointRefs)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            FABRIC_FIELDS_02(
                Name,
                EndpointRefs);

            Common::ErrorCode FromPublicApi(NETWORK_REF const & publicNetworkRef);
            void ToPublicApi(__in Common::ScopedHeap &, __out NETWORK_REF &) const;

        public:
            std::wstring Name;
            std::vector<ServiceModel::ModelV2::EndpointRef> EndpointRefs;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::NetworkRef);
