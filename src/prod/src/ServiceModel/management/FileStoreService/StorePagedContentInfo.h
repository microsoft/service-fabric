// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class StorePagedContentInfo 
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
        {
            DENY_COPY(StorePagedContentInfo)

        public:
            StorePagedContentInfo();

            StorePagedContentInfo(
                std::vector<StoreFileInfo> const & storeFiles,
                std::vector<StoreFolderInfo> const & storeFolders,
                std::wstring && continuationToken);

            StorePagedContentInfo & operator = (StorePagedContentInfo && other);

            __declspec(property(get = get_StoreFiles, put=put_StoreFiles)) std::vector<StoreFileInfo> const & StoreFiles;
            std::vector<StoreFileInfo> const & get_StoreFiles() const { return storeFiles_; }
            void put_StoreFiles(std::vector<StoreFileInfo> && storeFiles) { this->storeFiles_ = std::move(storeFiles); }

            __declspec(property(get = get_StoreFolders, put=put_StoreFolders)) std::vector<StoreFolderInfo> const & StoreFolders;
            std::vector<StoreFolderInfo> const & get_StoreFolders() const { return storeFolders_; }
            void put_StoreFolders(std::vector<StoreFolderInfo> && storeFolders) { this->storeFolders_ = std::move(storeFolders); }

            __declspec(property(get = get_ContinuationToken, put = put_ContinuationToken)) std::wstring const & ContinuationToken;
            std::wstring const & get_ContinuationToken() const { return continuationToken_; }
            void put_ContinuationToken(std::wstring && continuationToken) { this->continuationToken_ = move(continuationToken); }

            std::wstring ToString() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            Common::ErrorCode TryAdd(StoreFileInfo && file);
            Common::ErrorCode TryAdd(StoreFolderInfo && folder);
            Common::ErrorCode EscapeContinuationToken();

            FABRIC_FIELDS_03(storeFiles_, storeFolders_, continuationToken_);	
           
            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StoreFiles, storeFiles_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StoreFolders, storeFolders_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ContinuationToken, continuationToken_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(storeFiles_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(storeFolders_)
                DYNAMIC_ENUM_ESTIMATION_MEMBER(continuationToken_)
            END_DYNAMIC_SIZE_ESTIMATION()
            

        private:

            Common::ErrorCode CheckEstimatedSize(
                size_t contentSize, 
                std::wstring const & storeRelativePath);

            std::vector<StoreFileInfo> storeFiles_;
            std::vector<StoreFolderInfo> storeFolders_;
            std::wstring continuationToken_;
            size_t maxAllowedSize_;
            size_t currentSize_;
        };
    }
}

