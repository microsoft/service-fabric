// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {        
        class CopyRequest : public ServiceModel::ClientServerMessageBody
        {
        public:
            CopyRequest();
            CopyRequest(
                std::wstring const & sourceRelativePath,
                StoreFileVersion const & sourceFileVersion,
                std::wstring const & destinationRelativePath,
                bool const shouldOverwrite);

            __declspec(property(get = get_SourceStoreRelativePath)) std::wstring const & SourceStoreRelativePath;
            inline std::wstring const & get_SourceStoreRelativePath() const { return sourceStoreRelativePath_; }

            __declspec(property(get = get_SourceFileVersion)) StoreFileVersion const & SourceFileVersion;
            inline StoreFileVersion const & get_SourceFileVersion() const { return sourceFileVersion_; }

            __declspec(property(get = get_DestinationStoreRelativePath)) std::wstring const & DestinationStoreRelativePath;
            inline std::wstring const & get_DestinationStoreRelativePath() const { return destinationStoreRelativePath_; }

            __declspec(property(get = get_ShouldOverwrite)) bool const ShouldOverwrite;
            inline bool const get_ShouldOverwrite() const { return shouldOverwrite_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_04(sourceStoreRelativePath_, sourceFileVersion_, destinationStoreRelativePath_, shouldOverwrite_);

        private:            
            std::wstring sourceStoreRelativePath_;
            StoreFileVersion sourceFileVersion_;
            std::wstring destinationStoreRelativePath_;
            bool shouldOverwrite_;
        };
    }
}
