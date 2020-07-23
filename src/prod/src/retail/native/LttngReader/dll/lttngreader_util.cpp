// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <iostream>
#include <cstring>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <lttng/lttng.h>
#include <babeltrace/context.h>
#include <babeltrace/ctf/events.h>
#include <babeltrace/iterator.h>
#include <babeltrace/ctf/iterator.h>
#include <babeltrace/list.h>
#include <babeltrace/iterator.h>
#include <babeltrace/format.h>
#include <lttng/tracepoint.h>
#include <iomanip>
#include <evntprov.h>
#include "lttngreader.h"
#include "lttngreader_util.h"

// TODO - remove just for debugging
#include <cstdio>


// Prototypes /////////////////////////////////////////////////////////////////
LttngReaderStatusCode initialize_trace_info(
        LTTngTraceHandle &trace_handle);

LttngReaderStatusCode read_current_timestamp(
        bt_ctf_iter *ctf_iter,
        uint64_t &timestamp);

LttngReaderEventReadStatusCode read_event_vpid_vtid(
    const struct bt_ctf_event *event,
    int32_t &vpid,
    int32_t &vtid);

LttngReaderEventReadStatusCode read_sequence_data(
        const struct bt_ctf_event *event,
        const struct bt_definition *field,
        unsigned char *data,
        unsigned int &num_data_elements);

LttngReaderEventReadStatusCode read_unstructured_event_fields(
    const struct bt_ctf_event *event,
    const char **task_name,
    const char **event_name,
    const wchar_t **idField,
    const wchar_t **dataField);

LttngReaderEventReadStatusCode read_event_descriptor(
    const struct bt_ctf_event *event,
    const struct bt_definition *fields_def,
    PEVENT_HEADER eventHeader);

const char* get_loglevel_str(int loglevel);

unsigned char get_loglevel_lttng_to_etw(int lttng_loglevel);

int get_event_id(const struct bt_ctf_event *event);

LARGE_INTEGER get_filetime_from_unix_epoch(uint64_t epoch_timestamp);

uint64_t get_epoch_from_filetime(LARGE_INTEGER filetime);

void print_unstructured_event(
    uint64_t timestamp_epoch,
    int32_t vtid,
    int32_t vpid,
    int loglevel,
    const char *task_name,
    const char *event_name,
    const wchar_t *idField,
    const wchar_t *dataField,
    wchar_t *event_string_representation);

// Implementations ////////////////////////////////////////////////////////////

// initialize the trace info and start and end time
// Trace handle id and ctx must already be initialized through initialize_trace_processing function
LttngReaderStatusCode initialize_trace(LTTngTraceHandle &trace_handle, uint64_t start_time, uint64_t end_time)
{
    LttngReaderStatusCode err = initialize_trace_info(trace_handle);

    if (err != LttngReaderStatusCode::SUCCESS )
        return err;

    return set_start_end_time(trace_handle, start_time, end_time);
}

// initializes the info about the trace. Trace handle and ctx must already be initialized.
// It first initializes the trace to include all traces then it reads the first, and last
// getting their initial timestamp, lost events count and last timestamp.
LttngReaderStatusCode initialize_trace_info(LTTngTraceHandle &trace_handle)
{
    LttngReaderStatusCode err = set_start_end_time(trace_handle, FIRST_TRACE_TIMESTAMP, LAST_TRACE_TIMESTAMP);

    if (err != LttngReaderStatusCode::SUCCESS)
    {
        return err;
    }

    // reading first event
    err = read_current_timestamp(trace_handle.ctf_iter, trace_handle.trace_info.start_time);

    if (err != LttngReaderStatusCode::SUCCESS)
    {
        return err;
    }

    // moving to last event
    bt_iter_pos *last_event_iter = create_bt_iter_pos(LAST_TRACE_TIMESTAMP);
    int err_babeltrace = bt_iter_set_pos(bt_ctf_get_iter(trace_handle.ctf_iter), last_event_iter);

    if (err_babeltrace != 0)
    {
        return LttngReaderStatusCode::FAILED_INITIALIZING_TRACEINFO;
    }

    trace_handle.trace_info.events_lost = bt_ctf_get_lost_events_count(trace_handle.ctf_iter);

    return read_current_timestamp(trace_handle.ctf_iter, trace_handle.trace_info.end_time);
}

// reads the timestamp of the current event pointed by the iterator.
// if you already have the event use read_event_timestamp.
// Returns negative on error.
LttngReaderStatusCode read_current_timestamp(bt_ctf_iter *ctf_iter, uint64_t &timestamp)
{
    bt_ctf_event *event = bt_ctf_iter_read_event(ctf_iter);

    if (event == NULL)
    {
        return LttngReaderStatusCode::UNEXPECTED_END_OF_TRACE;
    }

    if (read_event_timestamp(event, timestamp) != LttngReaderEventReadStatusCode::SUCCESS)
    {
        return LttngReaderStatusCode::FAILED_INITIALIZING_TRACEINFO;
    }

    return LttngReaderStatusCode::SUCCESS;
}

// Reads events process id and thread id. Returns negative number on error.
LttngReaderEventReadStatusCode read_event_vpid_vtid(const struct bt_ctf_event *event, int32_t &vpid, int32_t &vtid)
{
    unsigned int count;
    struct bt_definition const* const* stream_context_fields_list;

    bt_definition const *stream_context_scope_def = bt_ctf_get_top_level_scope(event, BT_STREAM_EVENT_CONTEXT);

    int err = bt_ctf_get_field_list(event, stream_context_scope_def, &stream_context_fields_list, &count);

    if (err != 0 || count != 2)
    {
        return LttngReaderEventReadStatusCode::FAILED_TO_READ_FIELD;
    }

    vtid = -1;
    vpid = -1;

    const struct bt_definition *vtid_def = bt_ctf_get_field(event, stream_context_scope_def, "vtid");
    const struct bt_definition *vpid_def = bt_ctf_get_field(event, stream_context_scope_def, "vpid");

    // checking whether vtid was found and getting its data
    if(vtid_def && bt_ctf_field_type(bt_ctf_get_decl_from_def(vtid_def)) == CTF_TYPE_INTEGER)
    {
        vtid = bt_ctf_get_int64(vtid_def);
    }

    // checking whether vpid was found and getting its data
    if(vpid_def && bt_ctf_field_type(bt_ctf_get_decl_from_def(vpid_def)) == CTF_TYPE_INTEGER)
    {
        vpid = bt_ctf_get_int64(vpid_def);
    }

    return LttngReaderEventReadStatusCode::SUCCESS;
}

// reads the binary data of the trace event into data buffer
// returns the size of the sequence read
LttngReaderEventReadStatusCode read_sequence_data(const struct bt_ctf_event *event, const struct bt_definition *field, unsigned char *data, unsigned int &num_data_elements)
{
    num_data_elements = 0;
    struct bt_definition const* const* sequence_list;

    // some trace events have no fields so this function returns an error and set
    // num_data elements to 0. This is expected and is not an error.
    int err = bt_ctf_get_field_list(event, field, &sequence_list, &num_data_elements);

    // to avoid memory corruption set the max number of elements to MAX_LTTNG_EVENT_DATA_SIZE
    // which is the maximum allowed to be written by a trace event
    if (num_data_elements > MAX_LTTNG_EVENT_DATA_SIZE)
    {
        return LttngReaderEventReadStatusCode::UNEXPECTED_EVENT_SIZE;
    }

    if (err == 0)
    {
        // needs to read each byte separately
        for (int j = 0; j < num_data_elements; j++)
        {
            data[j] = bt_ctf_get_uint64(sequence_list[j]);
        }
    }

    return LttngReaderEventReadStatusCode::SUCCESS;
}

// Reads the data from a trace written in the unstructured format
LttngReaderEventReadStatusCode read_unstructured_event_fields(const struct bt_ctf_event *event, const char **task_name, const char **event_name, const wchar_t **idField, const wchar_t **dataField)
{
    unsigned int count;
    struct bt_definition const* const* fields_list;
    bt_definition const *fields_def = bt_ctf_get_top_level_scope(event, BT_EVENT_FIELDS);

    // getting the list of fields in the event
    int err = bt_ctf_get_field_list(event, fields_def, &fields_list, &count);

    if (err < 0 )
    {
        return LttngReaderEventReadStatusCode::FAILED_TO_READ_FIELD;
    }

    // getting definitions of the TaskNameField and EventNameField
    const struct bt_definition *task_name_def = bt_ctf_get_field(event, fields_def, "taskNameField");
    const struct bt_definition *event_name_def = bt_ctf_get_field(event, fields_def, "eventNameField");

    // getting defintions of the idField and dataFields
    const struct bt_definition *idField_def          = bt_ctf_get_field(event, fields_def, "idField");
    const struct bt_definition *dataField_def        = bt_ctf_get_field(event, fields_def, "dataField");

    // checking whether all fields were found
    if (!(task_name_def && event_name_def && idField_def && dataField_def))
    {
        std::cerr << "Unable to find all fields from unstructured trace" << std::endl;
        return LttngReaderEventReadStatusCode::INVALID_NUM_FIELDS;
    }

    // reading taskNameField and EventNameField values
    *task_name = bt_ctf_get_string(task_name_def);
    *event_name = bt_ctf_get_string(event_name_def);
    *idField = (wchar_t*)bt_ctf_get_string(idField_def);
    *dataField = (wchar_t*)bt_ctf_get_string(dataField_def);

    return LttngReaderEventReadStatusCode::SUCCESS;
}

// Returns the text representation of the loglevel
const char* get_loglevel_str(int loglevel)
{
    switch(loglevel)
    {
        case TRACE_ALERT:        return "Silent";
        case TRACE_CRIT :        return "Critical";
        case TRACE_ERR:          return "Error";
        case TRACE_WARNING:      return "Warning";
        case TRACE_INFO:         return "Info";
        case TRACE_DEBUG_SYSTEM: return "Noise";
        case TRACE_DEBUG_LINE:   return "All";
        default:                 return "???";
    }
}

// converts between LTTng's level codes to Service Fabric ETW error codes
unsigned char get_loglevel_lttng_to_etw(int lttng_loglevel)
{
    switch(lttng_loglevel)
    {
        case TRACE_ALERT:        return 0;   // "Silent"
        case TRACE_CRIT :        return 1;   // "Critical"
        case TRACE_ERR:          return 2;   // "Error"
        case TRACE_WARNING:      return 3;   // "Warning"
        case TRACE_INFO:         return 4;   // "Info"
        case TRACE_DEBUG_SYSTEM: return 5;   // "Noise"
        case TRACE_DEBUG_LINE:   return 99;  // "All"
        default:                 return 255; // ** undefined **
    }
}

// Read the timestamp of an event. Returns negative on error.
LttngReaderEventReadStatusCode read_event_timestamp(const struct bt_ctf_event *event, uint64_t &timestamp_epoch)
{
    // getting event time since epoch
    timestamp_epoch = bt_ctf_get_timestamp(event);

    if (timestamp_epoch == -1ULL)
    {
        return LttngReaderEventReadStatusCode::FAILED_READING_TIMESTAMP;
    }

    return LttngReaderEventReadStatusCode::SUCCESS;
}

// converts Unix's epoch time in nanoseconds to Windows' file time
LARGE_INTEGER get_filetime_from_unix_epoch(uint64_t epoch_timestamp)
{
    // Windows fileTime is used in the rest of the code. It has a resolution of 100 nanoseconds.
    // 116444736000000 is the number of 100ns from filetime 1601-01-01 until epoch
    LARGE_INTEGER ret;
    ret.QuadPart = (epoch_timestamp / 100LL) + 116444736000000000LL;

    return ret;
}

// formats unstructured event fields into Service Fabric's .dtr format
void print_unstructured_event(
    uint64_t timestamp_epoch,
    int32_t vtid,
    int32_t vpid,
    int loglevel,
    const char *task_name,
    const char *event_name,
    const wchar_t *idField,
    const wchar_t *dataField,
    wchar_t *event_string_representation)
{
    std::wstringstream event_text;

    // printing time portion of trace
    std::time_t time = (std::time_t) (timestamp_epoch / 1000000000ULL);
    int64_t milliseconds = (timestamp_epoch)/1000000 % 1000ULL;
    struct tm *datetime = gmtime(&time);
    event_text << (1900 + datetime->tm_year) << L"-";
    event_text << (1 + datetime->tm_mon) << L"-";
    event_text << datetime->tm_mday << L" ";
    event_text << std::setfill((wchar_t)L'0');
    event_text << std::setw(2) << datetime->tm_hour << L":";
    event_text << std::setw(2) << datetime->tm_min << L":";
    event_text << std::setw(2) << datetime->tm_sec << L".";
    event_text << std::setw(3) << milliseconds;
    event_text << L",";

    // printing LogLevel
    event_text << get_loglevel_str(loglevel) << L",";

    // printing threadid and processid
    event_text << vtid << L"," << vpid << L",";

    // printing TaskName and EventName
    event_text << task_name;
    event_text << L".";
    event_text << event_name;

    // printing EventName@idField if idField not empty
    if (wcslen(idField) > 0)
    {
        event_text << L"@";
        event_text << idField;
    }

    event_text << L",";

    // printing the trace text
    event_text << dataField;

    // copying entire event text to buffer
    std::wstring unstructured_event_string = event_text.str();

    size_t copy_size = std::min(unstructured_event_string.size(), MAX_LTTNG_UNSTRUCTURED_EVENT_LEN);
    wcsncpy(event_string_representation, unstructured_event_string.c_str(), copy_size+1);
    event_string_representation[MAX_LTTNG_UNSTRUCTURED_EVENT_LEN] = L'\0';
}

// Reads unstructured event. Returns negative on error.
LttngReaderEventReadStatusCode read_unstructured_event(const struct bt_ctf_event *event, uint64_t &timestamp_epoch, wchar_t *event_string_representation, char *taskname_eventname)
{

    // reading event's timestamp
    LttngReaderEventReadStatusCode err = read_event_timestamp(event, timestamp_epoch);

    if (err != LttngReaderEventReadStatusCode::SUCCESS)
    {
        return err;
    }

    // reading processId and threadId
    int32_t vpid, vtid;

    err = read_event_vpid_vtid(event, vpid, vtid);

    if (err != LttngReaderEventReadStatusCode::SUCCESS)
    {
        vpid = vtid = -1;
         // not fatal
        std::cerr << "Failed to read vpid and vtid from event" << std::endl;
    }

    // reading loglevel
    int loglevel = bt_ctf_event_loglevel(event);

    // reading event data
    const char *task_name;
    const char *event_name;
    const wchar_t *idField;
    const wchar_t *dataField;

    // read fields from event
    err = read_unstructured_event_fields(event, &task_name, &event_name, &idField, &dataField);

    if (err != LttngReaderEventReadStatusCode::SUCCESS)
    {
        return err;
    }

    print_unstructured_event(timestamp_epoch, vtid, vpid, loglevel, task_name, event_name, idField, dataField, event_string_representation);

    // Optimization for filtering table events in managed code.
    // TODO - Should be removed after complete migration to structured events
    sprintf(taskname_eventname, "%s.%s", task_name, event_name);

    return LttngReaderEventReadStatusCode::SUCCESS;
}

// reads the EVENT_DESCRIPTOR of the trace event
LttngReaderEventReadStatusCode read_event_descriptor(const struct bt_ctf_event *event, const struct bt_definition *fields_def, PEVENT_HEADER eventHeader)
{
    // tries to find the ProviderId data
    const struct bt_definition *provider_id_def    = bt_ctf_get_field(event, fields_def, "ProviderId");
    const struct bt_definition *id_def             = bt_ctf_get_field(event, fields_def, "Id");
    const struct bt_definition *version_def        = bt_ctf_get_field(event, fields_def, "Version");
    const struct bt_definition *channel_def        = bt_ctf_get_field(event, fields_def, "Channel");
    const struct bt_definition *opcode_def         = bt_ctf_get_field(event, fields_def, "Opcode");
    const struct bt_definition *task_def           = bt_ctf_get_field(event, fields_def, "Task");
    const struct bt_definition *keyword_def        = bt_ctf_get_field(event, fields_def, "Keyword");

    if (!(provider_id_def && id_def && version_def && channel_def && opcode_def && task_def && keyword_def))
    {
        return LttngReaderEventReadStatusCode::INVALID_EVENT_DESCRIPTOR;
    }

    // reads the providerId data
    int lenGuid = bt_ctf_get_array_len(bt_ctf_get_decl_from_def(provider_id_def));

    // checks whether the size to be read is 16 bytes (GUID) to avoid buffer overflow
    if (lenGuid != 16)
    {
        return LttngReaderEventReadStatusCode::UNEXPECTED_FIELD_DATA_SIZE;
    }

    unsigned int guidSize = lenGuid;

    LttngReaderEventReadStatusCode err = read_sequence_data(event, provider_id_def, (unsigned char*)&eventHeader->ProviderId, guidSize);

    if (err != LttngReaderEventReadStatusCode::SUCCESS)
    {
        return err;
    }

    // checks the size of GUID again, this time it checks whether there were any issues reading the GUID
    if (guidSize != 16)
    {
        return LttngReaderEventReadStatusCode::UNEXPECTED_FIELD_DATA_SIZE;
    }

    // reads the event descriptor from the event
    eventHeader->EventDescriptor.Id      = bt_ctf_get_uint64(id_def);
    eventHeader->EventDescriptor.Version = bt_ctf_get_uint64(version_def);
    eventHeader->EventDescriptor.Channel = bt_ctf_get_uint64(channel_def);
    eventHeader->EventDescriptor.Opcode  = bt_ctf_get_uint64(opcode_def);
    eventHeader->EventDescriptor.Task    = bt_ctf_get_uint64(task_def);
    eventHeader->EventDescriptor.Keyword = bt_ctf_get_uint64(keyword_def);

    return LttngReaderEventReadStatusCode::SUCCESS;
}

// reads the trace event. Assumes it only tries to read events that are in the expected format
LttngReaderEventReadStatusCode read_structured_event(const struct bt_ctf_event *event, PEVENT_RECORD eventRecord, uint64_t &epoch_timestamp, unsigned char *data_buffer)
{
    LttngReaderEventReadStatusCode err;

    // getting event time since epoch
    epoch_timestamp = bt_ctf_get_timestamp(event);

    if (epoch_timestamp == -1ULL)
    {
        return LttngReaderEventReadStatusCode::FAILED_READING_TIMESTAMP;
    }

    // Windows fileTime is used in the rest of the code. It has a resolution of 100 nanoseconds. 116444736000000 is the number of 100ns from filetime 1601-01-01 until epoch
    eventRecord->EventHeader.TimeStamp.QuadPart = (long long)((epoch_timestamp / 100LL) + 116444736000000000LL);

    // getting process id and thread id
    int32_t vtid, vpid;

    if (read_event_vpid_vtid(event, vpid, vtid) != LttngReaderEventReadStatusCode::SUCCESS)
    {
        vpid = vtid = -1;
         // not fatal
    }

    eventRecord->EventHeader.ThreadId = vtid;
    eventRecord->EventHeader.ProcessId = vpid;

    // event field types
    const struct bt_definition *fields_def = bt_ctf_get_top_level_scope(event, BT_EVENT_FIELDS);

    if (!fields_def)
    {
        return LttngReaderEventReadStatusCode::FAILED_TO_READ_FIELD;
    }

    // reading eventDescriptor
    err = read_event_descriptor(event, fields_def, &eventRecord->EventHeader);

    if (err != LttngReaderEventReadStatusCode::SUCCESS)
    {
        return err;
    }

    // reading event data and size
    unsigned int dataNumBytes;
    const struct bt_definition *data_def = bt_ctf_get_field(event, fields_def, "data");

    if (!data_def)
    {
        return LttngReaderEventReadStatusCode::FAILED_TO_READ_FIELD;
    }

    err = read_sequence_data(event, data_def, data_buffer, dataNumBytes);

    if (err != LttngReaderEventReadStatusCode::SUCCESS)
    {
        return err;
    }

    eventRecord->UserDataLength = (unsigned short)dataNumBytes;
    eventRecord->UserData = data_buffer;

    // reading loglevel
    eventRecord->EventHeader.EventDescriptor.Level = get_loglevel_lttng_to_etw(bt_ctf_event_loglevel(event));

    return LttngReaderEventReadStatusCode::SUCCESS;
}

// Create an new iterator based on the timestamp passed.
// It handles getting the first trace and last timestamp through
// the defined flags
bt_iter_pos* create_bt_iter_pos(uint64_t timestamp_epoch)
{
    bt_iter_pos *iter_pos = bt_iter_create_time_pos(NULL, timestamp_epoch);

    switch (timestamp_epoch)
    {
        // starting reading traces from beginning
        case FIRST_TRACE_TIMESTAMP:
            iter_pos->type = BT_SEEK_BEGIN;
            break;
        // starting reading traces from end (points to last event)
        case LAST_TRACE_TIMESTAMP :
            iter_pos->type = BT_SEEK_LAST;
            break;
        // starting reading traces from the specified timestamp
        default:
            break;
    }

    return iter_pos;
}
