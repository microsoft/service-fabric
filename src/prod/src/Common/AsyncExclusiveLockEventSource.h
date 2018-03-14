// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AsyncExclusiveLockEventSource
    { 
    public:
        Common::TraceEventWriter<uint64> BeginLockAcquire;
        Common::TraceEventWriter<uint64> EndLockAcquireSuccess;
        Common::TraceEventWriter<uint64> EndLockAcquireTimeout;
        Common::TraceEventWriter<uint64> LockRelease;

        AsyncExclusiveLockEventSource()
          : BeginLockAcquire(Common::TraceTaskCodes::AsyncExclusiveLock, 4, "BeginLockAcquire", Common::LogLevel::Noise
          , "{0}: Begin acquire lock"
          , "lockOperationPtr")
          , EndLockAcquireSuccess(Common::TraceTaskCodes::AsyncExclusiveLock, 5, "EndLockAcquireSuccess", Common::LogLevel::Noise
          , "{0}: Lock acquired successfully"
          , "lockOperationPtr")
          , EndLockAcquireTimeout(Common::TraceTaskCodes::AsyncExclusiveLock, 6, "EndLockAcquireTimeout", Common::LogLevel::Noise
          , "{0}: Lock acquisition timed out"
          , "lockOperationPtr")
          , LockRelease(Common::TraceTaskCodes::AsyncExclusiveLock, 7, "LockRelease", Common::LogLevel::Noise
          , "{0}: Released lock"
          , "lockOperationPtr")
        {
        }  
    };
}
