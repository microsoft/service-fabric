// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PartitionKey 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DEFAULT_COPY_CONSTRUCTOR(PartitionKey)
        DEFAULT_COPY_ASSIGNMENT(PartitionKey)
    
    public:
        PartitionKey();
        
        explicit PartitionKey(__int64 intKey);
        explicit PartitionKey(std::wstring const & stringKey);
        explicit PartitionKey(std::wstring && stringKey);
        PartitionKey(PartitionKey && other);

        __declspec(property(get=get_KeyType)) FABRIC_PARTITION_KEY_TYPE KeyType;
        inline FABRIC_PARTITION_KEY_TYPE get_KeyType() const { return keyType_; }
        
        __declspec(property(get=get_Int64Key)) __int64 Int64Key;
        inline __int64 get_Int64Key() const 
        { 
            ASSERT_IFNOT(keyType_ == FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64, "Wrong key type {0} for the Int64 property.", static_cast<int>(keyType_));
            return intKey_;
        }
        
        __declspec(property(get=get_StringKey)) std::wstring StringKey;
        inline std::wstring get_StringKey() const 
        { 
            ASSERT_IFNOT(keyType_ == FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING, "Wrong key type {0} for the String property.", static_cast<int>(keyType_));
            return stringKey_;
        }

        __declspec(property(get=get_IsWholeService)) bool IsWholeService;
        inline bool get_IsWholeService() const { return isWholeService_; }        

        bool operator ==(PartitionKey const & other) const
        {
            if (KeyType != other.KeyType)
            {
                return false;
            }

            switch(KeyType)
            {
            case FABRIC_PARTITION_KEY_TYPE_INT64:
                return Int64Key == other.Int64Key;
            case FABRIC_PARTITION_KEY_TYPE_STRING:
                return StringKey == other.StringKey;
            default:
                return true;
            }
        }

        Common::ErrorCode FromPublicApi(
            FABRIC_PARTITION_KEY_TYPE keyType,
            const void * partitionKey);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        static std::string AddField(Common::TraceEvent &traceEvent, std::string const& name);
        void FillEventData(__in Common::TraceEventContext &) const;

        FABRIC_FIELDS_04(keyType_, intKey_, stringKey_, isWholeService_);

        // Used by CITs only
        static PartitionKey Test_GetWholeServicePartitionKey();

        // KeyType is initialized with None (value 1), so dynamic size is 0
        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(stringKey_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        FABRIC_PARTITION_KEY_TYPE keyType_;
        __int64 intKey_;
        std::wstring stringKey_;
        bool isWholeService_;
    };
}
