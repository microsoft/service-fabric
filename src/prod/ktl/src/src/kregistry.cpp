
#include <ktl.h>


// KRegistry::Open
//
// Opens a subkey of HKEY_LOCAL_MACHINE
//
//
NTSTATUS
KRegistry::Open(
    __in const KWString& HKLM_Subkey
    )
{
    NTSTATUS Res = STATUS_INTERNAL_ERROR;

#if KTL_USER_MODE


    Res = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        PWSTR(HKLM_Subkey),
        0,
        KEY_ALL_ACCESS,
        &_hKey
        );

    if (Res == ERROR_SUCCESS)
    {
        return STATUS_SUCCESS;
    }

    if (Res == ERROR_ACCESS_DENIED)
    {
        return STATUS_ACCESS_DENIED;
    }

    if (Res == ERROR_FILE_NOT_FOUND)
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    return STATUS_UNSUCCESSFUL;

#else
    OBJECT_ATTRIBUTES Attr;

    KWString RealKey(_Allocator, L"\\Registry\\Machine\\");
    RealKey += HKLM_Subkey;

    if (!NT_SUCCESS(RealKey.Status()))
    {
        return RealKey.Status();
    }

    InitializeObjectAttributes(
        &Attr,
        PUNICODE_STRING(RealKey),
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    Res = ZwOpenKey(
        &_hKey,
        KEY_ALL_ACCESS,
        &Attr
        );

    return Res;
#endif
}

// KRegistry::Close
//
// Closes the currently open key
//
VOID
KRegistry::Close()
{
#if KTL_USER_MODE
    RegCloseKey(_hKey);
#else
    ZwClose(_hKey);
    _hKey = NULL;
#endif
}


// KRegistry destructor
//
//
KRegistry::~KRegistry()
{
    if (_hKey != NULL)
    {
        Close();
    }
}


//  KRegistry::ReadValue
//
//
NTSTATUS
KRegistry::ReadValue(
    __in  const KWString& ValueName,
    __out ULONG& Value
    )
{
    PVOID Mem = nullptr;
    KFinally( [&](){ _delete(Mem); });

    ULONG Length = 0;
    NTSTATUS Res = GenericRead(ValueName, Mem, Length);

    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

#if KTL_USER_MODE

    if (Length != sizeof(ULONG))
    {
        return STATUS_UNSUCCESSFUL;
    }

    Value = *(PULONG) Mem;
    return STATUS_SUCCESS;

#else
    PKEY_VALUE_PARTIAL_INFORMATION KeyInf = PKEY_VALUE_PARTIAL_INFORMATION(Mem);

    if (KeyInf->DataLength != sizeof(ULONG))
    {
        return STATUS_UNSUCCESSFUL;
    }
    Value = *(PULONG) KeyInf->Data;
    return STATUS_SUCCESS;

#endif
}

//  KRegistry::ReadValue
//
//
NTSTATUS
KRegistry::ReadValue(
    __in  const KWString& ValueName,
    __out ULONGLONG& Value
    )
{
    PVOID Mem = nullptr;
    KFinally( [&](){ _delete(Mem); });

    ULONG Length = 0;
    NTSTATUS Res = GenericRead(ValueName, Mem, Length);

    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

#if KTL_USER_MODE

    if (Length != sizeof(ULONGLONG))
    {
        return STATUS_UNSUCCESSFUL;
    }

    Value = *(PULONGLONG) Mem;
    return STATUS_SUCCESS;

#else
    PKEY_VALUE_PARTIAL_INFORMATION KeyInf = PKEY_VALUE_PARTIAL_INFORMATION(Mem);

    if (KeyInf->DataLength != sizeof(ULONGLONG))
    {
        return STATUS_UNSUCCESSFUL;
    }
    Value = *(PULONGLONG) KeyInf->Data;
    return STATUS_SUCCESS;

#endif
}



//  KRegistry::ReadValue
//
//
NTSTATUS
KRegistry::ReadValue(
    __in  const KWString& ValueName,
    __out KWString& Value
    )
{
    PVOID Mem = nullptr;
    KFinally( [&](){ _delete(Mem); });

    ULONG Length = 0;
    NTSTATUS Res = GenericRead(ValueName, Mem, Length);

    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

#if KTL_USER_MODE

    Value = PWSTR(Mem);
    return Value.Status();

#else
    PKEY_VALUE_PARTIAL_INFORMATION KeyInf = PKEY_VALUE_PARTIAL_INFORMATION(Mem);

    UNICODE_STRING Str;
    Str.Buffer = (PWSTR) KeyInf->Data;
    Str.Length = (USHORT) KeyInf->DataLength;
    Str.MaximumLength = (USHORT) KeyInf->DataLength;

    Value = Str;
    return Value.Status();

#endif
}

// WriteValue
//
// Writes a KWString
//
NTSTATUS
KRegistry::WriteValue(
    __in  const KWString& ValueName,
    __in  KWString& Value
    )
{
    PUNICODE_STRING PStr = Value;

    return GenericWrite(
        ValueName,
        REG_SZ,
        PStr->Buffer,
        PStr->Length
        );
}


// WriteValue
//
// Writes a ULONG
//
NTSTATUS
KRegistry::WriteValue(
    __in  const KWString& ValueName,
    __in  ULONG& Value
    )
{
    return GenericWrite(
        ValueName,
        REG_DWORD,
        &Value,
        sizeof(ULONG)
        );
}

// WriteValue
//
// Writes a ULONGLONG
//
NTSTATUS
KRegistry::WriteValue(
    __in  const KWString& ValueName,
    __in  ULONGLONG& Value
    )
{
    return GenericWrite(
        ValueName,
        REG_QWORD,
        &Value,
        sizeof(ULONGLONG)
        );
}


//  KRegistry::GenericWrite
//
//  Implements the actual read and works for all data types.
//  The other functions call this one.

NTSTATUS
KRegistry::GenericWrite(
    __in const KWString& ValueName,
    __in ULONG DataType,
    __in PVOID DataPtr,
    __in ULONG DataLength
    )
{
#if KTL_USER_MODE
    LONG Result = RegSetValueEx(
        _hKey,
        PWSTR(ValueName),
        0,
        DataType,
        (BYTE*) DataPtr,
        DataLength
        );

    if (Result == ERROR_SUCCESS)
    {
        return STATUS_SUCCESS;
    }
    if (Result == ERROR_ACCESS_DENIED)
    {
        return STATUS_ACCESS_DENIED;
    }
    return STATUS_UNSUCCESSFUL;

#else
    return ZwSetValueKey(
        _hKey,
        PUNICODE_STRING(const_cast<KWString&>(ValueName)),
        0,
        DataType,
        DataPtr,
        DataLength
        );
#endif
}


//  KRegistry::GenericRead
//
//  Implements the actual read and works for all data types.
//  The other functions call this one.
//
//  Parameters:
//      ValueName           The specified value name under the key.
//      Data                Receives a dynamically allocated pointer which
//                          must be deallocated at some point.
//      Length              The number of bytes in Data.
//
//  Return value:
//      STATUS_SUCCESS
//      STATUS_OBJECT_NAME_NOT_FOUND    The specified value could not be found.
//      STATUS_UNSCCESSFUL  Other errors
//
NTSTATUS
KRegistry::GenericRead(
    __in  const KWString& ValueName,
    __out PVOID& Data,
    __out ULONG& Length
    )
{
#if KTL_USER_MODE
    ULONG Required = 0;

    LONG Result = RegQueryValueEx(
        _hKey,
        PWSTR(ValueName),
        0,
        NULL,
        NULL,
        &Required
        );

    if (Result != NO_ERROR)
    {
        if (Result == ERROR_FILE_NOT_FOUND)
        {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
        else
            return STATUS_UNSUCCESSFUL;
    }

    // If here, we now know how much memory to allocate.
    //
    PUCHAR Mem = _newArray<UCHAR>(KTL_TAG_REGISTRY, _Allocator, Required);
    if (!Mem)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
	Length = Required;
    Result = RegQueryValueEx(
        _hKey,
        PWSTR(ValueName),
        0,
        NULL,
        Mem,
        &Length
        );

    if (Result == ERROR_SUCCESS && Required == Length)
    {
        Data = Mem;
        return STATUS_SUCCESS;
    }

    _deleteArray(Mem);
    return STATUS_UNSUCCESSFUL;

#else  // Kernel mode implementation

    ULONG SizeNeeded = 0;

    // Determine the value size in the first call.
    //
    NTSTATUS Result = ZwQueryValueKey(_hKey,
        PUNICODE_STRING(const_cast<KWString&>(ValueName)),
        KeyValuePartialInformation,
        NULL,
        0,
        &SizeNeeded
        );

    if ((Result == STATUS_BUFFER_TOO_SMALL) || (Result == STATUS_BUFFER_OVERFLOW))
    {
        // Allocate space to hold the info
        //
        PUCHAR Mem = _newArray<UCHAR>(KTL_TAG_REGISTRY, _Allocator, SizeNeeded);
        if (Mem == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(Mem, SizeNeeded);

        // Get the value into our temp buffer
        //
        Result = ZwQueryValueKey(
             _hKey,
             PUNICODE_STRING(const_cast<KWString&>(ValueName)),
             KeyValuePartialInformation,
             Mem,
             SizeNeeded,
             &Length
             );

        if ((Result != STATUS_SUCCESS) || (Length != SizeNeeded))
        {
            _deleteArray(Mem);
            return STATUS_UNSUCCESSFUL;
        }

        // Assign it
        //
        Data = Mem;
    }

    return Result;

#endif
}



    // Deletes a named value from the key
    //
NTSTATUS
KRegistry::DeleteValue(
    __in const KWString& ValueName
    )
{
#if KTL_USER_MODE
    LONG Res = RegDeleteValue(_hKey, PWSTR(ValueName));
    if (Res == ERROR_SUCCESS)
    {
        return STATUS_SUCCESS;
    }
    if (Res == ERROR_FILE_NOT_FOUND)
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    if (Res == ERROR_ACCESS_DENIED)
    {
        return STATUS_ACCESS_DENIED;
    }
    return STATUS_UNSUCCESSFUL;

#else
    return ZwDeleteValueKey(_hKey, PUNICODE_STRING(const_cast<KWString&>(ValueName)));
#endif
}

