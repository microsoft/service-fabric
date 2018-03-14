// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class FileStorePartitionInfo : public Serialization::FabricSerializable
        {
        public:
            FileStorePartitionInfo() {}
            FileStorePartitionInfo(std::wstring const & primaryServiceLocation) : primaryLocation_(primaryServiceLocation)
            {
            }

            __declspec(property(get=get_PrimaryLocation)) std::wstring const & PrimaryLocation;
            inline std::wstring const & get_PrimaryLocation() const { return primaryLocation_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
            {
                w.WriteLine("FileStorePartitionInfo{{0}}", primaryLocation_);
            }

            FABRIC_FIELDS_01(primaryLocation_);

        private:
            std::wstring primaryLocation_;
        };
    }
}
