// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class GetInvokeQuorumLossProgressReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            GetInvokeQuorumLossProgressReplyMessageBody() : progress_() { }

            GetInvokeQuorumLossProgressReplyMessageBody(InvokeQuorumLossProgress && progress) : progress_(std::move(progress)) { }

            __declspec(property(get=get_Progress)) InvokeQuorumLossProgress & Progress;

            InvokeQuorumLossProgress & get_Progress() { return progress_; }

            FABRIC_FIELDS_01(progress_);

        private:
            InvokeQuorumLossProgress progress_;
        };
    }
}
