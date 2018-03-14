// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    typedef std::function<void(Transport::MessageUPtr &, OneWayReceiverContextUPtr &)> OneWayMessageHandler;
    typedef std::function<void(Transport::MessageUPtr &, RequestReceiverContextUPtr &)> RequestMessageHandler;

    class MessageHandlerPair
    {
    public:
        MessageHandlerPair()
        {
        }

        MessageHandlerPair(OneWayMessageHandler const & oneWay, RequestMessageHandler const & request, bool dispatchOnTransportThread)
            : oneWay_(oneWay), request_(request), dispatchOnTransportThread_(dispatchOnTransportThread)
        {
        }

        bool HandleOneWayMessage(__inout Transport::MessageUPtr & message, __inout OneWayReceiverContextUPtr & context)
        {
            if (!oneWay_)
            {
                return false;
            }

            if (dispatchOnTransportThread_)
            {
                oneWay_(message, context);
            }
            else
            {
                Common::MoveUPtr<Transport::Message> messageMover(std::move(message));
                Common::MoveUPtr<OneWayReceiverContext> contextMover(std::move(context));
                OneWayMessageHandler & oneWayMessageHandler = oneWay_;
                Common::Threadpool::Post([oneWayMessageHandler, messageMover, contextMover] () mutable
                {
                    Transport::MessageUPtr msg = messageMover.TakeUPtr();
                    OneWayReceiverContextUPtr ctx = contextMover.TakeUPtr();
                    oneWayMessageHandler(msg, ctx);
                });
            }

            return true;
        }

        bool HandleRequestMessage(__inout Transport::MessageUPtr & message, __inout RequestReceiverContextUPtr & context)
        {
            if (!request_)
            {
                return false;
            }

            if (dispatchOnTransportThread_)
            {
                request_(message, context);
            }
            else
            {
                Common::MoveUPtr<Transport::Message> messageMover(std::move(message));
                Common::MoveUPtr<RequestReceiverContext> contextMover(std::move(context));
                RequestMessageHandler & requestHandler = request_;
                Common::Threadpool::Post([requestHandler, messageMover, contextMover] () mutable
                {
                    Transport::MessageUPtr msg = messageMover.TakeUPtr();
                    RequestReceiverContextUPtr ctx = contextMover.TakeUPtr();
                    requestHandler(msg, ctx);
                });
            }

            return true;
        }

    private:
        OneWayMessageHandler oneWay_;
        RequestMessageHandler request_;
        bool dispatchOnTransportThread_;
    };
}
