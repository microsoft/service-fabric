// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class VersionNumber : public Store::StoreData
        {
        public:
            VersionNumber();
            VersionNumber(uint64 const versionNumber);
            virtual ~VersionNumber();

            __declspec(property(get=get_LastDeletedVersionNumber, put=set_LastDeletedVersionNumber)) uint64 LastDeletedVersionNumber;
            uint64 get_LastDeletedVersionNumber() const { return versionNumber_; }
            void set_LastDeletedVersionNumber(uint64 const value) { versionNumber_ = value; }
            
            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const { return *(Constants::StoreType::LastDeletedVersionNumber); }

            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;        

            FABRIC_FIELDS_01(versionNumber_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            uint64 versionNumber_;
        };
    }
}
