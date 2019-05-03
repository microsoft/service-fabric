;/*--
;
; Copyright (C) 1989-2005 Microsoft Corporation
;
; Module Name:
;
;   kstatus.h
;
; Abstract:
;
;   This module contains the specific error codes and messages returned
;   by KTL modules
;
; Author:
;
;   Alan Warwick (AlanWar)   21-Aug-2014
;
; Environment:
;
;   User and Kernel modes
;
; Notes:
;
; --*/
;
;#pragma once
;
;
;//
;// Facility codes.  Define KTL to be Facility 0xFF.  Other components that use KTL will use other facility codes.
;//
;
;const USHORT FacilityKtl = 0xFF;
;const USHORT FacilityLogger = 0xFC;
;const USHORT FacilityBtree = 0xBB;
;
;//
;// Macro for defining NTSTATUS codes.
;//
;
;#define KtlStatusCode(Severity, Facility, ErrorCode) ((NTSTATUS) ((Severity)<<30) | (1<<29) | ((Facility)<<16) | ErrorCode)
;
;//
;// Macro for extracting facility code from a KTL NTSTATUS code.
;//
;
;#define KtlStatusFacilityCode(ErrorCode) (((ErrorCode) >> 16) & 0x0FFF)
;

MessageIdTypedef = NTSTATUS

SeverityNames =
(
    Success         = 0x0 : STATUS_SEVERITY_SUCCESS
    Informational   = 0x1 : STATUS_SEVERITY_INFORMATIONAL
    Warning         = 0x2 : STATUS_SEVERITY_WARNING
    Error           = 0x3 : STATUS_SEVERITY_ERROR
)

FacilityNames =
(
    FacilityKtl = 0xFF : FACILITY_KTL
    FacilityBTree = 0xBB : FACILITY_BTREE
    FacilityLogger = 0xFC : FACILITY_LOGGER
)


;/***************************************************************************/
;/*                   NTSTATUS codes for KTL                                */
;/***************************************************************************/
MessageId    = 0x1
SymbolicName = K_STATUS_NOT_STARTED
Facility     = FacilityKtl
Severity     = Success
Language     = English
The Async has not started.
.

MessageId    = 0x2
SymbolicName = K_STATUS_MISALIGNED_IO
Facility     = FacilityKtl
Severity     = Error
Language     = English
A mis-aligned IO request buffer was used.
.

MessageId    = 0x3
SymbolicName = K_STATUS_SHUTDOWN_PENDING
Facility     = FacilityKtl
Severity     = Error
Language     = English
The operation could not be completed because a shutdown is pending.
.

MessageId    = 0x14
SymbolicName = K_STATUS_BUFFER_TOO_SMALL
Facility     = FacilityKtl
Severity     = Success
Language     = English
The buffer passed is too small.
.

MessageId    = 0x15
SymbolicName = K_STATUS_INVALID_STREAM_FORMAT
Facility     = FacilityKtl
Severity     = Error
Language     = English
The stream format is invalid.
.

MessageId    = 0x16
SymbolicName = K_STATUS_OBJECT_ACTIVATION_FAILED
Facility     = FacilityKtl
Severity     = Error
Language     = English
Object activation failed.
.

MessageId    = 0x17
SymbolicName = K_STATUS_UNEXPECTED_METADATA_READ
Facility     = FacilityKtl
Severity     = Error
Language     = English
Metadata was not expected to be read.
.

MessageId    = 0x18
SymbolicName = K_STATUS_NO_MORE_EXTENSION_BUFFERS
Facility     = FacilityKtl
Severity     = Error
Language     = English
No more extension buffers are available.
.

MessageId    = 0x19
SymbolicName = K_STATUS_SCOPE_END
Facility     = FacilityKtl
Severity     = Success
Language     = English
Scope End.
.

MessageId    = 0x1a
SymbolicName = K_STATUS_INVALID_UNKNOWN_EXTENSION_BUFFERS
Facility     = FacilityKtl
Severity     = Error
Language     = English
An invalid or unknown extension buffer was encountered.
.

MessageId    = 0x1b
SymbolicName = K_STATUS_COULD_NOT_READ_STREAM
Facility     = FacilityKtl
Severity     = Error
Language     = English
Stream count not be read.
.

MessageId    = 0x1c
SymbolicName = K_STATUS_NO_MORE_UNKNOWN_BUFFERS
Facility     = FacilityKtl
Severity     = Error
Language     = English
There are no more unknown buffers.
.

MessageId    = 0x1d
SymbolicName = K_STATUS_XML_END_OF_INPUT
Facility     = FacilityKtl
Severity     = Error
Language     = English
Reached the end of XML input.
.

MessageId    = 0x1e
SymbolicName = K_STATUS_XML_HEADER_ERROR
Facility     = FacilityKtl
Severity     = Error
Language     = English
An XML header error was encountered.
.

MessageId    = 0x1f
SymbolicName = K_STATUS_XML_INVALID_ATTRIBUTE
Facility     = FacilityKtl
Severity     = Error
Language     = English
An invalid XML attribute error was encountered.
.

MessageId    = 0x20
SymbolicName = K_STATUS_XML_SYNTAX_ERROR
Facility     = FacilityKtl
Severity     = Error
Language     = English
An invalid XML syntax error was encountered.
.

MessageId    = 0x21
SymbolicName = K_STATUS_XML_INVALID_NAME
Facility     = FacilityKtl
Severity     = Error
Language     = English
An invalid XML name error was encountered.
.

MessageId    = 0x22
SymbolicName = K_STATUS_XML_PARSER_LIMIT
Facility     = FacilityKtl
Severity     = Error
Language     = English
The XML parser limit was reached.
.

MessageId    = 0x23
SymbolicName = K_STATUS_XML_MISMATCHED_ELEMENT
Facility     = FacilityKtl
Severity     = Error
Language     = English
A mismatched XML element was encountered.
.

MessageId    = 0x24
SymbolicName = K_STATUS_OBJECT_NOT_CLOSED
Facility     = FacilityKtl
Severity     = Error
Language     = English
The object is not closed.
.

MessageId    = 0x25
SymbolicName = K_STATUS_OBJECT_NOT_ATTACHED
Facility     = FacilityKtl
Severity     = Error
Language     = English
The object is not attached.
.

MessageId    = 0x26
SymbolicName = K_STATUS_OBJECT_UNKNOWN_TYPE
Facility     = FacilityKtl
Severity     = Error
Language     = English
The object type is unknown.
.

MessageId    = 0x27
SymbolicName = K_STATUS_IOCP_ERROR
Facility     = FacilityKtl
Severity     = Error
Language     = English
An error occurred with an IOCP operation.
.

MessageId    = 0x28
SymbolicName = K_STATUS_HTTP_API_CALL_FAILURE
Facility     = FacilityKtl
Severity     = Error
Language     = English
A call to an http API (http.sys) function failed.
.

MessageId    = 0x29
SymbolicName = K_STATUS_WEBSOCKET_API_CALL_FAILURE
Facility     = FacilityKtl
Severity     = Error
Language     = English
A call to a WebSocket Protocol Component API function failed.
.

MessageId    = 0x30
SymbolicName = K_STATUS_WEBSOCKET_STANDARD_VIOLATION
Facility     = FacilityKtl
Severity     = Error
Language     = English
The WebSocket standard was violated.
.

MessageId    = 0x31
SymbolicName = K_STATUS_PROTOCOL_TIMEOUT
Facility     = FacilityKtl
Severity     = Error
Language     = English
A protocol-defined timeout has been exceeded.
.

MessageId    = 0x32
SymbolicName = K_STATUS_OUT_OF_BOUNDS
Facility     = FacilityKtl
Severity     = Error
Language     = English
An integer value was outside the allowed range.
.

MessageId    = 0x33
SymbolicName = K_STATUS_TRUNCATED_TRANSPORT_SEND
Facility     = FacilityKtl
Severity     = Error
Language     = English
The transport failed to fully send the requested bytes.
.

MessageId    = 0x34
SymbolicName = K_STATUS_API_CLOSED
Facility     = FacilityKtl
Severity     = Error
Language     = English
The called API is closed, and is not valid to be called in this state.
.

MessageId    = 0x35
SymbolicName = K_STATUS_WINHTTP_API_CALL_FAILURE
Facility     = FacilityKtl
Severity     = Error
Language     = English
A call to a Winhttp API function failed.
.

MessageId    = 0x36
SymbolicName = K_STATUS_WINHTTP_ERROR
Facility     = FacilityKtl
Severity     = Error
Language     = English
A Winhttp error occurred.
.

MessageId    = 0x37
SymbolicName = K_STATUS_OPTION_UNSUPPORTED
Facility     = FacilityKtl
Severity     = Error
Language     = English
The requested option is unsupported.
.

;/***************************************************************************/
;/*                   NTSTATUS codes for KBTree                             */
;/***************************************************************************/
MessageId    = 0x64
SymbolicName = B_STATUS_IDEMPOTENCE
Facility     = FacilityBtree
Severity     = Error
Language     = English
Idempotence.
.

MessageId    = 0x65
SymbolicName = B_STATUS_RECORD_EXISTS
Facility     = FacilityBtree
Severity     = Error
Language     = English
Record already exists.
.

MessageId    = 0x66
SymbolicName = B_STATUS_RECORD_NOT_EXIST
Facility     = FacilityBtree
Severity     = Error
Language     = English
Record does not exist.
.

MessageId    = 0x67
SymbolicName = B_STATUS_CONDITIONAL_CHECK_FAIL
Facility     = FacilityBtree
Severity     = Error
Language     = English
Conditional check failed.
.

;/***************************************************************************/
;/*                   NTSTATUS codes for KTL Logger                         */
;/***************************************************************************/
MessageId    = 0x1
SymbolicName = K_STATUS_LOG_STRUCTURE_FAULT
Facility     = FacilityLogger
Severity     = Error
Language     = English
The log structure is corrupt.
.

MessageId    = 0x2
SymbolicName = K_STATUS_LOG_INVALID_LOGID
Facility     = FacilityLogger
Severity     = Error
Language     = English
The log id is invalid.
.

MessageId    = 0x3
SymbolicName = K_STATUS_LOG_RESERVE_TOO_SMALL
Facility     = FacilityLogger
Severity     = Error
Language     = English
The log reservation is not large enough.
.

MessageId    = 0x4
SymbolicName = K_STATUS_LOG_DATA_FILE_FAULT
Facility     = FacilityLogger
Severity     = Error
Language     = English
The log file is in an inconsistent state.
.

