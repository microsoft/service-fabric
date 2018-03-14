// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PutPropertyBatchOperationDescription : public PropertyBatchOperationDescription
    {
        DENY_COPY(PutPropertyBatchOperationDescription);

    public:
        PutPropertyBatchOperationDescription()
            : PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT)
            , customTypeId_()
            , value_()
        {
        }

        PutPropertyBatchOperationDescription(
            std::wstring const & propertyName,
            PropertyValueDescriptionSPtr const & value)
            : PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT, propertyName)
            , customTypeId_()
            , value_(value)
        {
        }

        PutPropertyBatchOperationDescription(
            std::wstring const & propertyName,
            std::wstring const & customTypeId,
            PropertyValueDescriptionSPtr const & value)
            : PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT, propertyName)
            , customTypeId_(customTypeId)
            , value_(value)
        {
        }

        virtual ~PutPropertyBatchOperationDescription() {};

        Common::ErrorCode Verify()
        {
            if (Kind == FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT && value_ != nullptr)
            {
                return Common::ErrorCode::Success();
            }
            return Common::ErrorCodeValue::InvalidArgument;
        }
        
        __declspec(property(get=get_CustomTypeId)) std::wstring const & CustomTypeId;
        __declspec(property(get=get_Value)) PropertyValueDescriptionSPtr const & Value;

        std::wstring const & get_CustomTypeId() const { return customTypeId_; }
        PropertyValueDescriptionSPtr const & get_Value() const { return value_; }

        std::wstring && TakeCustomTypeId() { return std::move(customTypeId_); }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::CustomTypeId, customTypeId_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Value, value_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring customTypeId_;
        PropertyValueDescriptionSPtr value_;
    };

    using PutPropertyBatchOperationDescriptionSPtr = std::shared_ptr<PutPropertyBatchOperationDescription>;

    template<> class PropertyBatchOperationTypeActivator<FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT>
    {
    public:
        static PropertyBatchOperationDescriptionSPtr CreateSPtr()
        {
            return std::make_shared<PutPropertyBatchOperationDescription>();
        }
    };
}
