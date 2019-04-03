// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TracePoints;

USHORT const DetourTransaction::Started = 0;
USHORT const DetourTransaction::Aborted = 1;
USHORT const DetourTransaction::Committed = 2;

DetourTransaction::DetourTransaction()
    : state_(DetourTransaction::Started)
{
    LONG error = DetourTransactionBegin();
    if (error != NO_ERROR)
    {
        throw Error::WindowsError(error, "Failed to begin the detour transaction.");
    }

    error = DetourUpdateThread(GetCurrentThread());
    if (error != NO_ERROR)
    {
        Abort();
        throw Error::WindowsError(error, "Failed to update the thread for the current transaction.");
    }
}

DetourTransaction::~DetourTransaction() noexcept(false)
{
    if (state_ == DetourTransaction::Started)
    {
        Abort();
        throw Error::WindowsError(ERROR_INVALID_OPERATION, "The transaction was abandoned.");
    }
}

void DetourTransaction::Abort()
{
    if (state_ != DetourTransaction::Started)
    {
        throw Error::WindowsError(ERROR_INVALID_STATE, "The transaction cannot be aborted.");
    }

    LONG error = DetourTransactionAbort();
    if (error != NO_ERROR)
    {
        throw Error::WindowsError(error, "Failed to abort the transaction.");
    }

    state_ = DetourTransaction::Aborted;
}

void DetourTransaction::Commit()
{
    if (state_ != DetourTransaction::Started)
    {
        throw Error::WindowsError(ERROR_INVALID_STATE, "The transaction cannot be committed.");
    }

    LONG error = DetourTransactionCommit();
    if (error != NO_ERROR)
    {
        throw Error::WindowsError(error, "Failed to commit the transaction.");
    }

    state_ = DetourTransaction::Committed;
}
