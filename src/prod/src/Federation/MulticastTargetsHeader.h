// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class MulticastTargetsHeader : public Transport::MessageHeader<Transport::MessageHeaderId::MulticastTargets>, public Serialization::FabricSerializable
    {
    public:
        MulticastTargetsHeader()
        {
        }

        MulticastTargetsHeader(std::vector<NodeInstance> const & targets)
             : targets_(targets)
        {
        }

        std::vector<NodeInstance> MoveTargets()
        {
            return std::move(targets_);
        }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const 
        { 
            w << targets_;
        }

        FABRIC_FIELDS_01(targets_);

    private:
        std::vector<NodeInstance> targets_;
    };
}
