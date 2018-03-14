// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class DeployedApplicationAttributesStoreData 
            : public AttributesStoreData
        {
            START_ATTRIBUTES_FLAGS(  )
                ATTRIBUTES_FLAGS_ENTRY(NodeName, 0x1)
                ATTRIBUTES_FLAGS_ENTRY(NodeInstanceId, 0x2)
            END_ATTRIBUTES_FLAGS( )            

        public:
            DeployedApplicationAttributesStoreData();
            
            DeployedApplicationAttributesStoreData(
                DeployedApplicationHealthId const & deployedApplicationId,
                Store::ReplicaActivityId const & activityId);

            DeployedApplicationAttributesStoreData(
                DeployedApplicationHealthId const & deployedApplicationId,
                ReportRequestContext const & context);

            DeployedApplicationAttributesStoreData(DeployedApplicationAttributesStoreData const & other) = default;
            DeployedApplicationAttributesStoreData & operator = (DeployedApplicationAttributesStoreData const & other) = default;

            DeployedApplicationAttributesStoreData(DeployedApplicationAttributesStoreData && other) = default;
            DeployedApplicationAttributesStoreData & operator = (DeployedApplicationAttributesStoreData && other) = default;
            
            virtual ~DeployedApplicationAttributesStoreData();

            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const; 
            
            __declspec (property(get=get_EntityId)) DeployedApplicationHealthId const & EntityId;
            DeployedApplicationHealthId const & get_EntityId() const { return deployedApplicationId_; }
                        
            FABRIC_INSTANCE_ID get_InstanceId() const override { return instanceId_; }

            __declspec (property(get=get_NodeInstanceId, put=set_NodeInstanceId)) FABRIC_NODE_INSTANCE_ID NodeInstanceId;
            FABRIC_NODE_INSTANCE_ID get_NodeInstanceId() const { return nodeInstanceId_; }
            void set_NodeInstanceId(FABRIC_NODE_INSTANCE_ID value) { nodeInstanceId_ = value; attributeFlags_.SetNodeInstanceId(); }

            __declspec (property(get=get_NodeName, put=set_NodeName)) std::wstring const & NodeName;
            std::wstring const & get_NodeName() const { return nodeName_; }
            void set_NodeName(std::wstring const & value) { nodeName_ = value; attributeFlags_.SetNodeName(); }

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

            FABRIC_FIELDS_06(deployedApplicationId_, instanceId_, nodeName_, nodeInstanceId_, stateFlags_, attributeFlags_);

            DECLARE_ATTRIBUTES_COMMON_METHODS( DeployedApplicationAttributesStoreData, DeployedApplicationHealthId )

        protected:

            // Gets the key that will be used as unique identifier of the record in the store
            virtual std::wstring ConstructKey() const;

        private:        
            // Primary key
            DeployedApplicationHealthId deployedApplicationId_;
            
            // Increasing instance ID used to identify specific instance of the entity (optional parameter)
            FABRIC_INSTANCE_ID instanceId_;

            // The node where the application is deployed
            std::wstring nodeName_;
            FABRIC_NODE_INSTANCE_ID nodeInstanceId_;
        };
    }
}
