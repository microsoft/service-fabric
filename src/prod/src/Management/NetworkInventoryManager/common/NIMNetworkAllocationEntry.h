// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once


namespace Management
{
    namespace NetworkInventoryManager
    {
        //--------
        // This class encapsulates the IP/MAC/PhyIp allocation entry that is distributed to nodes.
        class NetworkAllocationEntry : public Serialization::FabricSerializable
        {
        public:
            __declspec(property(get = get_IpAddress, put = set_IpAddress)) const std::wstring IpAddress;
            const std::wstring &  get_IpAddress() const { return ipAddress_; }
            void set_IpAddress(const std::wstring & value) { ipAddress_ = value; }

            __declspec(property(get = get_InfraIpAddress, put = set_InfraIpAddress)) const std::wstring InfraIpAddress;
            const std::wstring &  get_InfraIpAddress() const { return infraIpAddress_; }
            void set_InfraIpAddress(const std::wstring & value) { infraIpAddress_ = value; }

            __declspec(property(get = get_MacAddress, put = set_MacAddress)) const std::wstring MacAddress;
            const std::wstring &  get_MacAddress() const { return macAddress_; }
            void set_MacAddress(const std::wstring & value) { macAddress_ = value; }

            __declspec (property(get = get_NodeName, put = set_NodeName)) const std::wstring NodeName;
            const std::wstring & get_NodeName() const { return nodeName_; }
            void set_NodeName(const std::wstring & value) { nodeName_ = value; }

            __declspec(property(get = get_ReplicaId, put = set_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }
            void set_ReplicaId(FABRIC_REPLICA_ID value) { replicaId_ = value; }

            FABRIC_FIELDS_05(ipAddress_, infraIpAddress_, macAddress_, nodeName_, replicaId_);

        private:
            std::wstring ipAddress_;
            std::wstring infraIpAddress_;
            std::wstring macAddress_;
            std::wstring nodeName_;
            FABRIC_REPLICA_ID replicaId_;
        };

        typedef std::shared_ptr<NetworkAllocationEntry> NetworkAllocationEntrySPtr;
        typedef std::shared_ptr<std::map<std::wstring, NetworkAllocationEntrySPtr>> NodeMapEntrySPtr;
    }
}