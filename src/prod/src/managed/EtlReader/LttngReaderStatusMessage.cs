namespace Tools.EtlReader
{
    public static class LttngReaderStatusMessage
    {
        public static string GetMessage(LttngReaderStatusCode error)
        {
            switch (error)
            {
                case LttngReaderStatusCode.END_OF_TRACE                    : return "Success - End of trace reached.";
                case LttngReaderStatusCode.SUCCESS                         : return "Success - No error found during execution.";
                case LttngReaderStatusCode.FAILED_TO_READ_EVENT            : return "Recoverable Error - Failed to read single event. It may be possible to continue reading traces.";
                case LttngReaderStatusCode.ERROR                           : return "Unrecoverable error happened when trying to read event. Unable to proceed processing traces.";
                case LttngReaderStatusCode.NOT_ENOUGH_MEM                  : return "Failed allocating memory.";
                case LttngReaderStatusCode.INVALID_BUFFER_SIZE             : return "Not enough space in buffer provided.";
                case LttngReaderStatusCode.INVALID_ARGUMENT                : return "One of the provided arguments is not valid.";
                case LttngReaderStatusCode.NO_LTTNG_SESSION_FOUND          : return "No active LTTng trace session found.";
                case LttngReaderStatusCode.FAILED_TO_LOAD_TRACES           : return "Failed opening trace folder for reading traces.";
                case LttngReaderStatusCode.FAILED_INITIALIZING_TRACEINFO   : return "Failure when initializing trace.";
                case LttngReaderStatusCode.FAILED_TO_MOVE_ITERATOR         : return "Failure occuring when trying to move iterator to next event.";
                case LttngReaderStatusCode.INVALID_CTF_ITERATOR            : return "Invalid (null) CTF iterator provided.";
                case LttngReaderStatusCode.UNEXPECTED_END_OF_TRACE         : return "Unexpectedly reached the end of the trace.";
                default:
                    return "Invalid error code provided.";
            }
        }
    }
}