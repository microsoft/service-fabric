// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ServiceReplicaDescription
        : public ServicePropertiesBase
        {
        public:
            ServiceReplicaDescription() = default;
            ServiceReplicaDescription(std::wstring const & replicaName);

            DEFAULT_MOVE_ASSIGNMENT(ServiceReplicaDescription)
            DEFAULT_MOVE_CONSTRUCTOR(ServiceReplicaDescription)
            DEFAULT_COPY_ASSIGNMENT(ServiceReplicaDescription)
            DEFAULT_COPY_CONSTRUCTOR(ServiceReplicaDescription)

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(L"replicaName", replicaName_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_01(replicaName_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(replicaName_)
            END_DYNAMIC_SIZE_ESTIMATION()

        protected:
            std::wstring replicaName_;
        };
    }
}
