/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KGuid.h

    Description:
      A comparable GUID struct.

      Generic definitions and all other includes

    History:
      RVD Group         26-Oct-2010         Initial version.

--*/

#pragma once

struct KGuid : public GUID
{
    KGuid(__in BOOLEAN Zero = TRUE)
    {
        if (Zero)
        {
            RtlZeroMemory(this, sizeof(GUID));
        }
    }

    KGuid(
        __in ULONG const    Data1,
        __in USHORT const   Data2,
        __in USHORT const   Data3,
        __in UCHAR const    Data4,
        __in UCHAR const    Data5,
        __in UCHAR const    Data6,
        __in UCHAR const    Data7,
        __in UCHAR const    Data8,
        __in UCHAR const    Data9,
        __in UCHAR const    Data10,
        __in UCHAR const    Data11)
    {
        this->Data1 = Data1;
        this->Data2 = Data2;
        this->Data3 = Data3;
        this->Data4[0] = Data4;
        this->Data4[1] = Data5;
        this->Data4[2] = Data6;
        this->Data4[3] = Data7;
        this->Data4[4] = Data8;
        this->Data4[5] = Data9;
        this->Data4[6] = Data10;
        this->Data4[7] = Data11;
    }

    KGuid(__in const GUID& Right)
    {
        KMemCpySafe(this, sizeof(KGuid), &Right, sizeof(GUID));
    }

    KGuid(__in const KGuid& Right)
    {
        KMemCpySafe(this, sizeof(KGuid), &Right, sizeof(GUID));
    }

    KGuid& operator=(__in const GUID& Right)
    {
        if (this != &Right)
        {
            KMemCpySafe(this, sizeof(KGuid), &Right, sizeof(GUID));
        }
        return *this;
    }

    KGuid& operator=(__in const KGuid& Right)
    {
        if (this != &Right)
        {
            KMemCpySafe(this, sizeof(KGuid), &Right, sizeof(GUID));
        }
        return *this;
    }

    BOOLEAN operator==(__in const GUID& Right) const
    {
        return (RtlCompareMemory(this, &Right, sizeof(GUID)) == sizeof(GUID)) ? TRUE : FALSE;
    }

    BOOLEAN operator==(__in const KGuid& Right) const
    {
        return (RtlCompareMemory(this, &Right, sizeof(GUID)) == sizeof(GUID)) ? TRUE : FALSE;
    }

    BOOLEAN operator!=(__in const GUID& Right) const
    {
        return !(*this == Right);
    }

    BOOLEAN operator!=(__in const KGuid& Right) const
    {
        return !(*this == Right);
    }

    BOOLEAN IsNull() const
    {
        ULONGLONG* Tmp = (ULONGLONG*) this;
        if (*Tmp++ || *Tmp)
        {
           return FALSE;
        }
        return TRUE;
    }

    VOID SetToNull()
    {
        RtlZeroMemory(this, sizeof(GUID));
    }

    VOID Set(__in const GUID& Right)
    {
        if (this != &Right)
        {
            KMemCpySafe(this, sizeof(KGuid), &Right, sizeof(GUID));
        }
    }

    VOID CreateNew()
    {
#if KTL_USER_MODE
        HRESULT r = UuidCreate(this);
        KFatal(r == RPC_S_OK);
#else
        NTSTATUS status = ExUuidCreate(this);
        KFatal(NT_SUCCESS(status));
#endif
    }
};

typedef KGuid* PKGuid;

static_assert(sizeof(GUID) == sizeof(KGuid), "KGuid and GUID sizes mismatch");
