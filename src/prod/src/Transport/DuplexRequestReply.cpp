// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Transport;

DuplexRequestReply::DuplexRequestReply(ComponentRoot const & root, IDatagramTransportSPtr const & datagramTransport)
    : RequestReply(root, datagramTransport)
    , notificationHandler_(nullptr)
    , notificationHandlerLock_()
    , dispatchOnTransportThread_(false)
{
}

void DuplexRequestReply::SetNotificationHandler(NotificationHandler const & handler, bool dispatchOnTransportThread)
{
    AcquireWriteLock lock(notificationHandlerLock_);

    notificationHandler_ = handler;
    dispatchOnTransportThread_ = dispatchOnTransportThread;
}

void DuplexRequestReply::RemoveNotificationHandler()
{
    AcquireWriteLock lock(notificationHandlerLock_);

    notificationHandler_ = nullptr;
}

bool DuplexRequestReply::OnReplyMessage(Message & reply, ISendTarget::SPtr const & sendTarget)
{
    if (reply.IsUncorrelatedReply)
    {
        NotificationHandler handler;
        {
            AcquireReadLock lock(notificationHandlerLock_);

            handler = notificationHandler_;
        }

        if (handler)
        {
            auto receiverContext = make_unique<ReceiverContext>(
                reply.MessageId, 
                sendTarget, 
                this->GetDatagramTransport());

            if (!dispatchOnTransportThread_)
            {
                MoveUPtr<Message> messageMover(reply.Clone());
                MoveUPtr<ReceiverContext> contextMover(move(receiverContext));

                Threadpool::Post([handler, messageMover, contextMover] () mutable
                {
                    auto context = contextMover.TakeUPtr();
                    MessageUPtr replyUPtr = messageMover.TakeUPtr();
                    handler(*replyUPtr, context);
                });
            }
            else
            {
                handler(reply, receiverContext);
            }
        }

        return handler != nullptr;
    }
    else
    {
        return RequestReply::OnReplyMessage(reply, sendTarget);
    }
}
