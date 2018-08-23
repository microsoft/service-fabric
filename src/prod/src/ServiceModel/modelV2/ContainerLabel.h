// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ContainerLabel
        : public ModelType
        {
        public:
            ContainerLabel() = default;

            DEFAULT_MOVE_ASSIGNMENT(ContainerLabel)
            DEFAULT_MOVE_CONSTRUCTOR(ContainerLabel)
            DEFAULT_COPY_ASSIGNMENT(ContainerLabel)
            DEFAULT_COPY_CONSTRUCTOR(ContainerLabel)

            __declspec(property(get=get_Name, put=put_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }
            void put_Name(std::wstring const & value) { name_ = value; }

            __declspec(property(get=get_Value, put=put_Value)) std::wstring const & Value;
            std::wstring const & get_Value() const { return value_; }
            void put_Value(std::wstring const & value) { value_ = value; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"name", name_)
                SERIALIZABLE_PROPERTY(L"value", value_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_02(name_, value_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(value_)
            END_DYNAMIC_SIZE_ESTIMATION()

            virtual Common::ErrorCode TryValidate(std::wstring const &traceId) const override
            {
                UNREFERENCED_PARAMETER(traceId);
                return Common::ErrorCodeValue::Success;
            }

        protected:
            std::wstring name_;
            std::wstring value_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::ContainerLabel);
