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
        //*** IKPhysicalLogContainer COM Implementation
        ComIKPhysicalLogContainer::ComIKPhysicalLogContainer(KtlLogContainer& Containter)
            :   _BackingContainer(&Containter)
        {
        }

        ComIKPhysicalLogContainer::~ComIKPhysicalLogContainer()
        {
        }

        HRESULT 
        ComIKPhysicalLogContainer::Create(
            __in KAllocator& Allocator,  
            __in KtlLogContainer& Containter, 
            __out ComIKPhysicalLogContainer::SPtr& Result)
        {
            Result = _new(KTL_TAG_LOGGER, Allocator) ComIKPhysicalLogContainer(Containter);
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

        //** IsFuntional
        HRESULT 
        ComIKPhysicalLogContainer::IsFunctional()
        {
            return Common::ComUtility::OnPublicApiReturn(_BackingContainer->IsFunctional() ? S_OK : S_FALSE);
        }


        //** CreateLogStream
        struct LogStreamOpResults
        {
            KtlLogStream::SPtr  LogStream;
            ULONG               MaxMetadataSize;
        };
        

        RvdLogStreamType const&
        ComIKPhysicalLogContainer::KtlDefaultStreamType()
        {
            // {B81BF9BA-CDA3-45ae-ADBB-09CAC3B11C01}
            static GUID const WfLLStreamType = { 0xb81bf9ba, 0xcda3, 0x45ae, { 0xad, 0xbb, 0x9, 0xca, 0xc3, 0xb1, 0x1c, 0x1 } };
            return *(reinterpret_cast<RvdLogStreamType const*>(&WfLLStreamType));
        }        

        HRESULT 
        ComIKPhysicalLogContainer::BeginCreateLogStream(
            __in KTL_LOG_STREAM_ID LogStreamId,
            __in KTL_LOG_STREAM_TYPE_ID LogStreamTypeId,
            __in_opt LPCWSTR OptionalLogStreamAlias,
            __in LPCWSTR Path,
            __in_opt IKBuffer* const OptionalSecurityInfo,
            __in LONGLONG MaximumSize,
            __in ULONG MaximumBlockSize,
            __in DWORD CreationFlags,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context)
        {
            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr    op;

            NTSTATUS    status = _BackingContainer->CreateAsyncCreateLogStreamContext(op);
            if (!NT_SUCCESS(status))
            {
                 return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            KString::SPtr       alias = KString::Create(OptionalLogStreamAlias, GetThisAllocator(), TRUE);
            KString::SPtr       path = KString::Create(Path, GetThisAllocator(), TRUE);
            if (((alias == nullptr) && (OptionalLogStreamAlias != nullptr)) || 
                ((path == nullptr) && (Path != nullptr)))
            {
                return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
            }

            KString::CSPtr aliasCSPtr = alias.RawPtr();
            KString::CSPtr pathCSPtr = path.RawPtr();

            if (KGuid(LogStreamTypeId).IsNull())
            {
                LogStreamTypeId = ComIKPhysicalLogContainer::KtlDefaultStreamType().Get();
            }

            KBuffer::SPtr       securityInfo;
            if (OptionalSecurityInfo != nullptr)
            {
                securityInfo = &((ComKBuffer*)OptionalSecurityInfo)->GetBackingBuffer();
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KtlLogContainer::AsyncCreateLogStreamContext& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<LogStreamOpResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<LogStreamOpResults>&)Adaptor;

                    Operation.StartCreateLogStream(
                        RvdLogStreamId(LogStreamId),
                        RvdLogStreamType(LogStreamTypeId),
                        aliasCSPtr,
                        pathCSPtr,
                        securityInfo,
                        0,
                        MaximumSize,
                        MaximumBlockSize,
                        CreationFlags,
                        adaptor.Extension.LogStream,
                        nullptr,
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom LogStreamOpResults adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<LogStreamOpResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        template <typename OpType>
        HRESULT 
        ComIKPhysicalLogContainer::CommonEndOpenLogStream(
            __in IFabricAsyncOperationContext* Context,
            __out IKPhysicalLogStream** Result)
        {
            if ((nullptr == Context) || (Result == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Result = nullptr;
            typename OpType::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            ComIKPhysicalLogStream::SPtr     sptr;

            hr = ComIKPhysicalLogStream::Create(
                GetThisAllocator(),
                *((AsyncCallInAdapter::Extended<LogStreamOpResults>&)(*Context)).Extension.LogStream,
                sptr);

            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            *Result = sptr.Detach();
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }


        HRESULT 
        ComIKPhysicalLogContainer::EndCreateLogStream(
            __in IFabricAsyncOperationContext* Context,
            __out IKPhysicalLogStream** Result) 
        {
            return CommonEndOpenLogStream<KtlLogContainer::AsyncCreateLogStreamContext>(
                Context,
                Result);
        }


        //** OpenLogStream
        HRESULT 
        ComIKPhysicalLogContainer::BeginOpenLogStream(
            __in KTL_LOG_STREAM_ID LogStreamId,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context) 
        {
            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr    op;

            NTSTATUS    status = _BackingContainer->CreateAsyncOpenLogStreamContext(op);
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
                     KtlLogContainer::AsyncOpenLogStreamContext& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<LogStreamOpResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<LogStreamOpResults>&)Adaptor;

                    Operation.StartOpenLogStream(
                        RvdLogStreamId(LogStreamId),
                        &adaptor.Extension.MaxMetadataSize,
                        adaptor.Extension.LogStream,
                        nullptr,
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom LogStreamOpResults adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<LogStreamOpResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogContainer::EndOpenLogStream(
            __in IFabricAsyncOperationContext* Context,
            __out IKPhysicalLogStream** Result) 
        {
            return CommonEndOpenLogStream<KtlLogContainer::AsyncOpenLogStreamContext>(
                Context,
                Result);
        }


        //** DeleteLogStream
        HRESULT 
        ComIKPhysicalLogContainer::BeginDeleteLogStream(
            __in KTL_LOG_STREAM_ID LogStreamId,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context) 
        {
            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr    op;

            NTSTATUS    status = _BackingContainer->CreateAsyncDeleteLogStreamContext(op);
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
                     KtlLogContainer::AsyncDeleteLogStreamContext& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    Operation.StartDeleteLogStream(RvdLogStreamId(LogStreamId), nullptr, AsyncCallback);
                    return STATUS_SUCCESS;
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogContainer::EndDeleteLogStream(
            __in IFabricAsyncOperationContext* Context) 
        {
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr    op;
            return Common::ComUtility::OnPublicApiReturn(AsyncCallInAdapter::End(Context, op));
        }


        //** AssignAlias
        HRESULT 
        ComIKPhysicalLogContainer::BeginAssignAlias(
            __in LPCWSTR Alias,
            __in KTL_LOG_STREAM_ID LogStreamId,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context) 
        {
            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogContainer::AsyncAssignAliasContext::SPtr    op;

            NTSTATUS    status = _BackingContainer->CreateAsyncAssignAliasContext(op);
            if (!NT_SUCCESS(status))
            {
                 return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            KString::SPtr       alias = KString::Create(Alias, GetThisAllocator(), TRUE);
            if (alias == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
            }

            KString::CSPtr aliasCSPtr = alias.RawPtr();

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter&,
                     KtlLogContainer::AsyncAssignAliasContext& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    Operation.StartAssignAlias(
                        aliasCSPtr,
                        RvdLogStreamId(LogStreamId),
                        nullptr,
                        AsyncCallback);

                    return STATUS_SUCCESS;
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogContainer::EndAssignAlias(
            __in IFabricAsyncOperationContext* Context) 
        {
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogContainer::AsyncAssignAliasContext::SPtr    op;
            return Common::ComUtility::OnPublicApiReturn(AsyncCallInAdapter::End(Context, op));
        }


        //** RemoveAlias
        HRESULT 
        ComIKPhysicalLogContainer::BeginRemoveAlias(
            __in LPCWSTR Alias,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context) 
        {
            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogContainer::AsyncRemoveAliasContext::SPtr    op;

            NTSTATUS    status = _BackingContainer->CreateAsyncRemoveAliasContext(op);
            if (!NT_SUCCESS(status))
            {
                 return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            KString::SPtr       alias = KString::Create(Alias, GetThisAllocator(), TRUE);
            if (alias == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
            }

            KString::CSPtr aliasCSPtr = alias.RawPtr();

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter&,
                     KtlLogContainer::AsyncRemoveAliasContext& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    Operation.StartRemoveAlias(
                        aliasCSPtr,
                        nullptr,
                        AsyncCallback);

                    return STATUS_SUCCESS;
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogContainer::EndRemoveAlias(
            __in IFabricAsyncOperationContext* Context) 
        {
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogContainer::AsyncRemoveAliasContext::SPtr    op;
            return Common::ComUtility::OnPublicApiReturn(AsyncCallInAdapter::End(Context, op));
        }


        //** ResolveAlias
        struct ResolveAliasOpResults
        {
            RvdLogStreamId   Id;
        };

        HRESULT 
        ComIKPhysicalLogContainer::BeginResolveAlias(
            __in LPCWSTR Alias,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context) 
        {
            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogContainer::AsyncResolveAliasContext::SPtr    op;

            NTSTATUS    status = _BackingContainer->CreateAsyncResolveAliasContext(op);
            if (!NT_SUCCESS(status))
            {
                 return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            KString::SPtr       alias = KString::Create(Alias, GetThisAllocator(), TRUE);
            if (alias == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
            }

            KString::CSPtr aliasCSPtr = alias.RawPtr();

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KtlLogContainer::AsyncResolveAliasContext& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<ResolveAliasOpResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<ResolveAliasOpResults>&)Adaptor;

                    Operation.StartResolveAlias(
                        aliasCSPtr,
                        &adaptor.Extension.Id,
                        nullptr,
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom ResolveAliasOpResults adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<ResolveAliasOpResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogContainer::EndResolveAlias(
            __in IFabricAsyncOperationContext* Context,
            __out KTL_LOG_STREAM_ID* const Result) 
        {
            if ((nullptr == Context) || (Result == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogContainer::AsyncResolveAliasContext::SPtr    op;
            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            *Result = ((AsyncCallInAdapter::Extended<ResolveAliasOpResults>&)(*Context)).Extension.Id.Get();
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }
    

        //** CloseContainer
        struct CloseContainerResults
        {
            class FakeAsync : public KAsyncContextBase
            {
            public:
                using KObject<KAsyncContextBase>::SetStatus;
            };

            KSharedPtr<FakeAsync>                   _FakeCloseOp;
            KAsyncContextBase::CompletionCallback   _OuterCallback;

            VOID CloseCallbackIntercept(
                KAsyncContextBase* const Parent,
                KtlLogContainer& Container,
                NTSTATUS Status)
            {
                UNREFERENCED_PARAMETER(Container);

                _FakeCloseOp->SetStatus(Status);
                _OuterCallback(Parent, *_FakeCloseOp);
            }
        };

        HRESULT 
        ComIKPhysicalLogContainer::BeginClose(
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context) 
        {
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KSharedPtr<CloseContainerResults::FakeAsync>    op;

            op = _new(GetThisAllocationTag(), GetThisAllocator()) CloseContainerResults::FakeAsync();
            if (op == nullptr)
            {
                 return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KAsyncContextBase&, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<CloseContainerResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<CloseContainerResults>&)Adaptor;

                    adaptor.Extension._FakeCloseOp = Ktl::Move(op);
                    adaptor.Extension._OuterCallback = AsyncCallback;
                    return  _BackingContainer->StartClose(
                        nullptr, 
                        KtlLogContainer::CloseCompletionCallback(
                            &adaptor.Extension, 
                            &CloseContainerResults::CloseCallbackIntercept));
                },

                // Custom Adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<CloseContainerResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogContainer::EndClose(
            __in IFabricAsyncOperationContext* Context) 
        {
            _BackingContainer = nullptr;

            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KAsyncContextBase::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }
    }
}


