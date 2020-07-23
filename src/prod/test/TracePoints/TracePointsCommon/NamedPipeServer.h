// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class NamedPipeServer : private DenyCopy
    {
    public:
        NamedPipeServer(ServerEventSource const & eventSource, int id);
        ~NamedPipeServer();

        static std::wstring GetPipeName(int id);
        bool Connect();
        void Disconnect();
        void Close();

        template <typename TMessage>
        bool Receive(TMessage & message)
        {
            return ReceiveBytes(&message, sizeof(TMessage));
        }

    private:
        static size_t const BufferSize;
        HANDLE pipe_;
        ServerEventSource const & eventSource_;
        
        static HANDLE CreatePipe(ServerEventSource const & eventSource, int id);
        bool ReceiveBytes(__inout void * buffer, size_t bufferSize);
    };
}
