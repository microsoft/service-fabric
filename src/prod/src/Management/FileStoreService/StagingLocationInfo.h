// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class StagingLocationInfo
        {
        public:
            StagingLocationInfo();
            StagingLocationInfo(StagingLocationInfo const & other);
            StagingLocationInfo const & operator = (StagingLocationInfo const & other);
            StagingLocationInfo const & operator = (StagingLocationInfo && other);
            StagingLocationInfo(std::wstring const & stagingLocation, uint64 const sequenceNumber);
            ~StagingLocationInfo();

            __declspec(property(get = get_ShareLocation)) std::wstring const & ShareLocation;
            inline std::wstring const & get_ShareLocation() const { return stagingLocation_; }

            __declspec(property(get = get_SequenceNumber)) uint64 const SequenceNumber;
            inline uint64 const get_SequenceNumber() const { return sequenceNumber_; }

            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        private:
            std::wstring stagingLocation_;
            uint64 sequenceNumber_;
        };
    }
}
