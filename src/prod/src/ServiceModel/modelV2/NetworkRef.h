//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

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

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::nameCamelCase, Name)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(Name)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            FABRIC_FIELDS_01(
                Name);

        public:
            std::wstring Name;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::NetworkRef);
