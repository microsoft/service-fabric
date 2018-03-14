// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

DesktopHandle::DesktopHandle(HDESK hValue)
    : HandleBase(hValue)
{
}

DesktopHandle::~DesktopHandle()
{
    Cleanup(Value);
}

DesktopHandleSPtr DesktopHandle::CreateSPtr(HDESK hValue)
{
    try
    {
        return std::make_shared<DesktopHandle>(hValue);
    }
    catch(...)
    {
        Cleanup(hValue);
        throw;
    }
}

DesktopHandleUPtr DesktopHandle::CreateUPtr(HDESK hValue)
{
    try
    {
        return make_unique<DesktopHandle>(hValue);
    }
    catch(...)
    {
        Cleanup(hValue);
        throw;
    }
}

#if !defined(PLATFORM_UNIX)
void DesktopHandle::Cleanup(HDESK hValue)
{
    Common::TraceNoise(
        Common::TraceTaskCodes::Common,
        "DesktopHandle",
        "Cleanup: CloseDesktop");

    if (!::CloseDesktop(hValue))
    {
        Common::TraceWarning(
            Common::TraceTaskCodes::Common,
            "DesktopHandle",
            "CloseDesktop failed: ErrorCode={0}",
            Common::ErrorCode::FromWin32Error());
    }
}
#endif
