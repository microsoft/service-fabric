// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <cstdint>
#include "lttngreader_statuscode.h"

struct bt_ctf_event;
struct bt_iter_pos;
struct LTTngTraceHandle;
typedef struct _EVENT_RECORD *PEVENT_RECORD;

LttngReaderStatusCode initialize_trace(
        LTTngTraceHandle &trace_handle,
        uint64_t start_time,
        uint64_t end_time);

LttngReaderEventReadStatusCode read_unstructured_event(
    const struct bt_ctf_event *event,
    uint64_t &timestamp_epoch,
    wchar_t *event_string_representation,
    char *taskname_eventname);

LttngReaderEventReadStatusCode read_structured_event(
    const struct bt_ctf_event *event,
    PEVENT_RECORD eventRecord,
    uint64_t &epoch_timestamp,
    unsigned char *data_buffer);

LttngReaderEventReadStatusCode read_event_timestamp(
    const struct bt_ctf_event *event,
    uint64_t &timestamp_epoch);

bt_iter_pos* create_bt_iter_pos(
        uint64_t timestamp_epoch);
