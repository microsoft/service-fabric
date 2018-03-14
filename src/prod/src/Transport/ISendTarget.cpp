// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;

ErrorCode ISendTarget::SendOneWay(MessageUPtr &&, TimeSpan, TransportPriority::Enum)
{
    Assert::CodingError("not implemented");
}

size_t ISendTarget::BytesPendingForSend()
{
    return 0;
}

size_t ISendTarget::MessagesPendingForSend()
{
    return 0;
}

bool ISendTarget::TryEnableDuplicateDetection()
{
    return false;
}

TransportFlags ISendTarget::GetTransportFlags() const
{
    return TransportFlags::None;
}

void ISendTarget::TargetDown(uint64)
{
}

void ISendTarget::Reset()
{
}

size_t ISendTarget::MaxOutgoingMessageSize() const
{
    return TcpFrameHeader::FrameSizeHardLimit();
}

void ISendTarget::Test_Block()
{
}

void ISendTarget::Test_Unblock()
{
}
