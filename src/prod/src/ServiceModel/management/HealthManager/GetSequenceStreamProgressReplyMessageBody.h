// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class GetSequenceStreamProgressReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            GetSequenceStreamProgressReplyMessageBody()
                : sequenceStreamResult_()
            {
            }

            explicit GetSequenceStreamProgressReplyMessageBody(
                SequenceStreamResult && sequenceStreamResult)
                : sequenceStreamResult_(std::move(sequenceStreamResult))
            {
            }

            SequenceStreamResult && MoveSequenceStreamResult() { return std::move(sequenceStreamResult_); }

            FABRIC_FIELDS_01(sequenceStreamResult_);

        private:
            SequenceStreamResult sequenceStreamResult_;
        };
    }
}
