// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class CopyDescription : public Serialization::FabricSerializable
        {
            DEFAULT_COPY_CONSTRUCTOR(CopyDescription)
            
        public:
            CopyDescription();
            CopyDescription(std::wstring const & storeRelativePath, StoreFileVersion const & version);
            CopyDescription const & operator = (CopyDescription const & other);
            CopyDescription const & operator = (CopyDescription && other);

            bool operator == (CopyDescription const & other) const;
            bool operator != (CopyDescription const & other) const;

            __declspec(property(get = get_RelativeLocation)) std::wstring const & StoreRelativeLocation;
            std::wstring const & get_RelativeLocation() const { return storeRelativeLocation_; }

            __declspec(property(get = get_Version)) StoreFileVersion Version;
            StoreFileVersion get_Version() const { return version_; }

            bool IsEmpty() const;

            std::wstring ToString() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_02(storeRelativeLocation_, version_);

        private:
            std::wstring storeRelativeLocation_;
            StoreFileVersion version_;
        };
    }
}
