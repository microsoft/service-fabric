// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class StoreFileInfo;
        typedef std::map<std::wstring, StoreFileInfo, Common::IsLessCaseInsensitiveComparer<std::wstring>> StoreFileInfoMap;

        class StoreFileInfo 
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
        {
            DEFAULT_COPY_CONSTRUCTOR(StoreFileInfo)

        public:
            StoreFileInfo();

            StoreFileInfo(
                std::wstring const & storeRelativePath, 
                StoreFileVersion const & version, 
                uint64 size, 
                Common::DateTime const & modifiedDate);

            StoreFileInfo const & operator = (StoreFileInfo const & other);
            StoreFileInfo const & operator = (StoreFileInfo && other);

            bool operator == (StoreFileInfo const & other) const;
            bool operator != (StoreFileInfo const & other) const;

            __declspec(property(get = get_StoreRelativePath)) std::wstring const & StoreRelativePath;
            std::wstring const & get_StoreRelativePath() const { return storeRelativePath_; }

            __declspec(property(get = get_StoreFileVersion)) StoreFileVersion const & Version;
            StoreFileVersion const & get_StoreFileVersion() const { return version_; }

            __declspec(property(get = get_StoreFileSize)) uint64 const & Size;
            uint64 get_StoreFileSize() const { return size_; }

            __declspec(property(get = get_StoreFileModifiedDate)) Common::DateTime const & ModifiedDate;
            Common::DateTime const & get_StoreFileModifiedDate() const { return modifiedDate_; }

            void ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_ITEM & publicImageStoreFileInfo) const;

            std::wstring ToString() const;
            virtual void WriteTo(
                Common::TextWriter & w, 
                Common::FormatOptions const &) const;

            FABRIC_FIELDS_04(storeRelativePath_, version_, size_, modifiedDate_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StoreRelativePath, storeRelativePath_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::FileVersion, version_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::FileSize, size_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ModifiedDate, modifiedDate_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(storeRelativePath_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(version_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(size_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(modifiedDate_)
            END_DYNAMIC_SIZE_ESTIMATION()

        private:
            std::wstring storeRelativePath_;
            StoreFileVersion version_;
            uint64  size_;
            Common::DateTime modifiedDate_;
        };
    }
}
