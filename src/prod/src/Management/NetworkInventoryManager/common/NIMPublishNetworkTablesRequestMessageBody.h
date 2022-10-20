// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        // Message to request network agent to update the mapping tables.
        class PublishNetworkTablesRequestMessage : public Serialization::FabricSerializable
        {
        public:
            PublishNetworkTablesRequestMessage()
            {}

            PublishNetworkTablesRequestMessage(
                const std::wstring &networkName,
                std::vector<Federation::NodeInstance>& nodeInstances,
                std::vector<NetworkAllocationEntrySPtr>& networkMappingTable,
                bool isDelta,
                const std::wstring &instanceID,
                int64 sequenceNumber)
                : sequenceNumber_(sequenceNumber),
                instanceID_(instanceID),
                isDelta_(isDelta),
                networkName_(networkName),
                nodeInstances_(nodeInstances),
                networkMappingTable_(networkMappingTable)
            {
            }
            
            __declspec (property(get = get_NetworkName, put = set_NetworkName)) const std::wstring NetworkName;
            const std::wstring & get_NetworkName() const { return networkName_; }
            void set_NetworkName(const std::wstring & value) { networkName_ = value; }

            __declspec (property(get = get_InstanceID, put = set_InstanceID)) const std::wstring InstanceID;
            const std::wstring & get_InstanceID() const { return instanceID_; }
            void set_InstanceID(const std::wstring & value) { instanceID_ = value; }

            __declspec(property(get=get_SequenceNumber)) int64 SequenceNumber;
            int64 get_SequenceNumber() const { return sequenceNumber_; }

            __declspec(property(get = get_IsDelta)) bool IsDelta;
            bool get_IsDelta() const { return isDelta_; }

            // Get the nodes where this network is active on.
            __declspec(property(get = get_NodeInstances, put = set_NodeInstances)) std::vector<Federation::NodeInstance> NodeInstances;
            std::vector<Federation::NodeInstance> get_NodeInstances() const { return this->nodeInstances_; }
            void set_NodeInstances(const std::vector<Federation::NodeInstance> & value) { this->nodeInstances_ = value; }
            
            // Get the current network mapping table for the network.
            __declspec(property(get = get_NetworkMappingTable, put = set_NetworkMappingTable)) std::vector<NetworkAllocationEntrySPtr> NetworkMappingTable;
            std::vector<NetworkAllocationEntrySPtr> get_NetworkMappingTable() const { return this->networkMappingTable_; }
            void set_NetworkMappingTable(const std::vector<NetworkAllocationEntrySPtr> & value) { this->networkMappingTable_ = value; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            static const std::wstring ActionName;

            FABRIC_FIELDS_06(instanceID_, sequenceNumber_, isDelta_, networkName_, nodeInstances_, networkMappingTable_);

        private:
            std::wstring networkName_;
            std::wstring instanceID_;
            std::vector<Federation::NodeInstance> nodeInstances_;
            std::vector<NetworkAllocationEntrySPtr> networkMappingTable_;
            int64 sequenceNumber_;
            bool isDelta_;
        };
    }
}

