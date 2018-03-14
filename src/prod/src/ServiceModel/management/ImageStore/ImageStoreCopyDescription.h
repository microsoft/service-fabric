// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageStore
    {
        class ImageStoreCopyDescription
            : public Common::IFabricJsonSerializable
        {
            DENY_COPY(ImageStoreCopyDescription);

        public:
            ImageStoreCopyDescription();
            ImageStoreCopyDescription(
                std::wstring const & remoteSource,
                std::wstring const & remoteDestination,
                Common::StringCollection const & skipFiles,
                Management::ImageStore::CopyFlag::Enum copyFlag,
                bool checkMarkFile);

            virtual ~ImageStoreCopyDescription() {};

            Common::ErrorCode FromPublicApi(FABRIC_IMAGE_STORE_COPY_DESCRIPTION const & copyDes);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_IMAGE_STORE_COPY_DESCRIPTION &) const;

            __declspec(property(get = get_RemoteResource, put = set_RemoteSource)) std::wstring const & RemoteSource;
            std::wstring const & get_RemoteResource() const { return remoteSource_; }
            void set_RemoteSource(std::wstring const & value) { remoteSource_ = value; }

            __declspec(property(get = get_RemoteDestination, put = set_RemoteDestination)) std::wstring const & RemoteDestination;
            std::wstring const & get_RemoteDestination() const { return remoteDestination_; }
            void set_RemoteDestination(std::wstring const & value) { remoteDestination_ = value; }

            __declspec(property(get = get_SkipFiles, put = set_SkipFiles)) Common::StringCollection & SkipFiles;
            Common::StringCollection const & get_SkipFiles() const { return skipFiles_; }
            void set_SkipFiles(Common::StringCollection && value) { skipFiles_ = move(value); }

            __declspec(property(get = get_CopyFlag, put = set_CopyFlag)) Management::ImageStore::CopyFlag::Enum CopyFlag;
            Management::ImageStore::CopyFlag::Enum get_CopyFlag() const { return copyFlag_; }
            void set_CopyFlag(Management::ImageStore::CopyFlag::Enum value) { copyFlag_ = value; }

            __declspec(property(get = get_CheckMarkFile, put = set_CheckMarkFile)) bool CheckMarkFile;
            bool get_CheckMarkFile() const { return checkMarkFile_; }
            void set_CheckMarkFile(bool value) { checkMarkFile_ = value; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RemoteSource, remoteSource_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RemoteDestination, remoteDestination_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::SkipFiles, skipFiles_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::CopyFlag, copyFlag_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::CheckMarkFile, checkMarkFile_)
                END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring remoteSource_;
            std::wstring remoteDestination_;
            Common::StringCollection skipFiles_;
            Management::ImageStore::CopyFlag::Enum copyFlag_;
            bool checkMarkFile_;
        };
    }
}
