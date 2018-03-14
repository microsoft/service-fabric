// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamePropertyMetadataResult : public NamePropertyMetadata
    {
        DEFAULT_COPY_CONSTRUCTOR(NamePropertyMetadataResult)
        DEFAULT_COPY_ASSIGNMENT(NamePropertyMetadataResult)
    
    public:
        NamePropertyMetadataResult();

        NamePropertyMetadataResult(NamePropertyMetadataResult && other);

        NamePropertyMetadataResult(
            Common::NamingUri const & name,
            NamePropertyMetadata && metadata,
            Common::DateTime lastModifiedUtc,
            _int64 sequenceNumber,
            size_t operationIndex);

        __declspec(property(get=get_LastModifiedUtc)) Common::DateTime LastModifiedUtc;
        __declspec(property(get=get_SequenceNumber)) _int64 SequenceNumber;
        __declspec(property(get=get_Name)) Common::NamingUri const & Name;
        __declspec(property(get=get_OperationIndex)) ULONG OperationIndex;

        inline Common::DateTime get_LastModifiedUtc() const { return lastModifiedUtc_; }
        inline _int64 get_SequenceNumber() const { return sequenceNumber_; }
        inline Common::NamingUri const & get_Name() const { return name_; }
        inline ULONG get_OperationIndex() const { return operationIndex_; }

        void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_NAMED_PROPERTY_METADATA &) const;

        FABRIC_FIELDS_04(lastModifiedUtc_, sequenceNumber_, operationIndex_, name_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(lastModifiedUtc_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
        END_DYNAMIC_SIZE_ESTIMATION()

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::LastModifiedUtcTimestamp, lastModifiedUtc_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::SequenceNumber, sequenceNumber_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Parent, name_)
        END_JSON_SERIALIZABLE_PROPERTIES()
        
    private:
        Common::DateTime lastModifiedUtc_;
        _int64 sequenceNumber_;
        ULONG operationIndex_;
        Common::NamingUri name_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Naming::NamePropertyMetadataResult);
