// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Federation/VoteOwnerState.h"

namespace Federation
{
    struct VoteOwnerState : public Serialization::FabricSerializable
    {
    public:
        VoteOwnerState()
            :   globalTicket_(0),
                superTicket_(0)
        {
        }

        __declspec (property(get=getGlobalTicket,put=setGlobalTicket)) Common::DateTime GlobalTicket;
        Common::DateTime getGlobalTicket() const
        {
            return globalTicket_;
        }
        void setGlobalTicket(Common::DateTime value)
        {
            globalTicket_ = value;
        }
            
        __declspec (property(get=getSuperTicket,put=setSuperTicket)) Common::DateTime SuperTicket;
        Common::DateTime getSuperTicket() const
        {
            return superTicket_;
        }
        void setSuperTicket(Common::DateTime value)
        {
            superTicket_ = value;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const 
        { 
            w.Write("{0},{1}", globalTicket_, superTicket_);
        }

        FABRIC_FIELDS_02(globalTicket_, superTicket_);

    private:
        Common::DateTime globalTicket_;
        Common::DateTime superTicket_;
    };
}
