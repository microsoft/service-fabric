/*++

    (c) 2016 by Microsoft Corp. All Rights Reserved.

    KTpl.h

    Description:
      Kernel Template Library (KTL): C# TPL-Like Support Definitions

    History:
      richhas          13-JUN-2016         Initial version.

--*/

#pragma once

#if defined(K_UseResumable)

namespace ktl
{
    using ::_delete;

    // Common User Mode Exceptions for ::ktl
    #if KTL_USER_MODE

    #include "dbghelp.h"
    #include <winbase.h>

	// Forward defs
	class ExceptionDebugInfoProvider;

	// Common KTL++ Exception base class definition
    class Exception : public KRTT
    {
        K_RUNTIME_TYPED(Exception);

    public:
        template <class T> struct HeapDeleter;

    private:
        struct StackTrace
        {
            #pragma warning(disable:4200) 
            void*       _stack[0];
            #pragma warning(default:4200) 
        };

        bool TryCopyStackTrace(
            KUniquePtr<StackTrace, HeapDeleter<StackTrace>>& Dest,
            KUniquePtr<StackTrace, HeapDeleter<StackTrace>> const& Src,
            USHORT Size) const noexcept;

        static void InitSymbols() noexcept;

        void CommonCtor() noexcept
        {
            // Now try and get a capture the stack trace back
            void*       stack[_maxStackItems];
            USHORT      size = ::CaptureStackBackTrace(1, _maxStackItems, &stack[0], nullptr);

            // Try and allocate heap space for _stack contents
            _stack = (StackTrace*)(::HeapAlloc(::GetProcessHeap(), 0, size * sizeof(void*)));
            if ((StackTrace*)_stack != nullptr)
            {
                // Have memory - copy and save length
                KMemCpySafe(&((StackTrace*)_stack)->_stack[0], size * sizeof(void*), &stack[0], size * sizeof(void*));
                static_assert((((_maxStackItems * sizeof(void*)) + sizeof(StackTrace)) < MAXINT), "Potential math overflow");   // keep ranges safe

                _stackSize = size;
            }
        }

        Exception() noexcept
        {
            _status = STATUS_SUCCESS;
            _stackSize = 0;
        }

	public:
		class DefaultDebugInfoProvider;

	private:
		friend DefaultDebugInfoProvider;
		
		NTSTATUS            _status;
        KUniquePtr<StackTrace, HeapDeleter<StackTrace>>   _stack;
        USHORT              _stackSize;
        std::exception_ptr  _innerExceptionPtr;

        static const int                _maxStackItems = 50;
        static KGlobalSpinLockStorage   g_SymInit;
        static volatile bool            g_SymbolsInited;
		static ExceptionDebugInfoProvider* g_InfoProvider;

    public:
        //* Global heap KUniquePtr<> Deleter
        template <class T>
        struct HeapDeleter
        {
            static void Delete(T * pointer)
            {
                if (pointer != nullptr)
                {
                    pointer->T::~T();
                    ::HeapFree(::GetProcessHeap(), 0, pointer);
                }
            }
        };

    public:
        template<typename _NestedT>
        Exception(NTSTATUS Status, _NestedT const& NestedException) noexcept
        {
            _status = Status;
            _stackSize = 0;
            _innerExceptionPtr = std::make_exception_ptr<_NestedT>(NestedException);
            CommonCtor();
        }

        Exception(NTSTATUS Status) noexcept
        {
            _status = Status;
            _stackSize = 0;
            _innerExceptionPtr = nullptr;
            CommonCtor();
        }

        Exception(Exception&& Right) noexcept
        {
            _status = Right._status;
            _stack = Ktl::Move(Right._stack);
            _stackSize = Right._stackSize; Right._stackSize = 0;
            _innerExceptionPtr = Ktl::Move(Right._innerExceptionPtr);
        }

        Exception(Exception const& Right) noexcept
        {
            _status = Right._status;
            _stackSize = (TryCopyStackTrace(_stack, Right._stack, Right._stackSize)) ? Right._stackSize : 0;
            _innerExceptionPtr = Right._innerExceptionPtr;
        }

        Exception& operator=(Exception&& Right) noexcept
        { 
            _status = Right._status;
            _stack = Ktl::Move(Right._stack);
            _stackSize = Right._stackSize;
            Right._stackSize = 0;
            _innerExceptionPtr = Ktl::Move(Right._innerExceptionPtr);
            return *this;
        }

        Exception& operator=(Exception const& Right) noexcept
        {
            _status = Right._status;
            _stackSize = (TryCopyStackTrace(_stack, Right._stack, Right._stackSize)) ? Right._stackSize : 0;
            _innerExceptionPtr = Right._innerExceptionPtr;
            return *this;
        }

        virtual ~Exception() noexcept {}

        NTSTATUS GetStatus() const noexcept { return _status; }

        // Utility to recreate an Exception instance from its Handle
        static Exception ToException(ktl::ExceptionHandle From)
        {
            try
            {
                rethrow_exception(From);
            }
            catch (Exception& Ex)
            {
                return Ex;
            }
            catch (...)
            {
                KInvariant(false);          // All exception must derive from ktl::Exception
            }
        }

        /// Returns null KUniquePtr if no inner exception OR OOM
        template<typename _ExcpT>
        KUniquePtr<_ExcpT, HeapDeleter<_ExcpT>> GetInnerException() const noexcept
        {
            if (!_innerExceptionPtr)
            {
                // Return nullptr
                return Ktl::Move(KUniquePtr<_ExcpT, HeapDeleter<_ExcpT>>());
            }

            try
            {
                rethrow_exception(_innerExceptionPtr);
            }
            catch (_ExcpT& Ex)
            {
                _ExcpT*  result = (_ExcpT*)::HeapAlloc(::GetProcessHeap(), 0, sizeof(_ExcpT));
                if (result != nullptr)
                {
                    new(result) _ExcpT(Ex);     // in-place
                }
                return Ktl::Move(KUniquePtr<_ExcpT, HeapDeleter<_ExcpT>>(result));
            }
            catch (...)
            {
                KInvariant(false);          // All exception must derive from ktl::Exception
            }
        }

        bool ToString(__out KDynStringA& Result, __in_z LPCSTR EolDelimator = "\n") noexcept;

		// NOTE: Vary restricted behavior: Once the default provider has been overriden via this method, only a call to
		//       ClearDebugInfoProvider() is allowed otherwise a process fault will occur. The purpose of this 
		//		 facility is to support very controlled overrides (and maybe reversals) to occur managed by some component
		//       having process-wide visability. That component must provide ordering between calls if needed.
		static void SetDebugInfoProvider(ExceptionDebugInfoProvider& NewProvider) noexcept;
		static void ClearDebugInfoProvider() noexcept;
    };

	// This interface is for a process wide wrapper over the dbghelp library //
	//
	//		By default Exception::DebugInfoProvider is set to a KTL++ provider which assumes minimal
	//		wrt other non-ktl components that would be using these dbghelp (non-thread safe) facilities.
	//		For those wishing to mix use of dbghelp, they must provide their own KExceptionDebugInfoProvider 
	//		implementation and set it via KException::SetDebugInfoProvider() and 
	//		KException::RestoreDebugInfoProvider().
	//
	class ExceptionDebugInfoProvider
	{
	public:
		virtual void AcquireLock() noexcept = 0;
		virtual void ReleaseLock() noexcept = 0;

		// Assumes AcquireLock() was called
		virtual BOOL UnsafeSymInitialize(_In_ HANDLE hProcess, _In_opt_ PCSTR UserSearchPath, _In_ BOOL fInvadeProcess) noexcept = 0;
		virtual BOOL UnsafeSymFromAddr(_In_ HANDLE hProcess, _In_ DWORD64 Address, _Out_opt_ PDWORD64 Displacement, _Inout_ PSYMBOL_INFO Symbol) noexcept = 0;
		virtual BOOL UnsafeSymGetLineFromAddr64(_In_ HANDLE hProcess, _In_ DWORD64 qwAddr, _Out_ PDWORD pdwDisplacement, _Out_ PIMAGEHLP_LINE64 Line64) noexcept = 0;
	};

    #endif


    //** AwaitableCompletionSource - A multi-Awaitable<> completion distributer
    //
    //  To support the notion that multiple async coroutines would like to suspend via co_wait
    //  until a common completion event is signalled. See C# TaskCompletionSource

    //* Common base class for AwaitableCompletionSource<void> and AwaitableCompletionSource<_RetT>

#if defined(PLATFORM_UNIX)
    template<typename _RetT> class AwaitableCompletionSource;
#endif
    class AwaitableCompletionSourceBase : public KShared<AwaitableCompletionSourceBase>, public KObject<AwaitableCompletionSourceBase>
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AwaitableCompletionSourceBase);

        //* Derivation support APIs and types
    protected:
        template<typename _RetT> friend class AwaitableCompletionSource;

        #pragma warning(disable:4201)
        union SetState
        {
            SHORT volatile   _Value;
            struct
            {
                bool    _ValueWasSet : 1;
                bool    _ExceptionWasSet : 1;
                bool    _CancelWasSet : 1;
            };

            SetState() { _Value = 0; }
            static __forceinline SetState MakeValueWasSet()      { SetState x; x._ValueWasSet = true; return x; }
            static __forceinline SetState MakeExceptionWasSet() { SetState x; x._ExceptionWasSet = true; return x; }
            static __forceinline SetState MakeCancelWasSet()     { SetState x; x._CancelWasSet = true; return x; }
        };
        #pragma warning(default:4201)

        //* Common instance state for all AwaitableCompletionSources
    protected:
        SetState            _setState;
        KAsyncEvent         _completionSignal;
        std::exception_ptr  _caughtExecption;

    protected:
        __forceinline bool TrySetState(SetState NewState) noexcept
        {
            return (InterlockedCompareExchange16(&_setState._Value, NewState._Value, 0) == 0) ? true : false;
        }

        __declspec(noinline) void ThrowIfExcpetionResult()
        {
            KInvariant(_completionSignal.IsSignaled());         // 06/09/2016
            if (_setState._ExceptionWasSet)
            {
                rethrow_exception(_caughtExecption);
            }
            if (_setState._CancelWasSet)
            {
                throw(ktl::Exception(STATUS_CANCELLED));
            }
        }

        //* Common factory
        template<typename _RetT>
        static NTSTATUS Create(__in KAllocator& Allocator, __in ULONG Tag, __out KSharedPtr<AwaitableCompletionSource<_RetT>>& Result) noexcept;
        
    public:
         //* Common public APIs 
        bool IsCompleted() const noexcept
        {
            return (_completionSignal.IsSignaled() == TRUE);
        }

        void SetException(const ktl::Exception& Exception)
        {
            if (TrySetState(SetState::MakeExceptionWasSet()) == false)
            {
                throw ktl::Exception(K_STATUS_API_CLOSED);
            }

            _caughtExecption = std::make_exception_ptr(Exception);
            _completionSignal.SetEvent();
        }

        bool TrySetException(const ktl::Exception& Exception) noexcept
        {
            if (TrySetState(SetState::MakeExceptionWasSet()) == false)
            {
                return false;
            }

            _caughtExecption = std::make_exception_ptr(Exception);
            _completionSignal.SetEvent();
            return true;
        }

        void SetCanceled()
        {
            if (TrySetState(SetState::MakeCancelWasSet()) == false)
            {
                throw ktl::Exception(K_STATUS_API_CLOSED);
            }

            _completionSignal.SetEvent();
        }

        bool TrySetCanceled() noexcept
        {
            if (TrySetState(SetState::MakeCancelWasSet()) == false)
            {
                return false;
            }

            _completionSignal.SetEvent();
            return true;
        }

        bool IsExceptionSet() const noexcept
        {
            return _setState._ExceptionWasSet;
        }
    };

    //** AwaitableCompletionSource<T> implementation
    template<typename _RetT = void>
    class AwaitableCompletionSource sealed : public AwaitableCompletionSourceBase
    {
        friend class AwaitableCompletionSourceBase;
        K_FORCE_SHARED(AwaitableCompletionSource<_RetT>);

    public:
        static NTSTATUS Create(__in KAllocator& Allocator, __in ULONG Tag, __out KSharedPtr<AwaitableCompletionSource<_RetT>>& Result) noexcept
        {
            return AwaitableCompletionSourceBase::Create<_RetT>(Allocator, Tag, Result);
        }

        //* AwaitableCompletionSource<T> specific APIs
        void SetResult(_RetT Result)
        {
            if (TrySetState(SetState::MakeValueWasSet()) == false)
            {
                throw ktl::Exception(K_STATUS_API_CLOSED);
            }

            _returnedValue = Result;
            _completionSignal.SetEvent();
        }

        bool TrySetResult(_RetT Result) noexcept
        {
            if (TrySetState(SetState::MakeValueWasSet()) == false)
            {
                return false;
            }

            _returnedValue = Result;
            _completionSignal.SetEvent();
            return true;
        }

        //* API for Awaiter's use after being signalled
        _RetT GetResult()
        {
            ThrowIfExcpetionResult();
            return _returnedValue;
        }

        ktl::Awaitable<_RetT> GetAwaitable()
        {
            if (!this->_completionSignal.IsSignaled())
            {
                KAsyncEvent::WaitContext::SPtr  thisWaitContext;
                NTSTATUS status = this->_completionSignal.CreateWaitContext(KTL_TAG_Awaitable, GetThisAllocator(), thisWaitContext);
                if (!NT_SUCCESS(status))
                {
                    throw(ktl::Exception(status));
                }

				_RetT	getResultResult;
				{
					// Make sure 'this' is stable across the co_await below
					AwaitableCompletionSource::SPtr myThis = this;
					co_await thisWaitContext->StartWaitUntilSetAsync(nullptr);
					getResultResult = GetResult();
				}
				//*** this is not stable at this point!!!
				co_return getResultResult;
            }

            // Already completed - no need for a KAsyncEvent::WaitContext
			co_await std::experimental::suspend_never{};		// Make sure the compiler does not throw any exceptions sync to this call
            co_return GetResult();
        }

    private:
        _RetT   _returnedValue;
    };

    template<typename _RetT>
    AwaitableCompletionSource<_RetT>::AwaitableCompletionSource()
        : AwaitableCompletionSourceBase()
    {}

    template<typename _RetT>
    AwaitableCompletionSource<_RetT>::~AwaitableCompletionSource()
    {}

    //* Implementation of AwaitableCompletionSource<void>
    template<>
    class AwaitableCompletionSource<void> sealed : public AwaitableCompletionSourceBase
    {
        friend class AwaitableCompletionSourceBase;
        K_FORCE_SHARED(AwaitableCompletionSource);

    public:
        // Factory for instances
        static NTSTATUS Create(__in KAllocator& Allocator, __in ULONG Tag, __out AwaitableCompletionSource<void>::SPtr& Result) noexcept;

        //** AwaitableCompletionSource<void> specoific APIs
        void Set()
        {
            if (TrySetState(SetState::MakeValueWasSet()) == false)
            {
                throw ktl::Exception(K_STATUS_API_CLOSED);
            }

            _completionSignal.SetEvent();
        }

        bool TrySet() noexcept
        {
            if (TrySetState(SetState::MakeValueWasSet()) == false)
            {
                return false;
            }

            _completionSignal.SetEvent();
            return true;
        }

        ktl::Awaitable<void> GetAwaitable()
        {
            if (!this->_completionSignal.IsSignaled())
            {
                KAsyncEvent::WaitContext::SPtr  thisWaitContext;
                NTSTATUS status = this->_completionSignal.CreateWaitContext(KTL_TAG_Awaitable, GetThisAllocator(), thisWaitContext);
                if (!NT_SUCCESS(status))
                {
                    throw(ktl::Exception(status));
                }

				{
					// Make sure 'this' is stable across the co_await below
					AwaitableCompletionSource::SPtr myThis = this;
					co_await thisWaitContext->StartWaitUntilSetAsync(nullptr);
						ThrowIfExcpetionResult();
				}
				//*** this is not stable at this point!!!
				co_return;
            }

            // Already completed - no need for a KAsyncEvent::WaitContext
		    co_await std::experimental::suspend_never{};		// Make sure the compiler does not throw any exceptions sync to this call
            ThrowIfExcpetionResult();
            co_return;
        }
    };

    //* Common Factory implementation for both AwaitableCompletionSource<void> and AwaitableCompletionSource<T>
    template<typename _RetT>
    NTSTATUS AwaitableCompletionSourceBase::Create(__in KAllocator& Allocator, __in ULONG Tag, __out KSharedPtr<AwaitableCompletionSource<_RetT>>& Result) noexcept
    {
        Result = _new(Tag, Allocator) AwaitableCompletionSource<_RetT>();
        if (Result == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(Result->Status()))
        {
            typename AwaitableCompletionSource<_RetT>::SPtr bad = Ktl::Move(Result);
            return bad->Status();
        }

        return STATUS_SUCCESS;
    }


    //* CancellationToken support
    //
    //  The following types (CancellationTokenSource and CancellationToken) mimic a simple and useful subset of their
    //  CLR counterparts. The main difference is that the Exception model wrt ThrowIfCancellationRequested() is different.
    //  In this implementation, there is a fixed exception thrown (ktl::Exception) where the NTSTATUS in that exception is
    //  supplied via Cancel();
    //
    struct CancellationToken;

    //* CancellationTokenSource definition
    class CancellationTokenSource sealed : public KShared<CancellationTokenSource>, public KObject<CancellationTokenSource>
    {
        K_FORCE_SHARED(CancellationTokenSource);

    public: // Application API - See MSDN CLR CancellationTokenSource docs
        static NTSTATUS Create(__in KAllocator& Allocator, __in ULONG Tag, __out CancellationTokenSource::SPtr& Result) noexcept;

        // Any NTSTATUS - defaulted to STATUS_CANCELLED
        // Will be carried in the ktl::Exception thrown by CancellationToken::ThrowIfCancellationRequested()
        //
        void Cancel(NTSTATUS Reason = STATUS_CANCELLED) noexcept;

                                                        __declspec(property(get = GetIsCancellationRequested)) 
        bool IsCancellationRequested;   /* ImpBy */     bool GetIsCancellationRequested() const noexcept;
                                                        __declspec(property(get = GetToken)) 
        CancellationToken Token;        /* ImpBy */     CancellationToken GetToken() noexcept;

    public: // Test support API
        using KShared<CancellationTokenSource>::_RefCount;

    private:
        friend struct CancellationToken;
        void ThrowIfCancellationRequested() const;

    private:
        bool volatile   _cancelWasSet = false;
    };

    //* CancelationToken imp: Just a CancellationTokenSource::SPtr with a few extra Props and methods
    //  to match the CLR model
    struct CancellationToken sealed : private KSharedPtr<CancellationTokenSource>
    {
    public:
        __forceinline
        CancellationToken() : KSharedPtr<CancellationTokenSource>(nullptr) {}

                                                    __declspec(property(get = GetIsCancellationRequested)) 
        bool IsCancellationRequested;   /* ImpBy */ __forceinline bool GetIsCancellationRequested() const noexcept
        {
            if (RawPtr() != nullptr)
            {
                return (*this)->IsCancellationRequested;
            }
            return false;
        }

        __forceinline
        void ThrowIfCancellationRequested() const
        {
            if (RawPtr() != nullptr)
            {
                (*this)->ThrowIfCancellationRequested();
            }
        }

        static CancellationToken None;

    public: // Test support API
        using KSharedPtr<CancellationTokenSource>::Reset;   // Allow CancellationToken relationship to its CTS to be broken

    private:
        friend class CancellationTokenSource;

        __forceinline
        CancellationToken(__in CancellationTokenSource& OwningToken) : CancellationTokenSource::SPtr(&OwningToken) {}
    };
}

#endif
