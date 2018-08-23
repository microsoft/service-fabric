// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class LTReceiveBuffer : public ReceiveBuffer
    {
    public:
        LTReceiveBuffer(TcpConnection* connectionPtr);

        NTSTATUS GetNextMessage(_Out_ MessageUPtr & message, Common::StopwatchTime recvTime) override;
        void ConsumeCurrentMessage() override;

    private:
        void AssembleMessage(MessageUPtr & message, Common::StopwatchTime recvTime, bool hasFrameHeader = true);

        LTFrameHeader currentFrame_;
    };
}
