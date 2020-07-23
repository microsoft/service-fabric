namespace Tools.EtlReader
{
    // Error Codes returned by LttngReaderBindings static methods
    public enum LttngReaderStatusCode : int
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
    }
}
