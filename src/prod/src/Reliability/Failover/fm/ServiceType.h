// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ServiceType : public StoreData
        {
            DENY_COPY(ServiceType);

        public:
            ServiceType();

            ServiceType(
                ServiceModel::ServiceTypeIdentifier const& typeId,
                ApplicationEntrySPtr const& applicationEntry);

            ServiceType(
                ServiceType const& other,
                ApplicationEntrySPtr const& applicationEntry);

            ServiceType(
                ServiceType const & other,
                Federation::NodeId const & nodeId,
                ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent);

            ServiceType(
                ServiceType const& other,
                ServiceModel::ServicePackageVersionInstance const& versionInstance,
                std::wstring const& codePackageName);

            static std::wstring const& GetStoreType();
            virtual std::wstring const& GetStoreKey() const;

            __declspec (property(get=get_TypeId)) ServiceModel::ServiceTypeIdentifier const& Type;
            ServiceModel::ServiceTypeIdentifier const& get_TypeId() const { return type_; }

            __declspec (property(get=get_Application)) ApplicationInfoSPtr const Application;
            ApplicationInfoSPtr const get_Application() const { return applicationEntry_->Get(); }

            __declspec (property(get=get_LastUpdateTime)) Common::DateTime LastUpdateTime;
            Common::DateTime get_LastUpdateTime() const { return lastUpdateTime_; }

            __declspec (property(get=get_IsStale, put=set_IsStale)) bool IsStale;
            bool get_IsStale() const { return isStale_; }
            void set_IsStale(bool value) { isStale_ = value; }

            LoadBalancingComponent::ServiceTypeDescription GetPLBServiceTypeDescription() const;

            bool IsServiceTypeDisabled(Federation::NodeId const& nodeId) const;

            bool TryGetCodePackageName(__out std::wstring & codePackageName) const;
            bool TryGetServicePackageVersionInstance(__out ServiceModel::ServicePackageVersionInstance & servicePackageVersionInstance) const;

            void PostUpdate(Common::DateTime updateTime);

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

            FABRIC_FIELDS_04(type_, serviceTypeDisabledNodes_, lastUpdateTime_, codePackages_);

        private:
            ServiceModel::ServiceTypeIdentifier type_;

            // Nodes with a disabled Service Type
            std::vector<Federation::NodeId> serviceTypeDisabledNodes_;

            std::map<ServiceModel::ServicePackageVersionInstance, std::wstring> codePackages_;

            ApplicationEntrySPtr applicationEntry_;

            Common::DateTime lastUpdateTime_;

            bool isStale_;

            mutable std::wstring idString_;
        };
    }
}

DEFINE_USER_MAP_UTILITY(ServiceModel::ServicePackageVersionInstance, std::wstring);
