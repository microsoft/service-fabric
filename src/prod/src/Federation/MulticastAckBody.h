// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class MulticastAckBody : public Serialization::FabricSerializable
    {
    public:
        MulticastAckBody() 
        {
        }

        MulticastAckBody(std::vector<NodeInstance> const & failed, std::vector<NodeInstance> const & pending) 
            : failed_(failed), pending_(pending)
        {
        }

        void MoveResults(__out std::vector<NodeInstance> & failed, __out std::vector<NodeInstance> & pending)
        {
            failed = std::move(failed_);
            pending = std::move(pending_);
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
            w.Write("failed: {0}, pending: {1}", failed_, pending_);
        }

        FABRIC_FIELDS_02(failed_, pending_);

    private:
        std::vector<NodeInstance> failed_;
        std::vector<NodeInstance> pending_;
    };
}
