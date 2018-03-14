// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class IListenSocket : public Common::FabricComponent, public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
    public:
        typedef std::shared_ptr<IListenSocket> SPtr;
        typedef std::function<void(IListenSocket &, Common::ErrorCode const &)> AcceptCompleteCallback;

        IListenSocket(
            Common::Endpoint const & endpoint,
            AcceptCompleteCallback && acceptCompleteCallback,
            std::wstring const & traceId);

        virtual ~IListenSocket() = default; 

        Common::Endpoint const & ListenEndpoint() const;

        virtual Common::ErrorCode SubmitAccept() = 0;
        void SuspendAccept();
        bool AcceptSuspended() const;

        Common::Socket & AcceptedSocket();
        Common::Endpoint const & AcceptedRemoteEndpoint() const;

    protected:
        Common::ErrorCode Bind();
        Common::ErrorCode AdjustSockName();

        Common::Endpoint listenEndpoint_;
        const AcceptCompleteCallback acceptCompleteCallback_;
        const std::wstring traceId_;
        Common::Socket listenSocket_;
        Common::Socket acceptedSocket_;
        Common::Endpoint acceptedRemoteEndpoint_;
        bool suspended_;
    };
}
