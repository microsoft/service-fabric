//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class SettingDescription
            : public ModelType
            , public Common::ISizeEstimator
        {
        public:
            SettingDescription() = default;

            bool operator==(SettingDescription const & other) const;
            bool operator!=(SettingDescription const & other) const;

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::nameCamelCase, Name)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::valueCamelCase, Value)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(Name)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(Value)
            END_DYNAMIC_SIZE_ESTIMATION()

            FABRIC_FIELDS_02(Name, Value);

        public:
            std::wstring Name;
            std::wstring Value;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::SettingDescription);
