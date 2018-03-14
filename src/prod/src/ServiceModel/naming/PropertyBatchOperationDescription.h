// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PropertyBatchOperationDescription;
    using PropertyBatchOperationDescriptionSPtr = shared_ptr<PropertyBatchOperationDescription>;

    class PropertyBatchOperationDescription : public Common::IFabricJsonSerializable
    {
        DENY_COPY(PropertyBatchOperationDescription);

    public:
        PropertyBatchOperationDescription();

        explicit PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND kind);

        PropertyBatchOperationDescription(
            FABRIC_PROPERTY_BATCH_OPERATION_KIND kind,
            std::wstring const & propertyName);

        ~PropertyBatchOperationDescription();

        virtual Common::ErrorCode Verify();

        static PropertyBatchOperationDescriptionSPtr CreateSPtr(FABRIC_PROPERTY_BATCH_OPERATION_KIND kind);
        
        __declspec(property(get=get_Kind)) FABRIC_PROPERTY_BATCH_OPERATION_KIND const & Kind;
        __declspec(property(get=get_PropertyName)) std::wstring const & PropertyName;

        FABRIC_PROPERTY_BATCH_OPERATION_KIND const & get_Kind() const { return kind_; }
        std::wstring const & get_PropertyName() const { return propertyName_; }

        std::wstring && TakePropertyName() { return std::move(propertyName_); }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PropertyName, propertyName_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        JSON_TYPE_ACTIVATOR_METHOD(PropertyBatchOperationDescription, FABRIC_PROPERTY_BATCH_OPERATION_KIND, kind_, CreateSPtr)

    private:
        FABRIC_PROPERTY_BATCH_OPERATION_KIND kind_;
        std::wstring propertyName_;
    };

    template<FABRIC_PROPERTY_BATCH_OPERATION_KIND>
    class PropertyBatchOperationTypeActivator
    {
    public:
        static PropertyBatchOperationDescriptionSPtr CreateSPtr()
        {
            return std::make_shared<PropertyBatchOperationDescription>();
        }
    };
}
