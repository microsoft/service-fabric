/*++

    (c) 2016 by Microsoft Corp. All Rights Reserved.

    KTpl.cpp

    Description:
      Kernel Template Library (KTL): C# TPL-Like Support Definitions

    History:
      richhas          13-JUN-2016         Initial version.

--*/

#include "ktl.h"
#include "KTpl.h"

#if defined(K_UseResumable)
#if KTL_USER_MODE

#pragma comment(lib, "Dbghelp.lib")


using namespace ktl;

//** Default ExceptionDebugInfoProvider implementation
class Exception::DefaultDebugInfoProvider : public ExceptionDebugInfoProvider
{
public:
	void AcquireLock() noexcept override
	{
		Exception::g_SymInit.Lock().Acquire();
	}

	void ReleaseLock() noexcept override
	{
		Exception::g_SymInit.Lock().Release();
	}

	// Assumes AcquireLock() was called
	BOOL UnsafeSymInitialize(_In_ HANDLE hProcess, _In_opt_ PCSTR UserSearchPath, _In_ BOOL fInvadeProcess) noexcept override
	{
		return ::SymInitialize(hProcess, UserSearchPath, fInvadeProcess);
	}

	BOOL UnsafeSymFromAddr(_In_ HANDLE hProcess, _In_ DWORD64 Address, _Out_opt_ PDWORD64 Displacement, _Inout_ PSYMBOL_INFO Symbol) noexcept override
	{
		return ::SymFromAddr(hProcess, Address, Displacement, Symbol);
	}

	// Assumes AcquireLock() was called
	BOOL UnsafeSymGetLineFromAddr64(_In_ HANDLE hProcess, _In_ DWORD64 qwAddr, _Out_ PDWORD pdwDisplacement, _Out_ PIMAGEHLP_LINE64 Line64) noexcept override
	{
		return ::SymGetLineFromAddr64(hProcess, qwAddr, pdwDisplacement, Line64);
	}
};


//* Exception support

KGlobalSpinLockStorage    Exception::g_SymInit;
volatile bool             Exception::g_SymbolsInited = false;
static_assert((sizeof(CancellationToken) == sizeof(CancellationTokenSource::SPtr)), "Unexpected size difference");

Exception::DefaultDebugInfoProvider defaultKExceptionDebugInfoProvider;
ExceptionDebugInfoProvider* Exception::g_InfoProvider = &((ExceptionDebugInfoProvider&)defaultKExceptionDebugInfoProvider);


void Exception::InitSymbols() noexcept
{
    if (!g_SymbolsInited)
    {
		g_SymbolsInited = true;

		ExceptionDebugInfoProvider*		provider = g_InfoProvider;

		provider->AcquireLock();
		KFinally([provider]() { provider->ReleaseLock(); });

		g_InfoProvider->UnsafeSymInitialize(GetCurrentProcess(), NULL, TRUE);
    }
}

void Exception::SetDebugInfoProvider(ExceptionDebugInfoProvider& NewProvider) noexcept
{
	ExceptionDebugInfoProvider*		currentProvider = nullptr;
	ExceptionDebugInfoProvider*		newProvider = &NewProvider;
	ExceptionDebugInfoProvider*		oldProvider = nullptr;

	do
	{
		currentProvider = g_InfoProvider;
		KInvariant(((ExceptionDebugInfoProvider*)&defaultKExceptionDebugInfoProvider) == currentProvider);

		oldProvider = (ExceptionDebugInfoProvider*)InterlockedCompareExchangePointer(
			(PVOID volatile *)&g_InfoProvider,
			newProvider,
			currentProvider);
	} while (oldProvider != currentProvider);

	Exception::g_SymbolsInited = false;
}

void Exception::ClearDebugInfoProvider() noexcept
{
	g_InfoProvider = &((ExceptionDebugInfoProvider&)defaultKExceptionDebugInfoProvider);
}

bool Exception::TryCopyStackTrace(
    KUniquePtr<StackTrace, HeapDeleter<StackTrace>>& Dest,
    KUniquePtr<StackTrace, HeapDeleter<StackTrace>> const& Src,
    USHORT Size) const noexcept
{
    Dest = (StackTrace*)(::HeapAlloc(::GetProcessHeap(), 0, Size * sizeof(void*)));
    if ((StackTrace*)Dest != nullptr)
    {
        // Have memory - copy and save length
        KMemCpySafe(&Dest->_stack[0], Size * sizeof(void*), &Src->_stack[0], Size * sizeof(void*));
        return true;
    }

    return false;
}

bool Exception::ToString(__out KDynStringA& Result, __in_z LPCSTR EolDelimator) noexcept
{
    Result.Clear();
    InitSymbols();

    HANDLE          process = GetCurrentProcess();
    const int       maxSymbolSize = 256;
    union
    {
        SYMBOL_INFO symbol;
        char        pad[sizeof(SYMBOL_INFO) + (maxSymbolSize * sizeof(char))];
    }               entry;

    entry.symbol.MaxNameLen = maxSymbolSize - 1;
    entry.symbol.SizeOfStruct = sizeof(SYMBOL_INFO);

    DWORD               lineDisplacement = 0;
    IMAGEHLP_LINE64     imageHelp; imageHelp.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    Exception           emptyEx;
    Exception*          currentExcp = this;

    {
		ExceptionDebugInfoProvider*		provider = g_InfoProvider;

		provider->AcquireLock();
		KFinally([provider]() { provider->ReleaseLock(); });
		
		// Sym* methods are not thread safe - Perf warning!
        while (currentExcp != nullptr)
        {
            DWORD64*            elements = (DWORD64*)&((StackTrace*)(currentExcp->_stack)->_stack)[0];

            if (!Result.Concat((currentExcp == this) ? "***** Exception *****" : "***** Nested Exception *****"))
            {
                return false;
            }

			if (!Result.Concat(EolDelimator))
			{
				return false;
			}

			for (int ix = 0; ix < currentExcp->_stackSize; ix++)
            {
                if (!provider->UnsafeSymFromAddr(process, elements[ix], 0, &entry.symbol))
                {
                    continue;
                }
                if (!provider->UnsafeSymGetLineFromAddr64(process, elements[ix], &lineDisplacement, &imageHelp))
                {
                    continue;
                }

                KStringViewA        funcName((LPCSTR)(&entry.symbol.Name[0]));
                if (funcName.LeftString(25).Compare("ktl::Exception::Exception") == 0)
                                               // 1234567890123456789012345
                {
                    // Drop noise
                    continue;
                }

                if (!Result.Concat(imageHelp.FileName))
                {
                    return false;
                }
                if (!Result.Concat("!"))
                {
                    return false;
                }
                if (!Result.Concat((LPCSTR)&entry.symbol.Name))
                {
                    return false;
                }
                if (!Result.Concat(" Line("))
                {
                    return false;
                }

                char        lineStr[25];
                snprintf(lineStr, sizeof(lineStr), "%u)%s", imageHelp.LineNumber, EolDelimator);
                if (!Result.Concat(lineStr))
                {
                    return false;
                }
            }

            if (!currentExcp->_innerExceptionPtr)
            {
                currentExcp = nullptr;
                continue;
            }

            try
            {
                rethrow_exception(currentExcp->_innerExceptionPtr);
            }
            catch (Exception& Ex)
            {
                emptyEx = Ex;
                currentExcp = &emptyEx;
            }
            catch (...)
            {
                KInvariant(false);          // All exception must derive from ktl::Exception
            }
        }
    }

    return true;
}

//* CancellationToken implementation
CancellationToken CancellationToken::None = {};

//* CancellationTokenSource implementation
NTSTATUS CancellationTokenSource::Create(__in KAllocator& Allocator, __in ULONG Tag, __out CancellationTokenSource::SPtr& Result) noexcept
{
    Result = _new(Tag, Allocator) CancellationTokenSource();
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(Result->Status()))
    {
        // null Result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (CancellationTokenSource::SPtr(Ktl::Move(Result)))->Status();
    }

    return STATUS_SUCCESS;
}

CancellationTokenSource::CancellationTokenSource() {}
CancellationTokenSource::~CancellationTokenSource() {}

void CancellationTokenSource::Cancel(NTSTATUS Reason) noexcept
{
    if (!_cancelWasSet)
    {
        // First caller thru #1 wins
        SetStatus(Reason);
        _cancelWasSet = true;   // #1
    }
}

bool CancellationTokenSource::GetIsCancellationRequested() const noexcept
{
    return _cancelWasSet;
}

CancellationToken CancellationTokenSource::GetToken() noexcept
{
    return CancellationToken(*this);
}

void CancellationTokenSource::ThrowIfCancellationRequested() const
{
    if (_cancelWasSet)
    {
        throw(ktl::Exception(Status()));
    }
}

NTSTATUS AwaitableCompletionSource<void>::Create(__in KAllocator& Allocator, __in ULONG Tag, __out KSharedPtr<AwaitableCompletionSource<void>>& Result) noexcept
{
    return AwaitableCompletionSourceBase::Create<void>(Allocator, Tag, Result);
}

AwaitableCompletionSource<void>::AwaitableCompletionSource()
    : AwaitableCompletionSourceBase()
{}

AwaitableCompletionSource<void>::~AwaitableCompletionSource()
{}

AwaitableCompletionSourceBase::AwaitableCompletionSourceBase()
    : _completionSignal(true)
{}

AwaitableCompletionSourceBase::~AwaitableCompletionSourceBase()
{}


#endif
#endif