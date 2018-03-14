// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PropertyInfo : public Common::IFabricJsonSerializable
    {
    public:
        PropertyInfo();

        explicit PropertyInfo(NamePropertyMetadataResult && metadata);

        PropertyInfo(
            PropertyValueDescriptionSPtr value,
            NamePropertyMetadataResult && metadata,
            bool includeValues);

        PropertyInfo(PropertyInfo && other);

        ~PropertyInfo();

        static PropertyInfo Create(NamePropertyResult && nameProperty, bool includeValues);

        __declspec(property(get=get_Name)) std::wstring const & Name;
        __declspec(property(get=get_Value)) PropertyValueDescriptionSPtr const & Value;
        __declspec(property(get=get_IncludeValues)) bool IncludeValues;

        std::wstring const & get_Name() const { return name_; }
        PropertyValueDescriptionSPtr const & get_Value() const { return value_; }
        bool get_IncludeValues() const { return includeValues_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Name, name_)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::Value, value_, includeValues_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Metadata, metadata_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring name_;
        PropertyValueDescriptionSPtr value_;
        NamePropertyMetadataResult metadata_;
        bool includeValues_;
    };

}
