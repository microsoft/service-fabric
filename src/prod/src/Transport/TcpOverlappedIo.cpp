// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include"stdafx.h"

using namespace Transport;
using namespace Common;

_Use_decl_annotations_
TcpConnectOverlapped::TcpConnectOverlapped(TcpConnection & owner) : owner_(owner)
{
}

void TcpConnectOverlapped::OnComplete(PTP_CALLBACK_INSTANCE, ErrorCode const & result, ULONG_PTR)
{
    owner_.ConnectComplete(result);
}

_Use_decl_annotations_
TcpSendOverlapped::TcpSendOverlapped(TcpConnection & owner) : owner_(owner)
{
}

void TcpSendOverlapped::OnComplete(PTP_CALLBACK_INSTANCE, ErrorCode const & result, ULONG_PTR bytesTransferred)
{
    owner_.SendComplete(result, bytesTransferred);
}

_Use_decl_annotations_
TcpReceiveOverlapped::TcpReceiveOverlapped(TcpConnection & owner) : owner_(owner)
{
}

void TcpReceiveOverlapped::OnComplete(PTP_CALLBACK_INSTANCE, ErrorCode const & result, ULONG_PTR bytesTransferred)
{
    owner_.ReceiveComplete(result, bytesTransferred);
}

