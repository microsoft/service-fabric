// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Transport;

ComFabricTransportClientConnection::ComFabricTransportClientConnection(IClientConnectionPtr const & impl)
: IFabricTransportClientConnection(),
ComUnknownBase(),
impl_(impl)
{
}

HRESULT  ComFabricTransportClientConnection::Send(
    /* [in] */ IFabricTransportMessage *message)
{
    MessageUPtr msg;
    auto    hr = ComFabricTransportMessage::ToNativeTransportMessage(message,
        [](ComPointer<IFabricTransportMessage> msg) 
    {
        msg->Dispose();
    },
        msg);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
    auto error = impl_->SendOneWay(move(msg));
    return ComUtility::OnPublicApiReturn(error.ToHResult());
}


COMMUNICATION_CLIENT_ID  ComFabricTransportClientConnection::get_ClientId()
{
    return impl_->Get_ClientId().c_str();
}
