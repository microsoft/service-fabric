// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceType("ProcessConsoleRedirector");
StringLiteral const ThreadpoolWaitTraceType("ProcessConsoleRedirector.ThreadpoolWaitImpl");

class ProcessConsoleRedirector::ThreadpoolWaitImpl : 
    public std::enable_shared_from_this<ThreadpoolWaitImpl>,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ThreadpoolWaitImpl);

public:
    ThreadpoolWaitImpl(Handle && handle, ProcessConsoleRedirector::WaitCallback const & callback)
    : handle_(std::move(handle)), callback_(callback), wait_(nullptr), cancelCalled_(false), cancelFinished_(false), callbackInProgress_(false), cancelLock_()
    {
        CODING_ERROR_ASSERT(handle_.Value != nullptr);
        CODING_ERROR_ASSERT(callback_ != nullptr);

        wait_ = CreateThreadpoolWait(&Threadpool::WaitCallback<ProcessConsoleRedirector::ThreadpoolWaitImpl>, this, NULL /*no specific environment*/);
        CHK_WBOOL(wait_ != nullptr);
        SetThreadpoolWait(wait_, handle_.Value, NULL);
    }
   
    ~ThreadpoolWaitImpl()
    {
        Cancel();        
    }

    void ReRegister()
    {
        if (!cancelCalled_)
        {
            CHK_WBOOL(wait_ != nullptr);
            {
                AcquireReadLock lock(cancelLock_);
                if(!cancelCalled_)
                {
                    SetThreadpoolWait(wait_, handle_.Value, NULL);
                }
            }
        }
    }

    void Cancel()
    {
        WriteNoise(ThreadpoolWaitTraceType, "{0}: Cancel called", static_cast<void*>(this));

        if (cancelCalled_)
        {
            // cancelled has already been called.
            return;
        }
        {
            AcquireWriteLock lock(cancelLock_);
            if (cancelCalled_)
            {
                // cancelled was called before lock acquisition
                return;
            }
            cancelCalled_ = true;
        }
        CODING_ERROR_ASSERT(wait_ != nullptr);
        CODING_ERROR_ASSERT(callback_ != nullptr);

        // Explicitly calling 2 WaitForThreadpoolWaitCallbacks. 
        // The first one ensures that no callbacks are still running which have not observed the flag.  
        // The second one ensures that we cancel the ones that may have started while we were calling SetThreadpoolWait with null event handle.
        WaitForThreadpoolWaitCallbacks(wait_, TRUE /*cancel pending callbacks*/);
        SetThreadpoolWait(this->wait_, nullptr, nullptr); 
        WaitForThreadpoolWaitCallbacks(wait_, TRUE /*cancel pending callbacks*/);
        CloseThreadpoolWait(wait_);

        cancelFinished_.store(true);

        // If the callback is in progress, we cannot set the callback to null.
        // But as we are setting cancelFinished to true, the callback is make sure to cleanup
        if (!callbackInProgress_.load()) 
        {
            callback_ = nullptr;
            wait_ = nullptr;
        }
    }

    static void Callback(PTP_CALLBACK_INSTANCE pci, void* callbackParameter, PTP_WAIT, TP_WAIT_RESULT)
    {
        // Increment ref count to make sure ThreadpoolWaitImpl object is alive 
        shared_ptr<ThreadpoolWaitImpl> thisSPtr = reinterpret_cast<ThreadpoolWaitImpl*>(callbackParameter)->shared_from_this();
        if (thisSPtr->cancelCalled_) { return; }
        thisSPtr->callbackInProgress_.store(true);            
        
        // If cancel has not been called and we have the ref of ThreadpoolWait and parent, we can safely call the callback
        auto callback = thisSPtr->callback_;

        // The following call makes sure WaitForThreadpoolWaitCallbacks() won't wait for this callback to complete. This way,
        // Delete() won't block on callbacks, since it only needs to wait for the completion of minimal work above.
        DisassociateCurrentThreadFromCallback(pci);
        
        callback();

        thisSPtr->callbackInProgress_.store(false);

        // This condition indicates that the cancel has been finished by the callback was not set to null.
        // We avoid Cancel setting callback_ to null by setting callbackInProgress to true at the begining
        // of this function.
        if (thisSPtr->cancelFinished_.load())
        {
            thisSPtr->callback_ = nullptr;
            thisSPtr->wait_ = nullptr;
        }
    }

private:
    Handle handle_;
    ProcessConsoleRedirector::WaitCallback callback_;    
    PTP_WAIT wait_;
    bool cancelCalled_;
    Common::atomic_bool cancelFinished_;
    Common::atomic_bool callbackInProgress_;
    Common::RwLock cancelLock_;
};

ProcessConsoleRedirector::ProcessConsoleRedirector(
    HandleUPtr & pipeHandle, 
    wstring const & redirectedFileLocation,
    wstring const & redirectedFileNamePrefix,
    bool isOutFile,
    int maxRetentionCount,
    LONG maxFileSizeInKb):
    MaxFileRetentionCount(maxRetentionCount),
    MaxFileSize(maxFileSizeInKb * 1024),    
    pipeHandle_(move(pipeHandle)),
    redirectedFileLocation_(redirectedFileLocation),
    redirectedFileNamePrefix_(redirectedFileNamePrefix),
    isOutFile_(isOutFile),
    fileName_(),
    redirectedFile_(),
    pipeReadWaitHandle_(),
    pipeReadWait_(),
    fileWriteWaitHandle_(),
    fileWriteWait_(),
    readOverlapped_(),
    writeOverlapped_(),
    bytesRead_(0),
    totalBytesWritten_(0)
{   
    ::ZeroMemory(&buffer_, sizeof(buffer_));
    pipeReadWaitHandle_ = make_unique<Handle>(
        ::CreateEvent(nullptr, true, false, nullptr));

    fileWriteWaitHandle_ = make_unique<Handle>(
        ::CreateEvent(nullptr, true, false, nullptr));
}

ErrorCode ProcessConsoleRedirector::CreateOutputRedirector(
    std::wstring const & redirectedFileLocation, 
    std::wstring const & redirectedFileNamePrefix, 
    int maxRetentionCount,
    LONG maxFileSizeInKb,
    __out Common::HandleUPtr & stdoutPipeHandle, 
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
    std::wstring const & redirectedFileLocation, 
    std::wstring const & redirectedFileNamePrefix, 
    int maxRetentionCount,
    LONG maxFileSizeInKb,
    __out Common::HandleUPtr & stderrPipeHandle, 
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
    std::wstring const & redirectedFileLocation, 
    std::wstring const & redirectedFileNamePrefix, 
    bool isOutFile, 
    int maxRetentionCount,
    LONG maxFileSizeInKb,
    __out Common::HandleUPtr & pipeHandle, 
    __out ProcessConsoleRedirectorSPtr & processRedirector)
{
    SECURITY_ATTRIBUTES saAttr; 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = true; 
    saAttr.lpSecurityDescriptor = NULL; 
    
    // For scalemin, guid will provide uniqueness
    wstring pipeName = GetPipeName(redirectedFileNamePrefix, isOutFile);
    
    // Temporary handle that can be inherited
    // PIPE_WAIT:Wait mode..we will still do async. Operations will be in pending state until completed
    // Without this, the operation completes with error if the console has finished writing current data.
    HANDLE readPipeHandle = CreateNamedPipe( 
      pipeName.c_str(),             
      PIPE_ACCESS_INBOUND| FILE_FLAG_OVERLAPPED,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 
      1, 
      BufferSize * sizeof(char),    
      BufferSize * sizeof(char),    
      0,             
      &saAttr);
    if (readPipeHandle == INVALID_HANDLE_VALUE) { return ErrorCode::FromWin32Error(); }
    Handle pipeReadTemp(readPipeHandle);

    // Pipe write handle. Used by console
    HANDLE writePipeHandle = CreateFile(
         pipeName.c_str(),
         FILE_WRITE_DATA,
         0,
         &saAttr,
         OPEN_EXISTING, 
         FILE_ATTRIBUTE_NORMAL,
         0 );
    if (writePipeHandle == INVALID_HANDLE_VALUE) { return ErrorCode::FromWin32Error(); }
    pipeHandle =  make_unique<Handle>(writePipeHandle);    
    
    // Duplicate of the pipe handle. This one does not support inheritance
    HandleUPtr pipeRead = make_unique<Handle>(pipeReadTemp, Handle::DUPLICATE());
    if(pipeRead->Value == INVALID_HANDLE_VALUE) { return ErrorCode::FromWin32Error(); }

    processRedirector = make_shared<ProcessConsoleRedirector>(
        pipeRead, 
        redirectedFileLocation,
        redirectedFileNamePrefix, 
        isOutFile,
        maxRetentionCount,
        maxFileSizeInKb);

    return ErrorCode::Success();
}

wstring ProcessConsoleRedirector::GetPipeName(wstring const & redirectedFileNamePrefix, bool isOutFile)
{
    return wformatString(
        "\\\\.\\pipe\\{0}_{1}_{2}",
        redirectedFileNamePrefix,
        isOutFile ? L"out": L"err",
        Guid::NewGuid().ToString());
}

Common::ErrorCode ProcessConsoleRedirector::OnOpen()
{
    ProcessConsoleRedirectorSPtr thisSPtr = this->shared_from_this();
    pipeReadWait_ = make_shared<ThreadpoolWaitImpl>(
        move(Handle(pipeReadWaitHandle_->Value, Handle::DUPLICATE())), 
        [thisSPtr]() { thisSPtr->OnPipeReadComplete(); });

    fileWriteWait_ = make_shared<ThreadpoolWaitImpl>(
        move(Handle(fileWriteWaitHandle_->Value, Handle::DUPLICATE())), 
        [thisSPtr]() { thisSPtr->OnFileWriteComplete(); });

    return SetRedirectionFile(true);
}

Common::ErrorCode ProcessConsoleRedirector::OnClose()
{
    OnAbort();
    return ErrorCode::Success();
}

void ProcessConsoleRedirector::OnAbort()
{
    pipeReadWait_->Cancel();
    fileWriteWait_->Cancel();   
    if (deleteTimer_) { deleteTimer_->Cancel(); }
}

ErrorCode ProcessConsoleRedirector::ReadPipe()
{   
    ::ZeroMemory(&readOverlapped_, sizeof(OVERLAPPED));
    readOverlapped_.hEvent = pipeReadWaitHandle_->Value;
    
    if (!::ReadFile(
            pipeHandle_->Value,
            buffer_,
            sizeof(char) * BufferSize,
            NULL,
            &readOverlapped_))
    {
        DWORD errorCode = ::GetLastError();
        
        if (errorCode != ERROR_IO_PENDING)
        {
            WriteWarning(
                TraceType,
                "Read failed with {0}. Console log {1} will not be available.",
                ErrorCode::FromWin32Error(errorCode),
                fileName_);
            if (errorCode == ERROR_BROKEN_PIPE)
            {
                this->Abort();
            }
            return ErrorCode::FromWin32Error(errorCode);
        }
    }

    return ErrorCode::Success();
}

ErrorCode ProcessConsoleRedirector::WriteFile()
{
    if (redirectedFile_== nullptr)
    {
        //there is no redirectedFile set. Just readpipe again
        return ReadPipe();
    }

    ::ZeroMemory(&writeOverlapped_, sizeof(OVERLAPPED));
    writeOverlapped_.hEvent = fileWriteWaitHandle_->Value;

    // Write to the end of the file
    writeOverlapped_.Offset = 0xFFFFFFFF;
    writeOverlapped_.OffsetHigh = 0xFFFFFFFF;
    
    if (!::WriteFile(
            redirectedFile_->GetHandle(),
            buffer_,
            bytesRead_,
            nullptr,
            &writeOverlapped_))
    {
        DWORD errorCode = ::GetLastError();
        
        if (errorCode != ERROR_IO_PENDING)
        {
            WriteWarning(
                TraceType,
                "Write failed with {0}. Console log {1} will not be available.",
                ErrorCode::FromWin32Error(errorCode),
                fileName_);
            //We need to continue to read from pipe even when writefile fails otherwise the
            //other end of pipe writing to it will be stuck
            return ReadPipe();
        }
    }

    return ErrorCode::Success();
}

void ProcessConsoleRedirector::OnPipeReadComplete()
{
    if (!ReRegisterThreadpoolWait(pipeReadWaitHandle_, pipeReadWait_)) { return; }

    // We are passing FALSE for bWait. However, we know that the operation
    // was completed. Hence it should not really hanppen and GetOverlappedResult should always
    // return true
    if (!::GetOverlappedResult(
        pipeHandle_->Value,
        &readOverlapped_,
        &bytesRead_,
        FALSE))
    {
        DWORD errorCode = ::GetLastError();
        WriteWarning(
            TraceType,
            "GetOverlappedResult failed with {0}. Console log {1} will not be available.",
            ErrorCode::FromWin32Error(errorCode),
            fileName_);
        if (errorCode == ERROR_BROKEN_PIPE)
        {
            this->Abort();
        }
        return;
    }

    if ((totalBytesWritten_ + bytesRead_) >= MaxFileSize)
    {    
        WriteNoise(
                TraceType,
                "File {0} current size is {1} bytes and data read is {2} bytes. Max allowed size is {3}. Creating new file.",
                fileName_,
                totalBytesWritten_,
                bytesRead_,
                MaxFileSize);     

        totalBytesWritten_ = bytesRead_;
        
        // SetRedirectionFile will take care of calling WriteFile
        SetRedirectionFile(false);
    }
    else
    {
        totalBytesWritten_ += bytesRead_;
        WriteFile(); 
    }    
}

void ProcessConsoleRedirector::OnFileWriteComplete()
{
    // Reset the event
    if (!ReRegisterThreadpoolWait(fileWriteWaitHandle_, fileWriteWait_)) { return; }
    
    // Initiate read again. No need to check for error. ReadFile takes care of tracing it.    
    ReadPipe();    
}

// Re-Register should always be called from the callback for the cancellation to work correctly.
bool ProcessConsoleRedirector::ReRegisterThreadpoolWait(Common::HandleUPtr const & eventHandle, ThreadpoolWaitImpSPtr const & threadPoolWait)
{
    if (!::ResetEvent(eventHandle->Value))
    {
        WriteWarning(
            TraceType,
            "Event could not be reset. Console for file {0} will not be redirected. Error code : {1}",
            fileName_,
            ErrorCode::FromWin32Error());
        return false;
    }

    // Reregister the wait for the events
    threadPoolWait->ReRegister();
    return true;
}

ProcessConsoleRedirector::~ProcessConsoleRedirector()
{
    Abort();
    if (redirectedFile_) { redirectedFile_->Close2(); }
}

ErrorCode ProcessConsoleRedirector::SetRedirectionFile(bool isOpening)
{
    wstring fileFindPattern = redirectedFileNamePrefix_;
    fileFindPattern.append(L"*");
    fileFindPattern.append(isOutFile_ ? L".out" : L".err");

    map<wstring, WIN32_FIND_DATA> files = Directory::FindWithFileProperties(redirectedFileLocation_, fileFindPattern, MaxFileRetentionCount, false);

    // first file for redirection
    if (files.size() == 0) 
    {
        fileName_ = wformatString(
            "{0}_{1}.{2}",
            redirectedFileNamePrefix_,
            L"0",
            isOutFile_ ? L"out" : L"err");   
        fileName_ = Common::Path::Combine(redirectedFileLocation_, fileName_);
        return CreateFileForRedirection(isOpening);        
    }
    else
    {
        // Not owned by Common::File. Hence need to close explicitly
        // Close need to be happen before we delete. This is especially needed if the retention count is 1.
        if (redirectedFile_) { redirectedFile_->Close2(); }

        // Look up the files and find out if we can increment the file prefix and write to next file.
        // or if we have exceeded MaxFileRetentionCount and need to delete one file.
        int64 max = -1;    
        wstring firstAccessedFile = L"";        
        DateTime firstAccessedTime = DateTime::Now();

        for (auto file : files)
        {
            wstring fileNameTemp = file.first;        
            StringCollection splitString;
            StringUtility::Split<wstring>(fileNameTemp, splitString, L".");
            fileNameTemp = splitString[splitString.size() - 2];
            StringUtility::Split<wstring>(fileNameTemp, splitString, L"_");
            fileNameTemp = splitString[splitString.size() - 1];
            int64 fileNum = ::_wtoi64(fileNameTemp.c_str());
        
            if (fileNum > max)
            {
                max = fileNum;
            }

            DateTime updatedTime(file.second.ftCreationTime);
            if (updatedTime < firstAccessedTime || firstAccessedFile == L"")
            {
                firstAccessedTime = updatedTime;
                firstAccessedFile = file.first;
            }
        }

        // file count less that the MaxFileRetentionCount. Increment the count and 
        // continue writing.
        if (files.size() < static_cast<size_t>(MaxFileRetentionCount))
        {
            fileName_ = wformatString(
                "{0}_{1}.{2}",
                redirectedFileNamePrefix_,
                StringUtility::ToWString(max+1),
                isOutFile_ ? L"out" : L"err");
            fileName_ = Common::Path::Combine(redirectedFileLocation_, fileName_);
            return CreateFileForRedirection(isOpening);            
        }   

        // File count exceeded or equal to the MaxFileRetentionCount.
        // first accessed file is the one that we need to delete and reuse the name
        return DeleteRedirectionFile(firstAccessedFile, 0, isOpening);          
    }        
}

ErrorCode ProcessConsoleRedirector::DeleteRedirectionFile(std::wstring const & firstAccessedFile, int deleteTrialCount, bool isOpening)
{    
    // Attempt to delete the file 
    if (!Common::File::Delete(firstAccessedFile, NOTHROW())) 
    {
        if (++deleteTrialCount >= DeleteFileRetryMaxCount)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
            TraceType,
            "Error while deleting file {0}. Console will not be redirected. Error code : {1}",
            firstAccessedFile,
            error);

            // When file delete fails because of any reason, continue to read from the pipe so that
            // the other end doesnt get stuck.
            ReadPipe();
            return error;
        }

        auto thisSPtr = this->shared_from_this();
        deleteTimer_ = Timer::Create(
            "DeleteFile", 
            [thisSPtr, firstAccessedFile, deleteTrialCount, isOpening](TimerSPtr const& timerSPtr) 
            { 
                timerSPtr->Cancel();
                thisSPtr->DeleteRedirectionFile(firstAccessedFile, deleteTrialCount, isOpening); 
            });        
        deleteTimer_->Change(TimeSpan::FromMilliseconds(DeleteFileBackoffIntervalInMs));
        return ErrorCode::Success();
    }   
    
    WriteNoise(
        TraceType,
        "Deleted file {0}",
        firstAccessedFile); 

    fileName_ = firstAccessedFile;
    return CreateFileForRedirection(isOpening);
}

ErrorCode ProcessConsoleRedirector::CreateFileForRedirection(bool isOpening)
{    
    SECURITY_ATTRIBUTES saAttr; 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.lpSecurityDescriptor = NULL; 
    saAttr.bInheritHandle = false;
    HANDLE fileHandle = ::CreateFile(
        fileName_.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        &saAttr,
        CREATE_ALWAYS,
        FILE_FLAG_OVERLAPPED,
        NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) 
    { 
        auto error = ErrorCode::FromWin32Error();
        WriteWarning(
            TraceType,
            "Error while creating file {0}. Console will not be redirected. Error code : {1} Continue to read pipe",
            fileName_,
            error);
        redirectedFile_ = nullptr;
    }
    else
    {
        redirectedFile_ = make_unique<File>(fileHandle);

        WriteInfo(
            TraceType,
            "File {0} created for console redirection.",
            fileName_);
    }
    if (isOpening || redirectedFile_ == nullptr)
    {
        return ReadPipe();        
    }
    else
    {
        return WriteFile();
    }
}
