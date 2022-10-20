// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class UpdateOverlayNetworkRoutesRequest : public Serialization::FabricSerializable
    {
    public:
        UpdateOverlayNetworkRoutesRequest() {}
        UpdateOverlayNetworkRoutesRequest(
            std::wstring networkName,
            std::wstring nodeIpAddress,
            std::wstring instanceID,
            int64 sequenceNumber,
            bool isDelta,
            std::vector<Management::NetworkInventoryManager::NetworkAllocationEntrySPtr> networkMappingTable);

        ~UpdateOverlayNetworkRoutesRequest();

        __declspec(property(get = get_NetworkMappingTable)) std::vector<Management::NetworkInventoryManager::NetworkAllocationEntrySPtr> const & NetworkMappingTable;
        std::vector<Management::NetworkInventoryManager::NetworkAllocationEntrySPtr> const & get_NetworkMappingTable() const { return networkMappingTable_; }

        __declspec(property(get = get_NetworkName)) std::wstring const & NetworkName;
        std::wstring const & get_NetworkName() const { return networkName_; }

        __declspec(property(get = get_NodeIpAddress)) std::wstring const & NodeIpAddress;
        std::wstring const & get_NodeIpAddress() const { return nodeIpAddress_; }

        __declspec(property(get = get_InstanceID)) std::wstring const & InstanceID;
        std::wstring const & get_InstanceID() const { return instanceID_; }

        __declspec(property(get = get_SequenceNumber)) int64 SequenceNumber;
        int64 get_SequenceNumber() const { return sequenceNumber_; }

        __declspec(property(get = get_IsDelta)) bool IsDelta;
        bool get_IsDelta() const { return isDelta_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_06(networkName_, nodeIpAddress_, instanceID_, sequenceNumber_, isDelta_, networkMappingTable_);

    private:
        std::wstring networkName_;
        std::wstring nodeIpAddress_;
        std::wstring instanceID_;
        int64 sequenceNumber_;
        bool isDelta_;
        std::vector<Management::NetworkInventoryManager::NetworkAllocationEntrySPtr> networkMappingTable_;
    };
}
