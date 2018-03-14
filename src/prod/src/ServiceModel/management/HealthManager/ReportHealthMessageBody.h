// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ReportHealthMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            ReportHealthMessageBody()
                : healthReports_()
                , sequenceStreamInformationList_()
            {
            }

            ReportHealthMessageBody(
                std::vector<ServiceModel::HealthReport> && healthReports,
                std::vector<SequenceStreamInformation> && sequenceStreamInformationList)
                : healthReports_(std::move(healthReports))
                , sequenceStreamInformationList_(std::move(sequenceStreamInformationList))
            {
            }

            std::vector<ServiceModel::HealthReport> && MoveEntries() { return std::move(healthReports_); }

            std::vector<SequenceStreamInformation> && MoveSequenceStreamInfo() { return std::move(sequenceStreamInformationList_); }

            FABRIC_FIELDS_02(healthReports_, sequenceStreamInformationList_);

        private:
            std::vector<ServiceModel::HealthReport> healthReports_;
            std::vector<SequenceStreamInformation> sequenceStreamInformationList_;
        };
    }
}
