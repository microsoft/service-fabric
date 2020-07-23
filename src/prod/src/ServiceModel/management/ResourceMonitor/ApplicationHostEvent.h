// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceMonitor
    {
        class ApplicationHostEvent : public Serialization::FabricSerializable
        {
        public:

            ApplicationHostEvent() = default;
            ApplicationHostEvent(
                Hosting2::CodePackageInstanceIdentifier const & codePackageInstanceIdentifier,
                std::wstring const & appName,
                ServiceModel::EntryPointType::Enum hostType,
                std::wstring const & appHostId,
                bool isUp,
                bool isLinuxContainerIsolated = false);

            __declspec(property(get=get_CodePackageInstanceIdentifier)) Hosting2::CodePackageInstanceIdentifier const & CodePackageInstanceIdentifier;
            Hosting2::CodePackageInstanceIdentifier const &  get_CodePackageInstanceIdentifier() const { return codePackageInstanceIdentifier_; }

            __declspec(property(get = get_PartitionId)) Common::Guid const & PartitionId;
            Common::Guid const & get_PartitionId() const { return codePackageInstanceIdentifier_.ServicePackageInstanceId.ActivationContext.ActivationGuid; }

            __declspec(property(get = get_ApplicationName)) std::wstring const & AppName;
            std::wstring const & get_ApplicationName() const { return appName_; }

            __declspec(property(get=get_JobObjectName)) std::wstring const & AppHostId;
            std::wstring const & get_JobObjectName() const { return appHostId_; }

            __declspec(property(get=get_HostType)) ServiceModel::EntryPointType::Enum HostType;
            ServiceModel::EntryPointType::Enum get_HostType() const { return hostType_; }

            __declspec(property(get=get_IsUp)) bool IsUp;
            bool get_IsUp() const { return isUp_; }

            __declspec(property(get = get_IsLinuxContainerIsolated)) bool IsLinuxContainerIsolated;
            bool get_IsLinuxContainerIsolated() const { return isLinuxContainerIsolated_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_06(codePackageInstanceIdentifier_,appName_, hostType_, appHostId_, isUp_, isLinuxContainerIsolated_);

        private:
            Hosting2::CodePackageInstanceIdentifier codePackageInstanceIdentifier_;
            std::wstring appName_;
            ServiceModel::EntryPointType::Enum hostType_;
            std::wstring appHostId_;
            bool isUp_;
            bool isLinuxContainerIsolated_;
        };
    }
}
