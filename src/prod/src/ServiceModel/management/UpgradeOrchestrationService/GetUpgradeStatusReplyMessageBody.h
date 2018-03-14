// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        class GetUpgradeStatusReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            GetUpgradeStatusReplyMessageBody() : progress_() { }

            GetUpgradeStatusReplyMessageBody(OrchestrationUpgradeProgress && progress) : progress_(std::move(progress)) { }

            __declspec(property(get = get_Progress)) OrchestrationUpgradeProgress & Progress;

            OrchestrationUpgradeProgress & get_Progress() { return progress_; }

            FABRIC_FIELDS_01(progress_);

        private:
            OrchestrationUpgradeProgress progress_;
        };
    }
}
