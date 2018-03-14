// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//************************************************************************************
//
// KtlLogStream Implementation
//
//************************************************************************************
class ShimFileIo : public KObject<ShimFileIo>
{
    public:
		ShimFileIo();
		~ShimFileIo();
		
        class AsyncWriteFileContext : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteFileContext);

            friend ShimFileIo;
            
            public:
                VOID
                StartWriteFile(
                    __in KWString& FileName,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in KSharedAsyncLock::Handle::SPtr& LockHandle,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                enum { Initial, WaitForLock, CreateFile, SetFileSize, WriteFile, Completed, CompletedWithError } _State;

                VOID DoComplete(
                    __in NTSTATUS Status
                    );
                
                VOID WriteFileFSM(
                    __in NTSTATUS Status
                    );

                VOID WriteFileCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                ULONG _AllocationTag;
                
                //
                // Parameters to api
                //
                KWString* _FileName;
                KIoBuffer::SPtr _IoBuffer;

                //
                // Members needed for functionality
                //
                BOOLEAN _OperationLockHeld;
                KSharedAsyncLock::Handle::SPtr _OperationLockHandle;
                KBlockFile::SPtr _File;
        };

        friend ShimFileIo::AsyncWriteFileContext;
        
        static NTSTATUS
        CreateAsyncWriteFileContext(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out AsyncWriteFileContext::SPtr& Context
            );

        
        class AsyncReadFileContext : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncReadFileContext);

            friend ShimFileIo;
            
            public:
                VOID
                StartReadFile(
                    __in KWString& FileName,
                    __out KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                enum { Initial, OpenFile, ReadFile, Completed, CompletedWithError } _State;
                
                VOID ReadFileFSM(
                    __in NTSTATUS Status
                    );


                VOID ReadFileCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                ULONG _AllocationTag;
                
                //
                // Parameters to api
                //
                KWString* _FileName;
                KIoBuffer::SPtr* _IoBufferPtr;

                //
                // Members needed for functionality
                //
                KBlockFile::SPtr _File;
                KIoBuffer::SPtr _IoBuffer;
        };

        friend ShimFileIo::AsyncReadFileContext;
        
        static NTSTATUS
        CreateAsyncReadFileContext(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out AsyncReadFileContext::SPtr& Context
            );

    private:
        ULONG _AllocationTag;            
};
