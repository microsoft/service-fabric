// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class ListenSocket
        : public IListenSocket
#ifndef PLATFORM_UNIX
        , public Common::OverlappedIo
#endif
    {
    public:
        ListenSocket(
            Common::Endpoint const & endpoint,
            AcceptCompleteCallback && acceptCompleteCallback,
            std::wstring const & traceId);

        ~ListenSocket() { Abort(); }

        Common::ErrorCode SubmitAccept() override;

    protected:
        Common::ErrorCode OnOpen() override;
        Common::ErrorCode OnClose() override;
        void OnAbort() override;

    private:
#ifdef PLATFORM_UNIX
        void AcceptCallback(int sd, uint events);

        Common::EventLoop & eventLoop_;
        Common::EventLoop::FdContext* fdContext_;
#else
        virtual void OnComplete(PTP_CALLBACK_INSTANCE, Common::ErrorCode const & error, ULONG_PTR) override;

        Common::ThreadpoolIo io_;
        byte connectData_[Common::Socket::MIN_ACCEPTEX_BUFFER_SIZE];
#endif
    };
}
