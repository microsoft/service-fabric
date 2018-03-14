// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;

IpcReceiverContext::IpcReceiverContext(
    IpcHeader && ipcHeader,
    Transport::MessageId const & messageId, 
    ISendTarget::SPtr const & replyTargetSPtr,
    IDatagramTransportSPtr const & datagramTransport)
    : ReceiverContext(messageId, replyTargetSPtr, datagramTransport), ipcHeader_(std::move(ipcHeader))
{
}
