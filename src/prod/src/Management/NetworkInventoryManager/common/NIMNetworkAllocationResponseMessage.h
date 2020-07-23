// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "NIMNetworkDefinition.h"

namespace Management
{
    namespace NetworkInventoryManager
    {
        //--------
        // This class is the response for NetworkAllocationRequest call.
        class NetworkAllocationResponseMessage : public NetworkDefinition
        {
        public:
            NetworkAllocationResponseMessage()
            {}

            // Error code
            __declspec(property(get = get_ErrorCode, put = set_ErrorCode)) Common::ErrorCode ErrorCode;
            Common::ErrorCode get_ErrorCode() const { return error_; }
            void set_ErrorCode(const Common::ErrorCode value) { error_ = value; }

            // Get start mac pool address of the overlay network.
            __declspec(property(get = get_StartMacPoolAddress, put = set_StartMacPoolAddress)) std::wstring StartMacPoolAddress;
            std::wstring get_StartMacPoolAddress() const { return this->startMacPoolAddress_; };
            void set_StartMacPoolAddress(const std::wstring & value) { this->startMacPoolAddress_ = value; }

            // Get end mac pool address of the overlay network.
            __declspec(property(get = get_EndMacPoolAddress, put = set_EndMacPoolAddress)) std::wstring EndMacPoolAddress;
            std::wstring get_EndMacPoolAddress() const { return this->endMacPoolAddress_; };
            void set_EndMacPoolAddress(const std::wstring & value) { this->endMacPoolAddress_ = value; }

            // Get the node ip address.
            __declspec(property(get = get_NodeIpAddress, put = set_NodeIpAddress)) std::wstring NodeIpAddress;
            std::wstring get_NodeIpAddress() const { return this->nodeIpAddress_; };
            void set_NodeIpAddress(const std::wstring & value) { this->nodeIpAddress_ = value; }

            // Get the ip/mac address collection to be added.
            __declspec(property(get = get_IpMacAddressMapToBeAdded, put = set_IpMacAddressMapToBeAdded)) std::map<std::wstring, std::wstring> & IpMacAddressMapToBeAdded;
            std::map<std::wstring, std::wstring> & get_IpMacAddressMapToBeAdded() { return this->ipMacAddressMapToBeAdded_; };
            void set_IpMacAddressMapToBeAdded(const std::map<std::wstring, std::wstring> & value) { this->ipMacAddressMapToBeAdded_ = value; }

            // Get the ip/mac address collection to be removed.
            __declspec(property(get = get_IpMacAddressMapToBeRemoved, put = set_IpMacAddressMapToBeRemoved)) std::map<std::wstring, std::wstring> & IpMacAddressMapToBeRemoved;
            std::map<std::wstring, std::wstring> & get_IpMacAddressMapToBeRemoved() { return this->ipMacAddressMapToBeRemoved_; };
            void set_IpMacAddressMapToBeRemoved(const std::map<std::wstring, std::wstring> & value) { this->ipMacAddressMapToBeRemoved_ = value; }

            // Get the current network mapping table for the node (including new allocations).
            __declspec(property(get = get_NetworkMappingTable, put = set_NetworkMappingTable)) std::vector<NetworkAllocationEntrySPtr> & NetworkMappingTable;
            std::vector<NetworkAllocationEntrySPtr> & get_NetworkMappingTable() { return this->networkMappingTable_; }
            void set_NetworkMappingTable(const std::vector<NetworkAllocationEntrySPtr> & value) { this->networkMappingTable_ = value; }

            FABRIC_FIELDS_07(nodeIpAddress_, startMacPoolAddress_, endMacPoolAddress_, ipMacAddressMapToBeAdded_, ipMacAddressMapToBeRemoved_, networkMappingTable_, error_);

        private:

            // Node ip address in dot format
            std::wstring nodeIpAddress_;

            // Start Mac pool address
            std::wstring startMacPoolAddress_;

            // End Mac pool address
            std::wstring endMacPoolAddress_;

            // Current batch of ip and mac supplied for container activation
            std::map<std::wstring, std::wstring> ipMacAddressMapToBeAdded_;

            // Batch of ip and mac removed for container activation
            std::map<std::wstring, std::wstring> ipMacAddressMapToBeRemoved_;

            // Current network mapping table for the node (including new allocations)
            std::vector<NetworkAllocationEntrySPtr> networkMappingTable_;

            // Error code
            Common::ErrorCode error_;
        };
    }
}