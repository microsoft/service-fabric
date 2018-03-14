// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <dirent.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>

typedef struct _LEASING_APPLICATION_REF_COUNT {

    //
    // Reference count for this leasing application.
    //
    INT Count;
    //
    // Marks that the leasing application is closed.
    //
    BOOL IsClosed;
    //
    // Close event signal.
    //
    std::shared_ptr<Common::ManualResetEvent> CloseEvent;
    //
    // Current Overlapped
    //
    POVERLAPPED_LEASE_LAYER_EXTENSION Overlapped;
    //
    // Lease handle (for tracing)
    //
    HANDLE LeasingApplicationHandle;

} LEASING_APPLICATION_REF_COUNT, *PLEASING_APPLICATION_REF_COUNT;

//
// Leasing application type definitions.
//
typedef std::map<HANDLE, LEASING_APPLICATION_REF_COUNT> LEASING_APPLICATION_REF_COUNT_HASH_TABLE;
typedef std::pair<HANDLE, LEASING_APPLICATION_REF_COUNT> LEASING_APPLICATION_REF_COUNT_HASH_TABLE_ENTRY;
typedef std::map<HANDLE, LEASING_APPLICATION_REF_COUNT>::iterator LEASING_APPLICATION_REF_COUNT_HASH_TABLE_ITERATOR;

class TTLFile;
typedef std::map<HANDLE, std::shared_ptr<TTLFile>> LEASING_APPLICATION_HANDLE_MAP;
