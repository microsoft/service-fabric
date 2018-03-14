// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class UploadRequest : public ServiceModel::ClientServerMessageBody
        {
        public:
            UploadRequest();
            UploadRequest(
                std::wstring const & stagingRelativePath,
                std::wstring const & destinationRelativePath,
                bool const shouldOverwrite);

            __declspec(property(get=get_StagingRelativePath)) std::wstring const & StagingRelativePath;
            inline std::wstring const & get_StagingRelativePath() const { return stagingRelativePath_; }

#if !defined(PLATFORM_UNIX)
            __declspec(property(get=get_StoreRelativePath)) std::wstring const & StoreRelativePath;
#else
            __declspec(property(get=get_StoreRelativePath, put=set_StoreRelativePath)) std::wstring const & StoreRelativePath;
#endif
            inline std::wstring const & get_StoreRelativePath() const { return storeRelativePath_; }

#if defined(PLATFORM_UNIX)
            inline void set_StoreRelativePath(wstring &val){ storeRelativePath_ = val; }
#endif

            __declspec(property(get=get_ShouldOverwrite)) bool const ShouldOverwrite;
            inline bool const get_ShouldOverwrite() const { return shouldOverwrite_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_03(stagingRelativePath_, storeRelativePath_, shouldOverwrite_);

        private:
            std::wstring stagingRelativePath_;
            std::wstring storeRelativePath_;
            bool shouldOverwrite_;
        };
    }
}
