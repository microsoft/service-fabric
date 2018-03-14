// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ServiceAttributesStoreData 
            : public AttributesStoreData
        {
            START_ATTRIBUTES_FLAGS(  )
                ATTRIBUTES_FLAGS_ENTRY(ApplicationName, 0x1)
                ATTRIBUTES_FLAGS_ENTRY(ServiceTypeName, 0x2)
            END_ATTRIBUTES_FLAGS( )
            
        public:
            ServiceAttributesStoreData();
            
            ServiceAttributesStoreData(
                ServiceHealthId const & serviceId,
                Store::ReplicaActivityId const & activityId);

            ServiceAttributesStoreData(
                ServiceHealthId const & serviceId,
                ReportRequestContext const & context);

            ServiceAttributesStoreData(ServiceAttributesStoreData const & other) = default;
            ServiceAttributesStoreData & operator = (ServiceAttributesStoreData const & other) = default;

            ServiceAttributesStoreData(ServiceAttributesStoreData && other) = default;
            ServiceAttributesStoreData & operator = (ServiceAttributesStoreData && other) = default;
            
            virtual ~ServiceAttributesStoreData();

            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const; 
            
            __declspec (property(get=get_EntityId)) ServiceHealthId const & EntityId;
            ServiceHealthId const & get_EntityId() const { return serviceId_; }

            FABRIC_INSTANCE_ID get_InstanceId() const override { return instanceId_; }
            
            __declspec (property(get=get_ApplicationName, put=set_ApplicationName)) std::wstring const & ApplicationName;
            std::wstring const & get_ApplicationName() const { return applicationName_; }
            void set_ApplicationName(std::wstring const & value) { applicationName_ = value; attributeFlags_.SetApplicationName(); }

            __declspec (property(get=get_ServiceTypeName, put=set_TypeServiceName)) std::wstring const & ServiceTypeName;
            std::wstring const & get_ServiceTypeName() const { return serviceTypeName_; }
            void set_TypeServiceName(std::wstring const & value) { serviceTypeName_ = value; attributeFlags_.SetServiceTypeName(); }

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

            FABRIC_FIELDS_06(serviceId_, instanceId_, applicationName_, serviceTypeName_, stateFlags_, attributeFlags_);

            DECLARE_ATTRIBUTES_COMMON_METHODS( ServiceAttributesStoreData, ServiceHealthId )

        protected:

            // Gets the key that will be used as unique identifier of the record in the store
            virtual std::wstring ConstructKey() const;

        private:        
            // Primary key
            ServiceHealthId serviceId_;

            // Increasing instance ID used to identify specific instance of the entity (optional parameter)
            FABRIC_INSTANCE_ID instanceId_;

            std::wstring applicationName_;
            std::wstring serviceTypeName_;
        };
    }
}
