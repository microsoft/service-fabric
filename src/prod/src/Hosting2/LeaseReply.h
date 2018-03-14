// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class LeaseReply : public Serialization::FabricSerializable
    {
    public:
        LeaseReply() = default;
        LeaseReply(Common::TimeSpan ttl) : ttl_(ttl)
        {
        }

        Common::TimeSpan TTL() const { return ttl_; }

        FABRIC_FIELDS_01(ttl_);

    private:
        Common::TimeSpan ttl_;
    };
}
