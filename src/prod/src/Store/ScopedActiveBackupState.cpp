// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    namespace StoreBackupRequester
    {
         void WriteToTextWriter(__in TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case User: w << "User"; return;
            case LogTruncation: w << "LogTruncation"; return;
            case System: w << "System"; return;
            default: w << "StoreBackupRequester(" << static_cast<ULONG>(e) << ')'; return;
            }
        }

         bool IsValid(Enum const & e)
         {
             return (e == User || e == LogTruncation || e == System);
         }
    }

    ScopedActiveBackupState::ScopedActiveBackupState(
        __in StoreBackupState & activeBackupState, 
        LONG desiredBackupState)
        : activeBackupState_(activeBackupState)
    {
        hasAcquired_ = activeBackupState_.TryAcquire(desiredBackupState, backupStateIfNotAcquired_);
    }

    ScopedActiveBackupState::~ScopedActiveBackupState()
    {
        if (hasAcquired_)
        {
            activeBackupState_.Release();
        }
    }

    bool ScopedActiveBackupState::TryAcquire(
        __in StoreBackupState & activeBackupState,
        __in StoreBackupRequester::Enum const & backupRequester,
        __out ScopedActiveBackupStateSPtr & scopedBackupState)
    {
        ASSERT_IFNOT(StoreBackupRequester::IsValid(backupRequester), "ScopedActiveBackupState::TryAcquire(): Invalid value for backupRequester=[{0}]", backupRequester);
        
        LONG desiredState = -1;

        switch (backupRequester)
        {
        case StoreBackupRequester::User: 
            desiredState = StoreBackupState::ActiveByUser; break;

        case StoreBackupRequester::LogTruncation: 
            desiredState = StoreBackupState::ActiveByLogTruncation; break;

        case StoreBackupRequester::System:
            desiredState = StoreBackupState::ActiveBySystem; break;            
        }

        scopedBackupState = ScopedActiveBackupStateSPtr(new ScopedActiveBackupState(activeBackupState, desiredState));
        return scopedBackupState->hasAcquired_;
    }
}
