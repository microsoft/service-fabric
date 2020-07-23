// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include "KtlLogShimKernel.h"

MBInfoAccess::BlockInfo::BlockInfo(
    __in KAllocator& Allocator
) : _Allocator(&Allocator)
{
}

MBInfoAccess::BlockInfo::~BlockInfo()
{
}                        

MBInfoAccess::BlockInfo&
MBInfoAccess::BlockInfo::operator=(
    __in const MBInfoAccess::BlockInfo& Source
    )
{
    SetConstructorStatus(Source.Status());
    return *this;
}

MBInfoAccess::MBInfoAccess(
) :
   _BlockInfo(GetThisAllocator()),
   _ExclAccessEvent(FALSE,         // set as AutoReset (not manual reset)
                    FALSE)         // InitialState is not signalled
{
}

MBInfoAccess::~MBInfoAccess(
)
{
    KInvariant(_ObjectState != Opened);
}

NTSTATUS
MBInfoAccess::Create(
    __out MBInfoAccess::SPtr& Context,
    __in KGuid DiskId,
    __in_opt KString const * const Path,
    __in KtlLogContainerId ContainerId,
    __in ULONG NumberOfBlocks,
    __in ULONG BlockSize,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status = STATUS_SUCCESS;
    MBInfoAccess::SPtr mbia;

    //
    // BlockSize must be a multiple of 4K
    //
    KInvariant( (BlockSize & ~0xfff) == BlockSize);
    KInvariant(NumberOfBlocks > 0);

    if ( ((BlockSize & ~0xfff) != BlockSize) ||
         (NumberOfBlocks == 0) )
    {
        return(STATUS_INVALID_PARAMETER);
    }
    
    Context = nullptr;
    
    mbia = _new(AllocationTag, Allocator) MBInfoAccess();
    if (mbia == nullptr)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(mbia->Status()))
    {
        return(mbia->Status());
    }

    //
    // Allocate space for BlockInfo array
    //
    status = mbia->_BlockInfo.Reserve(NumberOfBlocks);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    if (! mbia->_BlockInfo.SetCount(NumberOfBlocks))
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    for (ULONG i = 0; i < NumberOfBlocks; i++)
    {
        mbia->_BlockInfo[i].LatestGenerationNumber = 0;
        mbia->_BlockInfo[i].LatestCopy = NotKnown;
        mbia->_BlockInfo[i].IoBuffer = nullptr;
    }

    mbia->_DiskId = DiskId;
    mbia->_Path = Path;
    mbia->_ContainerId = ContainerId;
    mbia->_NumberOfBlocks = NumberOfBlocks;
    mbia->_BlockSize = BlockSize;
    mbia->_TotalBlockSize = (NumberOfBlocks * BlockSize);
    mbia->_ObjectState = Closed;
    mbia->_OperationState = Unassigned;
    mbia->_File = nullptr;
    mbia->_OwningAsync = NULL;
    mbia->_ExclAccessEvent.SetEvent();
    mbia->_CompletionCounter = 0;
    mbia->_LastError = STATUS_SUCCESS;
    mbia->_FinalStatus = STATUS_SUCCESS;
    
    Context = Ktl::Move(mbia);
    
    return(status);
}

VOID
MBInfoAccess::OnReuse(
    )
{
    _OperationState = Unassigned;
    _FinalStatus = STATUS_SUCCESS;
    
}

VOID
MBInfoAccess::DispatchToFSM(
    __in NTSTATUS Status                            
    )
{
#ifdef VERBOSE
    KDbgCheckpointWData(0, "MBInfoAccess::DispatchToFSM", Status,
                        (ULONGLONG)_OperationState,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
#endif                    
    switch (_OperationState & ~0xFF)
    {
        case InitializeInitial:
        {
            InitializeMetadataFSM(Status);
            break;
        }

        case CleanupInitial:
        {
            CleanupMetadataFSM(Status);
            break;
        }
        
        case OpenInitial:
        {
            OpenMetadataFSM(Status);
            break;
        }
        
        case CloseInitial:
        {
            CloseMetadataFSM(Status);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
            break;
        }
    }
}

VOID
MBInfoAccess::OnStart(
    )
{
    KInvariant( (_OperationState == InitializeInitial) ||
             (_OperationState == CleanupInitial) ||
             (_OperationState == OpenInitial) ||
             (_OperationState == CloseInitial) );

    DispatchToFSM(STATUS_SUCCESS);
}

VOID
MBInfoAccess::DoComplete(
    __in NTSTATUS Status
    )
{
    switch (_OperationState & ~0xFF)
    {
        case InitializeInitial:
        {
            if (_File)
            {
                _File->Close();
                _File = nullptr;
            }
            
            if (! NT_SUCCESS(Status) && (_OperationState > InitializeCreateFile))
            {
                //
                // If we encounter an error after the file was created
                // then we want to cleanup up the created file
                //
                KWString fileName(GetThisAllocator());
                
                _FinalStatus = Status;
                _OperationState = InitializeDeleteFile;
                
                KAsyncContextBase::CompletionCallback completion(this, &MBInfoAccess::MBInfoAccessCompletion);
                Status = GenerateFileName(fileName);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                    Status = _FinalStatus;
                    _ObjectState = Closed;
                    break;
                }

                Status = KVolumeNamespace::DeleteFileOrDirectory(fileName,
                                                                 GetThisAllocator(),
                                                                 completion,
                                                                 this);

                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                    Status = _FinalStatus;
                    _ObjectState = Closed;
                    break;
                }
                
                return;
            } else {
                _ObjectState = Closed;
            }
            
            break;
        }

        case CleanupInitial:
        {
            break;
        }
        
        case OpenInitial:
        {
            if (! NT_SUCCESS(Status))
            {
                if (_File)
                {
                    _File->Close();
                    _File = nullptr;
                }
                _ObjectState = Closed;              
            }
            break;
        }
        
        case CloseInitial:
        {
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
            break;
        }
    }
    
    if (NT_SUCCESS(Status))
    {
        _OperationState = Completed;
    } else {
        _OperationState = CompletedWithError;
    }

    Complete(Status);
}

VOID
MBInfoAccess::MBInfoAccessCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    DispatchToFSM(Async.Status());
}


NTSTATUS
MBInfoAccess::GenerateFileName(
    __out KWString& FileName
    )
{
    // CONSIDER: Remove use of alternate streams
    
    NTSTATUS status;
    KStringView const fileName1Suffix(L".log:MBInfo");
    KStringView const fileName2Suffix(L":MBInfo");
    KGuid logContainerGuid;
    KString::SPtr guidString;
    BOOLEAN b;

    if (_Path)
    {
        //
        // Caller specified a filename and so let's use it
        //
        FileName = *_Path;
        FileName += fileName2Suffix;
    } else {
        //
        // Caller us using the default file name as only specified the disk
        //
        guidString = KString::Create(GetThisAllocator(),
                                     GUID_STRING_LENGTH);
        if (! guidString)
        {
            KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        
#if !defined(PLATFORM_UNIX)
        //
        // On windows filename expected to be of the form:
        //    "\GLOBAL??\Volume{78572cc3-1981-11e2-be6c-806e6f6e6963}\RvdLog\Log{39a26fb9-eaf6-49d8-8432-cf6d9fb9b5e2}.log"
        //
        b = guidString->FromGUID(_DiskId);
        if (! b)
        {
            return(STATUS_UNSUCCESSFUL);
        }

        b = guidString->SetNullTerminator();
        if (! b)
        {
            return(STATUS_UNSUCCESSFUL);
        }   

        FileName = L"\\GLOBAL??\\Volume";
        FileName += static_cast<WCHAR*>(*guidString);
#else
        FileName = L"";
#endif

        logContainerGuid = _ContainerId.GetReference();

        b = guidString->FromGUID(logContainerGuid);
        if (! b)
        {
            return(STATUS_UNSUCCESSFUL);
        }

        b = guidString->SetNullTerminator();
        if (! b)
        {
            return(STATUS_UNSUCCESSFUL);
        }   

#if !defined(PLATFORM_UNIX)
        FileName += L"\\RvdLog\\Log";
#else
        FileName += L"/RvdLog/Log";
#endif
        FileName += static_cast<WCHAR*>(*guidString);
        FileName += fileName1Suffix;
    }
        
    status = FileName.Status();
    
    return(status);
}


VOID
MBInfoAccess::StartInitializeMetadataBlock(
    __in_opt KArray<KIoBuffer::SPtr>* const InitializationData,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _InitializationData = InitializationData;
    _OperationState = InitializeInitial;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at MBInfoAccess::InitializeMetadataFSM
}


VOID MBInfoAccess::InitializeMetadataFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &MBInfoAccess::MBInfoAccessCompletion);

    switch (_OperationState)
    {
        case InitializeInitial:
        {
            //
            // First step is to create the file we use to store our
            // data
            //
            KWString fileName(GetThisAllocator());
            
            _OperationState = InitializeCreateFile;

            if ((_File) || (_ObjectState != Closed))
            {
                KInvariant(FALSE);
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(STATUS_INTERNAL_ERROR);
                return;
            }
            
            Status = GenerateFileName(fileName);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(Status);
                return;             
            }

            _ObjectState = Initializing;
            
            Status = KBlockFile::Create(fileName,
                                        TRUE,        // IsWriteThrough
                                        KBlockFile::eCreateNew,
                                        KBlockFile::eShareRead,
                                        _File,
                                        completion,
                                        this,        // Parent
                                        GetThisAllocator(),
                                        GetThisAllocationTag());
            if (! NT_SUCCESS(Status))
            {
                KDbgPrintfInformational("Failed to create MBInfo file %ws", (LPCWSTR)fileName);
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(Status);
                return;             
            }
            
            break;
        }

        case InitializeCreateFile:
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OperationState, _CompletionCounter);
                DoComplete(Status);
                return;
            }
            
            //
            // Next step is to allocate space needed in the file. We
            // need space for 2 copies of all blocks.
            //
            _OperationState = InitializeAllocateSpace;
            ULONG fileSize = 2 * _TotalBlockSize;

            Status = _File->SetFileSize(fileSize,
                                        completion,
                                        this
                                       );
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(Status);
                return;             
            }

            break;
        }

        case InitializeAllocateSpace:
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OperationState, _CompletionCounter);
                DoComplete(Status);
                return;
            }
            
            //
            // Next step is to initialize each copy of each block with our header and
            // initial generation numbers
            //
            _OperationState = InitializeWriteCopyA;

            KInvariant( (_InitializationData == NULL) ||
                        (_InitializationData->Count() == _NumberOfBlocks) );
            //
            // Setup BlockInfo to write initial information to copy A
            //
            _CompletionCounter = 0;
            _FinalStatusOfWrites = STATUS_SUCCESS;
            
            for (ULONG i = 0; i < _NumberOfBlocks; i++)
            {
                AsyncWriteSectionContext::SPtr writeContext;
                KIoBuffer::SPtr ioBuffer;
                VOID* buffer;

                //
                // Configure block info to write generation 2 into
                // CopyA
                //
                _BlockInfo[i].LatestGenerationNumber = 1;
                _BlockInfo[i].LatestCopy = CopyB;

                if (_InitializationData == NULL)
                {
                    Status = KIoBuffer::CreateSimple(_BlockSize,
                                                     ioBuffer,
                                                     buffer,
                                                     GetThisAllocator(),
                                                     GetThisAllocationTag());
                    if (! NT_SUCCESS(Status))
                    {
                        if (_CompletionCounter != 0)
                        {
                            _FinalStatusOfWrites = Status;
                        } else {
                            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                            DoComplete(Status);
                        }
                        return;             
                    }

                    RtlZeroMemory(buffer, _BlockSize);
                } else {
                    ioBuffer = (*_InitializationData)[i];
                }

                Status = CreateAsyncWriteSectionContext(writeContext);
                if (! NT_SUCCESS(Status))
                {
                    if (_CompletionCounter != 0)
                    {
                        _FinalStatusOfWrites = Status;
                    } else {
                        KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                        DoComplete(Status);
                    }
                    return;             
                }

                writeContext->StartWriteSection(i * _BlockSize,
                                                ioBuffer,
                                                this,
                                                completion);
                _CompletionCounter++;               
            }

            break;          
        }

        case InitializeWriteCopyA:
        {
            if ((NT_SUCCESS(_FinalStatusOfWrites)) && (! NT_SUCCESS(Status)))
            {
                //
                // Remember if any of the write asyncs failed and then
                // deal with it after all writes complete
                //
                KTraceFailedAsyncRequest(Status, this, _OperationState, _CompletionCounter);
                _FinalStatusOfWrites = Status;
            }    
            
            //
            // Wait until all of the writes for CopyA are completed and
            // then start writes to CopyB
            //
            KAssert(_CompletionCounter != 0);
            _CompletionCounter--;

            if (_CompletionCounter == 0)
            {
                if (! NT_SUCCESS(_FinalStatusOfWrites))
                {
                    //
                    // One of the write asyncs failed, let's fail the
                    // operation
                    //
                    DoComplete(_FinalStatusOfWrites);
                    return;
                } else {
                    //
                    // All writes completed, move on to write CopyB
                    //
                    _OperationState = InitializeWriteCopyB;
                    for (ULONG i = 0; i < _NumberOfBlocks; i++)
                    {
                        AsyncWriteSectionContext::SPtr writeContext;
                        KIoBuffer::SPtr ioBuffer;
                        VOID* buffer;

                        if (_InitializationData == NULL)
                        {
                            Status = KIoBuffer::CreateSimple(_BlockSize,
                                                             ioBuffer,
                                                             buffer,
                                                             GetThisAllocator(),
                                                             GetThisAllocationTag());
                            if (! NT_SUCCESS(Status))
                            {
                                if (_CompletionCounter != 0)
                                {
                                    _FinalStatusOfWrites = Status;
                                } else {
                                    KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                                    DoComplete(Status);
                                }
                                return;             
                            }

                            RtlZeroMemory(buffer, _BlockSize);
                        } else {
                            ioBuffer = (*_InitializationData)[i];
                        }

                        Status = CreateAsyncWriteSectionContext(writeContext);
                        if (! NT_SUCCESS(Status))
                        {
                            if (_CompletionCounter != 0)
                            {
                                _FinalStatusOfWrites = Status;
                            } else {
                                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                                DoComplete(Status);
                            }
                            return;             
                        }

                        writeContext->StartWriteSection(i * _BlockSize,
                                                        ioBuffer,
                                                        this,
                                                        completion);                
                        _CompletionCounter++;
                    }

                }
            } else {
                //
                // Still waiting for more writes to complete
                //
            }
            
            break;
        }
        
        case InitializeWriteCopyB:
        {
            if ((NT_SUCCESS(_FinalStatusOfWrites)) && (! NT_SUCCESS(Status)))
            {
                //
                // Remember if any of the write asyncs failed and then
                // deal with it after all writes complete
                //
                KTraceFailedAsyncRequest(Status, this, _OperationState, _CompletionCounter);
                _FinalStatusOfWrites = Status;
            }

            KAssert(_CompletionCounter != 0);
            _CompletionCounter--;

            if (_CompletionCounter == 0)
            {
                if (! NT_SUCCESS(_FinalStatusOfWrites))
                {
                    DoComplete(_FinalStatusOfWrites);
                    return;
                } else {
                    //
                    // All writes completed, close up the file and finish
                    // up. Require caller to open it via StartOpenMetadata()
                    //
                    _OperationState = InitializeCloseFile;

                    DoComplete(STATUS_SUCCESS);
                }
            } else {
                //
                // Still waiting for more writes to complete
                //
            }
            
            break;
        }

        case InitializeDeleteFile:
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OperationState, _CompletionCounter);
            }
            
            //
            // File was deleted as part of cleaning up
            //
            KInvariant(! NT_SUCCESS(_FinalStatus));
            DoComplete(_FinalStatus);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}


VOID
MBInfoAccess::StartCleanupMetadataBlock(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OperationState = CleanupInitial;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at MBInfoAccess::CleanupMetadataFSM
}

VOID
MBInfoAccess::CleanupMetadataFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &MBInfoAccess::MBInfoAccessCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_OperationState)
    {
        case CleanupInitial:
        {
            //
            // File already deleted
            //
            _OperationState = CleanupFileRemoved;
#if PLATFORM_UNIX
            KWString fileName(GetThisAllocator());
            Status = GenerateFileName(fileName);
            if (! NT_SUCCESS(Status))
            {
                DoComplete(Status);

                break;
            }
            
            Status = KVolumeNamespace::DeleteFileOrDirectory(
                fileName,
                GetThisAllocator(),
                completion,
                this);

            if (! NT_SUCCESS(Status))
            {
                DoComplete(Status);

                break;
            }
            break;
#else
            // Fall through
#endif
        }
        
        case CleanupFileRemoved:
        {

            DoComplete(STATUS_SUCCESS);

            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}


VOID
MBInfoAccess::StartOpenMetadataBlock(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OperationState = OpenInitial;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at MBInfoAccess::OpenMetadataFSM
}

VOID MBInfoAccess::OpenMetadataFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &MBInfoAccess::MBInfoAccessCompletion);

    switch (_OperationState)
    {
        case OpenInitial:
        {
            //
            // First step is to create the file we use to store our
            // data
            //
            KWString fileName(GetThisAllocator());

            _OperationState = OpenOpenFile;

            if ((_File) || (_ObjectState != Closed))
            {
                KInvariant(FALSE);
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(STATUS_INTERNAL_ERROR);
                return;
            }

            _ObjectState = Opening;
            
            Status = GenerateFileName(fileName);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(Status);
                return;             
            }

            Status = KBlockFile::Create(fileName,
                                        TRUE,        // IsWriteThrough
                                        KBlockFile::eOpenExisting,
                                        KBlockFile::eShareRead,
                                        _File,
                                        completion,
                                        this,        // Parent
                                        GetThisAllocator(),
                                        GetThisAllocationTag());
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(Status);
                return;             
            }

            break;
        }

        case OpenOpenFile:
        {

            //
            // If open failed then report the error back up
            //
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(Status);
                return;
            }
            
            //
            // Next step is to read both copies of each block, validate
            // them and decide which one is the latest version and initialize
            // the BlockInfo array
            //
            _OperationState = OpenReadBlocks;
            
            //
            // Setup BlockInfo to force read from both copies and
            // determine the latest
            //
            _FinalStatus = STATUS_SUCCESS;
            _CompletionCounter = 0;
            
            for (ULONG i = 0; i < _NumberOfBlocks; i++)
            {
                AsyncReadSectionContext::SPtr readContext;

                //
                // Configure block info to read from both copies
                //
                _BlockInfo[i].LatestGenerationNumber = 0;
                _BlockInfo[i].LatestCopy = NotKnown;

                Status = CreateAsyncReadSectionContext(readContext);
                if (! NT_SUCCESS(Status))
                {
                    if (_CompletionCounter != 0)
                    {
                        _FinalStatus = Status;
                    } else {
                        KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                        DoComplete(Status);
                        return;
                    }
                }

                readContext->StartReadSection(i * _BlockSize,
                                              _BlockSize,
                                              TRUE,                      // Cache results
                                              &_BlockInfo[i].IoBuffer,
                                              this,
                                              completion);
                _CompletionCounter++;
            }

            break;          
        }

        case OpenReadBlocks:
        {
            if ((NT_SUCCESS(_FinalStatus)) && (! NT_SUCCESS(Status)))
            {
                //
                // If one of the reads failed then note it as we do not
                // want to complete until all reads are done
                //
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                _FinalStatus = Status;
            }
            
            //
            // Wait until all of the reads complete. Validation and
            // setting of the BlockInfo happens in the read async
            //
            _CompletionCounter--;

            if (_CompletionCounter == 0)
            {
                if (NT_SUCCESS(_FinalStatus))
                {
                    //
                    // All reads completed
                    //
                    _OperationState = OpenFreeReadBuffers;
                    _ObjectState = Opened;
                }
                
                DoComplete(_FinalStatus);
            } else {
                //
                // Still waiting for more reads to complete
                //
            }
            
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}

VOID
MBInfoAccess::StartCloseMetadataBlock(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OperationState = CloseInitial;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at MBInfoAccess::CloseMetadataFSM
}

VOID MBInfoAccess::CloseMetadataFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &MBInfoAccess::MBInfoAccessCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_OperationState)
    {
        case CloseInitial:
        {

            // CONSIDER: Synchronize with outstanding read & write ops
            
            //
            // Cleanup BlockInfo file and close the handle
            //
            _OperationState = CloseFileClosed;

            if ((! _File) || (_ObjectState != Opened))
            {
                KInvariant(FALSE);
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(STATUS_INTERNAL_ERROR);
                return;
            }

            for (ULONG i = 0; i < _NumberOfBlocks; i++)
            {
                _BlockInfo[i].LatestGenerationNumber = 0;
                _BlockInfo[i].LatestCopy = NotKnown;
                _BlockInfo[i].IoBuffer = nullptr;
            }
            
            _File->Close();
            _File = nullptr;
            _ObjectState = Closed;

            DoComplete(STATUS_SUCCESS);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}

VOID
MBInfoAccess::DumpCachedReads(
)
{
    for (ULONG i = 0; i < _NumberOfBlocks; i++)
    {
        _BlockInfo[i].IoBuffer = nullptr;
    }
}

NTSTATUS
MBInfoAccess::ValidateSection(
    __in KIoBuffer::SPtr& SectionData,
    __in CopyTypeEnum CopyType,
    __in ULONGLONG FileOffset,
    __out MBInfoHeader*& Header
)
{
    UNREFERENCED_PARAMETER(CopyType);
    
    KIoBufferElement* ioElement;

    ioElement = SectionData->First();
    Header = (MBInfoHeader*)ioElement->GetBuffer();

    if (Header->Signature != MBInfoHeader::Sig)
    {
        return(K_STATUS_LOG_STRUCTURE_FAULT);
    }

    if (Header->FileOffset != FileOffset)
    {
        return(K_STATUS_LOG_STRUCTURE_FAULT);
    }

    if (Header->Version != MBInfoHeader::CurrentVersion)
    {
        return(K_STATUS_LOG_STRUCTURE_FAULT);
    }

    if (Header->SectionSize != _BlockSize)
    {
        return(K_STATUS_LOG_STRUCTURE_FAULT);
    }

    ULONGLONG headerChecksum = Header->BlockChecksum;
    KFinally([&] { Header->BlockChecksum = headerChecksum; });
    
    Header->BlockChecksum = 0;
    ULONGLONG checksum = 0;
    KtlLogVerify::ComputeCrc64ForIoBuffer(SectionData,
                                          Header->ChecksumSize,
                                          checksum);
    if (checksum != headerChecksum)
    {
        return(K_STATUS_LOG_STRUCTURE_FAULT);       
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS
MBInfoAccess::ChecksumSection(
    __in KIoBuffer::SPtr& SectionData,
    __in CopyTypeEnum CopyType,
    __in ULONGLONG GenerationNumber,
    __in ULONGLONG FileOffset
)
{
    UNREFERENCED_PARAMETER(CopyType);

    KIoBufferElement* ioElement = SectionData->First();
    MBInfoHeader* header = (MBInfoHeader*)ioElement->GetBuffer();

    header->Signature = MBInfoHeader::Sig;
    header->FileOffset = FileOffset;
    header->Version = MBInfoHeader::CurrentVersion;
    header->SectionSize = _BlockSize;
    header->Generation = GenerationNumber;
    header->Timestamp = KNt::GetSystemTime();
    header->ChecksumSize = SectionData->QuerySize();
    
    header->BlockChecksum = 0;
    ULONGLONG checksum = 0;
    KtlLogVerify::ComputeCrc64ForIoBuffer(SectionData,
                                          SectionData->QuerySize(),
                                          checksum);
    header->BlockChecksum = checksum;
    return(STATUS_SUCCESS);
}


//
// ReadSection
//
VOID
MBInfoAccess::AsyncReadSectionContext::OnStart(
    )
{
    KInvariant(_State == Initial);

    ReadSectionFSM(STATUS_SUCCESS);
}

VOID
MBInfoAccess::AsyncReadSectionContext::ReadSectionCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    ReadSectionFSM(Async.Status());
}

VOID
MBInfoAccess::AsyncReadSectionContext::DoComplete(
    __in NTSTATUS Status
    )
{
    if (NT_SUCCESS(Status))
    {
        _State = Completed;
    } else {
        _State = CompletedWithError;
    }

    if (! _CacheResults)
    {
        if (_BlockInfo)
        {
            _BlockInfo->IoBuffer = nullptr;
        }
    }

    _CopyAIoBuffer = nullptr;
    _CopyBIoBuffer = nullptr;
    
    Complete(Status);
}

VOID
MBInfoAccess::AsyncReadSectionContext::StartReadSection(
    __in ULONGLONG FileOffset,
    __in ULONG SectionSize,
    __in BOOLEAN CacheResults,
    __out KIoBuffer::SPtr* SectionData,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _State = Initial;

    _CacheResults = CacheResults;
    _FileOffset = FileOffset;
    _SectionData = SectionData;
    _SectionSize = SectionSize;
    
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at ReadSectionFSM
}

VOID
MBInfoAccess::AsyncReadSectionContext::ReadSectionFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &MBInfoAccess::AsyncReadSectionContext::ReadSectionCompletion);

#ifdef VERBOSE
    KDbgCheckpointWData(0, "MBInfoAccess::AsyncReadSectionContext::ReadSectionFSM", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)_MBInfo->_ObjectState,
                        (ULONGLONG)0);                                
#endif
    
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        DoComplete(Status);
        return;
    }

    if (_MBInfo->_ObjectState == MBInfoAccess::InError)
    {
        Status = _MBInfo->_LastError;
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        DoComplete(Status);
        return;
    }

    #pragma warning(disable:4127)   // C4127: conditional expression is constant
    while(TRUE)
    {
        switch (_State)
        {
            case Initial:
            {           
                //
                // Cleanup BlockInfo file and close the handle
                //
                VOID* buffer;
                
                _State = DetermineBlockToRead;

                if (_SectionSize > _MBInfo->_BlockSize)
                {
                    KInvariant(FALSE);
                    
                    Status = STATUS_INTERNAL_ERROR;
                    KTraceFailedAsyncRequest(Status, this, _State, _SectionSize);
                    DoComplete(Status);
                    return;                                     
                }
                
                if ((_MBInfo->_ObjectState != MBInfoAccess::Opened) &&
                    (_MBInfo->_ObjectState != MBInfoAccess::Opening))
                {
                    //
                    // Can't read if access is not opened
                    //
                    KInvariant(FALSE);
                    
                    Status = STATUS_INTERNAL_ERROR;
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }
                
                if (! _MBInfo->IsValidFileOffset(_FileOffset))
                {
                    Status = STATUS_INVALID_PARAMETER;
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }

                ULONGLONG index = _FileOffset / _MBInfo->_BlockSize;
                _BlockInfo = &_MBInfo->_BlockInfo[(ULONG)index];
                
                if (_BlockInfo->LatestCopy != NotKnown)
                {
                    //                  
                    // Since we know where the latest copy of the
                    // section is located, we can skip to reading it
                    //
                    _State = GetLatestBlock;
                    continue;
                }

                //
                // Read both blocks to determine which one is the
                // latest. Start with block A
                //
                _State = GetBlockA;

                Status = KIoBuffer::CreateSimple(_SectionSize,
                                                 _CopyAIoBuffer,
                                                 buffer,
                                                 GetThisAllocator(),
                                                 GetThisAllocationTag());
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }
            
                Status = _MBInfo->_File->Transfer(KBlockFile::eForeground,
                                                  KBlockFile::eRead,
                                                  _MBInfo->ActualFileOffset(MBInfoAccess::CopyA, _FileOffset),
                                                  *_CopyAIoBuffer,
                                                  completion,
                                                  this);
                                   
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }
                
                return;
            }

            case GetBlockA:
            {
                //
                // Block A is read, validate it and stash it as a
                // possibility
                //
                MBInfoHeader* header;
                VOID* buffer;
                
                _State = GetBlockB;

                Status = _MBInfo->ValidateSection(_CopyAIoBuffer,
                                                  MBInfoAccess::CopyA,
                                                  _FileOffset,
                                                  header);

                if (NT_SUCCESS(Status))
                {
                    //
                    // CopyA looks ok so consider this the good copy
                    // for now
                    //
                    _BlockInfo->LatestCopy = MBInfoAccess::CopyA;
                    _BlockInfo->LatestGenerationNumber = header->Generation;
                    _BlockInfo->IoBuffer = _CopyAIoBuffer;
                } else {
                    //
                    // Trace a failure in the log structure. Don't fail
                    // yet though as the other copy might be good
                    //
                    KTraceFailedAsyncRequest(K_STATUS_LOG_STRUCTURE_FAULT, this, _State, _FileOffset);
                }                
                
                //
                // Now go ahead and read block B
                //
                Status = KIoBuffer::CreateSimple(_SectionSize,
                                                 _CopyBIoBuffer,
                                                 buffer,
                                                 GetThisAllocator(),
                                                 GetThisAllocationTag());
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }
            
                Status = _MBInfo->_File->Transfer(KBlockFile::eForeground,
                                                  KBlockFile::eRead,
                                                  _MBInfo->ActualFileOffset(MBInfoAccess::CopyB, _FileOffset),
                                                  *_CopyBIoBuffer,
                                                  completion,
                                                  this);
                                   
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }
                
                return;
            }

            case GetBlockB:
            {
                MBInfoHeader* header;
                
                Status = _MBInfo->ValidateSection(_CopyBIoBuffer,
                                                  MBInfoAccess::CopyB,
                                                  _FileOffset,
                                                  header);

                if (NT_SUCCESS(Status))
                {
                    //
                    // If CopyB has a later generation number than
                    // CopyA (or 0 in the case CopyA was corrupt) then
                    // it is the right copy.
                    //
                    if (header->Generation > _BlockInfo->LatestGenerationNumber)
                    {
                        _BlockInfo->LatestCopy = MBInfoAccess::CopyB;
                        _BlockInfo->LatestGenerationNumber = header->Generation;
                        _BlockInfo->IoBuffer = _CopyBIoBuffer;
                    }
                } else {
                    //
                    // CopyB is bad. If CopyA is also bad then game
                    // over for this metadata - we mark it bad.
                    // Otherwise we live with CopyA
                    //
                    if (_BlockInfo->LatestCopy == NotKnown)
                    {
                        //
                        // Flag this entire metadata block as corrupt
                        //
                        Status = K_STATUS_LOG_STRUCTURE_FAULT;
                        _MBInfo->SetError(Status);

                        KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                        DoComplete(Status);
                        return;                                         
                    }
                }
                
                *_SectionData = _BlockInfo->IoBuffer;
                DoComplete(STATUS_SUCCESS);
                return;
                
            }

            case GetLatestBlock:
            {
                PVOID buffer;
                
                //
                // Since we know which block is the latest, go ahead
                // and read it. If we read from disk we also need to
                // verify the block
                //
                if (_BlockInfo->IoBuffer)
                {
                    //
                    // Since we've got the buffer in cache we can go
                    // ahead and return it
                    //
                    if (_SectionSize == _MBInfo->_BlockSize)
                    {
                        *_SectionData = _BlockInfo->IoBuffer;
                    } else {
                        KIoBuffer::SPtr result;

                        Status = KIoBuffer::CreateEmpty(result,
                                                        GetThisAllocator(),
                                                        GetThisAllocationTag());
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);
                            DoComplete(Status);
                            return;                                         
                        }

                        KInvariant(_BlockInfo->IoBuffer->QueryNumberOfIoBufferElements() == 1);
                        Status = result->AddIoBufferElementReference(*_BlockInfo->IoBuffer->First(),
                                                                     0,
                                                                     _SectionSize,
                                                                     GetThisAllocationTag());
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);
                            DoComplete(Status);
                            return;                                         
                        }
                        *_SectionData = result;                     
                    }
                    DoComplete(STATUS_SUCCESS);
                    return;
                }

                //
                // We need to actually go to the disk and read it
                //
                _State = ReadBlockFromDisk;
                
                Status = KIoBuffer::CreateSimple(_MBInfo->_BlockSize,
                                                 _BlockInfo->IoBuffer,
                                                 buffer,
                                                 GetThisAllocator(),
                                                 GetThisAllocationTag());
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }
            
                Status = _MBInfo->_File->Transfer(KBlockFile::eForeground,
                                                  KBlockFile::eRead,
                                                  _MBInfo->ActualFileOffset(_BlockInfo->LatestCopy, _FileOffset),
                                                  *_BlockInfo->IoBuffer,
                                                  completion,
                                                  this);
                                   
                if (! NT_SUCCESS(Status))
                {
                    _BlockInfo->IoBuffer = nullptr;
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }
                
                return;
            }

            case ReadBlockFromDisk:
            {
                //
                // Validate block read from disk
                //
                MBInfoHeader* header;
                
                Status = _MBInfo->ValidateSection(_BlockInfo->IoBuffer,
                                                  _BlockInfo->LatestCopy,
                                                  _FileOffset,
                                                  header);

                if (! NT_SUCCESS(Status))
                {
                    //
                    // Flag this entire metadata block as corrupt
                    // CONSIDER: Do we fall back to previous metadata
                    //           block ? We know it is the old one though.
                    //
                    _MBInfo->SetError(K_STATUS_LOG_STRUCTURE_FAULT);
                    
                    _BlockInfo->IoBuffer = nullptr;
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }
                
                *_SectionData = _BlockInfo->IoBuffer;
                DoComplete(STATUS_SUCCESS);
                return;             
            }

            default:
            {
                KInvariant(FALSE);

                Status = STATUS_UNSUCCESSFUL;
                KTraceFailedAsyncRequest(Status, this, _State, 0);            
                DoComplete(Status);
                return;
            }
        }
    };
}
            
VOID
MBInfoAccess::AsyncReadSectionContext::OnReuse(
    )
{
    _State = NotAssigned;
    _BlockInfo = NULL;
}

MBInfoAccess::AsyncReadSectionContext::AsyncReadSectionContext()
{
}

MBInfoAccess::AsyncReadSectionContext::~AsyncReadSectionContext()
{
}

NTSTATUS
MBInfoAccess::CreateAsyncReadSectionContext(
    __out AsyncReadSectionContext::SPtr& Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    AsyncReadSectionContext::SPtr context;
    
    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReadSectionContext();
    if (context == nullptr)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    context->_MBInfo = this;
    context->_CopyAIoBuffer = nullptr;
    context->_CopyBIoBuffer = nullptr;
    
    Context = Ktl::Move(context);
    
    return(status);
}


//
// WriteSection
//
VOID
MBInfoAccess::AsyncWriteSectionContext::OnStart(
    )
{
    KInvariant(_State == Initial);
    
    WriteSectionFSM(STATUS_SUCCESS);
}

VOID
MBInfoAccess::AsyncWriteSectionContext::WriteSectionCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    WriteSectionFSM(Async.Status());
}

VOID
MBInfoAccess::AsyncWriteSectionContext::DoComplete(
    __in NTSTATUS Status
    )
{
    if (NT_SUCCESS(Status))
    {
        _State = Completed;
    } else {
        _State = CompletedWithError;
    }

    // CONSIDER: write through cache ?
    if (_BlockInfo)
    {
        _BlockInfo->IoBuffer = nullptr;
    }
    _SectionData = nullptr;

    Complete(Status);
}

VOID
MBInfoAccess::AsyncWriteSectionContext::StartWriteSection(
    __in ULONGLONG FileOffset,
    __in KIoBuffer::SPtr& SectionData,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _State = Initial;
    
    _FileOffset = FileOffset;
    _SectionData = SectionData;
    
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at WriteSectionFSM
}

VOID
MBInfoAccess::AsyncWriteSectionContext::WriteSectionFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &MBInfoAccess::AsyncWriteSectionContext::WriteSectionCompletion);

#ifdef VERBOSE
    KDbgCheckpointWData(0, "MBInfoAccess::AsyncWriteSectionContext::WriteSectionFSM", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)_MBInfo->_ObjectState,
                        (ULONGLONG)0);                                
#endif
    
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        DoComplete(Status);
        return;
    }

    if (_MBInfo->_ObjectState == MBInfoAccess::InError)
    {
        Status = _MBInfo->_LastError;
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        DoComplete(Status);
        return;
    }
    
    do
    {
        switch (_State)
        {
            case Initial:
            {           
                //
                // Cleanup BlockInfo file and close the handle
                //
                _State = DetermineBlockToWrite;

                if ((_MBInfo->_ObjectState != MBInfoAccess::Opened) &&
                    (_MBInfo->_ObjectState != MBInfoAccess::Initializing))
                {
                    //
                    // Can't write if access is not opened
                    //
                    KInvariant(FALSE);
                    
                    Status = STATUS_INTERNAL_ERROR;
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }
                
                if (! _MBInfo->IsValidFileOffset(_FileOffset))
                {
                    //
                    // File offset must be within range
                    //
                    Status = STATUS_INVALID_PARAMETER;
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }

                if (_SectionData->QuerySize() > _MBInfo->_BlockSize)
                {
                    //
                    // Write buffer must be up to the size of the block
                    //
                    Status = STATUS_INVALID_PARAMETER;
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }

                ULONGLONG index = _FileOffset / _MBInfo->_BlockSize;
                _BlockInfo = &_MBInfo->_BlockInfo[(ULONG)index];
                
                if (_BlockInfo->LatestCopy == NotKnown)
                {
                    //                  
                    // Since we should know where the latest copy of the
                    // section is located, there is a bug here
                    //
                    KInvariant(FALSE);
                    
                    Status = STATUS_INTERNAL_ERROR;
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }

                //
                // Update header and write block to location to place next generation
                //
                _State = WriteBlock;

                _NextGenerationNumber = _BlockInfo->LatestGenerationNumber + 1; 
                _CopyType = _BlockInfo->LatestCopy == MBInfoAccess::CopyA ?
                                                              MBInfoAccess::CopyB : MBInfoAccess::CopyA;
                Status = _MBInfo->ChecksumSection(_SectionData,
                                                  _CopyType,
                                                  _NextGenerationNumber,
                                                  _FileOffset);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }

                Status = _MBInfo->_File->Transfer(KBlockFile::eForeground,
                                                  KBlockFile::eWrite,
                                                  _MBInfo->ActualFileOffset(_CopyType, _FileOffset),
                                                  *_SectionData,
                                                  completion,
                                                  this);
                
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _FileOffset);
                    DoComplete(Status);
                    return;                 
                }
                
                return;
            }

            case WriteBlock:
            {
                //
                // Block successfully written, update BlockInfo data
                // structures and complete
                //
                _BlockInfo->LatestCopy = _CopyType;
                _BlockInfo->LatestGenerationNumber = _NextGenerationNumber;
                _BlockInfo->IoBuffer = _SectionData;

                DoComplete(STATUS_SUCCESS);
                return;
            }


            default:
            {
                KInvariant(FALSE);

                Status = STATUS_UNSUCCESSFUL;
                KTraceFailedAsyncRequest(Status, this, _State, 0);            
                DoComplete(Status);
                return;
            }
        }
#pragma warning(disable:4127)   // C4127: conditional expression is constant    
    } while(TRUE);
}
            
VOID
MBInfoAccess::AsyncWriteSectionContext::OnReuse(
    )
{
    _State = NotAssigned;
    _BlockInfo = NULL;
}

MBInfoAccess::AsyncWriteSectionContext::AsyncWriteSectionContext()
{
}

MBInfoAccess::AsyncWriteSectionContext::~AsyncWriteSectionContext()
{
}

NTSTATUS
MBInfoAccess::CreateAsyncWriteSectionContext(
    __out AsyncWriteSectionContext::SPtr& Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    AsyncWriteSectionContext::SPtr context;

    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncWriteSectionContext();
    if (context == nullptr)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    context->_MBInfo = this;
    
    Context = Ktl::Move(context);
    
    return(status);
}


//
// AcquireExclusive
//

VOID MBInfoAccess::AsyncAcquireExclusiveContext::StartAcquireExclusive(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    KInvariant(! _OwningExclusive);
    Start(ParentAsyncContext, CallbackPtr);
    
    // Continued at OnStart
}

VOID
MBInfoAccess::AsyncAcquireExclusiveContext::OnStart()
{
    KInvariant(! _OwningExclusive);

    KAsyncContextBase::CompletionCallback completion(this, &MBInfoAccess::AsyncAcquireExclusiveContext::AcquireExclusiveCompletion);
    _ExclWaitContext->StartWaitUntilSet(this, completion);
    
    // Continued at AcquireExclusiveCompletion
}

VOID
MBInfoAccess::AsyncAcquireExclusiveContext::AcquireExclusiveCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    UNREFERENCED_PARAMETER(ParentAsync);
    
    if (NT_SUCCESS(status))
    {
        //
        // Lock acquired, setup the data structures
        //
        KInvariant(! _OwningExclusive);
        KInvariant(_MBInfo->_OwningAsync == NULL);
        
        _OwningExclusive = TRUE;
        _MBInfo->_OwningAsync = this;
    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }
    
    Complete(status);
}

VOID
MBInfoAccess::AsyncAcquireExclusiveContext::OnCancel(
)
{
    _ExclWaitContext->Cancel(); 
}



NTSTATUS
MBInfoAccess::AsyncAcquireExclusiveContext::ReleaseExclusive(
)
{
    NTSTATUS status = STATUS_RESOURCE_NOT_OWNED;
    
    if (_OwningExclusive)
    {
        if (_MBInfo->_OwningAsync == this)
        {
            //
            // Fix our state and release the lock
            //
            _OwningExclusive = FALSE;
            _MBInfo->_OwningAsync = NULL;
            _MBInfo->_ExclAccessEvent.SetEvent();
            status = STATUS_SUCCESS;
        } else {
            //
            // MBInfo does not think this async owns the lock
            //
            KInvariant(FALSE);
        }       
    } else {
        //
        // Expect call only when owned
        //
        KInvariant(FALSE);
    }

    return(status);
}

                
VOID
MBInfoAccess::AsyncAcquireExclusiveContext::OnReuse(
)
{
    //
    // Do not reuse if owning the lock
    //
    KInvariant(! _OwningExclusive);
    
    _ExclWaitContext->Reuse();
}

MBInfoAccess::AsyncAcquireExclusiveContext::AsyncAcquireExclusiveContext()
{
}

MBInfoAccess::AsyncAcquireExclusiveContext::~AsyncAcquireExclusiveContext()
{
    KInvariant(! _OwningExclusive);
}

NTSTATUS
MBInfoAccess::CreateAsyncAcquireExclusiveContext(
    __out AsyncAcquireExclusiveContext::SPtr& Context
)
{
    NTSTATUS status = STATUS_SUCCESS;
    AsyncAcquireExclusiveContext::SPtr context;
    
    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncAcquireExclusiveContext();
    if (context == nullptr)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    context->_MBInfo = this;
    context->_OwningExclusive = FALSE;

    status = _ExclAccessEvent.CreateWaitContext(GetThisAllocationTag(),
                                                 GetThisAllocator(),
                                                 context->_ExclWaitContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    Context = Ktl::Move(context);
    
    return(status);
}


//
// Shared Container Metadata Block Access
//
SharedLCMBInfoAccess::SharedLCMBInfoAccess(
) : _AvailableEntries(GetThisAllocator())
{
}

SharedLCMBInfoAccess::~SharedLCMBInfoAccess(
)
{
}

NTSTATUS
SharedLCMBInfoAccess::Create(
    __out SharedLCMBInfoAccess::SPtr& Context,
    __in KGuid DiskId,
    __in_opt KString const * const Path,                            
    __in KtlLogContainerId ContainerId,
    __in ULONG MaxNumberStreams,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status = STATUS_SUCCESS;
    SharedLCMBInfoAccess::SPtr context;
    
    Context = nullptr;
    
    context = _new(AllocationTag, Allocator) SharedLCMBInfoAccess();
    if (context == nullptr)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    //
    // Allocate the underlying MBInfoAccess object
    //
    context->_BlockSize = sizeof(SharedLCSection);
    context->_NumberOfBlocks = (MaxNumberStreams + (_EntriesPerSection - 1)) / _EntriesPerSection; 
    status = MBInfoAccess::Create(context->_MBInfo,
                                  DiskId,
                                  Path,
                                  ContainerId,
                                  context->_NumberOfBlocks,
                                  context->_BlockSize,
                                  Allocator,
                                  AllocationTag);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }   

    //
    // Initialize our own data structures
    //
    context->_MaxNumberEntries = context->_NumberOfBlocks * context->_EntriesPerSection;
    status = context->_AvailableEntries.Reserve(context->_MaxNumberEntries);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    if (! context->_AvailableEntries.SetCount(context->_MaxNumberEntries))
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    
    for (ULONG i = 0; i < context->_MaxNumberEntries; i++)
    {
        context->_AvailableEntries[i] = FALSE;
    }
    
    context->_MaxNumberStreams = MaxNumberStreams;

    context->_OperationState = Unassigned;

    Context = Ktl::Move(context);
    
    return(status);
}

VOID
SharedLCMBInfoAccess::OnReuse(
    )
{
    _OperationState = Unassigned;
    _FinalStatus = STATUS_SUCCESS;
}

VOID
SharedLCMBInfoAccess::DispatchToFSM(
    __in NTSTATUS Status,           
    __in KAsyncContextBase* Async
    )
{
#ifdef VERBOSE
    KDbgCheckpointWData(0, "SharedLCMBInfoAccess::DispatchToFSM", Status,
                        (ULONGLONG)_OperationState,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
#endif
    
    switch (_OperationState & ~0xFF)
    {
        case InitializeInitial:
        {
            InitializeMetadataFSM(Status, Async);
            break;
        }

        case CleanupInitial:
        {
            CleanupMetadataFSM(Status, Async);
            break;
        }
        
        case OpenInitial:
        {
            OpenMetadataFSM(Status, Async);
            break;
        }
        
        case CloseInitial:
        {
            CloseMetadataFSM(Status, Async);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
            break;
        }
    }
}

VOID
SharedLCMBInfoAccess::OnStart(
    )
{   
    KInvariant( (_OperationState == InitializeInitial) ||
            (_OperationState == CleanupInitial) ||
            (_OperationState == OpenInitial) ||
            (_OperationState == CloseInitial) );

    DispatchToFSM(STATUS_SUCCESS, NULL);
}

VOID
SharedLCMBInfoAccess::DoComplete(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &SharedLCMBInfoAccess::SharedLCMBInfoAccessCompletion);
    
    switch (_OperationState & ~0xFF)
    {
        case InitializeInitial:
        {            
            break;
        }

        case CleanupInitial:
        {
            break;
        }
        
        case OpenInitial:
        {
            if (! NT_SUCCESS(Status) && (_OperationState > OpenOpenFile))
            {
                //
                // We encountered an error while processing the info so
                // we need to close the underlying layer before
                // propogating back the error
                //
                _OperationState = OpenCloseOnError;
                _FinalStatus = Status;

                _MBInfo->Reuse();
                _MBInfo->StartCloseMetadataBlock(this,
                                                 completion);
                return;
            }
            
            for (ULONG i = 0; i < _ParallelCount; i++)
            {
                _SectionAsync[i] = nullptr;
            }

            break;
        }
        
        case CloseInitial:
        {
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
            break;
        }
    }
    
    if (NT_SUCCESS(Status))
    {
        _OperationState = Completed;
    } else {
        _OperationState = CompletedWithError;
    }

    Complete(Status);
}

VOID
SharedLCMBInfoAccess::SharedLCMBInfoAccessCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    DispatchToFSM(Async.Status(), &Async);
}

VOID
SharedLCMBInfoAccess::StartInitializeMetadataBlock(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OperationState = InitializeInitial;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at SharedLCMBInfoAccess::InitializeMetadataFSM
}


VOID SharedLCMBInfoAccess::InitializeMetadataFSM(
    __in NTSTATUS Status,
    __in KAsyncContextBase* Async
    )
{
    UNREFERENCED_PARAMETER(Async);
    
    KAsyncContextBase::CompletionCallback completion(this, &SharedLCMBInfoAccess::SharedLCMBInfoAccessCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _OperationState, _CompletionCounter);
        DoComplete(Status);
        return;
    }
    
    switch (_OperationState)
    {
        case InitializeInitial:
        {
            //
            // First step is to get the lower layer to create and
            // initialize our file
            //
            _OperationState = InitializeCreateFile;
            
            _MBInfo->Reuse();
            _MBInfo->StartInitializeMetadataBlock(NULL,
                                                  this,
                                                  completion);

            break;
        }

        case InitializeCreateFile:
        {
            //
            // If the lower layer was successful then we are good to
            // go. Since the lower layer initializes all blocks to
            // zeros and unused streams are also marked with zero guid
            // then there is nothing more to do.
            //            
            DoComplete(STATUS_SUCCESS);
            
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}


VOID
SharedLCMBInfoAccess::StartCleanupMetadataBlock(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OperationState = CleanupInitial;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at SharedLCMBInfoAccess::CleanupMetadataFSM
}

VOID
SharedLCMBInfoAccess::CleanupMetadataFSM(
    __in NTSTATUS Status,
    __in KAsyncContextBase* Async
    )
{
    UNREFERENCED_PARAMETER(Async);
    
    KAsyncContextBase::CompletionCallback completion(this, &SharedLCMBInfoAccess::SharedLCMBInfoAccessCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_OperationState)
    {
        case CleanupInitial:
        {
            //
            // Pass down to lower layer to delete the file
            //            
            _OperationState = CleanupDeleteFile;

            _MBInfo->Reuse();
            _MBInfo->StartCleanupMetadataBlock(this,
                                               completion);

            break;
        }

        case CleanupDeleteFile:
        {
            //
            // File successfully deleted
            //
            DoComplete(STATUS_SUCCESS);
            
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}

SharedLCMBInfoAccess::LCEntryEnumAsync::LCEntryEnumAsync()
{
}

SharedLCMBInfoAccess::LCEntryEnumAsync::~LCEntryEnumAsync()
{
}

VOID
SharedLCMBInfoAccess::LCEntryEnumAsync::OnStart()
{
    NTSTATUS status;
    
    status = _EnumCallback(*this);

    if (status != STATUS_PENDING)
    {
        Complete(status);
    }
}

VOID
SharedLCMBInfoAccess::LCEntryEnumAsync::StartCallback(
    __in SharedLCEntry& Entry,
    __in LCEntryEnumCallback EnumCallback,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Entry = Entry;
    _EnumCallback = EnumCallback;
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
SharedLCMBInfoAccess::LCSectionAsync::OnCompleted(
    )
{
    _SectionData = nullptr;
}

VOID
SharedLCMBInfoAccess::LCSectionAsync::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &SharedLCMBInfoAccess::LCSectionAsync::OperationCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _BlockIndex, _EntryIndex);
        Complete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            _State = ReadLCSection;
            _ReadAsync->StartReadSection(_BlockIndex * _LCMBInfo->GetBlockSize(),
                                         _LCMBInfo->GetBlockSize(),
                                         TRUE,                  // Return contents in _SectionData
                                         &_SectionData,
                                         this,
                                         completion);
            break;
        }


        case ReadLCSection:
        {
            //
            // _Index is the index to the entry within the current section we are processing
            // _EntryIndex is the index to the entry within all sections
            //
            _Index = 0;
            _EntryIndex = _BlockIndex * _LCMBInfo->GetEntriesPerSection();
            
            KInvariant(_SectionData->QueryNumberOfIoBufferElements() == 1);
            SharedLCSection* section = (SharedLCSection*)(_SectionData->First()->GetBuffer());

            if (! _LCMBInfo->IsValidSection(section, _EntryIndex))
            {
                //
                // Something is wrong with this section, this
                // bad section will ruin the whole thing
                //
                Status = K_STATUS_LOG_STRUCTURE_FAULT;
                KTraceFailedAsyncRequest(Status, this, _EntryIndex, _BlockIndex);
                Complete(Status);
                return;
            }
                        
            // fall through
            _State = ProcessLCSection;
        }

        case ProcessLCSection:
        {               
            GUID nullGUID = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
            SharedLCSection* section = (SharedLCSection*)(_SectionData->First()->GetBuffer());

            while ( _Index < _LCMBInfo->GetEntriesPerSection())
            {
                SharedLCEntry* entry = &section->Entries[_Index];

                if ((entry->Index != _EntryIndex) && (entry->Index != 0))
                {
                    //
                    // Something is wrong with this entry, this
                    // bad entry will ruin the whole thing
                    //
                    Status = K_STATUS_LOG_STRUCTURE_FAULT;
                    KTraceFailedAsyncRequest(Status, this, _BlockIndex, _EntryIndex);
                    Complete(Status);
                    return;
                }

                if (! IsEqualGUID(entry->LogStreamId.Get(), nullGUID))
                {
                    //
                    // Since there is an entry in the table, go ahead
                    // and perform the callback so that the entry is
                    // processed
                    //
                    _LCMBInfo->MarkAvailableEntry(_EntryIndex, FALSE);
                    
                    _EntryIndex++;
                    _Index++;
                    
                    if (_EnumCallback)
                    {
                        _EnumAsync->Reuse();
                        _EnumAsync->StartCallback(*entry,
                                                  _EnumCallback,
                                                  this,
                                                  completion);
                        return;
                    }
                }
                else {
                    //
                    // Note that this is a free index
                    //                          
                    _LCMBInfo->MarkAvailableEntry(_EntryIndex, TRUE);
                    
                    _EntryIndex++;
                    _Index++;
                }
            }

            //
            // All entries have been processed
            //
            Complete(STATUS_SUCCESS);
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
SharedLCMBInfoAccess::LCSectionAsync::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
SharedLCMBInfoAccess::LCSectionAsync::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
SharedLCMBInfoAccess::LCSectionAsync::OnStart(
    )
{
    KInvariant(_State == Initial);

    FSMContinue(STATUS_SUCCESS);
}


VOID
SharedLCMBInfoAccess::LCSectionAsync::StartProcessLCSection(
    __in ULONG BlockIndex,
    __in_opt LCEntryEnumCallback EnumCallback,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;

    _BlockIndex = BlockIndex;
    _EnumCallback = EnumCallback;
    Start(ParentAsyncContext, CallbackPtr);
}

SharedLCMBInfoAccess::LCSectionAsync::~LCSectionAsync()
{
}

VOID
SharedLCMBInfoAccess::LCSectionAsync::OnReuse(
    )
{
    _ReadAsync->Reuse();
    _EnumAsync->Reuse();
}

SharedLCMBInfoAccess::LCSectionAsync::LCSectionAsync(
    __in SharedLCMBInfoAccess& LCMBInfo
    ) : _LCMBInfo(&LCMBInfo)
{
    NTSTATUS status;
    SharedLCMBInfoAccess::LCEntryEnumAsync::SPtr enumAsync;
    MBInfoAccess::AsyncReadSectionContext::SPtr readAsync;

    status = _LCMBInfo->_MBInfo->CreateAsyncReadSectionContext(readAsync);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }   
                    
    enumAsync = new(GetThisAllocationTag(), GetThisAllocator()) LCEntryEnumAsync();
    if (! enumAsync)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, this, sizeof(LCEntryEnumAsync), 0);
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
    
    status = enumAsync->Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }   

    _ReadAsync = Ktl::Move(readAsync);
    _EnumAsync = Ktl::Move(enumAsync);
}

NTSTATUS
SharedLCMBInfoAccess::CreateLCSectionAsync(
    __out LCSectionAsync::SPtr& Context
    )
{
    NTSTATUS status;
    SharedLCMBInfoAccess::LCSectionAsync::SPtr context;

    Context = nullptr;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) LCSectionAsync(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, this, sizeof(LCSectionAsync), GetThisAllocationTag());
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}


VOID
SharedLCMBInfoAccess::StartOpenMetadataBlock(
    __in_opt LCEntryEnumCallback EnumCallback,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OperationState = OpenInitial;
    _EnumCallback = EnumCallback;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at SharedLCMBInfoAccess::OpenMetadataFSM
}

VOID SharedLCMBInfoAccess::OpenMetadataFSM(
    __in NTSTATUS Status,
    __in KAsyncContextBase* Async
    )
{
    UNREFERENCED_PARAMETER(Async);
    
    KAsyncContextBase::CompletionCallback completion(this, &SharedLCMBInfoAccess::SharedLCMBInfoAccessCompletion);

    switch (_OperationState)
    {
        case OpenInitial:
        {
            //
            // Allocate resources we need for parallel reads
            //
            ULONG count = _ParallelCount;
            
            if (count > _NumberOfBlocks)
            {
                count = _NumberOfBlocks;
            }
            
            for (ULONG i = 0; i < count; i++)
            {
                Status = CreateLCSectionAsync(_SectionAsync[i]);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                    DoComplete(Status);
                    return;             
                }               
            }
                                    
            _FinalStatus = STATUS_SUCCESS;

            //
            // Open access to the file we use to store our data
            //
            _OperationState = OpenOpenFile;
            _MBInfo->Reuse();
            _MBInfo->StartOpenMetadataBlock(this,
                                            completion);

            break;
        }

        case OpenOpenFile:
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, 0, (ULONGLONG)Async);
                DoComplete(Status);
                return;             
            }
            
            _OperationState = OpenProcessSectionData;

            //
            // Start the first set of parallel reads
            //
            ULONG count = _ParallelCount;
            
            if (count > _NumberOfBlocks)
            {
                count = _NumberOfBlocks;
            }
            
            _NextBlockIndexToRead = 0;
            _CompletionCounter = count;

            for (ULONG i = 0; i < count; i++)
            {
                _SectionAsync[i]->StartProcessLCSection(_NextBlockIndexToRead,
                                                       _EnumCallback,
                                                       this,
                                                       completion);
                _NextBlockIndexToRead++;
            }
            
            break;          
        }

        case OpenProcessSectionData:
        {       
            _CompletionCounter--;
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _CompletionCounter, (ULONGLONG)Async);
                _FinalStatus = Status;
            }

            if ((NT_SUCCESS(_FinalStatus)) && (_NextBlockIndexToRead < _NumberOfBlocks))
            {
                LCSectionAsync* async = (LCSectionAsync*)Async;
                async->Reuse();
                async->StartProcessLCSection(_NextBlockIndexToRead,
                                             _EnumCallback,
                                             this,
                                             completion);
                _NextBlockIndexToRead++;
                _CompletionCounter++;
            }


            if (_CompletionCounter == 0)
            {
                DoComplete(_FinalStatus);
                return;
            }
            
            break;
        }

        case OpenCloseOnError:
        {
            KInvariant(! NT_SUCCESS(_FinalStatus));
            DoComplete(_FinalStatus);
            break;
        }
                
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}

VOID
SharedLCMBInfoAccess::StartCloseMetadataBlock(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OperationState = CloseInitial;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at SharedLCMBInfoAccess::CloseMetadataFSM
}

VOID SharedLCMBInfoAccess::CloseMetadataFSM(
    __in NTSTATUS Status,
    __in KAsyncContextBase* Async
    )
{
    UNREFERENCED_PARAMETER(Async);
    
    KAsyncContextBase::CompletionCallback completion(this, &SharedLCMBInfoAccess::SharedLCMBInfoAccessCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_OperationState)
    {
        case CloseInitial:
        {
            //
            // Close up the underlying layer
            //
            _OperationState = CloseFileClosed;

            _MBInfo->Reuse();
            _MBInfo->StartCloseMetadataBlock(this,
                                             completion);
            break;
        }
        
        case CloseFileClosed:
        {
            DoComplete(STATUS_SUCCESS);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}

VOID SharedLCMBInfoAccess::FreeAvailableEntry(
    __in ULONG Index
    )
{           
    KInvariant(! _AvailableEntries[Index]);
    K_LOCK_BLOCK(_Lock)
    {
        _AvailableEntries[Index] = TRUE;
    }
}

NTSTATUS SharedLCMBInfoAccess::AllocateAvailableEntry(
    __out ULONG& Index
    )
{
#pragma prefast(disable: 28167, "DevDiv:422165: Prefast not recognizing c++ dtor being called") 
    K_LOCK_BLOCK(_Lock)
    {
        for (ULONG i = 0; i < _MaxNumberEntries; i++)
        {
            if (_AvailableEntries[i])
            {
                _AvailableEntries[i] = FALSE;
                Index = i;
                return(STATUS_SUCCESS);
            }
        }
    }

    return(STATUS_NO_MORE_ENTRIES);
}


//
// AddEntry
//
VOID
SharedLCMBInfoAccess::AsyncAddEntryContext::AddEntryCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    AddEntryFSM(Async.Status());
}

VOID SharedLCMBInfoAccess::AsyncAddEntryContext::DoComplete(
    __in NTSTATUS Status
    )
{
    NTSTATUS statusDontCare;
    
    if (_State > AcquireExclusiveLock)
    {
        statusDontCare = _MBInfoLock->ReleaseExclusive();
        if (! NT_SUCCESS(statusDontCare))
        {
            KTraceFailedAsyncRequest(statusDontCare, this, _State, 0);                      
        }
    }

    if (_EntryIndex != (ULONG)-1)
    {
        _SLCMB->FreeAvailableEntry(_EntryIndex);
        _EntryIndex = (ULONG)-1;
    }
    
    _Alias = nullptr;
    _DedicatedContainerPath = nullptr;
    _EntrySection = nullptr;

    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
            
    Complete(Status);
}

                
VOID
SharedLCMBInfoAccess::AsyncAddEntryContext::StartAddEntry(
    __in KtlLogStreamId StreamId,
    __in ULONG MaxLLMetadataSize,                                  
    __in ULONG MaxRecordSize,
    __in LONGLONG StreamSize,
    __in DispositionFlags Flags,
    __in KStringView const * DedicatedContainerPath,
    __in KStringView const * Alias,
    __out ULONG* EntryIndex,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _State = Initial;
    _StreamId = StreamId;
    _MaxLLMetadataSize = MaxLLMetadataSize;
    _MaxRecordSize = MaxRecordSize;
    _StreamSize = StreamSize;
    _Flags = Flags;

    _EntryIndexPtr = EntryIndex;
    *_EntryIndexPtr = (ULONG)-1;
    
    _EntryIndex = (ULONG)-1;

    if (Alias)
    {
        KString::SPtr alias = KString::Create(*Alias, GetThisAllocator());
        _Alias = KString::CSPtr(alias.RawPtr());
        if (! _Alias)
        {
            _State = InitialError;
            KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
            _FinalStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        _Alias = nullptr;
    }

    if (DedicatedContainerPath)
    {   
        KString::SPtr path = KString::Create(*DedicatedContainerPath, GetThisAllocator());
        _DedicatedContainerPath = KString::CSPtr(path.RawPtr());
        if (! _DedicatedContainerPath)
        {
            _State = InitialError;
            KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
            _FinalStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        _DedicatedContainerPath = nullptr;
    }
                
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
SharedLCMBInfoAccess::AsyncAddEntryContext::OnStart(
    )
{
    KInvariant((_State == Initial) ||
           (_State == InitialError));

    if (_State == InitialError)
    {
        KTraceFailedAsyncRequest(_FinalStatus, this, _State, 0);            
        DoComplete(_FinalStatus);
    } else {                
        AddEntryFSM(STATUS_SUCCESS);
    }
}

VOID
SharedLCMBInfoAccess::AsyncAddEntryContext::AddEntryFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &SharedLCMBInfoAccess::AsyncAddEntryContext::AddEntryCompletion);

#ifdef VERBOSE
    KDbgCheckpointWData(0, "SharedLCMBInfoAccess::AsyncAddEntryContext::AddEntryFSM", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
#endif
    
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
            //
            // First Step is to acquire the lock
            //
            _State = AcquireExclusiveLock;

            _MBInfoLock->Reuse();
            _MBInfoLock->StartAcquireExclusive(this,
                                               completion);
            break;
        }
        
        case AcquireExclusiveLock:
        {
            //
            // Next step is to find a free space to put the new entry
            // and read its section
            //
            _State = FindFreeEntry;
            
            Status = _SLCMB->AllocateAvailableEntry(_EntryIndex);           
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            _ReadContext->Reuse();
            _ReadContext->StartReadSection(_SLCMB->EntryIndexToFileOffset(_EntryIndex),
                                           _SLCMB->_BlockSize,
                                          FALSE,
                                          &_EntrySection,
                                          this,
                                          completion);
                        
            break;
        }
        
        case FindFreeEntry:
        {
            //
            // Now update the entry in the section and write back to the metadata block
            //          
            _State = WriteSection;

            KInvariant(_EntrySection->QueryNumberOfIoBufferElements() == 1);
            SharedLCSection* section = (SharedLCSection*)(_EntrySection->First()->GetBuffer());

            if (! _SLCMB->IsValidSection(section, _EntryIndex))
            {
                //
                // Something is wrong with this section, this
                // bad section will ruin the whole thing
                //
                Status = K_STATUS_LOG_STRUCTURE_FAULT;
                KTraceFailedAsyncRequest(Status, this, _State, _EntryIndex);
                DoComplete(Status);
                return;                         
            }
            
            ULONG sectionIndex = _SLCMB->EntryIndexToSectionIndex(_EntryIndex);
            SharedLCEntry* entry = &section->Entries[sectionIndex];

            entry->LogStreamId = _StreamId;
            entry->MaxLLMetadataSize = _MaxLLMetadataSize;
            entry->MaxRecordSize = _MaxRecordSize;
            entry->StreamSize = _StreamSize;
            entry->Flags = _Flags;
            entry->Index = _EntryIndex;

            if (_Alias)
            {
                KInvariant(_Alias->Length() <= KtlLogContainer::MaxAliasLength);
                ULONG len = MIN(KtlLogContainer::MaxAliasLength, _Alias->Length());
                
                KMemCpySafe(entry->AliasName, sizeof(entry->AliasName), *_Alias, len*sizeof(WCHAR));
                entry->AliasName[len] = 0;
            } else {
                *entry->AliasName = 0;
            }

            if (_DedicatedContainerPath)
            {
                KInvariant(_DedicatedContainerPath->Length() <= (KtlLogManager::MaxPathnameLengthInChar-1));
                ULONG len = MIN(KtlLogManager::MaxPathnameLengthInChar-1, _DedicatedContainerPath->Length());
                
                KMemCpySafe(
                    entry->PathToDedicatedContainer, 
                    KtlLogManager::MaxPathnameLengthInChar*sizeof(WCHAR),
                    *_DedicatedContainerPath, 
                    len*sizeof(WCHAR));

                entry->PathToDedicatedContainer[len] = 0;
            } else {
                *entry->PathToDedicatedContainer = 0;
            }

            _SLCMB->SetSectionHeader(section, _EntryIndex);
            
            _WriteContext->Reuse();
            _WriteContext->StartWriteSection(_SLCMB->EntryIndexToFileOffset(_EntryIndex),
                                             _EntrySection,
                                             this,
                                             completion);                                    
            break;
        }
        
        case WriteSection:
        {
            //
            // All done successfully
            //
            *_EntryIndexPtr = _EntryIndex;
            _EntryIndex = (ULONG)-1;
            
            DoComplete(STATUS_SUCCESS);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _State, 0);            
            DoComplete(Status);
            return;
        }
    }
}


VOID
SharedLCMBInfoAccess::AsyncAddEntryContext::OnReuse(
    )
{
    _State = NotAssigned;
    _FinalStatus = STATUS_SUCCESS;
}

SharedLCMBInfoAccess::AsyncAddEntryContext::AsyncAddEntryContext()
{
}

SharedLCMBInfoAccess::AsyncAddEntryContext::~AsyncAddEntryContext()
{
}

NTSTATUS
SharedLCMBInfoAccess::CreateAsyncAddEntryContext(
    __out SharedLCMBInfoAccess::AsyncAddEntryContext::SPtr& Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    AsyncAddEntryContext::SPtr context;
    
    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncAddEntryContext();
    if (context == nullptr)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    status = _MBInfo->CreateAsyncAcquireExclusiveContext(context->_MBInfoLock);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }   
    
    status = _MBInfo->CreateAsyncWriteSectionContext(context->_WriteContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = _MBInfo->CreateAsyncReadSectionContext(context->_ReadContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    context->_SLCMB = this;
    context->_DedicatedContainerPath = nullptr;
    context->_Alias = nullptr;
    context->_FinalStatus = STATUS_SUCCESS;
    context->_State = SharedLCMBInfoAccess::AsyncAddEntryContext::NotAssigned;
    
    Context = Ktl::Move(context);
    
    return(status);
}


//
// UpdateEntry
//
VOID
SharedLCMBInfoAccess::AsyncUpdateEntryContext::UpdateEntryCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    UpdateEntryFSM(Async.Status());
}

VOID SharedLCMBInfoAccess::AsyncUpdateEntryContext::DoComplete(
    __in NTSTATUS Status
    )
{
    NTSTATUS statusDontCare;
    
    if (_State > AcquireExclusiveLock)
    {
        statusDontCare = _MBInfoLock->ReleaseExclusive();
        if (! NT_SUCCESS(statusDontCare))
        {
            KTraceFailedAsyncRequest(statusDontCare, this, _State, 0);                      
        }
    }

    _Alias = nullptr;
    _DedicatedContainerPath = nullptr;
    _EntrySection = nullptr;

    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
            
    Complete(Status);
}
                
VOID
SharedLCMBInfoAccess::AsyncUpdateEntryContext::StartUpdateEntry(
    __in KtlLogStreamId StreamId,
    __in KStringView const * DedicatedContainerPath,
    __in KStringView const * Alias,
    __in DispositionFlags Flags,
    __in BOOLEAN RemoveEntry,
    __in ULONG EntryIndex,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _State = Initial;
    _StreamId = StreamId;
    _EntryIndex = EntryIndex;
    _RemoveEntry = RemoveEntry;
    _Flags = Flags;

    if (EntryIndex > _SLCMB->_MaxNumberEntries)
    {
        KInvariant(FALSE);
        _State = InitialError;
        _FinalStatus = STATUS_INVALID_PARAMETER;
    }
    
    if (Alias)
    {
        KString::SPtr alias = KString::Create(*Alias, GetThisAllocator());
        _Alias = KString::CSPtr(alias.RawPtr());
        if (! _Alias)
        {
            _State = InitialError;
            KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
            _FinalStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        _Alias = nullptr;
    }

    if (DedicatedContainerPath)
    {
        KString::SPtr dedicatedContainerPath = KString::Create(*DedicatedContainerPath, GetThisAllocator());
        _DedicatedContainerPath = KString::CSPtr(dedicatedContainerPath.RawPtr());
        if (! _DedicatedContainerPath)
        {
            _State = InitialError;
            KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
            _FinalStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        _DedicatedContainerPath = nullptr;
    }
                
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
SharedLCMBInfoAccess::AsyncUpdateEntryContext::OnStart(
    )
{
    KInvariant((_State == Initial) ||
           (_State == InitialError));

    if (_State == InitialError)
    {
        KTraceFailedAsyncRequest(_FinalStatus, this, _State, 0);            
        DoComplete(_FinalStatus);
    } else {        
        UpdateEntryFSM(STATUS_SUCCESS);
    }
}

VOID
SharedLCMBInfoAccess::AsyncUpdateEntryContext::UpdateEntryFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &SharedLCMBInfoAccess::AsyncUpdateEntryContext::UpdateEntryCompletion);

#ifdef VERBOSE
    KDbgCheckpointWData(0, "SharedLCMBInfoAccess::AsyncUpdatedEntryContext::UpdateEntryFSM", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
#endif
    
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
            //
            // First Step is to acquire the lock
            //
            _State = AcquireExclusiveLock;

            _MBInfoLock->Reuse();
            _MBInfoLock->StartAcquireExclusive(this,
                                               completion);
            break;
        }
        
        case AcquireExclusiveLock:
        {
            //
            // Next step is to find a free space to put the new entry
            // and read its section
            //          
            _State = FindEntry;

            _ReadContext->Reuse();
            _ReadContext->StartReadSection(_SLCMB->EntryIndexToFileOffset(_EntryIndex),
                                           _SLCMB->_BlockSize,
                                          FALSE,
                                          &_EntrySection,
                                          this,
                                          completion);
                        
            break;
        }
        
        case FindEntry:
        {
            //
            // Now update the entry in the section and write back to the metadata block
            //
            
            _State = WriteSection;
            
            KInvariant(_EntrySection->QueryNumberOfIoBufferElements() == 1);
            SharedLCSection* section = (SharedLCSection*)(_EntrySection->First()->GetBuffer());
            
            if (! _SLCMB->IsValidSection(section, _EntryIndex))
            {
                //
                // Something is wrong with this section, this
                // bad section will ruin the whole thing
                //
                Status = K_STATUS_LOG_STRUCTURE_FAULT;
                KTraceFailedAsyncRequest(Status, this, _State, _EntryIndex);
                DoComplete(Status);
                return;                         
            }
            
            ULONG sectionIndex = _SLCMB->EntryIndexToSectionIndex(_EntryIndex);
            SharedLCEntry* entry = &section->Entries[sectionIndex];

            KInvariant(entry->Index == _EntryIndex);
            KInvariant(IsEqualGUID(entry->LogStreamId.Get(), _StreamId.Get()));

            if (_RemoveEntry)
            {
                KGuid nullGUID(TRUE);
                
                entry->LogStreamId = nullGUID;
            }                                   

            if (_Alias)
            {
                KInvariant(_Alias->Length() <= KtlLogContainer::MaxAliasLength);
                ULONG len = MIN(KtlLogContainer::MaxAliasLength, _Alias->Length());
                
                KMemCpySafe(entry->AliasName, KtlLogManager::MaxPathnameLengthInChar*sizeof(WCHAR), *_Alias, len*sizeof(WCHAR));
                entry->AliasName[len] = 0;
            }

            if (_DedicatedContainerPath)
            {
                KInvariant(_DedicatedContainerPath->Length() <= (KtlLogManager::MaxPathnameLengthInChar-1));
                ULONG len = MIN(KtlLogManager::MaxPathnameLengthInChar-1, _DedicatedContainerPath->Length());
                
                KMemCpySafe(
                    entry->PathToDedicatedContainer, 
                    _DedicatedContainerPath->Length()*sizeof(WCHAR), 
                    *_DedicatedContainerPath, 
                    len*sizeof(WCHAR));

                entry->PathToDedicatedContainer[len] = 0;
            }

            if (_Flags != (SharedLCMBInfoAccess::DispositionFlags)-1)
            {
                entry->Flags = _Flags;
            }

            _SLCMB->SetSectionHeader(section, _EntryIndex);
            
            _WriteContext->Reuse();
            _WriteContext->StartWriteSection(_SLCMB->EntryIndexToFileOffset(_EntryIndex),
                                            _EntrySection,
                                            this,
                                            completion);                                    
            break;
        }
        
        case WriteSection:
        {
            //
            // All done successfully
            //
            if (_RemoveEntry)
            {
                _SLCMB->FreeAvailableEntry(_EntryIndex);
            }
            
            DoComplete(STATUS_SUCCESS);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _State, 0);            
            DoComplete(Status);
            return;
        }
    }
}

VOID
SharedLCMBInfoAccess::AsyncUpdateEntryContext::OnReuse(
    )
{
    _State = NotAssigned;
    _FinalStatus = STATUS_SUCCESS;
    _MBInfoLock->Reuse();
    _WriteContext->Reuse();
    _ReadContext->Reuse();
}

SharedLCMBInfoAccess::AsyncUpdateEntryContext::AsyncUpdateEntryContext()
{
}

SharedLCMBInfoAccess::AsyncUpdateEntryContext::~AsyncUpdateEntryContext()
{
}

NTSTATUS
SharedLCMBInfoAccess::CreateAsyncUpdateEntryContext(
    __out SharedLCMBInfoAccess::AsyncUpdateEntryContext::SPtr& Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    AsyncUpdateEntryContext::SPtr context;
    
    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncUpdateEntryContext();
    if (context == nullptr)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    status = _MBInfo->CreateAsyncAcquireExclusiveContext(context->_MBInfoLock);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }   

    status = _MBInfo->CreateAsyncWriteSectionContext(context->_WriteContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = _MBInfo->CreateAsyncReadSectionContext(context->_ReadContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    context->_SLCMB = this;
    context->_DedicatedContainerPath = nullptr;
    context->_Alias = nullptr;
    context->_FinalStatus = STATUS_SUCCESS;
    context->_State = SharedLCMBInfoAccess::AsyncUpdateEntryContext::NotAssigned;
    
    Context = Ktl::Move(context);
    
    return(status);
}


//
// Dedicated Container Metadata Block Access
//

DedicatedLCMBInfoAccess::DedicatedLCMBInfoAccess(
) : _InitialData(GetThisAllocator(), 1)
{
}

DedicatedLCMBInfoAccess::~DedicatedLCMBInfoAccess(
)
{
}

NTSTATUS
DedicatedLCMBInfoAccess::Create(
    __out DedicatedLCMBInfoAccess::SPtr& Context,
    __in KGuid DiskId,
    __in_opt KString const * const Path,
    __in KtlLogContainerId ContainerId,
    __in ULONG MaxMetadataSize,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status = STATUS_SUCCESS;
    DedicatedLCMBInfoAccess::SPtr context;
    
    Context = nullptr;
    
    context = _new(AllocationTag, Allocator) DedicatedLCMBInfoAccess();
    if (context == nullptr)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    //
    // Allocate the underlying MBInfoAccess object
    //
    context->_MaxMetadataSize = MaxMetadataSize;
    context->_BlockSize = sizeof(DedicatedLCHeader) + MaxMetadataSize;
    context->_NumberOfBlocks = 1;
    status = MBInfoAccess::Create(context->_MBInfo,
                                  DiskId,
                                  Path,
                                  ContainerId,
                                  context->_NumberOfBlocks,
                                  context->_BlockSize,
                                  Allocator,
                                  AllocationTag);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }   

    //
    // Initialize our own data structures
    //
    context->_Header = NULL;
    context->_HeaderIoBuffer = nullptr;
    context->_OperationState = Unassigned;

    Context = Ktl::Move(context);
    
    return(status);
}

VOID
DedicatedLCMBInfoAccess::OnReuse(
    )
{
    _OperationState = Unassigned;
    _FinalStatus = STATUS_SUCCESS;
}

VOID
DedicatedLCMBInfoAccess::DispatchToFSM(
    __in NTSTATUS Status,           
    __in KAsyncContextBase* Async
    )
{
#ifdef VERBOSE
    KDbgCheckpointWData(0, "DedicatedLCMBInfoAccess::DispatchToFSM", Status,
                        (ULONGLONG)_OperationState,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
#endif
    
    switch (_OperationState & ~0xFF)
    {
        case InitializeInitial:
        {
            InitializeMetadataFSM(Status, Async);
            break;
        }

        case CleanupInitial:
        {
            CleanupMetadataFSM(Status, Async);
            break;
        }
        
        case OpenInitial:
        {
            OpenMetadataFSM(Status, Async);
            break;
        }
        
        case CloseInitial:
        {
            CloseMetadataFSM(Status, Async);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
            break;
        }
    }
}

VOID
DedicatedLCMBInfoAccess::OnStart(
    )
{   
    KInvariant( (_OperationState == InitializeInitial) ||
            (_OperationState == CleanupInitial) ||
            (_OperationState == OpenInitial) ||
            (_OperationState == CloseInitial) );

    DispatchToFSM(STATUS_SUCCESS, NULL);
}

VOID
DedicatedLCMBInfoAccess::DoComplete(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &DedicatedLCMBInfoAccess::DedicatedLCMBInfoAccessCompletion);
    
    switch (_OperationState & ~0xFF)
    {
        case InitializeInitial:
        {
            _InitialData[0] = nullptr;
            break;
        }

        case CleanupInitial:
        {
            break;
        }
        
        case OpenInitial:
        {            
            if (! NT_SUCCESS(Status) && (_OperationState > OpenOpenFile))
            {
                //
                // We encountered an error while processing the info so
                // we need to close the underlying layer before
                // propogating back the error
                //
                _OperationState = OpenCloseOnError;
                _FinalStatus = Status;

                _MBInfo->Reuse();
                _MBInfo->StartCloseMetadataBlock(this,
                                                 completion);
                return;
            }
            
            break;
        }
        
        case CloseInitial:
        {
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
            break;
        }
    }
    
    if (NT_SUCCESS(Status))
    {
        _OperationState = Completed;
    } else {
        _OperationState = CompletedWithError;
    }

    Complete(Status);
}

VOID
DedicatedLCMBInfoAccess::DedicatedLCMBInfoAccessCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    DispatchToFSM(Async.Status(), &Async);
}

VOID
DedicatedLCMBInfoAccess::StartInitializeMetadataBlock(
    __in KBuffer::SPtr& SecurityDescriptor,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OperationState = InitializeInitial;
    _SecurityDescriptor = SecurityDescriptor;
    
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at DedicatedLCMBInfoAccess::InitializeMetadataFSM
}


VOID DedicatedLCMBInfoAccess::InitializeMetadataFSM(
    __in NTSTATUS Status,
    __in KAsyncContextBase* Async
    )
{
    UNREFERENCED_PARAMETER(Async);
    
    KAsyncContextBase::CompletionCallback completion(this, &DedicatedLCMBInfoAccess::DedicatedLCMBInfoAccessCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_OperationState)
    {
        case InitializeInitial:
        {
            VOID* p;
            DedicatedLCHeader* header;
            BOOLEAN b;
            
            //
            // First step is to get the lower layer to create and
            // initialize our file
            //
            _OperationState = InitializeCreateFile;


            if (! _SecurityDescriptor)
            {
                Status = KBuffer::Create(0, _SecurityDescriptor, GetThisAllocator(), GetThisAllocationTag());
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                    DoComplete(Status);
                    return;
                }
            }
            
            if (_SecurityDescriptor->QuerySize() > _MaxSecurityDescriptorSize)
            {
                Status = STATUS_INVALID_PARAMETER;
                KTraceFailedAsyncRequest(Status, this, _OperationState, _SecurityDescriptor ? _SecurityDescriptor->QuerySize() : 0);
                DoComplete(Status);
                return;             
            }

            b = _InitialData.SetCount(1);
            if (! b)
            {
                KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceFailedAsyncRequest(Status, this, _OperationState, 1);
                DoComplete(Status);
                return;                             
            }

            Status = KIoBuffer::CreateSimple(sizeof(DedicatedLCHeader),
                                             _InitialData[0],
                                             p,
                                             GetThisAllocator(),
                                             GetThisAllocationTag());
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(Status);
                return;
            }
            
            header = (DedicatedLCHeader*)p;
            header->MaxMetadataSize = _MaxMetadataSize;

            header->SecurityDescriptorSize = _SecurityDescriptor->QuerySize();
            KMemCpySafe(
                header->SecurityDescriptor, 
                sizeof(header->SecurityDescriptor), 
                _SecurityDescriptor->GetBuffer(), 
                _SecurityDescriptor->QuerySize());

            //
            // Initialize to -1 to indicate that the metadata block is
            // empty
            //
            header->ActualMetadataSize = (ULONG)-1;
            
            _MBInfo->Reuse();
            _MBInfo->StartInitializeMetadataBlock(&_InitialData,
                                                  this,
                                                  completion);

            break;
        }

        case InitializeCreateFile:
        {
            //
            // If the lower layer was successful then we are good to
            // go. 
            //            
            DoComplete(STATUS_SUCCESS);
            
            break;
        }

        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}


VOID
DedicatedLCMBInfoAccess::StartCleanupMetadataBlock(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OperationState = CleanupInitial;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at DedicatedLCMBInfoAccess::CleanupMetadataFSM
}

VOID
DedicatedLCMBInfoAccess::CleanupMetadataFSM(
    __in NTSTATUS Status,
    __in KAsyncContextBase* Async
    )
{
    UNREFERENCED_PARAMETER(Async);
    
    KAsyncContextBase::CompletionCallback completion(this, &DedicatedLCMBInfoAccess::DedicatedLCMBInfoAccessCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_OperationState)
    {
        case CleanupInitial:
        {
            //
            // Pass down to lower layer to delete the file
            //            
            _OperationState = CleanupDeleteFile;

            _MBInfo->Reuse();
            _MBInfo->StartCleanupMetadataBlock(this,
                                               completion);

            break;
        }

        case CleanupDeleteFile:
        {
            //
            // File successfully deleted
            //
            DoComplete(STATUS_SUCCESS);
            
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}


VOID
DedicatedLCMBInfoAccess::StartOpenMetadataBlock(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OperationState = OpenInitial;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at DedicatedLCMBInfoAccess::OpenMetadataFSM
}

VOID DedicatedLCMBInfoAccess::OpenMetadataFSM(
    __in NTSTATUS Status,
    __in KAsyncContextBase* Async
    )
{
    UNREFERENCED_PARAMETER(Async);
    
    KAsyncContextBase::CompletionCallback completion(this, &DedicatedLCMBInfoAccess::DedicatedLCMBInfoAccessCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_OperationState)
    {
        case OpenInitial:
        {
            //
            // First step is to open the file we use to store our
            // data
            //
            _OperationState = OpenOpenFile;

            _MBInfo->Reuse();
            _MBInfo->StartOpenMetadataBlock(this,
                                            completion);

            break;
        }

        case OpenOpenFile:
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, 0, (ULONGLONG)Async);
                DoComplete(Status);
                return;             
            }
            
            //
            // Next step is to read the section and cache the header
            //
            _OperationState = OpenReadHeader;

            MBInfoAccess::AsyncReadSectionContext::SPtr readContext;

            Status = _MBInfo->CreateAsyncReadSectionContext(readContext);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(Status);
                return;             
            }

            readContext->StartReadSection(0,                          // File Offset
                                          sizeof(DedicatedLCHeader),
                                          FALSE,                      // Cache results
                                          &_HeaderIoBuffer,
                                          this,
                                          completion);
            break;          
        }

        case OpenReadHeader:
        {       
            KInvariant(_HeaderIoBuffer->QueryNumberOfIoBufferElements() == 1);
            _Header = (DedicatedLCHeader*)(_HeaderIoBuffer->First()->GetBuffer());

            if (! IsValidHeader(_Header))
            {
                //
                // Something is wrong with this section, this
                // bad section will ruin the whole thing
                //
                Status = K_STATUS_LOG_STRUCTURE_FAULT;
                KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
                DoComplete(Status);
                return;                         
            }
                                            
            //
            // All is good
            //
            DoComplete(STATUS_SUCCESS);
            
            break;
        }

        case OpenCloseOnError:
        {
            KInvariant(! NT_SUCCESS(_FinalStatus));
            DoComplete(_FinalStatus);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}

VOID
DedicatedLCMBInfoAccess::StartCloseMetadataBlock(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OperationState = CloseInitial;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued at DedicatedLCMBInfoAccess::CloseMetadataFSM
}

VOID DedicatedLCMBInfoAccess::CloseMetadataFSM(
    __in NTSTATUS Status,
    __in KAsyncContextBase* Async
    )
{
    UNREFERENCED_PARAMETER(Async);
    
    KAsyncContextBase::CompletionCallback completion(this, &DedicatedLCMBInfoAccess::DedicatedLCMBInfoAccessCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _OperationState, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_OperationState)
    {
        case CloseInitial:
        {
            //
            // Close up the underlying layer
            //
            _OperationState = CloseFileClosed;

            _MBInfo->Reuse();
            _MBInfo->StartCloseMetadataBlock(this,
                                             completion);
            break;
        }
        
        case CloseFileClosed:
        {
            DoComplete(STATUS_SUCCESS);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _OperationState, 0);            
            DoComplete(Status);
            return;
        }
    }
}


//
// WriteMetadata
//
VOID
DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::WriteMetadataCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    WriteMetadataFSM(Async.Status());
}

VOID DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::DoComplete(
    __in NTSTATUS Status
    )
{
    NTSTATUS statusDontCare;
    
    if (_State > AcquireExclusiveLock)
    {
        statusDontCare = _MBInfoLock->ReleaseExclusive();
        if (! NT_SUCCESS(statusDontCare))
        {
            KTraceFailedAsyncRequest(statusDontCare, this, _State, 0);                      
        }
    }

    _LLMetadata = nullptr;

    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
            
    Complete(Status);
}

                
VOID
DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::StartWriteMetadata(
    __in KIoBuffer::SPtr& LLMetadata,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _State = Initial;
    _LLMetadata = LLMetadata;
                
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::OnStart(
    )
{
    KInvariant(_State == Initial);

    WriteMetadataFSM(STATUS_SUCCESS);
}

VOID
DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::WriteMetadataFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::WriteMetadataCompletion);

#ifdef VERBOSE
    KDbgCheckpointWData(0, "DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::WriteMetadataFSM", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
#endif                                        
    
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
            //
            // Validation of input parameters
            //
            if (_LLMetadata->QuerySize() > _DLCMB->_MaxMetadataSize)
            {
                Status = STATUS_INVALID_PARAMETER;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            //
            // First Step is to acquire the lock
            //
            _State = AcquireExclusiveLock;

            _MBInfoLock->Reuse();
            _MBInfoLock->StartAcquireExclusive(this,
                                               completion);
            break;
        }
        
        case AcquireExclusiveLock:
        {
            //
            // Next step is to combined the cached header with the user
            // metadata into a single KIoBuffer
            //
            KIoBuffer::SPtr ioBuffer;
            
            _State = WriteMetadata;

            Status = KIoBuffer::CreateEmpty(ioBuffer,
                                            GetThisAllocator(),
                                            GetThisAllocationTag());
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            KInvariant(_DLCMB->_HeaderIoBuffer->QueryNumberOfIoBufferElements() == 1);
            Status = ioBuffer->AddIoBufferElementReference(*_DLCMB->_HeaderIoBuffer->First(),
                                                           0,                // SourceStartingOffset
                                                           _DLCMB->_HeaderIoBuffer->QuerySize(),
                                                           GetThisAllocationTag());
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            for (KIoBufferElement* element = _LLMetadata->First(); element != NULL; element = _LLMetadata->Next(*element))
            {
                Status = ioBuffer->AddIoBufferElementReference(*element,
                                                               0,                // SourceStartingOffset
                                                               element->QuerySize(),
                                                               GetThisAllocationTag());
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, (ULONGLONG)element);
                    DoComplete(Status);
                    return;
                }
            }
            
            _DLCMB->_Header->ActualMetadataSize = _LLMetadata->QuerySize();

            _WriteContext->Reuse();
            _WriteContext->StartWriteSection(0,                // File Offset
                                             ioBuffer,
                                             this,
                                             completion);
                        
            break;
        }
        
        case WriteMetadata:
        {
            //
            // All done successfully
            //
            
            DoComplete(STATUS_SUCCESS);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _State, 0);            
            DoComplete(Status);
            return;
        }
    }
}


VOID
DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::OnReuse(
    )
{
    _State = NotAssigned;
}

DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::AsyncWriteMetadataContext()
{
}

DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::~AsyncWriteMetadataContext()
{
}

NTSTATUS
DedicatedLCMBInfoAccess::CreateAsyncWriteMetadataContext(
    __out DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::SPtr& Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    AsyncWriteMetadataContext::SPtr context;
    
    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncWriteMetadataContext();
    if (context == nullptr)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    status = _MBInfo->CreateAsyncAcquireExclusiveContext(context->_MBInfoLock);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }   
    
    status = _MBInfo->CreateAsyncWriteSectionContext(context->_WriteContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_DLCMB = this;
    context->_State = DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::NotAssigned;
    
    Context = Ktl::Move(context);
    
    return(status);
}


//
// ReadMetadata
//
VOID
DedicatedLCMBInfoAccess::AsyncReadMetadataContext::ReadMetadataCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    ReadMetadataFSM(Async.Status());
}

VOID DedicatedLCMBInfoAccess::AsyncReadMetadataContext::DoComplete(
    __in NTSTATUS Status
    )
{
    NTSTATUS statusDontCare;
    
    if (_State > AcquireExclusiveLock)
    {
        statusDontCare = _MBInfoLock->ReleaseExclusive();
        if (! NT_SUCCESS(statusDontCare))
        {
            KTraceFailedAsyncRequest(statusDontCare, this, _State, 0);                      
        }
    }

    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
            
    Complete(Status);
}
                
VOID
DedicatedLCMBInfoAccess::AsyncReadMetadataContext::StartReadMetadata(
    __out KIoBuffer::SPtr& LLMetadata,  
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _State = Initial;

    _LLMetadata = &LLMetadata;
                
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
DedicatedLCMBInfoAccess::AsyncReadMetadataContext::OnStart(
    )
{
    KInvariant(_State == Initial);

    ReadMetadataFSM(STATUS_SUCCESS);
}

VOID
DedicatedLCMBInfoAccess::AsyncReadMetadataContext::ReadMetadataFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &DedicatedLCMBInfoAccess::AsyncReadMetadataContext::ReadMetadataCompletion);

#ifdef VERBOSE
    KDbgCheckpointWData(0, "DedicatedLCMBInfoAccess::AsyncReadMetadataContext::ReadMetadataFSM", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
#endif
    
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
            //
            // First Step is to acquire the lock
            //
            _State = AcquireExclusiveLock;

            *_LLMetadata = nullptr;
            
            _MBInfoLock->Reuse();
            _MBInfoLock->StartAcquireExclusive(this,
                                               completion);
            break;
        }
        
        case AcquireExclusiveLock:
        {
            //
            // Next step is to read in the metadata 
            //          
            _State = ReadMetadata;

            _ReadContext->Reuse();
            _ReadContext->StartReadSection(0,            // FileOffset
                                           _DLCMB->_BlockSize,
                                           FALSE,
                                           &_IoBuffer,
                                           this,
                                           completion);
                        
            break;
        }
        
        case ReadMetadata:
        {
            //
            // Now extract the logical log metadata from the buffer read
            //            
            KIoBuffer::SPtr result;

            _State = ExtractLLMetadata;
            
            KInvariant(_IoBuffer->QueryNumberOfIoBufferElements() == 1);
            DedicatedLCHeader* header = (DedicatedLCHeader*)(_IoBuffer->First()->GetBuffer());
            
            if (! _DLCMB->IsValidHeader(header))
            {
                //
                // Something is wrong with this section, this
                // bad section will ruin the whole thing
                //
                Status = K_STATUS_LOG_STRUCTURE_FAULT;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;                         
            }

            if (header->ActualMetadataSize == (ULONG)-1)
            {
                //
                // This indicates that the metadata block was never
                // written. In this case we report back an error
                //
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;                         
            }
            
            Status = KIoBuffer::CreateEmpty(result,
                                            GetThisAllocator(),
                                            GetThisAllocationTag());
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            Status = result->AddIoBufferElementReference(*_IoBuffer->First(),
                                                         sizeof(DedicatedLCHeader),
                                                         header->ActualMetadataSize,
                                                         GetThisAllocationTag());
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            *_LLMetadata = result;
            DoComplete(STATUS_SUCCESS);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _State, 0);            
            DoComplete(Status);
            return;
        }
    }
}

VOID
DedicatedLCMBInfoAccess::AsyncReadMetadataContext::OnReuse(
    )
{
    _State = NotAssigned;
}

DedicatedLCMBInfoAccess::AsyncReadMetadataContext::AsyncReadMetadataContext()
{
}

DedicatedLCMBInfoAccess::AsyncReadMetadataContext::~AsyncReadMetadataContext()
{
}

NTSTATUS
DedicatedLCMBInfoAccess::CreateAsyncReadMetadataContext(
    __out DedicatedLCMBInfoAccess::AsyncReadMetadataContext::SPtr& Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    AsyncReadMetadataContext::SPtr context;
    
    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReadMetadataContext();
    if (context == nullptr)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    status = _MBInfo->CreateAsyncAcquireExclusiveContext(context->_MBInfoLock);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }   

    status = _MBInfo->CreateAsyncReadSectionContext(context->_ReadContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    context->_DLCMB = this;
    context->_State = DedicatedLCMBInfoAccess::AsyncReadMetadataContext::NotAssigned;
    
    Context = Ktl::Move(context);
    
    return(status);
}
