// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ProgressVectorEntry : public Serialization::FabricSerializable
    {
    public:
        ProgressVectorEntry();

        ProgressVectorEntry(
            ::FABRIC_EPOCH const & epoch, 
            ::FABRIC_SEQUENCE_NUMBER lastOperationLSN);
        
        __declspec(property(get=get_Epoch)) ::FABRIC_EPOCH const & Epoch;
        __declspec(property(get=get_LastOperationLSN)) ::FABRIC_SEQUENCE_NUMBER LastOperationLSN;

        ::FABRIC_EPOCH const & get_Epoch() const { return epoch_; };
        ::FABRIC_SEQUENCE_NUMBER get_LastOperationLSN() const { return lastOperationLSN_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(epoch_, lastOperationLSN_);

    private:
        ::FABRIC_EPOCH epoch_;
        ::FABRIC_SEQUENCE_NUMBER lastOperationLSN_;
    };

    class ProgressVectorData : public Serialization::FabricSerializable
    {
    public:
        ProgressVectorData();
        ProgressVectorData(std::vector<ProgressVectorEntry> &&);

        __declspec(property(get=get_ProgressVector)) std::vector<ProgressVectorEntry> & ProgressVector;
        std::vector<ProgressVectorEntry> & get_ProgressVector() { return progressVector_; }

        bool TryTruncate(::FABRIC_SEQUENCE_NUMBER upToLsn);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(progressVector_);

    private:
        std::vector<ProgressVectorEntry> progressVector_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Store::ProgressVectorEntry);

