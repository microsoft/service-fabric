// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class ImageStoreBaseRequest : public ServiceModel::ClientServerMessageBody
        {
        public:
            ImageStoreBaseRequest();
            ImageStoreBaseRequest(std::wstring const & storeRelativePath);

            __declspec(property(get=get_StoreRelativePath)) std::wstring const & StoreRelativePath;
            inline std::wstring const & get_StoreRelativePath() const { return storeRelativePath_; }
            
            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            FABRIC_FIELDS_01(storeRelativePath_);

        private:
            std::wstring storeRelativePath_;            
        };
    }
}
