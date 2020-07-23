// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

TracePointPipeClient::TracePointPipeClient(wstring const & pipeName, TracePointTracer const& tracer) 
    : pipeName_(pipeName), tracer_(tracer)
{
    DWORD mode = PIPE_READMODE_MESSAGE;
    DWORD retriesRemaining = 120;
    DWORD lastError = ERROR_SUCCESS;

    wstring log = L"TracePointPipeClient::TracePointPipeClient pipeName=";
    log.append(pipeName);
    tracer.Trace(pipeName, TRACE_LEVEL_INFORMATION);

    while (retriesRemaining > 0)
    {
        retriesRemaining --;
        pipe_ = CreateFile (pipeName.c_str(), GENERIC_WRITE|GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

        if (pipe_ != INVALID_HANDLE_VALUE)
        {
            tracer.Trace(L"TracePointPipeClient::TracePointPipeClient succeeded.", TRACE_LEVEL_CRITICAL);
            break;
        }

        lastError = GetLastError();
        if (lastError != ERROR_PIPE_BUSY) 
        {
            tracer.Trace(L"TracePointPipeClient::TracePointPipeClient failed and was not ERROR_PIPE_BUSY lastError=", lastError, (UCHAR) TRACE_LEVEL_CRITICAL);
            throw runtime_error("TracePointPipeClient::TracePointPipeClient CreateFile Failed.");
        }

        if(retriesRemaining == 0)
        {
            tracer.Trace(L"TracePointPipeClient::TracePointPipeClient ran out of retries.", (UCHAR)TRACE_LEVEL_CRITICAL);
            throw runtime_error("TracePointPipeClient::TracePointPipeClient timeout");
        }

        Sleep(200);
    }
    
    if (!SetNamedPipeHandleState(pipe_, &mode, nullptr, nullptr))
    {
        tracer.Trace(L"TracePointPipeClient::TracePointPipeClient unable to SetNamedPipeHandleState", (UCHAR)TRACE_LEVEL_CRITICAL);
        throw runtime_error("TracePointPipeClient::TracePointPipeClient SetNamedPipeHandleState unable to change mode.");
    }
}

void TracePointPipeClient::WriteString(wstring const & message)
{
        wchar_t buffer[512];
        DWORD bytesWritten = 0;
        DWORD bufferSize = 512 * sizeof(wchar_t);
        wstringstream messageInitializer(message.c_str());

        memset((void*) buffer, 0, bufferSize);
        messageInitializer >> buffer;

        BOOL success = WriteFile(pipe_, buffer, bufferSize, &bytesWritten, nullptr);
        if (!success || bytesWritten != bufferSize)
        {
           tracer_.Trace(L"TracePointPipeClient::WriteString WriteFile failed.", TRACE_LEVEL_CRITICAL);
           throw runtime_error("TracePointPipeClient::::WriteString WriteFile failed.");
        }
}

wstring TracePointPipeClient::ReadString()
{
    vector<WCHAR> buffer;
    DWORD bytesRead = 0;
    buffer.resize(512,0);
       
    if(!ReadFile( pipe_, buffer.data(), (DWORD) (sizeof(WCHAR) * buffer.size()), &bytesRead, nullptr))
    {
        tracer_.Trace(L"TracePointPipeClient::ReadString ReadFile failed.", TRACE_LEVEL_CRITICAL);
        throw runtime_error("TracePointPipeClient::::ReadString ReadFile failed.");
    }
        
    return wstring(buffer.data());
}

TracePointPipeClient::~TracePointPipeClient()
{
    if(INVALID_HANDLE_VALUE != pipe_)
    {
        tracer_.Trace(L"TracePointPipeClient::~TracePointPipeClient", TRACE_LEVEL_INFORMATION);
        CloseHandle(pipe_);
        pipe_ = INVALID_HANDLE_VALUE;
    }
}
