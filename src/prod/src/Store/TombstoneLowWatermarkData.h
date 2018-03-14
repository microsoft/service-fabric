// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class TombstoneLowWatermarkData : public Serialization::FabricSerializable
    {
    public:
        TombstoneLowWatermarkData() : operationLSN_(0) { }
        TombstoneLowWatermarkData(::FABRIC_SEQUENCE_NUMBER operationLSN) : operationLSN_(operationLSN) { }
        
        __declspec(property(get=get_OperationLSN)) ::FABRIC_SEQUENCE_NUMBER OperationLSN;
        ::FABRIC_SEQUENCE_NUMBER get_OperationLSN() const { return operationLSN_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("LSN={0}", operationLSN_);
        }

        FABRIC_FIELDS_01(operationLSN_);

    private:
        ::FABRIC_SEQUENCE_NUMBER operationLSN_;
    };
}

