// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if KTL_USER_MODE

#include "FabricAsyncContextBase.h"
#include "KComUtility.h"

namespace Ktl
{
    namespace Com
    {
        using ::_delete;

#if defined(PLATFORM_UNIX)
        class AsyncCallInAdapter;
#endif

        //* Common base class used for various types of IFabricAsyncOperationContext adapter
        //  KTL-centric implementations
        class AsyncCallInAdapterBase :  public KShared<AsyncCallInAdapter>,
                                        public KObject<AsyncCallInAdapter>,
                                        public IFabricAsyncOperationContext,
                                        public KThreadPool::WorkItem
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncCallInAdapterBase)

            K_BEGIN_COM_INTERFACE_LIST(AsyncCallInAdapterBase)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationContext)
                COM_INTERFACE_ITEM(IID_IFabricAsyncOperationContext, IFabricAsyncOperationContext)
            K_END_COM_INTERFACE_LIST()

        private:
            // IFabricAsyncOperationContext implementation
            BOOLEAN STDMETHODCALLTYPE 
            IsCompleted() override;

            BOOLEAN STDMETHODCALLTYPE 
            CompletedSynchronously() override;

            HRESULT STDMETHODCALLTYPE 
            get_Callback(__out IFabricAsyncOperationCallback** Callback) override;

        protected:
            virtual VOID OnExecuted() {}
            VOID ProcessOperationCompleted();
            HRESULT WaitCompleted();

        protected:
            Common::ComPointer<IFabricAsyncOperationCallback>   _OuterAsyncCallback;

        private:
            VOID Execute() override;

        private:
            KSpinLock                   _ThisLock;
            KUniquePtr<KEvent>          _WaitHandle; 
            BOOLEAN                     _Completed;
        };

        //
        // This class is used to implement the Begin/End pair of asynchronous operations with
        // FabricAsyncContextBase or raw KAsyncContextBase derived operations.
        //
        // Begin:
        //      MyOperation::SPtr operation; // create or reuse async operation
        //
        //      return AsyncCallInAdapter::CreateAndStart(
        //          GetThisAllocator(),
        //          operation,
        //          callback,                // IFabricAsyncOperationCallback
        //          context,                 // IFabricAsyncOperationContext (out)
        //          [&] (AsyncCallInAdapter& a, MyOperation& o, KAsyncContextBase::CompletionCallback asyncCallback) -> HRESULT
        //          {
        //             // start the operation, pass any arguments required
        //             return o.StartMyOperation(myArgument1, myArgument2, nullptr, asyncCallback);
        //          });
        //
        //      -- OR --
        //
        //      return AsyncCallInAdapter::CreateAndStart(
        //          GetThisAllocator(),
        //          operation,
        //          callback,                // IFabricAsyncOperationCallback
        //          context,                 // IFabricAsyncOperationContext (out)
        //          [&] (AsyncCallInAdapter& a, MyOperation& o, KAsyncContextBase::CompletionCallback asyncCallback) -> NTSTATUS
        //          {
        //             // start the operation, pass any arguments required
        //             return o.StartMyOperation(myArgument1, myArgument2, nullptr, asyncCallback);
        //          });
        //
        //
        // End:
        //      MyOperation::SPtr operation;
        //      HRESULT hr = AsyncCallInAdapter::End(
        //          context,                 // IFabricAsyncOperationContext (adaptor)
        //          operation);
        //
        //      if (SUCCEEDED(hr))
        //      {
        //          // extact output values, if any
        //          *sequenceNumber = operation->SequenceNumber();
        //      }
        //
        //      return hr;
        //
        //**
        //  In some applications of AsyncCallInAdapter, it is important to be able to create custom
        //  extensions to AsyncCallInAdapter - for example to hold results from the async op passed
        //  to underlying KTL Start*() methods that take _out parameters which will be written to
        //  while the async op is executing can then be retrieved as part of End processing.
        //
        //  For example:
        //      struct MyAdaptorExt
        //      {
        //          MyState     MyResults;
        //      };
        //
        //  
        //
        //  Begin:
        //      return AsyncCallInAdapter::CreateAndStart(
        //          GetThisAllocator(),
        //          operation,
        //          callback,                // IFabricAsyncOperationCallback
        //          context,                 // IFabricAsyncOperationContext (out)
        //          [&] (AsyncCallInAdapter& a, MyOperation& o, KAsyncContextBase::CompletionCallback asyncCallback) -> NTSTATUS
        //          {
        //             // start the operation, pass any arguments required
        //             MyAdaptorExt&   myExt = (MyAdaptorExt&)a;
        //             return o.StartMyOperation(myArgument1, myArgument2, myExt.MyResults, nullptr, asyncCallback);
        //          },
        //
        //          // Optional Adaptor factory lambda
        //          [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
        //          {
        //              return AsyncCallInAdapter::Extended<MyAdaptorExt>::Create(Allocator, Adapter);
        //          });
        //
        //
        // End:
        //      MyOperation::SPtr operation;
        //      HRESULT hr = AsyncCallInAdapter::End(
        //          context,                 // IFabricAsyncOperationContext (adaptor)
        //          operation);
        //
        //      if (SUCCEEDED(hr))
        //      {
        //          // extact output values, if any
        //          *sequenceNumber = operation->SequenceNumber();
        //          *myResultingState = ((AsyncCallInAdapter::Extended<MyAdaptorExt>*)context)->MyResults;
        //      }
        //
        //      return hr;
        //
        class AsyncCallInAdapter :  public AsyncCallInAdapterBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncCallInAdapter)

        public:
            // IFabricAsyncOperationContext implementation
            HRESULT STDMETHODCALLTYPE 
            Cancel() override;

            // Creates the adapter and starts the async operation
            //  
            //  InstanceFactory = Override for the AsyncCallInAdapter factory (Create()) method below
            //
            template <typename Operation, typename StartOperation, typename FactoryFunction>
            static HRESULT 
            CreateAndStart(
                __in KAllocator& Allocator,
                __in KSharedPtr<Operation> ForOp,
                __in IFabricAsyncOperationCallback* Callback,
                __out IFabricAsyncOperationContext** Context,
                __in StartOperation const& StartOp,
                __in FactoryFunction const& InstanceFactory);

            template <typename StartOperation, typename FactoryFunction>
            static HRESULT 
            CreateAndStart(
                __in KAllocator& Allocator,
                __in IFabricAsyncOperationCallback* Callback,
                __out IFabricAsyncOperationContext** Context,
                __in StartOperation const& StartOp,
                __in FactoryFunction const& InstanceFactory);

            template <typename Operation, typename StartOperation>
            static HRESULT 
            CreateAndStart(
                __in KAllocator& Allocator,
                __in KSharedPtr<Operation> ForOp,
                __in IFabricAsyncOperationCallback* Callback,
                __out IFabricAsyncOperationContext** Context,
                __in StartOperation const& StartOp);

            template <typename StartOperation>
            static HRESULT 
            CreateAndStart(
                __in KAllocator& Allocator,
                __in IFabricAsyncOperationCallback* Callback,
                __out IFabricAsyncOperationContext** Context,
                __in StartOperation const& StartOp);

            // Completes (and waits for if needed) an adapted async operation started via CreateAndStart
            template <typename T>
            HRESULT static
            End(__in IFabricAsyncOperationContext* Context, __out KSharedPtr<T>& Operation);

        private:
            VOID OnOperationCompleted(__in KAsyncContextBase *const Parent, __in KAsyncContextBase& Context);
            VOID OnExecuted() override;
            static NTSTATUS Create(__in KAllocator& Allocator, __out AsyncCallInAdapter::SPtr& Adapter);

            template <typename Operation, typename StartOperation, typename FactoryFunction>
            static HRESULT 
            InternalCreateAndStart(
                __in BOOLEAN HasForOp,
                __in KAllocator& Allocator,
                __in KSharedPtr<Operation> ForOp,
                __in IFabricAsyncOperationCallback* Callback,
                __out IFabricAsyncOperationContext** Context,
                __in StartOperation const& StartOp,
                __in FactoryFunction const& InstanceFactory);

        private:
            BOOLEAN                 _InnerOpIsFabricAsyncContextBase;
            KAsyncContextBase::SPtr _InnerOperation;

        public:
            template <typename ExtType> class Extended;
        };

        //* Helper template for generating custom AsyncCallInAdapter extensions
        //
        //  NOTE: Typically used when _out parameters must be supplied to KAsyncContextBase Start*() methods
        //
        template <typename ExtType>
        class AsyncCallInAdapter::Extended : public AsyncCallInAdapter
        {
            K_FORCE_SHARED(Extended)

        public:
            ExtType     Extension;

        public:
            static NTSTATUS Create(__in KAllocator& Allocator, __out AsyncCallInAdapter::SPtr& Adaptor);
        };
    }
}


//*** AsyncCallInAdapter templated implementation ***
namespace Ktl
{
    namespace Com
    {
        template <typename Operation, typename StartOperation, typename FactoryFunction>
        HRESULT 
        AsyncCallInAdapter::InternalCreateAndStart(
            __in BOOLEAN HasForOp,
            __in KAllocator& Allocator,
            __in KSharedPtr<Operation> ForOp,
            __in IFabricAsyncOperationCallback* Callback,
            __out IFabricAsyncOperationContext** Context,
            __in StartOperation const& StartOp,
            __in FactoryFunction const& InstanceFactory)
        {
            if (HasForOp)
            {
                ASSERT_IF(nullptr == ForOp.RawPtr(), "operation is null");
            }

            // Create and initialize the adapter.
            NTSTATUS status = STATUS_SUCCESS;

            AsyncCallInAdapter::SPtr adapter;
            status = InstanceFactory(Allocator, adapter);
            if (!NT_SUCCESS(status))
            {
                return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            adapter->_InnerOperation.Attach(ForOp.Detach());
            adapter->_OuterAsyncCallback.SetAndAddRef(Callback);

            KAsyncContextBase::CompletionCallback innerCallback;
            innerCallback.Bind(adapter.RawPtr(), &AsyncCallInAdapter::OnOperationCompleted);

            // Keep the adapter alive as long as the operation has not completed.
            // Release is called in OnOperationCompleted() 
            // This must be done before starting the inner operation. Since it may complete immediately and Release this adapter. 
            adapter->AddRef();
            KFinally([&adapter](){ if (adapter == TRUE) adapter->Release(); });

            //
            // Call the caller provided functor to start the operation.
            //
            __if_exists(Operation::IsFabricAsyncContextBase)
            {
                adapter->_InnerOpIsFabricAsyncContextBase = TRUE;
                HRESULT started = StartOp(*adapter, static_cast<Operation&>(*adapter->_InnerOperation.RawPtr()), innerCallback);
                if (!SUCCEEDED(started))
                {
                    return Common::ComUtility::OnPublicApiReturn(started);
                }
            }
            __if_not_exists(Operation::IsFabricAsyncContextBase)
            {
                adapter->_InnerOpIsFabricAsyncContextBase = FALSE;
                NTSTATUS started = StartOp(*adapter, static_cast<Operation&>(*adapter->_InnerOperation.RawPtr()), innerCallback);
                if (!NT_SUCCESS(started))
                {
                    return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(started));
                }
            }

            // Return to COM
            *Context = static_cast<IFabricAsyncOperationContext*>(adapter.Detach());
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }

        template <typename Operation, typename StartOperation, typename FactoryFunction>
        HRESULT 
        AsyncCallInAdapter::CreateAndStart(
            __in KAllocator& Allocator,
            __in KSharedPtr<Operation> ForOp,
            __in IFabricAsyncOperationCallback* Callback,
            __out IFabricAsyncOperationContext** Context,
            __in StartOperation const& StartOp,
            __in FactoryFunction const& InstanceFactory)
        {
            return InternalCreateAndStart(TRUE, Allocator, ForOp, Callback, Context, StartOp, InstanceFactory);
        }

        template <typename StartOperation, typename FactoryFunction>
        HRESULT 
        AsyncCallInAdapter::CreateAndStart(
            __in KAllocator& Allocator,
            __in IFabricAsyncOperationCallback* Callback,
            __out IFabricAsyncOperationContext** Context,
            __in StartOperation const& StartOp,
            __in FactoryFunction const& InstanceFactory)
        {
            return InternalCreateAndStart<KAsyncContextBase>(
                FALSE, 
                Allocator, 
                KAsyncContextBase::SPtr(nullptr), 
                Callback, 
                Context, 
                StartOp, 
                InstanceFactory);
        }

        template <typename Operation, typename StartOperation>
        HRESULT 
        AsyncCallInAdapter::CreateAndStart(
            __in KAllocator& Allocator,
            __in KSharedPtr<Operation> ForOp,
            __in IFabricAsyncOperationCallback* Callback,
            __out IFabricAsyncOperationContext** Context,
            __in StartOperation const& StartOp)
        {
            return InternalCreateAndStart(TRUE, Allocator, ForOp, Callback, Context, StartOp, AsyncCallInAdapter::Create);
        }

        template <typename StartOperation>
        HRESULT 
        AsyncCallInAdapter::CreateAndStart(
            __in KAllocator& Allocator,
            __in IFabricAsyncOperationCallback* Callback,
            __out IFabricAsyncOperationContext** Context,
            __in StartOperation const& StartOp)
        {
            return InternalCreateAndStart<KAsyncContextBase>(
                FALSE, 
                Allocator, 
                KAsyncContextBase::SPtr(nullptr), 
                Callback, 
                Context, 
                StartOp, 
                AsyncCallInAdapter::Create);
        }

        template <typename OpType>
        HRESULT 
        AsyncCallInAdapter::End(
            __in IFabricAsyncOperationContext* Context,
            __out KSharedPtr<OpType>& Operation)
        {
            if (!Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
            }

            AsyncCallInAdapter * adapter = static_cast<AsyncCallInAdapter*>(Context);

            HRESULT hr = adapter->WaitCompleted();

            if (!SUCCEEDED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            Operation = adapter->_InnerOperation.DownCast<OpType>();

            __if_exists(OpType::IsFabricAsyncContextBase)
            {
                return Operation->Result();
            }
            __if_not_exists(OpType::IsFabricAsyncContextBase)
            {
                return KComUtility::ToHRESULT(Operation->Status());
            }
        }

        //* Extender support
        template <typename ExtType>
        AsyncCallInAdapter::Extended<ExtType>::Extended()
        {
        }

        template <typename ExtType>
        AsyncCallInAdapter::Extended<ExtType>::~Extended()
        {
        }

        template <typename ExtType>
        NTSTATUS
        AsyncCallInAdapter::Extended<ExtType>::Create(__in KAllocator& Allocator, __out AsyncCallInAdapter::SPtr& Adaptor)
        {
            Adaptor = _new(KTL_TAG_COM_DLL, Allocator) Extended();
            if (Adaptor == nullptr)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            NTSTATUS        status = Adaptor->Status();
            if (!NT_SUCCESS(status))
            {
                Adaptor = nullptr;
            }

            return status;
        }
    }
}

#endif
