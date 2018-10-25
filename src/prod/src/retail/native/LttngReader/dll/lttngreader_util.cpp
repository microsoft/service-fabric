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
#include <babeltrace/ctf/iterator.h>
#include <babeltrace/list.h>
#include <babeltrace/iterator.h>
#include <babeltrace/format.h>
#include "lttngreader_util.h"
#include <lttng/tracepoint.h>
#include <iomanip>

// TODO - not thread safe
unsigned char dataBuffer[64000];

// TODO - remove just for debugging
#include <cstdio>

int read_event_vpid_vtid(
    const struct bt_ctf_event *event,
    int32_t &vpid,
    int32_t &vtid);

unsigned int read_sequence_data(
    const struct bt_ctf_event *event,
    const struct bt_definition *field,
    unsigned char *data);

int read_unstructured_event_fields(
    const struct bt_ctf_event *event,
    const char **task_name,
    const char **event_name,
    const wchar_t **idField,
    const wchar_t **dataField);

int read_event_timestamp(
    const struct bt_ctf_event *event,
    uint64_t &timestamp_epoch);

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

int read_event_descriptor(const struct bt_ctf_event *event, const struct bt_definition *fields_def, PEVENT_HEADER eventHeader);

int read_event_vpid_vtid(const struct bt_ctf_event *event, int32_t &vpid, int32_t &vtid)
{
    // getting process id and thread id
    unsigned int count;
    struct bt_definition const* const* stream_context_fields_list;

    bt_definition const *stream_context_scope_def = bt_ctf_get_top_level_scope(event, BT_STREAM_EVENT_CONTEXT);

    int err = bt_ctf_get_field_list(event, stream_context_scope_def, &stream_context_fields_list, &count);

    if (err != 0)
        return err;

    if (count != 2)
        return -1;

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

    return 0;
}

// reads the binary data of the trace event into data buffer
// returns the size of the sequence read
unsigned int read_sequence_data(const struct bt_ctf_event *event, const struct bt_definition *field, unsigned char *data)
{
    unsigned int num_data_elements;
    struct bt_definition const* const* sequence_list;

    int err = bt_ctf_get_field_list(event, field, &sequence_list, &num_data_elements);

    // some trace events have no fields so the method above will return an error and set
    // num_data elements to 0. This is expected and is not an error.
    if (err == 0)
    {
        // needs to read each byte separately
        for (int j = 0; j < num_data_elements; j++)
        {
            data[j] = bt_ctf_get_uint64(sequence_list[j]);
        }
    }

    return num_data_elements;
}

int read_unstructured_event_fields(const struct bt_ctf_event *event, const char **task_name, const char **event_name, const wchar_t **idField, const wchar_t **dataField)
{
    unsigned int count;
    struct bt_definition const* const* fields_list;
    bt_definition const *fields_def = bt_ctf_get_top_level_scope(event, BT_EVENT_FIELDS);

    // getting the list of fields in the event
    int err = bt_ctf_get_field_list(event, fields_def, &fields_list, &count);

    if (err < 0 )
    {
        std::cerr << "Failed to get fields for unstructured event." << std::endl;
        return -err;
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
        return -1;
    }

    // reading taskNameField and EventNameField values
    *task_name = bt_ctf_get_string(task_name_def);
    *event_name = bt_ctf_get_string(event_name_def);
    *idField = (wchar_t*)bt_ctf_get_string(idField_def);
    *dataField = (wchar_t*)bt_ctf_get_string(dataField_def);

    return 0;
}

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

int read_event_timestamp(const struct bt_ctf_event *event, uint64_t &timestamp_epoch)
{
    // getting event time since epoch
    timestamp_epoch = bt_ctf_get_timestamp(event);

    if (timestamp_epoch == -1ULL)
    {
        std::cerr << "Error trying to read timestamp from event" << std::endl;
        return -1;
    }

    return 0;
}

LARGE_INTEGER get_filetime_from_unix_epoch(uint64_t epoch_timestamp)
{
    // Windows fileTime is used in the rest of the code. It has a resolution of 100 nanoseconds.
    // 116444736000000 is the number of 100ns from filetime 1601-01-01 until epoch
    LARGE_INTEGER ret;
    ret.QuadPart = (epoch_timestamp / 100LL) + 116444736000000000LL;
 
    return ret;
}

uint64_t get_epoch_from_filetime(LARGE_INTEGER filetime)
{
    uint64_t ufiletime = (uint64_t)filetime.QuadPart;
    return (uint64_t)(ufiletime - 116444736000000000ULL) * 100ULL;
}

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

    size_t copy_size = std::min(unstructured_event_string.size(), 64000 - sizeof(wchar_t));
    wcsncpy(event_string_representation, unstructured_event_string.c_str(), copy_size+1);
}

int read_unstructured_event(const struct bt_ctf_event *event, uint64_t &timestamp_epoch, wchar_t *event_string_representation, char *taskname_eventname)
{

    // reading event's timestamp
    int err = read_event_timestamp(event, timestamp_epoch);

    if (err < 0)
    {
        return err;
    }

    // reading processId and threadId
    int32_t vpid, vtid;

    err = read_event_vpid_vtid(event, vpid, vtid);

    if (err < 0)
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
    read_unstructured_event_fields(event, &task_name, &event_name, &idField, &dataField);

    print_unstructured_event(timestamp_epoch, vtid, vpid, loglevel, task_name, event_name, idField, dataField, event_string_representation);

    // Optimization for filtering table events in managed code.
    // TODO - Should be removed after complete migration to structured events
    sprintf(taskname_eventname, "%s.%s", task_name, event_name);

    return 0;
}

// reads the EVENT_DESCRIPTOR of the trace event
int read_event_descriptor(const struct bt_ctf_event *event, const struct bt_definition *fields_def, PEVENT_HEADER eventHeader)
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
        // TODO add more meaningful error numbers
        std::cerr << "Unabled to get eventdescriptor info from trace" << std::endl;
        return -1;
    }

    // reads the providerId data
    unsigned int guidSize = bt_ctf_get_array_len(bt_ctf_get_decl_from_def(provider_id_def));

    // checks whether the size to be read is 16 bytes (GUID) to avoid buffer overflow
    if (guidSize != 16)
    {
        std::cerr << "Not expected GUID size of size " << guidSize << std::endl;
        return -1;
    }

    guidSize = read_sequence_data(event, provider_id_def, (unsigned char*)&eventHeader->ProviderId);

    // checks the size of GUID again, this time it checks whether there were any issues reading the GUID
    if (guidSize != 16)
    {
        std::cerr << "Problem reading GUID, expected size 16 but read " << guidSize << std::endl;
        return -1;
    }

    // reads the event descriptor from the event
    eventHeader->EventDescriptor.Id      = bt_ctf_get_uint64(id_def);
    eventHeader->EventDescriptor.Version = bt_ctf_get_uint64(version_def);
    eventHeader->EventDescriptor.Channel = bt_ctf_get_uint64(channel_def);
    eventHeader->EventDescriptor.Opcode  = bt_ctf_get_uint64(opcode_def);
    eventHeader->EventDescriptor.Task    = bt_ctf_get_uint64(task_def);
    eventHeader->EventDescriptor.Keyword = bt_ctf_get_uint64(keyword_def);

    return 0;
}

// reads the trace event. Assumes it only tries to read events that are in the expected format
int read_structured_event(const struct bt_ctf_event *event, PEVENT_RECORD eventRecord, uint64_t &epoch_timestamp)
{
    int err;

    // getting event time since epoch
    epoch_timestamp = bt_ctf_get_timestamp(event);

    if (epoch_timestamp == -1ULL)
        return -1;

    // Windows fileTime is used in the rest of the code. It has a resolution of 100 nanoseconds. 116444736000000 is the number of 100ns from filetime 1601-01-01 until epoch
    eventRecord->EventHeader.TimeStamp.QuadPart = (long long)((epoch_timestamp / 100LL) + 116444736000000000LL);

    // getting process id and thread id
    int32_t vtid, vpid;

    if (read_event_vpid_vtid(event, vpid, vtid) != 0)
    {
        vpid = vtid = -1;
         // not fatal
        std::cerr << "Failed to read vpid and vtid from event" << std::endl;
    }

    eventRecord->EventHeader.ThreadId = vtid;
    eventRecord->EventHeader.ProcessId = vpid;

    // event field types
    const struct bt_definition *fields_def = bt_ctf_get_top_level_scope(event, BT_EVENT_FIELDS);

    if (!fields_def)
    {
        std::cerr << "Unable to read events fields" << std::endl;
        return -1;
    }

    // reading eventDescriptor
    err = read_event_descriptor(event, fields_def, &eventRecord->EventHeader);

    if (err != 0)
    {
        std::cerr << "Failed to read EventDescriptor" << std::endl;
        return err;
    }

    // reading event data and size
    unsigned int dataNumBytes;
    const struct bt_definition *data_def = bt_ctf_get_field(event, fields_def, "data");

    if (!data_def)
    {
        std::cerr << "Unable to find data field in structured trace" << std::endl;
        return -1;
    }

    dataNumBytes = read_sequence_data(event, data_def, dataBuffer);

    eventRecord->UserDataLength = (unsigned short)dataNumBytes;
    eventRecord->UserData = dataBuffer;

    // reading loglevel
    eventRecord->EventHeader.EventDescriptor.Level = get_loglevel_lttng_to_etw(bt_ctf_event_loglevel(event));

    return 0;
}
