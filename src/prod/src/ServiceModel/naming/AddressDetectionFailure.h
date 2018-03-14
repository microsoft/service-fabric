// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class AddressDetectionFailure 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        AddressDetectionFailure();

        AddressDetectionFailure(
            Common::NamingUri const & name,
            PartitionKey const & partitionKey,
            Common::ErrorCodeValue::Enum const error,
            __int64 storeVersion);

        AddressDetectionFailure(AddressDetectionFailure && other);
        AddressDetectionFailure & operator = (AddressDetectionFailure && other);
        
        AddressDetectionFailure(AddressDetectionFailure const & other);
        AddressDetectionFailure & operator = (AddressDetectionFailure const & other);
        
        __declspec(property(get=get_Name)) Common::NamingUri const & Name;
        inline Common::NamingUri const & get_Name() const { return name_; }

        __declspec(property(get=get_PartitionData)) PartitionKey PartitionData;
        inline PartitionKey get_PartitionData() const { return partitionData_; }

        __declspec(property(get=get_StoreVersion)) __int64 StoreVersion;
        inline __int64 get_StoreVersion() const { return storeVersion_; }

        __declspec(property(get=get_Error)) Common::ErrorCodeValue::Enum Error;
        inline Common::ErrorCodeValue::Enum  get_Error() const { return error_; }
        
        bool IsObsolete(AddressDetectionFailure const & update) const;
        static bool IsObsolete(
            Common::ErrorCodeValue::Enum const oldError, 
            __int64 oldStoreVersion,
            Common::ErrorCodeValue::Enum const newError, 
            __int64 newStoreVersion);

        HRESULT GetHResult() const { return Common::ErrorCode(error_).ToHResult(); }
        
        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        std::wstring ToString() const;

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionData_)
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_04(name_, partitionData_, error_, storeVersion_);

    private:
        Common::NamingUri name_;
        PartitionKey partitionData_;
        Common::ErrorCodeValue::Enum error_;
        __int64 storeVersion_;
    };
 
    typedef std::shared_ptr<AddressDetectionFailure> AddressDetectionFailureSPtr;
}

DEFINE_USER_ARRAY_UTILITY(Naming::AddressDetectionFailure);
