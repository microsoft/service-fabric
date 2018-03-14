// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

VOID
RWTStress(
	KAllocator& Allocator,
    KGuid diskId,
    KtlLogManager::SPtr logManager,
	ULONG numberAsyncs = 64,
	ULONG numberRepeats = 8,
	ULONG numberRecordsPerIteration = 8,
	ULONG numberSizeOfRecord = 4 * 0x100000  // 4MB    
    );
