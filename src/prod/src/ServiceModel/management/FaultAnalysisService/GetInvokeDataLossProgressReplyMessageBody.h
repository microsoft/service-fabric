// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class GetInvokeDataLossProgressReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            GetInvokeDataLossProgressReplyMessageBody() : progress_() { }

            GetInvokeDataLossProgressReplyMessageBody(InvokeDataLossProgress && progress) : progress_(std::move(progress)) { }

            __declspec(property(get=get_Progress)) InvokeDataLossProgress & Progress;

            InvokeDataLossProgress & get_Progress() { return progress_; }

            FABRIC_FIELDS_01(progress_);

        private:
            InvokeDataLossProgress progress_;
        };
    }
}
