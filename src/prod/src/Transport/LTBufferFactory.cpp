// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

LTBufferFactory::~LTBufferFactory()
{
}

unique_ptr<SendBuffer> LTBufferFactory::CreateSendBuffer(TcpConnection* connection)
{
    return make_unique<LTSendBuffer>(connection);
}

unique_ptr<ReceiveBuffer> LTBufferFactory::CreateReceiveBuffer(TcpConnection* connection)
{
    return make_unique<LTReceiveBuffer>(connection);
}
