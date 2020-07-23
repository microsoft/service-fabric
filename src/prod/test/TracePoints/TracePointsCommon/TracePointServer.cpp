// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

TracePointServer::TracePointServer(ProviderMap & providerMap, ServerEventSource const & eventSource)
    : thread_(INVALID_HANDLE_VALUE),
    server_(nullptr),
    modules_(),
    providerMap_(providerMap),
    eventSource_(eventSource)
{
}

TracePointServer::~TracePointServer()
{
    if (thread_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(thread_);
    }
}

void TracePointServer::Start()
{
    DWORD processId = GetCurrentProcessId();
    server_ = UniquePtr::Create<NamedPipeServer>(eventSource_, processId);

    thread_ = CreateThread(nullptr, 0, &TracePointServer::ThreadStart, this, 0, nullptr);
    if (thread_ == INVALID_HANDLE_VALUE)
    {
        server_->Close();
        server_ = nullptr;
        eventSource_.ServerStartFailed(GetLastError());
        throw Error::LastError("Failed to create server thread.");
    }

    eventSource_.ServerStarted();
}

void TracePointServer::Stop()
{
    if (!server_)
    {
        throw Error::WindowsError(ERROR_INVALID_OPERATION, "Server has not been started.");
    }

    server_->Close();

    DWORD error = WaitForSingleObject(thread_, INFINITE);
    if (error != ERROR_SUCCESS)
    {
        eventSource_.ServerWaitForThreadFailed(error);
        throw Error::WindowsError(error, "Failed to wait for server thread completion.");
    }

    eventSource_.ServerStopped();
}

DWORD WINAPI TracePointServer::ThreadStart(__in void * state)
{
    TracePointServer * tracePointServer = static_cast<TracePointServer *>(state);
    tracePointServer->OnStart();
    return ERROR_SUCCESS;
}

void TracePointServer::OnStart()
{
    bool connected;
    do
    {
        connected = server_->Connect();
        if (connected)
        {
            TracePointCommand::Data command;
            while (server_->Receive<TracePointCommand::Data>(command))
            {
                switch (command.OpCode)
                {
                case TracePointCommand::SetupOpCode:
                    eventSource_.ReceivedSetupCommand(command);
                    SetupModule(command);
                    break;
                case TracePointCommand::InvokeOpCode:
                    eventSource_.ReceivedInvokeCommand(command);
                    InvokeOrCleanupModule(command, false);
                    break;
                case TracePointCommand::CleanupOpCode:
                    eventSource_.ReceivedCleanupCommand(command);
                    InvokeOrCleanupModule(command, true);
                    break;
                default:
                    eventSource_.ReceivedUnknownCommand(command);
                    throw Error::WindowsError(ERROR_INVALID_DATA, "The command is invalid.");
                    break;
                }
            }

            server_->Disconnect();
        }
    }
    while (connected);
}

void TracePointServer::SetupModule(TracePointCommand::Data const & command)
{
    wstring name(command.DllName);
    ModuleMap::iterator it = modules_.find(name);
    if (it == modules_.end())
    {
        TracePointModuleUPtr module(UniquePtr::Create<TracePointModule>(name));
        module->Invoke(command.FunctionName, providerMap_, command.UserData);
        modules_[name] = move(module);
    }
    else
    {
        throw Error::WindowsError(ERROR_INVALID_LIBRARY, "The module is already loaded.");
    }
}

void TracePointServer::InvokeOrCleanupModule(TracePointCommand::Data const & command, bool cleanup)
{
    wstring name(command.DllName);
    ModuleMap::iterator it = modules_.find(name);
    if (it != modules_.end())
    {
        it->second->Invoke(command.FunctionName, providerMap_, command.UserData);
        if (cleanup)
        {
            modules_.erase(it);
        }
    }
    else
    {
        throw Error::WindowsError(ERROR_INVALID_LIBRARY, "The module has not been loaded.");
    }
}
