// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#if !defined(_LEASELISTENENDPOINT_H_)
#define _LEASELISTENENDPOINT_H_

#define SHA1_LENGTH 20

// DNS allows up to 255 UTF8 characters, we need to allow 255 UTF16 characters locally
#define ENDPOINT_ADDR_CCH_MAX 256 // 255 + 1

// SSPI target size limit is the max of the following two: 
// 1. Security principal name: max length is 64 (domain) + 1 (\) + 20 (username)  + 1 (null) = 85
// 2. Service principal name: max length is 260 (null terminator included)
#define SSPI_TARGET_MAX 260 

// the length of the alias of testing transport rule
#define TEST_TRANSPORT_ALIAS_LENGTH 128 

typedef struct
{
    // Address can be either numerical IP v4 or v6 address string, or hostname string.
    WCHAR Address[ENDPOINT_ADDR_CCH_MAX];

    // when Address is hostname instead of numerical address, resolve type can be:
    //   AF_UNSPEC : unspecified
    //   AF_INET   : IP v4
    //   AF_INET6  : IP v6
    ADDRESS_FAMILY ResolveType;

    unsigned short Port;

    // SspiTarget is the target identity for authenticating listener. Currently
    // it is only used for Kerberos.The value could be logon name, upn or spn.
    WCHAR SspiTarget[SSPI_TARGET_MAX];
} TRANSPORT_LISTEN_ENDPOINT, *PTRANSPORT_LISTEN_ENDPOINT;

typedef const TRANSPORT_LISTEN_ENDPOINT * PCTRANSPORT_LISTEN_ENDPOINT;

typedef struct
{
    // Maintenance interval
    LONG MaintenanceInterval;
    // Process assert exit timeout
    LONG ProcessAssertExitTimeout;
    // Delay lease agent close interval
    LONG DelayLeaseAgentCloseInterval;

} LEASE_GLOBAL_CONFIG, *PLEASE_GLOBAL_CONFIG;

// This is defined here because it is used in both user mode LeaseAgent, lease layer dll and kerenl lease agent
typedef struct
{
    // Regular lease duration in Federation config
    LONG LeaseDuration;
    // Lease duration for nodes across fault domain or DC
    LONG LeaseDurationAcrossFD;
    // User mode unresponsive duration
    LONG UnresponsiveDuration;
    // Longest duration that lease renewed indirectly
    LONG MaxIndirectLeaseTimeout;

} LEASE_CONFIG_DURATIONS, *PLEASE_CONFIG_DURATIONS;

typedef enum
{
    LEASE_DURATION,
    LEASE_DURATION_ACROSS_FAULT_DOMAIN,
    UNRESPONSIVE_DURATION

} LEASE_DURATION_TYPE, *PLEASE_DURATION_TYPE;

//
// Blocking lease action, test purpose only
//
typedef enum _LEASE_BLOCKING_ACTION_TYPE {

    LEASE_ESTABLISH_ACTION,
    LEASE_ESTABLISH_RESPONSE,
    LEASE_PING_RESPONSE,
    LEASE_PING_REQUEST,
    LEASE_INDIRECT,
    LEASE_BLOCKING_ALL //it blocks all above types

} LEASE_BLOCKING_ACTION_TYPE, *PLEASE_BLOCKING_ACTION_TYPE;

//
// Block lease connection IOCTL input buffer.
//
typedef struct _TRANSPORT_BEHAVIOR_BUFFER {

    //
    // Lease agent listening socket address (input).
    //
    TRANSPORT_LISTEN_ENDPOINT LocalSocketAddress;
    //
    // From any source address
    //
    BOOLEAN FromAny;
    //
    // Remote listening address of a lease agent.
    //
    TRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress;
    //
    // From to any destination address
    //
    BOOLEAN ToAny;
    //
    // Lease agent listening socket address (input).
    //
    LEASE_BLOCKING_ACTION_TYPE BlockingType;
    //
    // A name used for lookup or delete
    //
    WCHAR Alias[TEST_TRANSPORT_ALIAS_LENGTH];
} TRANSPORT_BEHAVIOR_BUFFER, *PTRANSPORT_BEHAVIOR_BUFFER;

// Max char count of lease agent identifier
#define LEASE_AGENT_ID_CCH_MAX (ENDPOINT_ADDR_CCH_MAX + 1/*:*/ + 5/*port in decimal*/)

#endif  // _LEASELISTENENDPOINT_H_
