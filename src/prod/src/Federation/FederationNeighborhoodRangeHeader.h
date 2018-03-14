// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    // Structure represents information about a particular instance of a node.
    struct FederationNeighborhoodRangeHeader : public Transport::MessageHeader<Transport::MessageHeaderId::FederationNeighborhoodRange>, public Serialization::FabricSerializable
    {
    public:
        FederationNeighborhoodRangeHeader()
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
        FederationNeighborhoodRangeHeader(NodeIdRange const& range)
             : range_(range)
        {
        }

        FederationNeighborhoodRangeHeader & operator=(FederationNeighborhoodRangeHeader const & rhs)
        {
            if (this != &rhs)
            {
                this->range_ = rhs.range_;
            }

            return *this;
        }

        FABRIC_FIELDS_01(range_);

        // Properties
        __declspec (property(get=getRange)) NodeIdRange Range;
        
        //Getter functions for properties.
        NodeIdRange getRange() const { return range_; }
        
        void WriteTo (Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write(range_);
        }

    private:       
        NodeIdRange range_;
    };
}
