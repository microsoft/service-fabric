// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamePropertyMetadata 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
        , public Common::IFabricJsonSerializable
    {
    public:
        NamePropertyMetadata();

        NamePropertyMetadata(NamePropertyMetadata const & other);
        NamePropertyMetadata & operator=(NamePropertyMetadata const & other);

        NamePropertyMetadata(NamePropertyMetadata && other);
        NamePropertyMetadata & operator=(NamePropertyMetadata && other);

        NamePropertyMetadata(
            std::wstring const & propertyName,
            ::FABRIC_PROPERTY_TYPE_ID typeId,
            ULONG sizeInBytes,
            std::wstring const & customTypeId);

        __declspec(property(get=get_PropertyName)) std::wstring const & PropertyName;
        inline std::wstring const & get_PropertyName() const { return propertyName_; }
        
        __declspec(property(get=get_NamePropertyTypeId)) ::FABRIC_PROPERTY_TYPE_ID TypeId;
        inline ::FABRIC_PROPERTY_TYPE_ID get_NamePropertyTypeId() const { return typeId_; }
        
        __declspec(property(get=get_SizeInBytes)) ULONG SizeInBytes;
        inline ULONG get_SizeInBytes() const { return sizeInBytes_; }

        __declspec(property(get=get_CustomTypeId)) std::wstring const & CustomTypeId;
        inline std::wstring const & get_CustomTypeId() const { return customTypeId_; }
        
        FABRIC_FIELDS_04(propertyName_, typeId_, sizeInBytes_, customTypeId_);

        BEGIN_BASE_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(propertyName_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(typeId_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(customTypeId_)
        END_DYNAMIC_SIZE_ESTIMATION()

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::TypeId, typeId_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::SizeInBytes, sizeInBytes_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::CustomTypeId, customTypeId_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    protected:
        std::wstring propertyName_;
        ::FABRIC_PROPERTY_TYPE_ID typeId_;
        ULONG sizeInBytes_;
        std::wstring customTypeId_;
    };
}
