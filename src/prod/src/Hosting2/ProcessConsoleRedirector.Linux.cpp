// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <fcntl.h>

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceType("ProcessConsoleRedirector");
StringLiteral const ThreadpoolWaitTraceType("ProcessConsoleRedirector.ThreadpoolWaitImpl");

ProcessConsoleRedirector::ProcessConsoleRedirector(
    int & pipeHandle,
    string const & redirectedFileLocation,
    string const & redirectedFileNamePrefix,
    bool isOutFile,
    int maxRetentionCount,
    LONG maxFileSizeInKb):
    MaxFileRetentionCount(maxRetentionCount),
    MaxFileSize(maxFileSizeInKb * 1024),    
    pipeHandle_(pipeHandle),
    redirectedFileLocation_(redirectedFileLocation),
    redirectedFileNamePrefix_(redirectedFileNamePrefix),
    isOutFile_(isOutFile),
    fileName_(),
    redirectedFile_(),
    bytesRead_(0),
    totalBytesWritten_(0)
{
    if(redirectedFileNamePrefix_.length() == 0)
    {
        isDisabled_ = true;
        return;
    }

    string fname = formatString("{0}_{1}.{2}.{3}",
                          redirectedFileNamePrefix_,
                          L"0",
                          Guid::NewGuid().ToString(),
                          isOutFile_ ? L"out" : L"err"
                          );
    fileName_ = redirectedFileLocation_;
    if (fileName_.back() != '/') {
        fileName_.push_back('/');
    }
    fileName_.append(fname);

    redirectedFile_ = open(fileName_.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    Threadpool::Post(
            [this]() -> void
            {
                int bytesRead = 0;
                do {
                    bytesRead = read(this->pipeHandle_, this->buffer_, this->BufferSize);
                    if (bytesRead > 0)
                        write(this->redirectedFile_, this->buffer_, bytesRead);
                    else if (bytesRead == 0)
                        break;
                    else if (bytesRead == -1)
                        if(errno == EINTR)
                            continue;
                        else
                            break;
                }while(true);
                close(this->redirectedFile_);
            });
}

ProcessConsoleRedirector::~ProcessConsoleRedirector()
{
}

ErrorCode ProcessConsoleRedirector::CreateOutputRedirector(
    std::string const & redirectedFileLocation,
    std::string const & redirectedFileNamePrefix,
    int maxRetentionCount,
    LONG maxFileSizeInKb,
    int & stdoutPipeHandle,
    __out ProcessConsoleRedirectorSPtr & stdoutProcessRedirector)    
{
    return Create(
        redirectedFileLocation,
        redirectedFileNamePrefix,
        true,
        maxRetentionCount,
        maxFileSizeInKb,
        stdoutPipeHandle,
        stdoutProcessRedirector);
}

ErrorCode ProcessConsoleRedirector::CreateErrorRedirector(
    std::string const & redirectedFileLocation,
    std::string const & redirectedFileNamePrefix,
    int maxRetentionCount,
    LONG maxFileSizeInKb,
    int & stderrPipeHandle,
    __out ProcessConsoleRedirectorSPtr & stderrProcessRedirector)
    
{
    return Create(
        redirectedFileLocation,
        redirectedFileNamePrefix,
        false,
        maxRetentionCount,
        maxFileSizeInKb,
        stderrPipeHandle,
        stderrProcessRedirector);
}

ErrorCode ProcessConsoleRedirector::Create(
    std::string const & redirectedFileLocation,
    std::string const & redirectedFileNamePrefix,
    bool isOutFile, 
    int maxRetentionCount,
    LONG maxFileSizeInKb,
    int & pipeHandle,
    __out ProcessConsoleRedirectorSPtr & processRedirector)
{
    processRedirector = make_shared<ProcessConsoleRedirector>(
            pipeHandle,
            redirectedFileLocation,
            redirectedFileNamePrefix,
            isOutFile,
            maxRetentionCount,
            maxFileSizeInKb);

    return ErrorCode::Success();
}

Common::ErrorCode ProcessConsoleRedirector::OnOpen()
{
    return ErrorCode::Success();
}

Common::ErrorCode ProcessConsoleRedirector::OnClose()
{
    return ErrorCode::Success();
}

void ProcessConsoleRedirector::OnAbort()
{
}
