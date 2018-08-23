
/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kregistry.h

    Description:
      Kernel Tempate Library (KTL): KRegistry

      Describes general purpose registry accessor class for
      the HKEY_LOCAL_MACHINE hive.

    History:
      raymcc          30-Mar-11     First version

--*/

#pragma once


class KRegistry
{
    K_DENY_COPY(KRegistry);
    K_NO_DYNAMIC_ALLOCATE();

public:
    // Constructor
    //
    KRegistry(KAllocator& Allocator)
        :   _Allocator(Allocator)
    {
        _hKey = NULL;
    }

    // Destructor
    //
    // Closes any open key resources.
    //
   ~KRegistry();

    // Open
    //
    // Opens a key within the HKEY_LOCAL_MACHINE hive.
    //
    // Parameters:
    //      HKLM_Subkey     The string representing a key name under HKLM.
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_ACCESS_DENIED
    //      STATUS_UNSUCCESSFULL
    //      STATUS_OBJECT_NAME_NOT_FOUND
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    Open(
        __in const KWString& HKLM_SubKey
        );

    // ReadValue
    //
    // Reads a string value
    //
    // Parameters:
    //      ValueName           The name of the regsitry value to read.
    //      Value               Receives the string.
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_ACCESS_DENIED
    //      STATUS_UNSUCCESSFULL
    //      STATUS_OBJECT_NAME_NOT_FOUND
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    //
    NTSTATUS
    ReadValue(
        __in  const KWString& ValueName,
        __out KWString& Value
        );

    // ReadValue
    //
    // Reads a ULONG
    //
    // Parameters:
    //
    //      ValueName           The name of the regsitry value to read.
    //      Value               Receives the ULONG value
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_ACCESS_DENIED
    //      STATUS_UNSUCCESSFULL
    //      STATUS_OBJECT_NAME_NOT_FOUND
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    //
    NTSTATUS
    ReadValue(
        __in  const KWString& ValueName,
        __out ULONG& Value
        );

    // ReadValue
    //
    // Reads a ULONGLONG
    //
    // Parameters:
    //
    //      ValueName           The name of the regsitry value to read.
    //      Value               Receives the ULONGLONG value
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_ACCESS_DENIED
    //      STATUS_UNSUCCESSFULL
    //      STATUS_OBJECT_NAME_NOT_FOUND
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    //
    NTSTATUS
    ReadValue(
        __in  const KWString& ValueName,
        __out ULONGLONG& Value
        );


    // WriteValue
    //
    // Writes a string
    //
    // Either updates the existing value or creates it if it is not present.
    //
    // Parameters:
    //      ValueName           The name of the value
    //      Value               The value to write
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_ACCESS_DENIED
    //      STATUS_UNSUCCESSFULL
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    WriteValue(
        __in  const KWString& ValueName,
        __out KWString& Value
        );

    // WriteValue
    //
    // Writes a ULONG
    //
    // Either updates the existing value or creates it if it is not present.
    //
    // Parameters:
    //      ValueName           The name of the value
    //      Value               The value to write
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_ACCESS_DENIED
    //      STATUS_UNSUCCESSFULL
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS                                                                                                                                                                                                                                          // Either updates the existing value or creates it if it is not present.
    //
    // Parameters:
    //      ValueName           The name of the value
    //      Value               The value to write
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_ACCESS_DENIED
    //      STATUS_UNSUCCESSFULL
    //      STATUS_INSUFFICIENT_RESOURCES

    WriteValue(
        __in  const KWString& ValueName,
        __out ULONG& Value
        );

    // WriteValue
    //
    // Writes a ULONGLONG
    //
    // Either updates the existing value or creates it if it is not present.
    //
    // Parameters:
    //      ValueName           The name of the value
    //      Value               The value to write
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_ACCESS_DENIED
    //      STATUS_UNSUCCESSFULL
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    WriteValue(
        __in  const KWString& ValueName,
        __out ULONGLONG& Value
        );

    // DeleteValue
    //
    // Deletes a named value from the key
    //
    // Parameters:
    //      ValueName           The name of the value
    //      Value               The value to write
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_ACCESS_DENIED
    //      STATUS_UNSUCCESSFULL
    //      STATUS_OBJECT_NAME_NOT_FOUND
    //
    //
    NTSTATUS
    DeleteValue(
        __in const KWString& ValueName
        );


    // Close
    //
    // Closes the key.
    //
    VOID
    Close();

private:
    KRegistry();            // Disallow


private:

    // UntypedRead
    //
    // An internal for reading all value types.  This is called
    // by the other type-specific read functions.
    //
    NTSTATUS
    GenericRead(
        __in  const KWString& ValueName,
        __out PVOID& Data,
        __out ULONG& Length
        );

    NTSTATUS
    GenericWrite(
        __in const KWString& ValueName,
        __in ULONG DataType,
        __in PVOID DataPtr,
        __in ULONG DataLength
        );

#if KTL_USER_MODE
    HKEY _hKey;
#else
    HANDLE _hKey;
#endif

    KAllocator&     _Allocator;
};
