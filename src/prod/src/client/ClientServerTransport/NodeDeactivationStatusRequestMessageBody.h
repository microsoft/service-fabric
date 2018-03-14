// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeDeactivationStatusRequestMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:

        NodeDeactivationStatusRequestMessageBody()
        {
        }

        NodeDeactivationStatusRequestMessageBody(std::wstring const& batchId)
            : batchId_(batchId)
        {
        }

        __declspec (property(get=get_BatchId)) std::wstring const& BatchId;
        std::wstring const& get_BatchId() const { return batchId_; }

        FABRIC_FIELDS_01(batchId_);

    private:

        std::wstring batchId_;
    };
}
