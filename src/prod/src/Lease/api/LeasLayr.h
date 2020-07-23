// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


typedef struct _OVERLAPPED_LEASE_LAYER_EXTENSION : OVERLAPPED {

    //
    // User mode.
    //
    HANDLE LeaseAgentHandle;
    HANDLE LeasingApplicationHandle;
    //
    // User mode and kernel mode.
    //
    LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER EventBuffer;
    //
    // User mode only.
    //
    LEASING_APPLICATION_EXPIRED_CALLBACK LeasingApplicationExpired;
    REMOTE_LEASING_APPLICATION_EXPIRED_CALLBACK RemoteLeasingApplicationExpired;
    LEASING_APPLICATION_ARBITRATE_CALLBACK LeasingApplicationArbitrate;
    LEASING_APPLICATION_LEASE_ESTABLISHED_CALLBACK LeasingApplicationLeaseEstablished;
    REMOTE_CERTIFICATE_VERIFY_CALLBACK RemoteCertificateVerify;
    HEALTH_REPORT_CALLBACK HealthReport;
    PVOID CallbackContext;

} OVERLAPPED_LEASE_LAYER_EXTENSION, *POVERLAPPED_LEASE_LAYER_EXTENSION;

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
    HANDLE CloseEvent;
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
