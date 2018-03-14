// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    // Node configuration information.
    struct NodeConfig : public Serialization::FabricSerializable
    {
    public:

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Arguments:
        //          address - The dispatch address of this node.
        //            id - A unique node id for this node.
        //
        NodeConfig()
        {
        }

        NodeConfig(NodeId id, std::wstring const& address, std::wstring const& leaseAgentAddress, std::wstring const& workingDir, std::wstring const& ringName = L"")
            : id_(id), address_(address), leaseAgentAddress_(leaseAgentAddress), workingDir_(workingDir), ringName_(ringName)
        {
        }

        NodeConfig(NodeConfig const & rhs)
            : id_(rhs.id_), address_(rhs.address_), leaseAgentAddress_(rhs.leaseAgentAddress_), workingDir_(rhs.workingDir_), ringName_(rhs.ringName_)
        {
        }

        __declspec (property(get=getId)) NodeId Id;
        __declspec (property(get=getAddress)) std::wstring const& Address;
        __declspec (property(get=getLeaseAgentAddress)) std::wstring const& LeaseAgentAddress;
        __declspec (property(get=getWorkingDir)) std::wstring const& WorkingDir;
        __declspec (property(get=getRingName)) std::wstring const& RingName;

        NodeId getId() const { return id_; }

        std::wstring const& getAddress() const { return address_; }

        std::wstring const& getLeaseAgentAddress() const { return leaseAgentAddress_; }

        std::wstring const& getWorkingDir() const { return workingDir_; }

        std::wstring const& getRingName() const { return ringName_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write("{0},{1},{2},{3}", id_, address_, leaseAgentAddress_, workingDir_);
        }

        FABRIC_FIELDS_03(id_, address_, leaseAgentAddress_);

    private:

        NodeId id_;
        std::wstring address_;
        std::wstring leaseAgentAddress_;
        std::wstring workingDir_;
        std::wstring ringName_;
    };
}
