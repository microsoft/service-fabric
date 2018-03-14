// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ReportHealthReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            ReportHealthReplyMessageBody()
                : healthReportResultList_()
                , sequenceStreamResultList_()
            {
            }

            ReportHealthReplyMessageBody(
                std::vector<HealthReportResult> && healthReportResultList,
                std::vector<SequenceStreamResult> && sequenceStreamResultList)
                : healthReportResultList_(move(healthReportResultList))
                , sequenceStreamResultList_(move(sequenceStreamResultList))
            {
            }

            ReportHealthReplyMessageBody(
                std::vector<HealthReportResult> const & healthReportResultList,
                std::vector<SequenceStreamResult> const & sequenceStreamResultList)
                : healthReportResultList_(healthReportResultList)
                , sequenceStreamResultList_(sequenceStreamResultList)
            {
            }

            __declspec(property(get=get_HealthReportResultList)) std::vector<HealthReportResult> const& HealthReportResultList;
            std::vector<HealthReportResult> const& get_HealthReportResultList() const { return healthReportResultList_; }

            std::vector<HealthReportResult> && MoveEntries() { return std::move(healthReportResultList_); }

            std::vector<SequenceStreamResult> && MoveSequenceStreamResults() { return std::move(sequenceStreamResultList_); }

            FABRIC_FIELDS_02(healthReportResultList_, sequenceStreamResultList_);

        private:
            std::vector<HealthReportResult> healthReportResultList_;
            std::vector<SequenceStreamResult> sequenceStreamResultList_;
        };
    }
}
