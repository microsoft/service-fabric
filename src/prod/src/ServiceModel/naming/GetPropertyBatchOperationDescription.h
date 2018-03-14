// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class GetPropertyBatchOperationDescription : public PropertyBatchOperationDescription
    {
        DENY_COPY(GetPropertyBatchOperationDescription);

    public:
        GetPropertyBatchOperationDescription()
            : PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET)
            , includeValue_(false)
        {
        }

        GetPropertyBatchOperationDescription(
            std::wstring const & propertyName,
            bool includeValue)
            : PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET, propertyName)
            , includeValue_(includeValue)
        {
        }

        virtual ~GetPropertyBatchOperationDescription() {};

        Common::ErrorCode Verify()
        {
            if (Kind == FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET)
            {
                return Common::ErrorCode::Success();
            }
            return Common::ErrorCodeValue::InvalidArgument;
        }
        
        __declspec(property(get=get_IncludeValue)) bool IncludeValue;
        bool get_IncludeValue() const { return includeValue_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::IncludeValue, includeValue_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        bool includeValue_;
    };

    using GetPropertyBatchOperationDescriptionSPtr = std::shared_ptr<GetPropertyBatchOperationDescription>;

    template<> class PropertyBatchOperationTypeActivator<FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET>
    {
    public:
        static PropertyBatchOperationDescriptionSPtr CreateSPtr()
        {
            return std::make_shared<GetPropertyBatchOperationDescription>();
        }
    };
}
