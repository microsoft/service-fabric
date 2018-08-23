// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include "KtlLogShimKernel.h"

//************************************************************************************
//
// ShimFileIo Implementation
//
//************************************************************************************
ShimFileIo::ShimFileIo(
)
{
}

ShimFileIo::~ShimFileIo()
{
}

//
// Write file
//
VOID
ShimFileIo::AsyncWriteFileContext::WriteFileCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    WriteFileFSM(Async.Status());
}

VOID
ShimFileIo::AsyncWriteFileContext::DoComplete(
    __in NTSTATUS Status
    )
{
    if (! NT_SUCCESS(Status))
    {
        _State = CompletedWithError;
    } else {
        _State = Completed;
    }

    if (_File)
    {
        _File->Close();     
    }
    
    if ( (_OperationLockHeld) && (_OperationLockHandle != nullptr) )
    {
        _OperationLockHandle->ReleaseLock();        
    }

    Complete(Status);
}

VOID
ShimFileIo::AsyncWriteFileContext::WriteFileFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &ShimFileIo::AsyncWriteFileContext::WriteFileCompletion);
    
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            _State = WaitForLock;

            //
            // Lock is used to ensure that only one async will write to
            // the alias file at one time
            //
            if (_OperationLockHandle != nullptr)
            {
                _OperationLockHandle->StartAcquireLock(this, completion);
                break;
            } else {
                _State = WaitForLock;
                // fall through
            }
        }

        case WaitForLock:
        {
            _State = CreateFile;

            _OperationLockHeld = TRUE;
            
            Status = KBlockFile::Create(*_FileName,
                                        TRUE,        // IsWriteThrough
                                        KBlockFile::eCreateAlways,
                                        KBlockFile::eShareRead,
                                        _File,
                                        completion,
                                        this,        // Parent
                                        GetThisAllocator(),
                                        _AllocationTag);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;             
            }

            break;
        }

        case CreateFile:
        {
            _State = SetFileSize;
            Status = _File->SetFileSize(_IoBuffer->QuerySize(),
                                        completion,
                                        this
                                       );
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;             
            }

            break;
        }

        case SetFileSize:
        {
            _State = WriteFile;
            Status = _File->Transfer(KBlockFile::eForeground,
                                    KBlockFile::eWrite,
                                    0,                 // FIle Offset
                                    *_IoBuffer,
                                    completion,
                                    this);
                                   
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;             
            }
            
            break;
        }

        case WriteFile:
        {
            DoComplete(Status);
            return;
        }

        case Completed:
        case CompletedWithError:
        default:
        {
            KInvariant(FALSE);
        }
    }        
}

VOID
ShimFileIo::AsyncWriteFileContext::OnStart(
    )
{
    KInvariant(_State == Initial);

    _OperationLockHeld = FALSE;
    
    WriteFileFSM(STATUS_SUCCESS);
}

VOID
ShimFileIo::AsyncWriteFileContext::StartWriteFile(
    __in KWString& FileName,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in KSharedAsyncLock::Handle::SPtr& LockHandle,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;

    KInvariant(! _File);
    
    _FileName = &FileName;
    _IoBuffer = IoBuffer;
    _OperationLockHandle = LockHandle;
        
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
ShimFileIo::AsyncWriteFileContext::OnReuse(
    )
{
    _File = nullptr;
}

ShimFileIo::AsyncWriteFileContext::AsyncWriteFileContext()
{
}

ShimFileIo::AsyncWriteFileContext::~AsyncWriteFileContext()
{
}

NTSTATUS ShimFileIo::CreateAsyncWriteFileContext(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out AsyncWriteFileContext::SPtr& Context
    )
{
    NTSTATUS status;
    ShimFileIo::AsyncWriteFileContext::SPtr context;

    context = _new(AllocationTag, Allocator) AsyncWriteFileContext();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, AllocationTag, 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_File = nullptr;
    context->_AllocationTag = AllocationTag;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Read File
//
VOID
ShimFileIo::AsyncReadFileContext::ReadFileCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    ReadFileFSM(Async.Status());
}

VOID
ShimFileIo::AsyncReadFileContext::ReadFileFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &ShimFileIo::AsyncReadFileContext::ReadFileCompletion);
    
    if (! NT_SUCCESS(Status))
    {
        _State = CompletedWithError;
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        Complete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            _State = OpenFile;
            
            Status = KBlockFile::Create(*_FileName,
                                        TRUE,        // IsWriteThrough
                                        KBlockFile::eOpenExisting,
                                        KBlockFile::eShareRead,
                                        _File,
                                        completion,
                                        this,        // Parent
                                        GetThisAllocator(),
                                        _AllocationTag);
            if (! NT_SUCCESS(Status))
            {
                _State = CompletedWithError;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;             
            }

            break;
        }

        case OpenFile:
        {
            PVOID buffer;
            
            _State = ReadFile;

            Status = KIoBuffer::CreateSimple((ULONG)_File->QueryFileSize(),
                                             _IoBuffer,
                                             buffer,
                                             GetThisAllocator(),
                                             _AllocationTag);
            if (! NT_SUCCESS(Status))
            {
                _File->Close();
                _State = CompletedWithError;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;             
            }

            KInvariant(_IoBuffer->QueryNumberOfIoBufferElements() == 1);
                                     
            
            Status = _File->Transfer(KBlockFile::eForeground,
                                    KBlockFile::eRead,
                                    0,                 // FIle Offset
                                    *_IoBuffer,
                                    completion,
                                    this);
                                   
            if (! NT_SUCCESS(Status))
            {
                _File->Close();
                _IoBuffer = nullptr;
                _State = CompletedWithError;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;             
            }
            
            break;
        }

        case ReadFile:
        {
            _State = Completed;
            _File->Close();
            *_IoBufferPtr = Ktl::Move(_IoBuffer);
            Complete(Status);
            return;
        }

        case Completed:
        case CompletedWithError:
        default:
        {
            KInvariant(FALSE);
        }
    }        
}

VOID
ShimFileIo::AsyncReadFileContext::OnStart(
    )
{
    KInvariant(_State == Initial);
    
    ReadFileFSM(STATUS_SUCCESS);
}

VOID
ShimFileIo::AsyncReadFileContext::StartReadFile(
    __in KWString& FileName,
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _FileName = &FileName;
    _IoBufferPtr = &IoBuffer;
    
    _State = Initial;
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
ShimFileIo::AsyncReadFileContext::OnReuse(
    )
{
}


ShimFileIo::AsyncReadFileContext::AsyncReadFileContext()
{
}

ShimFileIo::AsyncReadFileContext::~AsyncReadFileContext()
{
}

NTSTATUS
ShimFileIo::CreateAsyncReadFileContext(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out AsyncReadFileContext::SPtr& Context
    )
{
    NTSTATUS status;
    ShimFileIo::AsyncReadFileContext::SPtr context;
    
    context = _new(AllocationTag, Allocator) AsyncReadFileContext();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, AllocationTag, 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_AllocationTag = AllocationTag;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}
