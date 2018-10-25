// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class NodeAttributesStoreData : public AttributesStoreData
        {
            START_ATTRIBUTES_FLAGS(  )
                ATTRIBUTES_FLAGS_ENTRY(NodeName, 0x1)
                ATTRIBUTES_FLAGS_ENTRY(IpAddressOrFqdn, 0x2)
                ATTRIBUTES_FLAGS_ENTRY(UpgradeDomain, 0x4)
                ATTRIBUTES_FLAGS_ENTRY(FaultDomain, 0x8)
            END_ATTRIBUTES_FLAGS( )

        public:
            NodeAttributesStoreData();

            NodeAttributesStoreData(
                NodeHealthId const & nodeId,
                Store::ReplicaActivityId const & activityId);

            NodeAttributesStoreData(
                NodeHealthId const & nodeId,
                ReportRequestContext const & context);
            
            NodeAttributesStoreData(NodeAttributesStoreData const & other) = default;
            NodeAttributesStoreData & operator = (NodeAttributesStoreData const & other) = default;

            NodeAttributesStoreData(NodeAttributesStoreData && other) = default;
            NodeAttributesStoreData & operator = (NodeAttributesStoreData && other) = default;
            
            virtual ~NodeAttributesStoreData();
            
            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const; 
            
            __declspec (property(get=get_EntityId)) NodeHealthId const & EntityId;
            NodeHealthId const & get_EntityId() const { return nodeId_; }

            __declspec (property(get=get_NodeInstanceId)) FABRIC_NODE_INSTANCE_ID NodeInstanceId;
            FABRIC_NODE_INSTANCE_ID get_NodeInstanceId() const { return nodeInstanceId_; }
            
            FABRIC_INSTANCE_ID get_InstanceId() const override { return static_cast<FABRIC_INSTANCE_ID>(nodeInstanceId_); }

            bool get_HasInvalidInstance() const override { return nodeInstanceId_ == FABRIC_INVALID_NODE_INSTANCE_ID; }

            _declspec (property(get=get_NodeName, put=set_NodeName)) std::wstring const & NodeName;
            std::wstring const & get_NodeName() const { return nodeName_; }
            void set_NodeName(std::wstring const & value) { nodeName_ = value; attributeFlags_.SetNodeName(); }

            __declspec (property(get=get_IpAddressOrFqdn, put=set_IpAddressOrFqdn)) std::wstring const & IpAddressOrFqdn;
            std::wstring const & get_IpAddressOrFqdn() const { return ipAddressOrFqdn_; }
            void set_IpAddressOrFqdn(std::wstring const & value) { ipAddressOrFqdn_ = value; attributeFlags_.SetIpAddressOrFqdn(); }
            
            __declspec (property(get=get_UpgradeDomain, put=set_UpgradeDomain)) std::wstring const & UpgradeDomain;
            std::wstring const & get_UpgradeDomain() const { return upgradeDomain_; }
            void set_UpgradeDomain(std::wstring const & value) { upgradeDomain_ = value; attributeFlags_.SetUpgradeDomain();}

            __declspec (property(get=get_FaultDomain, put=set_FaultDomain)) std::wstring const & FaultDomain;
            std::wstring const & get_FaultDomain() const { return faultDomain_; }
            void set_FaultDomain(std::wstring const & value) { faultDomain_ = value; attributeFlags_.SetFaultDomain();}

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

            FABRIC_FIELDS_08(nodeId_, nodeInstanceId_, nodeName_, ipAddressOrFqdn_, upgradeDomain_, faultDomain_, stateFlags_, attributeFlags_);
                     
            DECLARE_ATTRIBUTES_COMMON_METHODS( NodeAttributesStoreData, NodeHealthId )
                        
        protected:

            // Gets the key that will be used as unique identifier of the record in the store
            virtual std::wstring ConstructKey() const;

        private:        
            // Primary key 
            NodeHealthId nodeId_;

            // Increasing instance ID used to identify specific instance of the entity (optional parameter)
            FABRIC_NODE_INSTANCE_ID nodeInstanceId_;

            std::wstring nodeName_;
            std::wstring ipAddressOrFqdn_;
            std::wstring upgradeDomain_;
            std::wstring faultDomain_;
        };
    }
}
