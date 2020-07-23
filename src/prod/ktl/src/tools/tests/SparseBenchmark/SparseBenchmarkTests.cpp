/*++

Copyright (c) Microsoft Corporation

Module Name:

    SparseBenchmarkTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in SparseBenchmarkTests.h.
    2. Add an entry to the gs_KuTestCases table in SparseBenchmarkTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "SparseBenchmarkTests.h"
#include <CommandLineParser.h>


#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif



/*
<?xml version="1.0" encoding="utf-8"?>
<PerformanceResults>
  <PerformanceResult>
    <Context>
      <Parameter Name="NonPageAligedAsyncCount" Value="1" />
      <Parameter Name="NonPageAligedAllocCount" Value="1" />
      <Parameter Name="PageAligedAsyncCount" Value="1" />
      <Parameter Name="PageAligedAllocCount" Value="1" />
    </Context>
    <Measurements>
      <Measurement Name="SparseAllocPerformance">
          <Value Metric="Ticks" Value="3421" />
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
    status = dom->SetAttribute(KIDomNode::QName(Name), value);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}

template<typename T>
NTSTATUS AddAttribute(
    __in KIMutableDomNode::SPtr dom,
    __in LPCWCHAR Name,
    __in T Value
    )
{
    NTSTATUS status;
    KVariant value(Value);
    KString::SPtr string;

    value.ToString(KtlSystem::GlobalNonPagedAllocator(), string);
    string->SetNullTerminator();
    LPCWSTR str = *string;
    KVariant valueString = KVariant::Create(str, KtlSystem::GlobalNonPagedAllocator());
    status = dom->SetAttribute(KIDomNode::QName(Name), valueString);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS AddULONGLine(
    __in KIMutableDomNode::SPtr context,
    __in LPCWSTR ChildName,
    __in LPCWSTR Attribute1Name,
    __in LPCWSTR Attribute1,
    __in LPCWSTR Attribute2Name,
    __in ULONG Attribute2
                     )
{
    NTSTATUS status;
    KIMutableDomNode::SPtr parameter1;
    
    status = context->AddChild(KIDomNode::QName(ChildName), parameter1);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = AddStringAttribute(parameter1, Attribute1Name, Attribute1);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = AddAttribute(parameter1, Attribute2Name, Attribute2);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}
 


NTSTATUS AddULONGLONGLine(
    __in KIMutableDomNode::SPtr context,
    __in LPCWSTR ChildName,
    __in LPCWSTR Attribute1Name,
    __in LPCWSTR Attribute1,
    __in LPCWSTR Attribute2Name,
    __in ULONGLONG Attribute2
                     )
{
    NTSTATUS status;
    KIMutableDomNode::SPtr parameter1;

    status = context->AddChild(KIDomNode::QName(ChildName), parameter1);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = AddStringAttribute(parameter1, Attribute1Name, Attribute1);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = AddAttribute(parameter1, Attribute2Name, Attribute2);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}
 
typedef struct
{
    ULONG OpenFilesCount;
    ULONG NumberWriteCount;
    ULONG WriteSize;
    ULONGLONG Ticks;
    PWCHAR TestName;
} RESULT;

NTSTATUS
PrintResults(
    __in PWCHAR ResultsXml,
    __in ULONG ResultCount,
    __in_ecount(ResultCount) RESULT* Results
    )
{
    NTSTATUS status;
    KIMutableDomNode::SPtr domRoot;

    status = KDom::CreateEmpty(domRoot, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = domRoot->SetName(KIMutableDomNode::QName(L"PerformanceResults"));
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    for (ULONG i = 0; i < ResultCount; i++)
    {
        KIMutableDomNode::SPtr performanceResult;
        status = domRoot->AddChild(KIDomNode::QName(L"PerformanceResult"), performanceResult);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr context;
        status = performanceResult->AddChild(KIDomNode::QName(L"Context"), context);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr parameter1;
        status = context->AddChild(KIDomNode::QName(L"Parameter"), parameter1);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddULONGLine(context, L"Parameter", L"Name", L"OpenFilesCount", L"Value", Results[i].OpenFilesCount);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddULONGLine(context, L"Parameter", L"Name", L"NumberWriteCount", L"Value", Results[i].NumberWriteCount);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddULONGLine(context, L"Parameter", L"Name", L"WriteSize", L"Value", Results[i].WriteSize);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }


        KIMutableDomNode::SPtr measurements;
        status = performanceResult->AddChild(KIDomNode::QName(L"Measurements"), measurements);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        KIMutableDomNode::SPtr measurement;
        status = measurements->AddChild(KIDomNode::QName(L"Measurement"), measurement);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddStringAttribute(measurement, L"Name", Results[i].TestName);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = AddULONGLONGLine(measurement, L"Value", L"Metric", L"Ticks", L"Value", Results[i].Ticks);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }
    }

    //
    // Write the results XML to a file
    //
    KBlockFile::SPtr file;
    KIoBuffer::SPtr writeBuffer;
    PVOID p;
    KSynchronizer sync;
    
    KString::SPtr domKString;
    status = KDom::ToString(domRoot, KtlSystem::GlobalNonPagedAllocator(), domKString);
    KInvariant(NT_SUCCESS(status));

    LPCWSTR domString = (LPCWSTR)*domKString;
    ULONG length = (ULONG)(wcslen(domString) * sizeof(WCHAR));
    ULONG writeBufferLength = (length + (0x1000-1)) & ~(0x1000-1);

    status = KIoBuffer::CreateSimple( writeBufferLength,
                                      writeBuffer,
                                      p,
                                      KtlSystem::GlobalNonPagedAllocator(),
                                      KTL_TAG_TEST);
    KInvariant(NT_SUCCESS(status));
    
    memset(p, 0, writeBufferLength);
    KMemCpySafe((PWCHAR)p, writeBufferLength, domString, length);

    KWString resultsXml(KtlSystem::GlobalNonPagedAllocator(), ResultsXml);
    status = resultsXml.Status();
    KInvariant(NT_SUCCESS(status));
    
    status = KBlockFile::Create(resultsXml,
                                FALSE,
                                KBlockFile::eCreateAlways,
                                file,
                                sync,
                                NULL,
                                KtlSystem::GlobalNonPagedAllocator());
    KInvariant(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));


    status = file->SetFileSize((ULONGLONG)writeBufferLength,
                               sync);
    KInvariant(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));
        

    status = file->Transfer(KBlockFile::eForeground,
                            KBlockFile::eWrite,
                            0,
                            *writeBuffer,
                            sync);
                   
    KInvariant(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    return(STATUS_SUCCESS);
}

class DoWrites : public KAsyncContextBase
{
    K_FORCE_SHARED(DoWrites);

    public:
        static NTSTATUS Create(__out DoWrites::SPtr& Context,
                                __in KAllocator& Allocator,
                                __in ULONG AllocationTag);
        
        VOID SetIt(
            __in KBlockFile& File,                     
            __in ULONG WriteCount,
            __in KIoBuffer& Buffer
                    );
        
        VOID StartIt(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr    
                    );

    protected:
        
        VOID
        OnStart() override;

        VOID FSM(__in NTSTATUS Status);

        VOID Completion(
            __in_opt KAsyncContextBase* const,
            __in KAsyncContextBase& Async
            );
        
    private:
        KBlockFile::SPtr _File;
        KIoBuffer::SPtr _Buffer;
        ULONG _WriteCount;
};

DoWrites::DoWrites()
{
}

DoWrites::~DoWrites()
{
}

NTSTATUS DoWrites::Create(__out DoWrites::SPtr& Context,
                                __in KAllocator& Allocator,
                                __in ULONG AllocationTag)
{
    NTSTATUS status;
    DoWrites::SPtr context;

    context = _new(AllocationTag, Allocator) DoWrites();
    if (! context)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context;
    
    return(status);
}

VOID DoWrites::FSM(
    __in NTSTATUS Status
    )
{
    if (! NT_SUCCESS(Status))
    {
        Complete(Status);
        return;
    }

    if (_WriteCount == 0)
    {
        Complete(STATUS_SUCCESS);
        return;
    }

    KAsyncContextBase::CompletionCallback completion(this, &DoWrites::Completion);
    
    Status = _File->Transfer(KBlockFile::eForeground,
                             KBlockFile::eWrite,
                             _WriteCount * _Buffer->QuerySize(),
                             *_Buffer,
                             completion,
                             this,
                             NULL);
    
    if (! NT_SUCCESS(Status))
    {
        Complete(Status);
        return;
    }

    _WriteCount--;
}

VOID DoWrites::Completion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    FSM(Async.Status());
}


VOID DoWrites::OnStart()
{
    FSM(STATUS_SUCCESS);
}

VOID DoWrites::SetIt(
    __in KBlockFile& File,                     
    __in ULONG WriteCount,
    __in KIoBuffer& Buffer
    )
{
    _File = &File;
    _WriteCount = WriteCount;
    _Buffer = &Buffer;
}

VOID DoWrites::StartIt(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr    
    )
{
    Start(ParentAsyncContext, CallbackPtr);
}

static const ULONG Variations = 1;
static const PWCHAR VariationNames[Variations] = { L"Sparse" };

static const ULONG OpenFilesCount[Variations] = {64};
static const ULONG NumberWriteCount[Variations] = {2048};
static const ULONG WriteSize[Variations] = {1048576};

NTSTATUS Test1()
{
    NTSTATUS status = STATUS_SUCCESS;
    RESULT results[Variations];

    for (ULONG v = 0; v < Variations; v++)
    {
        ULONG count = OpenFilesCount[v];
        KArray<DoWrites::SPtr> DoWrites(KtlSystem::GlobalNonPagedAllocator());
        KSynchronizer** sync;
        BOOLEAN b;
        KSynchronizer ssync;

        KTestPrintf("SparseBenchmark Variation %d\n", v);

        status = DoWrites.Reserve(count);
        KInvariant(NT_SUCCESS(status));

        b = DoWrites.SetCount(count);
        KInvariant(b);

        sync = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) KSynchronizer*[count];
        KInvariant(sync != NULL);


        KIoBuffer::SPtr ioBuffer;
        PVOID p;
        status = KIoBuffer::CreateSimple(WriteSize[v], ioBuffer, p, KtlSystem::GlobalNonPagedAllocator());
        KInvariant(NT_SUCCESS(status));
        
        for (ULONG i = 0; i < count; i++)
        {
            sync[i] = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) KSynchronizer();
            KInvariant(sync[i] != NULL);

            status = DoWrites::Create(DoWrites[i], KtlSystem::GlobalNonPagedAllocator(), 72);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("DoWrites::Create(%d) failed %x\n", i, status);
                return(status);
            }

            KBlockFile::SPtr file;
            KWString filename(KtlSystem::GlobalNonPagedAllocator());
            KGuid guid;

            filename = L"\\??\\e:\\";
            guid.CreateNew();
            filename += guid;

            status = filename.Status();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("Filename(%d) failed %x\n", i, status);
                return(status);             
            }
            
            status = KBlockFile::CreateSparseFile(filename,
                                                  TRUE,
                                                  KBlockFile::eCreateAlways,
                                                  file,
                                                  ssync,
                                                  NULL,
                                                  KtlSystem::GlobalNonPagedAllocator());
            status = ssync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("KBlockFile::CreateSparseFile(%d) failed %x\n", i, status);
                return(status);
            }

            file->SetFileSize(NumberWriteCount[v] * ioBuffer->QuerySize(), ssync, NULL);
            status = ssync.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("KBlockFile::SetFileSize(%d) failed %x\n", i, status);
                return(status);
            }

            DoWrites[i]->SetIt(*file, NumberWriteCount[v], *ioBuffer);          
        }

        ULONGLONG tickStart, tickEnd, totalTicks;

        tickStart = KNt::GetTickCount64();

        for (ULONG i = 0; i < count; i++)
        {
            DoWrites[i]->StartIt(NULL,
                                 *(sync[i]));
        }

        for (ULONG i = 0; i < count; i++)
        {
            status = sync[i]->WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("WaitForCompletion(%d) failed %x\n", i, status);
                return(status);
            }
        }

        tickEnd = KNt::GetTickCount64();

        totalTicks = tickEnd - tickStart;

#if !defined(PLATFORM_UNIX)
        KTestPrintf("%ws: Total ticks %I64d, %I64d, %I64d\n", VariationNames[v], tickStart, tickEnd, totalTicks);
#else
        KTestPrintf("%s: Total ticks %I64d, %I64d, %I64d\n", Utf16To8(VariationNames[v]).c_str(), tickStart, tickEnd, totalTicks);
#endif

        results[v].TestName = VariationNames[v];
        results[v].OpenFilesCount = OpenFilesCount[v];
        results[v].NumberWriteCount = NumberWriteCount[v];
        results[v].WriteSize = WriteSize[v];
        results[v].Ticks = totalTicks;
        
        for (ULONG i = 0; i < count; i++)
        {
            _delete(sync[i]);
        }
        _delete(sync);
        
    }
    
    PrintResults(L"\\??\\c:\\SparseBenchmarkResults.xml",
                Variations,
                results);


    return(status);
}


NTSTATUS TestSequence()
{
    NTSTATUS Result = 0;
    LONGLONG InitialAllocations = KAllocatorSupport::gs_AllocsRemaining;

    KTestPrintf("Starting TestSequence...\n");

    Result = Test1();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Test1() failure\n");
        return Result;
    }

    // Sleep to give time for asyncs to be cleaned up before allocation
    // check. It requires a context switch and a little time on UP machines.
    KNt::Sleep(500);

    LONGLONG FinalAllocations = KAllocatorSupport::gs_AllocsRemaining;
    if (FinalAllocations != InitialAllocations)
    {
        KTestPrintf("Outstanding allocations.  Expected: %d.  Actual: %d\n", InitialAllocations, FinalAllocations);
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }

    KTestPrintf("SUCCESS\n");
    return STATUS_SUCCESS;
}


NTSTATUS
SparseBenchmarkTest(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif

    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting ProcessAffinityTest test");

    KtlSystem* system = nullptr;
    NTSTATUS Result = KtlSystem::Initialize(&system);
    KFatal(NT_SUCCESS(Result));

    system->SetStrictAllocationChecks(TRUE);

    Result = TestSequence();

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return Result;
}

#if CONSOLE_TEST
int
main(int argc, WCHAR* args[])
{
    return SparseBenchmarkTest(argc, args);
}
#endif
