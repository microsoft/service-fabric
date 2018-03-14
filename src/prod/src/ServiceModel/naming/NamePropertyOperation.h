// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamePropertyOperation : public Serialization::FabricSerializable
    {
    public:
        NamePropertyOperation();

        NamePropertyOperation(
            NamePropertyOperationType::Enum operationType,
            std::wstring const & propertyName);

        NamePropertyOperation(
            NamePropertyOperationType::Enum operationType,
            std::wstring const & propertyName,
            ::FABRIC_PROPERTY_TYPE_ID propertyTypeId,
            std::vector<byte> && value);

        NamePropertyOperation(
            NamePropertyOperationType::Enum operationType,
            std::wstring const & propertyName,
            ::FABRIC_PROPERTY_TYPE_ID propertyTypeId,
            std::vector<byte> && value,
            std::wstring && customTypeId);

        __declspec(property(get=get_OperationType)) NamePropertyOperationType::Enum OperationType;
        NamePropertyOperationType::Enum get_OperationType() const { return operationType_; }
        
        __declspec(property(get=get_PropertyName)) std::wstring const & PropertyName;
        std::wstring const & get_PropertyName() const { return propertyName_; }
        
        __declspec(property(get=get_PropertyTypeId)) ::FABRIC_PROPERTY_TYPE_ID const & PropertyTypeId;
        ::FABRIC_PROPERTY_TYPE_ID const & get_PropertyTypeId() const { return propertyTypeId_; }
        
        __declspec(property(get=get_CustomTypeId)) std::wstring const & CustomTypeId;
        std::wstring const & get_CustomTypeId() const { return customTypeId_; }

        __declspec(property(get=get_IsWrite)) bool IsWrite;
        bool get_IsWrite() const 
        { 
            return (operationType_ == NamePropertyOperationType::PutProperty || 
                    operationType_ == NamePropertyOperationType::DeleteProperty ||
                    operationType_ == NamePropertyOperationType::PutCustomProperty); 
        }

        __declspec(property(get=get_IsCheck)) bool IsCheck;
        bool get_IsCheck() const 
        { 
            return (operationType_ == NamePropertyOperationType::CheckExistence || 
                    operationType_ == NamePropertyOperationType::CheckSequence ||
                    operationType_ == NamePropertyOperationType::CheckValue); 
        }

        // Parameters specific to different operations
        __declspec(property(get=get_BoolParam, put=set_BoolParam)) bool BoolParam;
        bool get_BoolParam() const { return boolParam_; }
        void set_BoolParam(bool value) { boolParam_ = value; }

        __declspec(property(get=get_Int64Param, put=set_Int64Param)) _int64 Int64Param;
        _int64 get_Int64Param() const { return int64Param_; }
        void set_Int64Param(_int64 value) { int64Param_ = value; }

        __declspec(property(get=get_BytesParam)) std::vector<byte> const & BytesParam;
        std::vector<byte> const & get_BytesParam() const { return bytesParam_; }
        
        std::wstring && TakePropertyName() { return std::move(propertyName_); }
        std::vector<byte> && TakeBytesParam() { return std::move(bytesParam_); }

        bool IsValid() const;
        bool ValidatePropertyName() const;
        void WriteToEtw(uint16 contextSequenceId) const;
        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        std::wstring ToString() const;

        FABRIC_FIELDS_07(operationType_, propertyName_, propertyTypeId_, boolParam_, int64Param_, bytesParam_, customTypeId_);

    protected:
        NamePropertyOperationType::Enum operationType_;
        std::wstring propertyName_;
        ::FABRIC_PROPERTY_TYPE_ID propertyTypeId_;
        bool boolParam_;
        _int64 int64Param_;

        // C2621
        std::vector<byte> bytesParam_;
        std::wstring customTypeId_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Naming::NamePropertyOperation);
