// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        class OrchestrationUpgradeProgress : public Serialization::FabricSerializable
        {
        public:
            OrchestrationUpgradeProgress();

            OrchestrationUpgradeProgress(OrchestrationUpgradeProgress &&);

            OrchestrationUpgradeProgress & operator=(OrchestrationUpgradeProgress &&);

			OrchestrationUpgradeProgress(OrchestrationUpgradeState::Enum state, uint32 progressStatus, const std::wstring &configVersion, const std::wstring &details);

            __declspec(property(get = get_State)) UpgradeOrchestrationService::OrchestrationUpgradeState::Enum State;
            OrchestrationUpgradeState::Enum const & get_State() const { return state_; }

            __declspec(property(get = get_ProgressStatus)) uint32 ProgressStatus;
            uint32 const & get_ProgressStatus() const { return progressStatus_; }

			__declspec(property(get = get_ConfigVersion)) std::wstring ConfigVersion;
			std::wstring const & get_ConfigVersion() const { return configVersion_; }

            __declspec(property(get = get_Details)) std::wstring Details;
            std::wstring const & get_Details() const { return details_; }

            Common::ErrorCode FromPublicApi(__in FABRIC_ORCHESTRATION_UPGRADE_PROGRESS const &);
			void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_ORCHESTRATION_UPGRADE_PROGRESS &) const;

            FABRIC_FIELDS_04(state_, progressStatus_, configVersion_, details_);

        private:
            OrchestrationUpgradeState::Enum state_;
            uint32 progressStatus_;
			std::wstring configVersion_;
            std::wstring details_;
        };
    }
}
