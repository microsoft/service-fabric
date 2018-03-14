// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        class UpgradeOrchestrationServiceState
			: public Serialization::FabricSerializable
			, public Common::IFabricJsonSerializable
        {
        public:
			UpgradeOrchestrationServiceState();

			UpgradeOrchestrationServiceState(UpgradeOrchestrationServiceState &&);

			UpgradeOrchestrationServiceState & operator=(UpgradeOrchestrationServiceState &&);

			UpgradeOrchestrationServiceState(
				const std::wstring &currentCodeVersion,
				const std::wstring &currentManifestVersion,
				const std::wstring &targetCodeVersion,
				const std::wstring &targetManifestVersion,
				const std::wstring &pendingUpgradeType);

			__declspec(property(get = get_CurrentCodeVersion)) std::wstring & CurrentCodeVersion;
			std::wstring const & get_CurrentCodeVersion() const { return currentCodeVersion_; }

			__declspec(property(get = get_CurrentManifestVersion)) std::wstring & CurrentManifestVersion;
			std::wstring const & get_CurrentManifestVersion() const { return currentManifestVersion_; }

			__declspec(property(get = get_TargetCodeVersion)) std::wstring & TargetCodeVersion;
			std::wstring const & get_TargetCodeVersion() const { return targetCodeVersion_; }

			__declspec(property(get = get_TargetManifestVersion)) std::wstring & TargetManifestVersion;
			std::wstring const & get_TargetManifestVersion() const { return targetManifestVersion_; }

			__declspec(property(get = get_PendingUpgradeType)) std::wstring & PendingUpgradeType;
			std::wstring const & get_PendingUpgradeType() const { return pendingUpgradeType_; }

			void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_UPGRADE_ORCHESTRATION_SERVICE_STATE &) const;

            FABRIC_FIELDS_05(currentCodeVersion_, currentManifestVersion_, targetCodeVersion_, targetManifestVersion_, pendingUpgradeType_);

			BEGIN_JSON_SERIALIZABLE_PROPERTIES()
				SERIALIZABLE_PROPERTY(ServiceModel::Constants::CurrentCodeVersion, currentCodeVersion_)
				SERIALIZABLE_PROPERTY(ServiceModel::Constants::CurrentManifestVersion, currentManifestVersion_)
				SERIALIZABLE_PROPERTY(ServiceModel::Constants::TargetCodeVersion, targetCodeVersion_)
				SERIALIZABLE_PROPERTY(ServiceModel::Constants::TargetManifestVersion, targetManifestVersion_)
				SERIALIZABLE_PROPERTY(ServiceModel::Constants::PendingUpgradeType, pendingUpgradeType_)
			END_JSON_SERIALIZABLE_PROPERTIES()

        private:

			std::wstring currentCodeVersion_;
			std::wstring currentManifestVersion_;
			std::wstring targetCodeVersion_;
			std::wstring targetManifestVersion_;
			std::wstring pendingUpgradeType_;
        };
    }
}
