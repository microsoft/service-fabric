// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ClaimsCollection : public Serialization::FabricSerializable
    {
    public:
        ClaimsCollection(); 

        ClaimsCollection(std::vector<Claim> && claimsList);
         
        __declspec(property(get=get_ValueType)) std::vector<Claim> const & ClaimsList;
        std::vector<Claim> const & get_ValueType() const { return claimsList_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(claimsList_);
    private:
        std::vector<Claim> claimsList_;
    };
}
