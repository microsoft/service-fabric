// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

RunBackupCopierExeAsyncOperation::RunBackupCopierExeAsyncOperation(
    __in BackupCopierProxy & owner,
    Common::ActivityId const & activityId,
    wstring const & operation,
    wstring const & args,
    wstring const & argsLogString,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : BackupCopierAsyncOperationBase(
        owner,
        activityId,
        timeout,
        callback,
        parent)
    , operation_(operation)
    , args_(args)
    , argsLogString_(argsLogString)
    , tempWorkingDirectory_()
    , errorDetailsFile_()
    , hProcess_(0)
    , hThread_(0)
{
}

ErrorCode RunBackupCopierExeAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<RunBackupCopierExeAsyncOperation>(operation)->Error;
}

void RunBackupCopierExeAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->DoRunBackupCopierExe(thisSPtr);
}

void RunBackupCopierExeAsyncOperation::OnCancel()
{
    if (hProcess_ != 0)
    {
        this->Owner.Abort(hProcess_);
    }
}

void RunBackupCopierExeAsyncOperation::OnProcessJob(AsyncOperationSPtr const &)
{
    // no-op
}


void RunBackupCopierExeAsyncOperation::DoRunBackupCopierExe(AsyncOperationSPtr const & thisSPtr)
{
    if (this->IsCancelRequested)
    {
        this->TryComplete(thisSPtr, ErrorCodeValue::BackupCopierAborted);
        return;
    }

    auto actualTimeout = this->Timeout;
    pid_t pid;
    auto error = this->Owner.TryStartBackupCopierProcess(
        operation_,
        args_,
        argsLogString_,
        actualTimeout, // inout
        tempWorkingDirectory_, // out
        errorDetailsFile_, // out
        hProcess_, // out
        hThread_, // out
        pid);

    if (error.IsSuccess())
    {
        processWait_ = ProcessWait::CreateAndStart(
            Handle(hProcess_, Handle::DUPLICATE()),
            pid,
            [this, thisSPtr](pid_t, ErrorCode const & waitResult, DWORD exitCode)
        {
            OnWaitOneComplete(thisSPtr, waitResult, exitCode);
        },
            actualTimeout);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void RunBackupCopierExeAsyncOperation::OnWaitOneComplete(
    AsyncOperationSPtr const & thisSPtr,
    ErrorCode const & waitResult,
    DWORD exitCode)
{
    auto error = this->Owner.FinishBackupCopierProcess(
        waitResult,
        exitCode,
        tempWorkingDirectory_,
        errorDetailsFile_,
        hProcess_,
        hThread_);

    this->TryComplete(thisSPtr, move(error));
}
