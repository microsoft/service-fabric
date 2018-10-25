// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <babeltrace/context.h>
#include <babeltrace/ctf/events.h>
#include <evntprov.h>

int read_unstructured_event(
    const struct bt_ctf_event *event,
    uint64_t &timestamp_epoch,
    wchar_t *event_string_representation,
    char *taskname_eventname);

int read_structured_event(
    const struct bt_ctf_event *event,
    PEVENT_RECORD eventRecord,
    uint64_t &epoch_timestamp);
