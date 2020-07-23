// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

size_t const NamedPipeServer::BufferSize = sizeof(TracePointCommand::Data);

NamedPipeServer::NamedPipeServer(ServerEventSource const & eventSource, int id)
    : pipe_(NamedPipeServer::CreatePipe(eventSource, id)),
    eventSource_(eventSource)
{
}

NamedPipeServer::~NamedPipeServer()
{
    Close();
}

wstring NamedPipeServer::GetPipeName(int id)
{
    wostringstream pipeName;
    pipeName << L"\\\\.\\pipe\\TracePoints\\" << id;

    return pipeName.str();
}

bool NamedPipeServer::Connect()
{
    BOOL success = ConnectNamedPipe(pipe_, nullptr);

    bool connected = false;
    if (success)
    {
        eventSource_.PipeConnected();
        connected = true;
    }
    else
    {
        DWORD error = GetLastError();
        eventSource_.PipeConnectFailed(error);
        if (error != ERROR_BROKEN_PIPE && error != ERROR_NO_DATA)
        {
            throw Error::LastError("Failed to connect the named pipe.");
        }
    }

    return connected;
}

void NamedPipeServer::Disconnect()
{
    BOOL success = DisconnectNamedPipe(pipe_);
    if (!success)
    {
        eventSource_.PipeDisconnectFailed(GetLastError());
        throw Error::LastError("Failed to disconnect the named pipe.");
    }

    eventSource_.PipeDisconnected();
}

void NamedPipeServer::Close()
{
    if (pipe_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pipe_);
        pipe_ = INVALID_HANDLE_VALUE;
        eventSource_.PipeClosed();
    }
}

bool NamedPipeServer::ReceiveBytes(__inout void * buffer, size_t bufferSize)
{
    ZeroMemory(buffer, bufferSize);
    DWORD bytesRead;
    BOOL success = ReadFile(pipe_, buffer, static_cast<DWORD>(bufferSize), &bytesRead, nullptr);

    bool messageReceived = false;
    if (success && bytesRead == bufferSize)
    {
        eventSource_.PipeReceived(bytesRead);
        messageReceived = true;
    }
    else
    {
        DWORD error = GetLastError();
        if (error != ERROR_BROKEN_PIPE && error != ERROR_NO_DATA)
        {
            eventSource_.PipeReceiveFailed(error);
            throw Error::LastError("Failed to read from the pipe.");
        }
        else
        {
            eventSource_.PipeEnded(error);
        }
    }

    return messageReceived;
}

HANDLE NamedPipeServer::CreatePipe(ServerEventSource const & eventSource, int id)
{
    wstring pipeName = NamedPipeServer::GetPipeName(id);

    // blocking, message-oriented pipe
    DWORD pipeMode = PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT;
    DWORD bufferSize = static_cast<DWORD>(NamedPipeServer::BufferSize);

    HANDLE pipe = CreateNamedPipe(
        pipeName.c_str(),     // pipe name 
        PIPE_ACCESS_INBOUND,  // read access
        pipeMode,
        1,                    // allow only 1 client connection  
        bufferSize,           // output buffer size 
        bufferSize,           // input buffer size 
        0,                    // client time-out 
        nullptr);             // default security attribute 
    if (pipe == INVALID_HANDLE_VALUE)
    {
        eventSource.PipeCreateFailed(GetLastError());
        throw Error::LastError("Failed to create the pipe.");
    }

    eventSource.PipeCreated(id);
    return pipe;
}
