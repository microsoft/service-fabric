// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class RestartPartitionMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            RestartPartitionMessageBody() : restartPartitionDescription_() { }

            RestartPartitionMessageBody(RestartPartitionDescription const & restartPartitionDescription) : restartPartitionDescription_(restartPartitionDescription) { }

            __declspec(property(get = get_RestartPartitionDescription)) RestartPartitionDescription const & Description;

            RestartPartitionDescription const & get_RestartPartitionDescription() const { return restartPartitionDescription_; }

            FABRIC_FIELDS_01(restartPartitionDescription_);

        private:
            RestartPartitionDescription restartPartitionDescription_;
        };
    }
}
