// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class FileMetadata : public Store::StoreData
        {
            DEFAULT_COPY_CONSTRUCTOR(FileMetadata)
            DEFAULT_MOVE_CONSTRUCTOR(FileMetadata)
        public:
            FileMetadata();
            FileMetadata(std::wstring const & relativeLocation);
            FileMetadata(std::wstring const & relativeLocation, StoreFileVersion const currentVersion, FileState::Enum const state, CopyDescription const & copyDesc);
            FileMetadata(std::wstring const & relativeLocation, StoreFileVersion const currentVersion, FileState::Enum const state, Common::Guid const & uploadRequestId);
            virtual ~FileMetadata();

            __declspec(property(get=get_RelativeLocation)) std::wstring const & StoreRelativeLocation;
            std::wstring const & get_RelativeLocation() const { return storeRelativeLocation_; }

            __declspec(property(get=get_CurrentVersion, put=set_CurrentVersion)) StoreFileVersion CurrentVersion;
            StoreFileVersion get_CurrentVersion() const { return currentVersion_; }
            void set_CurrentVersion(StoreFileVersion const & value) { currentVersion_ = value; }

            __declspec(property(get=get_PreviousVersion, put=set_PreviousVersion)) StoreFileVersion PreviousVersion;
            StoreFileVersion get_PreviousVersion() const { return previousVersion_; }
            void set_PreviousVersion(StoreFileVersion const & value) { previousVersion_ = value; }

            __declspec(property(get=get_State, put=set_State)) FileState::Enum State;
            FileState::Enum get_State() const { return state_; }
            void set_State(FileState::Enum const value) { state_ = value; }

            /* When this property is set the secondary will try to perform local copy from the file specified
               in the description */
            __declspec(property(get = get_CopyDesc, put = set_CopyDesc)) CopyDescription CopyDesc;
            CopyDescription get_CopyDesc() const { return copyDesc_; }
            void set_CopyDesc(CopyDescription const & value) { copyDesc_ = value; }

            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const { return *(Constants::StoreType::FileMetadata); }

            __declspec(property(get = get_UploadRequestId, put = set_UploadRequestId)) Common::Guid UploadRequestId;
            Common::Guid get_UploadRequestId() const { return uploadRequestId_; }
            void set_UploadRequestId(Common::Guid const & value) { uploadRequestId_ = value; }

            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_06(storeRelativeLocation_, currentVersion_, previousVersion_, state_, copyDesc_, uploadRequestId_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            std::wstring storeRelativeLocation_;
            StoreFileVersion currentVersion_;
            StoreFileVersion previousVersion_;
            FileState::Enum state_;
            CopyDescription copyDesc_;
            Common::Guid uploadRequestId_;
        };
    }
}
