// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicationOperation : public Serialization::FabricSerializable
    {
        DENY_COPY(ReplicationOperation)

    public:

        ReplicationOperation() 
            : operation_()
            , type_()
            , key_()
            , sequenceNumber_(-1)
            , operationLSN_(-1)
            , newKey_()
            , bytes_()
            , lastModifiedOnPrimaryUtc_(Common::DateTime::Zero)
        {
        }

        ReplicationOperation(ReplicationOperation && other) 
            : operation_(std::move(other.operation_))
            , type_(std::move(other.type_))
            , key_(std::move(other.key_))
            , sequenceNumber_(std::move(other.sequenceNumber_))
            , operationLSN_(std::move(other.operationLSN_))
            , newKey_(std::move(other.newKey_))
            , bytes_(std::move(other.bytes_))
            , lastModifiedOnPrimaryUtc_(std::move(other.lastModifiedOnPrimaryUtc_))
        {
        }

        ReplicationOperation(
            ReplicationOperationType::Enum operation,
            std::wstring const & type,
            std::wstring const & key)
            : operation_(operation)
            , type_(type)
            , key_(key)
            , sequenceNumber_(-1)
            , operationLSN_(-1)
            , newKey_()
            , bytes_()
            , lastModifiedOnPrimaryUtc_(Common::DateTime::Zero)
        {
            ASSERT_IFNOT(operation_ == ReplicationOperationType::Delete, "Invalid ctor used: operation = {0}", operation);
        }

        ReplicationOperation(
            ReplicationOperationType::Enum operation,
            std::wstring const & type,
            std::wstring const & key,
            __in byte const * value,
            size_t valueSizeInBytes,
            FILETIME const & lastModifiedOnPrimaryUtc)
            : operation_(operation)
            , type_(type)
            , key_(key)
            , sequenceNumber_(0)
            , operationLSN_(0)
            , newKey_()
            , bytes_()
            , lastModifiedOnPrimaryUtc_(lastModifiedOnPrimaryUtc)
        {
            ASSERT_IFNOT(operation_ == ReplicationOperationType::Insert, "Invalid ctor used: operation = {0}", operation);
            bytes_.insert(bytes_.end(), value, value + valueSizeInBytes);
        }

        ReplicationOperation(
            ReplicationOperationType::Enum operation,
            std::wstring const & type,
            std::wstring const & key,
            __in byte const * value,
            size_t valueSizeInBytes,
            ::FABRIC_SEQUENCE_NUMBER operationLSN,
            FILETIME const & lastModifiedOnPrimaryUtc)
            : operation_(operation)
            , type_(type)
            , key_(key)
            , sequenceNumber_(0)
            , operationLSN_(operationLSN)
            , newKey_()
            , bytes_()
            , lastModifiedOnPrimaryUtc_(lastModifiedOnPrimaryUtc)
        {
            ASSERT_IFNOT(operation_ == ReplicationOperationType::Copy, "Invalid ctor used: operation = {0}", operation);
            bytes_.insert(bytes_.end(), value, value + valueSizeInBytes);
        }

        ReplicationOperation(
            ReplicationOperationType::Enum operation,
            std::wstring const & type,
            std::wstring const & key,
            __in KBuffer & buffer)
            : operation_(operation)
            , type_(type)
            , key_(key)
            , sequenceNumber_(0)
            , operationLSN_(-1)
            , newKey_()
            , lastModifiedOnPrimaryUtc_(Common::DateTime::Zero)
        {
            ASSERT_IFNOT(operation_ == ReplicationOperationType::Copy, "Invalid ctor used: operation = {0}", operation);
            byte * srcPtr = reinterpret_cast<byte*>(buffer.GetBuffer());
            bytes_.insert(bytes_.end(), srcPtr, srcPtr + buffer.QuerySize());
        }

        ReplicationOperation(
            ReplicationOperationType::Enum operation,
            std::wstring const & type,
            std::wstring const & key,
            std::wstring newKey,
            __in byte const * value,
            size_t valueSizeInBytes,
            FILETIME const & lastModifiedOnPrimaryUtc)
            : operation_(operation)
            , type_(type)
            , key_(key)
            , sequenceNumber_(0)
            , operationLSN_(0)
            , newKey_(newKey)
            , bytes_()
            , lastModifiedOnPrimaryUtc_(lastModifiedOnPrimaryUtc)
        {
            ASSERT_IFNOT(operation_ == ReplicationOperationType::Update, "Invalid ctor used: operation = {0}", operation);
            bytes_.insert(bytes_.end(), value, value + valueSizeInBytes);
        }

        __declspec(property(get=get_Operation)) ReplicationOperationType::Enum Operation;
        __declspec(property(get=get_Type)) std::wstring const & Type;
        __declspec(property(get=get_Key)) std::wstring const & Key;
        __declspec(property(get=get_OperationLSN)) ::FABRIC_SEQUENCE_NUMBER OperationLSN;
        __declspec(property(get=get_NewKey)) std::wstring const & NewKey;
        __declspec(property(get=get_Bytes)) std::vector<byte> const & Bytes;
        __declspec(property(get=get_Size)) size_t Size;
        __declspec(property(get = get_LastModifiedOnPrimaryUtc)) FILETIME LastModifiedOnPrimaryUtc;

        ReplicationOperationType::Enum get_Operation() const { return operation_; }
        std::wstring const & get_Type() const { return type_; }
        std::wstring const & get_Key() const { return key_; }
        ::FABRIC_SEQUENCE_NUMBER get_OperationLSN() const { return operationLSN_; }
        std::wstring const & get_NewKey() const { return newKey_; }
        std::vector<byte> const & get_Bytes() const { return bytes_; }
        FILETIME get_LastModifiedOnPrimaryUtc() const { return lastModifiedOnPrimaryUtc_.AsFileTime; }
        size_t get_Size() const
        {
            return sizeof(ReplicationOperationType::Enum) +
                sizeof(wchar_t) * type_.size() +
                sizeof(wchar_t) * key_.size() +
                sizeof(_int64) + 
                sizeof(::FABRIC_SEQUENCE_NUMBER) + 
                sizeof(wchar_t) * newKey_.size() +
                bytes_.size() +
                sizeof(Common::DateTime);
        }

        std::unique_ptr<FILETIME> GetLastModifiedOnPrimaryUtcAsUPtr() const
        {
            if (lastModifiedOnPrimaryUtc_.IsValid())
            {
                return Common::make_unique<FILETIME>(lastModifiedOnPrimaryUtc_.AsFileTime);
            }

            return nullptr;
        }
        
        Common::DateTime GetLastModifiedOnPrimaryUtcAsDateTime() const
        {
            if (lastModifiedOnPrimaryUtc_.IsValid())
            {
                return lastModifiedOnPrimaryUtc_;
            }

            return Common::DateTime::Zero;
        }

        void operator = (ReplicationOperation && other) 
        {
            operation_ = std::move(other.operation_);
            type_ = std::move(other.type_);
            key_ = std::move(other.key_);
            sequenceNumber_ = std::move(other.sequenceNumber_);
            operationLSN_ = std::move(other.operationLSN_);
            newKey_ = std::move(other.newKey_);
            bytes_ = std::move(other.bytes_);
            lastModifiedOnPrimaryUtc_ = std::move(other.lastModifiedOnPrimaryUtc_);
        }

        FABRIC_FIELDS_08(operation_, type_, key_, sequenceNumber_, operationLSN_, newKey_, bytes_, lastModifiedOnPrimaryUtc_);

    private:
        ReplicationOperationType::Enum operation_;
        std::wstring type_;
        std::wstring key_;
        _int64 sequenceNumber_; // only here for back-compat with V1
        ::FABRIC_SEQUENCE_NUMBER operationLSN_;
        std::wstring newKey_;
        std::vector<byte> bytes_;
        Common::DateTime lastModifiedOnPrimaryUtc_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Store::ReplicationOperation);
