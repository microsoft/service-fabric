// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

//
// Factory class interface that creates send/receiver buffer manager class
//

#pragma once

namespace Transport
{
    class IBufferFactory
    {
    public:
        virtual ~IBufferFactory() = default;

        virtual std::unique_ptr<SendBuffer> CreateSendBuffer(TcpConnection* connection) = 0;
        virtual std::unique_ptr<ReceiveBuffer> CreateReceiveBuffer(TcpConnection* connection) = 0;
    };
}
