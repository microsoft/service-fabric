// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    // Structure represents information about a particular instance of a node.
    struct NodeInstance : public Serialization::FabricSerializable
    {
    public:
        //constructors
        NodeInstance()
            : id_() , instanceId_(0)
        {}

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //      Takes a node id and instance id and creates a node instance.
        //
        // Arguments:
        //      instanceId - the instance id.
        //        id - the node id.
        //
        NodeInstance(NodeId const& id, uint64 instance)
            : id_(id), instanceId_(instance)
        {
        }

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //      Copy Constructor.
        //
        //
        NodeInstance(NodeInstance const& nodeInstance)
            : id_(nodeInstance.Id), instanceId_(nodeInstance.InstanceId)
        {
        }

        FABRIC_FIELDS_02(id_, instanceId_);

        // Properties
        __declspec (property(get=getInstanceId)) uint64 InstanceId;
        __declspec (property(get=getId)) NodeId Id;

        // Getter functions for properties.
        NodeId getId() const { return id_; }
        uint64 getInstanceId() const { return instanceId_; }

        void IncrementInstanceId()
        {
            instanceId_++;
        }

        // Operators
        bool operator < (NodeInstance const& other ) const
        {
            return ( id_ < other.id_ || (id_ == other.id_ && instanceId_ < other.instanceId_) );
        }

        bool operator <= (NodeInstance const& other ) const
        {
            return ( id_ <= other.id_ || (id_ == other.id_ && instanceId_ <= other.instanceId_) );
        }

        bool operator == (NodeInstance const& other ) const
        {
            return id_ == other.id_ && instanceId_ == other.instanceId_;
        }

        bool operator != (NodeInstance const& other ) const
        {
            return id_ != other.id_ || instanceId_ != other.instanceId_;
        }

        bool Match(NodeInstance const & other) const
        {
            // special case instanceId, if 0 it is match all and if not it must match the instance id
            return id_ == other.id_ && (instanceId_ == other.instanceId_ || other.instanceId_ == 0 || instanceId_ == 0);
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        std::wstring ToString() const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        static bool TryParse(std::wstring const & data, __out NodeInstance & nodeInstance);

    private:
        // The node id of the node which this instance is of.
        NodeId id_;

        // Ths unique instance of the node.
        uint64 instanceId_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Federation::NodeInstance);
