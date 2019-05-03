// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
/*
 * The header containing our TRACEPOINT_EVENTs.
 */
#define TRACEPOINT_DEFINE
#include "retail/native/FabricCommon/TraceTemplates.Linux.h"

#if !defined(LINUX_DISTRIB_DEBIAN)
static void __attribute__((constructor)) sf_disable_lttng_destructors(void)
{
    tracepoint_disable_destructors();
}
#endif
