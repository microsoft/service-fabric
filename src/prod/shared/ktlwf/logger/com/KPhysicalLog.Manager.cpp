// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "KPhysicalLog.h"
#include <FabricAsyncServiceBase.h>
#include <AsyncCallInAdapter.h>

namespace Ktl
{
    namespace Com
    {
        //*** IKPhysicalLogManager COM Implementation
        ComIKPhysicalLogManager::ComIKPhysicalLogManager(__in BOOLEAN Inproc)
        {
            NTSTATUS status;

            if (Inproc)
            {
                status = KtlLogManager::CreateInproc(GetThisAllocationTag(), GetThisAllocator(), _BackingManager);
            } else {
                status = KtlLogManager::CreateDriver(GetThisAllocationTag(), GetThisAllocator(), _BackingManager);
            }
            
            SetConstructorStatus(status);
        }

        ComIKPhysicalLogManager::~ComIKPhysicalLogManager()
        {
        }

        HRESULT 
        ComIKPhysicalLogManager::Create(__in KAllocator& Allocator, __out ComIKPhysicalLogManager::SPtr& Result)
        {
            Result = _new(KTL_TAG_LOGGER, Allocator) ComIKPhysicalLogManager(FALSE);
            if (Result == nullptr)
            {
                return E_OUTOFMEMORY;
            }

            NTSTATUS    status = Result->Status();
            if (!NT_SUCCESS(status))
            {
                Result = nullptr;

                return Ktl::Com::KComUtility::ToHRESULT(status);
            }

            return S_OK;
        }

        HRESULT 
        ComIKPhysicalLogManager::CreateInproc(__in KAllocator& Allocator, __out ComIKPhysicalLogManager::SPtr& Result)
        {
            Result = _new(KTL_TAG_LOGGER, Allocator) ComIKPhysicalLogManager(TRUE);
            if (Result == nullptr)
            {
                return E_OUTOFMEMORY;
            }

            NTSTATUS    status = Result->Status();
            if (!NT_SUCCESS(status))
            {
                Result = nullptr;

                return Ktl::Com::KComUtility::ToHRESULT(status);
            }

            return S_OK;
        }
        
        //** OpenLogManager
        HRESULT 
        ComIKPhysicalLogManager::BeginOpen( 
            __in IFabricAsyncOperationCallback *const Callback,
            __out IFabricAsyncOperationContext **const Context)
        {
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;

            auto StartOpen = [&](KtlLogManager& Service, KAsyncServiceBase::OpenCompletionCallback Callback) -> NTSTATUS
            {
                return Service.StartOpenLogManager(nullptr, Callback); 
            };

            HRESULT hr = ServiceOpenCloseAdapter::StartOpen<KtlLogManager>(
                GetThisAllocator(), 
                _BackingManager, 
                Callback, 
                Context, 
                StartOpen);

            return Common::ComUtility::OnPublicApiReturn(hr);
        }
        
        HRESULT 
        ComIKPhysicalLogManager::EndOpen( 
            __in IFabricAsyncOperationContext *const Context)
        {
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogManager::SPtr service;
            HRESULT hr = ServiceOpenCloseAdapter::EndOpen(static_cast<ServiceOpenCloseAdapter*>(Context), service);
            KAssert(service.RawPtr() == _BackingManager.RawPtr());
            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        
        //** CloseLogManager
        HRESULT 
        ComIKPhysicalLogManager::BeginClose( 
            __in IFabricAsyncOperationCallback *const Callback,
            __out IFabricAsyncOperationContext **const Context)
        {
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;

            auto StartClose = [&](KtlLogManager& Service, KAsyncServiceBase::CloseCompletionCallback Callback) -> NTSTATUS
            {
                return Service.StartCloseLogManager(nullptr, Callback); 
            };

            HRESULT hr = ServiceOpenCloseAdapter::StartClose<KtlLogManager>(
                GetThisAllocator(), 
                _BackingManager, 
                Callback, 
                Context, 
                StartClose);

            return Common::ComUtility::OnPublicApiReturn(hr);
        }
        
        HRESULT 
        ComIKPhysicalLogManager::EndClose( 
            __in IFabricAsyncOperationContext *const Context)
        {
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogManager::SPtr service;
            HRESULT hr = ServiceOpenCloseAdapter::EndClose(static_cast<ServiceOpenCloseAdapter*>(Context), service);
            KAssert(service.RawPtr() == _BackingManager.RawPtr());
            _BackingManager == nullptr;

            return Common::ComUtility::OnPublicApiReturn(hr);
        }


        //** CreateLogContainer
        struct LogContainerOpResults
        {
            KtlLogContainer::SPtr   LogContainer;
        };
        
        HRESULT 
        ComIKPhysicalLogManager::BeginCreateLogContainer( 
            __in LPCWSTR FullyQualifiedLogFilePath,
            __in KTL_LOG_ID LogId,
            __in LPCWSTR LogType,
            __in LONGLONG LogSize,
            __in ULONG MaxAllowedStreams,
            __in ULONG MaxRecordSize,
            __in DWORD CreationFlags,
            __in IFabricAsyncOperationCallback *const Callback,
            __out IFabricAsyncOperationContext **const Context)
        {
            if ((nullptr == Context) || (FullyQualifiedLogFilePath == nullptr) || (LogType == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogManager::AsyncCreateLogContainer::SPtr    op;

            NTSTATUS    status = _BackingManager->CreateAsyncCreateLogContainerContext(op);
            if (!NT_SUCCESS(status))
            {
                 return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            KString::SPtr       fullyQualifiedLogFilePath = KString::Create(FullyQualifiedLogFilePath, GetThisAllocator(), TRUE);
            if (fullyQualifiedLogFilePath == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KtlLogManager::AsyncCreateLogContainer& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<LogContainerOpResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<LogContainerOpResults>&)Adaptor;

                    Operation.StartCreateLogContainer(
                        *fullyQualifiedLogFilePath,
                        RvdLogId(LogId), 
                        LogSize,
                        MaxAllowedStreams, 
                        MaxRecordSize,
                        CreationFlags,
                        adaptor.Extension.LogContainer,
                        nullptr,
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom CreateLogOpResults adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<LogContainerOpResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }
        
        HRESULT 
        ComIKPhysicalLogManager::EndCreateLogContainer( 
            __in IFabricAsyncOperationContext *const Context,
            __out IKPhysicalLogContainer **const Result)
        {
            if ((nullptr == Context) || (Result == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Result = nullptr;
            KtlLogManager::AsyncCreateLogContainer::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            ComIKPhysicalLogContainer::SPtr     sptr;

            hr = ComIKPhysicalLogContainer::Create(
                GetThisAllocator(),
                *((AsyncCallInAdapter::Extended<LogContainerOpResults>&)(*Context)).Extension.LogContainer,
                sptr);

            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            *Result = sptr.Detach();
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }

        
        //** OpenLogContainer
        HRESULT 
        ComIKPhysicalLogManager::BeginOpenLogContainer( 
            __in LPCWSTR FullyQualifiedLogFilePath,
            __in KTL_LOG_ID LogId,
            __in IFabricAsyncOperationCallback *const Callback,
            __out IFabricAsyncOperationContext **const Context)
        {
            if ((Context == nullptr) || (FullyQualifiedLogFilePath == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogManager::AsyncOpenLogContainer::SPtr    op;

            NTSTATUS    status = _BackingManager->CreateAsyncOpenLogContainerContext(op);
            if (!NT_SUCCESS(status))
            {
                 return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            KString::SPtr       fullyQualifiedLogFilePath = KString::Create(FullyQualifiedLogFilePath, GetThisAllocator(), TRUE);
            if (fullyQualifiedLogFilePath == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KtlLogManager::AsyncOpenLogContainer& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<LogContainerOpResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<LogContainerOpResults>&)Adaptor;

                    Operation.StartOpenLogContainer(
                        *fullyQualifiedLogFilePath,
                        RvdLogId(LogId),
                        adaptor.Extension.LogContainer,
                        nullptr,
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom CreateLogOpResults adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<LogContainerOpResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }
        
        HRESULT 
        ComIKPhysicalLogManager::BeginOpenLogContainerById( 
            __in KTL_DISK_ID DiskId,
            __in KTL_LOG_ID LogId,
            __in IFabricAsyncOperationCallback *const Callback,
            __out IFabricAsyncOperationContext **const Context)
        {
            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogManager::AsyncOpenLogContainer::SPtr    op;

            NTSTATUS    status = _BackingManager->CreateAsyncOpenLogContainerContext(op);
            if (!NT_SUCCESS(status))
            {
                 return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KtlLogManager::AsyncOpenLogContainer& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<LogContainerOpResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<LogContainerOpResults>&)Adaptor;

                    Operation.StartOpenLogContainer(
                        KGuid(DiskId),
                        RvdLogId(LogId),
                        adaptor.Extension.LogContainer,
                        nullptr,
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom CreateLogOpResults adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<LogContainerOpResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }
        
        HRESULT 
        ComIKPhysicalLogManager::EndOpenLogContainer( 
            __in IFabricAsyncOperationContext *const Context,
            __out IKPhysicalLogContainer **const Result)
        {
            if ((nullptr == Context) || (Result == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Result = nullptr;
            KtlLogManager::AsyncOpenLogContainer::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            ComIKPhysicalLogContainer::SPtr     sptr;

            hr = ComIKPhysicalLogContainer::Create(
                GetThisAllocator(),
                *((AsyncCallInAdapter::Extended<LogContainerOpResults>&)(*Context)).Extension.LogContainer,
                sptr);

            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            *Result = sptr.Detach();
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }
        

        //** DeleteLogContainer
        HRESULT 
        ComIKPhysicalLogManager::BeginDeleteLogContainer( 
            __in LPCWSTR FullyQualifiedLogFilePath,
            __in KTL_LOG_ID LogId,
            __in IFabricAsyncOperationCallback *const Callback,
            __out IFabricAsyncOperationContext **const Context)
        {
            if ((Context == nullptr) || (FullyQualifiedLogFilePath == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogManager::AsyncDeleteLogContainer::SPtr    op;

            NTSTATUS    status = _BackingManager->CreateAsyncDeleteLogContainerContext(op);
            if (!NT_SUCCESS(status))
            {
                 return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            KString::SPtr       fullyQualifiedLogFilePath = KString::Create(FullyQualifiedLogFilePath, GetThisAllocator(), TRUE);
            if (fullyQualifiedLogFilePath == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter&,
                     KtlLogManager::AsyncDeleteLogContainer& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    Operation.StartDeleteLogContainer(
                        *fullyQualifiedLogFilePath,
                        RvdLogId(LogId),
                        nullptr,
                        AsyncCallback);

                    return STATUS_SUCCESS;
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }
        
        HRESULT 
        ComIKPhysicalLogManager::BeginDeleteLogContainerById( 
            __in KTL_DISK_ID DiskId,
            __in KTL_LOG_ID LogId,
            __in IFabricAsyncOperationCallback *const Callback,
            __out IFabricAsyncOperationContext **const Context)
        {
            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogManager::AsyncDeleteLogContainer::SPtr    op;

            NTSTATUS    status = _BackingManager->CreateAsyncDeleteLogContainerContext(op);
            if (!NT_SUCCESS(status))
            {
                 return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter&,
                     KtlLogManager::AsyncDeleteLogContainer& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    Operation.StartDeleteLogContainer(
                        KGuid(DiskId),
                        RvdLogId(LogId),
                        nullptr,
                        AsyncCallback);

                    return STATUS_SUCCESS;
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }
        
        HRESULT 
        ComIKPhysicalLogManager::EndDeleteLogContainer( 
            __in IFabricAsyncOperationContext *const Context)
        {
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogManager::AsyncDeleteLogContainer::SPtr    op;
            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }
        

        //* EnumerateLogContainers()
        HRESULT 
        ComIKPhysicalLogManager::BeginEnumerateLogContainers( 
            __in KTL_DISK_ID DiskId,
            __in IFabricAsyncOperationCallback *const Callback,
            __out IFabricAsyncOperationContext **const Context)
        {
            Callback;
            DiskId;
            *Context = nullptr;
            return Common::ComUtility::OnPublicApiReturn(E_NOTIMPL);
        }
        
        HRESULT 
        ComIKPhysicalLogManager::EndEnumerateLogContainers( 
            __in IFabricAsyncOperationContext *const Context,
            __out IKArray **const Result)
        {
            Context;
            *Result = nullptr;
            return Common::ComUtility::OnPublicApiReturn(E_NOTIMPL);
        }

#if !defined(PLATFORM_UNIX)
        //* GetVolumeIdFromPath()
        struct GetVolumeIdFromPathResults
        {
            KGuid       VolumeId;
        };
        

        HRESULT ComIKPhysicalLogManager::BeginGetVolumeIdFromPath(
            __in LPCWSTR Path,
            __in IFabricAsyncOperationCallback* const Callback,     
            __out IFabricAsyncOperationContext** const Context)
        {
            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;

            KWString    path(GetThisAllocator(), Path);
            NTSTATUS    status = path.Status();
            if (!NT_SUCCESS(status))
            {
                 return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                Callback,
                Context,

                // Start()
                [&] (AsyncCallInAdapter& Adaptor,
                     KAsyncContextBase&, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<GetVolumeIdFromPathResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<GetVolumeIdFromPathResults>&)Adaptor;

                    return KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(
                        path,
                        GetThisAllocator(),
                        adaptor.Extension.VolumeId,
                        AsyncCallback,
                        nullptr);
                },

                // CreateAdaptor()
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<GetVolumeIdFromPathResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT ComIKPhysicalLogManager::EndGetVolumeIdFromPath(
            __in IFabricAsyncOperationContext* const Context,
            __out GUID* const Result)
        {
            if ((Context == nullptr) || (Result == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KAsyncContextBase::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            *Result = ((AsyncCallInAdapter::Extended<GetVolumeIdFromPathResults>&)(*Context)).Extension.VolumeId;
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }
#endif
    }
}


