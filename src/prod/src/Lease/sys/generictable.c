// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "LeasLayr.h"
#include "GenericTable.tmh"

RTL_GENERIC_COMPARE_RESULTS
GenericTableCompareLeaseRelationshipContexts(
    __in PRTL_GENERIC_TABLE GenericTable,
    __in PVOID Obj1,
    __in PVOID Obj2
    )

/*++

Routine Description:

    This routine is the lease relationship compare routine called 
    by the generic table.

Arguments:

    GenericTable - This is the table being searched.

    Obj1 - First key value (PLEASE_RELATIONSHIP_IDENTIFIER).

    Obj2 - Second key value (PLEASE_RELATIONSHIP_IDENTIFIER).

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    LPCWSTR RemStr1 = ((*(PLEASE_RELATIONSHIP_IDENTIFIER*)Obj1)->RemoteLeasingApplicationIdentifier);
    LPCWSTR RemStr2 = ((*(PLEASE_RELATIONSHIP_IDENTIFIER*)Obj2)->RemoteLeasingApplicationIdentifier);
    LPCWSTR Str1 = ((*(PLEASE_RELATIONSHIP_IDENTIFIER*)Obj1)->LeasingApplicationContext->LeasingApplicationIdentifier);
    LPCWSTR Str2 = ((*(PLEASE_RELATIONSHIP_IDENTIFIER*)Obj2)->LeasingApplicationContext->LeasingApplicationIdentifier);
    INT Result1 = wcscmp(RemStr1, RemStr2);
    INT Result2 = 0;

    UNREFERENCED_PARAMETER(GenericTable);

    if (Result1 < 0) {

        return GenericLessThan;

    } else if (Result1 > 0) {

        return GenericGreaterThan;
        
    } else if (0 == Result1) {

        Result2 = wcscmp(Str1, Str2);

        if (Result2 < 0) {
            
            return GenericLessThan;
            
        } else if (Result2 > 0) {
            
            return GenericGreaterThan;
            
        } else if (0 == Result2) {

            return GenericEqual;
            
        } else {

            LAssert(FALSE);
        }
    }

    LAssert(FALSE);
    return GenericEqual;
}

RTL_GENERIC_COMPARE_RESULTS
GenericTableCompareLeaseAgentContexts(
    __in PRTL_GENERIC_TABLE GenericTable,
    __in PVOID Obj1,
    __in PVOID Obj2
    )

/*++

Routine Description:

    This routine is the lease agent compare routine called 
    by the generic table.

Arguments:

    GenericTable - This is the table being searched.

    Obj1 - First key value (PLEASE_AGENT_CONTEXT).

    Obj2 - Second key value (PLEASE_AGENT_CONTEXT).

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    LARGE_INTEGER Id1 = ((*(PLEASE_AGENT_CONTEXT*)Obj1)->Instance);
    LARGE_INTEGER Id2 = ((*(PLEASE_AGENT_CONTEXT*)Obj2)->Instance);

    UNREFERENCED_PARAMETER(GenericTable);

    if (IS_LARGE_INTEGER_LESS_THAN_LARGE_INTEGER(Id1, Id2)) {

        return GenericLessThan;

    } else if (IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(Id1, Id2)) {

        return GenericGreaterThan;
    }
    
    return GenericEqual;
}
RTL_GENERIC_COMPARE_RESULTS
GenericTableCompareTransportBlocking(
    __in PRTL_GENERIC_TABLE GenericTable,
    __in PVOID Obj1,
    __in PVOID Obj2
)

/*++

    Routine Description:

    This routine is the lease agent compare routine called
    by the generic table.

    Arguments:

    GenericTable - This is the table being searched.

    Obj1 - First key value (PLEASE_AGENT_CONTEXT).

    Obj2 - Second key value (PLEASE_AGENT_CONTEXT).

    Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
    input structures

--*/

{
    LARGE_INTEGER Id1 = ((*(PTRANSPORT_BLOCKING_TEST_DESCRIPTOR*)Obj1)->Instance);
    LARGE_INTEGER Id2 = ((*(PTRANSPORT_BLOCKING_TEST_DESCRIPTOR*)Obj2)->Instance);

    UNREFERENCED_PARAMETER(GenericTable);

    if (IS_LARGE_INTEGER_LESS_THAN_LARGE_INTEGER(Id1, Id2)) {

        return GenericLessThan;

    }
    else if (IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(Id1, Id2)) {

        return GenericGreaterThan;
    }

    return GenericEqual;
}

RTL_GENERIC_COMPARE_RESULTS
GenericTableCompareRemoteLeaseAgentContexts(
    __in PRTL_GENERIC_TABLE GenericTable,
    __in PVOID Obj1,
    __in PVOID Obj2
    )

/*++

Routine Description:

    This routine is the remote lease agent compare routine called by the generic table.

Arguments:

    GenericTable - This is the table being searched.

    Obj1 - First key value (PREMOTE_LEASE_AGENT_CONTEXT).

    Obj2 - Second key value (PREMOTE_LEASE_AGENT_CONTEXT).

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    LARGE_INTEGER Id1 = ((*(PREMOTE_LEASE_AGENT_CONTEXT*)Obj1)->Instance);
    LARGE_INTEGER Id2 = ((*(PREMOTE_LEASE_AGENT_CONTEXT*)Obj2)->Instance);

    UNREFERENCED_PARAMETER(GenericTable);

    if (IS_LARGE_INTEGER_LESS_THAN_LARGE_INTEGER(Id1, Id2)) {

        return GenericLessThan;

    } else if (IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(Id1, Id2)) {

        return GenericGreaterThan;
    }
    
    return GenericEqual;
}

RTL_GENERIC_COMPARE_RESULTS
GenericTableCompareLeasingApplicationContexts(
    __in PRTL_GENERIC_TABLE GenericTable,
    __in PVOID Obj1,
    __in PVOID Obj2
    )

/*++

Routine Description:

    This routine is the leasing application compare routine called 
    by the generic table.

Arguments:

    GenericTable - This is the table being searched.

    Obj1 - First key value (PLEASING_APPLICATION_CONTEXT).

    Obj2 - Second key value (PLEASING_APPLICATION_CONTEXT).

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    LPWSTR Str1 = ((*(PLEASING_APPLICATION_CONTEXT*)Obj1)->LeasingApplicationIdentifier);
    LPWSTR Str2 = ((*(PLEASING_APPLICATION_CONTEXT*)Obj2)->LeasingApplicationIdentifier);
    INT Result = wcscmp(Str1, Str2);

    UNREFERENCED_PARAMETER(GenericTable);

    if (Result < 0) {

        return GenericLessThan;

    } else if (Result > 0) {

        return GenericGreaterThan;
    }
    
    return GenericEqual;
}

RTL_GENERIC_COMPARE_RESULTS
GenericTableCompareLeaseEvents(
    __in PRTL_GENERIC_TABLE GenericTable,
    __in PVOID Obj1,
    __in PVOID Obj2
    )

/*++

Routine Description:

    This routine is the lease event compare routine called by the generic table.

Arguments:

    GenericTable - This is the table being searched.

    Obj1 - First key value (PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER).

    Obj2 - Second key value (PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER).

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER Ev1 = (*(PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER*)Obj1);
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER Ev2 = (*(PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER*)Obj2);

    LONGLONG InsertTimeEv1 = Ev1->insertionTime.QuadPart;
    LONGLONG InsertTimeEv2 = Ev2->insertionTime.QuadPart;

    UNREFERENCED_PARAMETER(GenericTable);

    if (InsertTimeEv1 < InsertTimeEv2)
    {
        return GenericLessThan;
    }
    else if (InsertTimeEv1 > InsertTimeEv2)
    {
        return GenericGreaterThan;
    }
    else
    {
        if (Ev1 < Ev2)
        {
            return GenericLessThan;
        }
        else if (Ev1 > Ev2)
        {
            return GenericGreaterThan;
        }
        else if (Ev1 == Ev2)
        {
            return GenericEqual;
        }
        else
        {
            LAssert(FALSE);
        }
    }

    return GenericEqual;
}

RTL_GENERIC_COMPARE_RESULTS
GenericTableCompareCertificates(
    __in PRTL_GENERIC_TABLE GenericTable,
    __in PVOID Obj1,
    __in PVOID Obj2
    )

/*++

Routine Description:

    This routine is the certificates compare routine called by the generic table.

--*/

{
    LONG Compare = 0;
    PLEASE_REMOTE_CERTIFICATE Cert1 = (*(PLEASE_REMOTE_CERTIFICATE*)Obj1);
    PLEASE_REMOTE_CERTIFICATE Cert2 = (*(PLEASE_REMOTE_CERTIFICATE*)Obj2);

    UNREFERENCED_PARAMETER(GenericTable);

    if (Cert1->cbCertificate < Cert2->cbCertificate)
    {
        return GenericLessThan;
    }
    else if (Cert1->cbCertificate > Cert2->cbCertificate)
    {
        return GenericGreaterThan;
    }

    Compare = memcmp(Cert1->pbCertificate, Cert2->pbCertificate, Cert1->cbCertificate);
    if (Compare < 0)
    {
        return GenericLessThan;
    }
    else if (Compare > 0)
    {
        return GenericGreaterThan;
    }

    return GenericEqual;
}

RTL_GENERIC_COMPARE_RESULTS
GenericTableCompareLeaseRemoteCertOp(
    __in PRTL_GENERIC_TABLE GenericTable,
    __in PVOID Obj1,
    __in PVOID Obj2
    )

/*++

Routine Description:

    This routine is the Lease Remote Cert structure verify operation compare routine called by the generic table.

--*/

{
    PLEASE_REMOTE_CERTIFICATE Cert1 = (*(PLEASE_REMOTE_CERTIFICATE*)Obj1);
    PLEASE_REMOTE_CERTIFICATE Cert2 = (*(PLEASE_REMOTE_CERTIFICATE*)Obj2);

    UNREFERENCED_PARAMETER(GenericTable);

    if (Cert1->verifyOperation < Cert2->verifyOperation) {

        return GenericLessThan;

    } else if (Cert1->verifyOperation > Cert2->verifyOperation) {

        return GenericGreaterThan;
    }
    
    return GenericEqual;
}

PVOID
GenericTableAllocate(
    __in PRTL_GENERIC_TABLE GenericTable,
    __in CLONG ByteSize
    )

/*++

Routine Description:

    This is a generic table support routine to allocate memory.

Arguments:

    GenericTable - Supplies the generic table being used.

    ByteSize - Supplies the number of bytes to allocate.

Return Value:

    PVOID - Returns a pointer to the allocated data

--*/

{
    UNREFERENCED_PARAMETER(GenericTable);

    return ExAllocatePoolWithTag(
        NonPagedPool, 
        ByteSize, 
        LEASE_TABLE_TAG
        );
}

VOID
GenericTableDeallocate(
    __in PRTL_GENERIC_TABLE GenericTable,
    __in __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer
    )

/*++

Routine Description:

    This is a generic table support routine that deallocates memory.

Arguments:

    GenericTable - Supplies the generic table being used.

    Buffer - Supplies the buffer being deallocated.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(GenericTable);

    ExFreePool(Buffer);
}
