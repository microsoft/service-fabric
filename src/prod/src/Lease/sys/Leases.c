// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "LeasLayr.h"
#include "Leases.tmh"

BOOLEAN
IsRemoteLeaseAgentFailed(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Checks to see if the remote lease agent has already failed.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent to check.

Return Value:

    TRUE if the remote lease agent has failed.

--*/

{
    return (FAILED == RemoteLeaseAgentContext->State);
}

BOOLEAN
IsRemoteLeaseAgentSuspended(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Checks to see if the remote lease agent cannot yet be used.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent to check.

Return Value:

    TRUE if the remote lease agent has failed.

--*/

{
    return (SUSPENDED == RemoteLeaseAgentContext->State);
}


BOOLEAN
IsRemoteLeaseAgentOrphaned(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Checks to see if the remote lease agent was orphaned at some point in time in the past.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent to check.

Return Value:

    TRUE if the remote lease agent has failed.

--*/

{
    LARGE_INTEGER Now;
    GetCurrentTime(&Now);

    BOOLEAN result = (
        OPEN == RemoteLeaseAgentContext->State &&
        LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState &&
        LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState &&
        FALSE == RemoteLeaseAgentContext->InPing &&
        IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(Now, RemoteLeaseAgentContext->TimeToBeFailed)
        );

    EventWriteRemoteLeaseAgentOrphan(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseAgentState(RemoteLeaseAgentContext->State),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
        RemoteLeaseAgentContext->IsActive,
        RemoteLeaseAgentContext->InPing,
        RemoteLeaseAgentContext->TimeToBeFailed.QuadPart,
        Now.QuadPart,
        result
        );

    return result;
}

BOOLEAN
IsRemoteLeaseAgentOpen(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )
/*++

Routine Description:
    Checks to see if the remote lease agent is in OPEN state and either subject or monitor is ACTIVE.

--*/
{
    return (
        OPEN == RemoteLeaseAgentContext->State && (
        LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState ||
        LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        );
}

BOOLEAN
IsRemoteLeaseAgentOpenTwoWay(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
)
/*++

Routine Description:
Checks to see if the remote lease agent is in OPEN state and subject and monitor are ACTIVE.

--*/
{
    return (
        OPEN == RemoteLeaseAgentContext->State &&
        LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState &&
        LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState
        );
}

BOOLEAN
IsRemoteLeaseAgentInPing(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Checks to see if the remote lease agent started the ping process.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent to check.

Return Value:

    TRUE if the remote lease agent has failed.

--*/

{
    return (
        OPEN == RemoteLeaseAgentContext->State &&
        LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState &&
        LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState &&
        TRUE == RemoteLeaseAgentContext->InPing
        );
}

BOOLEAN
IsLeaseAgentFailed(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    )

/*++

Routine Description:

    Checks to see if the lease has already expired.

Parameters Description:

    LeaseAgentContext - remote lease agent to check.

Return Value:

    TRUE if the lease agent has failed.

--*/

{
    return (FAILED == LeaseAgentContext->State);
}

BOOLEAN
CanLeaseAgentBeFailed(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    )

/*++

Routine Description:

    Checks if the lease agent can be failed at this point.

Parameters Description:

    LeaseAgentContext - lease agent to be failed.

Return Value:

    n/a

--*/

{

    if (HasLeasingApplication(LeaseAgentContext))
    {
        // still has leasing app, can't fail this LA yet
        return FALSE;
    }

    if (HasNonFailedRemoteLeaseAgent(LeaseAgentContext))
    {
        // still has RLA that is not in the FAILED state, can't fail this LA yet
        return FALSE;
    }

    EventWriteCanFailLA(
        NULL,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart
        );

    return TRUE;
}

BOOLEAN
HasLeasingApplication(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext
)
{
    PVOID IterLeasingApplication = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;

    //
    // Check registered leasing applications on this lease agent.
    //
    if (0 != RtlNumberGenericTableElements(&LeaseAgentContext->LeasingApplicationContextHashTable))
    {
        //
        // Get the first application that is still registered.
        //
        IterLeasingApplication = RtlGetElementGenericTable(
            &LeaseAgentContext->LeasingApplicationContextHashTable,
            0
            );
        LAssert(NULL != IterLeasingApplication);

        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

        //
        // We still have leasing applications that have not yet unregistered.
        //
        EventWriteCannotFailLALeaseAppRegistered(
            NULL,
            TransportIdentifier(LeaseAgentContext->Transport),
            LeaseAgentContext->Instance.QuadPart,
            LeasingApplicationContext->LeasingApplicationIdentifier
            );

        return TRUE;
    }

    return FALSE;
}

BOOLEAN
HasNonFailedRemoteLeaseAgent(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext
)
{
    PVOID IterRemoteLeaseAgent = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;

    for (IterRemoteLeaseAgent = RtlEnumerateGenericTable(
        &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
        TRUE);
    NULL != IterRemoteLeaseAgent;
    IterRemoteLeaseAgent = RtlEnumerateGenericTable(
        &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
        FALSE))
    {
        RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

        if (!IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext))
        {
            //
            // We found at least one remote lease agent that has not yet failed.
            //
            EventWriteCannotFailLARemoteLAInState(
                NULL,
                TransportIdentifier(LeaseAgentContext->Transport),
                LeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                GetLeaseAgentState(RemoteLeaseAgentContext->State)
                );

            return TRUE;
        }
    }

    return FALSE;
}

PREMOTE_LEASE_AGENT_CONTEXT
GetActiveRemoteLeaseAgent(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext
)
{
    PVOID IterRemoteLeaseAgent = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;

    for (IterRemoteLeaseAgent = RtlEnumerateGenericTable(
        &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
        TRUE);
    NULL != IterRemoteLeaseAgent;
    IterRemoteLeaseAgent = RtlEnumerateGenericTable(
        &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
        FALSE))
    {
        RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

        if (IsRemoteLeaseAgentOpen(RemoteLeaseAgentContext) && TRUE == RemoteLeaseAgentContext->IsActive)
        {
            //
            // We found at least one remote lease agent that is active.
            //
            EventWriteCanNotDelLARemoteLAState(
                NULL,
                TransportIdentifier(LeaseAgentContext->Transport),
                LeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                GetLeaseAgentState(RemoteLeaseAgentContext->State)
                );

            return RemoteLeaseAgentContext;
        }
    }

    return NULL;
}

BOOLEAN
IsLeaseAgentReadyForDeallocation(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    )

/*++

Routine Description:

    Checks if the lease agent can be deallocated.

Parameters Description:

    LeaseAgentContext - lease agent to be deallocated.

Return Value:

    n/a

--*/

{
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PVOID IterRemoteLeaseAgent = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PVOID IterLeasingApplication = NULL;

    LAssert(IsLeaseAgentFailed(LeaseAgentContext));

    //
    // Check if listening socket was closed already.
    // This implies all dependent connections are closed as well.
    //
    if (!IsTransportClosed(LeaseAgentContext->Transport)) {

        EventWriteCanNotDelLASocNotClosed(
            NULL,
            TransportIdentifier(LeaseAgentContext->Transport),
            LeaseAgentContext->Instance.QuadPart
            );
        return FALSE;
    }

    if (LeaseAgentContext->IsInDelayLeaseFailureLeaseAgentCloseTimer == TRUE)
    {
        EventWriteCanNotDelLeaseAgentInDelayLeaseFailureLeaseAgentClose(
            NULL,
            TransportIdentifier(LeaseAgentContext->Transport),
            LeaseAgentContext->Instance.QuadPart,
            GetLeaseAgentState(LeaseAgentContext->State)
            );

        return FALSE;
    }

    //
    // Check subject failed pending list for each remote lease agent.
    //
    for (IterRemoteLeaseAgent = RtlEnumerateGenericTable(
            &LeaseAgentContext->RemoteLeaseAgentContextHashTable, 
            TRUE);
         NULL != IterRemoteLeaseAgent;
         IterRemoteLeaseAgent = RtlEnumerateGenericTable(
            &LeaseAgentContext->RemoteLeaseAgentContextHashTable, 
            FALSE)) {
        
        //
        // Retrieve remote lease agent context.
        //
        RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

        if (!IsRemoteLeaseAgentReadyForDeallocation(RemoteLeaseAgentContext, FALSE)) {

            //
            // We found at least one remote lease agent that has not yet failed or cannot be deallocated yet.
            //
            
            EventWriteCanNotDelLARemoteLAState(
                NULL,
                TransportIdentifier(LeaseAgentContext->Transport),
                LeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                GetLeaseAgentState(RemoteLeaseAgentContext->State)
                ); 

            return FALSE;
        }
    }
    //
    // Check if any application is being unregistered
    //
    for (IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextHashTable, TRUE);
        NULL != IterLeasingApplication;
        IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextHashTable, FALSE))
    {
        //
        // Retrieve leasing application.
        //
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);
        if (TRUE == LeasingApplicationContext->IsBeingUnregistered)
        {
            EventWriteCanNotDelLALeasingAppIsBeingUnregistered(
                NULL,
                LeasingApplicationContext->LeasingApplicationIdentifier,
                LeasingApplicationContext->IsBeingUnregistered
                );

            return FALSE;
        }

    }

    //
    // Check if any application not exited
    //
    for (IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextUnregisterList, TRUE);
        NULL != IterLeasingApplication;
        IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextUnregisterList, FALSE))
    {
        //
        // Retrieve leasing application.
        //
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

        if (IS_LARGE_INTEGER_GREATER_THAN_INTEGER(LeasingApplicationContext->ProcessAssertTime, 0))
        {
            LARGE_INTEGER Now;
            GetCurrentTime(&Now);
            LARGE_INTEGER ProcessAssertDiff;
            LARGE_INTEGER_SUBTRACT_LARGE_INTEGER_AND_ASSIGN(ProcessAssertDiff, Now, LeasingApplicationContext->ProcessAssertTime);
            EventWriteProcessAssertExitPending(NULL, LeasingApplicationContext->ProcessAssertTime.QuadPart, ProcessAssertDiff.QuadPart, DriverContext->ProcessAssertExitTimeout * MILLISECOND_TO_NANOSECOND_FACTOR);

            return FALSE;
        }

    }

    EventWriteCanDelLA(
        NULL,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart
        ); 

    return TRUE;
}

VOID
SetRemoteLeaseAgentFailed(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Marks this remote lease agent as failed.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent to be failed.

Return Value:

    n/a

--*/

{
    //
    // Uninitialize the remote lease agent context.
    //
    RemoteLeaseAgentUninitialize(RemoteLeaseAgentContext);
}

VOID
SetRemoteLeaseAgentState(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LEASE_AGENT_STATE State
    )

/*++

Routine Description:

    Marks this remote lease agent as failed.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent.

    State - new state of the remote lease agent.

Return Value:

    n/a

--*/

{
    EventWriteRemoteLAStateTransition(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        GetLeaseAgentState(RemoteLeaseAgentContext->State),
        GetLeaseAgentState(State)
        );
    RemoteLeaseAgentContext->State = State;
}

VOID
SetLeaseAgentFailed(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in BOOLEAN IsDelayLeaseAgentClose
    )

/*++

Routine Description:

    Marks this lease agent and all its remote lease agents as failed.

Parameters Description:

    LeaseAgentContext - lease agent to be failed.

Return Value:

    n/a

--*/

{
    //
    // Quiesce the lease agent.
    //
    LeaseAgentUninitialize(LeaseAgentContext, IsDelayLeaseAgentClose);
}

VOID
OnLeaseFailure(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    )
    
/*++

Routine Description:

    Subject expiration routine.

Parameters Description:

    LeaseAgentContext - lease agent that expired.

Return Value:

    n/a

--*/

{
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PVOID IterLeasingApplication = NULL;
    LONG LeasingApplicationTTL = 0;
    LONG MaxTTL = 0;

    //
    // If the lease agent is already failed then do nothing.
    //
    if (!IsLeaseAgentFailed(LeaseAgentContext) && !LeaseAgentContext->IsInDelayLeaseFailureLeaseAgentCloseTimer)
    {
        // Fail all applications.
        //
        for (IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextHashTable, TRUE);
             NULL != IterLeasingApplication;
             IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextHashTable, FALSE))
        {
            // Retrieve leasing application.
            //
            LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

            GetLeasingApplicationTTL(
                LeasingApplicationContext,
                0, // requestTTL
                &LeasingApplicationTTL
                );

            if(MaxTTL < LeasingApplicationTTL)
            {
                MaxTTL = LeasingApplicationTTL;
            }
        }

        // The unregister application will happen after ApplicationTTL. The failed
        // termination request will be sent to the neighbors.
        //

        if (LeasingApplicationTTL > LeaseAgentContext->LeaseSuspendDuration)
        {
            // Use suspend duration as a TTL upper bound
            EventWriteLeaseDriverTextTraceError(
                NULL,
                L"Delay failed lease agent get invalid TTL ",
                LeasingApplicationTTL);

            LeasingApplicationTTL = LeaseAgentContext->LeaseSuspendDuration;
        }

        ArmDelayedLeaseFailureTimer(
            LeaseAgentContext,
            MaxTTL
            );
        
    }
    else
    {
        if (LeaseAgentContext->IsInDelayLeaseFailureLeaseAgentCloseTimer)
        {
            EventWriteDelayTimer(
                NULL,
                L" was enqueued ",
                TransportIdentifier(LeaseAgentContext->Transport),
                LeaseAgentContext->Instance.QuadPart
                );
        }
        else
        {
            EventWriteLeaseAgentInState(
                NULL,
                TransportIdentifier(LeaseAgentContext->Transport),
                LeaseAgentContext->Instance.QuadPart,
                GetLeaseAgentState(FAILED)
                );
        }
    }
}

VOID
OnSubjectExpired(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )
    
/*++

Routine Description:

    Subject expiration routine.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent (subject) that expired.

Return Value:

    n/a

--*/

{
    EventWriteLeaseRelationInState1(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        );
    //
    // The lease relationship has failed.
    //

    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_FAILED;
    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_INACTIVE;

    //
    // Signal lease failure.
    //
    OnLeaseFailure(RemoteLeaseAgentContext->LeaseAgentContext);
}

VOID
InformLeasingAplicationsRemoteLeaseAgentFailed(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN SubjectLeasingApplications
    )

/*++

Routine Description:

    Raise remote leasing application event to all local leasing applications that have
    established lease through this remote lease agent..

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent that failed.

    SubjectLeasingApplications - indicates subject/monitor leasing applications to be informed.

Return Value:

    n/a

--*/

{
    PVOID IterLeasingApplication = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PVOID IterLeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PRTL_GENERIC_TABLE  HashTable = SubjectLeasingApplications ? 
        &RemoteLeaseAgentContext->SubjectHashTable :
        &RemoteLeaseAgentContext->MonitorHashTable;
        
    //
    // Inform all local subjects/monitors that the remote monitors/subjects have failed.
    //
    for (IterLeaseRelationshipIdentifier = RtlEnumerateGenericTable(
            HashTable,
            TRUE);
         NULL != IterLeaseRelationshipIdentifier;
         IterLeaseRelationshipIdentifier = RtlEnumerateGenericTable(
            HashTable,
            FALSE)) {

        //
        // Retrieve lease identifier.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterLeaseRelationshipIdentifier);

        if (!SubjectLeasingApplications) {

            SwitchLeaseRelationshipLeasingApplicationIdentifiers(LeaseRelationshipIdentifier);
        }
        
        //
        // Retrieve leasing application.
        //
        IterLeasingApplication = RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable,
            &LeaseRelationshipIdentifier->LeasingApplicationContext
            );

        if (!SubjectLeasingApplications) {

            SwitchLeaseRelationshipLeasingApplicationIdentifiers(LeaseRelationshipIdentifier);
        }

        if (NULL != IterLeasingApplication) {

            LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);
        
            //
            // Fire remote leasing application expired notification. 
            //
            TryDelayedRequestCompletion(
                LeasingApplicationContext,
                REMOTE_LEASING_APPLICATION_EXPIRED,
                NULL,
                LeasingApplicationContext->Instance,
                SubjectLeasingApplications ? 
                    LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier:
                    LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                -1,
                -1,
                NULL,
                0,
                0,
                STATUS_SUCCESS,
                TRUE,
                0,
                NULL,
                NULL,
                0,
                NULL,
                NULL,
                0,
                0,
                0,
                0
                );
        }
    }
}

VOID
MarkLeaseRelationshipSent(
    __in PVOID State,
    __in BOOLEAN sending)
{
    PREMOTE_LEASE_AGENT_CONTEXT Context = (PREMOTE_LEASE_AGENT_CONTEXT)State;

    if (FALSE == Context->LeaseRelationshipContext->LeaseMessageSent && TRUE == sending)
    {
        Context->LeaseRelationshipContext->LeaseMessageSent = TRUE;
    }

    Release(Context);
}

VOID
OnMonitorExpired(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )
    
/*++

Routine Description:

    Monitor expiration procedure.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent (subject) that expired.

Return Value:

    n/a

--*/

{
    if (!IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext)) {

        EventWriteLeaseRelationInState2(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
            );
        //
        // The lease relationship has failed.
        //
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_INACTIVE;
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_FAILED;

        //
        // Signal lease failure.
        //
        SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);

        //
        // Inform applications that the remote lease agent has failed.
        //
        InformLeasingAplicationsRemoteLeaseAgentFailed(
            RemoteLeaseAgentContext,
            FALSE
            );
        InformLeasingAplicationsRemoteLeaseAgentFailed(
            RemoteLeaseAgentContext,
            TRUE
            );
    }
}

VOID
CreateArbitrationRequest(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Creates an arbitration request.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent.

Return Value:

    n/a

--*/

{
    LARGE_INTEGER ArbitrationTimeout;
    LARGE_INTEGER SuspendTimeout;
    LONG RemainingMonTTLMilliseconds = 0;
    LONG RemainingSubTTLMilliseconds = 0;
    LARGE_INTEGER Now;
    LONG remoteArbitrationDurationUpperBound = 0;

    PVOID IterLeasingApplication = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER RemoteRelationshipIdentifier = NULL;
    LPWSTR RemoteLeasingApplicationIdentifier = NULL;

    //
    // Take the system time.
    //
    GetCurrentTime(&Now);

    ArbitrationTimeout = RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectFailTime;

    //
    // Check to see if arbitration result was delayed. The arbitration result callback could be blocked. We double check the time here.
    // In case the arbitration result was delayed, we fail the lease.
    //
    if (IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(Now, ArbitrationTimeout))
    {
        EventWriteArbitrationSendTimeout(NULL,
           RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
           RemoteLeaseAgentContext->Instance.QuadPart,
           RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
           RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
           GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
           GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
           );
    
        goto Done;
    }

    // Enqueue the Arbitration timeout timer
    ArmTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->RenewOrArbitrateTimer,
        ArbitrationTimeout,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->RenewOrArbitrateTimerDpc
        );

    SuspendTimeout.QuadPart =
        ((LONGLONG)RemoteLeaseAgentContext->LeaseRelationshipContext->LeaseSuspendDuration * MILLISECOND_TO_NANOSECOND_FACTOR);

    EventWriteLeaseRelationInState3(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        );
    //
    // The remote lease agent is now close to failing.
    // Lease establishment and termination cannot proceed anymore for a while.
    //
    if (OPEN == RemoteLeaseAgentContext->State) {

        //
        // The remote lease agent will temporarily become unavailable.
        //
        SetRemoteLeaseAgentState(RemoteLeaseAgentContext, SUSPENDED);
    }
    
    //
    // Retrieve the first arbitration enabled application.
    //
    IterLeasingApplication = RtlEnumerateGenericTable(
        &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable, 
        TRUE
        );
        
    while (NULL != IterLeasingApplication) {
             
        //
        // Retrieve leasing application identifier.
        //
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

        if (LeasingApplicationContext->IsArbitrationEnabled) {

            break; // found one
        }
            
        //
        // Move on the next element.
        //
        IterLeasingApplication = RtlEnumerateGenericTable(
            &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable, 
            FALSE
            );
        LeasingApplicationContext = NULL;
    }
    
    if (NULL == LeasingApplicationContext) {

        //
        // There is no application left to arbitrate.
        //
        EventWriteArbAbandonedLeaseRelationInState4(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
            NULL
            );

        goto Done;
    }

    //
    // Set remaining TTL in milliseconds.
    //
    RemainingMonTTLMilliseconds = (LONG)((RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorExpireTime.QuadPart
        - Now.QuadPart + SuspendTimeout.QuadPart) / MILLISECOND_TO_NANOSECOND_FACTOR);

    RemainingSubTTLMilliseconds = (LONG)((RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime.QuadPart
        - Now.QuadPart + SuspendTimeout.QuadPart) / MILLISECOND_TO_NANOSECOND_FACTOR);

    EventWriteLeaseAppChosenLeaseRelationInState(
        NULL,
        LeasingApplicationContext->LeasingApplicationIdentifier,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
        RemainingMonTTLMilliseconds,
        RemainingSubTTLMilliseconds
        );

    //
    // Save the application context for arbitration
    //
    RemoteLeaseAgentContext->LeasingApplicationForArbitration = LeasingApplicationContext;

    //
    // Get 1 remote lease application identifier from either subject or monitor table
    //
    for (IterLeasingApplication = RtlEnumerateGenericTable(&RemoteLeaseAgentContext->MonitorHashTable, TRUE);
        NULL != IterLeasingApplication && NULL == RemoteRelationshipIdentifier;
        IterLeasingApplication = RtlEnumerateGenericTable(&RemoteLeaseAgentContext->MonitorHashTable, FALSE))
    {
        RemoteRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterLeasingApplication);
    }

    for (IterLeasingApplication = RtlEnumerateGenericTable(&RemoteLeaseAgentContext->SubjectHashTable, TRUE);
        NULL != IterLeasingApplication && NULL == RemoteRelationshipIdentifier;
        IterLeasingApplication = RtlEnumerateGenericTable(&RemoteLeaseAgentContext->SubjectHashTable, FALSE))
    {
        RemoteRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterLeasingApplication);
    }

    if (NULL != RemoteRelationshipIdentifier)
    {
        RemoteLeasingApplicationIdentifier = RemoteRelationshipIdentifier->RemoteLeasingApplicationIdentifier;
    }

    if (RemoteLeaseAgentContext->RenewedBefore)
    {
        // t = subject expired time - TTL (lease duration)
        remoteArbitrationDurationUpperBound = (LONG) ((Now.QuadPart / MILLISECOND_TO_NANOSECOND_FACTOR) - (RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime.QuadPart / MILLISECOND_TO_NANOSECOND_FACTOR - GetConfigLeaseDuration(RemoteLeaseAgentContext)));
    }
    else
    {
        remoteArbitrationDurationUpperBound = (LONG) ((Now.QuadPart - RemoteLeaseAgentContext->PingSendTime.QuadPart) / MILLISECOND_TO_NANOSECOND_FACTOR);
    }

    EventWriteCreateRemoteLeaseAgentArbitration(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
        RemoteLeaseAgentContext->RenewedBefore,
        Now.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime.QuadPart,
        RemoteLeaseAgentContext->PingSendTime.QuadPart,
        remoteArbitrationDurationUpperBound
        );


    //
    // Surface the arbitration event.
    //
    LAssert(!IS_LARGE_INTEGER_EQUAL_TO_INTEGER(RemoteLeaseAgentContext->RemoteLeaseAgentInstance, 0));
    TryDelayedRequestCompletion(
        LeasingApplicationContext,
        LEASING_APPLICATION_ARBITRATE,
        NULL,
        LeasingApplicationContext->Instance,
        RemoteLeasingApplicationIdentifier,
        RemainingMonTTLMilliseconds,
        RemainingSubTTLMilliseconds,
        &RemoteLeaseAgentContext->RemoteSocketAddress,
        RemoteLeaseAgentContext->LeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart,
        STATUS_SUCCESS,
        TRUE,
        0,
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        RemoteLeaseAgentContext->RemoteVersion,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        remoteArbitrationDurationUpperBound
        );

    return;

Done:

    //
    // It is possible that there is no leasing application registered on this lease agent anymore.
    // We will treat this case as if the remote lease agent has gone into arbitration but has lost it. 
    // Fail the lease agent and all its remote lease agents.
    //
    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_FAILED;
    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_INACTIVE;
    SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);

    //
    // Signal lease failure.
    //
    OnLeaseFailure(RemoteLeaseAgentContext->LeaseAgentContext);
}

NTSTATUS
CreateRemoteCertVerifyRequest(
    __in PVOID Context,
    __in ULONG cbCertificate,
    __in_bcount(cbCertificate) PBYTE pbCertificate,
    __in PVOID remoteCertVerifyCompleteOp,
    __inout PBOOLEAN isTrusted
    )

/*++

Routine Description:

    Creates a remote certificate verification request.

Parameters Description:



Return Value:

    n/a

--*/

{
    KIRQL OldIrql;
    PLEASE_AGENT_CONTEXT LeaseAgentContext;
    PVOID IterLeasingApplication = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PLEASE_REMOTE_CERTIFICATE RemoteCertContext = NULL;
    PLEASE_REMOTE_CERTIFICATE* RemoteCertContextExistent = NULL;
    PVOID IterCertExistent = NULL;
    BOOLEAN Inserted = FALSE;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    RemoteCertContext = LeaseRemoteCertificateConstructor(
        cbCertificate, 
        pbCertificate,
        remoteCertVerifyCompleteOp,
        NULL);

    if (NULL == RemoteCertContext)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LeaseAgentContext = (PLEASE_AGENT_CONTEXT)Context;
    LAssert(LeaseAgentContext);

    KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

    //
    // First check if the certificate is in the trusted cert cache.
    //
    IterCertExistent = RtlLookupElementGenericTable(
        &LeaseAgentContext->TrustedCertificateCache,
        &RemoteCertContext
        );
        
    if (NULL != IterCertExistent)
    {
        //
        // A match is found, the certificate is trusted
        //
        EventWriteFoundCertInTrustedList(NULL, (LONGLONG)remoteCertVerifyCompleteOp, STATUS_SUCCESS);

        *isTrusted = TRUE;
        status = STATUS_SUCCESS;
        goto End;
    }

    //
    // Retrieve the first application for certificate verification.
    //
    IterLeasingApplication = RtlEnumerateGenericTable(
        &LeaseAgentContext->LeasingApplicationContextHashTable, 
        TRUE
        );

    if (IterLeasingApplication != NULL)
    {
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);
    }

    if (NULL == LeasingApplicationContext)
    {
        //
        // There is no application left to certificate verification.
        //
        EventWriteRemoteCertVerifyReqFailNoLeasingApp(
            NULL,
            TransportIdentifier(LeaseAgentContext->Transport),
            LeaseAgentContext->Instance.QuadPart,
            cbCertificate
            );

        goto End;
    }

    EventWriteRemoteCertVerifyReq(
        NULL,
        LeasingApplicationContext->LeasingApplicationIdentifier,
        cbCertificate,
        (LONGLONG)remoteCertVerifyCompleteOp
        );

    //
    // Surface the verify event.
    //

    TryDelayedRequestCompletion(
        LeasingApplicationContext,
        REMOTE_CERTIFICATE_VERIFY,
        NULL,
        LeasingApplicationContext->Instance,
        NULL,
        -1,
        -1,
        NULL,
        0,
        0,
        STATUS_SUCCESS,
        TRUE,
        cbCertificate,
        pbCertificate,
        remoteCertVerifyCompleteOp,
        0,
        NULL,
        NULL,
        0,
        0,
        0,
        0
        );

    //
    // The certificate is not in the trusted cert cache.
    // Add it to the pending verify list
    //

    RemoteCertContext->certLeasingAppHandle = LeasingApplicationContext;

    RemoteCertContextExistent = RtlInsertElementGenericTable(
        &LeaseAgentContext->CertificatePendingList,
        &RemoteCertContext,
        sizeof(PLEASE_REMOTE_CERTIFICATE),
        &Inserted
        );
        
    //
    // Check insert result.
    //
    if (NULL == RemoteCertContextExistent)
    {
        LAssert(!Inserted);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto End;
    }
    else if (*RemoteCertContextExistent != RemoteCertContext)
    {
        LAssert(!Inserted);

        EventWriteInsertCertInPendingListAlreadyExistent(
            NULL,
            LeasingApplicationContext->LeasingApplicationIdentifier,
            (LONGLONG)RemoteCertContext->verifyOperation
            );

        status = STATUS_INVALID_PARAMETER;
        goto End;
    }
    else
    {
        EventWriteInsertCertInPendingList(
            NULL,
            LeasingApplicationContext->LeasingApplicationIdentifier,
            (LONGLONG)RemoteCertContext->verifyOperation
            );

        LAssert(Inserted);
    }

    KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

    return STATUS_SUCCESS;

End:

    KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    //
    // Cleanup.
    //
    LeaseRemoteCertificateDestructor(RemoteCertContext);

    return status;

}

NTSTATUS GetListenerForKerbAuth(_In_ PVOID Context, _Out_ PVOID * LeaseAgentListener)
{
    PLEASE_AGENT_CONTEXT LeaseAgentContext;
    LeaseAgentContext = (PLEASE_AGENT_CONTEXT)Context;
    LAssert(LeaseAgentContext);

    *LeaseAgentListener = LeaseAgentContext->Transport;

    return STATUS_SUCCESS;

}

KDEFERRED_ROUTINE SubjectExpiredTimer;

VOID
SubjectExpiredTimer (
    __in PKDPC Dpc,
    __in_opt PVOID DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
    
/*++

Routine Description:

    Lease subject timer.

Parameters Description:

    Dpc - subject timer Dpc routine.

    DeferredContext - remote lease agent context.

    SystemArgument1 - n/a

    SystemArgument2 - n/a

Return Value:

    n/a

--*/

{
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = (PREMOTE_LEASE_AGENT_CONTEXT)DeferredContext;
    ONE_WAY_LEASE_STATE OldSubjectState = LEASE_STATE_INACTIVE;
    ONE_WAY_LEASE_STATE OldMonitorState = LEASE_STATE_INACTIVE;
    LARGE_INTEGER SubjectExpiration;
    LARGE_INTEGER Now;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (NULL == RemoteLeaseAgentContext)
    {
        return;
    }

    //
    // Take the system time.
    //
    GetCurrentTime(&Now);

    EventWriteSubjectExpiredOneway(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
        Now.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime.QuadPart
        );

    KeAcquireSpinLockAtDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);

    OldSubjectState = RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState;
    OldMonitorState = RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState;
        
    if (IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext)) {
    
         //
         // Nothing to do.
         //
         goto Done;
    }

    //
    // In ping scenario
    //
    if ( LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState )
    {
        //
        // The ping process did not succeed, fail silently and all timer will be dequeued
        //
        if (RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState == LEASE_STATE_ACTIVE)
        {
            //
            // Monitor lease has been successful
            //

            EventWriteSubExpiredTimerInWrongState(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
                );

            RemoteLeaseAgentContext->InPing = FALSE;

            //
            // Set the subject expire time back to default MaxLongLong.
            // The lease is not established, the GetTTL would report wrong value without this.
            //
            SubjectExpiration.QuadPart = MAXLONGLONG;

            SetSubjectExpireTime(
                RemoteLeaseAgentContext->LeaseRelationshipContext,
                SubjectExpiration,
                DURATION_MAX_VALUE,
                DURATION_MAX_VALUE
                );
        }
        else
        {
            SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);
        }

        goto Done;
    }

    //
    // If there's no monitor and we never sent any messages, fail silently
    //
    if ((RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState != LEASE_STATE_ACTIVE) &&
        (RemoteLeaseAgentContext->LeaseRelationshipContext->LeaseMessageSent != TRUE))
    {
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_INACTIVE;

        SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);
    }
    else
    {
        USHORT localLeaseAgentVersion = GetLeaseAgentVersion(LEASE_PROTOCOL_MAJOR_VERSION, LEASE_PROTOCOL_MINOR_VERSION);
        
        EventWriteRemoteLeaseAgentVersion(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            localLeaseAgentVersion,
            RemoteLeaseAgentContext->RemoteVersion
            );

        if (RemoteLeaseAgentContext->RemoteVersion == 257)
        {
            //
            // Old lease version cannot handle one way arbitration, fail it
            //
            OnSubjectExpired(RemoteLeaseAgentContext);
        }
        else
        {
            //
            // Monitor and subject have now expired.
            //
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_EXPIRED;
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_EXPIRED;

            //
            // Perform arbitration.
            //
            CreateArbitrationRequest(RemoteLeaseAgentContext);
        }
    }

Done:

    EventWriteLeaseRelationTransition1(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(OldSubjectState),
        GetLeaseState(OldMonitorState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        );

    KeReleaseSpinLockFromDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);

    //
    // Dereference remote lease agent.
    //
    Release(RemoteLeaseAgentContext);
}

KDEFERRED_ROUTINE MonitorExpiredTimer;

VOID
MonitorExpiredTimer(
    __in PKDPC Dpc,
    __in_opt PVOID DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
/*++

Routine Description:

    Lease monitor timer.

Parameters Description:

    Dpc - monitor timer Dpc routine.

    DeferredContext - remote lease agent context.

    SystemArgument1 - n/a

    SystemArgument2 - n/a

Return Value:

    n/a

--*/
{
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = (PREMOTE_LEASE_AGENT_CONTEXT)DeferredContext;
    ONE_WAY_LEASE_STATE OldSubjectState = LEASE_STATE_INACTIVE;
    ONE_WAY_LEASE_STATE OldMonitorState = LEASE_STATE_INACTIVE;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (NULL == RemoteLeaseAgentContext)
    {
        return;
    }

    KeAcquireSpinLockAtDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);

    OldSubjectState = RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState;
    OldMonitorState = RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState;

    if (LEASE_STATE_FAILED == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState ||
        LEASE_STATE_ACTIVE != RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState ||
        LEASE_STATE_EXPIRED <= RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState ||
        IsLeaseAgentFailed(RemoteLeaseAgentContext->LeaseAgentContext)) {

        //
        // Nothing to do.
        //
        goto Done;
    }

    if (LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState) {
        
        //
        // One way lease scenario. Monitor has now failed. We'll ignore this and
        // siliently remove it without notify others.
        //
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_INACTIVE;
        RemoteLeaseAgentUninitialize(RemoteLeaseAgentContext);

        goto Done;
    }

    //
    // Monitor and subject have now expired.
    //
    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_EXPIRED;
    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_EXPIRED;

    EventWriteMonitorExpired(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
        (LONGLONG)0, 
        (LONGLONG)0 // TODO seperate template for this trace
        );

    //
    // Cancel renew timer.
    //
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->RenewOrArbitrateTimer
        );

    //
    // Cancel prearbitration subject timer.
    //
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationSubjectTimer
        );

    //
    // Dequeue the retry timer.
    //
    DequeueTimer(RemoteLeaseAgentContext, &RemoteLeaseAgentContext->LeaseRelationshipContext->PingRetryTimer);

    //
    // Go to arbitration.
    //
    CreateArbitrationRequest(RemoteLeaseAgentContext);

Done:

    EventWriteLeaseRelationTransition2(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(OldSubjectState),
        GetLeaseState(OldMonitorState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        );

    KeReleaseSpinLockFromDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);

    //
    // Dereference remote lease agent.
    //
    Release(RemoteLeaseAgentContext);
}

KDEFERRED_ROUTINE RenewOrArbitrateTimer;
    
VOID
RenewOrArbitrateTimer(
    __in PKDPC Dpc,
    __in_opt PVOID DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
    
/*++

Routine Description:

    Lease relationship renew timer.

Parameters Description:

    Dpc - renew timer Dpc routine.

    DeferredContext - remote lease agent context.

    SystemArgument1 - n/a

    SystemArgument2 - n/a

Return Value:

    n/a

--*/

{
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = (PREMOTE_LEASE_AGENT_CONTEXT)DeferredContext;
    ONE_WAY_LEASE_STATE OldSubjectState = LEASE_STATE_INACTIVE;
    ONE_WAY_LEASE_STATE OldMonitorState = LEASE_STATE_INACTIVE;
    LARGE_INTEGER Now;
    PVOID LeaseMessage = NULL;
    ULONG LeaseMessageSize = 0;

    LARGE_INTEGER NewExpiration;
    LARGE_INTEGER TerminateExpiration;
    LARGE_INTEGER RenewTime;
    LARGE_INTEGER OutgoingMsgId;
    LARGE_INTEGER PreArbitrationSubjectTime;
    
    LONG RequestLeaseDuration;
    LONG RequestLeaseSuspendDuration;
    LONG RequestArbitrationDuration;

    BOOLEAN OldDurationUpdateState = FALSE;

    NTSTATUS status;

    TerminateExpiration.QuadPart = MAXLONGLONG;
    OutgoingMsgId.QuadPart = 0;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    KeAcquireSpinLockAtDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);

    OldSubjectState = RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState;
    OldMonitorState = RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState;

    //
    // Take the system time.
    //
    GetCurrentTime(&Now);

    if (IsLeaseAgentFailed(RemoteLeaseAgentContext->LeaseAgentContext))
    {
        if (IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(
                Now, 
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectFailTime
                )
            ) {
            
            //
            // Perform subject expiration.
            //
            OnSubjectExpired(RemoteLeaseAgentContext);
        }

        goto Done;
    }

    if (LEASE_STATE_ACTIVE != RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState ||
        LEASE_STATE_EXPIRED <= RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState) {

        //
        // If the arbitration result timeout, this callback will be called. We detect if we are in arbitration state.
        // We check if the time now is greater or equals to the subject fail time. This is to avoid the race that monitor expired timer
        // and renew arbitrate timer called at the same time, that caused the arbitration result timeout detection to fail.
        //
        if (RemoteLeaseAgentContext->State == SUSPENDED &&
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState == LEASE_STATE_EXPIRED &&
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState == LEASE_STATE_EXPIRED &&
            IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(Now, RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectFailTime))
        {
            EventWriteArbitrationResultTimeout(NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
                );

            //
            // Fail lease since the arbitration result timeout
            //
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_FAILED;
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_INACTIVE;

            OnLeaseFailure(RemoteLeaseAgentContext->LeaseAgentContext);
        }

        //
        // This timer fired too late.
        //
        goto Done;
    }

    //
    // Has lease expired?
    //
    if (IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(Now, RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime))
    {
        // The subject timer will take care of lease failure.
        //
        if (LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        {
            goto Done;
        }

        EventWriteSubjectExpired(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
            Now.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime.QuadPart
            );

        // Monitor and subject have now expired.
        //
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_EXPIRED;
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_EXPIRED;

        // Cancel the monitor timer.
        //
        DequeueTimer(
            RemoteLeaseAgentContext,
            &RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorTimer
            );

        // If there's no monitor and we never sent any messages, fail silently
        //
        if ((RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState != LEASE_STATE_ACTIVE) &&
            (RemoteLeaseAgentContext->LeaseRelationshipContext->LeaseMessageSent != TRUE))
        {
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_INACTIVE;

            SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);
        }
        else
        {
            // Go to arbitration.
            //
            CreateArbitrationRequest(RemoteLeaseAgentContext);
        }
    }
    else   // Regular renew logic
    {
        if (IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext))
        {
            // The remote lease agent should not send any more renew messages.
            goto Done;
        }

        if (RemoteLeaseAgentContext->LeaseRelationshipContext->IsRenewRetry == TRUE)
        {
            //
            // RenewRetry is true, initial the indirect lease logic
            //
            RemoteLeaseAgentContext->LeaseRelationshipContext->IndirectLeaseCount++;

            RenewTime = RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime;

            //
            // Fire the timer when the lease expires.
            //
            ArmTimer(
                RemoteLeaseAgentContext,
                &RemoteLeaseAgentContext->LeaseRelationshipContext->RenewOrArbitrateTimer,
                RenewTime,
                &RemoteLeaseAgentContext->LeaseRelationshipContext->RenewOrArbitrateTimerDpc
                );
            
            //
            // Enqueue the prearbitration subject timer.
            //
            LARGE_INTEGER_SUBTRACT_INTEGER_AND_ASSIGN(PreArbitrationSubjectTime, RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime, PRE_ARBITRATION_TIME);
            ArmTimer(
                RemoteLeaseAgentContext,
                &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationSubjectTimer,
                PreArbitrationSubjectTime,
                &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationSubjectTimerDpc
                );

            if (RemoteLeaseAgentContext->LeaseAgentContext->ConsecutiveIndirectLeaseLimit != 0 &&
                RemoteLeaseAgentContext->LeaseRelationshipContext->IndirectLeaseCount < RemoteLeaseAgentContext->LeaseAgentContext->ConsecutiveIndirectLeaseLimit)
            {
                //
                // The renew need to be retried, forward the request (regular/termination) to all neighbors
                //
                SendIndirectLeaseForwardMessages(RemoteLeaseAgentContext);
            }
            else
            {
                // Reach the limit, don't initiate the indirect lease process
                EventWriteIndirectLeaseReachLimit(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->IndirectLeaseCount,
                    RemoteLeaseAgentContext->LeaseAgentContext->ConsecutiveIndirectLeaseLimit,
                    RemoteLeaseAgentContext->LeaseAgentContext->LeaseConfigDurations.MaxIndirectLeaseTimeout,
                    RemoteLeaseAgentContext->LeaseAgentContext->LeaseConfigDurations.LeaseDuration
                    );

            }

            goto Done;
        }

        // Remember the old flag before it is changed in GetDurationsForRequest
        OldDurationUpdateState = RemoteLeaseAgentContext->LeaseRelationshipContext->IsDurationUpdated;

        GetDurationsForRequest(
            RemoteLeaseAgentContext,
            &RequestLeaseDuration,
            &RequestLeaseSuspendDuration,
            &RequestArbitrationDuration,
            FALSE);

        NewExpiration.QuadPart = Now.QuadPart + (LONGLONG)RequestLeaseDuration * MILLISECOND_TO_NANOSECOND_FACTOR;
        if (IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime, NewExpiration))
        {
            // Duration may be dynamically changed
            // If this happens, we should reset the renew timer without sending out the renew request
            // Since it does make sense to request a new expiration that is less than the current one

            EventWriteSubjectRequestedLeaseExpiration(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RequestLeaseDuration,
                Now.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime.QuadPart,
                NewExpiration.QuadPart,
                OldDurationUpdateState
                );

            SetRenewTimer(RemoteLeaseAgentContext, FALSE, RequestLeaseDuration);

            // Restore this flag since the request wasn't really sent out
            RemoteLeaseAgentContext->LeaseRelationshipContext->IsDurationUpdated = OldDurationUpdateState;
            goto Done;

        }

        //
        // Set the next timer for renew/expire.
        //
        SetRenewTimer(RemoteLeaseAgentContext, TRUE, RequestLeaseDuration);

        //
        // Create lease renew request message. Check to see if we need to send
        // a regular renew request or a lease termination request.
        //
        if (0 == RtlNumberGenericTableElements(&RemoteLeaseAgentContext->SubjectHashTable))
        {
            // Set the remote lease agent state to suspended.
            // When the termination ack comes back, then we can enable it again.
            //
            LAssert(OPEN == RemoteLeaseAgentContext->State || SUSPENDED == RemoteLeaseAgentContext->State);
            if (OPEN == RemoteLeaseAgentContext->State)
            {
                SetRemoteLeaseAgentState(RemoteLeaseAgentContext, SUSPENDED);
            }

            // Create lease renew terminate request message.
            //
            SerializeLeaseMessage(
                RemoteLeaseAgentContext,
                LEASE_REQUEST,
                &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
                &RemoteLeaseAgentContext->SubjectFailedPendingHashTable,
                &RemoteLeaseAgentContext->MonitorFailedPendingHashTable,
                NULL,
                NULL,
                NULL,
                NULL,
                &RemoteLeaseAgentContext->SubjectTerminatePendingHashTable,
                NULL,
                DURATION_MAX_VALUE,
                TerminateExpiration,
                DURATION_MAX_VALUE,
                DURATION_MAX_VALUE,
                FALSE,
                &LeaseMessage,
                &LeaseMessageSize
                );

            if (NULL != LeaseMessage) 
            {
                OutgoingMsgId.QuadPart = ((PLEASE_MESSAGE_HEADER) LeaseMessage)->MessageIdentifier.QuadPart;
            }
            EventWriteLeaseRelationSendingTermination(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                GetMessageType(LEASE_REQUEST),
                OutgoingMsgId.QuadPart
                );

        }
        else    // Create regular lease renew request message.
        {
 
            // Send out the request with the requested duration and expiration
            SerializeLeaseMessage(
                RemoteLeaseAgentContext,
                LEASE_REQUEST,
                &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
                &RemoteLeaseAgentContext->SubjectFailedPendingHashTable,
                &RemoteLeaseAgentContext->MonitorFailedPendingHashTable,
                NULL,
                NULL,
                NULL,
                NULL,
                &RemoteLeaseAgentContext->SubjectTerminatePendingHashTable,
                NULL,
                RequestLeaseDuration,
                NewExpiration,
                RequestLeaseSuspendDuration,
                RequestArbitrationDuration,
                FALSE,
                &LeaseMessage,
                &LeaseMessageSize
                );

            if (NULL != LeaseMessage) 
            {
                OutgoingMsgId.QuadPart = ((PLEASE_MESSAGE_HEADER) LeaseMessage)->MessageIdentifier.QuadPart;
            }

            EventWriteLeaseRelationSendingLeaseRenewal(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                GetMessageType(LEASE_REQUEST),
                OutgoingMsgId.QuadPart
                ); 
        }

        //
        // Send request if it was successfully created.
        //
        if (NULL != LeaseMessage) {

            AddRef(RemoteLeaseAgentContext);

            status = TransportSendBufferNotification(
                RemoteLeaseAgentContext->PartnerTarget,
                LeaseMessage,
                LeaseMessageSize,
                MarkLeaseRelationshipSent,
                RemoteLeaseAgentContext,
                FALSE);

            if (!NT_SUCCESS(status))
            {
                Release(RemoteLeaseAgentContext);
            }
        }
    }

Done:

    if (OldSubjectState != RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState ||
        OldMonitorState != RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
    {
        // Trace only if the state is changed
        //
        EventWriteLeaseRelationTransition3(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(OldSubjectState),
            GetLeaseState(OldMonitorState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
            ); 
    }

    KeReleaseSpinLockFromDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);

    //
    // Dereference remote lease agent.
    //
    Release(RemoteLeaseAgentContext);
}
KDEFERRED_ROUTINE PreArbitrationTimer;

VOID
PreArbitrationTimer(
__in PKDPC Dpc,
__in_opt PVOID DeferredContext,
__in_opt PVOID SystemArgument1,
__in_opt PVOID SystemArgument2
)

/*++

Routine Description:

    Lease relationship pre arbitration timer which fires before real arbitration

    request indicating that current lease agent is alive.

Parameters Description:

    Dpc - n/a

    DeferredContext - n/a

    SystemArgument1 - n/a

    SystemArgument2 - n/a

Return Value:

    n/a

--*/

{
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = (PREMOTE_LEASE_AGENT_CONTEXT)DeferredContext;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PVOID IterLeasingApplication = NULL;
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (NULL == RemoteLeaseAgentContext)
    {
        return;
    }

    KeAcquireSpinLockAtDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);

    IterLeasingApplication = RtlEnumerateGenericTable(
        &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable,
        TRUE
        );
    while (NULL != IterLeasingApplication) {

        //
        // Retrieve leasing application identifier.
        //
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

        if (LeasingApplicationContext->IsArbitrationEnabled) {
            break; // found one
        }

        //
        // Move on the next element.
        //
        IterLeasingApplication = RtlEnumerateGenericTable(
            &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable,
            FALSE
            );
        LeasingApplicationContext = NULL;
    }

    if (NULL != LeasingApplicationContext)
    {
        EventWriteFirePreArbitration(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
            );

        TryDelayedRequestCompletion(
            LeasingApplicationContext,
            LEASING_APPLICATION_ARBITRATE,
            NULL,
            LeasingApplicationContext->Instance,
            NULL,
            0,
            0,
            NULL,
            RemoteLeaseAgentContext->LeaseAgentContext->Instance.QuadPart,
            0,
            STATUS_SUCCESS,
            TRUE,
            0,
            NULL,
            NULL,
            0,
            NULL,
            NULL,
            RemoteLeaseAgentContext->RemoteVersion,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            0
            );
    }
   
    KeReleaseSpinLockFromDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);

    //
    // Dereference remote lease agent.
    //
    Release(RemoteLeaseAgentContext);

}

KDEFERRED_ROUTINE PostArbitrationTimer;
    
VOID
PostArbitrationTimer(
    __in PKDPC Dpc,
    __in_opt PVOID DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
    
/*++

Routine Description:

    Lease relationship renew timer.

Parameters Description:

    Dpc - renew timer Dpc routine.

    DeferredContext - remote lease agent context.

    SystemArgument1 - n/a

    SystemArgument2 - n/a

Return Value:

    n/a

--*/

{
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = (PREMOTE_LEASE_AGENT_CONTEXT)DeferredContext;
    ONE_WAY_LEASE_STATE OldSubjectState = LEASE_STATE_INACTIVE;
    ONE_WAY_LEASE_STATE OldMonitorState = LEASE_STATE_INACTIVE;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (NULL == RemoteLeaseAgentContext)
    {
        return;
    }

    KeAcquireSpinLockAtDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);

    OldSubjectState = RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState;
    OldMonitorState = RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState;

    //
    // Perform monitor expiration.
    //
    OnMonitorExpired(RemoteLeaseAgentContext);

    EventWriteLeaseRelationTransition4(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(OldSubjectState),
        GetLeaseState(OldMonitorState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        );

        KeReleaseSpinLockFromDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);

    //
    // Dereference remote lease agent.
    //
    Release(RemoteLeaseAgentContext);
}

KDEFERRED_ROUTINE PingRetryTimer;
VOID
PingRetryTimer(
__in PKDPC Dpc,
__in_opt PVOID DeferredContext,
__in_opt PVOID SystemArgument1,
__in_opt PVOID SystemArgument2
)
{
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = (PREMOTE_LEASE_AGENT_CONTEXT)DeferredContext;
    LARGE_INTEGER Now;
    LARGE_INTEGER NextPingRetryTime;
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (NULL == RemoteLeaseAgentContext)
    {
        return;
    }

    KeAcquireSpinLockAtDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);
    
    // If not in Ping process, abort this.
    if (FALSE == IsRemoteLeaseAgentInPing(RemoteLeaseAgentContext))
    {
        //
        // Attempt to dequeue the timer.
        //
        DequeueTimer(RemoteLeaseAgentContext, &RemoteLeaseAgentContext->LeaseRelationshipContext->PingRetryTimer);
    }
    else
    {
        //
        // Send Ping request and set the retry
        //
        SendPingRequestMessage(RemoteLeaseAgentContext);

        GetCurrentTime(&Now);
        LARGE_INTEGER_ADD_INTEGER_AND_ASSIGN(NextPingRetryTime, Now, PING_RETRY_INTERVAL);

        ArmTimer(
            RemoteLeaseAgentContext,
            &RemoteLeaseAgentContext->LeaseRelationshipContext->PingRetryTimer,
            NextPingRetryTime,
            &RemoteLeaseAgentContext->LeaseRelationshipContext->PingRetryTimerDpc
            );
    }

    KeReleaseSpinLockFromDpcLevel(&RemoteLeaseAgentContext->LeaseAgentContext->Lock);
    //
    // Dereference remote lease agent.
    //
    Release(RemoteLeaseAgentContext);
}

NTSTATUS
LeaseRelationshipConstructor(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __out PLEASE_RELATIONSHIP_CONTEXT * result
    )

/*++

Routine Description:

    Constructs a new lease context structure.

Arguments:

    RemoteLeaseAgentContext - remote lease agent containing this lease relationship

Return Value:

    NULL if remote lease context structure could not be allocated and/or initialized.

--*/

{
    PLEASE_RELATIONSHIP_CONTEXT LeaseRelationshipContext = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(LEASE_RELATIONSHIP_CONTEXT),
        LEASE_RELATHIONSHIP_CTXT_TAG
        );

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    *result = LeaseRelationshipContext;
    if (NULL == LeaseRelationshipContext) {

        EventWriteAllocateFail6(
            NULL
            );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    RtlZeroMemory(LeaseRelationshipContext, sizeof(LEASE_RELATIONSHIP_CONTEXT));

    //
    // Initialize subject information.
    //
    LeaseRelationshipContext->SubjectState = LEASE_STATE_INACTIVE;
    //
    // Initialize subject timer.
    //
    KeInitializeTimer(&LeaseRelationshipContext->SubjectTimer);
    //
    // Initialize the subject timer deferred procedure call.
    //
    KeInitializeDpc(
        &LeaseRelationshipContext->SubjectTimerDpc,
        SubjectExpiredTimer,
        RemoteLeaseAgentContext
        );
    //
    // Set subject times.
    //
    LeaseRelationshipContext->SubjectExpireTime.QuadPart = MAXLONGLONG;
    LeaseRelationshipContext->SubjectSuspendTime.QuadPart = MAXLONGLONG;
    LeaseRelationshipContext->SubjectFailTime.QuadPart = MAXLONGLONG;

    //
    // Initialize monitor information.
    //
    LeaseRelationshipContext->MonitorState = LEASE_STATE_INACTIVE;
    //
    // Initialize monitor timer.
    //
    KeInitializeTimer(&LeaseRelationshipContext->MonitorTimer);
    //
    // Initialize the monitor timer deferred procedure call.
    //
    KeInitializeDpc(
        &LeaseRelationshipContext->MonitorTimerDpc,
        MonitorExpiredTimer,
        RemoteLeaseAgentContext
        );
    //
    // Set monitor times.
    //
    LeaseRelationshipContext->MonitorExpireTime.QuadPart = MAXLONGLONG;

    //
    // Initialize common to both subject and monitor.
    //

    //
    // Initialize renew lease timer.
    //
    KeInitializeTimer(&LeaseRelationshipContext->RenewOrArbitrateTimer);
    //
    // Initialize the renew timer deferred procedure call.
    //
    KeInitializeDpc(
        &LeaseRelationshipContext->RenewOrArbitrateTimerDpc,
        RenewOrArbitrateTimer,
        RemoteLeaseAgentContext
        );

    //
    // Initialize the pre-arbitration subject timer.
    //
    KeInitializeTimer(&LeaseRelationshipContext->PreArbitrationSubjectTimer);
    //
    // Initialize the pre-arbitration subject timer deferred procedure call.
    //
    KeInitializeDpc(
        &LeaseRelationshipContext->PreArbitrationSubjectTimerDpc,
        PreArbitrationTimer,
        RemoteLeaseAgentContext
        );
        
    //
    // Initialize the pre-arbitration monitor timer.
    //
    KeInitializeTimer(&LeaseRelationshipContext->PreArbitrationMonitorTimer);
    //
    // Initialize the pre-arbitration monitor timer deferred procedure call.
    //
    KeInitializeDpc(
        &LeaseRelationshipContext->PreArbitrationMonitorTimerDpc,
        PreArbitrationTimer,
        RemoteLeaseAgentContext
        );
        
    //
    // Initialize arbitration timer.
    //
    KeInitializeTimer(&LeaseRelationshipContext->PostArbitrationTimer);
    //
    // Initialize the arbitration timer deferred procedure call.
    //
    KeInitializeDpc(
        &LeaseRelationshipContext->PostArbitrationTimerDpc,
        PostArbitrationTimer,
        RemoteLeaseAgentContext
        );

    //
    // Initialize arbitration timer.
    //
    KeInitializeTimer(&LeaseRelationshipContext->PingRetryTimer);
    //
    // Initialize the arbitration timer deferred procedure call.
    //
    KeInitializeDpc(
        &LeaseRelationshipContext->PingRetryTimerDpc,
        PingRetryTimer,
        RemoteLeaseAgentContext
        );

    //
    // Set invalid duration.
    //
    LeaseRelationshipContext->Duration = DURATION_MAX_VALUE;
    LeaseRelationshipContext->LeaseSuspendDuration = DURATION_MAX_VALUE;
    LeaseRelationshipContext->ArbitrationDuration = DURATION_MAX_VALUE;
    // By default, use regular duration index
    LeaseRelationshipContext->EstablishDurationType = LEASE_DURATION;

    // Durations requested by the lease partner; initialized to MAX
    LeaseRelationshipContext->RemoteLeaseDuration = DURATION_MAX_VALUE;
    LeaseRelationshipContext->RemoteSuspendDuration = DURATION_MAX_VALUE;
    LeaseRelationshipContext->RemoteArbitrationDuration = DURATION_MAX_VALUE;

    LeaseRelationshipContext->IsRenewRetry = FALSE;
    LeaseRelationshipContext->RenewRetryCount = 0;

    return STATUS_SUCCESS;
    
Error:

    return status;
}

VOID 
LeaseRelationshipDestructor(
    PLEASE_RELATIONSHIP_CONTEXT LeaseRelationshipContext
    )

/*++

Routine Description:

    Destroys an existent lease context structure. At this point 
    in time all the timers and dpcs associated with this lease
    context should have been terminated.

Arguments:

    LeaseContext - lease context to be deallocated

Return Value:

    n/a

--*/

{
    //
    // This cannot be called until the timers are absolutely terminated.
    //
    ExFreePool(LeaseRelationshipContext);
}

PLEASE_RELATIONSHIP_IDENTIFIER 
LeaseRelationshipIdentifierConstructor(
    __in LPCWSTR LeasingApplicationIdentifier,
    __in LPCWSTR RemoteLeasingApplicationIdentifier
    )

/*++

Routine Description:

    Constructs a new lease relationship identifier.

Arguments:

    LeasingApplicationIdentifier - leasing application identifier.
    
    RemoteLeasingApplicationIdentifier - remote leasing application identifier.

Return Value:

    NULL if lease relationship identifier could not be allocated and/or initialized.

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    size_t WcharLength = 0;
    size_t AppIdLength = 0;
    size_t AppIdSize = 0;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(LEASE_RELATIONSHIP_IDENTIFIER),
        LEASE_RELATHIONSHIP_ID_TAG
        );

    if (NULL == LeaseRelationshipIdentifier) {

        EventWriteAllocateFail27(
            NULL
            );
        goto Error;
    }

    RtlZeroMemory(LeaseRelationshipIdentifier, sizeof(LEASE_RELATIONSHIP_IDENTIFIER));

    //
    // Populate remote leasing application identifier.
    //
    Status = RtlStringCchLengthW(
        RemoteLeasingApplicationIdentifier,
        MAX_PATH + 1,
        &WcharLength
        );

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Str length check failed with ", Status);
        goto Error;
    }

    Status = RtlULongLongAdd(
        WcharLength,
        1,
        &AppIdLength);

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"AppIdSize RtlULongLongAdd failed in LeaseRelationshipIdentifierConstructor",
            Status);

        goto Error;
    }

    Status = RtlULongLongMult(
        AppIdLength,
        sizeof(WCHAR),
        &AppIdSize);

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"AppIdSize RtlULongLongMult failed in LeaseRelationshipIdentifierConstructor",
            Status);

        goto Error;
    }

    LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier = ExAllocatePoolWithTag(
        NonPagedPool,
        AppIdSize,
        LEASE_RELATHIONSHIP_ID_TAG
        );
    
    if (NULL == LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier) {

        EventWriteAllocateFail28(
            NULL
            );
        goto Error;
    }

    RtlZeroMemory(
        LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier, 
        (WcharLength + 1) * sizeof(WCHAR)
        );

    Status = RtlStringCchCopyW(
        LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
        WcharLength + 1,
        RemoteLeasingApplicationIdentifier
        );

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Str copy failed with ", Status);
        goto Error;
    }

    //
    // Used for serialization.
    //
    LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifierByteCount = 
        (ULONG)(WcharLength + 1) * sizeof(WCHAR);

    //
    // Populate lease relationship leasing application.
    //
    LeaseRelationshipIdentifier->LeasingApplicationContext = LeasingApplicationConstructor(
        LeasingApplicationIdentifier,
        NULL
        );
    
    if (NULL == LeaseRelationshipIdentifier->LeasingApplicationContext) {

        goto Error;
    }

    EventWriteLeaseRelationshipCreateTrace(
        NULL,
        LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
        LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
        (LONGLONG)LeaseRelationshipIdentifier
        );

    return LeaseRelationshipIdentifier;

Error:

    //
    // Perform cleanup if any.
    //
    if (NULL != LeaseRelationshipIdentifier)
    {
        if (NULL != LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier)
        {
            ExFreePool( LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier );
        }

        if (NULL != LeaseRelationshipIdentifier->LeasingApplicationContext)
        {
            LeasingApplicationDestructor(LeaseRelationshipIdentifier->LeasingApplicationContext);
        }

        ExFreePool(LeaseRelationshipIdentifier);
    }

    return NULL;
}

VOID 
LeaseRelationshipIdentifierDestructor(
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier
    )

/*++

Routine Description:

    Destroys an existent lease relationship identifier.

Arguments:

    LeaseRelationshipIdentifier - lease relationship identifier to be deallocated

Return Value:

    n/a

--*/

{
    EventWriteLeaseRelationshipIdentifierDelete(
        NULL,
        LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
        LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
        (LONGLONG)LeaseRelationshipIdentifier
        );

    LeasingApplicationDestructor(LeaseRelationshipIdentifier->LeasingApplicationContext);
    
    ExFreePool( LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier );
    
    ExFreePool(LeaseRelationshipIdentifier);
}

VOID
SwitchLeaseRelationshipLeasingApplicationIdentifiers(
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier
    )
    
/*++

Routine Description:

    Temporarily switches identifiers in the lease relationship.
    Use this method with caution and in pairs.

Arguments:

    LeaseRelationshipIdentifier - the lease relationship to modify.

Return Value:

    n/a

--*/

{
    LPWSTR LeasingApplicationIdentifier = NULL;
    
    LeasingApplicationIdentifier = LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier;
    LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier = LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier;
    LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier = LeasingApplicationIdentifier;
}

PLEASE_RELATIONSHIP_IDENTIFIER 
AddToLeaseRelationshipIdentifierList(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LPCWSTR RemoteLeasingApplicationIdentifier,
    __in PRTL_GENERIC_TABLE HashTable
    )

/*++

Routine Description:

    Adds a lease relationship identifier to one of the remote lease agent lists.

Arguments:

    LeasingApplicationContext - initiating leasing application.

    RemoteLeasingApplicationIdentifier - target remote leasing aplication.

    HashTable - hash table representing the list to add into.

Return Value:

    NULL if lease identifier structure could not be allocated and/or initialized.

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER* LeaseRelationshipIdentifierExistent = NULL;
    BOOLEAN Inserted = FALSE;
    
    //
    // Create lease relationship identifier.
    //
    LeaseRelationshipIdentifier = LeaseRelationshipIdentifierConstructor(
        LeasingApplicationContext->LeasingApplicationIdentifier,
        RemoteLeasingApplicationIdentifier
        );

    if (NULL == LeaseRelationshipIdentifier) {

        goto Error;
    }
        
    //
    // Populate list in the remote lease agent.
    //
    LeaseRelationshipIdentifierExistent = RtlInsertElementGenericTable(
        HashTable,
        &LeaseRelationshipIdentifier,
        sizeof(PLEASE_RELATIONSHIP_IDENTIFIER),
        &Inserted
        );

    if (NULL == LeaseRelationshipIdentifierExistent) {
    
        LAssert(!Inserted);
    
        //
        // Hash table memory allocation failed.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        
    } else if (*LeaseRelationshipIdentifierExistent != LeaseRelationshipIdentifier) {
    
        LAssert(!Inserted);
    
        //
        // Cannot have a duplicate lease relationship.
        //
        Status = STATUS_INVALID_PARAMETER;
        
    } else {
    
        LAssert(Inserted);
        
        //
        // Lease relationship identifier inserted successfully.
        //
        Status = STATUS_SUCCESS;
    }
    
    if (!NT_SUCCESS(Status)) {

        goto Error;
    }
    
    return LeaseRelationshipIdentifier;

Error:

    if (NULL != LeaseRelationshipIdentifier) {
        
        LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);
    }

    return NULL;
}

BOOLEAN
RemoveFromLeaseRelationshipIdentifierList(
    __in PRTL_GENERIC_TABLE HashTable,
    __in PLEASE_RELATIONSHIP_IDENTIFIER* LeaseRelationshipIdentifier
    )

/*++

Routine Description:

    Adds a lease relationship identifier to one of the remote lease agent lists.

Arguments:

    RemoteLeaseAgentContext - remote lease agent used to establish the lease with

    HashTable - hash table representing the list to add into

    LeaseRelationshipIdentifier - lease relationship identifier to remove

Return Value:

    TRUE if the lease relationship identifier was found and deallocated.

--*/

{
    BOOLEAN Deleted = FALSE;
        
    //
    // Remove from list in the remote lease agent.
    //
    Deleted = RtlDeleteElementGenericTable(
        HashTable,
        LeaseRelationshipIdentifier
        );

    //
    // Deallocate identifier.
    //
    LeaseRelationshipIdentifierDestructor(*LeaseRelationshipIdentifier);

    return Deleted;
}

ULONG
GetLeaseRelationshipIdentifierListSize(
    __in PRTL_GENERIC_TABLE HashTable
    )

/*++

Routine Description:

    Computes the size of the list of identifiers from the list.

Arguments:

    HashTable - list with identifiers.

Return Value:

    Size of list in bytes.
    
--*/

{
    PVOID Iter = NULL;
    ULONG Size = 0;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;

    Size += sizeof(ULONG); /* size required for the size of the list */
    Size += sizeof(ULONG); /* size required for the count of elements */

    if (NULL == HashTable) {

        return Size;
    }
    
    for (Iter = RtlEnumerateGenericTable(HashTable, TRUE);
         NULL != Iter;
         Iter = RtlEnumerateGenericTable(HashTable, FALSE)) {
             
        //
        // Retrieve lease relationship identifier.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)Iter);

        //
        // Byte count of the identifier.
        //
        Size += sizeof(ULONG);
        //
        // Size of the identifier.
        //
        Size += LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifierByteCount;

        //
        // Byte count of the identifier.
        //
        Size += sizeof(ULONG);
        //
        // Size of the identifier.
        //
        Size += LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifierByteCount;
    }

    return Size;
}

NTSTATUS
SerializeLeaseRelationshipIdentifierList(
    __in PRTL_GENERIC_TABLE HashTable,
    __in_bcount(BufferSize) PBYTE Buffer,
    __in ULONG ElementCount,
    __in ULONG BufferSize
    )

/*++

Routine Description:

    Serializes the list of identifiers from the list into a buffer.

Arguments:

    HashTable - list with identifiers.

    ElementCount -  count of elements to serialize.

    Buffer - holds the serialized list.

    BufferSize - complete buffer size for this list.

Return Value:

    n/a
    
--*/

{
    PVOID Iter = NULL;
    ULONG ByteCount = 0;
    PBYTE Destination = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;

    if (NULL == Buffer) {

        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // Initialize destination buffer.
    //
    Destination = Buffer;
    
    //
    // Check buffer size.
    //
    if (sizeof(ULONG) > BufferSize) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    if (memcpy_s(
        Destination,
        BufferSize,
        &BufferSize,
        sizeof(ULONG)
        ))
    {
        return STATUS_DATA_ERROR;
    }

    //
    // Move destination.
    //
    Destination += sizeof(ULONG);
    //
    // Adjust buffer size.
    //
    BufferSize -= sizeof(ULONG);
    
    //
    // Serialize the number of elements from the list.
    //
    if (NULL == HashTable || 0 == ElementCount) {
        
        //
        // Check buffer size.
        //
        if (sizeof(ULONG) > BufferSize) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (memcpy_s(
            Destination,
            BufferSize,
            &ElementCount,
            sizeof(ULONG)
            ))
        {
            return STATUS_DATA_ERROR;
        }

        //
        // Adjust buffer size.
        //
        BufferSize -= sizeof(ULONG);

        LAssert(0 == BufferSize);

        return STATUS_SUCCESS;
    }

    //
    // Retrieve element count in the list.
    //
    ElementCount = RtlNumberGenericTableElements(HashTable);
    
    //
    // Check buffer size.
    //
    if (sizeof(ULONG) > BufferSize) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (memcpy_s(
        Destination,
        BufferSize,
        &ElementCount,
        sizeof(ULONG)
        ))
    {
        return STATUS_DATA_ERROR;
    }

    //
    // Move destination.
    //
    Destination += sizeof(ULONG);
    //
    // Adjust buffer size.
    //
    BufferSize -= sizeof(ULONG);

    for (Iter = RtlEnumerateGenericTable(HashTable, TRUE);
         NULL != Iter;
         Iter = RtlEnumerateGenericTable(HashTable, FALSE)) {
             
        //
        // Retrieve lease relationship identifier.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)Iter);

        //
        // Serialize the byte count for the identifier.
        //
        ByteCount = LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifierByteCount;

        //
        // Check buffer size.
        //
        if (sizeof(ULONG) > BufferSize) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (memcpy_s(
            Destination,
            BufferSize,
            &ByteCount,
            sizeof(ULONG)
            ))
        {
            return STATUS_DATA_ERROR;
        }

        //
        // Move destination.
        //
        Destination += sizeof(ULONG);
        //
        // Adjust buffer size.
        //
        BufferSize -= sizeof(ULONG);

        //
        // Check buffer size.
        //
        if (ByteCount > BufferSize) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }
       
        if (memcpy_s(
            Destination,
            BufferSize,
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifierByteCount
            ))
        {
            return STATUS_DATA_ERROR;
        }

        //
        // Move destination.
        //
        Destination += ByteCount;
        //
        // Adjust buffer size.
        //
        BufferSize -= ByteCount;

        //
        // Serialize the byte count for the identifier.
        //
        ByteCount = LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifierByteCount;

        //
        // Check buffer size.
        //
        if (sizeof(ULONG) > BufferSize) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (memcpy_s(
            Destination,
            BufferSize,
            &ByteCount,
            sizeof(ULONG)
            ))
        {
            return STATUS_DATA_ERROR;
        }

        //
        // Move destination.
        //
        Destination += sizeof(ULONG);
        //
        // Adjust buffer size.
        //
        BufferSize -= sizeof(ULONG);

        //
        // Serialize the bytes for the identifier.
        //
        //
        // Check buffer size.
        //
        if (ByteCount > BufferSize) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (memcpy_s(
            Destination,
            BufferSize,
            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifierByteCount
            ))
        {
            return STATUS_DATA_ERROR;
        }

        //
        // Move destination.
        //
        Destination += ByteCount;
        //
        // Adjust buffer size.
        //
        BufferSize -= ByteCount;
    }

    LAssert(0 == BufferSize);

    return STATUS_SUCCESS;
}

NTSTATUS
SerializeMessageListenEndPoint(
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in_bcount(BufferSize) PBYTE StartBuffer,
    __in ULONG BufferSize
    )

/*++
Routine description
    
    - Serialize the Socket Address in the message
    - BufferSize is the total length including the resove type and port. 
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AddressSize = BufferSize - sizeof(ADDRESS_FAMILY) - sizeof(USHORT);

    if (memcpy_s(
        StartBuffer,
        BufferSize,
        RemoteSocketAddress->Address,
        AddressSize))
    {
        return STATUS_DATA_ERROR;
    }

    StartBuffer += AddressSize;
    BufferSize -= AddressSize;

    if (memcpy_s(
        StartBuffer,
        BufferSize,
        &RemoteSocketAddress->ResolveType,
        sizeof(ADDRESS_FAMILY)))
    {
        return STATUS_DATA_ERROR;
    }

    StartBuffer += sizeof(ADDRESS_FAMILY);
    BufferSize -= sizeof(ADDRESS_FAMILY);

    if (memcpy_s(
        StartBuffer,
        BufferSize,
        &RemoteSocketAddress->Port,
        sizeof(USHORT)))
    {
        return STATUS_DATA_ERROR;
    }

    return Status;
}

NTSTATUS
SerializeMessageExtension(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in_bcount(BufferSize) PBYTE StartBuffer,
    __in ULONG BufferSize
    )
/*++
    - Serialize the message extension
--*/
{

    ULONG MsgExtSize = sizeof(LEASE_MESSAGE_EXT);
    ULONG InstanceSize = sizeof(LARGE_INTEGER);

    if (NULL == StartBuffer)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (BufferSize < MsgExtSize)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (memcpy_s(
        StartBuffer,
        BufferSize,
        &RemoteLeaseAgentContext->RemoteLeaseAgentInstance,
        InstanceSize))
    {
        return STATUS_DATA_ERROR;
    }

    return STATUS_SUCCESS;

}

NTSTATUS
SerializeLeaseMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LEASE_MESSAGE_TYPE LeaseMessageType,
    __in_opt PRTL_GENERIC_TABLE SubjectEstablishPendingList,
    __in_opt PRTL_GENERIC_TABLE SubjectFailedPendingList,
    __in_opt PRTL_GENERIC_TABLE MonitorFailedPendingList,
    __in_opt PRTL_GENERIC_TABLE SubjectPendingAcceptedList,
    __in_opt PRTL_GENERIC_TABLE SubjectPendingRejectedList,
    __in_opt PRTL_GENERIC_TABLE SubjectFailedAcceptedList,
    __in_opt PRTL_GENERIC_TABLE MonitorFailedAcceptedList,
    __in_opt PRTL_GENERIC_TABLE SubjectTerminatePendingList,
    __in_opt PRTL_GENERIC_TABLE SubjectTerminateAcceptedList,
    __in LONG Duration,
    __in LARGE_INTEGER Expiration,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration,
    __in BOOLEAN IsTwoWayTermination,
    __in PVOID* Buffer,
    __in PULONG BufferSize
    )

/*++

Routine Description:

    Creates lease response message (header + body).

Arguments:

    RemoteLeaseAgentContext - remote lease agent context.

    LeaseMessageType - lease message type.

    SubjectEstablishPendingList - subject establish pending list.
    
    SubjectFailedPendingList - subject failed pending list.
    
    MonitorFailedPendingList - monitor failed dpending list.

    SubjectPendingAcceptedList - subject pending accepted list.

    SubjectPendingRejectedList - subject pending rejected list.

    SubjectFailedAcceptedList - subject accepted list.
    
    MonitorFailedAcceptedList - monitor accepted list.

    SubjectTerminatePendingList - subject terminate pending list.
    
    SubjectTerminateAcceptedList - subject terminate accepted list.

    Duration - requested lease TTL.
    
    Buffer - output buffer address.

    BufferSize - output buffer size address.

Return Value:

    STATUS_SUCCESS if lease message could be allocated and initialized.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLEASE_MESSAGE_HEADER LeaseMessageHeader = NULL;
    PVOID LeaseMessage = NULL;
    PBYTE StartBuffer = NULL;
    ULONG LeaseMessageSize = 0;
    ULONG LeaseMessageSubjectPendingListSize = 0;
    ULONG LeaseMessageSubjectPendingListCount = 0;
    ULONG LeaseMessageSubjectFailedPendingListSize = 0;
    ULONG LeaseMessageSubjectFailedPendingListCount = 0;
    ULONG LeaseMessageMonitorFailedPendingListSize = 0;
    ULONG LeaseMessageMonitorFailedPendingListCount = 0;
    ULONG LeaseMessageSubjectPendingAcceptedListSize = 0;
    ULONG LeaseMessageSubjectPendingAcceptedListCount = 0;
    ULONG LeaseMessageSubjectFailedAcceptedListSize = 0;
    ULONG LeaseMessageSubjectFailedAcceptedListCount = 0;
    ULONG LeaseMessageMonitorFailedAcceptedListSize = 0;
    ULONG LeaseMessageMonitorFailedAcceptedListCount = 0;
    ULONG LeaseMessageSubjectPendingRejectedListSize = 0;
    ULONG LeaseMessageSubjectPendingRejectedListCount = 0;
    ULONG LeaseMessageSubjectTerminatePendingListSize = 0;
    ULONG LeaseMessageSubjectTerminatePendingListCount = 0;
    ULONG LeaseMessageSubjectTerminateAcceptedListSize = 0;
    ULONG LeaseMessageSubjectTerminateAcceptedListCount = 0;

    // Listen end point size in message body
    ULONGLONG LeaseMessageListenEndpointAddressSize = 0;
    ULONG LeaseMessageListenEndpointSize = 0;

    // Remote Listen end point size in message body
    // This is for FORWARD_REQUEST/RESPONSE
    ULONGLONG RemoteListenEndpointAddressSize = 0;
    ULONG RemoteListenEndpointSize = 0;

    ULONG MsgHeaderSize = 0;
    ULONG MsgExtSize = 0;

    // Relay messages use a different serialization function
    LAssert(LeaseMessageType != RELAY_REQUEST && LeaseMessageType != RELAY_RESPONSE);

    if (IsIndirectLeaseMsg(LeaseMessageType))
    {
        //For indirect lease forward messages, lease source address is added to header and body
        //
        MsgHeaderSize = sizeof(LEASE_MESSAGE_HEADER);
    }
    else
    {
        // Set the header size for regular lease message to be the same as V1 for backward compatibility
        // The header in V2 add RemoteListenEndPoint for indirect lease logic
        //
        MsgHeaderSize = sizeof(LEASE_MESSAGE_HEADER_V1);
    }

    //
    // Populate output arguments.
    //
    *Buffer = NULL;
    *BufferSize = 0;

    //
    // Compute number of elements in each list of interest.
    //
    LeaseMessageSubjectPendingListCount = 
        (NULL == SubjectEstablishPendingList) ? 0 : RtlNumberGenericTableElements(SubjectEstablishPendingList);
    LeaseMessageSubjectFailedPendingListCount = 
        (NULL == SubjectFailedPendingList) ? 0 : RtlNumberGenericTableElements(SubjectFailedPendingList);
    LeaseMessageMonitorFailedPendingListCount = 
        (NULL == MonitorFailedPendingList) ? 0 : RtlNumberGenericTableElements(MonitorFailedPendingList);
    LeaseMessageSubjectPendingAcceptedListCount = 
        (NULL == SubjectPendingAcceptedList) ? 0 : RtlNumberGenericTableElements(SubjectPendingAcceptedList);
    LeaseMessageSubjectFailedAcceptedListCount = 
        (NULL == SubjectFailedAcceptedList) ? 0 : RtlNumberGenericTableElements(SubjectFailedAcceptedList);
    LeaseMessageMonitorFailedAcceptedListCount = 
        (NULL == MonitorFailedAcceptedList) ? 0 : RtlNumberGenericTableElements(MonitorFailedAcceptedList);
    LeaseMessageSubjectPendingRejectedListCount = 
        (NULL == SubjectPendingRejectedList) ? 0 : RtlNumberGenericTableElements(SubjectPendingRejectedList);
    LeaseMessageSubjectTerminatePendingListCount = 
        (NULL == SubjectTerminatePendingList) ? 0 : RtlNumberGenericTableElements(SubjectTerminatePendingList);
    LeaseMessageSubjectTerminateAcceptedListCount = 
        (NULL == SubjectTerminateAcceptedList) ? 0 : RtlNumberGenericTableElements(SubjectTerminateAcceptedList);

    //
    // Compute message size from all the lists that need to be sent.
    //
    LeaseMessageSize = MsgHeaderSize;

    LeaseMessageSubjectPendingListSize = 
        GetLeaseRelationshipIdentifierListSize(SubjectEstablishPendingList);
    LeaseMessageSize += LeaseMessageSubjectPendingListSize;

    LeaseMessageSubjectFailedPendingListSize = 
        GetLeaseRelationshipIdentifierListSize(SubjectFailedPendingList);
    LeaseMessageSize += LeaseMessageSubjectFailedPendingListSize;

    LeaseMessageMonitorFailedPendingListSize = 
        GetLeaseRelationshipIdentifierListSize(MonitorFailedPendingList);
    LeaseMessageSize += LeaseMessageMonitorFailedPendingListSize;

    LeaseMessageSubjectPendingAcceptedListSize = 
        GetLeaseRelationshipIdentifierListSize(SubjectPendingAcceptedList);
    LeaseMessageSize += LeaseMessageSubjectPendingAcceptedListSize;

    LeaseMessageSubjectFailedAcceptedListSize = 
        GetLeaseRelationshipIdentifierListSize(SubjectFailedAcceptedList);
    LeaseMessageSize += LeaseMessageSubjectFailedAcceptedListSize;

    LeaseMessageMonitorFailedAcceptedListSize = 
        GetLeaseRelationshipIdentifierListSize(MonitorFailedAcceptedList);
    LeaseMessageSize += LeaseMessageMonitorFailedAcceptedListSize;

    LeaseMessageSubjectPendingRejectedListSize = 
        GetLeaseRelationshipIdentifierListSize(SubjectPendingRejectedList);
    LeaseMessageSize += LeaseMessageSubjectPendingRejectedListSize;

    LeaseMessageSubjectTerminatePendingListSize = 
        GetLeaseRelationshipIdentifierListSize(SubjectTerminatePendingList);
    LeaseMessageSize += LeaseMessageSubjectTerminatePendingListSize;

    LeaseMessageSubjectTerminateAcceptedListSize = 
        GetLeaseRelationshipIdentifierListSize(SubjectTerminateAcceptedList);
    LeaseMessageSize += LeaseMessageSubjectTerminateAcceptedListSize;

    GetListenEndPointSize(
        &RemoteLeaseAgentContext->LeaseAgentContext->ListenAddress,
        &LeaseMessageListenEndpointAddressSize,
        &LeaseMessageListenEndpointSize);

    LeaseMessageSize += LeaseMessageListenEndpointSize;

    if (FORWARD_REQUEST == LeaseMessageType || FORWARD_RESPONSE == LeaseMessageType)
    {
        GetListenEndPointSize(
            &RemoteLeaseAgentContext->RemoteSocketAddress,
            &RemoteListenEndpointAddressSize,
            &RemoteListenEndpointSize);

        LeaseMessageSize += RemoteListenEndpointSize;
    }

    MsgExtSize = sizeof(LEASE_MESSAGE_EXT);

    Status = RtlULongAdd(
        LeaseMessageSize,
        MsgExtSize,
        &LeaseMessageSize);

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"SerializeLeaseMessage failed in RtlULongAdd ",
            Status);

        return Status;
    }

    //
    // Allocate message buffer.
    //
    LeaseMessage = ExAllocatePoolWithTag(
        NonPagedPool,
        LeaseMessageSize,
        LEASE_MESSAGE_TAG
        );

    if (NULL == LeaseMessage)
    {
        EventWriteAllocateFail25( NULL );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(LeaseMessage, LeaseMessageSize);

    //
    // Start with the header.
    //
    LeaseMessageHeader = (PLEASE_MESSAGE_HEADER)LeaseMessage;

    //
    // Populate header information.
    //
    LeaseMessageHeader->MajorVersion = LEASE_PROTOCOL_MAJOR_VERSION;
    LeaseMessageHeader->MinorVersion = LEASE_PROTOCOL_MINOR_VERSION;
    LeaseMessageHeader->MessageHeaderSize = MsgHeaderSize;
    LeaseMessageHeader->MessageSize = LeaseMessageSize;

    if (IsSendingRequest(LeaseMessageType))
    {
        LeaseMessageHeader->LeaseInstance = RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier;
    }
    else if (IsSendingResponse(LeaseMessageType))
    {
        LeaseMessageHeader->LeaseInstance = RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier;
    }
    else
    {
        LeaseMessageHeader->LeaseInstance.QuadPart = 0;
    }

    LeaseMessageHeader->RemoteLeaseAgentInstance = RemoteLeaseAgentContext->LeaseAgentContext->Instance;
    LeaseMessageHeader->Duration = Duration;
    LeaseMessageHeader->Expiration = Expiration;
    LeaseMessageHeader->LeaseSuspendDuration = LeaseSuspendDuration;
    LeaseMessageHeader->ArbitrationDuration = ArbitrationDuration;
    LeaseMessageHeader->IsTwoWayTermination = IsTwoWayTermination;
    LeaseMessageHeader->MessageType = LeaseMessageType;
    LeaseMessageHeader->MessageIdentifier = GetInstance();

    //
    // Set sizes/counts for the variable data segment in the message body.
    //
    LeaseMessageHeader->SubjectPendingListDescriptor.Size = LeaseMessageSubjectPendingListSize;
    LeaseMessageHeader->SubjectFailedPendingListDescriptor.Size = LeaseMessageSubjectFailedPendingListSize;
    LeaseMessageHeader->MonitorFailedPendingListDescriptor.Size = LeaseMessageMonitorFailedPendingListSize;
    LeaseMessageHeader->SubjectPendingAcceptedListDescriptor.Size = LeaseMessageSubjectPendingAcceptedListSize;
    LeaseMessageHeader->SubjectFailedAcceptedListDescriptor.Size = LeaseMessageSubjectFailedAcceptedListSize;
    LeaseMessageHeader->MonitorFailedAcceptedListDescriptor.Size = LeaseMessageMonitorFailedAcceptedListSize;
    LeaseMessageHeader->SubjectPendingRejectedListDescriptor.Size = LeaseMessageSubjectPendingRejectedListSize;
    LeaseMessageHeader->SubjectTerminatePendingListDescriptor.Size = LeaseMessageSubjectTerminatePendingListSize;
    LeaseMessageHeader->SubjectTerminateAcceptedListDescriptor.Size = LeaseMessageSubjectTerminateAcceptedListSize;

    LeaseMessageHeader->SubjectPendingListDescriptor.Count = LeaseMessageSubjectPendingListCount;
    LeaseMessageHeader->SubjectFailedPendingListDescriptor.Count = LeaseMessageSubjectFailedPendingListCount;
    LeaseMessageHeader->MonitorFailedPendingListDescriptor.Count = LeaseMessageMonitorFailedPendingListCount;
    LeaseMessageHeader->SubjectPendingAcceptedListDescriptor.Count = LeaseMessageSubjectPendingAcceptedListCount;
    LeaseMessageHeader->SubjectFailedAcceptedListDescriptor.Count = LeaseMessageSubjectFailedAcceptedListCount;
    LeaseMessageHeader->MonitorFailedAcceptedListDescriptor.Count = LeaseMessageMonitorFailedAcceptedListCount;
    LeaseMessageHeader->SubjectPendingRejectedListDescriptor.Count = LeaseMessageSubjectPendingRejectedListCount;
    LeaseMessageHeader->SubjectTerminatePendingListDescriptor.Count = LeaseMessageSubjectTerminatePendingListCount;
    LeaseMessageHeader->SubjectTerminateAcceptedListDescriptor.Count = LeaseMessageSubjectTerminateAcceptedListCount;

    LeaseMessageHeader->MessageListenEndPointDescriptor.Size = LeaseMessageListenEndpointSize;

    if (FORWARD_REQUEST == LeaseMessageType || FORWARD_RESPONSE == LeaseMessageType)
    {
        LeaseMessageHeader->LeaseListenEndPointDescriptor.Size = RemoteListenEndpointSize;
    }

    //
    // Set offsets for the lists.
    //

    // Set the message body begin offset by the MsgHeaderSize variable
    // , which varies if the message is for indirect lease
    //
    LeaseMessageHeader->SubjectPendingListDescriptor.StartOffset = MsgHeaderSize;

    LeaseMessageHeader->SubjectFailedPendingListDescriptor.StartOffset = 
        LeaseMessageHeader->SubjectPendingListDescriptor.StartOffset +
        LeaseMessageSubjectPendingListSize;
    LeaseMessageHeader->MonitorFailedPendingListDescriptor.StartOffset = 
        LeaseMessageHeader->SubjectFailedPendingListDescriptor.StartOffset +
        LeaseMessageSubjectFailedPendingListSize;
    LeaseMessageHeader->SubjectPendingAcceptedListDescriptor.StartOffset = 
        LeaseMessageHeader->MonitorFailedPendingListDescriptor.StartOffset +
        LeaseMessageMonitorFailedPendingListSize;
    LeaseMessageHeader->SubjectFailedAcceptedListDescriptor.StartOffset = 
        LeaseMessageHeader->SubjectPendingAcceptedListDescriptor.StartOffset +
        LeaseMessageSubjectPendingAcceptedListSize;
    LeaseMessageHeader->MonitorFailedAcceptedListDescriptor.StartOffset = 
        LeaseMessageHeader->SubjectFailedAcceptedListDescriptor.StartOffset +
        LeaseMessageSubjectFailedAcceptedListSize;
    LeaseMessageHeader->SubjectPendingRejectedListDescriptor.StartOffset = 
        LeaseMessageHeader->MonitorFailedAcceptedListDescriptor.StartOffset +
        LeaseMessageMonitorFailedAcceptedListSize;
    LeaseMessageHeader->SubjectTerminatePendingListDescriptor.StartOffset = 
        LeaseMessageHeader->SubjectPendingRejectedListDescriptor.StartOffset +
        LeaseMessageSubjectPendingRejectedListSize;
    LeaseMessageHeader->SubjectTerminateAcceptedListDescriptor.StartOffset = 
        LeaseMessageHeader->SubjectTerminatePendingListDescriptor.StartOffset +
        LeaseMessageSubjectTerminatePendingListSize;

    LeaseMessageHeader->MessageListenEndPointDescriptor.StartOffset =
        LeaseMessageHeader->SubjectTerminateAcceptedListDescriptor.StartOffset +
        LeaseMessageSubjectTerminateAcceptedListSize;

    if (FORWARD_REQUEST == LeaseMessageType || FORWARD_RESPONSE == LeaseMessageType)
    {
        LeaseMessageHeader->LeaseListenEndPointDescriptor.StartOffset = 
            LeaseMessageHeader->MessageListenEndPointDescriptor.StartOffset +
            LeaseMessageHeader->MessageListenEndPointDescriptor.Size;
    }

    //
    // Serialize lists.
    //
    StartBuffer = (PBYTE)LeaseMessage + MsgHeaderSize;

    //
    // Subject pending list.
    //
    Status = SerializeLeaseRelationshipIdentifierList(
        SubjectEstablishPendingList,
        StartBuffer,
        LeaseMessageSubjectPendingListCount,
        LeaseMessageSubjectPendingListSize
        );
    if (!NT_SUCCESS(Status)) goto Error;
    StartBuffer += LeaseMessageSubjectPendingListSize;

    //
    // Subject failed pending list.
    //
    Status = SerializeLeaseRelationshipIdentifierList(
        SubjectFailedPendingList,
        StartBuffer,
        LeaseMessageSubjectFailedPendingListCount,
        LeaseMessageSubjectFailedPendingListSize
        );
    if (!NT_SUCCESS(Status)) goto Error;
    StartBuffer += LeaseMessageSubjectFailedPendingListSize;

    //
    // Monitor failed pending list.
    //
    Status = SerializeLeaseRelationshipIdentifierList(
        MonitorFailedPendingList,
        StartBuffer,
        LeaseMessageMonitorFailedPendingListCount,
        LeaseMessageMonitorFailedPendingListSize
        );
    if (!NT_SUCCESS(Status)) goto Error;
    StartBuffer += LeaseMessageMonitorFailedPendingListSize;

    //
    // Subject pending accepted list.
    //
    Status = SerializeLeaseRelationshipIdentifierList(
        SubjectPendingAcceptedList,
        StartBuffer,
        LeaseMessageSubjectPendingAcceptedListCount,
        LeaseMessageSubjectPendingAcceptedListSize
        );
    if (!NT_SUCCESS(Status)) goto Error;
    StartBuffer += LeaseMessageSubjectPendingAcceptedListSize;

    //
    // Subject failed accepted list.
    //
    Status = SerializeLeaseRelationshipIdentifierList(
        SubjectFailedAcceptedList,
        StartBuffer,
        LeaseMessageSubjectFailedAcceptedListCount,
        LeaseMessageSubjectFailedAcceptedListSize
        );
    if (!NT_SUCCESS(Status)) goto Error;
    StartBuffer += LeaseMessageSubjectFailedAcceptedListSize;

    //
    // Monitor failed accepted list.
    //
    Status = SerializeLeaseRelationshipIdentifierList(
        MonitorFailedAcceptedList,
        StartBuffer,
        LeaseMessageMonitorFailedAcceptedListCount,
        LeaseMessageMonitorFailedAcceptedListSize
        );
    if (!NT_SUCCESS(Status)) goto Error;
    StartBuffer += LeaseMessageMonitorFailedAcceptedListSize;

    //
    // Subject pending rejected list.
    //
    Status = SerializeLeaseRelationshipIdentifierList(
        SubjectPendingRejectedList,
        StartBuffer,
        LeaseMessageSubjectPendingRejectedListCount,
        LeaseMessageSubjectPendingRejectedListSize
        );
    if (!NT_SUCCESS(Status)) goto Error;
    StartBuffer += LeaseMessageSubjectPendingRejectedListSize;
    
    //
    // Subject terminate pending list.
    //
    Status = SerializeLeaseRelationshipIdentifierList(
        SubjectTerminatePendingList,
        StartBuffer,
        LeaseMessageSubjectTerminatePendingListCount,
        LeaseMessageSubjectTerminatePendingListSize
        );
    if (!NT_SUCCESS(Status)) goto Error;
    StartBuffer += LeaseMessageSubjectTerminatePendingListSize;

    //
    // Subject terminate accepted list.
    //
    Status = SerializeLeaseRelationshipIdentifierList(
        SubjectTerminateAcceptedList,
        StartBuffer,
        LeaseMessageSubjectTerminateAcceptedListCount,
        LeaseMessageSubjectTerminateAcceptedListSize
        );
    if (!NT_SUCCESS(Status)) goto Error;
    StartBuffer += LeaseMessageSubjectTerminateAcceptedListSize;

    Status = SerializeMessageListenEndPoint(
        &RemoteLeaseAgentContext->LeaseAgentContext->ListenAddress,
        StartBuffer,
        LeaseMessageListenEndpointSize
        );
    if (!NT_SUCCESS(Status)) goto Error;
    StartBuffer += LeaseMessageListenEndpointSize;

    if (FORWARD_REQUEST == LeaseMessageType || FORWARD_RESPONSE == LeaseMessageType)
    {
        Status = SerializeMessageListenEndPoint(
            &RemoteLeaseAgentContext->RemoteSocketAddress,
            StartBuffer,
            RemoteListenEndpointSize
            );
        if (!NT_SUCCESS(Status)) goto Error;
        StartBuffer += RemoteListenEndpointSize;
    }

    Status = SerializeMessageExtension(
        RemoteLeaseAgentContext,
        StartBuffer,
        MsgExtSize
        );
    if (!NT_SUCCESS(Status)) goto Error;

    //
    // Populate output arguments.
    //
    *Buffer = LeaseMessage;
    *BufferSize = LeaseMessageSize;
    
    return STATUS_SUCCESS;

Error:

    //
    // Deallocate lease message. There was an error serializing the message.
    //
    ExFreePool(LeaseMessage);
    
    return Status;
}

NTSTATUS IsValidBodyDescriptor(
    __in PLEASE_MESSAGE_BODY_LIST_DESCRIPTOR Descriptor,
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader)
{
    // Descriptor does not overlap header
    if (Descriptor->StartOffset < LeaseMessageHeader->MessageHeaderSize) 
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Descriptor starts inside message body, and leaves room for its length
    if (Descriptor->StartOffset >= LeaseMessageHeader->MessageSize) 
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Descriptor ends inside message body
    if (Descriptor->StartOffset + Descriptor->Size > LeaseMessageHeader->MessageSize) 
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Descriptor fits inside message body (redundant?)
    if (Descriptor->Size >= LeaseMessageHeader->MessageSize - LeaseMessageHeader->MessageHeaderSize) 
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IsValidMessageHeader(
    __in PVOID Buffer,
    __in ULONG BufferSize
    )

/*

Routine Description:

    Validates the message header.

Arguments:

    Buffer - the buffer received.

    BufferSize - buffer size received.

Return Value:

   STATUS_SUCCESS if the message header is valid.
   
*/

{
    PLEASE_MESSAGE_HEADER LeaseMessageHeader = (PLEASE_MESSAGE_HEADER)Buffer;
    NTSTATUS Status;

    //
    // Check size.
    //
    if ((sizeof(LEASE_MESSAGE_HEADER) > BufferSize) ||
        (LeaseMessageHeader->MessageSize > BufferSize) ||
        (0 >= LeaseMessageHeader->MessageHeaderSize) ||
        (0 >= LeaseMessageHeader->MessageSize) ||
        (LeaseMessageHeader->MessageHeaderSize >= LeaseMessageHeader->MessageSize))
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"buffer size"
            );

        return STATUS_INVALID_PARAMETER;
    }

    if (LeaseMessageHeader->MessageType != LEASE_REQUEST &&
        LeaseMessageHeader->MessageType != LEASE_RESPONSE &&
        LeaseMessageHeader->MessageType != PING_REQUEST &&
        LeaseMessageHeader->MessageType != PING_RESPONSE &&
        LeaseMessageHeader->MessageType != FORWARD_REQUEST &&
        LeaseMessageHeader->MessageType != FORWARD_RESPONSE &&
        LeaseMessageHeader->MessageType != RELAY_REQUEST &&
        LeaseMessageHeader->MessageType != RELAY_RESPONSE
        ) 
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"message type"
            );

        return STATUS_INVALID_PARAMETER;
    }

    if (!IsValidDuration(LeaseMessageHeader->Duration) && 
        LeaseMessageHeader->MessageType != PING_REQUEST &&
        LeaseMessageHeader->MessageType != PING_RESPONSE ) 
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"duration"
            );

        return STATUS_INVALID_PARAMETER;
    }

    if (IS_LARGE_INTEGER_LESS_THAN_OR_EQUAL_INTEGER(LeaseMessageHeader->LeaseInstance, 0) &&
        LeaseMessageHeader->MessageType != PING_REQUEST &&
        LeaseMessageHeader->MessageType != PING_RESPONSE ) 
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"lease instance"
            );

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check body descriptors.
    //
    Status = IsValidBodyDescriptor(&LeaseMessageHeader->SubjectPendingListDescriptor, LeaseMessageHeader);
    if (!NT_SUCCESS(Status))
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"SubjectPendingList"
            );

        return Status;
    }

    Status = IsValidBodyDescriptor(&LeaseMessageHeader->SubjectFailedPendingListDescriptor, LeaseMessageHeader);
    if (!NT_SUCCESS(Status)) 
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"SubjectFailedPendingList"
            );

        return Status;
    }

    Status = IsValidBodyDescriptor(&LeaseMessageHeader->MonitorFailedPendingListDescriptor, LeaseMessageHeader);
    if (!NT_SUCCESS(Status)) 
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"MonitorFailedPendingList"
            );

        return Status;
    }

    Status = IsValidBodyDescriptor(&LeaseMessageHeader->SubjectPendingAcceptedListDescriptor, LeaseMessageHeader);
    if (!NT_SUCCESS(Status)) 
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"SubjectPendingAcceptedList"
            );

        return Status;
    }

    Status = IsValidBodyDescriptor(&LeaseMessageHeader->SubjectFailedAcceptedListDescriptor, LeaseMessageHeader);
    if (!NT_SUCCESS(Status)) 
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"SubjectFailedAcceptedList"
            );

        return Status;
    }

    Status = IsValidBodyDescriptor(&LeaseMessageHeader->MonitorFailedAcceptedListDescriptor, LeaseMessageHeader);
    if (!NT_SUCCESS(Status)) 
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"MonitorFailedAcceptedList"
            );

        return Status;
    }

    Status = IsValidBodyDescriptor(&LeaseMessageHeader->SubjectPendingRejectedListDescriptor, LeaseMessageHeader);
    if (!NT_SUCCESS(Status)) 
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"SubjectPendingRejectedList"
            );

        return Status;
    }

    Status = IsValidBodyDescriptor(&LeaseMessageHeader->SubjectTerminateAcceptedListDescriptor, LeaseMessageHeader);
    if (!NT_SUCCESS(Status)) 
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"SubjectTerminatedAcceptedList"
            );

        return Status;
    }

    Status = IsValidBodyDescriptor(&LeaseMessageHeader->SubjectTerminatePendingListDescriptor, LeaseMessageHeader);
    if (!NT_SUCCESS(Status)) 
    {
        EventWriteInvalidMessageHeader(
            NULL,
            L"SubjectTerminatePendingList"
            );

        return Status;
    }

    return STATUS_SUCCESS;
}

VOID
ClearRelationshipIdentifierList(
    __in PRTL_GENERIC_TABLE HashTable
    )

/*++

Routine Description:

    Empties a hash table containing lease identifiers.

Arguments:

    HashTable - list with identifiers to clear out.

Return Value:

    n/a
    
--*/

{
    PVOID Iter = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    BOOLEAN Deleted = FALSE;

    //
    // Iterate over the lease relationships and deallocate them.
    //
    Iter = RtlEnumerateGenericTable(HashTable, TRUE);
        
    while (NULL != Iter) {
             
        //
        // Retrieve lease relationship identifier.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)Iter);

        //
        // Remove list entry.
        //
        Deleted = RtlDeleteElementGenericTable(
            HashTable, 
            &LeaseRelationshipIdentifier
            );
        LAssert(Deleted);
        
        //
        // Deallocate lease relationship identifier.
        //
        LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);

        //
        // Move on to the next element.
        //
        Iter = RtlEnumerateGenericTable(HashTable, TRUE);
    }
}

VOID
ClearLeasingApplicationList(
    __in PRTL_GENERIC_TABLE HashTable
    )

/*++

Routine Description:

    Empties a hash table containing leasing application identifiers.

Arguments:

    HashTable - list with identifiers to clear out.

Return Value:

    n/a
    
--*/

{
    PVOID Iter = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    BOOLEAN Deleted = FALSE;

    //
    // Iterate over the leasing applications and deallocate them.
    //
    Iter = RtlEnumerateGenericTable(HashTable, TRUE);
        
    while (NULL != Iter) {
             
        //
        // Retrieve lease relationship identifier.
        //
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)Iter);

        if (IS_LARGE_INTEGER_EQUAL_TO_INTEGER(LeasingApplicationContext->ProcessAssertTime, 0))
        {
            //
            // Remove list entry.
            //
            Deleted = RtlDeleteElementGenericTable(
                HashTable,
                &LeasingApplicationContext
            );
            LAssert(Deleted);

            //
            // Deallocate leasing application identifier.
            //
            LeasingApplicationDestructor(LeasingApplicationContext);

            //
            // Move on the next element.
            //
            Iter = RtlEnumerateGenericTable(HashTable, TRUE);
        }
        else
        {
            //
            // Move on the next element.
            //
            Iter = RtlEnumerateGenericTable(HashTable, FALSE);
        }
    }
}

VOID CheckHangLeasingApplication(__in PRTL_GENERIC_TABLE HashTable)
{
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    PVOID LeasingApplicationIter = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;

    for (LeasingApplicationIter = RtlEnumerateGenericTable(HashTable, TRUE);
        NULL != LeasingApplicationIter;
        LeasingApplicationIter = RtlEnumerateGenericTable(HashTable, FALSE)) {
        //
        // Retrieve leasing application context.
        //
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)LeasingApplicationIter);

        if (IS_LARGE_INTEGER_GREATER_THAN_INTEGER(LeasingApplicationContext->ProcessAssertTime, 0))
        {
            LARGE_INTEGER Now;
            GetCurrentTime(&Now);
            LARGE_INTEGER ProcessAssertDiff;
            LARGE_INTEGER_SUBTRACT_LARGE_INTEGER_AND_ASSIGN(ProcessAssertDiff, Now, LeasingApplicationContext->ProcessAssertTime);

            if (IS_LARGE_INTEGER_LESS_THAN_INTEGER(ProcessAssertDiff, DriverContext->ProcessAssertExitTimeout * MILLISECOND_TO_NANOSECOND_FACTOR))
            {
                EventWriteProcessAssertExitPending(NULL, LeasingApplicationContext->ProcessAssertTime.QuadPart, ProcessAssertDiff.QuadPart, DriverContext->ProcessAssertExitTimeout * MILLISECOND_TO_NANOSECOND_FACTOR);
            }
            else
            {
                EventWriteProcessAssertExitTimeout(NULL, LeasingApplicationContext->ProcessAssertTime.QuadPart, ProcessAssertDiff.QuadPart, DriverContext->ProcessAssertExitTimeout * MILLISECOND_TO_NANOSECOND_FACTOR);

                KeBugCheckEx(
                    LEASLAYR_BUGCHECK_CODE,
                    LEASLAYR_PROCESS_ASSERT_EXIT_TIMEOUT_BUGCHECK_CODE,
                    0,
                    0,
                    0);
            }
        }

    }
}

VOID UpdateAssertLeasingApplication(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PEPROCESS TerminatingProcessHandle,
    __in BOOLEAN IsProcessAssert)
{
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    PVOID IterLeasingApplication = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    BOOLEAN Compare = FALSE;

    //
    // Check to see if this lease agent contains the leasing application.
    //
    for (IterLeasingApplication = RtlEnumerateGenericTable(
        &LeaseAgentContext->LeasingApplicationContextHashTable,
        TRUE);
        NULL != IterLeasingApplication;
        IterLeasingApplication = RtlEnumerateGenericTable(
            &LeaseAgentContext->LeasingApplicationContextHashTable,
            FALSE)) {

        //
        // Retrieve leasing application context.
        //
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

        Compare = (TerminatingProcessHandle == LeasingApplicationContext->ProcessHandle);

        if (Compare) {

            if (IsProcessAssert == TRUE)
            {
                LARGE_INTEGER Now;
                GetCurrentTime(&Now);
                LeasingApplicationContext->ProcessAssertTime = Now;
            }
            else
            {
                // In case process exit so quick, and leasing application has not been moved to the unregister list yet
                LeasingApplicationContext->ProcessAssertTime.QuadPart = 0;
            }
        }
    }

    if (IsProcessAssert == FALSE)
    {
        IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextUnregisterList, TRUE);

        while (NULL != IterLeasingApplication) {
            LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);


            if (TerminatingProcessHandle == LeasingApplicationContext->ProcessHandle)
            {
                // Reset the process assert time back to 0
                if (IS_LARGE_INTEGER_GREATER_THAN_INTEGER(LeasingApplicationContext->ProcessAssertTime, 0))
                {
                    LARGE_INTEGER ProcessAssertTime = LeasingApplicationContext->ProcessAssertTime;
                    LeasingApplicationContext->ProcessAssertTime.QuadPart = 0;

                    LARGE_INTEGER Now;
                    GetCurrentTime(&Now);
                    LARGE_INTEGER ProcessAssertDiff;
                    LARGE_INTEGER_SUBTRACT_LARGE_INTEGER_AND_ASSIGN(ProcessAssertDiff, Now, ProcessAssertTime);
                    EventWriteProcessAssertExit(NULL, ProcessAssertTime.QuadPart, ProcessAssertDiff.QuadPart, DriverContext->ProcessAssertExitTimeout);
                }
            }

            //
            // Move on the next element.
            //
            IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextUnregisterList, FALSE);
        }
    }
}

NTSTATUS
DeserializeLeaseRelationshipIdentifierList(
__in PRTL_GENERIC_TABLE HashTable,
__in PVOID Buffer,
__in ULONG Size
)

/*++

Routine Description:

Deserializes the list of identifiers from buffer into the list.

Arguments:

HashTable - list to populate with with identifiers

Buffer - holds the serialized list

Return Value:

n/a

--*/

{
    ULONG ElementCount = 0;
    ULONG ByteCount = 0;
    ULONG BufferSize = 0;
    ULONG CurrentBufferSize = 0;
    ULONG Index = 0;
    PBYTE Source = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER* LeaseRelationshipIdentifierExistent = NULL;
    LPCWSTR LeasingApplicationIdentifier = NULL;
    LPCWSTR RemoteLeasingApplicationIdentifier = NULL;
    BOOLEAN Inserted = FALSE;

    if (Size < sizeof(ULONG))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize source buffer.
    //
    Source = Buffer;

    //
    // Deserialize the size of the list.
    //
    if (memcpy_s(
        &BufferSize,
        sizeof(ULONG),
        Source,
        sizeof(ULONG)
        ))
    {
        return STATUS_DATA_ERROR;
    }

    if (BufferSize > Size)
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // Move source.
    //
    if (CurrentBufferSize + sizeof(ULONG) > BufferSize) {

        return STATUS_INVALID_PARAMETER;
    }

    Source = Source + sizeof(ULONG);
    CurrentBufferSize += sizeof(ULONG);

    //
    // Deserialize the number of elements from the list.
    //
    if (memcpy_s(
        &ElementCount,
        sizeof(ULONG),
        Source,
        sizeof(ULONG)
        ))
    {
        return STATUS_DATA_ERROR;
    }

    if (0 == BufferSize && 0 != ElementCount) {

        return STATUS_INVALID_PARAMETER;
    }

    if (0 == ElementCount) {

        return STATUS_SUCCESS;
    }

    //
    // Move source.
    //
    if (CurrentBufferSize + sizeof(ULONG) > BufferSize) {

        return STATUS_INVALID_PARAMETER;
    }

    Source = Source + sizeof(ULONG);
    CurrentBufferSize += sizeof(ULONG);

    for (Index = 0; Index < ElementCount; Index++) {

        //
        // Deserialize the leasing application identifier byte count.
        //
        if (memcpy_s(
            &ByteCount,
            sizeof(ULONG),
            Source,
            sizeof(ULONG)
            ))
        {
            return STATUS_DATA_ERROR;
        }
            
        //
        // Need positive even number of bytes.
        //
        if (0 == ByteCount || 
            0 != ByteCount % 2 ||
            sizeof(WCHAR) * (MAX_PATH + 1) < ByteCount) {

            return STATUS_INVALID_PARAMETER;
        }

        //
        // Move source.
        //
        if (CurrentBufferSize + sizeof(ULONG) > BufferSize) {
        
            return STATUS_INVALID_PARAMETER;
        }
        
        Source = Source + sizeof(ULONG);
        CurrentBufferSize += sizeof(ULONG);

        //
        // Check incoming string.
        //
        if (!IsValidString((PWCHAR)Source, ByteCount / sizeof(WCHAR))) {

            return STATUS_INVALID_PARAMETER;
        }
        
        //
        // Remember leasing application identifier offset.
        //
        LeasingApplicationIdentifier = (LPCWSTR)Source;
    
        //
        // Move source.
        //
        if (CurrentBufferSize + ByteCount > BufferSize) {
        
            return STATUS_INVALID_PARAMETER;
        }

        Source = Source + ByteCount;
        CurrentBufferSize += ByteCount;

        //
        // Deserialize the leasing application identifier byte count.
        //
        if (memcpy_s(
            &ByteCount,
            sizeof(ULONG),
            Source,
            sizeof(ULONG)
            ))
        {
            return STATUS_DATA_ERROR;
        }
            
        //
        // Need positive even number of bytes.
        //
        if (0 == ByteCount || 
            0 != ByteCount % 2 ||
            sizeof(WCHAR) * (MAX_PATH + 1) < ByteCount) {

            return STATUS_INVALID_PARAMETER;
        }

        //
        // Move source.
        //
        if (CurrentBufferSize + sizeof(ULONG) > BufferSize) {
        
            return STATUS_INVALID_PARAMETER;
        }

        Source = Source + sizeof(ULONG);
        CurrentBufferSize += sizeof(ULONG);

        //
        // Check incoming string.
        //
        if (!IsValidString((PWCHAR)Source, ByteCount / sizeof(WCHAR))) {

            return STATUS_INVALID_PARAMETER;
        }

        //
        // Remember leasing application identifier offset.
        //
        RemoteLeasingApplicationIdentifier = (LPCWSTR)Source;

        //
        // Construct lease relationship identifier.
        //
        LeaseRelationshipIdentifier = LeaseRelationshipIdentifierConstructor(
            LeasingApplicationIdentifier, 
            RemoteLeasingApplicationIdentifier
            );
        
        if (NULL == LeaseRelationshipIdentifier) {
            
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    
        //
        // Insert new element into the hash table.
        //
        LeaseRelationshipIdentifierExistent = RtlInsertElementGenericTable(
            HashTable,
            &LeaseRelationshipIdentifier,
            sizeof(PLEASE_RELATIONSHIP_CONTEXT),
            &Inserted
            );
        
        //
        // Check insert result.
        //
        if (NULL == LeaseRelationshipIdentifierExistent) {
        
            LAssert(!Inserted);

            //
            // Cleanup.
            //
            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);

            //
            // Hash table memory allocation failed.
            //
            return STATUS_INSUFFICIENT_RESOURCES;
            
        } else if (*LeaseRelationshipIdentifierExistent != LeaseRelationshipIdentifier) {
        
            LAssert(!Inserted);
        
            //
            // Cleanup.
            //
            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);

            return STATUS_INVALID_PARAMETER;
            
         } else {

            LAssert(Inserted);
        }

        //
        // Move source.
        //
        if (CurrentBufferSize + ByteCount > BufferSize) {
        
            return STATUS_INVALID_PARAMETER;
        }
        
        Source = Source + ByteCount;
        CurrentBufferSize += ByteCount;
    }

    if (CurrentBufferSize != BufferSize) {

        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DeserializeMessageListenEndPoint(
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in PVOID Buffer,
    __in ULONG Size
    )
{
    PBYTE Source = NULL;
    ULONG ListenEndPointAddressSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    Source = (PBYTE)Buffer;

    //
    // Check incoming string.
    //
    ListenEndPointAddressSize = Size - sizeof(ADDRESS_FAMILY) - sizeof(USHORT);
    if (ListenEndPointAddressSize > ENDPOINT_ADDR_CCH_MAX * sizeof(WCHAR))
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Deserialize end point fail with invalid address size", Size);
        return STATUS_INVALID_PARAMETER;
    }

    if (!IsValidString((PWCHAR)Source, ListenEndPointAddressSize / sizeof(WCHAR)))
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Deserialize end point fail with invalid address string", ListenEndPointAddressSize);
        return STATUS_INVALID_PARAMETER;
    }

    Status = RtlStringCchCopyW(
        RemoteSocketAddress->Address,
        ListenEndPointAddressSize / sizeof(WCHAR),
        (PWCHAR)Source);

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Str copy failed with ", Status);
        return Status;
    }
    Source = Source + ListenEndPointAddressSize;
    if (memcpy_s(&RemoteSocketAddress->ResolveType, sizeof(ADDRESS_FAMILY), Source, sizeof(ADDRESS_FAMILY)))
    {
        return STATUS_DATA_ERROR;
    }

    Source = Source + sizeof(ADDRESS_FAMILY);
    if (memcpy_s(&RemoteSocketAddress->Port, sizeof(USHORT), Source, sizeof(USHORT)))
    {
        return STATUS_DATA_ERROR;
    }

    return Status;
}

NTSTATUS
DeserializeMessageExtension(
    __in PLEASE_MESSAGE_EXT MessageExtension,
    __in PVOID Buffer,
    __in ULONG Size
    )
{
    ULONG MsgExtInstanceSize = sizeof(LARGE_INTEGER);
    if (Size < MsgExtInstanceSize)
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Deserialize message extension failed with invalid buffer size ", Size);
        return STATUS_INVALID_PARAMETER;
    }

    if (memcpy_s(&MessageExtension->MsgLeaseAgentInstance, MsgExtInstanceSize, Buffer, MsgExtInstanceSize))
    {
        return STATUS_DATA_ERROR;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DeserializeLeaseMessage(
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader,
    __in PVOID Buffer,
    __in PRTL_GENERIC_TABLE SubjectPendingList,
    __in PRTL_GENERIC_TABLE SubjectFailedPendingList,
    __in PRTL_GENERIC_TABLE MonitorFailedPendingList,
    __in PRTL_GENERIC_TABLE SubjectPendingAcceptedList,
    __in PRTL_GENERIC_TABLE SubjectPendingRejectedList,
    __in PRTL_GENERIC_TABLE SubjectFailedAcceptedList,
    __in PRTL_GENERIC_TABLE MonitorFailedAcceptedList,
    __in PRTL_GENERIC_TABLE SubjectTerminatePendingList,
    __in PRTL_GENERIC_TABLE SubjectTerminateAcceptedList,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteMessageSocketAddress,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteLeaseSocketAddress,
    __in PLEASE_MESSAGE_EXT MessageExtension
    )

/*

Routine Description:

    Reads in the message body. Assumes that the message hader has 
    been already validated.

Arguments:

    LeaseMessageHeader - message header describing the message body.

    Buffer - the buffer received.

    BufferSize - buffer size received.

    MessageIdentifier - message identifier to be processed.
    
    MessageType - received message type.
    
    Identifier - remote lease agent identifier.
    
    Duration - lease TTL.

    SubjectPendingList - subject pending list.
    
    SubjectFailedPendingList - subject failed pending list.
    
    MonitorFailedPendingList - monitor failed pending list.

    SubjectPendingAcceptedList - subject pending accepted list.

    SubjectPendingRejectedList - subject pending rejected.
    
    SubjectFailedAcceptedList - subject failed accepted list.
    
    MonitorFailedAcceptedList - monitor failed accepted.

    SubjectTerminatePendingList - subject terminate pending list.
    
    SubjectTerminateAcceptedList - subject teminate accepted list.

Return Value:

   STATUS_SUCCESS if the message body is valid and was processed successfully.
   
*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID SubjectPendingListStartBuffer = NULL;
    PVOID SubjectFailedPendingListStartBuffer = NULL;
    PVOID MonitorFailedListStartBuffer = NULL;
    PVOID SubjectPendingAcceptedListStartBuffer = NULL;
    PVOID SubjectFailedAcceptedListStartBuffer = NULL;
    PVOID MonitorFailedAcceptedListStartBuffer = NULL;
    PVOID SubjectPendingRejectedListStartBuffer = NULL;
    PVOID SubjectTerminatePendingListStartBuffer = NULL;
    PVOID SubjectTerminateAcceptedListStartBuffer = NULL;

    PVOID MessageListenEndPointStartBuffer = NULL;
    PVOID LeaseListenEndPointStartBuffer = NULL;

    PVOID MessageExtensionStartBuffer = NULL;
    ULONG MessageBodySize = 0;
    ULONG IncomingMessageExtensionSize = 0;

    //
    // Start deserialization.
    //
    SubjectPendingListStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->SubjectPendingListDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;
    SubjectFailedPendingListStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->SubjectFailedPendingListDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;
    MonitorFailedListStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->MonitorFailedPendingListDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;
    SubjectPendingAcceptedListStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->SubjectPendingAcceptedListDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;
    SubjectFailedAcceptedListStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->SubjectFailedAcceptedListDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;
    MonitorFailedAcceptedListStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->MonitorFailedAcceptedListDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;
    SubjectPendingRejectedListStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->SubjectPendingRejectedListDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;
    SubjectTerminatePendingListStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->SubjectTerminatePendingListDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;
    SubjectTerminateAcceptedListStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->SubjectTerminateAcceptedListDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;

    MessageListenEndPointStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->MessageListenEndPointDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;

    MessageBodySize = LeaseMessageHeader->MessageListenEndPointDescriptor.StartOffset
        + LeaseMessageHeader->MessageListenEndPointDescriptor.Size;

    if (IsIndirectLeaseMsg (LeaseMessageHeader->MessageType) &&
        LeaseMessageHeader->LeaseListenEndPointDescriptor.Size != 0 &&
        LeaseMessageHeader->LeaseListenEndPointDescriptor.StartOffset != 0)
    {
        LeaseListenEndPointStartBuffer = 
            (PBYTE)Buffer + 
            LeaseMessageHeader->LeaseListenEndPointDescriptor.StartOffset - 
            LeaseMessageHeader->MessageHeaderSize;

        MessageBodySize = LeaseMessageHeader->LeaseListenEndPointDescriptor.StartOffset
            + LeaseMessageHeader->LeaseListenEndPointDescriptor.Size;
    }

    if (MessageBodySize < LeaseMessageHeader->MessageSize)
    {
        // There is message extension
        MessageExtensionStartBuffer = (PBYTE)Buffer + MessageBodySize - LeaseMessageHeader->MessageHeaderSize;
        IncomingMessageExtensionSize = LeaseMessageHeader->MessageSize - MessageBodySize;
    }

    //
    // Deserialize subject pending list.
    //
    Status = DeserializeLeaseRelationshipIdentifierList(
        SubjectPendingList,
        SubjectPendingListStartBuffer,
        LeaseMessageHeader->SubjectPendingListDescriptor.Size
        );

    if (!NT_SUCCESS(Status)) {

        goto Error;
    }
    
    //
    // Deserialize subject failed pending list.
    //
    Status = DeserializeLeaseRelationshipIdentifierList(
        SubjectFailedPendingList,
        SubjectFailedPendingListStartBuffer,
        LeaseMessageHeader->SubjectFailedPendingListDescriptor.Size
        );

    if (!NT_SUCCESS(Status)) {

        goto Error;
    }

    //
    // Deserialize monitor failed list.
    //
    Status = DeserializeLeaseRelationshipIdentifierList(
        MonitorFailedPendingList,
        MonitorFailedListStartBuffer,
        LeaseMessageHeader->MonitorFailedPendingListDescriptor.Size
        );

    if (!NT_SUCCESS(Status)) {

        goto Error;
    }
    
    //
    // Deserialize subject pending accepted list.
    //
    Status = DeserializeLeaseRelationshipIdentifierList(
        SubjectPendingAcceptedList,
        SubjectPendingAcceptedListStartBuffer,
        LeaseMessageHeader->SubjectPendingAcceptedListDescriptor.Size
        );

    if (!NT_SUCCESS(Status)) {

        goto Error;
    }

    //
    // Deserialize subject failed accepted list.
    //
    Status = DeserializeLeaseRelationshipIdentifierList(
        SubjectFailedAcceptedList,
        SubjectFailedAcceptedListStartBuffer,
        LeaseMessageHeader->SubjectFailedAcceptedListDescriptor.Size
        );

    if (!NT_SUCCESS(Status)) {

        goto Error;
    }

    //
    // Deserialize monitor failed accepted list.
    //
    Status = DeserializeLeaseRelationshipIdentifierList(
        MonitorFailedAcceptedList,
        MonitorFailedAcceptedListStartBuffer,
        LeaseMessageHeader->MonitorFailedAcceptedListDescriptor.Size
        );

    if (!NT_SUCCESS(Status)) {

        goto Error;
    }

    //
    // Deserialize subject pending rejected list.
    //
    Status = DeserializeLeaseRelationshipIdentifierList(
        SubjectPendingRejectedList,
        SubjectPendingRejectedListStartBuffer,
        LeaseMessageHeader->SubjectPendingRejectedListDescriptor.Size
        );

    if (!NT_SUCCESS(Status)) {

        goto Error;
    }

    //
    // Deserialize subject terminate pending list.
    //
    Status = DeserializeLeaseRelationshipIdentifierList(
        SubjectTerminatePendingList,
        SubjectTerminatePendingListStartBuffer,
        LeaseMessageHeader->SubjectTerminatePendingListDescriptor.Size
        );

    if (!NT_SUCCESS(Status)) {

        goto Error;
    }

    //
    // Deserialize subject terminate accepted list.
    //
    Status = DeserializeLeaseRelationshipIdentifierList(
        SubjectTerminateAcceptedList,
        SubjectTerminateAcceptedListStartBuffer,
        LeaseMessageHeader->SubjectTerminateAcceptedListDescriptor.Size
        );

    if (!NT_SUCCESS(Status)) {

        goto Error;
    }

    Status = DeserializeMessageListenEndPoint(
        RemoteMessageSocketAddress,
        MessageListenEndPointStartBuffer,
        LeaseMessageHeader->MessageListenEndPointDescriptor.Size);

    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }

    if (LeaseListenEndPointStartBuffer != NULL)
    {
        Status = DeserializeMessageListenEndPoint(
            RemoteLeaseSocketAddress,
            LeaseListenEndPointStartBuffer,
            LeaseMessageHeader->LeaseListenEndPointDescriptor.Size);

        if (!NT_SUCCESS(Status))
        {
            EventWriteLeaseDriverTextTraceError(NULL, L"Deserialize Lease EndPoint failed with ", Status);
            goto Error;
        }
    }

    if (MessageExtensionStartBuffer != NULL)
    {
        Status = DeserializeMessageExtension(
            MessageExtension,
            MessageExtensionStartBuffer,
            IncomingMessageExtensionSize);

        if (!NT_SUCCESS(Status))
        {
            goto Error;
        }
    }

    return Status;

Error:

    //
    // Clear lists.
    //
    ClearRelationshipIdentifierList(SubjectPendingList);
    ClearRelationshipIdentifierList(SubjectFailedPendingList);
    ClearRelationshipIdentifierList(MonitorFailedPendingList);
    ClearRelationshipIdentifierList(SubjectPendingAcceptedList);
    ClearRelationshipIdentifierList(SubjectPendingRejectedList);
    ClearRelationshipIdentifierList(SubjectFailedAcceptedList);
    ClearRelationshipIdentifierList(MonitorFailedAcceptedList);
    ClearRelationshipIdentifierList(SubjectTerminatePendingList);
    ClearRelationshipIdentifierList(SubjectTerminateAcceptedList);

    LeaseListenEndpointDestructor(RemoteMessageSocketAddress);
    LeaseListenEndpointDestructor(RemoteLeaseSocketAddress);

    MessageExtensionDestructor(MessageExtension);

    return Status;
}

const LPWSTR GetTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PKTIMER Timer
    )
    
/*++

Routine Description:

    Returns readable lease timer name.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent.

    Timer - timer in the lease context of the remote lease agent.

Return Value:

    Stringyfied timer.

--*/

{
    LPWSTR TimerString = L"";

    if (Timer == &RemoteLeaseAgentContext->LeaseRelationshipContext->RenewOrArbitrateTimer) {

        TimerString = L"Renew/Arbitrate";
        
    } else if (Timer == &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimer) {

        TimerString = L"Subject Expired";

    } else if (Timer == &RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorTimer) {

        TimerString = L"Monitor Expired";

    } else if (Timer == &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationSubjectTimer) {

        TimerString = L"Pre Arbitration Subject Timer";

    } else if (Timer == &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationMonitorTimer) {

        TimerString = L"Pre Arbitration Monitor Timer";

    }
    else if (Timer == &RemoteLeaseAgentContext->LeaseRelationshipContext->PostArbitrationTimer) {

        TimerString = L"Post Arbitration";

    }
    else if (Timer == &RemoteLeaseAgentContext->LeaseRelationshipContext->PingRetryTimer) {

        TimerString = L"Ping Retry";

    }
    else {

        LAssert(FALSE);
    }

    return TimerString;
}

VOID
EnqueueTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PKTIMER Timer,
    __in LARGE_INTEGER ExpireTime,
    __in PKDPC TimerDpc
    )
    
/*++

Routine Description:

    Enqueues a timer for execution.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent context.
    
    Timer - timer to enqueue.

    ExpireTime - expiration for this timer.

    TimerDpc - timer routine.

Return Value:

    n/a

--*/

{
    BOOLEAN SetTimer = FALSE;
    LARGE_INTEGER Now;
    LARGE_INTEGER TTL;
    //
    // Increment the reference count of the remote lease agent.
    //
    AddRef(RemoteLeaseAgentContext);

    //
    // Enqueue the timer.
    //
    GetCurrentTime(&Now);
    if (IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(ExpireTime, Now))
    {
        LARGE_INTEGER_SUBTRACT_LARGE_INTEGER_AND_ASSIGN(TTL, Now, ExpireTime);
    }
    else
    {
        TTL.QuadPart = 0;
    }

    SetTimer = KeSetTimer(
        Timer, 
        TTL, 
        TimerDpc
        );
    LAssert(!SetTimer);

    EventWriteLeaseRelationEnqueue1(
        NULL,
        GetTimer(RemoteLeaseAgentContext, Timer),
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
		TTL.QuadPart,
		ExpireTime.QuadPart
        );
}

VOID
DequeueTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PKTIMER Timer
    )
    
/*++

Routine Description:

    Dequeues a timer from execution.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent context.
    
    Timer - timer to dequeue.

Return Value:

    n/a

--*/

{
    BOOLEAN CancelTimer = FALSE;

    //
    // Enqueue the timer.
    //
    CancelTimer = KeCancelTimer(Timer);

    if (CancelTimer) {

        //
        // Since the timer is not executing, then the ref count has to be adjusted.
        //
        Release(RemoteLeaseAgentContext);
    }

    if (CancelTimer)
    {
        EventWriteLeaseRelationEnqueue2(
            NULL,
            GetTimer(RemoteLeaseAgentContext, Timer),
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart
            );
    }
}

VOID
ArmTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PKTIMER Timer,
    __in LARGE_INTEGER ExpireTime,
    __in PKDPC TimerDpc
    )
    
/*++

Routine Description:

    Dequeues/Cancels a timer and then re-enqueues it.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent context.
    
    Timer - timer to dequeue/enqueue.

    ExpireTime - expiration for this timer.

    TimerDpc - timer routine.

Return Value:

    n/a

--*/

{
    //
    // Attempt to dequeue the timer.
    //
    DequeueTimer(RemoteLeaseAgentContext, Timer);

    //
    // Enqueue timer.
    //
    EnqueueTimer(RemoteLeaseAgentContext, Timer, ExpireTime, TimerDpc);
}

VOID
StartUnregisterApplication(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG TTLInMilliseconds
)
/*++

Routine Description:

    Complete delayed IO request when enqueue the unregister timer.

Parameters Description:

    LeasingApplicationContext - leasing application to be unregistered

Return Value:

    n/a

--*/
{
    //
    // Complete delayed IO request
    // To the user mode, this leasing application does not exist any more
    //

    if (NULL != LeasingApplicationContext->DelayedRequest) {

        //
        // Need to complete the delayed request.
        //
        DelayedRequestCompletion(
            LeasingApplicationContext,
            LEASING_APPLICATION_NONE,
            NULL,
            LeasingApplicationContext->Instance,
            NULL,
            -1,
            -1,
            NULL,
            0,
            0,
            STATUS_SUCCESS,
            TRUE,
            0,
            NULL,
            NULL,
            0,
            NULL,
            NULL,
            0,
            0,
            0,
            0
            );
    }

    ArmUnregisterTimer(
        LeasingApplicationContext,
        TTLInMilliseconds
        );
}

VOID
EnqueueUnregisterTimer(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG TTLInMilliseconds
    )
    
/*++

Routine Description:

    Enqueues a timer for unregister a leasing application.

Parameters Description:

    LeasingApplicationContext - leasing application to be unregistered

    TTL - Time to live in milliseconds.

Return Value:

    n/a

--*/

{
    BOOLEAN SetTimerRet = FALSE;
    LARGE_INTEGER TimerTTL;

    if (MAXLONG == TTLInMilliseconds || TTLInMilliseconds <= 0)
        TimerTTL.QuadPart = 0;
    else
        TimerTTL.QuadPart =  -((LONGLONG)TTLInMilliseconds) * MILLISECOND_TO_NANOSECOND_FACTOR;

    EventWriteUnregisterTimerEnqueue(
        NULL,
        LeasingApplicationContext->LeasingApplicationIdentifier,
        TTLInMilliseconds,
        SetTimerRet,
        TimerTTL.QuadPart
        );

    SetTimerRet = KeSetTimer(
        &LeasingApplicationContext->UnregisterTimer, 
        TimerTTL, 
        &LeasingApplicationContext->UnregisterTimerDpc
        );

    LAssert(!SetTimerRet);
}

VOID
DequeueUnregisterTimer(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    )
    
/*++
Routine Description:
    Dequeues a timer for unregister a leasing application.
--*/

{
    BOOLEAN CancelTimer = FALSE;

    //
    // Enqueue the timer.
    //
    CancelTimer = KeCancelTimer(&LeasingApplicationContext->UnregisterTimer);

    if (CancelTimer)
    {
        EventWriteCancelUnregisterTimer(
            NULL,
            LeasingApplicationContext->LeasingApplicationIdentifier,
            CancelTimer
            );
    }

}

VOID
ArmUnregisterTimer(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG TTLInMilliseconds
    )
    
/*++
Routine Description:
Enqueues a timer for unregister a leasing application.
--*/
{
    //
    // Application timer should only enqueue once .
    //
    LAssert(FALSE == LeasingApplicationContext->IsBeingUnregistered);

    LeasingApplicationContext->IsBeingUnregistered = TRUE;
    //
    // Enqueue timer.
    //
    EnqueueUnregisterTimer(LeasingApplicationContext, TTLInMilliseconds);
}

VOID
EnqueueDelayedLeaseFailureTimer(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in LONG TTLInMilliseconds
    )
/*++
Routine Description:
    Enqueues a timer for lease failure and sending failed termination to neighbors.
--*/
{
    BOOLEAN SetTimerRet = FALSE;
    LARGE_INTEGER TimerTTL;
    if (MAXLONG == TTLInMilliseconds || TTLInMilliseconds <= 0)
        TimerTTL.QuadPart = 0;
    else
        TimerTTL.QuadPart =  -((LONGLONG)TTLInMilliseconds) * MILLISECOND_TO_NANOSECOND_FACTOR;

    SetTimerRet = KeSetTimer(
        &LeaseAgentContext->DelayedLeaseFailureTimer,
        TimerTTL, 
        &LeaseAgentContext->DelayedLeaseFailureTimerDpc
        );

    EventWriteDelayTimerEnqueue(
        NULL,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart,
        TTLInMilliseconds,
        SetTimerRet,
        TimerTTL.QuadPart
        );

    LAssert(!SetTimerRet);
}

VOID
ArmDelayedLeaseFailureTimer(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in LONG TTLInMilliseconds
    )
{
    // Lease agent delay timer should only enqueue once .
    //
    LAssert(FALSE == LeaseAgentContext->IsInDelayLeaseFailureLeaseAgentCloseTimer);

    LeaseAgentContext->IsInDelayLeaseFailureLeaseAgentCloseTimer = TRUE;

    // Enqueue timer.
    //
    EnqueueDelayedLeaseFailureTimer(LeaseAgentContext, TTLInMilliseconds);
}

VOID
DequeueDelayedLeaseFailureTimer(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext
)
/*++
Routine Description:
Dequeues delay lease failure timer.
--*/
{
    BOOLEAN CancelTimer = FALSE;
    //
    // dequeue the timer.
    //
    CancelTimer = KeCancelTimer(&LeaseAgentContext->DelayedLeaseFailureTimer);

    EventWriteDelayTimer(
        NULL,
        L" cancelled ",
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart
        );

}

VOID
EnqueueDelayLeaseAgentCloseTimer(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
)
/*++
Routine Description:
Enqueues delay lease agent close timer.
--*/
{
    BOOLEAN SetTimerRet = FALSE;
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    LONG TTLInMilliseconds = DriverContext->DelayLeaseAgentCloseInterval;
    LARGE_INTEGER TimerTTL;
    if (MAXLONG == TTLInMilliseconds || TTLInMilliseconds <= 0)
        TimerTTL.QuadPart = 0;
    else
        TimerTTL.QuadPart = -((LONGLONG)TTLInMilliseconds) * MILLISECOND_TO_NANOSECOND_FACTOR;

    SetTimerRet = KeSetTimer(
        &LeaseAgentContext->DelayLeaseAgentCloseTimer,
        TimerTTL,
        &LeaseAgentContext->DelayLeaseAgentCloseTimerDpc
        );

    EventWriteDelayLeaseAgentCloseTimerEnqueue(
        NULL,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart,
        TTLInMilliseconds,
        SetTimerRet,
        TimerTTL.QuadPart
        );

    LAssert(!SetTimerRet);
}

VOID
ArmDelayLeaseAgentCloseTimer(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
)
{
    //
    // Enqueue timer.
    //
    EnqueueDelayLeaseAgentCloseTimer(LeaseAgentContext);
}

VOID
DequeueDelayLeaseAgentCloseTimer(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
)
/*++
Routine Description:
Dequeues delay lease agent close timer.
--*/
{
    BOOLEAN CancelTimer = FALSE;
    //
    // dequeue the timer.
    //
    CancelTimer = KeCancelTimer(&LeaseAgentContext->DelayLeaseAgentCloseTimer);

    EventWriteDelayLeaseAgentCloseTimerDequeue(
        NULL,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart
        );
}

VOID
SetSubjectExpireTime(
    __in PLEASE_RELATIONSHIP_CONTEXT LeaseRelationshipContext,
    __in LARGE_INTEGER Expiration,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration
   )
    
/*++

Routine Description:

    Sets the subject expiration timer of a lease relationship.

Parameters Description:

    LeaseRelationshipContext - lease relationship containing the subject.

    Duration - duration until expiration in milliseconds.

Return Value:

    n/a

--*/

{
    //
    // Set subject fail time.
    //
    LARGE_INTEGER Now;
    if (MAXLONGLONG != Expiration.QuadPart) {

        //
        // Set the subject expire time.
        //
        LeaseRelationshipContext->SubjectExpireTime = Expiration;

        //
        // Set subject fail time and subject suspend time.
        //
        LeaseRelationshipContext->SubjectFailTime = LeaseRelationshipContext->SubjectExpireTime;
        LeaseRelationshipContext->SubjectSuspendTime = LeaseRelationshipContext->SubjectExpireTime;

        GetCurrentTime(&Now);
        EventWriteSetSubjectExpiration(
            NULL, 
            LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            GetLeaseState(LeaseRelationshipContext->SubjectState),
            (int)((LeaseRelationshipContext->SubjectExpireTime.QuadPart - Now.QuadPart)/(LONGLONG)MILLISECOND_TO_NANOSECOND_FACTOR),
            LeaseRelationshipContext->SubjectFailTime.QuadPart);

        //
        // Adjust subject fail time and subject suspend time. This applies to both one-way lease and two-way lease.
        //
        LARGE_INTEGER_ADD_INTEGER(
            LeaseRelationshipContext->SubjectFailTime,
            ((LONGLONG)ArbitrationDuration * MILLISECOND_TO_NANOSECOND_FACTOR)
            );
        LARGE_INTEGER_ADD_INTEGER(
            LeaseRelationshipContext->SubjectSuspendTime,
                ((LONGLONG)LeaseSuspendDuration * MILLISECOND_TO_NANOSECOND_FACTOR)
            );
    } else {

        EventWriteClearSubjectExpiration(
            NULL,
            LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            GetLeaseState(LeaseRelationshipContext->SubjectState));

        LeaseRelationshipContext->SubjectExpireTime.QuadPart = MAXLONGLONG;
        LeaseRelationshipContext->SubjectFailTime.QuadPart = MAXLONGLONG; 
        LeaseRelationshipContext->SubjectSuspendTime.QuadPart = MAXLONGLONG; 
    }
}

VOID
SetRenewTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN ByRenewTimerDPC,
    __in LONG RenewDuration
    )
    
/*++

Routine Description:

    Sets the renew expiration timer of a lease relationship.

Parameters Description:

    RemoteLeaseAgentContext - remote lease agent with lease relationship to renew.

Return Value:

    n/a

--*/

{
    LARGE_INTEGER RenewTime;
    LARGE_INTEGER RemainingTime;

    LARGE_INTEGER Now;
    LARGE_INTEGER RenewRetryInterval;
    LARGE_INTEGER RenewTimeTemp;
    LARGE_INTEGER RenewRetryTotal;

    LONG RetryCount;
    LONG RetryBegin;
    LONG Duration;
    LONG RemainDuration;

    LAssert(
        LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState &&
        LEASE_STATE_ACTIVE >= RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState
        );

    //
    // Take the system time.
    //
    GetCurrentTime(&Now);
    //
    // Compute remaining time.
    //
    LARGE_INTEGER_SUBTRACT_LARGE_INTEGER_AND_ASSIGN(
        RemainingTime, 
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime, 
        Now
        );

    // Compute renew interval based on the numberOfRetry config passed from user mode.
    //

    RetryCount = RemoteLeaseAgentContext->LeaseAgentContext->LeaseRetryCount;
    RetryBegin = RemoteLeaseAgentContext->LeaseAgentContext->LeaseRenewBeginRatio;

    Duration = (RenewDuration != 0) ? RenewDuration : RemoteLeaseAgentContext->LeaseRelationshipContext->Duration;
    RemainDuration = Duration - Duration / RetryBegin;
    RenewRetryInterval.QuadPart = (LONGLONG)(RemainDuration) * MILLISECOND_TO_NANOSECOND_FACTOR / RetryCount;

    // if it is renew retry, the RenewTimer DPC would start the indirect lease process
    RemoteLeaseAgentContext->LeaseRelationshipContext->IsRenewRetry = ByRenewTimerDPC;

    // If this is the first renew, reset the counter
    if (RemoteLeaseAgentContext->LeaseRelationshipContext->IsRenewRetry == FALSE)
    {
        RemoteLeaseAgentContext->LeaseRelationshipContext->RenewRetryCount = 0;
    }

    //
    // Do we have enough time to send another renew request?
    //
    if (IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(RemainingTime, RenewRetryInterval))
    {
        RenewTimeTemp.QuadPart = 
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime.QuadPart -
            (LONGLONG)(RemainDuration) * MILLISECOND_TO_NANOSECOND_FACTOR;

        if (RemoteLeaseAgentContext->LeaseRelationshipContext->IsRenewRetry == TRUE)
        {
            RemoteLeaseAgentContext->LeaseRelationshipContext->RenewRetryCount++;
            // RenewRetryTotal = RenewRetryInterval * RenewRetryCount
            RenewRetryTotal.QuadPart = RenewRetryInterval.QuadPart * RemoteLeaseAgentContext->LeaseRelationshipContext->RenewRetryCount;
            LARGE_INTEGER_ADD_LARGE_INTEGER_AND_ASSIGN(
                RenewTimeTemp,
                RenewTimeTemp,
                RenewRetryTotal);
        }
        
        RenewTime = RenewTimeTemp;
    }
    else
    {
        //
        // No need to fire this timer.  The subject timer will fire when lease expires.
        //
        if (LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        {
            return;
        }

        //
        // Fire the timer when the lease expires. The subject timer won't fire.
        //
        RenewTime = RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime;
    }
    
    //
    // Enqueue the renew timer.
    //
    ArmTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->RenewOrArbitrateTimer,
        RenewTime,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->RenewOrArbitrateTimerDpc
        );
}

VOID 
EstablishLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )
/*++
    Establishes a new lease relationship with a remote application.
--*/
{
    PVOID LeaseMessage = NULL;
    ULONG LeaseMessageSize = 0;
    LARGE_INTEGER Expiration;

    LONG Duration;
    LONG LeaseSuspendDuration;
    LONG ArbitrationDuration;

    NTSTATUS status;

    LARGE_INTEGER OutgoingMsgId;
    OutgoingMsgId.QuadPart = 0;

    LAssert(
        LEASE_STATE_EXPIRED >= RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState &&
        LEASE_STATE_EXPIRED >= RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState);

    GetDurationsForRequest(
        RemoteLeaseAgentContext,
        &Duration,
        &LeaseSuspendDuration,
        &ArbitrationDuration,
        FALSE);

    LAssert(Duration > 0 && Duration != DURATION_MAX_VALUE);

    GetCurrentTime(&Expiration);
    Expiration.QuadPart += (LONGLONG)Duration * MILLISECOND_TO_NANOSECOND_FACTOR;
    //
    // Check if this is the first lease relationship. If not, just send the lease request message.
    //
    if (LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState)
    {
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_ACTIVE;

        //
        // Set the subject identifier.
        //
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier = GetInstance();

        //
        // Set the lease relationship duration.
        //

        SetLeaseRelationshipDuration(
            RemoteLeaseAgentContext,
            Duration,
            LeaseSuspendDuration,
            ArbitrationDuration
            );

        //
        // Set the subject timer expiration time.
        //
        SetSubjectExpireTime(
            RemoteLeaseAgentContext->LeaseRelationshipContext,
            Expiration,
            LeaseSuspendDuration,
            ArbitrationDuration
            );

        //
        // Set subject timer, if necessary.
        //
        if (LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        {
            ArmTimer(
                RemoteLeaseAgentContext,
                &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimer,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime,
                &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimerDpc
                );
        }
        
        //
        // Set the renew timer.
        //
        SetRenewTimer(RemoteLeaseAgentContext, FALSE, 0);
    }

    //
    // Create lease establish request message.
    //
    SerializeLeaseMessage(
        RemoteLeaseAgentContext,
        LEASE_REQUEST,
        &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
        &RemoteLeaseAgentContext->SubjectFailedPendingHashTable,
        &RemoteLeaseAgentContext->MonitorFailedPendingHashTable,
        NULL,
        NULL,
        NULL,
        NULL,
        &RemoteLeaseAgentContext->SubjectTerminatePendingHashTable,
        NULL,
        Duration,
        Expiration,
        LeaseSuspendDuration,
        ArbitrationDuration,
        FALSE,
        &LeaseMessage,
        &LeaseMessageSize
        );

    if (NULL != LeaseMessage) 
    {
        OutgoingMsgId.QuadPart = ((PLEASE_MESSAGE_HEADER) LeaseMessage)->MessageIdentifier.QuadPart;
    }

    EventWriteLeaseRelationSendingLease(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
        GetMessageType(LEASE_REQUEST),
        OutgoingMsgId.QuadPart
        );

    //
    // If we could not allocate the message, the subject relationship is still
    // considered to be active from this point on.
    //
    if (NULL != LeaseMessage) {

        if (RemoteLeaseAgentContext->PartnerTarget == NULL) 
        {
            TransportResolveTarget(
                RemoteLeaseAgentContext->LeaseAgentContext->Transport, 
                &RemoteLeaseAgentContext->RemoteSocketAddress, 
                &RemoteLeaseAgentContext->PartnerTarget);
        }

        //
        // Send the lease establish request.
        //
        AddRef(RemoteLeaseAgentContext);

        status = TransportSendBufferNotification(
            RemoteLeaseAgentContext->PartnerTarget,
            LeaseMessage,
            LeaseMessageSize,
            MarkLeaseRelationshipSent,
            RemoteLeaseAgentContext,
            FALSE);

        if (!NT_SUCCESS(status))
        {
            Release(RemoteLeaseAgentContext);
        }
    }
}

VOID 
EstablishLeaseSendPingRequestMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Send ping message before establishing a new lease relationship with a remote application.

Arguments:

    RemoteLeaseAgentContext - remote lease agent used to establish the lease with.

Return Value:

    n/a

--*/

{
    LARGE_INTEGER Now;

    if (TRUE == IsRemoteLeaseAgentInPing(RemoteLeaseAgentContext))
    {
        //
        // ping was already started
        //
        EventWritePingProcessAlreadyStarted(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart
            );

        return;
    }

    RemoteLeaseAgentContext->InPing = TRUE;

    GetCurrentTime(&Now);

    //
    // Create and send ping request message.
    //
    ArmTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->PingRetryTimer,
        Now,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->PingRetryTimerDpc
        );
}

VOID 
SendPingRequestMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Create and send ping message.

Arguments:

    RemoteLeaseAgentContext - remote lease agent used to establish the lease with.

Return Value:

    n/a

--*/

{
    PVOID PingMessage = NULL;
    ULONG PingMessageSize = 0;

    LARGE_INTEGER OutgoingMsgId;
    OutgoingMsgId.QuadPart = 0;

    //
    // Create ping request message.
    //
    SerializeLeaseMessage(
        RemoteLeaseAgentContext,
        PING_REQUEST,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime,
        0,
        0,
        FALSE,
        &PingMessage,
        &PingMessageSize
        );

    if (NULL != PingMessage) 
    {
        OutgoingMsgId.QuadPart = ((PLEASE_MESSAGE_HEADER) PingMessage)->MessageIdentifier.QuadPart;
    }

    EventWriteLeaseRelationSendingLease(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
        GetMessageType(PING_REQUEST),
        OutgoingMsgId.QuadPart
        );

    //
    // if could not allocate ping message due to low resource
    // retry when renew timer fires
    //
    if (NULL != PingMessage)
    {
        if (RemoteLeaseAgentContext->PartnerTarget == NULL) 
        {
            TransportResolveTarget(
                RemoteLeaseAgentContext->LeaseAgentContext->Transport, 
                &RemoteLeaseAgentContext->RemoteSocketAddress, 
                &RemoteLeaseAgentContext->PartnerTarget);
        }

        //
        // Send the ping request.
        //
        TransportSendBuffer(
            RemoteLeaseAgentContext->PartnerTarget,
            PingMessage,
            PingMessageSize,
            FALSE);

        //
        // Set the Ping request time
        //
        if (IS_LARGE_INTEGER_EQUAL_TO_INTEGER(RemoteLeaseAgentContext->PingSendTime, 0))
        {
            GetCurrentTime(&RemoteLeaseAgentContext->PingSendTime);
        }
    }
}


VOID
SendPingResponseMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Send ping response back once receive a ping request.

--*/

{

    PVOID PingResponse = NULL;
    ULONG PingResponseSize = 0;

    LARGE_INTEGER Expiration;
    LARGE_INTEGER OutgoingMsgId;
    Expiration.QuadPart = 0;
    OutgoingMsgId.QuadPart = 0;

    //
    // send back the PING response
    //

    SerializeLeaseMessage(
        RemoteLeaseAgentContext,
        PING_RESPONSE,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        Expiration,
        0,
        0,
        FALSE,
        &PingResponse,
        &PingResponseSize
        );

    if (NULL != PingResponse) 
    {
        OutgoingMsgId.QuadPart = ((PLEASE_MESSAGE_HEADER) PingResponse)->MessageIdentifier.QuadPart;
    }

    EventWriteLeaseRelationSendingLease(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
        GetMessageType(PING_RESPONSE),
        OutgoingMsgId.QuadPart
        );

    //
    // if could not allocate ping response due to low resource
    // the ping request will retry after ping interval on sending side
    //
    if (NULL != PingResponse)
    {
        if (RemoteLeaseAgentContext->PartnerTarget == NULL) 
        {
            TransportResolveTarget(
                RemoteLeaseAgentContext->LeaseAgentContext->Transport, 
                &RemoteLeaseAgentContext->RemoteSocketAddress, 
                &RemoteLeaseAgentContext->PartnerTarget);
        }

        //
        // Send the ping response.
        //
        TransportSendBuffer(
            RemoteLeaseAgentContext->PartnerTarget,
            PingResponse,
            PingResponseSize,
            FALSE);

        //
        // Set the Ping response time
        //
        if (IS_LARGE_INTEGER_EQUAL_TO_INTEGER(RemoteLeaseAgentContext->PingSendTime, 0))
        {
            GetCurrentTime(&RemoteLeaseAgentContext->PingSendTime);
        }
    }

}

VOID
AbortPingProcess(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )
{
    LARGE_INTEGER SubjectExpiration;

    RemoteLeaseAgentContext->InPing = FALSE;

    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimer
        );

    //
    // Set the subject expire time back to default MaxLongLong.
    // The lease is not established, the GetTTL would report wrong value without this.
    //
    SubjectExpiration.QuadPart = MAXLONGLONG;

    SetSubjectExpireTime(
        RemoteLeaseAgentContext->LeaseRelationshipContext,
        SubjectExpiration,
        DURATION_MAX_VALUE,
        DURATION_MAX_VALUE
        );

    //
    // Attempt to dequeue the retry timer.
    //
    DequeueTimer(RemoteLeaseAgentContext, &RemoteLeaseAgentContext->LeaseRelationshipContext->PingRetryTimer);
}

VOID 
EstablishReverseLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier
    )

/*++

Routine Description:

    Establishes the reverse lease relationship upon receiving a lease request

    1. This routine is an optimization and no error status is returned;
    2. If anything goes wrong in the process, it will jump to the end for clean-up.
    3. The lease will be established later through user mode request.

Arguments:

    RemoteLeaseAgentContext - remote lease agent used to establish the lease with.

Return Value:

    n/a

--*/

{

    BOOLEAN InsertedToSubjectList = FALSE;
    BOOLEAN InsertedToSubjectPendingList = FALSE;

    PLEASE_RELATIONSHIP_IDENTIFIER* ExistentInSubjectList = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER* ExistentInSubjectPendingList = NULL;

    // LeaseRelationship for subject list is passed in as argument
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierPending = NULL;

    BOOLEAN Deleted = FALSE;

    PVOID IterLeasingApplication = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;

    //
    // The reverse lease effort is done when receive a lease request
    // Monitor would just be set to Active and there should be no subject timer in queue
    //

    if (IsRemoteLeaseAgentSuspended(RemoteLeaseAgentContext))
    {
        EventWriteReverseLeaseEstablishRLASuspended(
            NULL,
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart
            ); 

        goto End;
    }

    AbortPingProcess(RemoteLeaseAgentContext);

    //
    // Check that the lease relationship identifier is not in the subject list,
    // or the subject establish pending list, or the subject pending failed list.
    // The reverse lease establish is an optimization and we don't want to interfere
    // with current establishing/established/terminating operations
    //

    if (NULL != RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable, 
            &LeaseRelationshipIdentifier)
            )
    {
        // identifier is in the establish pending list
        // but the subject state is Inactive, still in Ping phase
        // Establish the lease immediately without adding the ID to the list

        EstablishLease( RemoteLeaseAgentContext );

        //
        // Jump to end to destruct the LeaseRelationshipIdentifier to avoid a leak
        //
        goto End;
    }
    else if (NULL != RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->SubjectHashTable, 
            &LeaseRelationshipIdentifier
            ) ||
        NULL != RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->SubjectFailedPendingHashTable, 
            &LeaseRelationshipIdentifier
            ) ||
        NULL != RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->SubjectTerminatePendingHashTable, 
            &LeaseRelationshipIdentifier
            )
        )
    {

        EventWriteLeaseRelationExistInReverseLeaseEstablish(
            NULL,
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart
            );

        goto End;
    }

    //
    // Add the lease relationship identifier to the subject list and 
    // subject pending list of remote lease agent.
    //
    ExistentInSubjectList = RtlInsertElementGenericTable(
        &RemoteLeaseAgentContext->SubjectHashTable,
        &LeaseRelationshipIdentifier,
        sizeof(PLEASE_RELATIONSHIP_IDENTIFIER),
        &InsertedToSubjectList
        );

    //
    // Check insert result.
    //
    if (NULL == ExistentInSubjectList)
    {
        LAssert(!InsertedToSubjectList);

        EventWriteRegisterLRIDSubListFail(
            NULL,
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart
            );

        goto End;

    }
    else
    {
        LAssert(InsertedToSubjectList);

        //
        // Add the lease identifier to the subject pending list of the remote lease agent.
        //
        LeaseRelationshipIdentifierPending = LeaseRelationshipIdentifierConstructor(
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier
            );

        if (NULL == LeaseRelationshipIdentifierPending)
        {

            goto End;

        }

        ExistentInSubjectPendingList = RtlInsertElementGenericTable(
            &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
            &LeaseRelationshipIdentifierPending,
            sizeof(PLEASE_RELATIONSHIP_IDENTIFIER),
            &InsertedToSubjectPendingList
            );

        if (NULL == ExistentInSubjectPendingList)
        {

            EventWriteRegisterLRIDSubPendingListFail(
                NULL,
                LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart
                );

            goto End;

        }
        else
        {

            EventWriteLeaseRelationRegisteredRemoteLA(
                NULL,
                LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart
                );
        }
    }


    //
    // Retrieve the leasing application.
    //
    IterLeasingApplication = RtlLookupElementGenericTable(
        &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable,
        &LeaseRelationshipIdentifier->LeasingApplicationContext
        );

    if (NULL != IterLeasingApplication)
    {
        //
        // Retrieve lease relationship identifier from the subject pending list.
        //
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

    }

    if (NULL == LeasingApplicationContext)
    {
        EventWriteLeasingAppNotRegisteredInReverseLease(
            NULL,
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
            TransportIdentifier(RemoteLeaseAgentContext->LeaseAgentContext->Transport),
            RemoteLeaseAgentContext->LeaseAgentContext->Instance.QuadPart
            );

        goto End;

    }

    EventWriteEstReverseLeaseRelationOnRemote(
        NULL,
        LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
        LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
        GetConfigLeaseDuration(RemoteLeaseAgentContext),
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart
        );

    //
    // Establish new lease relationship.
    //
    EstablishLease( RemoteLeaseAgentContext );

    return;

End:

    if (ExistentInSubjectList != NULL)
    {
        //
        // Remove lease identifier from subject list.
        // 
        Deleted = RtlDeleteElementGenericTable(
            &RemoteLeaseAgentContext->SubjectHashTable,
            &LeaseRelationshipIdentifier
            );
        LAssert(Deleted);
    }

    if (LeaseRelationshipIdentifierPending != NULL)
    {
        if (ExistentInSubjectPendingList != NULL)
        {
            //
            // Remove lease identifier from subject list.
            // 
            Deleted = RtlDeleteElementGenericTable(
                &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
                &LeaseRelationshipIdentifier
                );
            LAssert(Deleted);
        }

        LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifierPending);
    }

    LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);

}


NTSTATUS 
TerminateMonitorLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier
    )

/*++

Routine Description:

    Terminates an existent lease relationship with a remote application.

Arguments:

    RemoteLeaseAgentContext - remote lease agent.

    LeaseRelationshipIdentifier - lease relationship identifier.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN Inserted = FALSE;
    BOOLEAN Deleted = FALSE;
    PLEASE_RELATIONSHIP_IDENTIFIER* LeaseRelationshipIdentifierExistent = NULL;

    //
    // Move the lease relationship identifier from the monitor list
    // to the monitor failed pending list.
    //
    LeaseRelationshipIdentifierExistent = RtlInsertElementGenericTable(
        &RemoteLeaseAgentContext->MonitorFailedPendingHashTable,
        &LeaseRelationshipIdentifier,
        sizeof(PLEASE_RELATIONSHIP_IDENTIFIER),
        &Inserted
        );
    
    //
    // Check insert result.
    //
    if (NULL == LeaseRelationshipIdentifierExistent) {

        LAssert(!Inserted);

        EventWriteLeaseRelationTerminateFail1(
            NULL,
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart
            );

        //
        // Terminating lease agent due to insufficient resources.
        // This should be very rare occurence (could not allocate 4 bytes).
        //
        SetLeaseAgentFailed(RemoteLeaseAgentContext->LeaseAgentContext, FALSE);

        Status = STATUS_INSUFFICIENT_RESOURCES;
                
    } else {
    
        //
        // There should be only one copy of this element in this hash table.
        //
        LAssert(Inserted);

        EventWriteTerminateSubLeaseMonitorFailed(
            NULL,
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart
            );
        //
        // Remove the lease relationship identifier from the subject list.
        //
        Deleted = RtlDeleteElementGenericTable(
            &RemoteLeaseAgentContext->MonitorHashTable,
            &LeaseRelationshipIdentifier
            );
        LAssert(Deleted);
    }

    return Status;
}

VOID
TerminateSubjectLeaseSendMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN IsTwoWayTermination
    )

/*++

Routine Description:

    Send a termination or renew message after the subject related list are updated.

Arguments:

    RemoteLeaseAgentContext - remote lease agent.

Return Value:

    n/a

--*/

{
    BOOLEAN LastRequest = FALSE;
    LARGE_INTEGER Expiration;
    PVOID LeaseMessage = NULL;
    ULONG LeaseMessageSize = 0;

    NTSTATUS status;

    LARGE_INTEGER OutgoingMsgId;
    OutgoingMsgId.QuadPart = 0;

    if (LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState)
    {
        if (TRUE == IsRemoteLeaseAgentInPing(RemoteLeaseAgentContext))
        {
            //
            // if the RLA is still in the ping phase, don't send out the term message
            //
            EventWriteSendingTerminationSkippedInPing(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
                ); 

            //
            // The lease is terminated before the ping is finished, abort the ping immediately
            //
            SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);
        }
        else
        {
            EventWriteSendingTerminationSkippedSubjectInactive(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                RemoteLeaseAgentContext->InPing
                );

            AbortPingProcess(RemoteLeaseAgentContext);
        }

        return;
    }

    //
    // Check to see if we still have subjects left.
    //
    LastRequest = (0 == RtlNumberGenericTableElements(&RemoteLeaseAgentContext->SubjectHashTable));
    if (LastRequest) {

        //
        // Set the remote lease agent state to terminated.
        // When the termination ack comes back, then we can enable it again.
        //
        LAssert(OPEN == RemoteLeaseAgentContext->State || SUSPENDED == RemoteLeaseAgentContext->State);
        if (OPEN == RemoteLeaseAgentContext->State)
        {
            SetRemoteLeaseAgentState(RemoteLeaseAgentContext, SUSPENDED);
        }

        if (TRUE == IsTwoWayTermination)
        {
            RemoteLeaseAgentContext->IsInTwoWayTermination = TRUE;
        }
    }

    if (LastRequest)
    {
        Expiration.QuadPart = MAXLONGLONG;
    }
    else
    {
        GetCurrentTime(&Expiration);
        Expiration.QuadPart += (LONGLONG)RemoteLeaseAgentContext->LeaseRelationshipContext->Duration * MILLISECOND_TO_NANOSECOND_FACTOR;
    }
    //
    // Create lease request message.
    //
    SerializeLeaseMessage(
        RemoteLeaseAgentContext,
        LEASE_REQUEST,
        &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
        &RemoteLeaseAgentContext->SubjectFailedPendingHashTable,
        &RemoteLeaseAgentContext->MonitorFailedPendingHashTable,
        NULL,
        NULL,
        NULL,
        NULL,
        &RemoteLeaseAgentContext->SubjectTerminatePendingHashTable,
        NULL,
        LastRequest ? DURATION_MAX_VALUE : RemoteLeaseAgentContext->LeaseRelationshipContext->Duration,
        Expiration,
        LastRequest ? DURATION_MAX_VALUE : RemoteLeaseAgentContext->LeaseRelationshipContext->LeaseSuspendDuration,
        LastRequest ? DURATION_MAX_VALUE : RemoteLeaseAgentContext->LeaseRelationshipContext->ArbitrationDuration,
        IsTwoWayTermination,
        &LeaseMessage,
        &LeaseMessageSize
        );

    if (NULL != LeaseMessage) 
    {
        OutgoingMsgId.QuadPart = ((PLEASE_MESSAGE_HEADER) LeaseMessage)->MessageIdentifier.QuadPart;
    }

    if (LastRequest)
    {
        EventWriteLeaseRelationSendingTermination(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
            GetMessageType(LEASE_REQUEST),
            OutgoingMsgId.QuadPart
            ); 
    }
    else
    {
        EventWriteLeaseRelationSendingLease(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
            GetMessageType(LEASE_REQUEST),
            OutgoingMsgId.QuadPart
            ); 
    }

    if (NULL != LeaseMessage) {
        
        //
        // Initiate lease termination request.
        //
        AddRef(RemoteLeaseAgentContext);

        if (RemoteLeaseAgentContext->TransportBufferPending)
        {
            EventWriteTransportSendMultipleBuffers(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                ((PLEASE_MESSAGE_HEADER) RemoteLeaseAgentContext->TransportBufferPending)->MessageIdentifier.QuadPart,
                OutgoingMsgId.QuadPart
                );

            PVOID MultipleMessages[2] = { 0 };
            ULONG MultipleMessageSizes[2] = { 0 };
            TRANSPORT_MESSAGE_SENDING_CALLBACK MessageCallbacks[2] = { 0 };
            PVOID MessageCallbackStates[2] = { 0 };
            MultipleMessages[0] = RemoteLeaseAgentContext->TransportBufferPending;
            RemoteLeaseAgentContext->TransportBufferPending = NULL;
            MultipleMessageSizes[0] = RemoteLeaseAgentContext->TransportBufferPendingSize;
            RemoteLeaseAgentContext->TransportBufferPendingSize = 0;
            MultipleMessages[1] = LeaseMessage;
            MultipleMessageSizes[1] = LeaseMessageSize;
            MessageCallbacks[1] = MarkLeaseRelationshipSent;
            MessageCallbackStates[1] = RemoteLeaseAgentContext;

            status = TransportSendBuffersNotification(
                RemoteLeaseAgentContext->PartnerTarget,
                MultipleMessages,
                2,
                MultipleMessageSizes,
                MessageCallbacks,
                MessageCallbackStates,
                FALSE);
        }
        else
        {
            status = TransportSendBufferNotification(
                RemoteLeaseAgentContext->PartnerTarget,
                LeaseMessage,
                LeaseMessageSize,
                MarkLeaseRelationshipSent,
                RemoteLeaseAgentContext,
                FALSE);
        }

        if (!NT_SUCCESS(status))
        {
            Release(RemoteLeaseAgentContext);
        }
    }
}

NTSTATUS 
TerminateSubjectLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier,
    __in BOOLEAN IsSubjectFailed
    )

/*++

Routine Description:

    Terminates an existent lease relationship with a remote application.

Arguments:

    RemoteLeaseAgentContext - remote lease agent.

    LeaseRelationshipIdentifier - lease relationship identifier.

    IsSubjectFailed - specifies whether the subject has failed or is just calling terminate.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID IterLeaseRelationshipIdentifierEstablish = NULL;
    PVOID IterLeasingApplication = NULL;
    BOOLEAN Inserted = FALSE;
    BOOLEAN Deleted = FALSE;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierEstablish = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER* LeaseRelationshipIdentifierExistent = NULL;
    PRTL_GENERIC_TABLE HashTable = NULL;

    //
    // Move the lease relationship identifier from the subject list
    // to the appropriate subject failed/terminate pending list.
    //
    HashTable = (IsSubjectFailed) ? 
         &RemoteLeaseAgentContext->SubjectFailedPendingHashTable : 
         &RemoteLeaseAgentContext->SubjectTerminatePendingHashTable;
     
    LeaseRelationshipIdentifierExistent = RtlInsertElementGenericTable(
        HashTable,
        &LeaseRelationshipIdentifier,
        sizeof(PLEASE_RELATIONSHIP_IDENTIFIER),
        &Inserted
        );
    
    //
    // Check insert result.
    //
    if (NULL == LeaseRelationshipIdentifierExistent) {

        LAssert(!Inserted);

        EventWriteLeaseRelationTerminateFail2(
            NULL,
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart
            );
        //
        // Terminating lease agent due to insufficient resources.
        // This should be very rare occurence (could not allocate 4 bytes).
        //
        OnLeaseFailure(RemoteLeaseAgentContext->LeaseAgentContext);

        Status = STATUS_INSUFFICIENT_RESOURCES;
                
    } else {
    
        //
        // There should be only one copy of this element in this hash table.
        //
        LAssert(Inserted);

        if (IsSubjectFailed)
        {
            EventWriteTerminateSubLeaseSubFailed(
                NULL,
                LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart
                );
        }
        else
        {
            EventWriteTerminateSubLeaseSubTerminated(
                NULL,
                LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart
                );
        }

        //
        // Remove the lease relationship identifier from the subject list.
        //
        RtlDeleteElementGenericTable(
            &RemoteLeaseAgentContext->SubjectHashTable,
            &LeaseRelationshipIdentifier
            );
    
        //
        // Find its match in the subject pending list.
        //
        IterLeaseRelationshipIdentifierEstablish = RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
            &LeaseRelationshipIdentifier
            );

        if (NULL != IterLeaseRelationshipIdentifierEstablish) {

            //
            // The lease relationship is being terminated before it has been acknowledged.
            //
            IterLeasingApplication = RtlLookupElementGenericTable(
                &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable,
                &LeaseRelationshipIdentifier->LeasingApplicationContext
                );
            LAssert(NULL != IterLeasingApplication);

            //
            // Retrieve lease relationship identifier.
            //
            LeaseRelationshipIdentifierEstablish = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterLeaseRelationshipIdentifierEstablish);

            //
            // Remove the lease relationship identifier.
            //
            Deleted = RtlDeleteElementGenericTable(
                &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
                &LeaseRelationshipIdentifierEstablish
                );
            LAssert(Deleted);

            //
            // Deallocate lease relationship identifier.
            //
            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifierEstablish);

        }

        if (LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState)
        {
            //
            // Subject is Inactive, meaning, the lease is not established yet.
            // Remove the lease relationship identifier from termination list,
            // since the termination message will not be sent out anyway
            //
            Deleted = RtlDeleteElementGenericTable(
                HashTable,
                &LeaseRelationshipIdentifier
                );
            LAssert(Deleted);

            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);

        }

    }

    return Status;
}

BOOLEAN
SendingFailedTerminationRequest(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )
{
    if (0 == RtlNumberGenericTableElements(&RemoteLeaseAgentContext->SubjectHashTable) && 
        0 == RtlNumberGenericTableElements(&RemoteLeaseAgentContext->MonitorHashTable))
    {
        return TRUE;
    }

    return FALSE;
}

NTSTATUS 
TerminateAllLeaseRelationships(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in BOOLEAN isSubjectFailed
    )

/*++

Routine Description:

    Terminates all lease relationships for this leasing application.

Arguments:

    LeasingApplicationIdentifier - leasing application identifier.

    LeasingApplicationContext - leasing application identifier.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID IterLeaseIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    BOOLEAN Restart = TRUE;

    //
    // If the SubjectHashTable is empty, we don't need to send out a request message
    // Only monitor hash table and monitor failed pending list would be updated in this case
    // 
    BOOLEAN SendMsg = FALSE;
    //
    // If both subject and monitor hash table is emtpy, the two way leases are terminated at once
    // Monitor timer will be updated beforehand since the remote side will update the subject timer once recived the request
    //
    BOOLEAN IsTwoWayTermination = FALSE;
    
    //
    // If Remote lease agent is in ping process, stop the ping process and ping retry timer will also stop.     
    //
    if (TRUE == IsRemoteLeaseAgentInPing(RemoteLeaseAgentContext))
    {
        AbortPingProcess(RemoteLeaseAgentContext);
    }
    
    //
    // Only do the TerminateMonitorLease if isSubjectFailed is true
    // When remote side process MonitorFailedList, it would raise REMOTE_EXPIRED event
    // For immediate regular termination (rather than delayed failed termination), we can't use MonitorFailedList for correctness
    //
    if (TRUE == isSubjectFailed)
    {
        //
        // Iterate over the monitor list and see which lease relationship
        // identifier contains this leasing application.
        //
        IterLeaseIdentifier = RtlEnumerateGenericTable(
            &RemoteLeaseAgentContext->MonitorHashTable, 
            TRUE
            );
    
        while (NULL != IterLeaseIdentifier) {

            //
            // Retrieve the lease identifier.
            //
            LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterLeaseIdentifier);

            //
            // Match the lease identifier with the provided remote leasing application.
            //
            if (0 == wcscmp(
                LeasingApplicationContext->LeasingApplicationIdentifier,
                LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier)
                ) {
        
                //
                // Terminate monitor lease relationship.
                //
                Status = TerminateMonitorLease(
                    RemoteLeaseAgentContext,
                    LeaseRelationshipIdentifier
                    );

                if (!NT_SUCCESS(Status))
                {
                    break;
                }

                //
                // Start from the beginning of the hash table.
                //
                Restart = TRUE;
            
            } else {

                //
                // Continue from current position in the hash table.
                //
                Restart = FALSE;
            }

            //
            // Move on to the next element.
            //
            IterLeaseIdentifier = RtlEnumerateGenericTable(
                &RemoteLeaseAgentContext->MonitorHashTable, 
                Restart
                );
        }

        if (!NT_SUCCESS(Status)) {
        
            //
            // Done.
            //
            return Status;
        }
    }

    //
    // Iterate over the subject list and see which lease relationship
    // identifier contains this leasing application.
    //
    IterLeaseIdentifier = RtlEnumerateGenericTable(
        &RemoteLeaseAgentContext->SubjectHashTable, 
        TRUE
        );
    
    while (NULL != IterLeaseIdentifier) {

        //
        // Retrieve the lease identifier.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterLeaseIdentifier);

        //
        // Match the lease identifier with the provided leasing application.
        //
        if (0 == wcscmp(
            LeasingApplicationContext->LeasingApplicationIdentifier,
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier)
            ) {
        
            //
            // Initiate terminate subject lease relationship.
            //
            Status = TerminateSubjectLease(
                RemoteLeaseAgentContext,
                LeaseRelationshipIdentifier,
                isSubjectFailed
                );

            if (!NT_SUCCESS(Status)) {
                
                //
                // Done.
                //
                break;
            }

            //
            // There is at lease one subject lease relationship existing for this leasing appliaction
            // A renew/termination request message need to be sent out
            //
            if (LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState)
            {
                //
                // Only send out the termination message when the subject is ACTIVE
                // This scenario may happen in the Ping process, 
                // The subjectPendlist is not empty, but the subject is Inactive
                // The Monitor may be Active when the remote side finish the ping first
                //
                SendMsg = TRUE;
            }

            //
            // Start from the beginning of the hash table.
            //
            Restart = TRUE;
            
        } else {

            //
            // Continue from current position in the hash table.
            //
            Restart = FALSE;
        }

        //
        // Move on to the next element.
        //
        IterLeaseIdentifier = RtlEnumerateGenericTable(
            &RemoteLeaseAgentContext->SubjectHashTable, 
            Restart
            );
    }
    
    if (NT_SUCCESS(Status) && TRUE == SendMsg)
    {
        //
        // If there is no subject and monitor left or we need to terminate all without unregister delay
        // A two way termination message will be sent out
        // monitor timer is updated in advance
        //
        IsTwoWayTermination = SendingFailedTerminationRequest(RemoteLeaseAgentContext) || !isSubjectFailed;
        TerminateSubjectLeaseSendMessage(RemoteLeaseAgentContext, IsTwoWayTermination);

        if (TRUE == IsTwoWayTermination && LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        {
            EventWriteInitiateTwoWayTermination(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart);

            UpdateMonitorTimerForTermination(RemoteLeaseAgentContext);

            //
            // Monitor is set to Inactive and timer is dequeued
            // Lease is essentially one-way now
            // subject  timer and subject expire/fail time need to be updated
            //
            SetSubjectExpireTime(
                RemoteLeaseAgentContext->LeaseRelationshipContext,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime,
                RemoteLeaseAgentContext->LeaseRelationshipContext->LeaseSuspendDuration,
                RemoteLeaseAgentContext->LeaseRelationshipContext->ArbitrationDuration
                );
        
            //
            // Enqueue the subject timer.
            //
            ArmTimer(
                RemoteLeaseAgentContext,
                &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimer,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime,
                &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimerDpc
                );
        }
    }

    return Status;
}

VOID 
ArbitrateLease(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext,
__in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
__in LONG LocalTimeToLive,
__in LONG RemoteTimeToLive,
__in BOOLEAN IsDelayed
)

/*++

Routine Description:

Terminates all lease relationships for this leasing application.

Arguments:

RemoteLeaseAgentContext - remote lease agent.

TimeToLive - time to live of the remote lease agent.

Return Value:

n/a

--*/

{
    LARGE_INTEGER ExpireTime;
    LARGE_INTEGER Now;
    GetCurrentTime(&Now);
    UNREFERENCED_PARAMETER(LeaseAgentContext);

    EventWriteLeaseRelationRcvArbitrationTTL(
        NULL,
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
        LocalTimeToLive,
        RemoteTimeToLive,
        IsDelayed
        );

    //
    // Since the arbitration result came back, we clear the arbitration timer.
    //
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->RenewOrArbitrateTimer
        );

    if (MAXLONG != LocalTimeToLive)
    {

        //
        // Arbitration failed.
        //
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_FAILED;
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_INACTIVE;
        
        //
        // Set the IsInArbitrationNeutral back, we could have neutral arbitration result earlier
        //
        RemoteLeaseAgentContext->IsInArbitrationNeutral = FALSE;

        //
        // Signal lease failure.
        //
        OnLeaseFailure(RemoteLeaseAgentContext->LeaseAgentContext);
    }
    else if (MAXLONG != RemoteTimeToLive)
    {
        //
        // Allow ping immediately
        //
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectSuspendTime.QuadPart = MAXLONGLONG;
        
        //
        // Set the IsInArbitrationNeutral back, we could have neutral arbitration result earlier
        //
        RemoteLeaseAgentContext->IsInArbitrationNeutral = FALSE;

        if (IsDelayed){
            //
            // Inform applications that the remote lease agent has failed.
            //
            InformLeasingAplicationsRemoteLeaseAgentFailed(
                RemoteLeaseAgentContext,
                FALSE
                );
            InformLeasingAplicationsRemoteLeaseAgentFailed(
                RemoteLeaseAgentContext,
                TRUE
                );
        }
        else if (0 == RemoteTimeToLive) {

            //
            // No remaining lifetime, set the other party as shutdown immediately.    
            //
            OnMonitorExpired(RemoteLeaseAgentContext);

        }
        else {
            GetCurrentTime(&ExpireTime);
            LARGE_INTEGER_ADD_INTEGER(
                ExpireTime,
                RemoteTimeToLive * MILLISECOND_TO_NANOSECOND_FACTOR
                );
            //
            // Set the node to shutdown when its remaining life time is expired.
            //
            EnqueueTimer(
                RemoteLeaseAgentContext,
                &RemoteLeaseAgentContext->LeaseRelationshipContext->PostArbitrationTimer,
                ExpireTime,
                &RemoteLeaseAgentContext->LeaseRelationshipContext->PostArbitrationTimerDpc
                );
        }
    }
    else
    {
        //
        // The lease relationship has failed.
        //
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_INACTIVE;
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_FAILED;

        //
        // Set the IsInArbitrationNeutral flag, so the maintenance thread would not destruct, GetRemoteLeaseExpirationTime would be able to query
        //
        RemoteLeaseAgentContext->IsInArbitrationNeutral = TRUE;

        //
        // Signal lease failure.
        //
        SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);
    }
}

VOID 
GetCurrentTime(
    __out PLARGE_INTEGER  CurrentTime
    )

/*++

Routine Description:

    Get time by query system counter and frequency

Arguments:

    CurrentTime - output argument.

Return Value:

    n/a

--*/

{
    LARGE_INTEGER TimeStamp;
    LARGE_INTEGER Frequency;

    TimeStamp = KeQueryPerformanceCounter(&Frequency);
    //
    // The following equation is used to avoid the floating point math; Precision is reduced to milliseconds 
    //
    CurrentTime->QuadPart = TimeStamp.QuadPart * 1000 / Frequency.QuadPart * MILLISECOND_TO_NANOSECOND_FACTOR;

}


NTSTATUS
SendIndirectLeaseForwardMessages(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextDirect
    )

/*++
Description:
    - LeaseAgentContext lock is already taken when this function is called
    - Search through the RLA table and send out a request/termination for each active RLA
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID IterRemoteLeaseAgent = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PLEASE_AGENT_CONTEXT LeaseAgentContext = RemoteLeaseAgentContextDirect->LeaseAgentContext;
    BOOLEAN IsTermination = (0 == RtlNumberGenericTableElements(&RemoteLeaseAgentContextDirect->SubjectHashTable));
    PVOID LeaseMessage = NULL;
    ULONG LeaseMessageSize = 0;

    LARGE_INTEGER Now;
    LARGE_INTEGER NewExpiration;
    LARGE_INTEGER TerminateExpiration;
    LARGE_INTEGER OutgoingMsgId;

    TerminateExpiration.QuadPart = MAXLONGLONG;
    OutgoingMsgId.QuadPart = 0;

    EventWriteLeaseRenewRetry(
        NULL,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContextDirect->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContextDirect->Instance.QuadPart,
        RemoteLeaseAgentContextDirect->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContextDirect->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContextDirect->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContextDirect->LeaseRelationshipContext->MonitorState),
        IsTermination
        );

    //
    // Send indirect lease forward messages to all neighbors
    // Lease agent lock is already set in the caller function
    //
    for (IterRemoteLeaseAgent = RtlEnumerateGenericTable(&LeaseAgentContext->RemoteLeaseAgentContextHashTable, TRUE);
         NULL != IterRemoteLeaseAgent;
         IterRemoteLeaseAgent = RtlEnumerateGenericTable(&LeaseAgentContext->RemoteLeaseAgentContextHashTable, FALSE))
    {

        RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

        // Forward request is best effort, we will only use active RLA
        // Subject and monitor active check is for RLA in ping stage
        if (IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext) || 
            IsRemoteLeaseAgentSuspended(RemoteLeaseAgentContext) ||
            FALSE == RemoteLeaseAgentContext->IsActive ||
            LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState ||
            LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState
            )
        {
            continue;
        }

        if (RemoteLeaseAgentContext == RemoteLeaseAgentContextDirect)
        {
            EventWriteSendForwardMsgForSameRLASkipped(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                GetLeaseAgentState(RemoteLeaseAgentContext->State)
                ); 

            continue;
        }

        if (FALSE == IsTermination)
        {
            //
            // Create regular lease renew request message.
            //
            GetCurrentTime(&Now);

            NewExpiration.QuadPart = Now.QuadPart + (LONGLONG)RemoteLeaseAgentContextDirect->LeaseRelationshipContext->Duration * MILLISECOND_TO_NANOSECOND_FACTOR;

            SerializeLeaseMessage(
                RemoteLeaseAgentContextDirect,
                FORWARD_REQUEST,
                &RemoteLeaseAgentContextDirect->SubjectEstablishPendingHashTable,
                &RemoteLeaseAgentContextDirect->SubjectFailedPendingHashTable,
                &RemoteLeaseAgentContextDirect->MonitorFailedPendingHashTable,
                NULL,
                NULL,
                NULL,
                NULL,
                &RemoteLeaseAgentContextDirect->SubjectTerminatePendingHashTable,
                NULL,
                RemoteLeaseAgentContextDirect->LeaseRelationshipContext->Duration,
                NewExpiration,
                RemoteLeaseAgentContextDirect->LeaseRelationshipContext->LeaseSuspendDuration,
                RemoteLeaseAgentContextDirect->LeaseRelationshipContext->ArbitrationDuration,
                FALSE,
                &LeaseMessage,
                &LeaseMessageSize
                );

            if (NULL != LeaseMessage) 
            {
                OutgoingMsgId.QuadPart = ((PLEASE_MESSAGE_HEADER) LeaseMessage)->MessageIdentifier.QuadPart;
            }

            EventWriteLeaseRelationSendingLease(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                GetMessageType(FORWARD_REQUEST),
                OutgoingMsgId.QuadPart
                );

        }
        else
        {
            //
            // Create lease renew terminate request message.
            //
            SerializeLeaseMessage(
                RemoteLeaseAgentContextDirect,
                FORWARD_REQUEST,
                &RemoteLeaseAgentContextDirect->SubjectEstablishPendingHashTable,
                &RemoteLeaseAgentContextDirect->SubjectFailedPendingHashTable,
                &RemoteLeaseAgentContextDirect->MonitorFailedPendingHashTable,
                NULL,
                NULL,
                NULL,
                NULL,
                &RemoteLeaseAgentContextDirect->SubjectTerminatePendingHashTable,
                NULL,
                DURATION_MAX_VALUE,
                TerminateExpiration,
                DURATION_MAX_VALUE,
                DURATION_MAX_VALUE,
                FALSE,
                &LeaseMessage,
                &LeaseMessageSize
                );

            if (NULL != LeaseMessage) 
            {
                OutgoingMsgId.QuadPart = ((PLEASE_MESSAGE_HEADER) LeaseMessage)->MessageIdentifier.QuadPart;
            }
            EventWriteLeaseRelationSendingTermination(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                GetMessageType(FORWARD_REQUEST),
                OutgoingMsgId.QuadPart
                );
        }
        //
        // Send request if it was successfully created.
        //
        if (NULL != LeaseMessage)
        {
            AddRef(RemoteLeaseAgentContext);

            Status = TransportSendBufferNotification(
                RemoteLeaseAgentContext->PartnerTarget,
                LeaseMessage,
                LeaseMessageSize,
                MarkLeaseRelationshipSent,
                RemoteLeaseAgentContext,
                FALSE);

            if (!NT_SUCCESS(Status))
            {
                Release(RemoteLeaseAgentContext);
            }
        }
    }
    
    return Status;

}


NTSTATUS
GetListenEndPointSize(
    __in PTRANSPORT_LISTEN_ENDPOINT ListenEndPoint,
    __in PULONGLONG ListenEndpointAddressSize,
    __in PULONG ListenEndpointSize)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (NULL == ListenEndPoint)
    {
        Status = STATUS_INVALID_PARAMETER;
        EventWriteLeaseDriverTextTraceError(NULL, L"Input Endpoint is NULL", Status);
        return Status;
    }
    //
    // Get the end point address length
    //

    Status = RtlStringCchLengthW(
        ListenEndPoint->Address,
        ENDPOINT_ADDR_CCH_MAX,
        (size_t *)ListenEndpointAddressSize
        );

    if (!NT_SUCCESS(Status))
    {
        // Lease message has not been allocated yet, just return the error status
        EventWriteLeaseDriverTextTraceError(NULL, L"Address lengh checked failed", Status);
        return Status;
    }

    *ListenEndpointAddressSize += 1;
    *ListenEndpointAddressSize *= sizeof(WCHAR);

    *ListenEndpointSize = (ULONG)(*ListenEndpointAddressSize)
        + sizeof(ADDRESS_FAMILY) // size of ResolveType
        + sizeof(USHORT); // size of port 

    return Status;
}
