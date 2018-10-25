/*--

 Copyright (C) 1989-2005 Microsoft Corporation

 Module Name:

   kstatus.h

 Abstract:

   This module contains the specific error codes and messages returned
   by KTL modules

 Author:

   Alan Warwick (AlanWar)   21-Aug-2014

 Environment:

   User and Kernel modes

 Notes:

 --*/

#pragma once


//
// Facility codes.  Define KTL to be Facility 0xFF.  Other components that use KTL will use other facility codes.
//

const USHORT FacilityKtl = 0xFF;
const USHORT FacilityLogger = 0xFC;
const USHORT FacilityBtree = 0xBB;

//
// Macro for defining NTSTATUS codes.
//

#define KtlStatusCode(Severity, Facility, ErrorCode) ((NTSTATUS) ((Severity)<<30) | (1<<29) | ((Facility)<<16) | ErrorCode)

//
// Macro for extracting facility code from a KTL NTSTATUS code.
//

#define KtlStatusFacilityCode(ErrorCode) (((ErrorCode) >> 16) & 0x0FFF)

/***************************************************************************/
/*                   NTSTATUS codes for KTL                                */
/***************************************************************************/
//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_LOGGER                  0xFC
#define FACILITY_KTL                     0xFF
#define FACILITY_BTREE                   0xBB


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: K_STATUS_NOT_STARTED
//
// MessageText:
//
// The Async has not started.
//
#define K_STATUS_NOT_STARTED             ((NTSTATUS)0x20FF0001L)

//
// MessageId: K_STATUS_MISALIGNED_IO
//
// MessageText:
//
// A mis-aligned IO request buffer was used.
//
#define K_STATUS_MISALIGNED_IO           ((NTSTATUS)0xE0FF0002L)

//
// MessageId: K_STATUS_SHUTDOWN_PENDING
//
// MessageText:
//
// The operation could not be completed because a shutdown is pending.
//
#define K_STATUS_SHUTDOWN_PENDING        ((NTSTATUS)0xE0FF0003L)

//
// MessageId: K_STATUS_BUFFER_TOO_SMALL
//
// MessageText:
//
// The buffer passed is too small.
//
#define K_STATUS_BUFFER_TOO_SMALL        ((NTSTATUS)0x20FF0014L)

//
// MessageId: K_STATUS_INVALID_STREAM_FORMAT
//
// MessageText:
//
// The stream format is invalid.
//
#define K_STATUS_INVALID_STREAM_FORMAT   ((NTSTATUS)0xE0FF0015L)

//
// MessageId: K_STATUS_OBJECT_ACTIVATION_FAILED
//
// MessageText:
//
// Object activation failed.
//
#define K_STATUS_OBJECT_ACTIVATION_FAILED ((NTSTATUS)0xE0FF0016L)

//
// MessageId: K_STATUS_UNEXPECTED_METADATA_READ
//
// MessageText:
//
// Metadata was not expected to be read.
//
#define K_STATUS_UNEXPECTED_METADATA_READ ((NTSTATUS)0xE0FF0017L)

//
// MessageId: K_STATUS_NO_MORE_EXTENSION_BUFFERS
//
// MessageText:
//
// No more extension buffers are available.
//
#define K_STATUS_NO_MORE_EXTENSION_BUFFERS ((NTSTATUS)0xE0FF0018L)

//
// MessageId: K_STATUS_SCOPE_END
//
// MessageText:
//
// Scope End.
//
#define K_STATUS_SCOPE_END               ((NTSTATUS)0x20FF0019L)

//
// MessageId: K_STATUS_INVALID_UNKNOWN_EXTENSION_BUFFERS
//
// MessageText:
//
// An invalid or unknown extension buffer was encountered.
//
#define K_STATUS_INVALID_UNKNOWN_EXTENSION_BUFFERS ((NTSTATUS)0xE0FF001AL)

//
// MessageId: K_STATUS_COULD_NOT_READ_STREAM
//
// MessageText:
//
// Stream count not be read.
//
#define K_STATUS_COULD_NOT_READ_STREAM   ((NTSTATUS)0xE0FF001BL)

//
// MessageId: K_STATUS_NO_MORE_UNKNOWN_BUFFERS
//
// MessageText:
//
// There are no more unknown buffers.
//
#define K_STATUS_NO_MORE_UNKNOWN_BUFFERS ((NTSTATUS)0xE0FF001CL)

//
// MessageId: K_STATUS_XML_END_OF_INPUT
//
// MessageText:
//
// Reached the end of XML input.
//
#define K_STATUS_XML_END_OF_INPUT        ((NTSTATUS)0xE0FF001DL)

//
// MessageId: K_STATUS_XML_HEADER_ERROR
//
// MessageText:
//
// An XML header error was encountered.
//
#define K_STATUS_XML_HEADER_ERROR        ((NTSTATUS)0xE0FF001EL)

//
// MessageId: K_STATUS_XML_INVALID_ATTRIBUTE
//
// MessageText:
//
// An invalid XML attribute error was encountered.
//
#define K_STATUS_XML_INVALID_ATTRIBUTE   ((NTSTATUS)0xE0FF001FL)

//
// MessageId: K_STATUS_XML_SYNTAX_ERROR
//
// MessageText:
//
// An invalid XML syntax error was encountered.
//
#define K_STATUS_XML_SYNTAX_ERROR        ((NTSTATUS)0xE0FF0020L)

//
// MessageId: K_STATUS_XML_INVALID_NAME
//
// MessageText:
//
// An invalid XML name error was encountered.
//
#define K_STATUS_XML_INVALID_NAME        ((NTSTATUS)0xE0FF0021L)

//
// MessageId: K_STATUS_XML_PARSER_LIMIT
//
// MessageText:
//
// The XML parser limit was reached.
//
#define K_STATUS_XML_PARSER_LIMIT        ((NTSTATUS)0xE0FF0022L)

//
// MessageId: K_STATUS_XML_MISMATCHED_ELEMENT
//
// MessageText:
//
// A mismatched XML element was encountered.
//
#define K_STATUS_XML_MISMATCHED_ELEMENT  ((NTSTATUS)0xE0FF0023L)

//
// MessageId: K_STATUS_OBJECT_NOT_CLOSED
//
// MessageText:
//
// The object is not closed.
//
#define K_STATUS_OBJECT_NOT_CLOSED       ((NTSTATUS)0xE0FF0024L)

//
// MessageId: K_STATUS_OBJECT_NOT_ATTACHED
//
// MessageText:
//
// The object is not attached.
//
#define K_STATUS_OBJECT_NOT_ATTACHED     ((NTSTATUS)0xE0FF0025L)

//
// MessageId: K_STATUS_OBJECT_UNKNOWN_TYPE
//
// MessageText:
//
// The object type is unknown.
//
#define K_STATUS_OBJECT_UNKNOWN_TYPE     ((NTSTATUS)0xE0FF0026L)

//
// MessageId: K_STATUS_IOCP_ERROR
//
// MessageText:
//
// An error occurred with an IOCP operation.
//
#define K_STATUS_IOCP_ERROR              ((NTSTATUS)0xE0FF0027L)

//
// MessageId: K_STATUS_HTTP_API_CALL_FAILURE
//
// MessageText:
//
// A call to an http API (http.sys) function failed.
//
#define K_STATUS_HTTP_API_CALL_FAILURE   ((NTSTATUS)0xE0FF0028L)

//
// MessageId: K_STATUS_WEBSOCKET_API_CALL_FAILURE
//
// MessageText:
//
// A call to a WebSocket Protocol Component API function failed.
//
#define K_STATUS_WEBSOCKET_API_CALL_FAILURE ((NTSTATUS)0xE0FF0029L)

//
// MessageId: K_STATUS_WEBSOCKET_STANDARD_VIOLATION
//
// MessageText:
//
// The WebSocket standard was violated.
//
#define K_STATUS_WEBSOCKET_STANDARD_VIOLATION ((NTSTATUS)0xE0FF0030L)

//
// MessageId: K_STATUS_PROTOCOL_TIMEOUT
//
// MessageText:
//
// A protocol-defined timeout has been exceeded.
//
#define K_STATUS_PROTOCOL_TIMEOUT        ((NTSTATUS)0xE0FF0031L)

//
// MessageId: K_STATUS_OUT_OF_BOUNDS
//
// MessageText:
//
// An integer value was outside the allowed range.
//
#define K_STATUS_OUT_OF_BOUNDS           ((NTSTATUS)0xE0FF0032L)

//
// MessageId: K_STATUS_TRUNCATED_TRANSPORT_SEND
//
// MessageText:
//
// The transport failed to fully send the requested bytes.
//
#define K_STATUS_TRUNCATED_TRANSPORT_SEND ((NTSTATUS)0xE0FF0033L)

//
// MessageId: K_STATUS_API_CLOSED
//
// MessageText:
//
// The called API is closed, and is not valid to be called in this state.
//
#define K_STATUS_API_CLOSED              ((NTSTATUS)0xE0FF0034L)

//
// MessageId: K_STATUS_WINHTTP_API_CALL_FAILURE
//
// MessageText:
//
// A call to a Winhttp API function failed.
//
#define K_STATUS_WINHTTP_API_CALL_FAILURE ((NTSTATUS)0xE0FF0035L)

//
// MessageId: K_STATUS_WINHTTP_ERROR
//
// MessageText:
//
// A Winhttp error occurred.
//
#define K_STATUS_WINHTTP_ERROR           ((NTSTATUS)0xE0FF0036L)

//
// MessageId: K_STATUS_OPTION_UNSUPPORTED
//
// MessageText:
//
// The requested option is unsupported.
//
#define K_STATUS_OPTION_UNSUPPORTED      ((NTSTATUS)0xE0FF0037L)

/***************************************************************************/
/*                   NTSTATUS codes for KBTree                             */
/***************************************************************************/
//
// MessageId: B_STATUS_IDEMPOTENCE
//
// MessageText:
//
// Idempotence.
//
#define B_STATUS_IDEMPOTENCE             ((NTSTATUS)0xE0BB0064L)

//
// MessageId: B_STATUS_RECORD_EXISTS
//
// MessageText:
//
// Record already exists.
//
#define B_STATUS_RECORD_EXISTS           ((NTSTATUS)0xE0BB0065L)

//
// MessageId: B_STATUS_RECORD_NOT_EXIST
//
// MessageText:
//
// Record does not exist.
//
#define B_STATUS_RECORD_NOT_EXIST        ((NTSTATUS)0xE0BB0066L)

//
// MessageId: B_STATUS_CONDITIONAL_CHECK_FAIL
//
// MessageText:
//
// Conditional check failed.
//
#define B_STATUS_CONDITIONAL_CHECK_FAIL  ((NTSTATUS)0xE0BB0067L)

/***************************************************************************/
/*                   NTSTATUS codes for KTL Logger                         */
/***************************************************************************/
//
// MessageId: K_STATUS_LOG_STRUCTURE_FAULT
//
// MessageText:
//
// The log structure is corrupt.
//
#define K_STATUS_LOG_STRUCTURE_FAULT     ((NTSTATUS)0xE0FC0001L)

//
// MessageId: K_STATUS_LOG_INVALID_LOGID
//
// MessageText:
//
// The log id is invalid.
//
#define K_STATUS_LOG_INVALID_LOGID       ((NTSTATUS)0xE0FC0002L)

//
// MessageId: K_STATUS_LOG_RESERVE_TOO_SMALL
//
// MessageText:
//
// The log reservation is not large enough.
//
#define K_STATUS_LOG_RESERVE_TOO_SMALL   ((NTSTATUS)0xE0FC0003L)

//
// MessageId: K_STATUS_LOG_DATA_FILE_FAULT
//
// MessageText:
//
// The log file is in an inconsistent state.
//
#define K_STATUS_LOG_DATA_FILE_FAULT     ((NTSTATUS)0xE0FC0004L)

