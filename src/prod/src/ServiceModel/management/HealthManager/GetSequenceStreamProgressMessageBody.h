// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class GetSequenceStreamProgressMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            GetSequenceStreamProgressMessageBody()
                : sequenceStreamId_()
                , instance_(FABRIC_INVALID_INSTANCE_ID)
            {
            }

            GetSequenceStreamProgressMessageBody(
                SequenceStreamId const & sequenceStreamId,
                FABRIC_INSTANCE_ID instance)
                : sequenceStreamId_(sequenceStreamId)
                , instance_(instance)
            {
            }

            SequenceStreamId && MoveSequenceStreamId() { return std::move(sequenceStreamId_); }

            __declspec (property(get=get_Instance)) FABRIC_INSTANCE_ID Instance;
            FABRIC_INSTANCE_ID get_Instance() const { return instance_; }

            FABRIC_FIELDS_02(sequenceStreamId_, instance_);

        private:
            SequenceStreamId sequenceStreamId_;
            FABRIC_INSTANCE_ID instance_;
        };
    }
}
