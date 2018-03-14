// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class NeighborhoodQueryRequestBody : public Serialization::FabricSerializable
    {
    public:
        NeighborhoodQueryRequestBody() 
        {
        }

        NeighborhoodQueryRequestBody(Common::StopwatchTime time) 
            : time_(time)
        {
        }

        Common::StopwatchTime get_Time() const { return this->time_; }

        FABRIC_FIELDS_01(time_);

    private:

        Common::StopwatchTime time_;
    };
}
