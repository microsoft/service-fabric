// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

NamedPipeClient::NamedPipeClient(ClientEventSource const & eventSource, int id)
    : pipe_(NamedPipeClient::CreatePipe(eventSource, id)),
    eventSource_(eventSource)
{
}

NamedPipeClient::~NamedPipeClient()
{
    CloseHandle(pipe_);
    eventSource_.PipeClientClosed();
}

void NamedPipeClient::SendBytes(void const * buffer, DWORD bufferSize)
{
    DWORD bytesWritten;
    BOOL success = WriteFile(pipe_, buffer, bufferSize, &bytesWritten, nullptr);
    if (!success || bytesWritten != bufferSize)
    {
        eventSource_.PipeSendFailed(GetLastError());
        throw Error::LastError("Failed to write to the pipe.");
    }

    eventSource_.PipeSent(bytesWritten);
}

HANDLE NamedPipeClient::CreatePipe(ClientEventSource const & eventSource, int id)
{
    wstring pipeName = NamedPipeServer::GetPipeName(id);

    HANDLE pipe = INVALID_HANDLE_VALUE;
    DWORD retriesRemaining = 50;

    while(retriesRemaining > 0)
    {
        retriesRemaining --;

        pipe = CreateFile( 
            pipeName.c_str(), // pipe name 
            GENERIC_WRITE,    // write access
            0,                // no sharing
            nullptr,          // default security attributes
            OPEN_EXISTING,    // pipe already created by server
            0,                // default attributes
            nullptr);         // no template file   
		
        if(pipe != INVALID_HANDLE_VALUE)
        {
            break;
        }

        if ((GetLastError() != ERROR_PIPE_BUSY) || (retriesRemaining == 0))
        {
            eventSource.PipeClientConnectFailed(GetLastError());
            throw Error::LastError("Failed to connect to the pipe.");
        }

        Sleep(500);
    }

    eventSource.PipeClientConnected(id);
    return pipe;
}
