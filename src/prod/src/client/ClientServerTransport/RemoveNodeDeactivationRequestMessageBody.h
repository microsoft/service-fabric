// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class RemoveNodeDeactivationRequestMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:

        RemoveNodeDeactivationRequestMessageBody()
        {
        }

        RemoveNodeDeactivationRequestMessageBody(std::wstring const& batchId, int64 sequenceNumber)
            : batchId_(batchId),
              sequenceNumber_(sequenceNumber)
        {
        }

        __declspec (property(get=get_BatchId)) std::wstring const& BatchId;
        std::wstring const& get_BatchId() const { return batchId_; }

        __declspec (property(get=get_SequenceNumber)) int64 SequenceNumber;
        int64 get_SequenceNumber() const { return sequenceNumber_; }

        FABRIC_FIELDS_02(batchId_, sequenceNumber_);

    private:

        std::wstring batchId_;
        int64 sequenceNumber_;
    };
}
