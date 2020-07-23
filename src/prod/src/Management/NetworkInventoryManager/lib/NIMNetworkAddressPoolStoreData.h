// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

DEFINE_USER_ARRAY_UTILITY(Management::NetworkInventoryManager::NIMMACAllocationPool);
DEFINE_USER_ARRAY_UTILITY(Management::NetworkInventoryManager::NIMIPv4AllocationPool);

namespace Management
{
    namespace NetworkInventoryManager
    {
        class NIMNetworkMACAddressPoolStoreData : public StoreData
        {
        public:
            NIMNetworkMACAddressPoolStoreData();

            NIMNetworkMACAddressPoolStoreData(MACAllocationPoolSPtr & macPool,
                VSIDAllocationPoolSPtr & vsidPool,
                std::wstring networkId,
                std::wstring poolName);

            static std::wstring const& GetStoreType();
            virtual std::wstring const& GetStoreKey() const;

            FABRIC_FIELDS_06(id_, lastUpdateTime_, networkId_, poolName_, macPool_, vsidPool_);

            // Id for the node allocation store key.
            __declspec(property(get = get_Id, put = set_Id)) std::wstring Id;
            std::wstring get_Id() const { return this->id_; };

            __declspec(property(get = get_MacPool, put = set_MacPool)) MACAllocationPoolSPtr MacPool;
            MACAllocationPoolSPtr get_MacPool() const { return this->macPool_; };
            void set_MacPool(MACAllocationPoolSPtr & value) { this->macPool_ = value; }

            __declspec(property(get = get_VSIDPool, put = set_VSIDPool)) VSIDAllocationPoolSPtr VSIDPool;
            VSIDAllocationPoolSPtr get_VSIDPool() const { return this->vsidPool_; };
            void set_VSIDPool(VSIDAllocationPoolSPtr & value) { this->vsidPool_ = value; }

            void PostUpdate(Common::DateTime updatedTime);

            void PostRead(int64 operationLSN)
            {
                operationLSN = 0;
                StoreData::PostRead(operationLSN);
            }

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            std::wstring id_;
            std::wstring networkId_;
            std::wstring poolName_;
            Common::DateTime lastUpdateTime_;

            MACAllocationPoolSPtr macPool_;
            VSIDAllocationPoolSPtr vsidPool_;
        };


        class NIMNetworkIPv4AddressPoolStoreData : public StoreData
        {
        public:
            NIMNetworkIPv4AddressPoolStoreData();

            NIMNetworkIPv4AddressPoolStoreData(IPv4AllocationPoolSPtr & ipPool,
                std::wstring networkId,
                std::wstring poolName);

            static std::wstring const& GetStoreType();
            virtual std::wstring const& GetStoreKey() const;

            FABRIC_FIELDS_05(id_, lastUpdateTime_, networkId_, poolName_, ipPool_);

            // Id for the node allocation store key.
            __declspec(property(get = get_Id, put = set_Id)) std::wstring Id;
            std::wstring get_Id() const { return this->id_; };

            __declspec(property(get = get_NetworkId)) std::wstring NetworkId;
            std::wstring get_NetworkId() const { return this->networkId_; };

            __declspec(property(get = get_PoolName)) std::wstring PoolName;
            std::wstring get_PoolName() const { return this->poolName_; };

            __declspec(property(get = get_IpPool, put = set_IpPool)) IPv4AllocationPoolSPtr IpPool;
            IPv4AllocationPoolSPtr get_IpPool() const { return this->ipPool_; };
            void set_IpPool(IPv4AllocationPoolSPtr & value) { this->ipPool_ = value; }

            void PostUpdate(Common::DateTime updatedTime);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            std::wstring id_;
            std::wstring networkId_;
            std::wstring poolName_;
            Common::DateTime lastUpdateTime_;

            IPv4AllocationPoolSPtr ipPool_;
        };

        typedef shared_ptr<NIMNetworkMACAddressPoolStoreData> NIMNetworkMACAddressPoolStoreDataSPtr;
        typedef shared_ptr<NIMNetworkIPv4AddressPoolStoreData> NIMNetworkIPv4AddressPoolStoreDataSPtr;
    }
}


