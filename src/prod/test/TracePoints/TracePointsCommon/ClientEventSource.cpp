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
        PipeClientConnected = 101,
        PipeClientConnectFailed = 102,
        PipeSent = 103,
        PipeSendFailed = 104,
        PipeClientClosed = 105,
        CreateCommandFailed = 106,
        SendingSetupCommand = 107,
        SendingInvokeCommand = 108,
        SendingCleanupCommand = 109,
        SendCommandFailed = 110,
    };
}

// {019DAA0F-E775-471A-AA85-49363C18E179}
GUID const ClientEventSource::TracePointsProviderId = { 0x19daa0f, 0xe775, 0x471a, { 0xaa, 0x85, 0x49, 0x36, 0x3c, 0x18, 0xe1, 0x79 } };

static vector<EVENT_DATA_DESCRIPTOR> EmptyData;

ClientEventSource::ClientEventSource()
    : regHandle_(RegisterProvider())
{
}

ClientEventSource::~ClientEventSource()
{
    EventUnregister(regHandle_);
}

void ClientEventSource::PipeClientConnected(int id) const
{
    WriteEvent(TraceCode::PipeClientConnected, TraceLevel::Informational, id);
}

void ClientEventSource::PipeClientConnectFailed(DWORD error) const
{
    WriteEvent(TraceCode::PipeClientConnectFailed, TraceLevel::Error, error);
}

void ClientEventSource::PipeSent(int bytesWritten) const
{
    WriteEvent(TraceCode::PipeSent, TraceLevel::Informational, bytesWritten);
}

void ClientEventSource::PipeSendFailed(DWORD error) const
{
    WriteEvent(TraceCode::PipeSendFailed, TraceLevel::Error, error);
}

void ClientEventSource::PipeClientClosed() const
{
    WriteEvent(TraceCode::PipeClientClosed, TraceLevel::Informational);
}

void ClientEventSource::CreateCommandFailed(DWORD error) const
{
    WriteEvent(TraceCode::CreateCommandFailed, TraceLevel::Error, error);
}

void ClientEventSource::SendingSetupCommand(TracePointCommand::Data const & command) const
{
    WriteCommandEvent(TraceCode::SendingSetupCommand, TraceLevel::Informational, command);
}

void ClientEventSource::SendingInvokeCommand(TracePointCommand::Data const & command) const
{
    WriteCommandEvent(TraceCode::SendingInvokeCommand, TraceLevel::Informational, command);
}

void ClientEventSource::SendingCleanupCommand(TracePointCommand::Data const & command) const
{
    WriteCommandEvent(TraceCode::SendingCleanupCommand, TraceLevel::Informational, command);
}

void ClientEventSource::SendCommandFailed(DWORD error) const
{
    WriteEvent(TraceCode::SendCommandFailed, TraceLevel::Error, error);
}

void ClientEventSource::WriteEvent(USHORT id, UCHAR level) const
{
    WriteEvent(id, level, EmptyData);
}

void ClientEventSource::WriteEvent(USHORT id, UCHAR level, vector<EVENT_DATA_DESCRIPTOR> & userData) const
{
    EVENT_DESCRIPTOR descriptor;
    EventDescCreate(&descriptor, id, 0, 0, level, 0, 0, 0);
    EventWrite(regHandle_, &descriptor, static_cast<ULONG>(userData.size()), userData.data());
}

void ClientEventSource::WriteCommandEvent(USHORT id, UCHAR level, TracePointCommand::Data const & command) const
{
    EVENT_DESCRIPTOR descriptor;
    EventDescCreate(&descriptor, id, 0, 0, level, 0, 0, 0);
    if (EventEnabled(regHandle_, &descriptor))
    {
        vector<EVENT_DATA_DESCRIPTOR> userData(3);
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

REGHANDLE ClientEventSource::RegisterProvider()
{
    REGHANDLE regHandle;
    ULONG error = EventRegister(&TracePointsProviderId, nullptr, nullptr, &regHandle);
    if (error != ERROR_SUCCESS)
    {
        throw Error::LastError("Failed to register the TracePoints trace provider.");
    }

    return regHandle;
}
