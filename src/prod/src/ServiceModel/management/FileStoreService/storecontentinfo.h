// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class StoreContentInfo
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DEFAULT_COPY_CONSTRUCTOR(StoreContentInfo)

        public:
            StoreContentInfo();

            StoreContentInfo(
                std::vector<StoreFileInfo> const & storeFiles,
                std::vector<StoreFolderInfo> const & storeFolders);

            StoreContentInfo & operator = (StoreContentInfo && other);

            __declspec(property(get = get_StoreFiles, put = put_StoreFiles)) std::vector<StoreFileInfo> const & StoreFiles;
            std::vector<StoreFileInfo> const & get_StoreFiles() const { return storeFiles_; }
            void put_StoreFiles(std::vector<StoreFileInfo> && storeFiles) { this->storeFiles_ = std::move(storeFiles); }

            __declspec(property(get = get_StoreFolders, put = put_StoreFolders)) std::vector<StoreFolderInfo> const & StoreFolders;
            std::vector<StoreFolderInfo> const & get_StoreFolders() const { return storeFolders_; }
            void put_StoreFolders(std::vector<StoreFolderInfo> && storeFolders) { this->storeFolders_ = std::move(storeFolders); }

            std::wstring ToString() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_02(storeFiles_, storeFolders_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StoreFiles, storeFiles_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StoreFolders, storeFolders_)
                END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            std::vector<StoreFileInfo> storeFiles_;
            std::vector<StoreFolderInfo> storeFolders_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::FileStoreService::StoreFileInfo);
DEFINE_USER_ARRAY_UTILITY(Management::FileStoreService::StoreFolderInfo);
