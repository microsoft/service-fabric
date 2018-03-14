// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    using namespace std;
    using namespace Common;

    bool RoutingToken::IsMergeSafe(uint64 version) const
    {
        ASSERT_IF(version_ < version,
            "The current token version {0} is smaller than the incoming {1}",
            version_, version);
        uint64 diff = version_ - version;
        return diff < RECOVERY_INCREMENT;
    }

    void RoutingToken::WriteTo(Common::TextWriter &w, Common::FormatOptions const &) const
    {
        w.Write("{0}:{1:x}", range_, version_);
    }

    void RoutingToken::SetEmpty()
    {
        range_ = NodeIdRange::Empty;
        version_++;
    }

    void RoutingToken::Update(NodeIdRange const & range, bool isRecovery)
    {
        range_ = range;
        version_++;

        if (isRecovery)
        {
            IncrementRecoveryVersion();
        }
    }

    bool RoutingToken::Accept(RoutingToken const & receivedToken, NodeId const & ownerId)
    {
        // Check if Merge is safe.
        if (!IsMergeSafe(receivedToken.Version))
        {
            return false;
        }

        if (IsEmpty)
        {
            // If the current token is empty, the incoming token must
            // contain the current node id.
            if (!receivedToken.Contains(ownerId))
            {
                return false;
            }

            Update(receivedToken.range_);
        }
        else
        {
            // Check whether the token is adjacent and disjoint with the current
            // token.
            if (IsSuccAdjacent(receivedToken) && IsPredAdjacent(receivedToken))
            {
                Update(NodeIdRange::Full);
            }
            else if (IsSuccAdjacent(receivedToken) && !Contains(receivedToken.Range.End))
            {
                Update(NodeIdRange(range_.Begin, receivedToken.Range.End));
            }
            else if (IsPredAdjacent(receivedToken) && !Contains(receivedToken.Range.Begin))
            {
                Update(NodeIdRange(receivedToken.Range.Begin, range_.End));
            }
            else
            {
                return (false);
            }
        }

        return true;
    }

    FederationRoutingTokenHeader RoutingToken::SplitFullToken(PartnerNodeSPtr const & neighbor, uint64 targetVersion, 
        NodeId const & ownerId)
    {
        NodeId succMid = ownerId.GetSuccMidPoint(neighbor->Id);
        NodeId predMid = ownerId.GetPredMidPoint(neighbor->Id);

        Update(NodeIdRange(predMid, succMid));

        return FederationRoutingTokenHeader(
            NodeIdRange(succMid.SuccWalk(LargeInteger::One), predMid.PredWalk(LargeInteger::One)), 
            version_, targetVersion);
    }

    FederationRoutingTokenHeader RoutingToken::SplitSuccToken(PartnerNodeSPtr const & succ, uint64 targetVersion, 
        NodeId const & ownerId)
    {
        if (succ->Id == ownerId)
        {
            return FederationRoutingTokenHeader();
        }

        // This is the boundary that should still belong to the current node.
        NodeId splitId = ownerId.GetSuccMidPoint(succ->Id);

        // This is the bounary that should belong to the neighbor.
        NodeId transferId = splitId.SuccWalk(LargeInteger::One);

        // Split is possible only when the transfer point is within the range.
        if (ownerId.SuccDist(transferId) > ownerId.SuccDist(range_.End))
        {
            return FederationRoutingTokenHeader();
        }

        // Split current token.
        NodeId oldEnd = range_.End;
        Update(NodeIdRange(range_.Begin, splitId));

        return FederationRoutingTokenHeader(NodeIdRange(transferId, oldEnd), version_, targetVersion);
    }

    FederationRoutingTokenHeader RoutingToken::SplitPredToken(PartnerNodeSPtr const & pred, uint64 targetVersion, 
        NodeId const & ownerId)
    {
        if (pred->Id == ownerId)
        {
            return FederationRoutingTokenHeader();
        }

        // This is the boundary that should still belong to the current node.
        NodeId splitID = ownerId.GetPredMidPoint(pred->Id);

        // This is the bounary that should belong to the neighbor.
        NodeId transferID = splitID.PredWalk(LargeInteger::One);

        // Split is possible only when the transfer point is within the range.
        if (ownerId.PredDist(transferID) > ownerId.PredDist(range_.Begin))
        {
            return FederationRoutingTokenHeader();
        }

        // Split current token.
        NodeId oldStart = range_.Begin;
        Update(NodeIdRange(splitID, range_.End));

        return FederationRoutingTokenHeader(NodeIdRange(oldStart, transferID), version_, targetVersion);
    }

    FederationRoutingTokenHeader RoutingToken::ReleaseSuccToken(NodeId const& predId, NodeId const& ownerId, PartnerNodeSPtr const& succ)
    {
        // Calculate the starting point that should belong to the successor.
        LargeInteger dist = (predId.SuccDist(succ->Id) >> 1) + LargeInteger::One;
        NodeId midPoint = predId.SuccWalk(dist);

        if (Contains(midPoint))
        {
            NodeId end = range_.End;
            if (range_.Begin == midPoint)
            {
                SetEmpty();
            }
            else
            {
                // Split token.
                Update(NodeIdRange(range_.Begin, midPoint - LargeInteger::One));
            }

            return FederationRoutingTokenHeader(NodeIdRange(midPoint, end), version_, succ->Token.Version);
        }
        else if (predId.SuccDist(ownerId) > dist)
        {
            // Transfering the entire token.
             FederationRoutingTokenHeader token(range_, version_, succ->Token.Version);
             SetEmpty();
             return token;
        }
        else
        {
            return FederationRoutingTokenHeader();
        }
    }

     FederationRoutingTokenHeader RoutingToken::ReleasePredToken(PartnerNodeSPtr const& pred, NodeId const& ownerId, NodeId const& succId)
     {
         // Calculate the starting point that should belong to predecessor.
         LargeInteger dist = (succId.PredDist(pred->Id) + LargeInteger::One) >> 1;
         NodeId midPoint = succId.PredWalk(dist);
         
         if (Contains(midPoint))
         {
             NodeId start = range_.Begin;
             if (range_.End == midPoint)
             {
                 SetEmpty();
             }
             else
             {
                 // Split token.
                 Update(NodeIdRange(midPoint + LargeInteger::One, range_.End));
             }

             return FederationRoutingTokenHeader(NodeIdRange(start, midPoint), version_, pred->Token.Version);
         }
         else if (succId.PredDist(ownerId) > dist)
         {
             // Transfering the entire token.
             FederationRoutingTokenHeader token(range_, version_, pred->Token.Version);

             SetEmpty();

             return token;
         }
         else
         {
             return FederationRoutingTokenHeader();
         }
     }
}
