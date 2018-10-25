// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ReplicaAttributesStoreData
            : public AttributesStoreData
        {
            START_ATTRIBUTES_FLAGS(  )
                ATTRIBUTES_FLAGS_ENTRY(NodeId, 0x1)
                ATTRIBUTES_FLAGS_ENTRY(NodeInstanceId, 0x2)
            END_ATTRIBUTES_FLAGS( )

        public:
            ReplicaAttributesStoreData();

            ReplicaAttributesStoreData(
                ReplicaHealthId const & replicaId,
                ReportRequestContext const & context);

            ReplicaAttributesStoreData(
                ReplicaHealthId const & replicaId,
                Store::ReplicaActivityId const & activityId);

            ReplicaAttributesStoreData(ReplicaAttributesStoreData const & other) = default;
            ReplicaAttributesStoreData & operator = (ReplicaAttributesStoreData const & other) = default;

            ReplicaAttributesStoreData(ReplicaAttributesStoreData && other) = default;
            ReplicaAttributesStoreData & operator = (ReplicaAttributesStoreData && other) = default;

            virtual ~ReplicaAttributesStoreData();

            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const;

            __declspec (property(get=get_EntityId)) ReplicaHealthId const & EntityId;
            ReplicaHealthId const & get_EntityId() const { return replicaId_; }

            FABRIC_INSTANCE_ID get_InstanceId() const override { return instanceId_; }

            __declspec (property(get=get_NodeId, put=set_NodeId)) NodeHealthId const & NodeId;
            NodeHealthId const & get_NodeId() const { return nodeId_; }
            void set_NodeId(NodeHealthId const & value) { nodeId_ = value; attributeFlags_.SetNodeId(); }

            __declspec (property(get=get_NodeInstanceId, put=set_NodeInstanceId)) FABRIC_NODE_INSTANCE_ID NodeInstanceId;
            FABRIC_NODE_INSTANCE_ID get_NodeInstanceId() const { return nodeInstanceId_; }
            void set_NodeInstanceId(FABRIC_NODE_INSTANCE_ID value) { nodeInstanceId_ = value; attributeFlags_.SetNodeInstanceId(); }

            __declspec (property(get = get_Kind)) FABRIC_SERVICE_KIND Kind;
            FABRIC_SERVICE_KIND get_Kind() const { return (this->UseInstance) ? FABRIC_SERVICE_KIND_STATEFUL : FABRIC_SERVICE_KIND_STATELESS;}

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

            Common::ErrorCode TryAcceptHealthReport(
                ServiceModel::HealthReport const & healthReport,
                __inout bool & skipLsnCheck,
                __out bool & replaceAttributesMetadata) const;

            FABRIC_FIELDS_06(replicaId_, instanceId_, nodeId_, nodeInstanceId_, stateFlags_, attributeFlags_);

            DECLARE_ATTRIBUTES_COMMON_METHODS( ReplicaAttributesStoreData, ReplicaHealthId )

        protected:

            // Gets the key that will be used as unique identifier of the record in the store
            virtual std::wstring ConstructKey() const;

        private:

            // Primary key
            ReplicaHealthId replicaId_;

            // Increasing instance ID used to identify specific instance of the entity (optional parameter)
            // Stateless instance uses special value to show that it doesn't have instance
            FABRIC_INSTANCE_ID instanceId_;

            // The node where the replica is running
            NodeHealthId nodeId_;
            FABRIC_NODE_INSTANCE_ID nodeInstanceId_;
        };
    }
}
