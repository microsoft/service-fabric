// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class PartialCopyProgressData : public StoreData
    {
    public:
        PartialCopyProgressData();
        PartialCopyProgressData(FABRIC_SEQUENCE_NUMBER lsn);

        virtual std::wstring const & get_Type() const override;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const override;

        FABRIC_FIELDS_01(lastStartLsn_);

    protected:

        virtual std::wstring ConstructKey() const override;

    private:
        FABRIC_SEQUENCE_NUMBER lastStartLsn_;
    };
}
