// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define LOG_OUTPUT(fmt, ...) Log::Comment(String().Format(fmt, __VA_ARGS__))

extern KAllocator* g_Allocator;

VOID
LongRunningWriteTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );
