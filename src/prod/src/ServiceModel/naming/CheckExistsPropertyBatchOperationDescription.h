// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class CheckExistsPropertyBatchOperationDescription : public PropertyBatchOperationDescription
    {
        DENY_COPY(CheckExistsPropertyBatchOperationDescription);

    public:
        CheckExistsPropertyBatchOperationDescription()
            : PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS)
            , exists_(false)
        {
        }

        CheckExistsPropertyBatchOperationDescription(
            std::wstring const & propertyName,
            bool const & exists)
            : PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS, propertyName)
            , exists_(exists)
        {
        }

        virtual ~CheckExistsPropertyBatchOperationDescription() {};

        Common::ErrorCode Verify()
        {
            if (Kind == FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS)
            {
                return Common::ErrorCode::Success();
            }
            return Common::ErrorCodeValue::InvalidArgument;
        }
        
        __declspec(property(get=get_Exists)) bool Exists;
        bool get_Exists() const { return exists_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Exists, exists_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        bool exists_;
    };

    using CheckExistsPropertyBatchOperationDescriptionSPtr = std::shared_ptr<CheckExistsPropertyBatchOperationDescription>;

    template<> class PropertyBatchOperationTypeActivator<FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS>
    {
    public:
        static PropertyBatchOperationDescriptionSPtr CreateSPtr()
        {
            return std::make_shared<CheckExistsPropertyBatchOperationDescription>();
        }
    };
}
