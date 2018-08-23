/*++

Copyright (c) Microsoft Corporation

Module Name:

    DiskPerfTool.cpp

Abstract:

    KTL based disk performance test

--*/
#pragma once
#define _CRT_RAND_S
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

static const ULONG _MaximumQueueDepth = 64;

VOID
Usage()
{
    printf("DiskPerfTool - disk performance tool using KTL KBlockFile\n\n");
    printf("diskperftool -t:<read or write> -i:<sequential or random> -l:<Total # IOs> -z:<WriteThrough - TRUE or FALSE>\n");
#if !defined(PLATFORM_UNIX)
    printf("             -f:<DriveLetter>");
#else
    printf("             -f:<Path>");
#endif
    printf(" -b:<block size in 4KB> -n:<# blocks>\n");
    printf("             -q:<# foreground outstanding I/O> -x:<# background outstanding I/O>\n");
    printf("             -s:<Sparse - TRUE or FALSE>\n");
    printf("             -a:<Automated mode result XML file>\n");
    printf("\n");
    printf("    -a is mutually exclusive with the -b option. When -a is specified the test runs in an automated mode\n");
    printf("       using various block sizes and writing results to a results XML file\n");
}

class TestParameters
{
    public:
        TestParameters();
        ~TestParameters();
        
        NTSTATUS Parse(int argc, WCHAR* args[]);

        typedef enum { Read = 0, Write = 1} TestOperation;
        typedef enum { Sequential = 0, Random = 1} TestAccess;
    
        TestOperation _TestOperation;
        TestAccess _TestAccess;
        BOOLEAN _WriteThrough;
        BOOLEAN _Sparse;
        BOOLEAN _AutomatedMode;
        ULONG _TotalNumberIO;
        PWCHAR _Filename;
        ULONG _BlockSizeIn4K;
        ULONG _NumberBlocks;
        ULONG _ForegroundQueueDepth;
        ULONG _BackgroundQueueDepth;
        PWCHAR _ResultsXML;
};

TestParameters::TestParameters()
{
    _TestOperation = TestOperation::Write;
    _TestAccess = TestAccess::Sequential;
    _WriteThrough = TRUE;
    _Sparse = FALSE;
    _TotalNumberIO = 0x10000;
#if !defined(PLATFORM_UNIX)
    _Filename = L"C";
#else
    _Filename = L"/tmp";
#endif
    _BlockSizeIn4K = 16;
    _NumberBlocks = 0x10000;
    _ForegroundQueueDepth = 32;
    _BackgroundQueueDepth = 0;
    _AutomatedMode = FALSE;
    _ResultsXML = NULL;
}

TestParameters::~TestParameters()
{
}

NTSTATUS
TestParameters::Parse(
    __in int argc,
    __in WCHAR* args[]
    )
{
    for (int i = 1; i < argc; i++)
    {
        WCHAR* arg = args[i];
        int c = toupper(arg[1]);
        WCHAR c1 = arg[2];
        WCHAR c2 = arg[0];
        arg += 3;

        if ((c1 != L':') || (c2 != L'-'))
        {
            Usage();
            printf("Invalid argument %ws\n", arg);
            return(STATUS_INVALID_PARAMETER);
        }
        
        switch(c)
        {
            case L'?':
            {
                Usage();
                return(STATUS_INVALID_PARAMETER);
            }

            case L'T':              
            {
                if ((*arg == L'R') || (*arg == L'r'))
                {
                    _TestOperation = Read;
                } else if ((*arg == L'W') || (*arg == L'w')) {
                    _TestOperation = Write;
                } else {
                    printf("Invalid argument %ws\n", arg);
                    return(STATUS_INVALID_PARAMETER);
                }
                break;
            }

            case L'I':              
            {
                if ((*arg == L'R') || (*arg == L'r'))
                {
                    _TestAccess = Random;
                } else if ((*arg == L'S') || (*arg == L's')) {
                    _TestAccess = Sequential;
                } else {
                    printf("Invalid argument %ws\n", arg);
                    return(STATUS_INVALID_PARAMETER);
                }
                break;
            }

            case L'Z':              
            {
                if ((*arg == L'T') || (*arg == L't'))
                {
                    _WriteThrough = TRUE;
                } else if ((*arg == L'F') || (*arg == L'f')) {
                    _WriteThrough = FALSE;
                } else {
                    printf("Invalid argument %ws\n", arg);
                    return(STATUS_INVALID_PARAMETER);
                }
                break;
            }

            case L'S':              
            {
                if ((*arg == L'T') || (*arg == L't'))
                {
                    _Sparse = TRUE;
                } else if ((*arg == L'F') || (*arg == L'f')) {
                    _Sparse = FALSE;
                } else {
                    printf("Invalid argument %ws\n", arg);
                    return(STATUS_INVALID_PARAMETER);
                }
                break;
            }

            case L'F':
            {
                _Filename = arg;
                break;
            }

            case L'A':
            {
                 _ResultsXML = arg;
                 _AutomatedMode = TRUE;
                 break;
            }

            case L'L':
            {
                KStringView s(arg);             
                s.ToULONG(_TotalNumberIO);
                if (_TotalNumberIO == 0)
                {
                    printf("Invalid argument %ws\n", arg);
                    return(STATUS_INVALID_PARAMETER);
                }
                break;
            }
            
            case L'B':              
            {
                KStringView s(arg);
                s.ToULONG(_BlockSizeIn4K);
                if (_BlockSizeIn4K == 0)
                {
                    printf("Invalid argument %ws\n", arg);
                    return(STATUS_INVALID_PARAMETER);
                }
                break;
            }
            
            case L'N':              
            {
                KStringView s(arg);
                s.ToULONG(_NumberBlocks);
                if (_NumberBlocks == 0)
                {
                    printf("Invalid argument %ws\n", arg);
                    return(STATUS_INVALID_PARAMETER);
                }
                break;
            }
            
            case L'Q':              
            {
                KStringView s(arg);
                s.ToULONG(_ForegroundQueueDepth);
                if (_ForegroundQueueDepth == 0)
                {
                    printf("Invalid argument %ws\n", arg);
                    return(STATUS_INVALID_PARAMETER);
                }
                break;
            }
            
            case L'X':              
            {
                KStringView s(arg);
                s.ToULONG(_BackgroundQueueDepth);
                break;
            }           

            default:
            {
                printf("Invalid argument %ws\n", arg);
                return(STATUS_INVALID_PARAMETER);
            }
        }
    }

    if ((_ForegroundQueueDepth > _MaximumQueueDepth) ||
        (_ForegroundQueueDepth > _TotalNumberIO))
    {
        //
        // Do not allow queue depth to be larger than maximum queue depth or total number of IO
        //
        printf("ForegroundQueueDepth %d must be less than MaximumQueueDepth %d and TotalNumberIO %d\n",
                _ForegroundQueueDepth, _MaximumQueueDepth, _TotalNumberIO);
        return(STATUS_INVALID_PARAMETER);
    }

    return(STATUS_SUCCESS);
}

#if !defined(PLATFORM_UNIX)
NTSTATUS FindDiskIdForDriveLetter(
    __in UCHAR DriveLetter,
    __out GUID& DiskIdGuid
    )
{
    NTSTATUS status;
    KSynchronizer sync;

    KVolumeNamespace::VolumeInformationArray volInfo(KtlSystem::GlobalNonPagedAllocator());
    status = KVolumeNamespace::QueryVolumeListEx(volInfo, KtlSystem::GlobalNonPagedAllocator(), sync);
    if (!K_ASYNC_SUCCESS(status))
    {
        return status;
    }
    status = sync.WaitForCompletion();

    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    //
    // Find Drive volume GUID
    //
    ULONG i;
    for (i = 0; i < volInfo.Count(); i++)
    {
        if (volInfo[i].DriveLetter == DriveLetter)
        {
            DiskIdGuid = volInfo[i].VolumeId;
            return(STATUS_SUCCESS);
        }
    }

    return(STATUS_UNSUCCESSFUL);
}
#endif

NTSTATUS GenerateFileName(
    __out KWString& FileName,
#if !defined(PLATFORM_UNIX)
    __in KGuid DiskId
#else
    __in PWCHAR PathName
#endif
    )
{
#if !defined(PLATFORM_UNIX)
    KString::SPtr guidString;
    BOOLEAN b;
    static const ULONG GUID_STRING_LENGTH = 40;

    //
    // Caller us using the default file name as only specified the disk
    //
    // Filename expected to be of the form:
    //    "\GLOBAL??\Volume{78572cc3-1981-11e2-be6c-806e6f6e6963}\Testfile.dat"


    guidString = KString::Create(KtlSystem::GlobalNonPagedAllocator(),
                                 GUID_STRING_LENGTH);
    if (! guidString)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    b = guidString->FromGUID(DiskId);
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
    FileName = PathName;
#endif
    FileName += KVolumeNamespace::PathSeparator;
    FileName += L"Testfile.dat";
    return(FileName.Status());
}

class WorkerAsync : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(WorkerAsync);

    
    public:     
        virtual VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

    protected:
        virtual VOID FSMContinue(
            __in NTSTATUS Status,
            __in KAsyncContextBase& Async
            )
        {
            UNREFERENCED_PARAMETER(Status);
            UNREFERENCED_PARAMETER(Async);
            
            Complete(STATUS_SUCCESS);
        }

        virtual VOID OnReuse() = 0;

        virtual VOID OnCompleted()
        {
        }
        
    private:
        VOID OnStart()
        {
            _Completion.Bind(this, &WorkerAsync::OperationCompletion);
            FSMContinue(STATUS_SUCCESS, *this);
        }

        VOID OnCancel()
        {
            FSMContinue(STATUS_CANCELLED, *this);
        }

        VOID OperationCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
        )
        {
            UNREFERENCED_PARAMETER(ParentAsync);
            FSMContinue(Async.Status(), Async);
        }

    protected:
        KAsyncContextBase::CompletionCallback _Completion;
        
};

WorkerAsync::WorkerAsync()
{
}

WorkerAsync::~WorkerAsync()
{
}

class WriteFileAsync : public WorkerAsync
{
    K_FORCE_SHARED(WriteFileAsync);

    public:
        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out WriteFileAsync::SPtr& Context
        )
        {
            NTSTATUS status;
            WriteFileAsync::SPtr context;
            
            context = _new(AllocationTag, Allocator) WriteFileAsync();
            if (context == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return(status);
            }

            status = context->Status();
            if (! NT_SUCCESS(status))
            {
                return(status);
            }

            context->_IoBuffer = nullptr;
            context->_File = nullptr;

            Context = context.RawPtr();

            return(STATUS_SUCCESS);     
        }

        struct StartParameters
        {
            KBlockFile::SPtr File;
            KIoBuffer::SPtr IoBuffer;
            ULONGLONG Offset;
        };
        
        VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            StartParameters* startParameters = (StartParameters*)Parameters;

            _State = Initial;
            _File = startParameters->File;
            _IoBuffer = startParameters->IoBuffer;
            _Offset = startParameters->Offset;

            Start(ParentAsyncContext, CallbackPtr);
        }

    protected:
        VOID FSMContinue(
            __in NTSTATUS Status,
            __in KAsyncContextBase& Async
        ) override
        {
            UNREFERENCED_PARAMETER(Async);

            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            switch(_State)
            {
                case Initial:
                {
                    //
                    // We have 
                    _State = PrepareToWrite;

                    // Fall through
                }

                case PrepareToWrite:
                {
                    _State = WriteData;
                    Status = _File->Transfer(KBlockFile::IoPriority::eForeground,  // TODO: Allow bg writes too
                                             KBlockFile::TransferType::eWrite,
                                             _Offset,
                                             *_IoBuffer,
                                             _Completion,
                                             this);

                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }
                    
                    break;
                }

                case WriteData:
                {
                    _State = Completed;

                    // Fall through
                }

                case Completed:
                {
                    Complete(STATUS_SUCCESS);
                    return;
                }

                default:
                {
                    Status = STATUS_UNSUCCESSFUL;
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }
            }

            return;
        }

        VOID OnReuse() override
        {
        }

        VOID OnCompleted() override
        {
            _IoBuffer = nullptr;
            _File = nullptr;
        }
        
    private:
        enum  FSMState { Initial=0, PrepareToWrite=1, WriteData=2, Completed=3 };
        FSMState _State;

        //
        // Parameters
        //
        KBlockFile::SPtr _File;
        ULONGLONG _Offset;
        KIoBuffer::SPtr _IoBuffer;

        //
        // Internal
        //
};

WriteFileAsync::WriteFileAsync()
{
}

WriteFileAsync::~WriteFileAsync()
{
}


NTSTATUS
CreateRandomKIoBuffer(
    __in ULONG Size,
    __out KIoBuffer::SPtr& IoBuffer,
    __out PVOID& Buffer
)
{
    NTSTATUS status;

    status = KIoBuffer::CreateSimple(Size, IoBuffer, Buffer, KtlSystem::GlobalNonPagedAllocator());
    if (NT_SUCCESS(status))
    {
        PUCHAR buffer = (PUCHAR)Buffer;
        for (ULONG i = 0; i < Size; i++)
        {
            buffer[i] = rand() % 255;
        }
    }

    return(status);
}

NTSTATUS
CreateTestFile(
    __out KBlockFile::SPtr& File,
    __in WCHAR* Filename,
    __in ULONG BlockSizeIn4K,
    __in ULONG NumberBlocks,
    __in BOOLEAN WriteThrough,
    __in BOOLEAN Sparse
    )
{
    NTSTATUS status;
    KBlockFile::SPtr file;
    KSynchronizer sync;
    KWString pathName(KtlSystem::GlobalNonPagedAllocator());
    BOOLEAN fileExists = FALSE;
    ULONGLONG totalSize = (ULONGLONG)NumberBlocks * ((ULONGLONG)BlockSizeIn4K * (ULONGLONG)0x1000);
    ULONGLONG fileSize = 0;
    BOOLEAN isSparseFile = FALSE;
    
    //
    // Build the filename appropriately
    //
#if !defined(PLATFORM_UNIX)
    UCHAR driveLetter = (UCHAR)toupper(*Filename);
    GUID volumeGuid;
    status = FindDiskIdForDriveLetter(driveLetter, volumeGuid);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    status = GenerateFileName(pathName, volumeGuid);
#else
    status = GenerateFileName(pathName, Filename);
#endif

    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    //
    // If file already exists and it is the right size and type then do
    // not bother recreating it
    //
    status = KBlockFile::Create(pathName,
                                WriteThrough,
                                KBlockFile::eOpenExisting,
                                file,
                                sync,
                                NULL,
                                KtlSystem::GlobalNonPagedAllocator());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = sync.WaitForCompletion();
    if (NT_SUCCESS(status))
    {
        fileSize = file->QueryFileSize();
        isSparseFile = file->IsSparseFile();
        fileExists = (fileSize == totalSize) && (isSparseFile == Sparse);
    }

    if (fileExists)
    {
        printf("File already exists length = %lld, isSparse = %s\n", fileSize, isSparseFile ? "TRUE" : "FALSE");
        File = file;
        return(STATUS_SUCCESS);
    }
    else 
    {
        file = nullptr;
    }
    
    //
    // Create the file
    //
    if (Sparse)
    {
        status = KBlockFile::CreateSparseFile(pathName,
                                WriteThrough,
                                KBlockFile::eCreateAlways,
                                file,
                                sync,
                                NULL,        // Parent
                                KtlSystem::GlobalNonPagedAllocator());
    } else {
        status = KBlockFile::Create(pathName,
                                WriteThrough,
                                KBlockFile::eCreateAlways,
                                file,
                                sync,
                                NULL,        // Parent
                                KtlSystem::GlobalNonPagedAllocator());
    }

    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    //
    // Set the file size
    //
    status = file->SetFileSize(totalSize, sync, NULL);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }


    //
    // Initialize the file with random data
    //
    ULONG blockSizeInBytes = BlockSizeIn4K * 0x1000;
    KIoBuffer::SPtr ioBuffer;
    PVOID buffer;

    srand((ULONG)GetTickCount());

    status = CreateRandomKIoBuffer(blockSizeInBytes, ioBuffer, buffer);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    const ULONG blocksPerLoop = 256;
    KSynchronizer syncArr[blocksPerLoop];
    WriteFileAsync::SPtr initFile[blocksPerLoop];

    for (ULONG i = 0; i < blocksPerLoop; i++)
    {
        status = WriteFileAsync::Create(KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_FILE, initFile[i]);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }
    }

    ULONG loops = NumberBlocks / blocksPerLoop;
    ULONG blocksToWrite = blocksPerLoop;
    for (ULONG i = 0; i < loops; i++)
    {
        NTSTATUS lastStatus = STATUS_SUCCESS;
        ULONGLONG baseOffset = i * (blocksPerLoop * blockSizeInBytes);
        for (ULONG j = 0; j < blocksToWrite; j++)
        {
            WriteFileAsync::StartParameters startParameters;

            startParameters.File = file;
            startParameters.IoBuffer = ioBuffer;
            startParameters.Offset = (baseOffset + (j * blockSizeInBytes));
            initFile[j]->Reuse();
            initFile[j]->StartIt(&startParameters, NULL, syncArr[j]);
        }

        for (ULONG j = 0; j < blocksPerLoop; j++)
        {
            status = syncArr[j].WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                lastStatus = status;
            }
        }

        if (! NT_SUCCESS(lastStatus))
        {
            return(lastStatus);
        }

        if (i == loops)
        {
            blocksToWrite = NumberBlocks - (loops * blocksPerLoop);
        }
        printf(".");
    }

    File = file;
    return(STATUS_SUCCESS);
}
    
class WriteTestAsync : public WorkerAsync
{
    K_FORCE_SHARED(WriteTestAsync);

    public:
        static const ULONG _MaximumQueueDepth = 64;

        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out WriteTestAsync::SPtr& Context
        )
        {
            NTSTATUS status;
            WriteTestAsync::SPtr context;
            
            context = _new(AllocationTag, Allocator) WriteTestAsync();
            if (context == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return(status);
            }

            status = context->Status();
            if (! NT_SUCCESS(status))
            {
                return(status);
            }

            context->_File = nullptr;

            Context = context.RawPtr();

            return(STATUS_SUCCESS);     
        }

        struct StartParameters
        {
            KBlockFile::SPtr File;
            TestParameters::TestOperation Operation;
            TestParameters::TestAccess Access;
            ULONG BlockSizeIn4K;
            ULONG NumberBlocks;
            ULONG TotalNumberIO;
            ULONG ForegroundQueueDepth;
            ULONG BackgroundQueueDepth;
        };
        
        VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            StartParameters* startParameters = (StartParameters*)Parameters;

            _State = Initial;
            _File = startParameters->File;
            _Operation = startParameters->Operation;
            _Access = startParameters->Access;
            _BlockSizeInBytes = startParameters->BlockSizeIn4K * 0x1000;
            _NumberBlocks = startParameters->NumberBlocks;
            _TotalNumberIO = startParameters->TotalNumberIO;
            _ForegroundQueueDepth = startParameters->ForegroundQueueDepth;
            _BackgroundQueueDepth = startParameters->BackgroundQueueDepth;

            Start(ParentAsyncContext, CallbackPtr);
        }

    protected:
        VOID FSMContinue(
            __in NTSTATUS Status,
             __in KAsyncContextBase& Async
            ) override
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _LastStatus = Status;
            }

            switch(_State)
            {
                case Initial:
                {
                    srand((ULONG)GetTickCount());

                    _LastStatus = STATUS_SUCCESS;
                    _TotalNumberBytesWritten = 0;

                    _TotalFileSize = (ULONGLONG)((ULONGLONG)_NumberBlocks * (ULONGLONG)_BlockSizeInBytes);

                    if ((_ForegroundQueueDepth > _MaximumQueueDepth) ||
                        (_ForegroundQueueDepth > _TotalNumberIO))
                    {
                        //
                        // Do not allow queue depth to be larger than maximum queue depth or total number of IO
                        //
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }

                    //
                    // Initialize the write asyncs and the random buffers to use
                    //
                    for (ULONG i = 0; i < _ForegroundQueueDepth; i++)
                    {
                        Status = WriteFileAsync::Create(KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST, _WriteFile[i]);
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);
                            Complete(Status);
                            return;
                        }

                        PVOID buffer;
                        Status = CreateRandomKIoBuffer(_BlockSizeInBytes, _IoBuffer[i], buffer);
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);
                            Complete(Status);
                            return;
                        }

                        _StartParameters[i].File = _File;
                        _StartParameters[i].IoBuffer = _IoBuffer[i];
                        if (_Access == TestParameters::TestAccess::Sequential)
                        {
                            _StartParameters[i].Offset = ((ULONGLONG)i * (ULONGLONG)_BlockSizeInBytes) % _TotalFileSize;
                        } else {
                            _StartParameters[i].Offset = (rand() % _NumberBlocks) * _BlockSizeInBytes;
                        }
                    }

                    _NumberWritesCompleted = 0;
                    _NumberWritesStarted = 0;

                    _State = StartAllWrites;

                    // Fall through
                }

                case StartAllWrites:
                {
                    _StartTick = GetTickCount64();
                    for (ULONG i = 0; i < _ForegroundQueueDepth; i++)
                    {
                        _WriteFile[i]->StartIt(&_StartParameters[i], this, _Completion);
                        _NumberWritesStarted++;
                    }
                    _State = StartNextWrite;

                    break;
                }

                case StartNextWrite:
                {
                    WriteFileAsync::StartParameters startParameters;

                    _NumberWritesCompleted++;
                    _TotalNumberBytesWritten += _BlockSizeInBytes;

                    if (_NumberWritesCompleted < (_TotalNumberIO-1))
                    {
                        if (_NumberWritesStarted < (_TotalNumberIO-1))
                        {
                            WriteFileAsync::SPtr w = (WriteFileAsync*)&Async;

                            WriteFileAsync::StartParameters startParameters1;

                            startParameters1.File = _File;
                            startParameters1.IoBuffer = _IoBuffer[(rand()%_ForegroundQueueDepth)];
                            if (_Access == TestParameters::TestAccess::Sequential)
                            {
                                startParameters1.Offset = ((ULONGLONG)_NumberWritesStarted * (ULONGLONG)_BlockSizeInBytes) % _TotalFileSize;
                            } else {
                                startParameters1.Offset = (rand() % _NumberBlocks) * _BlockSizeInBytes;
                            }

                            w->Reuse();
                            w->StartIt(&startParameters1, this, _Completion);
                            _NumberWritesStarted++;
                        }

                        break;
                    }

                    _State = Completed;
                    
                    //
                    // fall through
                    //
                }

                case Completed:
                {
                    _EndTick = GetTickCount64();
                    Complete(_LastStatus);
                    return;
                }

                default:
                {
                    Status = STATUS_UNSUCCESSFUL;
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }
            }

            return;
        }

        VOID OnReuse() override
        {
        }

        VOID OnCompleted() override
        {
            for (ULONG i = 0; i < _ForegroundQueueDepth; i++)
            {
                _IoBuffer[i] = nullptr;
                _WriteFile[i] = nullptr;
                _StartParameters[i].File = nullptr;
                _StartParameters[i].IoBuffer = nullptr;
            }
            _File = nullptr;
        }
        
    private:
        enum  FSMState { Initial=0, StartAllWrites=1, StartNextWrite=2, Completed=3 };
        FSMState _State;

        //
        // Parameters
        //
        KBlockFile::SPtr _File;
        TestParameters::TestOperation _Operation;
        TestParameters::TestAccess _Access;
        ULONG _BlockSizeInBytes;
        ULONG _NumberBlocks;
        ULONG _TotalNumberIO;
        ULONG _ForegroundQueueDepth;
        ULONG _BackgroundQueueDepth;
        ULONGLONG _TotalFileSize;

        //
        // Internal
        //
        NTSTATUS _LastStatus;
        KIoBuffer::SPtr _IoBuffer[_MaximumQueueDepth];
        WriteFileAsync::SPtr _WriteFile[_MaximumQueueDepth];
        WriteFileAsync::StartParameters _StartParameters[_MaximumQueueDepth];

        //
        // Results
        //
    public:
        ULONGLONG _StartTick;
        ULONGLONG _EndTick;
        ULONGLONG _TotalNumberBytesWritten;
        ULONG _NumberWritesStarted;
        ULONG _NumberWritesCompleted;

};

WriteTestAsync::WriteTestAsync()
{
}

WriteTestAsync::~WriteTestAsync()
{
}

class ReadFileAsync : public WorkerAsync
{
    K_FORCE_SHARED(ReadFileAsync);

    public:
        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out ReadFileAsync::SPtr& Context
        )
        {
            NTSTATUS status;
            ReadFileAsync::SPtr context;
            
            context = _new(AllocationTag, Allocator) ReadFileAsync();
            if (context == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return(status);
            }

            status = context->Status();
            if (! NT_SUCCESS(status))
            {
                return(status);
            }

            context->_IoBuffer = nullptr;
            context->_File = nullptr;

            Context = context.RawPtr();

            return(STATUS_SUCCESS);     
        }

        struct StartParameters
        {
            KBlockFile::SPtr File;
            KIoBuffer::SPtr IoBuffer;
            ULONGLONG Offset;
        };
        
        VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            StartParameters* startParameters = (StartParameters*)Parameters;

            _State = Initial;
            _File = startParameters->File;
            _Offset = startParameters->Offset;
            _IoBuffer = startParameters->IoBuffer;

            Start(ParentAsyncContext, CallbackPtr);
        }

    protected:
        VOID FSMContinue(
            __in NTSTATUS Status,
            __in KAsyncContextBase& Async
        ) override
        {
            UNREFERENCED_PARAMETER(Async);

            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            switch(_State)
            {
                case Initial:
                {
                    //
                    // We have 
                    _State = PrepareToRead;

                    // Fall through
                }

                case PrepareToRead:
                {
                    _State = ReadData;
                    Status = _File->Transfer(KBlockFile::IoPriority::eForeground,  // TODO: Allow bg writes too
                                             KBlockFile::TransferType::eRead,
                                             _Offset,
                                             *_IoBuffer,
                                             _Completion,
                                             this);

                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }
                    
                    break;
                }

                case ReadData:
                {
                    _State = Completed;

                    // Fall through
                }

                case Completed:
                {
                    Complete(STATUS_SUCCESS);
                    return;
                }

                default:
                {
                    Status = STATUS_UNSUCCESSFUL;
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }
            }

            return;
        }

        VOID OnReuse() override
        {
        }

        VOID OnCompleted() override
        {
            _IoBuffer = nullptr;
            _File = nullptr;
        }
        
    private:
        enum  FSMState { Initial=0, PrepareToRead=1, ReadData=2, Completed=3 };
        FSMState _State;

        //
        // Parameters
        //
        KBlockFile::SPtr _File;
        ULONGLONG _Offset;
        KIoBuffer::SPtr _IoBuffer;

        //
        // Internal
        //
};

ReadFileAsync::ReadFileAsync()
{
}

ReadFileAsync::~ReadFileAsync()
{
}

class ReadTestAsync : public WorkerAsync
{
    K_FORCE_SHARED(ReadTestAsync);

    public:
        static const ULONG _MaximumQueueDepth = 64;

        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out ReadTestAsync::SPtr& Context
        )
        {
            NTSTATUS status;
            ReadTestAsync::SPtr context;
            
            context = _new(AllocationTag, Allocator) ReadTestAsync();
            if (context == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return(status);
            }

            status = context->Status();
            if (! NT_SUCCESS(status))
            {
                return(status);
            }

            context->_File = nullptr;

            Context = context.RawPtr();

            return(STATUS_SUCCESS);     
        }

        struct StartParameters
        {
            KBlockFile::SPtr File;
            TestParameters::TestOperation Operation;
            TestParameters::TestAccess Access;
            ULONG BlockSizeIn4K;
            ULONG NumberBlocks;
            ULONG TotalNumberIO;
            ULONG ForegroundQueueDepth;
            ULONG BackgroundQueueDepth;
        };
        
        VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            StartParameters* startParameters = (StartParameters*)Parameters;

            _State = Initial;
            _File = startParameters->File;
            _Operation = startParameters->Operation;
            _Access = startParameters->Access;
            _BlockSizeInBytes = startParameters->BlockSizeIn4K * 0x1000;
            _NumberBlocks = startParameters->NumberBlocks;
            _TotalNumberIO = startParameters->TotalNumberIO;
            _ForegroundQueueDepth = startParameters->ForegroundQueueDepth;
            _BackgroundQueueDepth = startParameters->BackgroundQueueDepth;

            Start(ParentAsyncContext, CallbackPtr);
        }

    protected:
        VOID FSMContinue(
            __in NTSTATUS Status,
             __in KAsyncContextBase& Async
            ) override
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _LastStatus = Status;
            }

            switch(_State)
            {
                case Initial:
                {
                    srand((ULONG)GetTickCount());

                    _LastStatus = STATUS_SUCCESS;
                    _TotalNumberBytesRead = 0;

                    if ((_Access == TestParameters::TestAccess::Sequential) &&
                        (_TotalNumberIO > _NumberBlocks))
                    {
                        //
                        // Do not allow sequential write past end of file
                        //
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }

                    if ((_ForegroundQueueDepth > _MaximumQueueDepth) ||
                        (_ForegroundQueueDepth > _TotalNumberIO))
                    {
                        //
                        // Do not allow queue depth to be larger than maximum queue depth or total number of IO
                        //
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }

                    //
                    // Initialize the write asyncs and the random buffers to use
                    //
                    for (ULONG i = 0; i < _ForegroundQueueDepth; i++)
                    {
                        Status = ReadFileAsync::Create(KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST, _ReadFile[i]);
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);
                            Complete(Status);
                            return;
                        }

                        PVOID buffer;
                        Status = KIoBuffer::CreateSimple(_BlockSizeInBytes, _IoBuffer[i], buffer, KtlSystem::GlobalNonPagedAllocator());
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);
                            Complete(Status);
                            return;
                        }

                        _StartParameters[i].IoBuffer = _IoBuffer[i];
                        _StartParameters[i].File = _File;
                        if (_Access == TestParameters::TestAccess::Sequential)
                        {
                            _StartParameters[i].Offset = i * _BlockSizeInBytes;
                        } else {
                            _StartParameters[i].Offset = (rand() % _NumberBlocks) * _BlockSizeInBytes;
                        }
                    }

                    _NumberReadsCompleted = 0;
                    _NumberReadsStarted = 0;

                    _State = StartAllReads;

                    // Fall through
                }

                case StartAllReads:
                {
                    _StartTick = GetTickCount64();
                    for (ULONG i = 0; i < _ForegroundQueueDepth; i++)
                    {
                        _ReadFile[i]->StartIt(&_StartParameters[i], this, _Completion);
                        _NumberReadsStarted++;
                    }
                    _State = StartNextRead;

                    break;
                }

                case StartNextRead:
                {
                    WriteFileAsync::StartParameters startParameters;

                    _NumberReadsCompleted++;
                    _TotalNumberBytesRead += _BlockSizeInBytes;

                    if (_NumberReadsCompleted < (_TotalNumberIO-1))
                    {
                        if (_NumberReadsStarted < (_TotalNumberIO-1))
                        {
                            ReadFileAsync::SPtr w = (ReadFileAsync*)&Async;

                            ReadFileAsync::StartParameters startParameters1;

                            startParameters1.IoBuffer = _IoBuffer[(rand()%_ForegroundQueueDepth)];
                            startParameters1.File = _File;
                            if (_Access == TestParameters::TestAccess::Sequential)
                            {
                                startParameters1.Offset = _NumberReadsStarted * _BlockSizeInBytes;
                            } else {
                                startParameters1.Offset = (rand() % _NumberBlocks) * _BlockSizeInBytes;
                            }

                            w->Reuse();
                            w->StartIt(&startParameters1, this, _Completion);
                            _NumberReadsStarted++;
                        }

                        break;
                    }

                    _State = Completed;
                    
                    //
                    // fall through
                    //
                }

                case Completed:
                {
                    _EndTick = GetTickCount64();
                    Complete(_LastStatus);
                    return;
                }

                default:
                {
                    Status = STATUS_UNSUCCESSFUL;
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }
            }

            return;
        }

        VOID OnReuse() override
        {
        }

        VOID OnCompleted() override
        {
            for (ULONG i = 0; i < _ForegroundQueueDepth; i++)
            {
                _ReadFile[i] = nullptr;
                _StartParameters[i].File = nullptr;
                _IoBuffer[i] = nullptr;
            }
            _File = nullptr;
        }
        
    private:
        enum  FSMState { Initial=0, StartAllReads=1, StartNextRead=2, Completed=3 };
        FSMState _State;

        //
        // Parameters
        //
        KBlockFile::SPtr _File;
        TestParameters::TestOperation _Operation;
        TestParameters::TestAccess _Access;
        ULONG _BlockSizeInBytes;
        ULONG _NumberBlocks;
        ULONG _TotalNumberIO;
        ULONG _ForegroundQueueDepth;
        ULONG _BackgroundQueueDepth;

        //
        // Internal
        //
        NTSTATUS _LastStatus;
        ReadFileAsync::SPtr _ReadFile[_MaximumQueueDepth];
        ReadFileAsync::StartParameters _StartParameters[_MaximumQueueDepth];
        KIoBuffer::SPtr _IoBuffer[_MaximumQueueDepth];

        //
        // Results
        //
    public:
        ULONGLONG _StartTick;
        ULONGLONG _EndTick;
        ULONGLONG _TotalNumberBytesRead;
        ULONG _NumberReadsStarted;
        ULONG _NumberReadsCompleted;
};

ReadTestAsync::ReadTestAsync()
{
}

ReadTestAsync::~ReadTestAsync()
{
}


NTSTATUS
PerformTest(
    __in KBlockFile::SPtr File,
    __in TestParameters::TestOperation Operation,
    __in TestParameters::TestAccess Access,
    __in ULONG BlockSizeIn4K,
    __in ULONG NumberBlocks,
    __in ULONG TotalNumberIO,
    __in ULONG ForegroundQueueDepth,
    __in ULONG BackgroundQueueDepth,
    __out float& IoPerSecond,
    __out float& MBPerSecond
    )
{
    NTSTATUS status;
    float ticks;
    KSynchronizer sync;

    if (Operation == TestParameters::TestOperation::Write)
    {
        WriteTestAsync::SPtr writeTest;
        WriteTestAsync::StartParameters startParameters;

        status = WriteTestAsync::Create(KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST, writeTest);
        if (! NT_SUCCESS(status))
        {
            printf("Create Write Test failed with Status %x\n", status);
            return(status);
        }

        startParameters.File = File;
        startParameters.Access = Access;
        startParameters.BlockSizeIn4K = BlockSizeIn4K;
        startParameters.NumberBlocks = NumberBlocks;
        startParameters.TotalNumberIO = TotalNumberIO;
        startParameters.ForegroundQueueDepth = ForegroundQueueDepth;
        startParameters.BackgroundQueueDepth = BackgroundQueueDepth;

        writeTest->StartIt(&startParameters, NULL, sync);

        do
        {
            status = sync.WaitForCompletion(10*1000);
            printf("%d IO Started %d IO Completed\n", writeTest->_NumberWritesStarted, writeTest->_NumberWritesCompleted);
        } while (status == STATUS_IO_TIMEOUT);

        if (! NT_SUCCESS(status))
        {
            printf("Write Test failed with Status %x\n", status);
            return(status);
        }

        ticks = (float)(writeTest->_EndTick - writeTest->_StartTick);
    } else {
        ReadTestAsync::SPtr readTest;
        ReadTestAsync::StartParameters startParameters;

        status = ReadTestAsync::Create(KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST, readTest);
        if (! NT_SUCCESS(status))
        {
            printf("Create Write Test failed with Status %x\n", status);
            return(status);
        }

        startParameters.File = File;
        startParameters.Access = Access;
        startParameters.BlockSizeIn4K = BlockSizeIn4K;
        startParameters.NumberBlocks = NumberBlocks;
        startParameters.TotalNumberIO = TotalNumberIO;
        startParameters.ForegroundQueueDepth = ForegroundQueueDepth;
        startParameters.BackgroundQueueDepth = BackgroundQueueDepth;

        readTest->StartIt(&startParameters, NULL, sync);

        do
        {
            status = sync.WaitForCompletion(10*1000);
            printf("%d IO Started %d IO Completed\n", readTest->_NumberReadsStarted, readTest->_NumberReadsCompleted);
        } while (status == STATUS_IO_TIMEOUT);

        if (! NT_SUCCESS(status))
        {
            printf("Write Test failed with Status %x\n", status);
            return(status);
        }

        ticks = (float)(readTest->_EndTick - readTest->_StartTick);
    }

    float fBlockSize = (float)(BlockSizeIn4K*0x1000);
    float fTotalNumberIO = (float)TotalNumberIO;
    float totalNumberBytes = fBlockSize * fTotalNumberIO;
    float seconds = ticks / 1000;
    float bytesPerSecond = (totalNumberBytes / seconds);
    float megaBytesPerSecond = bytesPerSecond / (0x100000);
    float numberIOs = (float)TotalNumberIO;
    float ioPerSecond = (float)numberIOs / seconds;
    
    printf("\n");
    printf("%f BlockSize, %f TotalNumberIo\n", fBlockSize, fTotalNumberIO);
    printf("%f bytes in %f seconds\n", totalNumberBytes, seconds);
    printf("%f bytes or %f MBytes per second\n", bytesPerSecond, megaBytesPerSecond);
    printf("    %f IO/sec\n", ioPerSecond);

    IoPerSecond = ioPerSecond;
    MBPerSecond = megaBytesPerSecond;

    return(STATUS_SUCCESS);
}


#if !defined(PLATFORM_UNIX)         // TODO: Port this code
/*
<?xml version="1.0" encoding="utf-8"?>
<PerformanceResults>
  <PerformanceResult>
    <Context>
      <Environment Name="MachineName" Value="DANVER2" />
      <Parameter Name="BlockSize" Value="1" />
      <Parameter Name="ForegroundQueueSIze" Value="32" />
      <Parameter Name="BackgroundQueueSize" Value="0" />
    </Context>
    <Measurements>
      <Measurement Name="RawDiskPerformance">
          <Value Metric="MBPerSecond" Value="3421.16921758783" When="2/13/2015 2:44:04 AM" />
      </Measurement>
    </Measurements>
  </PerformanceResult>
</PerformanceResults>
 */

NTSTATUS AddStringAttribute(
    __in KIMutableDomNode::SPtr dom,
    __in LPCWCHAR Name,
    __in LPCWCHAR Value
    )
{
    NTSTATUS status;

    KVariant value;
    value = KVariant::Create(Value, KtlSystem::GlobalNonPagedAllocator());
    status = value.Status();

    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    KIDomNode::QName name(Name);
    status = dom->SetAttribute(name, value);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS AddULONGAttribute(
    __in KIMutableDomNode::SPtr dom,
    __in LPCWCHAR Name,
    __in ULONG Value
    )
{
    NTSTATUS status;
    KVariant value;
    WCHAR str[MAX_PATH];
    HRESULT hr;

    hr = StringCchPrintf(str, MAX_PATH, L"%d", Value);

    value = KVariant::Create(str, KtlSystem::GlobalNonPagedAllocator());
    status = value.Status();

    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    KIDomNode::QName name(Name);
    status = dom->SetAttribute(name, value);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS AddFloatAttribute(
    __in KIMutableDomNode::SPtr dom,
    __in LPCWCHAR Name,
    __in float Value
    )
{
    NTSTATUS status;
    KVariant value;
    WCHAR str[MAX_PATH];
    HRESULT hr;
    
    hr = StringCchPrintf(str, MAX_PATH, L"%f", Value);

    value = KVariant::Create(str, KtlSystem::GlobalNonPagedAllocator());
    status = value.Status();

    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    KIDomNode::QName name(Name);
    status = dom->SetAttribute(name, value);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
PrintResults(
    __in PWCHAR ResultsXml,
    __in PWCHAR TestName,
    __in ULONG ResultCount,
    __in ULONG* BlockSizes,
    __in_ecount(ResultCount) float* MBPerSecond
    )
{
    NTSTATUS status;
    ULONG error;
    HRESULT hr;
    KIMutableDomNode::SPtr domRoot;

    // TODO: include all parameters for the run

    status = KDom::CreateEmpty(domRoot, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    KIMutableDomNode::QName name(L"PerformanceResults");
    status = domRoot->SetName(name);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    for (ULONG i = 0; i < ResultCount; i++)
    {
        KIMutableDomNode::SPtr performanceResult;
        KIDomNode::QName name1(L"PerformanceResult");
        status = domRoot->AddChild(name1, performanceResult);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr context;
        KIDomNode::QName name2(L"Context");
        status = performanceResult->AddChild(name2, context);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr environment;
        KIDomNode::QName name3(L"Environment");
        status = context->AddChild(name3, environment);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddStringAttribute(environment, L"Name", L"MachineName");
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        BOOL b;
        WCHAR computerName[MAX_PATH];
        ULONG computerNameLen = MAX_PATH;
        b = GetComputerName(computerName, &computerNameLen);
        if (!b)
        {
            error = GetLastError();
            return(error);
        }

        status = AddStringAttribute(environment, L"Value", computerName);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr parameter1;;
        KIDomNode::QName name4(L"Parameter");
        status = context->AddChild(name4, parameter1);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddStringAttribute(parameter1, L"Name", L"BlockSizeInKB");
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddULONGAttribute(parameter1, L"Value", BlockSizes[i] * 4);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr measurements;
        KIDomNode::QName name5(L"Measurements");
        status = performanceResult->AddChild(name5, measurements);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr measurement;
        KIDomNode::QName name6(L"Measurement");
        status = measurements->AddChild(name6, measurement);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddStringAttribute(measurement, L"Name", TestName);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr value;
        KIDomNode::QName name7(L"Value");
        status = measurement->AddChild(name7, value);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddStringAttribute(value, L"Metric", L"MBPerSecond");
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddFloatAttribute(value, L"Value", MBPerSecond[i]);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        SYSTEMTIME systemTime;
        WCHAR time[MAX_PATH];
        GetSystemTime(&systemTime);
        hr = StringCchPrintf(time, MAX_PATH, L"%02d/%02d%/%d %02d:%02d:%02d",
                            systemTime.wMonth, systemTime.wDay, systemTime.wYear,
                            systemTime.wHour, systemTime.wMinute, systemTime.wSecond);

        status = AddStringAttribute(value, L"When", time); 
        if (!NT_SUCCESS(status))
        {
            return(status);
        }
    }

    KString::SPtr domKString;
    status = KDom::ToString(domRoot, KtlSystem::GlobalNonPagedAllocator(), domKString);

    HANDLE file;

    file = CreateFile(ResultsXml, GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == NULL)
    {
        error = GetLastError();
        return(error);
    }

    LPCWSTR domString = (LPCWSTR)*domKString;
    ULONG bytesWritten;

    error = WriteFile(file, domString, (ULONG)(wcslen(domString) * sizeof(WCHAR)), &bytesWritten, NULL);
    CloseHandle(file);

    if (error != ERROR_SUCCESS)
    {
        return(error);
    }

    return(STATUS_SUCCESS);
}
#endif

NTSTATUS
KBlockFileTestX(
    __in int argc, 
    __in WCHAR* args[]
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KBlockFile::SPtr file;
    TestParameters testParameters;

    if (argc == 1)
    {
        Usage();
        return(STATUS_INVALID_PARAMETER);
    }

    
    //
    // Disable tracing for perf
    //
//    EventRegisterMicrosoft_Windows_KTL();

    status = testParameters.Parse(argc, args);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    //
    // Number of bytes written for each test is constant so we scale the number of blocks
    // based on the block size
    //
    ULONG automationBlockSizes[] = { 1, 4, 16, 32, 64, 128, 256, 512, 1024 };
    ULONG numberOfBlocksMultiple[] = { 1024, 512, 256, 128, 64, 32, 16, 4, 1 };
    const ULONG automationBlockCount = (sizeof(automationBlockSizes) / sizeof(ULONG));

    printf("\n");
    printf("Running diskperftool with the following:\n");
    printf("    TestOperation: %d\n", testParameters._TestOperation);
    printf("    TestAccess: %d\n", testParameters._TestAccess);
    printf("    WriteThrough: %ws\n", testParameters._WriteThrough ? L"TRUE" : L"FALSE");
    printf("    Sparse: %ws\n", testParameters._Sparse ? L"TRUE" : L"FALSE");
    printf("    TotalNumberIO: %d\n", testParameters._TotalNumberIO);
    printf("    Filename: %ws\n", testParameters._Filename);
    if (testParameters._AutomatedMode)
    {
        printf("    BlockSizeIn4K: ");
        for (ULONG i = 0; i < automationBlockCount; i++)
        {
            printf("%d, ", automationBlockSizes[i]);
        }
        printf("\n");
        printf("    Results XML: %ws\n", testParameters._ResultsXML);
    }
    else 
    {
        printf("    BlockSizeIn4K: %d\n", testParameters._BlockSizeIn4K);
    }
    printf("    NumberBlocks: %d\n", testParameters._NumberBlocks);
    printf("    ForegroundQueueDepth: %d\n", testParameters._ForegroundQueueDepth);
    printf("    BackgroundQueueDepth: %d\n", testParameters._BackgroundQueueDepth);
    printf("\n");

    //
    // Test file needs to be created to account for the largest run
    //
    ULONG blockSizeIn4K;
    if (testParameters._AutomatedMode)
    {
        blockSizeIn4K = 1024;
    }
    else 
    {
        blockSizeIn4K = testParameters._BlockSizeIn4K;
    }

    printf("Creating test file %d blocks %d bytes per block\n", testParameters._NumberBlocks, (blockSizeIn4K * (4 * 1024)));
    status = CreateTestFile(file, testParameters._Filename,
                                  blockSizeIn4K, testParameters._NumberBlocks, testParameters._WriteThrough, testParameters._Sparse);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    printf("\n");


    if (testParameters._AutomatedMode)
    {
        float ioPerSecond[automationBlockCount];
        float mbPerSecond[automationBlockCount];

        for (ULONG i = 0; i < automationBlockCount; i++)
        {
            ULONG numberBlocks = testParameters._NumberBlocks * numberOfBlocksMultiple[i];
            ULONG totalNumberIO = testParameters._TotalNumberIO * numberOfBlocksMultiple[i];
            blockSizeIn4K = automationBlockSizes[i];

            printf("Starting test %d blocks %d total IO %d bytes per block....\n", numberBlocks, totalNumberIO, blockSizeIn4K * 4096);
            status = PerformTest(file, testParameters._TestOperation, testParameters._TestAccess,
                blockSizeIn4K, numberBlocks,
                totalNumberIO,
                testParameters._ForegroundQueueDepth,
                testParameters._BackgroundQueueDepth,
                ioPerSecond[i],
                mbPerSecond[i]);
            printf("\n");

            if (!NT_SUCCESS(status))
            {
                return(status);
            }
        }
#if !defined(PLATFORM_UNIX)      // TODO: Port this code
        PrintResults(testParameters._ResultsXML, L"RawDiskPerformance", automationBlockCount, automationBlockSizes, mbPerSecond);
#endif
    }
    else 
    {
        float ioPerSecond, mbPerSecond;

        printf("Starting test %d bytes per block....\n", (testParameters._BlockSizeIn4K * 4096));
        status = PerformTest(file, testParameters._TestOperation, testParameters._TestAccess,
            testParameters._BlockSizeIn4K, testParameters._NumberBlocks,
            testParameters._TotalNumberIO,
            testParameters._ForegroundQueueDepth,
            testParameters._BackgroundQueueDepth,
            ioPerSecond,
            mbPerSecond);

        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        // TODO: Dump results to XML ??
    }

//    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}

//
// This test is useful for manually testing for handle leaks. To see
// the open handles for this process on linux you would type
//    sudo lsof -c D 
VOID OpenCloseTest()
{
    NTSTATUS status;
    KBlockFile::SPtr file;
    KSynchronizer sync;
    KWString pathName(KtlSystem::GlobalNonPagedAllocator());

    pathName = L"/tmp/foo";
    do
    {
        KNt::Sleep(250);
        status = KBlockFile::Create(pathName,
                                    TRUE,
                                    KBlockFile::eCreateAlways,
                                    file,
                                    sync,
                                    NULL,
                                    KtlSystem::GlobalNonPagedAllocator());
        if (! NT_SUCCESS(status))
        {
            printf("1: %x\n", status);
            return;
        }
        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            printf("2: %x\n", status);
            return;
        }

        KNt::Sleep(500);
        
        file->Close();
        file = nullptr;
    } while(TRUE);
}

NTSTATUS
KBlockFileTest(
    __in int argc, 
    __in WCHAR* args[]
    )
{
    NTSTATUS status;

    status = KtlSystem::Initialize();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Enable this for manual testing
//  OpenCloseTest();
    
    status = KBlockFileTestX(argc, args);

    KtlSystem::Shutdown();

    return status;
}


#if !defined(PLATFORM_UNIX)
int
wmain(int argc, WCHAR* args[])
{
    return RtlNtStatusToDosError(KBlockFileTest(argc, args));
}
#else
#include <vector>
int main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs);
    return RtlNtStatusToDosError(KBlockFileTest(argc, args));   
}
#endif


