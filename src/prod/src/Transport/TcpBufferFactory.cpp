// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

unique_ptr<SendBuffer> TcpBufferFactory::CreateSendBuffer(TcpConnection* connection)
{
    return make_unique<TcpSendBuffer>(connection);
}

unique_ptr<ReceiveBuffer> TcpBufferFactory::CreateReceiveBuffer(TcpConnection* connection)
{
    return make_unique<TcpReceiveBuffer>(connection);
}
