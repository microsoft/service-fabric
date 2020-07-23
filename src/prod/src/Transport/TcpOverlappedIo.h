// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class TcpSendOverlapped : public Common::OverlappedIo
    {
    public:
        TcpSendOverlapped(_In_ TcpConnection & owner);

    private:
        virtual void OnComplete(PTP_CALLBACK_INSTANCE, Common::ErrorCode const & error, ULONG_PTR bytesTransfered) override;
        TcpConnection & owner_;
    };

    class TcpReceiveOverlapped : public Common::OverlappedIo
    {
    public:
        TcpReceiveOverlapped(_In_ TcpConnection & owner);

    private:
        virtual void OnComplete(PTP_CALLBACK_INSTANCE, Common::ErrorCode const & error, ULONG_PTR bytesTransfered) override;
        TcpConnection & owner_;
    };

    class TcpConnectOverlapped : public Common::OverlappedIo
    {
    public:
        TcpConnectOverlapped(_In_ TcpConnection & owner);

    private:
        virtual void OnComplete(PTP_CALLBACK_INSTANCE, Common::ErrorCode const & error, ULONG_PTR bytesTransfered) override;
        TcpConnection & owner_;
    };
}
