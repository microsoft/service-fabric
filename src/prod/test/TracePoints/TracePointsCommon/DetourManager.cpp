// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TracePoints;

DetourManager::DetourManager(bool restore)
    : restored_(DetourManager::Restore(restore)),
    transaction_()
{
}

DetourManager::~DetourManager()
{
    transaction_.Commit();
}

void DetourManager::Attach(__in void ** oldMethod, __in void * newMethod)
{
    LONG error = DetourAttach(oldMethod, newMethod);
    if (error != NO_ERROR)
    {
        throw Error::WindowsError(error, "Failed to attach a detour to the target function.");
    }
}

void DetourManager::Detach(__in void ** oldMethod, __in void * newMethod)
{
    LONG error = DetourDetach(oldMethod, newMethod);
    if (error != NO_ERROR)
    {
        throw Error::WindowsError(error, "Failed to detach a detour from the target function.");
    }
}

bool DetourManager::Restore(bool restore)
{
    bool restored = false;
    if (restore)
    {
        BOOL succeeded = DetourRestoreAfterWith();
        if (!succeeded)
        {
            throw Error::LastError("Failed to restore the memory import table.");
        }

        restored = true;
    }

    return restored;
}
