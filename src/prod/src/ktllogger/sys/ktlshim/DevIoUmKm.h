// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

class FileObjectTable;

class OpenCloseDriver : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(OpenCloseDriver);

    public:
        static NTSTATUS Create(
            __out OpenCloseDriver::SPtr& Context,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );
        
        virtual VOID StartOpenDevice(
            __in const KStringView& FileName,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        ) = 0;
            
        virtual VOID StartWaitForCloseDevice(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        ) = 0;
        
        virtual VOID SignalCloseDevice(
        ) = 0;
        
        virtual VOID PostCloseCompletion(
        ) = 0;       

        virtual FileObjectTable* GetFileObjectTable(
        ) = 0;

#ifdef UDRIVER
		virtual PVOID GetKernelPointerFromObjectId(
		    __in ULONGLONG ObjectId
        ) = 0;		
#endif
};

class DevIoControlUser : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(DevIoControlUser);

    public:
        static NTSTATUS Create(
            __out DevIoControlUser::SPtr& Context,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );
        
        virtual VOID StartDeviceIoControl(
            __in OpenCloseDriver::SPtr& OpenCloseHandle,
            __in ULONG Ioctl,
            __in KBuffer::SPtr InKBuffer,
            __in ULONG InBufferSize,
            __in_opt KBuffer::SPtr OutKBuffer,
            __in ULONG OutBufferSize,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        ) = 0;
            
        virtual ULONG GetOutBufferSize() = 0;

        virtual VOID SetRetryCount(
            __in ULONG MaxRetries
        ) = 0;
        
    public:
        KBuffer::SPtr _KBuffer;
        
};


class DevIoControlKernel abstract : public KObject<DevIoControlKernel>, public KShared<DevIoControlKernel>
{
    K_FORCE_SHARED_WITH_INHERITANCE(DevIoControlKernel);

    public:
        virtual ULONG GetInBufferSize() = 0;
        virtual ULONG GetOutBufferSize() = 0;
        virtual PUCHAR GetBuffer() = 0;

        virtual VOID Initialize(
            __in PUCHAR Buffer,
            __in ULONG InBufferSize,
            __in ULONG OutBufferSize
        ) = 0;
        
        virtual VOID CompleteRequest(
            __in NTSTATUS Status,
            __in ULONG OutBufferSize
        ) = 0;
};
