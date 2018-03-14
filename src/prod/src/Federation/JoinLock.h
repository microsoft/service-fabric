// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct JoinLock : public Serialization::FabricSerializable
    {
    public:
        JoinLock()
            : id_(0), hoodRange_(), isRenew_(false)
        {
        }

        JoinLock(int64 id, NodeIdRange range, bool isRenew = false)
            : id_(id), hoodRange_(range), isRenew_(isRenew)
        {
        }

        __declspec(property(get=getId)) int64 Id;
        __declspec(property(get=getHoodRange)) NodeIdRange HoodRange;
        __declspec(property(get=getIsRenew)) bool IsRenew;

        int64 getId() const { return id_; }
        NodeIdRange getHoodRange() const { return hoodRange_; }
        bool getIsRenew() const { return isRenew_; }

        FABRIC_FIELDS_03(id_, hoodRange_, isRenew_);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const & /*option*/) const 
        {
            w.Write("{0} {1} {2}", id_, hoodRange_, isRenew_);
        }

    private:
        int64 id_;
        NodeIdRange hoodRange_;
        bool isRenew_;
    };
}
