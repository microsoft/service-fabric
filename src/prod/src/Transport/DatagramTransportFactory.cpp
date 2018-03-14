// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

IDatagramTransportSPtr DatagramTransportFactory::CreateTcp(
    wstring const & address,
    wstring const & id,
    wstring const & owner)
{
    if (TransportConfig::GetConfig().InMemoryTransportEnabled)
    {
        return CreateMem(address, id);
    }

    return TcpDatagramTransport::Create(address, id, owner);
}

IDatagramTransportSPtr DatagramTransportFactory::CreateTcpClient(wstring const & id, wstring const & owner)
{
    if (TransportConfig::GetConfig().InMemoryTransportEnabled)
    {
        return CreateMem(L"", id);
    }

    return TcpDatagramTransport::CreateClient(id, owner);
}

IDatagramTransportSPtr DatagramTransportFactory::CreateMem(wstring const & name, wstring const & id)
{
    wstring address;
    if (StringUtility::StartsWith(name, L"localhost"))
    {
        address = L"127.0.0.1" + name.substr(9);
    }
    else
    {
        address = name;
    }

    return make_shared<MemoryTransport>(address, id.empty() ? address : id);
}

IDatagramTransportSPtr DatagramTransportFactory::CreateUnreliable(
    ComponentRoot const & root,
    IDatagramTransportSPtr const & innerTransport)
{
    return make_shared<UnreliableTransport>(root, innerTransport);
}
