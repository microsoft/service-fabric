// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    // This is the tombstone data format used when EnableTombstoneCleanup2
    // is set to "true"
    //
    class TombstoneData : public StoreData
    {
    public:
        TombstoneData();
        TombstoneData(
            std::wstring const & type, 
            std::wstring const & key,
            FABRIC_SEQUENCE_NUMBER lsn,
            size_t index); 

        virtual std::wstring const & get_Type() const override;

        __declspec (property(get=get_LiveEntryType)) std::wstring const & LiveEntryType;
        virtual std::wstring const & get_LiveEntryType() const;
        
        __declspec (property(get=get_LiveEntryKey)) std::wstring const & LiveEntryKey;
        virtual std::wstring const & get_LiveEntryKey() const;
        
        __declspec (property(get=get_Index)) size_t Index;
        virtual size_t get_Index() const;

        bool operator==(TombstoneData const & other) const;
        bool operator!=(TombstoneData const & other) const;
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const override;

        FABRIC_FIELDS_03(type_, key_, index_);

    protected:

        virtual std::wstring ConstructKey() const override;

    private:
        std::wstring type_;
        std::wstring key_;
        size_t index_;
    };
}
