// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class TracePointServer;
    typedef std::unique_ptr<TracePointServer> TracePointServerUPtr;

    class TracePointServer : private DenyCopy
    {
    public:
        TracePointServer(ProviderMap & providerMap, ServerEventSource const & eventSource);
        ~TracePointServer();

        void Start();
        void Stop();

    private:
        typedef std::unordered_map<std::wstring, TracePointModuleUPtr> ModuleMap;

        HANDLE thread_;
        std::unique_ptr<NamedPipeServer> server_;
        ModuleMap modules_;
        ProviderMap & providerMap_;
        ServerEventSource const & eventSource_;

        static DWORD WINAPI ThreadStart(__in void * state);
        void OnStart();
        void SetupModule(TracePointCommand::Data const & command);
        void InvokeOrCleanupModule(TracePointCommand::Data const & command, bool cleanup);
    };
}
