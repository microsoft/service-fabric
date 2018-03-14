// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if defined(PLATFORM_UNIX)
#include <boost/test/unit_test.hpp>
#include "boost-taef.h"
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#include <KtlLogShimUser.h>
#include <KtlLogShimKernel.h>
#include <KLogicalLog.h>

#include "KtlLoggerTests.h"

#if !defined(PLATFORM_UNIX)
#include "WexTestClass.h"
#endif

#include "CloseSync.h"

#include "AsyncInterceptor.h"

#define ALLOCATION_TAG 'LLKT'
 
extern KAllocator* g_Allocator;

#include "VerifyQuiet.h"



#if defined(UDRIVER)
//

CompleteOnStartInterceptor<OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext>::SPtr FailCoalesceFlushInterceptor;

VOID
FailCoalesceFlushTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);
    
    //
    // Create container and a stream within it
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;
        KtlLogContainer::SPtr logContainer;
        KSynchronizer sync;
        KGuid logStreamGuid;
        KtlLogStreamId logStreamId;

        logStreamGuid.CreateNew();
        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);
        
        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength = 0x10000;
            
            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            KBuffer::SPtr securityDescriptor = nullptr;
            KtlLogStreamType logStreamType = KLogicalLogInformation::GetLogicalLogStreamType();

            createStreamAsync->StartCreateLogStream(logStreamId,
                                                    logStreamType,
                                                    nullptr,           // Alias
                                                    nullptr,
                                                    securityDescriptor,
                                                    metadataLength,
                                                    DEFAULT_STREAM_SIZE,
                                                    DEFAULT_MAX_RECORD_SIZE,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
          
            //
            // Verify that log stream id is set correctly
            //
            KtlLogStreamId queriedLogStreamId;
            logStream->QueryLogStreamId(queriedLogStreamId);

            VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
			logStream = nullptr;
        }

        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;
            
            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));


            //
            // Create an interceptor for the closing flush for the
            // coalesce records and then apply it. Once applied close
            // the stream and verify it doesn't stop responding.
            //
            status = CompleteOnStartInterceptor<OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext>::Create(KTL_TAG_TEST,
                                                        *g_Allocator,
                                                        STATUS_INSUFFICIENT_RESOURCES,
                                                        FailCoalesceFlushInterceptor);
            VERIFY_IS_TRUE(NT_SUCCESS(status));


            KtlLogStreamUser* logStreamUser = static_cast<KtlLogStreamUser*>(logStream.RawPtr());
            KtlLogStreamKernel* logStreamKernel = static_cast<KtlLogStreamKernel*>(logStreamUser->GetKernelPointer());
            OverlayStream* overlayStream = logStreamKernel->GetOverlayStream();
            OverlayStream::CoalesceRecords* coalesceRecords = (overlayStream->GetCoalesceRecords()).RawPtr();
            OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext* closeFlushContext = coalesceRecords->GetCloseFlushContext();

            FailCoalesceFlushInterceptor->EnableIntercept(*closeFlushContext);
            
            
            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            // Allow time for the stream to close and the coalesce
            // flush async to finish up.
            KNt::Sleep(1000);
            
            FailCoalesceFlushInterceptor->DisableIntercept();
            FailCoalesceFlushInterceptor = nullptr;
            
            status = closeStreamSync.WaitForCompletion(60 * 1000);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
			logStream = nullptr;
        }
        
        //
        // Delete the stream
        //
        {
            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;
            
            status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            deleteStreamAsync->StartDeleteLogStream(logStreamId,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));            
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;
        
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }
}
//
#endif
