// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

CriticalSection EventSourceCriticalSection;
unique_ptr<ClientEventSource> EventSource;

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(hinst);
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(reserved);

    return TRUE;
}

DLLEXPORT_(DWORD) TracePointsRemoveEventMap(ProviderMapHandle h, ProviderId providerId)
{
    DWORD result;
    if (h)
    {
        bool succeeded = static_cast<ProviderMap *>(h)->RemoveEventMap(providerId);
        result = succeeded ? ERROR_SUCCESS : ERROR_NOT_FOUND;
    }
    else
    {
        result = ERROR_INVALID_HANDLE;
    }

    return result;
}

DLLEXPORT_(DWORD) TracePointsAddEventMapEntry(ProviderMapHandle h, ProviderId providerId, EventId eventId, TracePointFunctionPtr action, void * context)
{    
    DWORD result = ERROR_SUCCESS;
    if (h)
    {
        TracePointData tracePointData;
        tracePointData.action = action;
        tracePointData.context = context;
        
        static_cast<ProviderMap *>(h)->AddEventMapEntry(providerId, eventId, tracePointData);
    }
    else
    {
        result = ERROR_INVALID_HANDLE;
    }

    return result;
}

DLLEXPORT_(DWORD) TracePointsRemoveEventMapEntry(ProviderMapHandle h, ProviderId providerId, EventId eventId)
{
    DWORD result;
    if (h)
    {
        bool succeeded = static_cast<ProviderMap *>(h)->RemoveEventMapEntry(providerId, eventId);
        result = succeeded ? ERROR_SUCCESS : ERROR_NOT_FOUND;
    }
    else
    {
        result = ERROR_INVALID_HANDLE;
    }

    return result;
}

DLLEXPORT_(DWORD) TracePointsClearEventMap(ProviderMapHandle h, ProviderId providerId)
{
    DWORD result = ERROR_SUCCESS;
    if (h)
    {
        static_cast<ProviderMap *>(h)->ClearEventMap(providerId);
    }
    else
    {
        result = ERROR_INVALID_HANDLE;
    }

    return result;
}

DLLEXPORT_(DWORD) TracePointsSetup(DWORD processId, LPCWSTR dllName, LPCSTR setupFunctionName, LPCWSTR userData)
{
    return TracePointsRemoteCall(processId, dllName, setupFunctionName, userData, TracePointCommand::SetupOpCode);
}

DLLEXPORT_(DWORD) TracePointsInvoke(DWORD processId, LPCWSTR dllName, LPCSTR invokeFunctionName, LPCWSTR userData)
{
    return TracePointsRemoteCall(processId, dllName, invokeFunctionName, userData, TracePointCommand::InvokeOpCode);
}

DLLEXPORT_(DWORD) TracePointsCleanup(DWORD processId, LPCWSTR dllName, LPCSTR cleanUpFunctionName, LPCWSTR userData)
{
    return TracePointsRemoteCall(processId, dllName, cleanUpFunctionName, userData, TracePointCommand::CleanupOpCode);
}

DLLEXPORT_(EventProviderHandle) TracePointsGetEventProvider(ProviderMapHandle h)
{
    EventProviderHandle eph = nullptr;
    if (h)
    {
        ServerEventSource & eventSource = static_cast<ProviderMap *>(h)->get_EventSource();
        eph = &eventSource;
    }

    return eph;
}

DLLEXPORT_(DWORD) TracePointsEventWriteString(EventProviderHandle h, UCHAR Level, ULONGLONG Keyword, PCWSTR String)
{
    DWORD error = ERROR_INVALID_HANDLE;
    if (h)
    {
        ServerEventSource * eventSource = static_cast<ServerEventSource *>(h);
        error = eventSource->UserWriteString(Level, Keyword, String);
    }

    return error;
}

DWORD TracePointsRemoteCall(DWORD processId, LPCWSTR dllName, LPCSTR functionName, LPCWSTR userData, BYTE opCode)
{
    DWORD error;
    TracePointCommand::Data command;
    ClientEventSource const & eventSource = GetEventSource();
    bool succeeded = TracePointCommand::TryCreate(dllName, functionName, userData, opCode, command);
    if (succeeded)
    {
        switch (command.OpCode)
        {
        case TracePointCommand::SetupOpCode:
            eventSource.SendingSetupCommand(command);
            break;
        case TracePointCommand::InvokeOpCode:
            eventSource.SendingInvokeCommand(command);
            break;
        case TracePointCommand::CleanupOpCode:
            eventSource.SendingCleanupCommand(command);
            break;
        }

        try
        {
            NamedPipeClient client(eventSource, processId);
            client.Send<TracePointCommand::Data>(command);
            error = ERROR_SUCCESS;
        }
        catch (runtime_error & e)
        {
            UNREFERENCED_PARAMETER(e);
            error = GetLastError();
            eventSource.SendCommandFailed(error);
        }
    }
    else
    {
        error = GetLastError();
        eventSource.CreateCommandFailed(error);
    }

    return error;
}

ClientEventSource const & GetEventSource()
{
    {
        AcquireCriticalSection grab(EventSourceCriticalSection);
        if (!EventSource)
        {
            EventSource = UniquePtr::Create<ClientEventSource>();
        }
    }

    return *EventSource;
}
