// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PropertyDescription : public Common::IFabricJsonSerializable
    {
    public:
        PropertyDescription()
            : propertyName_()
            , customTypeId_()
            , value_()
        {
        }

        PropertyDescription(
            std::wstring const & propertyName,
            PropertyValueDescriptionSPtr const & value)
            : propertyName_(propertyName)
            , customTypeId_()
            , value_(value)
        {
        }

        PropertyDescription(
            std::wstring const & propertyName,
            PropertyValueDescriptionSPtr const & value,
            std::wstring const & customTypeId)
            : propertyName_(propertyName)
            , customTypeId_(customTypeId)
            , value_(value)
        {
        }

        virtual ~PropertyDescription() {};

        Common::ErrorCode Verify()
        {
            return value_->Verify();
        }
        
        __declspec(property(get=get_PropertyName)) std::wstring const & PropertyName;
        __declspec(property(get=get_CustomTypeId)) std::wstring const & CustomTypeId;
        __declspec(property(get=get_Value)) PropertyValueDescriptionSPtr const & Value;

        std::wstring const & get_PropertyName() const { return propertyName_; }
        std::wstring const & get_CustomTypeId() const { return customTypeId_; }
        PropertyValueDescriptionSPtr const & get_Value() const { return value_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PropertyName, propertyName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::CustomTypeId, customTypeId_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Value, value_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring propertyName_;
        std::wstring customTypeId_;
        PropertyValueDescriptionSPtr value_;
    };
}
