/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    keventlog

    Description:
      Kernel Tempate Library (KTL): KEventLog

      Wrappers for the Windows Event Log

    History:
      raymcc          16-Aug-2010         Initial version.

--*/

#pragma once


class KEventLogEntry
{
public:
    // Constructors
    //
    // Each overload is designed to be able to construct the event entirely via the constructor.
    // For a few optional fields (Category, hex dump, etc.) there are special member functions.
    //
    // If numeric values need to be logged, use KWString with the KULONG auxiliary constructor, which
    // will automatically convert ULONGs to the required string types.
    //
    KEventLogEntry(
        __in NTSTATUS EventId
        );

    KEventLogEntry(
        __in NTSTATUS EventId,
        __in PUNICODE_STRING InsertString0
        );

    KEventLogEntry(
        __in NTSTATUS EventId,
        __in PUNICODE_STRING InsertString0
        __in PUNICODE_STRING InsertString1
        );

    KEventLogEntry(
        __in NTSTATUS EventId,
        __in PUNICODE_STRING InsertString0
        __in PUNICODE_STRING InsertString1,
        __in PUNICODE_STRING InsertString2
        );

    KEventLogEntry(
        __in NTSTATUS EventId,
        __in PUNICODE_STRING InsertString0
        __in PUNICODE_STRING InsertString1,
        __in PUNICODE_STRING InsertString2,
        __in PUNICODE_STRING InsertString3
        );

    KEventLogEntry(
        __in NTSTATUS EventId,
        __in PUNICODE_STRING InsertString0
        __in PUNICODE_STRING InsertString1,
        __in PUNICODE_STRING InsertString2,
        __in PUNICODE_STRING InsertString3,
        __in PUNICODE_STRING InsertString4
        );

    // SetMajorFunctionCode
    //
    // Indicates the IRP_MJ_XXX major function code of the IRP the driver was handling when the error occurred.
    // Setting this value is optional
    //
    void SetMajorFunctionCode(
        __in UCHAR MajorFunctionCode
        );

    // SetUniqueErrorValue
    //
    // A driver-specific value that indicates where the error was detected in the driver. Setting this value is optional
    //
    void SetUniqueErrorValue(
        __in ULONG Value
        );

    // SetFinalStatus
    //
    // Specifies the NTSTATUS value to be returned for the operation that triggered the error. Setting this value is optional.
    //
    void SetFinalStatus(
        __in ULONG FinalStatus
        );

    // SetSequenceNumber
    //
    // Specifies a driver-assigned sequence number for the current IRP, which should be constant for the life of a given request. Setting this value is optional.
    //
    void SetSequenceNumber(
        __in ULONG SequenceNumber
        );

    // SetIoControlCode
    //
    // For an IRP_MJ_DEVICE_CONTROL or IRP_MJ_INTERNAL_DEVICE_CONTROL IRP, this member specifies the I/O control code for the request that
    // trigged the error. Otherwise, this value is zero. Setting this value is optional.
    //
    void SetIoControlCode(
        __in ULONG IoControlCode
        );
};

class KEventLog
{
public:
    // Log
    //
    // Write an entry to the Windows event log for this driver.
    //
    static NTSTATUS Log(
        __in KEventLogEntry& EventToLog
        );
};

