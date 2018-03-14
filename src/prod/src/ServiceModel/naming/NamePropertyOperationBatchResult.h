// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamePropertyOperationBatchResult : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_ASSIGNMENT(NamePropertyOperationBatchResult)
    public:
        NamePropertyOperationBatchResult()
            : error_(Common::ErrorCodeValue::Success) 
            , failedOperationIndex_(FABRIC_INVALID_OPERATION_INDEX)
            , metadata_()
            , properties_()
            , propertiesNotFound_()
        {
        }

        NamePropertyOperationBatchResult(NamePropertyOperationBatchResult && batchResult)
            : error_(batchResult.Error) 
            , failedOperationIndex_(std::move(batchResult.FailedOperationIndex))
            , metadata_(std::move(batchResult.Metadata))
            , properties_(std::move(batchResult.Properties))
            , propertiesNotFound_(std::move(batchResult.PropertiesNotFound))
        {
        }

        __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
        __declspec(property(get=get_FailedOperationIndex)) ULONG const & FailedOperationIndex;
        __declspec(property(get=get_Metadata)) std::vector<NamePropertyMetadataResult> const & Metadata;
        __declspec(property(get=get_Properties)) std::vector<NamePropertyResult> const & Properties;
        __declspec(property(get=get_PropertiesNotFound)) std::vector<ULONG> const & PropertiesNotFound;

        inline Common::ErrorCode const & get_Error() const { return error_; }
        inline ULONG const & get_FailedOperationIndex() const { return failedOperationIndex_; }
        inline std::vector<NamePropertyMetadataResult> const & get_Metadata() const { return metadata_; }
        inline std::vector<NamePropertyResult> const & get_Properties() const { return properties_; }
        inline std::vector<ULONG> const & get_PropertiesNotFound() const { return propertiesNotFound_; }

        inline void SetFailedOperationIndex(Common::ErrorCode && error, size_t index) 
        { 
            // Convert here since the Naming Gateway will just forward the batch result
            // without looking at its contents
            // 
            error_ = NamingErrorCategories::ToClientError(move(error)); 
            failedOperationIndex_ = static_cast<ULONG>(index); 
        }
        inline void AddMetadata(NamePropertyMetadataResult && item) { metadata_.push_back(std::move(item)); }
        inline void AddProperty(NamePropertyResult && item) { properties_.push_back(std::move(item)); }
        inline void AddPropertyNotFound(size_t index) { propertiesNotFound_.push_back(static_cast<ULONG>(index)); }
        
        std::vector<NamePropertyResult> && TakeProperties() { return std::move(properties_); }
        std::vector<NamePropertyMetadataResult> && TakeMetadata() { return std::move(metadata_); }
        std::vector<ULONG> && TakePropertiesNotFound() { return std::move(propertiesNotFound_); }
        Common::ErrorCode && TakeError() { return std::move(error_); }

        FABRIC_FIELDS_05(error_, failedOperationIndex_, metadata_, properties_, propertiesNotFound_);

    protected:
        Common::ErrorCode error_;
        ULONG failedOperationIndex_;
        std::vector<NamePropertyMetadataResult> metadata_;
        std::vector<NamePropertyResult> properties_;
        std::vector<ULONG> propertiesNotFound_; 
    };
}

DEFINE_USER_ARRAY_UTILITY(Naming::NamePropertyResult);

