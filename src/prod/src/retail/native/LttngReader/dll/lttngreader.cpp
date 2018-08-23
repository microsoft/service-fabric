// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <iostream>
#include <lttng/lttng.h>
#include <babeltrace/ctf/iterator.h>
#include <babeltrace/iterator.h>
#include "lttngreader_util.h"

// TODO - remove just for debugging
#include <cstdio>

// initializes the trace processing by creating context, associating a folder and getting the iterator for the trace events.
// returns the trace handle id
extern "C" int initialize_trace_processing(const char* trace_folder_path, struct bt_context **ctx, bt_ctf_iter **ctf_iter, const bool seek_timestamp, const uint64_t timestamp_filetime)
{
    // creating context
    *ctx = bt_context_create();

    // adding trace to context
    int t_handle_id = bt_context_add_traces_recursive(*ctx, trace_folder_path, "ctf", NULL);

    if (t_handle_id < 0)
    {
        return t_handle_id;
    }

    // starting reading traces from beginning
    if (!seek_timestamp)
    {
        // getting iterator for trace
        *ctf_iter =  bt_ctf_iter_create(*ctx, NULL, NULL);
    }
    else
    {
        // getting iterator at a timestamp
        bt_iter_pos iter_pos;
        iter_pos.type = BT_SEEK_TIME;

        // DCA's code is in Windows' filetime so we need to convert it first to epoch.
        iter_pos.u.seek_time = timestamp_filetime; //get_epoch_from_filetime(timestamp_filetime);

        *ctf_iter = bt_ctf_iter_create(*ctx, &iter_pos, NULL);
    }

    if (ctf_iter == NULL)
    {
        std::cerr << "Unable to create iterator. A iterator may already exist for the context" << std::endl;
        return -1;
    }

    return t_handle_id;
}

// cleans up by destroying the iterators, removing the trace from the context and freeing the context.
extern "C" int finalize_trace_processing(struct bt_context *ctx, bt_ctf_iter *ctf_iter)
{
    // destroying iterators
    if (ctf_iter != NULL)
    {
        bt_ctf_iter_destroy(ctf_iter);
    }

    // freeing context
    bt_context_put(ctx);

    return 0;
}

extern "C" int read_next_event(PEVENT_RECORD eventRecord, wchar_t *unstructured_event_text, char *taskname_eventname, bt_ctf_iter *ctf_iter, uint64_t &timestamp_epoch, bool &is_unstructured)
{
    int err;

    // reading event
    bt_ctf_event *event = bt_ctf_iter_read_event(ctf_iter);

    if (event == NULL)
    {
        return -2;
    }

    // deciding whether it is an unstructured trace or structured
    if (strncmp("service_fabric:tracepoint_structured", bt_ctf_event_name(event), 36))
    {
        is_unstructured = true;
        err = read_unstructured_event(event, timestamp_epoch, unstructured_event_text, taskname_eventname);
    }
    else
    {
        is_unstructured = false;
        err = read_structured_event(event, eventRecord, timestamp_epoch);
    }

    if (err != 0)
    {
        return err;
    }

    // moving iterator to next trace event (returns 0 on success)
    return bt_iter_next(bt_ctf_get_iter(ctf_iter));
}

extern "C" bool get_active_lttng_session_path(const char* session_name_startswith, char *output_active_session_path, size_t active_session_buffer_size)
{
    struct lttng_session *sessions;

    if (session_name_startswith == NULL)
    {
        std::cerr << "Error - NULL session name when trying to find active sessions." << std::endl;
        return false;
    }

    // getting list of all existing lttng sessions.
    int num_sessions = lttng_list_sessions(&sessions);

    if (num_sessions <= 0)
    {
        return false;
    }

    size_t startswith_size = strlen(session_name_startswith);

    // looking for the first active (enabled) session starting with session_name_startswith
    for(int i = 0; i < num_sessions; i++)
    {
        if (sessions[i].enabled != 0 &&
            strncmp(sessions[i].name, session_name_startswith, startswith_size) == 0)
        {
            // checking whether the path fits in the buffer provided.
            if (sessions[i].path != NULL && strlen(sessions[i].path) < active_session_buffer_size)
            {
                strcpy(output_active_session_path, sessions[i].path);
                return true;
            }
        }
    }

    return false;
}
