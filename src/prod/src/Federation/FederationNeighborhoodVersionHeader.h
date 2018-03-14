// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    // Structure represents information about a particular instance of a node.
    struct FederationNeighborhoodVersionHeader : public Transport::MessageHeader<Transport::MessageHeaderId::FederationNeighborhoodVersion>, public Serialization::FabricSerializable
    {
    public:
        FederationNeighborhoodVersionHeader()
        {
        }
        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //        Takes a large integer and sets it accordingly for this object.
        //
        // Arguments:
        //        from - the large integer.
        //
        FederationNeighborhoodVersionHeader(uint version)
             : version_(version)
        {
        }

        FederationNeighborhoodVersionHeader & operator=(FederationNeighborhoodVersionHeader const & rhs)
        {
            if (this != &rhs)
            {
                this->version_ = rhs.version_;
            }

            return *this;
        }

        FABRIC_FIELDS_01(version_);

        // Properties
        __declspec (property(get=getVersion)) uint Version;
        
        //Getter functions for properties.
        uint getVersion() const { return version_; }
        
        void WriteTo (Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write(version_);
        }

    private:       
        uint version_;
    };
}
