// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if KTL_USER_MODE

#include "ktl.h"
#include "AsyncCallInAdapter.h"

namespace Ktl
{
    namespace Com
    {
        using ::_delete;

        //
        // This class allows derived KAsyncServiceBase implementation to use HResult/ErrorCode instead 
        // of NTSTATUS for completion of OpenService() and CloseService() 
        //
        // Assumption: The reader is versed in the KAsyncServiceBase class usage
        //
        class FabricAsyncServiceBase : public KAsyncServiceBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(FabricAsyncServiceBase)

        public:
            const static BOOLEAN IsFabricAsyncServiceBase = TRUE;   // Used for compile-time type check

            // HRESULT forms of KAsyncServiceBase open and close result status
            inline HRESULT OpenResult() { return _OpenResult; }
            inline HRESULT CloseResult() { return _CloseResult; }

        protected:
            // Alternate forms of CompleteOpen() service methods
            BOOLEAN CompleteOpen(__in HRESULT Hr);
            BOOLEAN CompleteOpen(__in Common::ErrorCode ErrorCode);
            BOOLEAN CompleteOpen(__in Common::ErrorCodeValue::Enum ErrorCode);

            // Alternate forms of CompleteClose() service methods
            BOOLEAN CompleteClose(__in HRESULT Hr);
            BOOLEAN CompleteClose(__in Common::ErrorCode ErrorCode);
            BOOLEAN CompleteClose(__in Common::ErrorCodeValue::Enum ErrorCode);

            // NOTE: FabricAsyncServiceBase implements OnServiceReuse() and if it's
            //       derivation wishes to do the same the following pattern must be
            //       used:
            //  
            //          void MyDerivation::OnServiceReuse() override
            //          {
            //              ... my reuse logic ...
            //              __super::OnServiceReuse();
            //          }
            //
            //      Failure to do so will result in a fail-fast condition onnext use
            //
            VOID OnServiceReuse() override;

        private:
            VOID OnUnsafeOpenCompleteCalled(void* HResultPtr) override;
            VOID OnUnsafeCloseCompleteCalled(void* HResultPtr) override;

        private:
            HRESULT         _OpenResult;
            HRESULT         _CloseResult;
        };


        //* Class to adapt KAsyncService derivation (including FabricAsyncServiceBase
        //  derivations) open and close behaviors to the COM world as define by the
        //  WinFab IFabricAsyncOperationContext pattern.
        //
        class ServiceOpenCloseAdapter : public AsyncCallInAdapterBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(ServiceOpenCloseAdapter)

        private:
            // IFabricAsyncOperationContext implementation

            HRESULT STDMETHODCALLTYPE 
            Cancel();     // Invokes KAsyncServiceBase::Abort()

        public:
            //* Creates an adapter and starts an async service open operation
            //
            // Example:
            //      
            //      MyService::SPtr service;
            //
            //      return ServiceOpenCloseAdapter::StartOpen(
            //          GetThisAllocator(),
            //          service,
            //          callback,                // IFabricAsyncOperationCallback
            //          context,                 // IFabricAsyncOperationContext (out)
            //
            //          // MyService is a FabricAsyncServiceBase derivation
            //          [&] (MyService& Service, KAsyncServiceBase::OpenCompletionCallback Callback, ..., argn) -> HRESULT
            //          {
            //             // start the operation, pass any arguments required
            //              return Service.StartMyOpen(nullptr, Callback, ..., argn);
            //          });
            //
            //      -- OR --
            //
            //      return ServiceOpenCloseAdapter::StartOpen(
            //          GetThisAllocator(),
            //          service,
            //          callback,                // IFabricAsyncOperationCallback
            //          context,                 // IFabricAsyncOperationContext (out)
            //
            //          // MyService is a non-FabricAsyncServiceBase KAsyncServiceBase derivation
            //          [&] (MyService& Service, KAsyncServiceBase::OpenCompletionCallback Callback, ..., argn) -> NTSTATUS
            //          {
            //             // start the operation, pass any arguments required
            //              return Service.StartMyOpen(nullptr, Callback, ..., argn);
            //          });
            //
            //  End:
            //
            //      MyService::SPtr openedService;
            //      HRESULT hr = ServiceOpenCloseAdapter::EndOpen(
            //          context,                 // IFabricAsyncOperationContext from StartOpen
            //          openedService);
            //
            //      if (SUCCEEDED(hr))
            //      {
            //          // extact output values from the close via openedService, if any
            //          ...
            //      }
            //
            //      return hr;
            //
            template <typename ServiceType, typename StartOperationType>
            static HRESULT 
            StartOpen(
                __in KAllocator& Allocator,
                __in KSharedPtr<ServiceType> ForService,
                __in IFabricAsyncOperationCallback* Callback,
                __out IFabricAsyncOperationContext** Context,
                __in StartOperationType const& StartOp);

            // Completes the async service open operation started via StartOpen()
            template <typename ServiceType>
            static HRESULT 
            EndOpen(__in IFabricAsyncOperationContext* Context, __out KSharedPtr<ServiceType>& Service);

            //* Creates the adapter and starts the async service close operation
            //
            // Example:
            //      
            //      MyService::SPtr service;
            //
            //      return ServiceOpenCloseAdapter::StartClose(
            //          GetThisAllocator(),
            //          service,
            //          callback,                // IFabricAsyncOperationCallback
            //          context,                 // IFabricAsyncOperationContext (out)
            //
            //          // MyService is a FabricAsyncServiceBase derivation
            //          [&] (MyService& Service, KAsyncServiceBase::CloseCompletionCallback Callback, ..., argn) -> HRESULT
            //          {
            //             // start the operation, pass any arguments required
            //              return Service.StartMyClose(nullptr, Callback, ..., argn);
            //          });
            //
            //      -- OR --
            //
            //      return ServiceOpenCloseAdapter::StartClose(
            //          GetThisAllocator(),
            //          service,
            //          callback,                // IFabricAsyncOperationCallback
            //          context,                 // IFabricAsyncOperationContext (out)
            //
            //          // MyService is a non-FabricAsyncServiceBase KAsyncServiceBase derivation
            //          [&] (MyService& Service, KAsyncServiceBase::CloseCompletionCallback Callback, ..., argn) -> NTSTATUS
            //          {
            //             // start the operation, pass any arguments required
            //              return Service.StartMyClose(nullptr, Callback, ..., argn);
            //          });
            //
            //  End:
            //
            //      MyService::SPtr closedService;
            //      HRESULT hr = ServiceOpenCloseAdapter::EndClose(
            //          context,                 // IFabricAsyncOperationContext from StartClose
            //          closedService);
            //
            //      if (SUCCEEDED(hr))
            //      {
            //          // extact output values from the close via closedService, if any
            //          ...
            //      }
            //
            //      return hr;
            //
            template <typename ServiceType, typename StartOperationType>
            static HRESULT 
            StartClose(
                __in KAllocator& Allocator,
                __in KSharedPtr<ServiceType> ForService,
                __in IFabricAsyncOperationCallback* Callback,
                __out IFabricAsyncOperationContext** Context,
                __in StartOperationType const& StartOp);

            // Completes the async service close operation started via StartClose()
            template <typename ServiceType>
            static HRESULT 
            EndClose(__in IFabricAsyncOperationContext* Context, __out KSharedPtr<ServiceType>& Service);

        private:
            ServiceOpenCloseAdapter(__in KAsyncServiceBase& Service);

            static NTSTATUS Create(
                __in KAllocator& Allocator, 
                __in KAsyncServiceBase& Service,
                __out ServiceOpenCloseAdapter::SPtr& Adapter);

            template <typename CallbackType, typename ServiceType, typename StartOperationType>
            static HRESULT 
            CreateAndStart(
                __in KAllocator& Allocator,
                __in KSharedPtr<ServiceType>& ForService,
                __in IFabricAsyncOperationCallback* Callback,
                __out IFabricAsyncOperationContext** Context,
                __in StartOperationType const& StartOp);

            template < typename GetOp, typename ServiceType>
            static HRESULT 
            End(
                __in GetOp Op,
                __in IFabricAsyncOperationContext* Context, 
                __out KSharedPtr<ServiceType>& Service);

            VOID OnOperationCompleted(
                __in KAsyncContextBase *const Parent,
                __in KAsyncServiceBase& Service,
                __in NTSTATUS OpStatus);

        private:
            KAsyncServiceBase::SPtr     _Service;
        };
    }
}

//*** ServiceOpenCloseAdapter templated method implementation ***
namespace Ktl
{
    namespace Com
    {
        //* Common logic used for both Open and Close operations
        template <typename CallbackType, typename ServiceType, typename StartOperationType>
        HRESULT
        ServiceOpenCloseAdapter::CreateAndStart(
            __in KAllocator& Allocator,
            __in KSharedPtr<ServiceType>& ForService,
            __in IFabricAsyncOperationCallback* Callback,
            __out IFabricAsyncOperationContext** Context,
            __in StartOperationType const& StartOp)
        {
            ASSERT_IF(!ForService, "ForService is null");

            // Create and initialize the adapter.
            NTSTATUS                        status = STATUS_SUCCESS;
            ServiceOpenCloseAdapter::SPtr   adapter;

            status = Create(Allocator, *ForService, adapter);
            if (!NT_SUCCESS(status))
            {
                return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }
            adapter->_OuterAsyncCallback.SetAndAddRef(Callback);

            CallbackType                    innerCallback(adapter.RawPtr(), &ServiceOpenCloseAdapter::OnOperationCompleted);

            // Keep the adapter alive as long as the operation has not completed.
            // Release is called in OnOperationCompleted() 
            // This must be done before starting the inner operation. Since it may complete immediately and Release this adapter. 
            adapter->AddRef();
            KFinally([&adapter](){ if (adapter == TRUE) adapter->Release(); });

            // Call the caller provided functor to start the operation.
            __if_exists(ServiceType::IsFabricAsyncServiceBase)
            {
                HRESULT started = StartOp(*ForService, innerCallback);
                if (!SUCCEEDED(started))
                {
                    return Common::ComUtility::OnPublicApiReturn(started);
                }
            }
            __if_not_exists(ServiceType::IsFabricAsyncServiceBase)
            {
                NTSTATUS started = StartOp(*ForService, innerCallback);
                if (!NT_SUCCESS(started))
                {
                    return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(started));
                }
            }

            // Return to COM
            *Context = static_cast<IFabricAsyncOperationContext*>(adapter.Detach());
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }

        //* Common logic used for completing both open and close operations
        template <typename GetOp, typename ServiceType>
        HRESULT 
        ServiceOpenCloseAdapter::End(
            __in GetOp Op,
            __in IFabricAsyncOperationContext* Context,
            __out KSharedPtr<ServiceType>& Service)
        {
            if (!Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
            }

            ServiceOpenCloseAdapter* adapter = static_cast<ServiceOpenCloseAdapter*>(Context);

            HRESULT hr = adapter->WaitCompleted();

            if (!SUCCEEDED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            Service = adapter->_Service.DownCast<ServiceType>();
            return Op(*Service);
        }

        
        //* Public Open Service Support
        template <typename ServiceType, typename StartOperationType>
        HRESULT 
        ServiceOpenCloseAdapter::StartOpen(
            __in KAllocator& Allocator,
            __in KSharedPtr<ServiceType> ForService,
            __in IFabricAsyncOperationCallback* Callback,
            __out IFabricAsyncOperationContext** Context,
            __in StartOperationType const& StartOp)
        {
            return ServiceOpenCloseAdapter::CreateAndStart<KAsyncServiceBase::OpenCompletionCallback>(
                Allocator,
                ForService,
                Callback,
                Context,
                StartOp);
        }

        // Complete an Open on a Service
        template <typename ServiceType>
        HRESULT 
        ServiceOpenCloseAdapter::EndOpen(
            __in IFabricAsyncOperationContext* Context,
            __out KSharedPtr<ServiceType>& Service)
        {
            auto op = [&](ServiceType& Service) -> HRESULT
            {
                __if_exists(ServiceType::IsFabricAsyncContextBase)
                {
                    return Service.OpenResult();
                }
                __if_not_exists(ServiceType::IsFabricAsyncContextBase)
                {
                    return KComUtility::ToHRESULT(Service.GetOpenStatus());
                }
            };

            return ServiceOpenCloseAdapter::End(op, Context, Service);
        }


        //* Public Close Service Support
        template <typename ServiceType, typename StartOperationType>
        HRESULT 
        ServiceOpenCloseAdapter::StartClose(
            __in KAllocator& Allocator,
            __in KSharedPtr<ServiceType> ForService,
            __in IFabricAsyncOperationCallback* Callback,
            __out IFabricAsyncOperationContext** Context,
            __in StartOperationType const& StartOp)
        {
            return ServiceOpenCloseAdapter::CreateAndStart<KAsyncServiceBase::CloseCompletionCallback>(
                Allocator,
                ForService,
                Callback,
                Context,
                StartOp);
        }

        // Complete an Close on a Service
        template <typename ServiceType>
        HRESULT 
        ServiceOpenCloseAdapter::EndClose(
            __in IFabricAsyncOperationContext* Context,
            __out KSharedPtr<ServiceType>& Service)
        {
            auto op = [&](ServiceType& Service) -> HRESULT
            {
                __if_exists(ServiceType::IsFabricAsyncContextBase)
                {
                    return Service.CloseResult();
                }
                __if_not_exists(ServiceType::IsFabricAsyncContextBase)
                {
                    return KComUtility::ToHRESULT(Service.Status());
                }
            };

            return ServiceOpenCloseAdapter::End(op, Context, Service);
        }
    }
}

#endif
