// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class TcpReceiveBuffer : public ReceiveBuffer
    {
    public:
        TcpReceiveBuffer(TcpConnection* connectionPtr);

        NTSTATUS GetNextMessage(_Out_ MessageUPtr & message, Common::StopwatchTime recvTime) override;
        void ConsumeCurrentMessage() override;

    private:
        TcpFrameHeader currentFrame_;
        TcpFrameHeader firstFrameHeader_;
    };
}
