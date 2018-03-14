// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class GetNodeTransitionProgressReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            GetNodeTransitionProgressReplyMessageBody() : progress_() { }

            GetNodeTransitionProgressReplyMessageBody(NodeTransitionProgress && progress) : progress_(std::move(progress)) { }

            __declspec(property(get = get_Progress)) NodeTransitionProgress & Progress;

            NodeTransitionProgress & get_Progress() { return progress_; }

            FABRIC_FIELDS_01(progress_);

        private:
            NodeTransitionProgress progress_;
        };
    }
}
