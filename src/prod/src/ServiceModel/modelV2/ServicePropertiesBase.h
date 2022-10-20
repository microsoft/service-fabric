// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ServicePropertiesBase
        : public ModelType
        {
        public:
            ServicePropertiesBase() = default;

            DEFAULT_MOVE_ASSIGNMENT(ServicePropertiesBase)
            DEFAULT_MOVE_CONSTRUCTOR(ServicePropertiesBase)
            DEFAULT_COPY_ASSIGNMENT(ServicePropertiesBase)
            DEFAULT_COPY_CONSTRUCTOR(ServicePropertiesBase)

            __declspec(property(get=get_OsType, put=put_OsType)) std::wstring const & OsType;
            std::wstring const & get_OsType() const { return osType_; }
            void put_OsType(std::wstring const & value) { osType_ = value; }

            __declspec(property(get=get_CodePackages, put=put_CodePackages)) std::vector<ContainerCodePackageDescription> & CodePackages;
            std::vector<ContainerCodePackageDescription> & get_CodePackages() { return codePackages_; }
            void put_CodePackages(std::vector<ContainerCodePackageDescription> const & value) { codePackages_ = value; }

            __declspec(property(get = get_AutoScalingPolicies, put = put_AutoScalingPolicies)) std::vector<AutoScalingPolicy> & AutoScalingPolicies;
            std::vector<AutoScalingPolicy> & get_AutoScalingPolicies() { return autoScalingPolicies_; }
            void put_AutoScalingPolicies(std::vector<AutoScalingPolicy> const & value) { autoScalingPolicies_ = value; }

            __declspec(property(get=get_NetworkRefs, put=put_NetworkRefs)) std::vector<NetworkRef> const & NetworkRefs;
            std::vector<NetworkRef> const & get_NetworkRefs() const { return networkRefs_; }
            void put_NetworkRefs(std::vector<NetworkRef> const & value) { networkRefs_ = value; }

            bool operator==(ServicePropertiesBase const & other) const;

            bool CanUpgrade(ServicePropertiesBase const & other) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::osType, osType_)
                SERIALIZABLE_PROPERTY(L"codePackages", codePackages_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScalingPolicies, autoScalingPolicies_)
                SERIALIZABLE_PROPERTY(L"networkRefs", networkRefs_)
                SERIALIZABLE_PROPERTY(L"diagnostics", diagnostics_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_05(osType_, codePackages_, networkRefs_, diagnostics_, autoScalingPolicies_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(osType_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(codePackages_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(autoScalingPolicies_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(networkRefs_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(diagnostics_)
            END_DYNAMIC_SIZE_ESTIMATION()

            virtual Common::ErrorCode TryValidate(std::wstring const &) const override
            {
                return Common::ErrorCodeValue::Success;
            }

        protected:
            std::wstring osType_;
            std::vector<ContainerCodePackageDescription> codePackages_;
            std::vector<AutoScalingPolicy> autoScalingPolicies_;
            std::vector<NetworkRef> networkRefs_;
            std::shared_ptr<DiagnosticsRef> diagnostics_;
        };
    }
}
