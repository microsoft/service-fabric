// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class VoterStoreHeader : public Transport::MessageHeader<Transport::MessageHeaderId::VoterStore>, public Serialization::FabricSerializable
    {
    public:
        VoterStoreHeader()
        {
        }

        VoterStoreHeader(std::vector<NodeInstance> && downVoters)
            : downVoters_(std::move(downVoters))
        {
        }

        __declspec(property(get = getDownVoters)) std::vector<NodeInstance> const & DownVoters;
        std::vector<NodeInstance> const & getDownVoters() const
        {
            return downVoters_;
        }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write(downVoters_);
        }

        FABRIC_FIELDS_01(downVoters_);

    private:
        std::vector<NodeInstance> downVoters_;
    };
}
