// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class CheckValuePropertyBatchOperationDescription : public PropertyBatchOperationDescription
    {
        DENY_COPY(CheckValuePropertyBatchOperationDescription);

    public:
        CheckValuePropertyBatchOperationDescription()
            : PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_VALUE)
            , value_()
        {
        }

        CheckValuePropertyBatchOperationDescription(
            std::wstring const & propertyName,
            PropertyValueDescriptionSPtr && value)
            : PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_VALUE, propertyName)
            , value_(std::move(value))
        {
        }

        virtual ~CheckValuePropertyBatchOperationDescription() {};

        Common::ErrorCode Verify()
        {
            if (Kind == FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_VALUE && value_ != nullptr)
            {
                return Common::ErrorCode::Success();
            }
            return Common::ErrorCodeValue::InvalidArgument;
        }
        
        __declspec(property(get=get_Value)) PropertyValueDescriptionSPtr const & Value;
        PropertyValueDescriptionSPtr const & get_Value() const { return value_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Value, value_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        PropertyValueDescriptionSPtr value_;
    };

    using CheckValuePropertyBatchOperationDescriptionSPtr = std::shared_ptr<CheckValuePropertyBatchOperationDescription>;

    template<> class PropertyBatchOperationTypeActivator<FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_VALUE>
    {
    public:
        static PropertyBatchOperationDescriptionSPtr CreateSPtr()
        {
            return std::make_shared<CheckValuePropertyBatchOperationDescription>();
        }
    };
}
