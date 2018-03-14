// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class GetRestartPartitionProgressReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            GetRestartPartitionProgressReplyMessageBody() : progress_() { }

            GetRestartPartitionProgressReplyMessageBody(RestartPartitionProgress && progress) : progress_(std::move(progress)) { }

            __declspec(property(get = get_Progress)) RestartPartitionProgress & Progress;

            RestartPartitionProgress & get_Progress() { return progress_; }

            FABRIC_FIELDS_01(progress_);

        private:
            RestartPartitionProgress progress_;
        };
    }
}
