// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER service_fabric

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "retail/native/FabricCommon/dll/TraceTemplates.Linux.h"

#if !defined(TEMPLATE_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define TEMPLATE_H

#if defined(LINUX_DISTRIB_DEBIAN)
// workaround a bug that trace after lttng destruct when process shutdowns
#include <retail/native/FabricCommon/dll/tracepoint.h>
#else
#include <lttng/tracepoint.h>
#endif

TRACEPOINT_EVENT(
    service_fabric,
    tracepoint_silent,
    TP_ARGS(
        char*, taskNameArg,
        char*, eventNameArg,
        char*, idArg,
        char*, dataArg
    ),
    TP_FIELDS(
        ctf_string(taskNameField, taskNameArg)
        ctf_string(eventNameField, eventNameArg)
        ctf_sequence_text(char, idField, idArg, size_t, wcslen(idArg) * 2 + 2)
        ctf_sequence_text(char, dataField, dataArg, size_t, wcslen(dataArg) * 2 + 2)
    )
)
TRACEPOINT_LOGLEVEL(service_fabric, tracepoint_silent, TRACE_ALERT)

TRACEPOINT_EVENT(
    service_fabric,
    tracepoint_critical,
    TP_ARGS(
        char*, taskNameArg,
        char*, eventNameArg,
        char*, idArg,
        char*, dataArg
    ),
    TP_FIELDS(
        ctf_string(taskNameField, taskNameArg)
        ctf_string(eventNameField, eventNameArg)
        ctf_sequence_text(char, idField, idArg, size_t, wcslen(idArg) * 2 + 2)
        ctf_sequence_text(char, dataField, dataArg, size_t, wcslen(dataArg) * 2 + 2)
    )
)
TRACEPOINT_LOGLEVEL(service_fabric, tracepoint_critical, TRACE_CRIT)

TRACEPOINT_EVENT(
    service_fabric,
    tracepoint_error,
    TP_ARGS(
        char*, taskNameArg,
        char*, eventNameArg,
        char*, idArg,
        char*, dataArg
    ),
    TP_FIELDS(
        ctf_string(taskNameField, taskNameArg)
        ctf_string(eventNameField, eventNameArg)
        ctf_sequence_text(char, idField, idArg, size_t, wcslen(idArg) * 2 + 2)
        ctf_sequence_text(char, dataField, dataArg, size_t, wcslen(dataArg) * 2 + 2)
    )
)
TRACEPOINT_LOGLEVEL(service_fabric, tracepoint_error, TRACE_ERR)

TRACEPOINT_EVENT(
    service_fabric,
    tracepoint_warning,
    TP_ARGS(
        char*, taskNameArg,
        char*, eventNameArg,
        char*, idArg,
        char*, dataArg
    ),
    TP_FIELDS(
        ctf_string(taskNameField, taskNameArg)
        ctf_string(eventNameField, eventNameArg)
        ctf_sequence_text(char, idField, idArg, size_t, wcslen(idArg) * 2 + 2)
        ctf_sequence_text(char, dataField, dataArg, size_t, wcslen(dataArg) * 2 + 2)
    )
)
TRACEPOINT_LOGLEVEL(service_fabric, tracepoint_warning, TRACE_WARNING)

TRACEPOINT_EVENT(
    service_fabric,
    tracepoint_info,
    TP_ARGS(
        char*, taskNameArg,
        char*, eventNameArg,
        char*, idArg,
        char*, dataArg
    ),
    TP_FIELDS(
        ctf_string(taskNameField, taskNameArg)
        ctf_string(eventNameField, eventNameArg)
        ctf_sequence_text(char, idField, idArg, size_t, wcslen(idArg) * 2 + 2)
        ctf_sequence_text(char, dataField, dataArg, size_t, wcslen(dataArg) * 2 + 2)
    )
)
TRACEPOINT_LOGLEVEL(service_fabric, tracepoint_info, TRACE_INFO)

TRACEPOINT_EVENT(
    service_fabric,
    tracepoint_noise,
    TP_ARGS(
        char*, taskNameArg,
        char*, eventNameArg,
        char*, idArg,
        char*, dataArg
    ),
    TP_FIELDS(
        ctf_string(taskNameField, taskNameArg)
        ctf_string(eventNameField, eventNameArg)
        ctf_sequence_text(char, idField, idArg, size_t, wcslen(idArg) * 2 + 2)
        ctf_sequence_text(char, dataField, dataArg, size_t, wcslen(dataArg) * 2 + 2)
    )
)
TRACEPOINT_LOGLEVEL(service_fabric, tracepoint_noise, TRACE_DEBUG_SYSTEM)

TRACEPOINT_EVENT(
    service_fabric,
    tracepoint_all,
    TP_ARGS(
        char*, taskNameArg,
        char*, eventNameArg,
        char*, idArg,
        char*, dataArg
    ),
    TP_FIELDS(
        ctf_string(taskNameField, taskNameArg)
        ctf_string(eventNameField, eventNameArg)
        ctf_sequence_text(char, idField, idArg, size_t, wcslen(idArg) * 2 + 2)
        ctf_sequence_text(char, dataField, dataArg, size_t, wcslen(dataArg) * 2 + 2)
    )
)
TRACEPOINT_LOGLEVEL(service_fabric, tracepoint_all, TRACE_DEBUG_LINE)


#endif /* TEMPLATE_H */

#include <lttng/tracepoint-event.h>
