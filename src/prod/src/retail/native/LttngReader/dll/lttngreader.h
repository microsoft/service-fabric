// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#include "lttngreader_statuscode.h"

// forward declarations
struct bt_context;
struct bt_ctf_iter;
struct bt_iter_pos;
typedef struct _EVENT_DESCRIPTOR *PEVENT_DESCRIPTOR;
typedef struct _EVENT_RECORD *PEVENT_RECORD;
// Info about trace being read
struct LTTngTraceInfo
{
    uint64_t start_time;
    uint64_t end_time;
    uint32_t events_lost;
};

// Main struct for manipulating traces
struct LTTngTraceHandle
{
    int id;
    bt_context *ctx;
    bt_ctf_iter *ctf_iter;
    LTTngTraceInfo trace_info;

    // for allowing freeing used memory
    bt_iter_pos *last_iter_pos;

    // buffer for reading events
    unsigned char *data_buffer;
};

// Flags for specifying the first and last trace events
enum LTTngTraceTimestampFlag : uint64_t
{
    FIRST_TRACE_TIMESTAMP = std::numeric_limits<uint64_t>::min(),
    LAST_TRACE_TIMESTAMP = std::numeric_limits<uint64_t>::max()
};

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize trace processing by opening the files in the trace folder and setting up context and iterator.
 * \param[in] trace_folder_path The path to the LTTng trace session folder.
 * \param[out] ctx Context used internally by Babeltrace API.
 * \param[out] ctf_iter Iterator used to read events in sequence.
 * \param[in] timestamp_unix_epoch_nano_sec Timestamp to initialize ctf_iter to. If set to 0 iterator points to oldest event. If set to max uint64_t iterator points to last event.
 * \return Error code
 * \retval LttngReaderErrorCode::SUCCESS on success
 */
LttngReaderStatusCode initialize_trace_processing(const char* trace_folder_path, uint64_t start_time_epoch, uint64_t end_time_epoch, LTTngTraceHandle &trace_handle);

/** Finalize trace processing by freeing the used resources..
 * \param[in] trace_handle initialized trace handle.
 */
void finalize_trace_processing(LTTngTraceHandle &trace_handle);

/** Reads the current trace event at the iterator position. It reads both structured and unstructured events.
 * \param[out] eventRecord ETW EventRecord struct. The value of the members are only populate if is_unstructured is false..
 * \param[out] unstructured_event_text String representation of the event in case is_unstructured is true..
 * \param[out] taskname_eventname Event's TaskName.EventName in case is_unstructured is true.
 * \param[in] ctf_iter Iterator pointing to current event.
 * \param[out] timestamp_epoch Timestamp of the current event.
 * \return Error code
 * \retval LttngReaderErrorCode::SUCCESS on success
 */
LttngReaderStatusCode read_next_event(LTTngTraceHandle &trace_handle, PEVENT_RECORD eventRecord, wchar_t *unstructured_event_text, char *taskname_eventname, uint64_t &timestamp_epoch, bool &is_unstructured);

/** Sets iterator to specified start_time
 * \param[in] start_time_epoch set the iterator to event with time in nanoseconds from epoch.
 * \param[in,out] ctf_iter the iterator to be moved to start_time
 * \return Success or failure code.
 * \retval LttngReaderErrorCode::SUCCESS on success
 */
LttngReaderStatusCode set_start_end_time(LTTngTraceHandle &handle, uint64_t start_time_epoch, uint64_t end_time_epoch);

/** Gets the folder path to the active LTTng trace session
 * \param[in] session_name_startswith LTTng trace session name initial matching string.
 * \param[out] output_active_session_path Folder path for first match of active LTTng sessions which starts with session_name_startswith. Needs to be allocated prior to call.
 * \param[in] active_session_buffer_size Size of the allocated size for output_active_session array. To prevent buffer overrun..
 * \return Success or failure
 * \retval LttngReaderErrorCode::SUCCESS on success
 */
LttngReaderStatusCode get_active_lttng_session_path(const char* session_name_startswith, char *output_active_session_path, size_t active_session_buffer_size);

#ifdef __cplusplus
}
#endif

