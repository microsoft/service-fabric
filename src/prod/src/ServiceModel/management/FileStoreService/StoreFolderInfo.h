// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class StoreFolderInfo
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
        {
            DEFAULT_COPY_CONSTRUCTOR(StoreFolderInfo)

        public:
            StoreFolderInfo();

            StoreFolderInfo(
                std::wstring const & storeRelativePath, 
                unsigned long long const & fileCount);

            StoreFolderInfo const & operator = (StoreFolderInfo const & other);
            StoreFolderInfo const & operator = (StoreFolderInfo && other);

            bool operator == (StoreFolderInfo const & other) const;
            bool operator != (StoreFolderInfo const & other) const;

            __declspec(property(get = get_StoreRelativePath)) std::wstring const & StoreRelativePath;
            std::wstring const & get_StoreRelativePath() const { return storeRelativePath_; }

            __declspec(property(get = get_StoreFolderFileCount)) unsigned long long const & FileCount;
            uint64 get_StoreFolderFileCount() const { return fileCount_; }

            void ToPublicApi(
                __in Common::ScopedHeap & heap, 
                __out FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_ITEM & publicImageStoreFolderInfo) const;

            std::wstring ToString() const;
            virtual void WriteTo(
                Common::TextWriter & w, 
                Common::FormatOptions const &) const;

            FABRIC_FIELDS_02(
                storeRelativePath_, 
                fileCount_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StoreRelativePath, storeRelativePath_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::FileCount, fileCount_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(storeRelativePath_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(fileCount_)
            END_DYNAMIC_SIZE_ESTIMATION()

        private:
            std::wstring storeRelativePath_;
            uint64 fileCount_;
        };
    }
}
