// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    /*
        Provides a single entry point to terminate the current process
        
        For Fabric.exe this is wired in via FabricNode
        
        It allows for fabrictest to be able to also validate that product
        code tried to terminate the process
    */
    class ProcessTerminationService
    {
        DENY_COPY(ProcessTerminationService);

    public:
        typedef std::function<void(std::wstring const &)> ExitProcessHandler;

        ProcessTerminationService();
        ProcessTerminationService(ExitProcessHandler const & exitProcessOverride);

        /*
            Aborts the process using ProcessUtility::Abort exit code
            Does not generate a dump
        */
        void ExitProcess(std::wstring const & reason);

        static std::shared_ptr<ProcessTerminationService> Create();

    private:
        ExitProcessHandler exitProcessOverride_;
    };

    typedef std::shared_ptr<ProcessTerminationService> ProcessTerminationServiceSPtr;
    
    typedef std::function<ProcessTerminationServiceSPtr()> ProcessTerminationServiceFactory;
}
