// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#include <ktllogger.h>
#include <klogicallog.h>

#include <bldver.h>

#include "ktlloggertests.h"
#include "LWTtests.h"

#include "rwtstress.h"
#include "WexTestClass.h"

#include "CloseSync.h"

#include "ControllerHost.h"
#include "llworkload.h"

#include "workerasync.h"

using namespace WEX::Logging;
using namespace WEX::Common;
using namespace WEX::TestExecution;

#include "VerifyQuiet.h"

#include "TestUtil.h"

#if KTL_USER_MODE
 #define _printf(...)   printf(__VA_ARGS__)
// #define _printf(...)   KDbgPrintf(__VA_ARGS__)

extern volatile LONGLONG gs_AllocsRemaining;
#else
 #define _printf(...)   KDbgPrintf(__VA_ARGS__)
 #define wprintf(...)    KDbgPrintf(__VA_ARGS__)
#endif

#define ALLOCATION_TAG 'LLKT'


//
// This test will allow any number of streams to perform a long running
// workload. The workload will write random size data and truncate
// periodically.
//
// Future iterations may check the size of the log to ensure it is not
// too large, perform record reads while writing, etc.
//

#include "LWTAsync.h"

VOID LongRunningWriteTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    const ULONG NumberStreams = 8;
    
    const ULONG NumberWriteStress = 8;
    const ULONG MaxWriteRecordSize = 128 * 1024;
    const ULONGLONG AllowedLogSpace = 0xC000000;
    const ULONGLONG MaxWriteAsn = AllowedLogSpace * 64;
	const LONGLONG StreamSize = MaxWriteAsn / 2;
    LongRunningWriteStress::SPtr writeStress[NumberWriteStress];
    KSynchronizer writeSyncs[NumberWriteStress];

    
    KtlLogStreamId logStreamId[NumberStreams];
    KtlLogStream::SPtr logStream[NumberStreams];


    NTSTATUS status;
    KSynchronizer sync;

    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogContainer::SPtr logContainer;
    
    KGuid logStreamGuid;
    ULONG metadataLength = 0x10000;
    StreamCloseSynchronizer closeStreamSync;

        
    //
    // Create container and a stream within it
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = 0x200000000;  // 8GB

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createContainerAsync->StartCreateLogContainer(diskId,
        logContainerId,
        logSize,
        0,            // Max Number Streams
        0,            // Max Record Size
        0,
        logContainer,
        NULL,         // ParentAsync
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KBuffer::SPtr securityDescriptor = nullptr;


    for (ULONG i = 0; i < NumberStreams; i++)
    {
        logStreamGuid.CreateNew();
        logStreamId[i] = static_cast<KtlLogStreamId>(logStreamGuid);

        createStreamAsync->Reuse();
        createStreamAsync->StartCreateLogStream(logStreamId[i],
            KLogicalLogInformation::GetLogicalLogStreamType(),
            nullptr,           // Alias
            nullptr,
            securityDescriptor,
            metadataLength,
            StreamSize,
            1024*1024,  // 1MB
            KtlLogManager::FlagSparseFile,
            logStream[i],
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    for (ULONG i = 0; i < NumberWriteStress; i++)
    {
        LongRunningWriteStress::StartParameters params;

        status = LongRunningWriteStress::Create(*g_Allocator,
                                                     KTL_TAG_TEST,
                                                     writeStress[i]);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        params.LogStream = logStream[i];
        params.MaxRandomRecordSize = MaxWriteRecordSize;
        params.LogSpaceAllowed = AllowedLogSpace;
        params.HighestAsn = MaxWriteAsn;
		params.WaitTimerInMs = 0;
        writeStress[i]->StartIt(&params,
                                NULL,
                                writeSyncs[i]);
    }

	ULONG completed = NumberWriteStress;

	while (completed > 0)
	{
		for (ULONG i = 0; i < NumberWriteStress; i++)
		{
			status = writeSyncs[i].WaitForCompletion(60 * 1000);

			if (status == STATUS_IO_TIMEOUT)
			{
				printf("writeStress[%d]: %I64x Version, %I64x Asn, %I64x TruncationAsn\n", i,
					   writeStress[i]->GetVersion(), writeStress[i]->GetAsn(), writeStress[i]->GetTruncationAsn());
			} else {				
				VERIFY_IS_TRUE(NT_SUCCESS(status));
				completed--;
			}
		}
	}
    

    for (ULONG i = 0; i < NumberStreams; i++)
    {
        logStream[i]->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
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

