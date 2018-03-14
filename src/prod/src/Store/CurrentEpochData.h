// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class CurrentEpochData : public Serialization::FabricSerializable
    {
    public:
        CurrentEpochData() { }
        CurrentEpochData(::FABRIC_EPOCH epoch) : epoch_(epoch) { }

        __declspec(property(get=get_Epoch)) ::FABRIC_EPOCH const & Epoch;
        ::FABRIC_EPOCH const & get_Epoch() { return epoch_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("{0}.{1:X}", epoch_.DataLossNumber, epoch_.ConfigurationNumber);
        }

        FABRIC_FIELDS_01(epoch_);

    private:
        ::FABRIC_EPOCH epoch_;
    };
}
