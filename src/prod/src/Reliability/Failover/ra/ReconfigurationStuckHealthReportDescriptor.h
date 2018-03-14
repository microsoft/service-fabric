// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ReconfigurationStuckHealthReportDescriptor : public Health::HealthReportDescriptorBase
        {
            DENY_COPY(ReconfigurationStuckHealthReportDescriptor);

        public:

            ReconfigurationStuckHealthReportDescriptor();

            ReconfigurationStuckHealthReportDescriptor(
                ReconfigurationProgressStages::Enum reconfigurationStuckReason,
                std::vector<HealthReportReplica> && replicas,
                Common::StopwatchTime const & reconfigurationStartTime,
                Common::StopwatchTime const & reconfigurationPhaseStartTime);

			bool operator == (ReconfigurationStuckHealthReportDescriptor const& rhs);
			bool operator != (ReconfigurationStuckHealthReportDescriptor const& rhs);

			__declspec(property(get = get_ReconfigurationStartTime)) Common::StopwatchTime const & ReconfigurationStartTime;
			Common::StopwatchTime const & get_ReconfigurationStartTime() const { return reconfigurationStartTime_; }

			__declspec(property(get = get_ReconfigurationPhaseStartTime)) Common::StopwatchTime const & ReconfigurationPhaseStartTime;
			Common::StopwatchTime const & get_ReconfigurationPhaseStartTime() const { return reconfigurationPhaseStartTime_; }

            __declspec(property(get = get_Replicas)) vector<HealthReportReplica> const & Replicas;
            vector<HealthReportReplica> const & get_Replicas() { return replicas_; }

            __declspec(property(get = get_ReconfigurationStuckReason)) ReconfigurationProgressStages::Enum ReconfigurationStuckReason;
            ReconfigurationProgressStages::Enum  get_ReconfigurationStuckReason() const { return reconfigurationStuckReason_; }

            inline int Test_GetReplicaCount() const { return (int) replicas_.size(); }

            std::wstring GenerateReportDescriptionInternal(Health::HealthContext const & c) const override;
            std::wstring GenerateReportDescriptionInternal() const;

        private:            
            std::wstring GetReasonDescription(size_t replicas) const;
            void TestAndWriteReplicaVector(const std::vector<HealthReportReplica> & v, Common::StringWriter & writer, wstring vectorName) const;
			bool CompareHealthReportReplicaVectors(std::vector<HealthReportReplica> const & lhs, std::vector<HealthReportReplica> const & rhs);

            const std::wstring documentationUrl_ = L"http://aka.ms/sfhealth";
            ReconfigurationProgressStages::Enum reconfigurationStuckReason_;
			std::vector<HealthReportReplica> replicas_;
            bool isClearWarningDescriptor_;
            Common::StopwatchTime const reconfigurationStartTime_;
            Common::StopwatchTime const reconfigurationPhaseStartTime_;
        };
    }
}
