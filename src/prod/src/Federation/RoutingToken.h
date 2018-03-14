// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct FederationRoutingTokenHeader;
    class PartnerNode;

    /// <summary>
    /// A routing token is used to guarantee routing consistency.
    /// A node can only accept a routed message when its routing
    /// token contains the target node id.
    /// A routing token contains a node id range and a version.  
    /// Version is used to allow a node to safely accept/recover
    /// tokens.  Logically speaking, the version really contains
    /// two part. The first part is incremented whenever a merge/split
    /// happens and the second part is incremented whenever the node
    /// is involved in a token recovery (does not need to be the
    /// recovering node). The implementation uses a single
    /// version for the convenience of version comparison and
    /// serialization.
    /// 
    /// TODO: consider to add ownerId field.
    /// </summary>
    struct RoutingToken : public Serialization::FabricSerializable
    {
    public:
        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //      Constructs an empty token.
        //
        // Arguments:
        //
        RoutingToken()
            : version_(0)
        {
        }

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //      Copy Constructor.
        //
        // Arguments:
        //      The routing token to be copied.
        //
        RoutingToken(RoutingToken const & original)
            : range_(original.Range), version_(original.Version)
        {
        }

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //      Creates a token based on the range and version.
        //
        // Arguments:
        //      range: The range of the token.
        //      version: The version of the token.
        //
        RoutingToken(NodeIdRange range, uint64 version)
            : range_(range), version_(version)
        {
        }

        bool Contains(NodeId const& id) const { return range_.Contains(id); }
        bool IsSuccAdjacent(RoutingToken const& token) const { return range_.IsSuccAdjacent(token.range_); }
        bool IsPredAdjacent(RoutingToken const& token) const { return range_.IsPredAdjacent(token.range_); }

        //---------------------------------------------------------------------
        // Description:
        //      Increase the version number when a token recovery is involved.
        //
        void IncrementRecoveryVersion() { version_ += RECOVERY_INCREMENT; }

        static bool TokenVersionMatch(uint64 version1, uint64 version2) { return (((uint32) version1) == ((uint32) version2)); }

        //---------------------------------------------------------------------
        // Description:
        //      Whether the initiator of the token recovery can safely
        //      recover token based on the echoed token.
        //      It is safe iif the initiator has not performed any
        //      merge/split operation on the token.
        //
        //  Arguments:
        //      version: The version in the echoed token.  Which is also the 
        //      token version when recovery starts.
        //
        //  Returns
        //      true: If recovery is possible.
        //      false: otherwise.
        bool IsRecoverySafe(uint64 version) const { return TokenVersionMatch(version_, version); }

        //---------------------------------------------------------------------
        // Description:
        //      Whether an incoming token is safe to be merged with the current
        //      token based on the version number.
        //      Note that it is safe to merge iif there has been no recovery
        //      involved since the originator of the transferred token last learnt
        //      about the current node's token.
        //
        //  Arguments:
        //      version: The incoming token's version.
        //
        //  Returns
        //      true: If merge is possible.
        //      false: otherwise.
        bool IsMergeSafe(uint64 version) const;

        //---------------------------------------------------------------------
        // Description:
        //     Release the correct part of the token to the successor before departing.
        //
        //  Returns
        //      FederationRoutingTokenHeader corresponding to the routing token being passed.
        FederationRoutingTokenHeader ReleaseSuccToken(NodeId const& predId, NodeId const& ownerId, std::shared_ptr<PartnerNode const> const& succ);

        //---------------------------------------------------------------------
        // Description:
        //     Release the correct part of the token to the predecessor before departing.
        //
        //  Returns
        //      FederationRoutingTokenHeader corresponding to the routing token being passed.
        FederationRoutingTokenHeader ReleasePredToken(std::shared_ptr<PartnerNode const> const& pred, NodeId const& ownerId, NodeId const& succId);

        //---------------------------------------------------------------------
        // Description:
        //      Merge an incoming token.  The merge can be performed
        //      if the token matches (no recovery not known by the origin)
        //      and the incoming token is adjacent to the existing token.
        //
        //  Arguments:
        //      receivedToken: The incoming token.
        //      ownerId: The id of the owner of the token.
        //
        //  Returns
        //      true: If merge is successfull.
        //      false: otherwise.
        bool Accept(RoutingToken const & receivedToken, NodeId const & ownerId);

        void Update(NodeIdRange const & range, bool isRecovery = false);

        __declspec (property(get=getRange)) NodeIdRange const& Range;
        __declspec (property(get=getTokenVersion)) uint TokenVersion; // Reflects only merges and splits.
        __declspec (property(get=getVersion)) uint64 Version; // Used on every change. Used to find the fresher token.
        __declspec (property(get=isEmpty)) bool IsEmpty;
        __declspec (property(get=isFull)) bool IsFull;

        NodeIdRange getRange() const { return range_; }
        uint64 getVersion() const { return version_; }
        uint getTokenVersion() const { return (uint) version_; }
        bool isEmpty() const { return range_.IsEmpty; }
        bool isFull() const { return range_.IsFull; }

        FABRIC_FIELDS_02(range_, version_);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    private:

        FederationRoutingTokenHeader SplitFullToken(std::shared_ptr<PartnerNode const> const & neighbor, uint64 targetVersion, 
            NodeId const & ownerId);
        FederationRoutingTokenHeader SplitSuccToken(std::shared_ptr<PartnerNode const> const & neighbor, uint64 targetVersion, 
            NodeId const & ownerId);
        FederationRoutingTokenHeader SplitPredToken(std::shared_ptr<PartnerNode const> const & neighbor, uint64 targetVersion, 
            NodeId const & ownerId);
        void SetEmpty();

        // Token version is incremented by this value when involved with
        // recovery.  This value must be large enough so that it is
        // not possible that there are so many token split/merge
        // within a token recovery.
        const static int SIZE_OF_UINT = sizeof(uint32)*8;
        const static uint64 RECOVERY_INCREMENT = (1ULL << SIZE_OF_UINT);

        // The version of the token.
        uint64 version_;

        // The node id range of the token.
        NodeIdRange range_;

        friend class RoutingTable;
    };
}
