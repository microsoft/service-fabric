// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class EdgeProbeMessageBody : public Serialization::FabricSerializable
    {
    public:
        EdgeProbeMessageBody()
        {
        }

        EdgeProbeMessageBody(uint version, bool isSuccDirection, int neighborhoodCount)
            :   version_(version),
                isSuccDirection_(isSuccDirection),
                neighborhoodCount_(neighborhoodCount)
        {
        }

        uint GetVersion() const
        {
            return version_;
        }

        bool IsSuccDirection() const
        {
            return isSuccDirection_;
        }

        int GetNeighborhoodCount() const
        {
            return neighborhoodCount_;
        }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const 
        {
            w.Write("version={0},isSuccDirection={1},neighborhoodCount={2}",
                version_, isSuccDirection_, neighborhoodCount_);
        }

        FABRIC_FIELDS_03(version_, isSuccDirection_, neighborhoodCount_);

    private:
        uint version_;
        bool isSuccDirection_;
        int neighborhoodCount_;
    };
}
