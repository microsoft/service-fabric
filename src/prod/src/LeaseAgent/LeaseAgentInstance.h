// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LeaseWrapper
{
    // Structure represents information about a particular instance of a node.
    struct LeaseAgentInstance : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(LeaseAgentInstance)
    
    public:
        //constructors
        LeaseAgentInstance()
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
        LeaseAgentInstance(std::wstring const& id, int64 instance)
            : id_(id), instanceId_(instance)
        {
        }

        LeaseAgentInstance(LeaseAgentInstance && other)
            : id_(std::move(other.id_)),
              instanceId_(other.instanceId_)
        {
        }

        LeaseAgentInstance & operator=(LeaseAgentInstance && other)
        {
            if (this != &other)
            {
                id_ = std::move(other.id_);
                instanceId_ = other.instanceId_;
            }
            
            return *this;
        }

        FABRIC_FIELDS_02(id_, instanceId_);

        // Properties
        __declspec (property(get=getInstanceId)) int64 InstanceId;
        __declspec (property(get=getId)) std::wstring const & Id;

        // Getter functions for properties.
        std::wstring const & getId() const { return id_; }
        int64 getInstanceId() const { return instanceId_; }

        // Operators
        bool operator == (LeaseAgentInstance const& other ) const
        {
            return id_ == other.id_ && instanceId_ == other.instanceId_;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write("{0}:{1}", id_, instanceId_);
        }

        std::wstring ToString() const
        {
            std::wstring result;
            Common::StringWriter(result).Write(*this);
            return result;
        }

    private:
        // The node id of the node which this instance is of.
        std::wstring id_;

        // Ths unique instance of the node.
        int64 instanceId_;
    };
}
