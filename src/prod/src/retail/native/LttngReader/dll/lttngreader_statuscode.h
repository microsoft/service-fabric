// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <cstdint>

enum class LttngReaderStatusCode : int32_t
{
    END_OF_TRACE                    = -1,    // Success - End of trace reached.
    SUCCESS                         =  0,    // Success - No error found during execution.
    FAILED_TO_READ_EVENT            =  1,    // Recoverable Error - Failed to read single event. It may be possible to continue reading traces.
    ERROR                           =  2,    // Error - Unrecoverable error happened when trying to read event. Unable to proceed processing traces.
    NOT_ENOUGH_MEM                  =  3,    // Error - Failed allocating memory.
    INVALID_BUFFER_SIZE             =  4,    // Error - Not enough space in buffer provided.
    INVALID_ARGUMENT                =  5,    // Error - One of the provided arguments is not valid.
    NO_LTTNG_SESSION_FOUND          =  6,    // Error - No active LTTng trace session found.
    FAILED_TO_LOAD_TRACES           =  7,    // Error - Failed opening trace folder for reading traces.
    FAILED_INITIALIZING_TRACEINFO   =  8,    // Error - Failure when initializing trace.
    FAILED_TO_MOVE_ITERATOR         =  9,    // Error - Failure occuring when trying to move iterator to next event.
    INVALID_CTF_ITERATOR            = 10,    // Error - Invalid (null) CTF iterator provided.
    UNEXPECTED_END_OF_TRACE         = 11     // Error - Unexpectedly reached the end of the trace.
};

enum class LttngReaderEventReadStatusCode : int32_t
{
    SUCCESS                         =  0,   // Success - No error found during execution.
    UNEXPECTED_EVENT_SIZE           =  1,   // Error - Event exceeds the buffer size allocated to read the events.
    FAILED_READING_TIMESTAMP        =  2,   // Error - Failed to read event's timestamp.
    FAILED_TO_READ_FIELD            =  3,   // Error - Failed to read a field from event.
    INVALID_NUM_FIELDS              =  4,   // Error - Number of fields in event is different than expected.
    INVALID_EVENT_DESCRIPTOR        =  5,   // Error - One or more fields in structured trace EventDescriptor are missing.
    UNEXPECTED_FIELD_DATA_SIZE      =  6    // Error - The size of the data read from field doesn't match the expected size for the field.
};
