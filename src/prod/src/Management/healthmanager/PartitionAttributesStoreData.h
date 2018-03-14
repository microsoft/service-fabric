// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class PartitionAttributesStoreData 
            : public AttributesStoreData
        {
            START_ATTRIBUTES_FLAGS(  )
                ATTRIBUTES_FLAGS_ENTRY(ServiceName, 0x1)
            END_ATTRIBUTES_FLAGS( )
            
        public:
            PartitionAttributesStoreData();

            PartitionAttributesStoreData(
                PartitionHealthId const & partitionId,
                Store::ReplicaActivityId const & activityId);

            PartitionAttributesStoreData(
                PartitionHealthId const & partitionId,
                ReportRequestContext const & context);

            PartitionAttributesStoreData(PartitionAttributesStoreData const & other) = default;
            PartitionAttributesStoreData & operator = (PartitionAttributesStoreData const & other) = default;

            PartitionAttributesStoreData(PartitionAttributesStoreData && other) = default;
            PartitionAttributesStoreData & operator = (PartitionAttributesStoreData && other) = default;
            
            virtual ~PartitionAttributesStoreData();

            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const; 
            
            __declspec (property(get=get_EntityId)) PartitionHealthId const & EntityId;
            PartitionHealthId const & get_EntityId() const { return partitionId_; }

            virtual FABRIC_INSTANCE_ID get_InstanceId() const { return Constants::UnusedInstanceValue; }
            
            __declspec (property(get=get_ServiceName, put=set_ServiceName)) std::wstring const & ServiceName;
            std::wstring const & get_ServiceName() const { return serviceName_; }
            void set_ServiceName(std::wstring const & value) { serviceName_ = value; attributeFlags_.SetServiceName(); }

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

            FABRIC_FIELDS_04(partitionId_, serviceName_, stateFlags_, attributeFlags_);

            DECLARE_ATTRIBUTES_COMMON_METHODS( PartitionAttributesStoreData, PartitionHealthId )

        protected:

            // Gets the key that will be used as unique identifier of the record in the store
            virtual std::wstring ConstructKey() const;

        private:        
            // Primary key
            PartitionHealthId partitionId_;

            std::wstring serviceName_;
        };
    }
}
