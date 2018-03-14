// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ImageStore
    {       
        class ImageStoreAccessDescription;
        typedef std::unique_ptr<ImageStoreAccessDescription> ImageStoreAccessDescriptionUPtr;

        class ImageStoreAccessDescription
        {            
        public:
            ImageStoreAccessDescription(
                Common::AccessTokenSPtr const & accessToken,
                Common::SidSPtr const & sid,
                bool const hasReadAccess,
                bool const hasWriteAccess);
            ~ImageStoreAccessDescription();

            __declspec(property(get=get_AccessToken)) Common::AccessTokenSPtr const & AccessToken;
            inline Common::AccessTokenSPtr const & get_AccessToken() const { return accessToken_; }

            __declspec(property(get=get_Sid)) Common::SidSPtr const & Sid;
            inline Common::SidSPtr const & get_Sid() const { return sid_; }

            __declspec(property(get=get_HasReadAccess)) bool const HasReadAccess;
            inline bool const get_HasReadAccess() const { return hasReadAccess_; }

            __declspec(property(get=get_HasWriteAccess)) bool const HasWriteAccess;
            inline bool const get_HasWriteAccess() const { return hasWriteAccess_; }

            static Common::ErrorCode Create(
                Common::AccessTokenSPtr const & imageStoreAccessToken_,
                std::wstring const & localCache,
                std::wstring const & localRoot,
                __out ImageStoreAccessDescriptionUPtr & imageStoreAccessDescription);

        private:
            static Common::ErrorCode CheckAccess(                
                std::wstring const & path,
                Common::AccessTokenSPtr const & accessToken,
                DWORD const accessMask,
                __out bool & hasAccess);

        private:
             Common::AccessTokenSPtr accessToken_;
             Common::SidSPtr sid_;
             bool hasReadAccess_;
             bool hasWriteAccess_;
        };
    }
}
