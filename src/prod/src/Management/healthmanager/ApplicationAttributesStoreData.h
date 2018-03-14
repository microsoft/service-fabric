// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ApplicationAttributesStoreData 
            : public AttributesStoreData
        {
            START_ATTRIBUTES_FLAGS(  )
                ATTRIBUTES_FLAGS_ENTRY(AppHealthPolicy, 0x1)
                ATTRIBUTES_FLAGS_ENTRY(ApplicationTypeName, 0x2)
                ATTRIBUTES_FLAGS_ENTRY(ApplicationDefinitionKind, 0x4)
            END_ATTRIBUTES_FLAGS( )
            
        public:
            ApplicationAttributesStoreData();

            ApplicationAttributesStoreData(
                ApplicationHealthId const & applicationId,
                Store::ReplicaActivityId const & activityId);

            ApplicationAttributesStoreData(
                ApplicationHealthId const & applicationId,
                ReportRequestContext const & context);

            ApplicationAttributesStoreData(ApplicationAttributesStoreData const & other) = default;
            ApplicationAttributesStoreData & operator = (ApplicationAttributesStoreData const & other) = default;

            ApplicationAttributesStoreData(ApplicationAttributesStoreData && other) = default;
            ApplicationAttributesStoreData & operator = (ApplicationAttributesStoreData && other) = default;
            
            virtual ~ApplicationAttributesStoreData();

            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const; 
            
            __declspec (property(get=get_EntityId)) ApplicationHealthId const & EntityId;
            ApplicationHealthId const & get_EntityId() const { return applicationId_; }

            FABRIC_INSTANCE_ID get_InstanceId() const override { return instanceId_;  }
                        
            __declspec (property(get=get_AppHealthPolicyString, put=set_AppHealthPolicyString)) std::wstring const & AppHealthPolicyString;
            std::wstring const & get_AppHealthPolicyString() const { return appHealthPolicyString_; }
            
            __declspec (property(get = get_ApplicationTypeName, put = set_ApplicationTypeName)) std::wstring const & ApplicationTypeName;
            std::wstring const & get_ApplicationTypeName() const { return applicationTypeName_; }
            void set_ApplicationTypeName(std::wstring const &  value) { applicationTypeName_ = value; attributeFlags_.SetApplicationTypeName(); }

            __declspec (property(get=get_ApplicationDefinitionKind, put=set_ApplicationDefinitionKind)) std::wstring const & ApplicationDefinitionKind;
            std::wstring const get_ApplicationDefinitionKind() const { return applicationDefinitionKind_; }
            void set_ApplicationDefinitionKind(std::wstring const & value) { applicationDefinitionKind_ = value; attributeFlags_.SetApplicationDefinitionKind(); }

            Common::ErrorCode GetAppHealthPolicy(__inout ServiceModel::ApplicationHealthPolicySPtr & policy) const;
            
            // Called only from the ctor of the ApplicationEntity.
            void SetAppHealthPolicy(ServiceModel::ApplicationHealthPolicy && policy);

            // When loaded from store, the app health policy string is reconstructed.
            // Since the sharedptr is not persisted, recreate it based on the string.
            // Set when attributes are read from store, before creating or assigning them to an entity.
            virtual void OnCompleteLoadFromStore() override;

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

            FABRIC_FIELDS_07(applicationId_, instanceId_, appHealthPolicyString_, stateFlags_, attributeFlags_, applicationTypeName_, applicationDefinitionKind_);

            DECLARE_ATTRIBUTES_COMMON_METHODS( ApplicationAttributesStoreData, ApplicationHealthId )

        protected:

            // Gets the key that will be used as unique identifier of the record in the store
            virtual std::wstring ConstructKey() const;
            
        private:        
            // Primary key
            ApplicationHealthId applicationId_;

            // Increasing instance ID used to identify specific instance of the entity (optional parameter)
            FABRIC_INSTANCE_ID instanceId_;

            std::wstring appHealthPolicyString_;

            std::wstring applicationTypeName_;

            std::wstring applicationDefinitionKind_;

            // Non-persisted, used only to avoid serialization/deserialization and copying on health evaluation
            mutable std::shared_ptr<ServiceModel::ApplicationHealthPolicy> appHealthPolicy_;
        };
    }
}
