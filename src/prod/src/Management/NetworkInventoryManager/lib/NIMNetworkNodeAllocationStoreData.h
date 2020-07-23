// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace Reliability::FailoverManagerComponent;

DEFINE_USER_ARRAY_UTILITY(Management::NetworkInventoryManager::NetworkAllocationEntry);
DEFINE_USER_MAP_UTILITY(std::wstring, Management::NetworkInventoryManager::NetworkAllocationEntrySPtr);

namespace Management
{
    namespace NetworkInventoryManager
    {
        class NIMNetworkNodeAllocationStoreData : public StoreData
        {
        public:
            NIMNetworkNodeAllocationStoreData();

            NIMNetworkNodeAllocationStoreData(NodeMapEntrySPtr & nodeAllocationMap,
                std::wstring networkId,
                std::wstring nodeName,
                Federation::NodeInstance & nodeInstance);

            static std::wstring const& GetStoreType();
            virtual std::wstring const& GetStoreKey() const;

            FABRIC_FIELDS_06(id_, lastUpdateTime_, networkId_, nodeName_, nodeInstance_, nodeAllocationMap_);

            // Id for the node allocation store key.
            __declspec(property(get = get_Id, put = set_Id)) std::wstring Id;
            std::wstring get_Id() const { return this->id_; };

            __declspec(property(get = get_NetworkId)) std::wstring NetworkId;
            std::wstring get_NetworkId() const { return this->networkId_; };

            __declspec(property(get = get_NodeName)) std::wstring NodeName;
            std::wstring get_NodeName() const { return this->nodeName_; };

            __declspec(property(get = get_NodeInstance)) const Federation::NodeInstance & NodeInstance;
            const Federation::NodeInstance & get_NodeInstance() const { return this->nodeInstance_; };

            __declspec(property(get = get_NodeAllocationMap, put = set_NodeAllocationMap)) NodeMapEntrySPtr NodeAllocationMap;
            NodeMapEntrySPtr get_NodeAllocationMap() const { return this->nodeAllocationMap_; };
            void set_NodeAllocationMap(NodeMapEntrySPtr & value) { this->nodeAllocationMap_ = value; }

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
            std::wstring nodeName_;
            Federation::NodeInstance nodeInstance_;
            Common::DateTime lastUpdateTime_;

            NodeMapEntrySPtr nodeAllocationMap_;
        };

        typedef shared_ptr<NIMNetworkNodeAllocationStoreData> NIMNetworkNodeAllocationStoreDataSPtr;
    }
}


