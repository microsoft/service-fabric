// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComFabricTransportDummyCallbackHandler.h"
#include "api/wrappers/ApiWrappers.h"

using namespace Common;
using namespace std;
using  namespace Communication::TcpServiceCommunication;
using namespace Api;

ComFabricTransportDummyCallbackHandler::ComFabricTransportDummyCallbackHandler()
{
}

HRESULT  ComFabricTransportDummyCallbackHandler::HandleOneWay(
    IFabricTransportMessage *message)
{
	UNREFERENCED_PARAMETER(message);
    
    this->recievedEvent.Set();
    return S_OK;
}

bool ComFabricTransportDummyCallbackHandler::IsMessageRecieved(TimeSpan timespan)
{
    return this->recievedEvent.WaitOne(timespan);
}
