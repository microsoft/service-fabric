// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

namespace TraceLevel
{
    enum Enum : UCHAR
    {
        Error = 2,
        Informational = 4,
    };
}

namespace TraceCode
{
    enum Enum : USHORT
    {
        PipeCreated = 1,
        PipeCreateFailed = 2,
        PipeConnected = 3,
        PipeConnectFailed = 4,
        PipeReceived = 5,
        PipeReceiveFailed = 6,
        PipeEnded = 7,
        PipeDisconnected = 8,
        PipeDisconnectFailed = 9,
        PipeClosed = 10,
        ServerStarted = 11,
        ServerStartFailed = 12,
        ServerStopped = 13,
        ServerWaitForThreadFailed = 14,
        ReceivedSetupCommand = 15,
        ReceivedInvokeCommand = 16,
        ReceivedCleanupCommand = 17,
        ReceivedUnknownCommand = 18,
        EventProviderRegistered = 51,
        EventProviderUnregistered = 52,
        InvokingUserFunction = 61,        
    };
}

static vector<EVENT_DATA_DESCRIPTOR> EmptyData;

ServerEventSource::ServerEventSource(REGHANDLE regHandle, EventWriteFunc eventWrite)
    : regHandle_(regHandle),
    eventWrite_(eventWrite)
{
}

ServerEventSource::~ServerEventSource()
{
}

void ServerEventSource::PipeCreated(int id) const
{
    WriteEvent(TraceCode::PipeCreated, TraceLevel::Informational, id);
}

void ServerEventSource::PipeCreateFailed(DWORD error) const
{
    WriteEvent(TraceCode::PipeCreateFailed, TraceLevel::Error, error);
}

void ServerEventSource::PipeConnected() const
{
    WriteEvent(TraceCode::PipeConnected, TraceLevel::Informational);
}

void ServerEventSource::PipeConnectFailed(DWORD error) const
{
    WriteEvent(TraceCode::PipeConnectFailed, TraceLevel::Error, error);
}

void ServerEventSource::PipeReceived(int bytesRead) const
{
    WriteEvent(TraceCode::PipeReceived, TraceLevel::Informational, bytesRead);
}

void ServerEventSource::PipeReceiveFailed(DWORD error) const
{
    WriteEvent(TraceCode::PipeReceiveFailed, TraceLevel::Error, error);
}

void ServerEventSource::PipeEnded(DWORD error) const
{
    WriteEvent(TraceCode::PipeEnded, TraceLevel::Informational, error);
}

void ServerEventSource::PipeDisconnected() const
{
    WriteEvent(TraceCode::PipeDisconnected, TraceLevel::Informational);
}

void ServerEventSource::PipeDisconnectFailed(DWORD error) const
{
    WriteEvent(TraceCode::PipeDisconnectFailed, TraceLevel::Error, error);
}

void ServerEventSource::PipeClosed() const
{
    WriteEvent(TraceCode::PipeClosed, TraceLevel::Informational);
}

void ServerEventSource::ServerStarted() const
{
    WriteEvent(TraceCode::ServerStarted, TraceLevel::Informational);
}

void ServerEventSource::ServerStartFailed(DWORD error) const
{
    WriteEvent(TraceCode::ServerStartFailed, TraceLevel::Error, error);
}

void ServerEventSource::ServerStopped() const
{
    WriteEvent(TraceCode::ServerStopped, TraceLevel::Informational);
}

void ServerEventSource::ServerWaitForThreadFailed(DWORD error) const
{
    WriteEvent(TraceCode::ServerWaitForThreadFailed, TraceLevel::Error, error);
}

void ServerEventSource::ReceivedSetupCommand(TracePointCommand::Data const & command) const
{
    WriteCommandEvent(TraceCode::ReceivedSetupCommand, TraceLevel::Informational, command);
}

void ServerEventSource::ReceivedInvokeCommand(TracePointCommand::Data const & command) const
{
    WriteCommandEvent(TraceCode::ReceivedInvokeCommand, TraceLevel::Informational, command);
}

void ServerEventSource::ReceivedCleanupCommand(TracePointCommand::Data const & command) const
{
    WriteCommandEvent(TraceCode::ReceivedCleanupCommand, TraceLevel::Informational, command);
}

void ServerEventSource::ReceivedUnknownCommand(TracePointCommand::Data const & command) const
{
    WriteCommandEvent(TraceCode::ReceivedUnknownCommand, TraceLevel::Error, command);
}

void ServerEventSource::EventProviderRegistered(LPCGUID providerId, PREGHANDLE regHandle) const
{
    WriteEvent(TraceCode::EventProviderRegistered, TraceLevel::Informational, *providerId, *regHandle);
}

void ServerEventSource::EventProviderUnregistered(REGHANDLE regHandle) const
{
    WriteEvent(TraceCode::EventProviderUnregistered, TraceLevel::Informational, regHandle);
}

void ServerEventSource::InvokingUserFunction(LPCGUID providerId, USHORT eventId) const
{
    WriteEvent(TraceCode::InvokingUserFunction, TraceLevel::Informational, *providerId, eventId);
}

DWORD ServerEventSource::UserWriteString(UCHAR Level, ULONGLONG Keyword, PCWSTR String) const
{
    return EventWriteString(regHandle_, Level, Keyword, String);
}

void ServerEventSource::WriteEvent(USHORT id, UCHAR level) const
{
    WriteEvent(id, level, EmptyData);
}

void ServerEventSource::WriteEvent(USHORT id, UCHAR level, vector<EVENT_DATA_DESCRIPTOR> & userData) const
{
    EVENT_DESCRIPTOR descriptor;
    EventDescCreate(&descriptor, id, 0, 0, level, 0, 0, 0);
    eventWrite_(regHandle_, &descriptor, static_cast<ULONG>(userData.size()), userData.data());
}

void ServerEventSource::WriteCommandEvent(USHORT id, UCHAR level, TracePointCommand::Data const & command) const
{
    EVENT_DESCRIPTOR descriptor;
    EventDescCreate(&descriptor, id, 0, 0, level, 0, 0, 0);
    if (EventEnabled(regHandle_, &descriptor))
    {
        vector<EVENT_DATA_DESCRIPTOR> userData;
        userData.reserve(3);
        EVENT_DATA_DESCRIPTOR dllNameItem;
        EventDataDescCreate(&dllNameItem, command.DllName, static_cast<ULONG>(sizeof(WCHAR) * (1 + wcslen(command.DllName))));
        userData.push_back(dllNameItem);
        EVENT_DATA_DESCRIPTOR functionNameItem;
        EventDataDescCreate(&functionNameItem, command.FunctionName, static_cast<ULONG>(1 + strlen(command.FunctionName)));
        userData.push_back(functionNameItem);
        EVENT_DATA_DESCRIPTOR userDataItem;
        EventDataDescCreate(&userDataItem, command.UserData, static_cast<ULONG>(sizeof(WCHAR) * (1 + wcslen(command.UserData))));
        userData.push_back(userDataItem);
        WriteEvent(id, level, userData);
    }
}
