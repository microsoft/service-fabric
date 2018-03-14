// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ProcessTerminationService::ProcessTerminationService()
{
}

ProcessTerminationService::ProcessTerminationService(ExitProcessHandler const & overrideFunc)
    : exitProcessOverride_(overrideFunc)
{
}

shared_ptr<ProcessTerminationService> ProcessTerminationService::Create()
{
    return make_shared<ProcessTerminationService>();
}

void ProcessTerminationService::ExitProcess(std::wstring const & reason)
{
    if (exitProcessOverride_ != nullptr)
    {
        exitProcessOverride_(reason);
    }
    else
    {
        GeneralEventSource eventSource;
        eventSource.ExitProcess(reason);
        ::ExitProcess(ErrorCodeValue::ProcessAborted & 0x0000FFFF);
    }
}
