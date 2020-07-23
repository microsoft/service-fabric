// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <iostream>
#include <errno.h>
#include <new>
#include <mutex>
#include <lttng/lttng.h>
#include <babeltrace/ctf/iterator.h>
#include <babeltrace/iterator.h>
#include <babeltrace/trace-handle.h>
#include <babeltrace/ctf/events.h>
#include <evntprov.h>
#include "lttngreader.h"
#include "lttngreader_util.h"

std::mutex add_traces_recursive_mtx;

// initializes the trace processing by creating context, associating a folder and getting the iterator for the trace events.
// returns the trace handle id
extern "C" LttngReaderStatusCode initialize_trace_processing(const char* trace_folder_path, uint64_t start_time_epoch, uint64_t end_time_epoch, LTTngTraceHandle &trace_handle)
{
    // creating context
    trace_handle.ctx = bt_context_create();

    // adding trace to context from provided path (locking since bt_context_add_traces_recursive is not thread-safe )
    add_traces_recursive_mtx.lock();
        trace_handle.id = bt_context_add_traces_recursive(trace_handle.ctx, trace_folder_path, "ctf", NULL);
    add_traces_recursive_mtx.unlock();

    if (trace_handle.id < 0)
    {
        return LttngReaderStatusCode::FAILED_TO_LOAD_TRACES;
    }

    // cleaning iterator for initialization
    trace_handle.ctf_iter = NULL;
    trace_handle.last_iter_pos = NULL;

    // allocating memory for reading events
    // this memory holds the data of the current event being read.
    // it needs to be allocated so managed code is able to read the event and decoded it.
    // It was previously a global variable in the stack but it is allocated for thread safety.
    trace_handle.data_buffer = new (std::nothrow) unsigned char[MAX_LTTNG_EVENT_DATA_SIZE];

    if (trace_handle.data_buffer == NULL)
    {
        return LttngReaderStatusCode::NOT_ENOUGH_MEM;
    }

    return initialize_trace(trace_handle, start_time_epoch, end_time_epoch);
}

// cleans up by destroying the iterators, removing the trace from the context and freeing the context.
extern "C" void finalize_trace_processing(LTTngTraceHandle &trace_handle)
{
    // destroying iterators
    if (trace_handle.ctf_iter != NULL)
    {
        bt_ctf_iter_destroy(trace_handle.ctf_iter);
    }

    // freeing context
    bt_context_put(trace_handle.ctx);

    if (trace_handle.data_buffer != NULL)
    {
        delete [] trace_handle.data_buffer;
        trace_handle.data_buffer = NULL;
    }
}

extern "C" LttngReaderStatusCode read_next_event(LTTngTraceHandle &trace_handle, PEVENT_RECORD eventRecord, wchar_t *unstructured_event_text, char *taskname_eventname, uint64_t &timestamp_epoch, bool &is_unstructured)
{
    // reading event
    if (trace_handle.ctf_iter == NULL)
    {
        return LttngReaderStatusCode::INVALID_CTF_ITERATOR;
    }

    bt_ctf_event *event = bt_ctf_iter_read_event(trace_handle.ctf_iter);

    if (event == NULL)
    {
        return LttngReaderStatusCode::END_OF_TRACE;
    }

    LttngReaderEventReadStatusCode event_read_err;

    // deciding whether it is an unstructured trace or structured
    if (strncmp("service_fabric:tracepoint_structured", bt_ctf_event_name(event), 36))
    {
        is_unstructured = true;
        event_read_err = read_unstructured_event(event, timestamp_epoch, unstructured_event_text, taskname_eventname);
    }
    else
    {
        is_unstructured = false;
        event_read_err = read_structured_event(event, eventRecord, timestamp_epoch, trace_handle.data_buffer);
    }

    // trying to move iterator even if event_read_err is not success code.
    // all current error codes returned by either read_structured_event and read_unstructured_event are recoverable
    int err_babeltrace = bt_iter_next(bt_ctf_get_iter(trace_handle.ctf_iter));

    if (err_babeltrace < 0)
    {
        return LttngReaderStatusCode::FAILED_TO_MOVE_ITERATOR;
    }

    if (event_read_err != LttngReaderEventReadStatusCode::SUCCESS)
    {
        return LttngReaderStatusCode::FAILED_TO_READ_EVENT;
    }

    return LttngReaderStatusCode::SUCCESS;
}

extern "C" LttngReaderStatusCode set_start_end_time(LTTngTraceHandle &trace_handle, uint64_t start_time_epoch, uint64_t end_time_epoch)
{
    bt_iter_pos *start_pos = create_bt_iter_pos(start_time_epoch);
    bt_iter_pos *end_pos = create_bt_iter_pos(end_time_epoch);

    // first destroy previous iterator
    if (trace_handle.ctf_iter != NULL)
    {
        bt_ctf_iter_destroy(trace_handle.ctf_iter);
        trace_handle.ctf_iter = NULL;
    }

    // destroy stored last position
    if (trace_handle.last_iter_pos != NULL)
    {
        bt_iter_free_pos(trace_handle.last_iter_pos);
        trace_handle.last_iter_pos = NULL;
    }

    // create new iterator
    trace_handle.ctf_iter = bt_ctf_iter_create(trace_handle.ctx, start_pos, end_pos);

    // start_pos is not needed after initialization (** end_pos is neeed **)
    bt_iter_free_pos(start_pos);

    // end_pos needs to be stored since it is used internally by babeltrace
    trace_handle.last_iter_pos = end_pos;

    if(trace_handle.ctf_iter == NULL)
    {
        return LttngReaderStatusCode::INVALID_CTF_ITERATOR;
    }

    return LttngReaderStatusCode::SUCCESS;
}

extern "C" LttngReaderStatusCode get_active_lttng_session_path(const char* session_name_startswith, char *output_active_session_path, size_t active_session_buffer_size)
{
    struct lttng_session *sessions;

    if (session_name_startswith == NULL)
    {
        return LttngReaderStatusCode::INVALID_ARGUMENT;
    }

    // getting list of all existing lttng sessions.
    int num_sessions = lttng_list_sessions(&sessions);

    if (num_sessions <= 0)
    {
        LttngReaderStatusCode::NO_LTTNG_SESSION_FOUND;
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
                free(sessions);

                return LttngReaderStatusCode::SUCCESS;
            }
            else {
                free(sessions);
                return LttngReaderStatusCode::INVALID_BUFFER_SIZE;
            }
        }
    }

    return LttngReaderStatusCode::NO_LTTNG_SESSION_FOUND;
}
