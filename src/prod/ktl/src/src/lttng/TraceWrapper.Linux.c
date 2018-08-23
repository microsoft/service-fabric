//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "TraceWrapper.Linux.h"
#include "TraceTemplates.Linux.h"

void LttngTraceWrapper(
	const char * taskName,
	const char * eventName,
	int level,
	const char * id,
	const char * data)
{
    switch (level)
    {
        case KTRACE_LEVEL_SILENT:
            tracepoint(service_fabric,
                tracepoint_silent,
                (char*) taskName,
                (char*) eventName,
                (char*) id,
                (char*) data);
            break;
        case KTRACE_LEVEL_CRITICAL:
            tracepoint(service_fabric,
                tracepoint_critical,
                (char*) taskName,
                (char*) eventName,
                (char*) id,
                (char*) data);
        case KTRACE_LEVEL_ERROR:
            tracepoint(service_fabric,
                tracepoint_error,
                (char*) taskName,
                (char*) eventName,
                (char*) id,
                (char*) data);
            break;
        case KTRACE_LEVEL_WARNING:
            tracepoint(service_fabric,
                tracepoint_warning,
                (char*) taskName,
                (char*) eventName,
                (char*) id,
                (char*) data);
            break;
        case KTRACE_LEVEL_INFO:
            tracepoint(service_fabric,
                tracepoint_info,
                (char*) taskName,
                (char*) eventName,
                (char*) id,
                (char*) data);
            break;
        case KTRACE_LEVEL_VERBOSE:
            tracepoint(service_fabric,
                tracepoint_noise,
                (char*) taskName,
                (char*) eventName,
                (char*) id,
                (char*) data);
            break;
        case 99:
            tracepoint(service_fabric,
                       tracepoint_all,
                       (char*) taskName,
                       (char*) eventName,
                       (char*) id,
                       (char*) data);
            break;
        default:
            // Unknown trace loglevel
            tracepoint(service_fabric,
                       tracepoint_warning,
                       (char*) taskName,
                       (char*) eventName,
                       (char*) id,
                       (char*) data);
    }
}
