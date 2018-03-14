// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Federation
{
    NodeId const NodeId::MinNodeId = StaticInit_MinNodeId();
    NodeId const NodeId::MaxNodeId = StaticInit_MaxNodeId();

    NodeId const & NodeId::StaticInit_MinNodeId()
    {
        static const NodeId min = NodeId(LargeInteger::StaticInit_Zero());
        return min;
    }

    NodeId const & NodeId::StaticInit_MaxNodeId()
    {
        static const NodeId max = NodeId(LargeInteger::StaticInit_MaxValue());
        return max;
    }

    NodeId::NodeId(LargeInteger const& from)
        : idValue_(from)
    {
    }

    NodeId::NodeId()
    {
    }

    bool NodeId::TryParse(wstring const& data, __out NodeId & id)
    {
        return LargeInteger::TryParse(data, id.idValue_);
    }

    LargeInteger const & NodeId::getIdValue() const
    {
        return idValue_;
    }

    bool NodeId::operator == (NodeId const& other ) const
    {
        return idValue_ == other.idValue_;
    }

    bool NodeId::operator != (NodeId const& other ) const
    {
        return idValue_ != other.idValue_;
    }

    bool NodeId::operator < (NodeId const& other ) const
    {
        return idValue_ < other.idValue_;
    }

    bool NodeId::operator >= (NodeId const& other ) const
    {
        return idValue_ >= other.idValue_;
    }

    bool NodeId::operator > (NodeId const& other ) const
    {
        return idValue_ > other.idValue_;
    }

    bool NodeId::operator <= (NodeId const& other ) const
    {
        return idValue_ <= other.idValue_;
    }

    const NodeId NodeId::operator +(LargeInteger const& value) const
    {
        return NodeId(idValue_ + value);
    }

    const NodeId NodeId::operator -(LargeInteger const& value) const
    {
        return NodeId(idValue_ - value);
    }

    void NodeId::operator++()
    {
        ++idValue_;
    }

    void NodeId::operator--() 
    {
        --idValue_;
    }

    NodeId NodeId::SuccWalk(LargeInteger const& value) const
    {
        return *this + value;
    }

    NodeId NodeId::PredWalk(LargeInteger const& value) const
    {
        return *this - value;
    }

    LargeInteger NodeId::SuccDist(NodeId const& id) const
    {
        return (id.idValue_ - idValue_);
    }

    LargeInteger NodeId::PredDist(NodeId const& id) const
    {
        return (idValue_ - id.idValue_);
    }

    LargeInteger NodeId::MinDist(NodeId const& id) const
    {
        LargeInteger succDist = SuccDist(id);
        if (succDist.IsSmallerPart())
        {
            return (succDist);
        }

        LargeInteger predDist = PredDist(id);
        return (predDist);
    }

    bool NodeId::Precedes(NodeId const& id) const
    {
        // Consider the common case to be true
        LargeInteger predDist = PredDist(id);
        if (predDist.IsSmallerPart())
        {
            return (false);
        }

        LargeInteger succDist = SuccDist(id);
        if (succDist.IsSmallerPart())
        {
            return (true);
        }

        // This can happen when the modulo size of the LargeInteger
        // is even. For example, n1=0 and n2=128 in the case of a byte
        // sized LargeInteger arrive at this point. We break the tie in
        // favor of the smaller number
        //
        // ************************************************************
        // GopalK - Important Implementation note
        // --------------------------------------
        // Though, the following is the mathematically correct behavior
        // routing table code makes the decision on whether n2 is a
        // successor of n1 solely on the succDist being less than
        // or equal to predDist in the interest of avoiding this third
        // check. This results in the anomaly of n1 considering n2 as
        // its successor and n2 considering n1 as its successor for the
        // nodes broken by this tie in the third case. This is not a
        // problem so far because nodes n1 and n2 do not have to agree
        // on this conflicting decision so far. If they do, this aspect
        // of routing table logic needs to be revisited
        // ************************************************************
        ASSERT_IFNOT(succDist == predDist, "succDist:{0} != predDist:{1}", succDist, predDist);
        return (idValue_ < id.idValue_);
    }

    NodeId NodeId::GetSuccMidPoint(NodeId const & succID) const
    {
        LargeInteger dist = SuccDist(succID);
        return (SuccWalk(dist >> 1));
    }

    NodeId NodeId::GetPredMidPoint(NodeId const & predID) const
    {
        LargeInteger dist = PredDist(predID);
        return (PredWalk((dist - LargeInteger::One) >> 1));
    }

    void NodeId::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
    {
        idValue_.WriteTo(w, options);
    }

    wstring NodeId::ToString() const
    {
        return idValue_.ToString();
    }

    void NodeId::ToPublicApi(__out FABRIC_NODE_ID & publicNodeId) const
    {
        publicNodeId = idValue_.ToPublicApi();
    }

    ErrorCode NodeId::FromPublicApi(__in FABRIC_NODE_ID const& publicNodeId)
    {
        idValue_.FromPublicApi(publicNodeId);
        return ErrorCode::Success();
    }
}
