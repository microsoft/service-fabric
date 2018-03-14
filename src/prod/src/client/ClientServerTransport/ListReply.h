// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class ListReply : public Serialization::FabricSerializable
        {
            DENY_COPY(ListReply)

        public:
            ListReply();

            ListReply(
                std::vector<StoreFileInfo> const & files,
                std::vector<StoreFolderInfo> const & folders,
                std::wstring const & primaryStoreShareLocation);

            ListReply(
                std::vector<StoreFileInfo> && files,
                std::vector<StoreFolderInfo> && folders,
                std::wstring const & primaryStoreShareLocation);
            ListReply(
                std::vector<StoreFileInfo> const & files,
                std::vector<StoreFolderInfo> const & folders,
                std::wstring const & primaryStoreShareLocation,
                std::wstring const & continuationToken);
            
            ListReply(
                std::vector<StoreFileInfo> && files,
                std::vector<StoreFolderInfo> && folders,
                std::wstring const & primaryStoreShareLocation,
                std::wstring && continuationToken);

            __declspec(property(get = get_Files)) std::vector<StoreFileInfo> const & Files;
            inline std::vector<StoreFileInfo> const & get_Files() const { return files_; }

            __declspec(property(get = get_Folders)) std::vector<StoreFolderInfo> const & Folders;
            inline std::vector<StoreFolderInfo> const & get_Folders() const { return folders_; }

            __declspec(property(get = get_PrimaryStoreShareLocation)) std::wstring const & PrimaryStoreShareLocation;
            inline std::wstring const & get_PrimaryStoreShareLocation() const { return primaryStoreShareLocation_; }

            __declspec(property(get = get_ContinuationToken)) std::wstring const & ContinuationToken;
            inline std::wstring const & get_ContinuationToken() const { return continuationToken_; }

            FABRIC_FIELDS_04(files_, primaryStoreShareLocation_, folders_, continuationToken_);

        private:
            std::vector<StoreFileInfo> files_;
            std::vector<StoreFolderInfo> folders_;
            std::wstring primaryStoreShareLocation_;
            std::wstring continuationToken_;
        };
    }
}
