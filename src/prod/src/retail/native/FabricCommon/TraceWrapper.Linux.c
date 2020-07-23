// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "TraceWrapper.Linux.h"
#include "TraceTemplates.Linux.h"

void TraceWrapper(
        const char * taskName,
        const char * eventName,
        int level,
        const char * id,
        const char * data)
{
    switch (level)
    {
        case 0:
            tracepoint(service_fabric,
                    tracepoint_silent,
                    (char*) taskName,
                    (char*) eventName,
                    (char*) id,
                    (char*) data);
            break;
        case 1:
            tracepoint(service_fabric,
                    tracepoint_critical,
                    (char*) taskName,
                    (char*) eventName,
                    (char*) id,
                    (char*) data);
        case 2:
            tracepoint(service_fabric,
                    tracepoint_error,
                    (char*) taskName,
                    (char*) eventName,
                    (char*) id,
                    (char*) data);
            break;
        case 3:
            tracepoint(service_fabric,
                    tracepoint_warning,
                    (char*) taskName,
                    (char*) eventName,
                    (char*) id,
                    (char*) data);
            break;
        case 4:
            tracepoint(service_fabric,
                    tracepoint_info,
                    (char*) taskName,
                    (char*) eventName,
                    (char*) id,
                    (char*) data);
            break;
        case 5:
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
            printf("Unknown trace loglevel %d\n", level);
    }
}
