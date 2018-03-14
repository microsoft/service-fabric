// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ClusterAttributesStoreData 
            : public AttributesStoreData
        {
            START_ATTRIBUTES_FLAGS(  )
                ATTRIBUTES_FLAGS_ENTRY(HealthPolicy, 0x1)
                ATTRIBUTES_FLAGS_ENTRY(UpgradeHealthPolicy, 0x2)
                ATTRIBUTES_FLAGS_ENTRY(ApplicationHealthPolicies, 0x4)
            END_ATTRIBUTES_FLAGS( )

        public:
            ClusterAttributesStoreData();
            
            ClusterAttributesStoreData(
                ClusterHealthId const & clusterId,
                Store::ReplicaActivityId const & activityId);

            ClusterAttributesStoreData(
                ClusterHealthId const & clusterId,
                ReportRequestContext const & context);

            ClusterAttributesStoreData(
                ClusterHealthId const & clusterId,
                ServiceModel::ClusterHealthPolicySPtr const & healthPolicy,
                ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradeHealthPolicy,
                ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
                Store::ReplicaActivityId const & activityId);

            ClusterAttributesStoreData(ClusterAttributesStoreData const & other) = default;
            ClusterAttributesStoreData & operator = (ClusterAttributesStoreData const & other) = default;

            ClusterAttributesStoreData(ClusterAttributesStoreData && other) = default;
            ClusterAttributesStoreData & operator = (ClusterAttributesStoreData && other) = default;
            
            virtual ~ClusterAttributesStoreData();
            
            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const; 

            __declspec (property(get = get_EntityId)) ClusterHealthId const & EntityId;
            ClusterHealthId const & get_EntityId() const { return this->Key; }

            virtual FABRIC_INSTANCE_ID get_InstanceId() const override { return Constants::UnusedInstanceValue; }
            
            __declspec (property(get=get_HealthPolicyString, put=set_HealthPolicyString)) std::wstring const & HealthPolicyString;
            std::wstring const & get_HealthPolicyString() const { return healthPolicyString_; }
            void set_HealthPolicyString(std::wstring const & value);

            __declspec (property(get = get_UpgradeHealthPolicyString, put=set_UpgradeHealthPolicyString)) std::wstring const & UpgradeHealthPolicyString;
            std::wstring const & get_UpgradeHealthPolicyString() const { return upgradeHealthPolicyString_; }
            void set_UpgradeHealthPolicyString(std::wstring const & value);

            __declspec (property(get = get_ApplicationHealthPoliciesString, put = set_ApplicationHealthPoliciesString)) std::wstring const & ApplicationHealthPoliciesString;
            std::wstring const & get_ApplicationHealthPoliciesString() const { return applicationHealthPoliciesString_; }
            void set_ApplicationHealthPoliciesString(std::wstring const & value);

            __declspec(property(get=get_HealthPolicy)) ServiceModel::ClusterHealthPolicySPtr const & HealthPolicy;
            ServiceModel::ClusterHealthPolicySPtr const & get_HealthPolicy() const;
            
            __declspec(property(get = get_UpgradeHealthPolicy)) ServiceModel::ClusterUpgradeHealthPolicySPtr const & UpgradeHealthPolicy;
            ServiceModel::ClusterUpgradeHealthPolicySPtr const & get_UpgradeHealthPolicy() const;
            
            __declspec(property(get = get_ApplicationHealthPolicies)) ServiceModel::ApplicationHealthPolicyMapSPtr const & ApplicationHealthPolicies;
            ServiceModel::ApplicationHealthPolicyMapSPtr const & get_ApplicationHealthPolicies() const;

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

            FABRIC_FIELDS_05(healthPolicyString_, stateFlags_, attributeFlags_, upgradeHealthPolicyString_, applicationHealthPoliciesString_);

            DECLARE_ATTRIBUTES_COMMON_METHODS( ClusterAttributesStoreData, ClusterHealthId )
                        
        protected:

            // Gets the key that will be used as unique identifier of the record in the store
            virtual std::wstring ConstructKey() const override;

        private:        
            std::wstring healthPolicyString_;

            std::wstring upgradeHealthPolicyString_;

            std::wstring applicationHealthPoliciesString_;
            
            // Non-persisted, used only to avoid serialization/deserialization and copying on health evaluation

            mutable ServiceModel::ClusterHealthPolicySPtr healthPolicy_;
            mutable ServiceModel::ClusterUpgradeHealthPolicySPtr upgradeHealthPolicy_;
            mutable ServiceModel::ApplicationHealthPolicyMapSPtr applicationHealthPolicies_;
        };
    }
}
