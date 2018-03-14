// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport 
{
    class LTBufferFactory : public IBufferFactory
    {
    public:
        ~LTBufferFactory() override;
        std::unique_ptr<SendBuffer> CreateSendBuffer(TcpConnection* connection) override; 
        std::unique_ptr<ReceiveBuffer> CreateReceiveBuffer(TcpConnection* connection) override; 
    };
}
