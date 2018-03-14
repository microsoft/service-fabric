// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class SecondaryLocationsHeader
        : public Transport::MessageHeader<Transport::MessageHeaderId::SecondaryLocations>
        , public Serialization::FabricSerializable
    {
    public:
        SecondaryLocationsHeader() : secondaryLocations_()
        {
        }

        explicit SecondaryLocationsHeader(std::vector<std::wstring> const & secondaryLocations) : secondaryLocations_(secondaryLocations)
        { 
        }

        explicit SecondaryLocationsHeader(std::vector<std::wstring> && secondaryLocations) : secondaryLocations_(move(secondaryLocations))
        { 
        }

        __declspec (property(get=get_SecondaryLocations)) std::vector<std::wstring> const & SecondaryLocations;
        std::vector<std::wstring>  const & get_SecondaryLocations() { return secondaryLocations_; }
        
        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("SecondaryLocationsHeader{");
            for (auto const & location : secondaryLocations_)
            {
                w.Write("{0},", location);
            }            
            w.Write("}");
        }

        FABRIC_FIELDS_01(secondaryLocations_);

    private:
        std::vector<std::wstring> secondaryLocations_;
    };
}
