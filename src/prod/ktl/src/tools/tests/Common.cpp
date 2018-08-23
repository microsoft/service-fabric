#pragma once
#include <ktl.h>
#include <ktrace.h>

#if KTL_USER_MODE
#include <KmUser.h>
#include <ktlevents.um.h>
#else
#include <KmUnit.h>
#include <ktlevents.km.h>
#endif

using namespace ktl::test::file;

ktl::test::file::TestContext::~TestContext(
)
{
}

NOFAIL
ktl::test::file::TestContext::TestContext(
)
{
    _Status = STATUS_SUCCESS;
}

NTSTATUS
ktl::test::file::TestContext::Create(
    __out TestContext::SPtr& Context
)
{
    TestContext* context;

    context = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) TestContext;
    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Context = context;

    return STATUS_SUCCESS;
}

VOID
ktl::test::file::TestContext::Completion(
    __in KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
)
{
    UNREFERENCED_PARAMETER(ParentAsync);

    _Status = Async.Status();
    _Event.SetEvent();
}

NTSTATUS
ktl::test::file::TestContext::Wait(
)
{
    _Event.WaitUntilSet();
    _Event.ResetEvent();
    return _Status;
}

NTSTATUS
CreateTestFileOnVolume(
    __in BOOLEAN SparseFile,
    __in BOOLEAN IsWriteThrough,
    __in BOOLEAN UseFileSystemOnly,
	__in ULONG QueueDepth,
    __in TestContext& Context,
    __in KAsyncContextBase::CompletionCallback const & Completion,
    __in GUID const & VolumeGuid,
    __in ULONGLONG const & FileSize,
    __out KWString& TestDirName,
    __out KWString& TestFileName,
    __out KBlockFile::SPtr& File,
    __in_opt LPCWSTR partialFileName = nullptr
)
{
    NTSTATUS status = STATUS_SUCCESS;

    KWString volumeName(KtlSystem::GlobalNonPagedAllocator());
    KWString testDirName(KtlSystem::GlobalNonPagedAllocator());
    KWString dirName(KtlSystem::GlobalNonPagedAllocator());
    KWString testFileName(KtlSystem::GlobalNonPagedAllocator());
    KWString fullyQualifiedTestFileName(KtlSystem::GlobalNonPagedAllocator());

    KBlockFile::CreateOptions createOptions;

	if (UseFileSystemOnly)
	{
		createOptions = (KBlockFile::CreateOptions)((ULONG)KBlockFile::eShareRead + (ULONG)KBlockFile::eInheritFileSecurity + (ULONG)KBlockFile::eAlwaysUseFileSystem);
	} else {
		createOptions = (KBlockFile::CreateOptions)((ULONG)KBlockFile::eShareRead + (ULONG)KBlockFile::eInheritFileSecurity);
	}
	
#if !defined(PLATFORM_UNIX)    
    status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(VolumeGuid, volumeName);
#else
    volumeName = L"/tmp";
    status = volumeName.Status();
#endif
    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create volume name %x\n", status);
        goto Finish;
    }    

    testDirName = L"KBlockFileTestDirectory";
    status = testDirName.Status();
    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create test dir name %x\n", status);
        goto Finish;
    }

    status = KVolumeNamespace::CreateFullyQualifiedName(volumeName, testDirName, dirName);
    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create dir name %x\n", status);
        goto Finish;
    }

    status = KVolumeNamespace::CreateDirectory(dirName, KtlSystem::GlobalNonPagedAllocator(), Completion);
    if (status == STATUS_PENDING) {
        status = Context.Wait();
    }
    if (status == STATUS_OBJECT_NAME_COLLISION)
    {
        KTestPrintf("test directory already exists\n");
    }
    else if (!NT_SUCCESS(status))
    {
        KTestPrintf("could not create directory %x\n", status);
        goto Finish;
    }

    testFileName = partialFileName != nullptr ? partialFileName : L"TestFile";
    status = testFileName.Status();
    if (! NT_SUCCESS(status)) {

        KTestPrintf("could not create test file name %x\n", status);
        goto Finish;
    }
    
    status = KVolumeNamespace::CreateFullyQualifiedName(dirName, testFileName, fullyQualifiedTestFileName);
    if (! NT_SUCCESS(status)) {
        KTestPrintf("could not create fully qualified test file name %x\n", status);
        goto Finish;
    }

    if (SparseFile)
    {
        status = KBlockFile::CreateSparseFile(
            fullyQualifiedTestFileName,
            IsWriteThrough,
            KBlockFile::eCreateAlways,
            createOptions,
			QueueDepth,
            File,
            Completion,
            NULL,
            KtlSystem::GlobalNonPagedAllocator());
    } else {
        status = KBlockFile::Create(
            fullyQualifiedTestFileName,
            IsWriteThrough,
            KBlockFile::eCreateAlways,
            createOptions,
			QueueDepth,
            File,
            Completion,
            NULL,
            KtlSystem::GlobalNonPagedAllocator());
    }

    if (status == STATUS_PENDING) {
        status = Context.Wait();
    }

    if (! NT_SUCCESS(status)) {
        KTestPrintf("could not create file %x\n", status);
        goto Finish;
    }

    status = File->SetFileSize(FileSize, Completion);

    if (status == STATUS_PENDING) {
        status = Context.Wait();
    }

    if (! NT_SUCCESS(status)) {
        KTestPrintf("set file size failed %x\n", status);
        goto Finish;
    }

Finish:

    if (NT_SUCCESS(status)) {
        TestDirName = dirName;
        status = TestDirName.Status();
        if (NT_SUCCESS(status)) {
            TestFileName = fullyQualifiedTestFileName;
            status = TestFileName.Status();
        }
    }

    if (! NT_SUCCESS(status)) {
        File = NULL;
    }

    return status;
}

NTSTATUS
ktl::test::file::CreateTestFile(
    __in BOOLEAN SparseFile,
    __in BOOLEAN IsWriteThrough,
    __in BOOLEAN UseFileSystemOnly,
    __in ULONGLONG FileSize,
    __in ULONG QueueDepth,
    __out KWString& TestDirName,
    __out KWString& TestFileName,
    __out KBlockFile::SPtr& File,
    __in_opt LPCWSTR partialFileName
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    TestContext::SPtr context;
    status = TestContext::Create(context);
    if (! NT_SUCCESS(status)) {
        KTestPrintf("Could not create test context. Status: %x\n", status);
        return status;
    }

    KAsyncContextBase::CompletionCallback completion;
    completion.Bind(context.RawPtr(), &TestContext::Completion);
    
#if !defined(PLATFORM_UNIX)
    //  First try to write to the D or C drive
    {
        KVolumeNamespace::VolumeInformationArray infoArray(KtlSystem::GlobalNonPagedAllocator());

        status = KVolumeNamespace::QueryVolumeListEx(infoArray, KtlSystem::GlobalNonPagedAllocator(), completion);

        if (status == STATUS_PENDING) {
            status = context->Wait();
        }

        if (! NT_SUCCESS(status)) {
            KTestPrintf("Could not query the volume ex list. Status: %x\n", status);
            return status;
        }

        GUID guid;

        if (KVolumeNamespace::QueryVolumeIdFromDriveLetter(infoArray, 'F', guid)) {
            status = CreateTestFileOnVolume(
                SparseFile,
                IsWriteThrough,
                UseFileSystemOnly,
				QueueDepth,
                *context,
                completion,
                guid,
                FileSize,
                TestDirName,
                TestFileName,
                File,
                partialFileName);

            if (status == STATUS_PENDING) {
                status = context->Wait();
            }

            if (NT_SUCCESS(status)) {
                return status;
            }

            if (status == STATUS_PRIVILEGE_NOT_HELD)
            {
                return status;
            }
            
            KTestPrintf("F: is not suitable for testing\n");
        }

        if (KVolumeNamespace::QueryVolumeIdFromDriveLetter(infoArray, 'D', guid)) {
            status = CreateTestFileOnVolume(
                SparseFile,
                IsWriteThrough,
                UseFileSystemOnly,
				QueueDepth,
                *context,
                completion,
                guid,
                FileSize,
                TestDirName,
                TestFileName,
                File,
                partialFileName);

            if (status == STATUS_PENDING) {
                status = context->Wait();
            }

            if (NT_SUCCESS(status)) {
                return status;
            }

            if (status == STATUS_PRIVILEGE_NOT_HELD)
            {
                return status;
            }
            
            KTestPrintf("D: is not suitable for testing\n");
        }

        if (KVolumeNamespace::QueryVolumeIdFromDriveLetter(infoArray, 'C', guid)) {
            status = CreateTestFileOnVolume(
                SparseFile,
                IsWriteThrough,
				UseFileSystemOnly,
				QueueDepth,
                *context,
                completion,
                guid,
                FileSize,
                TestDirName,
                TestFileName,
                File,
                partialFileName);

            if (status == STATUS_PENDING) {
                status = context->Wait();
            }

            if (NT_SUCCESS(status)) {
                return status;
            }

            if (status == STATUS_PRIVILEGE_NOT_HELD)
            {
                return status;
            }
            
            KTestPrintf("C: is not suitable for testing\n");
        }
    }

    //  If no F: or D: or C: then try all the stable-guid volumes until one can be written to
    {
        KVolumeNamespace::VolumeIdArray idArray(KtlSystem::GlobalNonPagedAllocator());

        status = KVolumeNamespace::QueryVolumeList(idArray, KtlSystem::GlobalNonPagedAllocator(), completion);

        if (status == STATUS_PENDING) {
            status = context->Wait();
        }

        if (! NT_SUCCESS(status)) {
            KTestPrintf("Could not query the volume list. Status: %x\n", status);
            return status;
        }

        for (ULONG i = 0; i < idArray.Count(); i++) {
            status = CreateTestFileOnVolume(
                SparseFile,
                IsWriteThrough,
                UseFileSystemOnly,
				QueueDepth,
                *context,
                completion,
                idArray[i],
                FileSize,
                TestDirName,
                TestFileName,
                File,
                partialFileName);

            if (status == STATUS_PENDING) {
                status = context->Wait();
            }

            if (NT_SUCCESS(status)) {
                return status;
            }
            
            if (status == STATUS_PRIVILEGE_NOT_HELD)
            {
                return status;
            }
            
        }
    }
#else
// {DAC0E64D-3315-4775-82C5-826393460CB0}
    static const GUID randomGuid = 
        { 0xdac0e64d, 0x3315, 0x4775, { 0x82, 0xc5, 0x82, 0x63, 0x93, 0x46, 0xc, 0xb0 } };
    
    status = CreateTestFileOnVolume(
        SparseFile,
        IsWriteThrough,
		UseFileSystemOnly,
		QueueDepth,
        *context,
        completion,
        randomGuid,
        FileSize,
        TestDirName,
        TestFileName,
        File,
        partialFileName);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (NT_SUCCESS(status)) {
        return status;
    }
    
#endif
    KTestPrintf("Unable to find any writeable volume to create a test file.\n");
    return STATUS_UNSUCCESSFUL;
}

