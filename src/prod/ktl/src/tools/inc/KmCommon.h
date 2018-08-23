#pragma once
#include <ktl.h>

#if KTL_USER_MODE
void KTestPrintf(const char *Fmt, ...);
#else
#define KTestPrintf(...) KDbgPrintf(__VA_ARGS__)
#endif

typedef
NTSTATUS
(*KU_TEST_CASE)(
    int argc, WCHAR* args[]
    );

//
// An entry to describe a unit test case.
//

typedef struct _KU_TEST_ENTRY {
    PCWSTR          TestCaseName;
    KU_TEST_CASE    TestRoutine;
    PCWSTR          Categories;
    PCWSTR          Help;
} KU_TEST_ENTRY, *PKU_TEST_ENTRY;

typedef const KU_TEST_ENTRY *   PCKU_TEST_ENTRY;

extern const KU_TEST_ENTRY gs_KuTestCases[];
extern const ULONG gs_KuTestCasesSize;

#ifdef KM_LIBRARY
#define ARRAY_SIZE(x)  x ## Size
#else
#define ARRAY_SIZE ARRAYSIZE
#endif

namespace ktl
{
    namespace test
    {
        namespace file
        {
            class TestContext : public KAsyncContextBase {

                K_FORCE_SHARED(TestContext);

            public:

                static NTSTATUS
                Create(__out TestContext::SPtr& Context);

                VOID
                Completion(
                    __in KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async);

                NTSTATUS
                Wait();

            private:

                KEvent _Event;
                NTSTATUS _Status;
            };

            NTSTATUS
            CreateTestFile(
                __in BOOLEAN SparseFile,
                __in BOOLEAN IsWriteThrough,
                __in BOOLEAN UseFileSystemOnly,
                __in ULONGLONG FileSize,
                __in ULONG QueueDepth,
                __out KWString& TestDirName,
                __out KWString& TestFileName,
                __out KBlockFile::SPtr& File,
                __in_opt LPCWSTR partialFileName = nullptr);
        }
    }
}

