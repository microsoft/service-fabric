// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class GeneralEventSource
    { 
    public:
        TraceEventWriter<std::string, std::wstring> Assert;
        TraceEventWriter<std::wstring> ExitProcess;

        GeneralEventSource()
            : Assert(TraceTaskCodes::General, 4, "Assert", LogLevel::Error, "Throwing coding error - {0}\r\n{1}", "explanation", "stackTrace")
            , ExitProcess(TraceTaskCodes::General, 5, "ExitProcess", LogLevel::Error, "Process termination requested. Reason: {0}", "reason")
        {
        }  
    };
}
