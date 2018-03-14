// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class DatagramTransportFactory
    {
    public:
        static IDatagramTransportSPtr CreateTcp(
            std::wstring const & address,
            std::wstring const & id = L"",
            std::wstring const & owner = L"");

        static IDatagramTransportSPtr CreateTcpClient(
            std::wstring const & id = L"",
            std::wstring const & owner = L"");

        // Returns an empty pointer if the local address already exists
        static IDatagramTransportSPtr  CreateMem(
            std::wstring const & name,
            std::wstring const & id = L"");

        static IDatagramTransportSPtr CreateUnreliable(
            Common::ComponentRoot const & root,
            IDatagramTransportSPtr const & innerTransport);
    };
}
