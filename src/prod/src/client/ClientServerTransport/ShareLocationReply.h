// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class ShareLocationReply : public Serialization::FabricSerializable
        {
        public:
            ShareLocationReply();
            ShareLocationReply(std::wstring const & shareLocation);

            __declspec(property(get=get_ShareLocation)) std::wstring const & ShareLocation;
            inline std::wstring const & get_ShareLocation() const { return shareLocation_; };            

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            FABRIC_FIELDS_01(shareLocation_);

        private:
            std::wstring shareLocation_;
        };
    }
}
