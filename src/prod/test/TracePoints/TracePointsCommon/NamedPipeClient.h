// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class NamedPipeClient : private DenyCopy
    {
    public:
        NamedPipeClient(ClientEventSource const & eventSource, int id);
        ~NamedPipeClient();

        template <typename TMessage>
        void Send(TMessage const & message)
        {
            SendBytes(&message, static_cast<DWORD>(sizeof(TMessage)));
        }

    private:
        HANDLE pipe_;
        ClientEventSource const & eventSource_;
        
        static HANDLE CreatePipe(ClientEventSource const & eventSource, int id);
        void SendBytes(void const * buffer, DWORD bufferSize);
    };
}
