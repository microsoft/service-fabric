// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Hosting2;

DockerProcessTerminatedNotification::DockerProcessTerminatedNotification()
    : exitCode_(),
    continuousFailureCount_(),
    nextStartTime_()
{
}

DockerProcessTerminatedNotification::DockerProcessTerminatedNotification(
    DWORD exitCode,
    ULONG continuousFailureCount,
    TimeSpan nextStartTime)
    : exitCode_(exitCode),
    continuousFailureCount_(continuousFailureCount),
    nextStartTime_(nextStartTime)
{
}

void DockerProcessTerminatedNotification::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DockerProcessTerminatedNotification { ");
    w.Write("ExitCode = {0}", exitCode_);
    w.Write("ContinousFailureCount = {0}", continuousFailureCount_);
    w.Write("NextStartTime = {0}", nextStartTime_);
    w.Write("}");
}
