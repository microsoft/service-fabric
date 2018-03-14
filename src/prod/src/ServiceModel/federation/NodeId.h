// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    //
    // The structure represends the NodeId being used by the site nodes.
    // Note: This structure is serialized and deserialized as a byte array, so its in-memory layout
    // should not be changed(eg: by adding/defining vftables or new fields before the existing member).
    //
    struct NodeId
    {
    public:
        //constructors

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //        Takes a large integer and sets it accordingly for this object.
        //
        // Arguments:
        //        from - the large integer.
        //
        NodeId(const Common::LargeInteger& from);

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //        Default constructor.
        //
        NodeId();

        // Parsers.

        //---------------------------------------------------------------------
        // Function: NodeId::TryParse
        //
        // Description:
        //        This function tries to parse a string and generate a NodeId based 
        //        on that string. 
        //
        // Arguments:
        //        from - This is the string that is to be parsed so as to create a Common::LargeInteger.
        //        id - The id which is to be set. The memory for the id must be allocated before
        //        the call.
        //
        // Returns:
        //        true - if the parsing succeeded.
        //        false - if an error occured during parsing.
        //
        // Exceptions: (optional)
        //       none (this must be guaranteed)
        //
        static bool TryParse(const std::wstring& data, __out NodeId & id);

        //---------------------------------------------------------------------
        // Function: NodeId::SuccWalk
        //
        // Description:
        //        Return the id that has the specified distance from the original id in
        //         the successor direction
        //
        // Arguments:
        //        value -the distance.
        //
        // Returns:
        //        NodeId - the updated node id.
        //
        // Exceptions: (optional)
        //       none (this must be guaranteed)
        //
        NodeId SuccWalk(const Common::LargeInteger& value) const;

        //---------------------------------------------------------------------
        // Function: NodeId::PredWalk
        //
        // Description:
        //             Return the id that has the specified distance from the original id in
        //             the predecessor direction
        //
        // Arguments:
        //        value -the distance.
        //
        // Returns:
        //        NodeId - the updated node id.
        //
        // Exceptions: (optional)
        //       none (this must be guaranteed)
        //
        NodeId PredWalk(const Common::LargeInteger& value) const;

        //---------------------------------------------------------------------
        // Function: NodeId::SuccDist
        //
        // Description:
        //            Compute the relative distance from this NodeID to NodeID y
        //             in the successor direction
        //
        // Arguments:
        //        id - the target id
        //
        // Returns:
        //        The distance.
        //
        // Exceptions: (optional)
        //       none (this must be guaranteed)
        //
        Common::LargeInteger SuccDist(const NodeId& id) const;

        //---------------------------------------------------------------------
        // Function: NodeId::PredDist
        //
        // Description:
        //            Compute the relative distance from this NodeID to NodeID y
        //             in the predecessor direction
        //
        // Arguments:
        //        id - the target id
        //
        // Returns:
        //        The distance.
        //
        // Exceptions: (optional)
        //       none (this must be guaranteed)
        //
        Common::LargeInteger PredDist(const NodeId& id) const;

        //---------------------------------------------------------------------
        // Function: NodeId::MinDist
        //
        // Description:
        //        Compute the smaller distance from this NodeID to NodeID y
        //        in both directions
        //
        // Arguments:
        //        id - the target id
        //
        // Returns:
        //        The distance.
        //
        // Exceptions: (optional)
        //       none (this must be guaranteed)
        //
        Common::LargeInteger MinDist(const NodeId& id) const;

        //---------------------------------------------------------------------
        // Function: NodeId::Precedes
        //
        // Description:
        //        Computes whether the specified node precedes this node or not.        
        //
        //
        // Arguments:
        //        id - the target id
        //
        // Returns:
        //        true - if the target id precedes the current node id.
        //        false - otherwise
        //
        // Exceptions: (optional)
        //       none (this must be guaranteed)
        //
        bool NodeId::Precedes(const NodeId& id) const;

        /// Calculate the middle point between this node and other node on the
        /// successor side.
        /// The middle point belongs to this node.
        NodeId GetSuccMidPoint(NodeId const & succId) const;

        /// Calculate the middle point between this node and other node on the
        /// predecessor side.
        /// The middle point belongs to this node.
        NodeId GetPredMidPoint(NodeId const & predId) const;

        // Comparison operators.
        bool operator == ( const NodeId& other ) const;
        bool operator != ( const NodeId& other ) const;
        bool operator < ( const NodeId& other ) const;
        bool operator >= ( const NodeId& other ) const;
        bool operator > ( const NodeId& other ) const;
        bool operator <= ( const NodeId& other ) const;

        // Arithmetic operators
        const NodeId operator +(const Common::LargeInteger& value) const;
        const NodeId operator -(const Common::LargeInteger& value) const;
        void operator ++();
        void operator --();

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        // Properties.
        __declspec (property(get=getIdValue)) Common::LargeInteger IdValue;

        // Static Fields
        // The maximum value of node id possible.
        const static NodeId MaxNodeId;

        // The minimum value of node id possible.
        const static NodeId MinNodeId;

        // Used to initialize for other compilation units
        static NodeId const & StaticInit_MinNodeId();

        // Used to initialize for other compilation units
        static NodeId const & StaticInit_MaxNodeId();

        Common::LargeInteger const & getIdValue() const;

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {
            string format = "{0}";
            size_t index = 0;

            traceEvent.AddEventField<Common::LargeInteger>(format, name, index);
            
            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            idValue_.FillEventData(context);
        }

        void ToPublicApi(__out FABRIC_NODE_ID & publicNodeId) const;

        Common::ErrorCode FromPublicApi(__in FABRIC_NODE_ID const& publicNodeId);

        struct Hasher
        {
            size_t operator() (NodeId const & key) const
            {
                return key.idValue_.Low;
            }

            bool operator() (NodeId const & left, NodeId const & right) const
            {
                return (left == right);
            }
        };

    private:
        Common::LargeInteger idValue_;
    };
}

FABRIC_SERIALIZE_AS_BYTEARRAY(Federation::NodeId);
