// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationParameter : public Common::IFabricJsonSerializable
    {
    public:
        ApplicationParameter() {};
        ApplicationParameter(std::wstring name, std::wstring value)
            : name_(name)
            , value_(value)
        {}

        ~ApplicationParameter() {};

        __declspec(property(get=get_Name, put=set_Name)) std::wstring const &Name;
        __declspec(property(get=get_Value, put=set_Value)) std::wstring const &Value;

        std::wstring const& get_Name() const { return name_; }
        std::wstring const& get_Value() const { return value_; }

        void set_Name(std::wstring const &name) { name_ = name; }
        void set_Value(std::wstring const &value) { value_ = value; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, name_)
            SERIALIZABLE_PROPERTY(Constants::Value, value_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring name_;
        std::wstring value_;
    };
}
