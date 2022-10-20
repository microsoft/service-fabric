

/*++
   (c) 2011 by Microsoft Corp.  All Rights Reserved.

   knetfaultinjector.cpp

   Description:

     Kernel Template Library (KTL)

     Implementation

   History:
     raymcc        20-Apr-2011        Initial version
     jhavens       20-Apr-2010        _ObjectDelayEngine

--*/

#include <ktl.h>

#define FAULT_TABLE_SIZE 123


VOID
KNetFaultInjector::Activate(
    __in LONG SecondsDelay,
    __in LONG SecondsDuration
    )
{
    LONGLONG Now = KNt::GetSystemTime();

    _StartTimestamp = Now + (SecondsDelay * 10000000);
    if (SecondsDuration != -1)
    {
        _StopTimestamp = _StartTimestamp + (SecondsDuration * 10000000);
    }
    else
    {
        _StopTimestamp = -1;
    }
}

// BUG, raymcc, xxxxx, Passing a KAllocator should not be needed as GetThisAllocator() should enough
KNetFaultInjector::KNetFaultInjector(
    __in KAllocator& Alloc
    )
    :   _MessageTable(Alloc),
        _UriTable(Alloc)
{
    // Initialize the various variables.
    Zero();

    NTSTATUS Res;
    _Alloc = &Alloc;

    // Check hash table initialization.
    //
    Res = _MessageTable.Initialize(FAULT_TABLE_SIZE, K_DefaultHashFunction);
    if (!NT_SUCCESS(Res))
    {
        SetConstructorStatus(Res);
        return;
    }

    Res = _UriTable.Initialize(FAULT_TABLE_SIZE, K_DefaultHashFunction);
    if (!NT_SUCCESS(Res))
    {
        SetConstructorStatus(Res);
        return;
    }

}


NTSTATUS
KNetFaultInjector::RegisterMessageDelay(
    __in KWString& DestinationUri,
    __in GUID* MessageId,
    __in ULONG Milliseconds,
    __in ULONG Ratio
    )
{
    NTSTATUS Res = STATUS_UNSUCCESSFUL;

    if (_StartTimestamp)
    {
        return STATUS_LOCK_NOT_GRANTED;
    }

    if (DestinationUri.Length())
    {
        _FaultDef* NewFault = _new (KTL_TAG_NET, *_Alloc) _FaultDef(GetThisAllocator());

        NewFault->_Uri = DestinationUri;
        NewFault->_DelayBy = Milliseconds;
        NewFault->_Delay = TRUE;
        NewFault->_Ratio = Ratio;

        if (MessageId)
        {
            NewFault->_MessageId = *MessageId;
        }


        Res = _UriTable.Put(DestinationUri, NewFault);
        if (!NT_SUCCESS(Res))
        {
            return Res;
        }
    }
    else if (MessageId)
    {
        _FaultDef* NewFault = _new (KTL_TAG_NET, *_Alloc) _FaultDef(GetThisAllocator());

        NewFault->_MessageId = *MessageId;
        NewFault->_DelayBy = Milliseconds;
        NewFault->_Ratio = Ratio;

        Res = _MessageTable.Put(*MessageId, NewFault);
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    return Res;
}


NTSTATUS
KNetFaultInjector::RegisterMessageDrop(
    __in KWString& DestinationUri,
    __in GUID* MessageId,
    __in ULONG Ratio
    )
{
    NTSTATUS Res = STATUS_UNSUCCESSFUL;

    if (_StartTimestamp)
    {
        return STATUS_LOCK_NOT_GRANTED;
    }

    if (DestinationUri.Length())
    {
        _FaultDef* NewFault = _new (KTL_TAG_NET, *_Alloc) _FaultDef(GetThisAllocator());

        NewFault->_Uri = DestinationUri;
        NewFault->_Drop = TRUE;
        NewFault->_Ratio = Ratio;

        if (MessageId)
        {
            NewFault->_MessageId = *MessageId;
        }


        Res = _UriTable.Put(DestinationUri, NewFault);
        if (!NT_SUCCESS(Res))
        {
            return Res;
        }
    }
    else if (MessageId)
    {
        _FaultDef* NewFault = _new (KTL_TAG_NET, *_Alloc) _FaultDef(GetThisAllocator());

        NewFault->_MessageId = *MessageId;
        NewFault->_Drop = TRUE;
        NewFault->_Ratio = Ratio;

        Res = _MessageTable.Put(*MessageId, NewFault);
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    return Res;
}

VOID
KNetFaultInjector::UnregisterAll()
{
    NTSTATUS Res;
    _UriTable.Reset();
    _MessageTable.Reset();

    for (;;)
    {
        KWString Uri(GetThisAllocator());
        _FaultDef* Fault;

        Res = _UriTable.Next(Uri, Fault);
        if (Res == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        _delete(Fault);
    }

    for (;;)
    {
        GUID g;
        _FaultDef* Fault;

        Res = _MessageTable.Next(g, Fault);
        if (Res == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        _delete(Fault);
    }

    _UriTable.Clear();
    _MessageTable.Clear();
}



//  KNetFaultInjector::Zero
//
//
VOID
KNetFaultInjector::Zero()
{
    _Alloc = nullptr;

    _MessageTable.Clear();
    _UriTable.Clear();

    _AllTraffic = 0;
    _Dropped = 0;
    _Delayed = 0;

    // Drop/delay all message types.
    //
    _UniversalDelayRatio = 0;
    _UniversalDropRatio = 0;
    _UniversalDelayMSec = 0;

    _UniversalDropInForce = FALSE;
    _UniversalDelayInForce = FALSE;

    _StartTimestamp = _StopTimestamp = 0;
}

VOID
KNetFaultInjector::Randomize(
    __in ULONG DropRatio,
    __in ULONG DelayRatio,
    __in ULONG DelayMilliseconds
    )
{
    _UniversalDropInForce = TRUE;
    _UniversalDelayInForce  = TRUE;

    _UniversalDropRatio = DropRatio;
    _UniversalDelayRatio = DelayRatio;

    _UniversalDelayMSec = DelayMilliseconds;
}


KNetFaultInjector::~KNetFaultInjector()
{
    UnregisterAll();
}

//  KNetFaultInjector::DropOrDelayThis
//
//
inline NTSTATUS
KNetFaultInjector::DropOrDelayThis(
    __in  _FaultDef* Fault,
    __out ULONG& Delay
        )
{
    if ((Fault->_Traffic % (Fault->_Ratio+1)) == 0)
    {
        if (Fault->_Drop)
        {
            _Dropped++;
            return STATUS_CANCELLED;
        }
        else if (Fault->_Delay)
        {
            Delay = Fault->_DelayBy;
            _Delayed++;
            return STATUS_WAIT_1;
        }
    }
    return STATUS_SUCCESS;
}

//
//  KNetFaultInjector::TestMessage
//
NTSTATUS
KNetFaultInjector::TestMessage(
    __in KWString& DestUri,
    __in GUID& MessageId,
    __out ULONG& Delay
    )
{
    // See if the message is within the fault injection time window.
    //
    LONGLONG Now = KNt::GetSystemTime();

    if (Now < _StartTimestamp || _StartTimestamp == 0)
    {
        return STATUS_SUCCESS;
    }
    if (_StopTimestamp != -1 && Now > _StopTimestamp)
    {
        return STATUS_SUCCESS;
    }

    // If here, the message nees to be checked, as we are 'active'.
    //
    _AllTraffic++;

    // See if there is a generic delay fault in place.
    //
    if (_UniversalDelayInForce)
    {
        if (_AllTraffic % ( _UniversalDelayRatio+1 ) == 0)
        {
            _Delayed++;
            Delay = _UniversalDelayMSec;
            return STATUS_WAIT_1;
        }
    }

    // See if there is a generic drop request in place.
    //
    if (_UniversalDropInForce)
    {
        if (_AllTraffic % (_UniversalDropRatio+1) == 0)
        {
            _Dropped++;
            return STATUS_CANCELLED;
        }
    }

    // If here, we have to check for URI or Message-Id-specific faults.
    //
    _FaultDef* MessageFault = nullptr;
    _FaultDef* UriFault = nullptr;

    // Read the relevant records from hash tables, if there are any.
    //
    // These are not currently protected by a lock under the assumption
    // that it is not legal to change the set of faults while executing.
    //
    NTSTATUS IdCheck   = _MessageTable.Get(MessageId, MessageFault);
    NTSTATUS UriCheck  = _UriTable.Get(DestUri, UriFault);

    // Check for a Message ID match.
    //
    if (NT_SUCCESS(IdCheck))
    {
        MessageFault->_Traffic++;
        if (MessageFault->_Uri.Length() == 0 ||
            MessageFault->_Uri == DestUri
            )
        {
            return DropOrDelayThis(MessageFault, Delay);
        }
    }

    // Check for a destination URI match
    //
    if (NT_SUCCESS(UriCheck))
    {
        UriFault->_Traffic++;
        if (UriFault->_Uri == DestUri)
        {
            return DropOrDelayThis(UriFault, Delay);
        }
    }

    // If here, no match.  Deliver the message to the lucky recipient.
    //
    return STATUS_SUCCESS;
}


const ULONG _ObjectDelayEngine::Element::EntryOffset =
        FIELD_OFFSET( _ObjectDelayEngine::Element, _Entry );

_ObjectDelayEngine::_ObjectDelayEngine(
    KAllocator& Allocator,
    KDelegate<VOID(PVOID Msg)> Callout
    ) : _Callout(Callout),
    _Allocator(Allocator),
    _Active(TRUE),
    _ElementList(_ObjectDelayEngine::Element::EntryOffset)
/*++
 *
 * Routine Description:
 *      This routine is the constructor for the object delay engine.  It intializes
 *      the internal timer object.
 *
 * Arguments:
 *      Allocator - Heap allocator to be used.
 *      Callout - Indicates the function to call with the message after the specified
 *      delay has occured.
 *
 * Return Value:
 *      None
 *
 * Note:
 *
-*/

{
#if KTL_USER_MODE
    _TimerPtr = CreateThreadpoolTimer(&TimerCallback, (PVOID)this, nullptr);
    if (_TimerPtr == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
    }
#else
    KeInitializeTimer(&_KTimer);
    KeInitializeDpc(&_KDpc, &DpcCallback, (PVOID)this);
#endif
}

NTSTATUS
_ObjectDelayEngine::DelayThis(
    PVOID Msg,
    ULONG MillisecondsToDelay
    )
/*++
 *
 * Routine Description:
 *      This functin inserts a element into the delay queue.  Elements are stored in
 *      the queue in expiration order.
 *
 * Arguments:
 *     Msg - Supplies the pointer to the message or context to be delivered via the
 *          callout when the delay time has elapsed.
 *
 *      MillisecondsToDelay - Specifies the number of milliseconds to delay the
 *          message.
 *
 * Return Value:
 *      Returns the status of the operation and error is returned if the operation
 *      whas not execepted for some reason.
 *
 * Note:
 *
-*/
{
    Element *NewEl;
    Element *El;
    ULONG RemainingTime = 0;

    NewEl = _new(KTL_TAG_NET, _Allocator) Element(Msg, MillisecondsToDelay);

    if (NewEl == nullptr)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    K_LOCK_BLOCK(_Lock)
    {
        LONGLONG CurrentTime = KNt::GetSystemTime();

        if (!_Active)
        {
            return(STATUS_FILE_CLOSED);
        }

        //
        //  There are basically two cases.  Either the element will go at the head of
        //  the list and the timer will be effected or it will go after the first
        //  element and the timer will not be effected.
        //

        El = _ElementList.PeekHead();

        if (El != nullptr)
        {
            //
            // Calculate the remaining time for until the first element is
            // dispatched.
            //

            RemainingTime = (ULONG) ((CurrentTime - _TimerStart)/10000i64);

            if (RemainingTime > El->_Delay)
            {
                RemainingTime = 0;
            }
            else
            {
                RemainingTime = El->_Delay - RemainingTime;
            }

            if (RemainingTime > NewEl->_Delay)
            {
                //
                // The new element will be put at the head of the list.  Adjust the
                // time remaining for the current head.
                //

                El->_Delay = RemainingTime - NewEl->_Delay;
            }
        }

        if (El == nullptr || RemainingTime > NewEl->_Delay )
        {
            //
            // The new element is going at the head of the list.  Start or restart
            // the timer with the NewEl delay and return.
            //

            _ElementList.InsertHead(NewEl);
            SetTimeout(NewEl->_Delay);
            _TimerStart = CurrentTime;
            break;
        }

        //
        // Look through the list looking where the new element should be inserted.
        //

        NewEl->_Delay -= RemainingTime;
        while((El = _ElementList.Successor(El)) != nullptr)
        {
            if (NewEl->_Delay < El->_Delay)
            {
                break;
            }

            NewEl->_Delay -= El->_Delay;
        }

        if (El == nullptr)
        {
            //
            // Place the new element at the end of the list.
            //

            _ElementList.AppendTail(NewEl);
        }
        else
        {
            //
            // Split the time and inserted the new element.
            //

            El->_Delay -= NewEl->_Delay;
            _ElementList.InsertBefore(NewEl, El);

        }
    }

    return(STATUS_SUCCESS);
}

VOID
_ObjectDelayEngine::StopAndDrain()
/*++
 *
 * Routine Description:
 *      This routine stops the timer and delievers all of the queue messages.  New
 *      messages are rejected.  This routine can be used as part of shutdown the
 *      network.
 *
 * Arguments:
 *      None
 *
 * Return Value:
 *      None
 *
 * Note:
 *      Note this routine does not guarantee that the DPC routine has completed
 *      before returning, but it should not cause trouble.
-*/

{
    KNodeList<Element> LocalElementList(_ObjectDelayEngine::Element::EntryOffset);
    Element *El;

    K_LOCK_BLOCK(_Lock)
    {
        if (_Active)
        {
            _Active = FALSE;
        }
        else
        {
            return;
        }
    }

#if KTL_USER_MODE
    if (_TimerPtr != nullptr)
    {
        SetThreadpoolTimer(_TimerPtr, nullptr, 0, 0);
        WaitForThreadpoolTimerCallbacks(_TimerPtr, TRUE);
    }
#else
   KeCancelTimer(&_KTimer);
#endif

   K_LOCK_BLOCK(_Lock)
   {

       while((El = _ElementList.RemoveHead()) != nullptr)
       {
           LocalElementList.AppendTail(El);
       }
   }

   while((El = LocalElementList.RemoveHead()) != nullptr)
   {
       _Callout(El->_Msg);
       _delete(El);
   }
}

_ObjectDelayEngine::~_ObjectDelayEngine()
/*++
 *
 * Routine Description:
 *      This routine is the destructor for the object delay engine.  It will stop and
 *      drain the engine if it has not been done previously.
 *
 * Arguments:
 *      None
 *
 * Return Value:
 *      None
 *
 * Note:
 *
-*/
{
    StopAndDrain();
#if KTL_USER_MODE
    if (_TimerPtr != nullptr)
    {
        CloseThreadpoolTimer(_TimerPtr);
        _TimerPtr = nullptr;
    }
#endif
}

#if KTL_USER_MODE
VOID CALLBACK
_ObjectDelayEngine::TimerCallback(
    __inout PTP_CALLBACK_INSTANCE   Instance,
    __inout_opt PVOID               Context,
    __inout PTP_TIMER               Timer
    )
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Timer);
    
    _ObjectDelayEngine* thisPtr = (_ObjectDelayEngine*)Context;

    thisPtr->ProcessTimeout();
}
#else
VOID
_ObjectDelayEngine::DpcCallback(
    _In_ struct _KDPC *Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);
    _ObjectDelayEngine* thisPtr = (_ObjectDelayEngine*)DeferredContext;

    thisPtr->ProcessTimeout();
}
#endif

VOID
_ObjectDelayEngine::ProcessTimeout()
/*++
 *
 * Routine Description:
 *      This routine does the processing when the timout timer completes.
 *
 * Arguments:
 *      None
 *
 * Return Value:
 *      None
 *
 * Note:
 *
-*/
{
    Element *El = nullptr;
    Element *ElNext;

    K_LOCK_BLOCK(_Lock)
    {
        if (!_Active)
        {
            return;
        }

        El = _ElementList.RemoveHead();

        ElNext =  _ElementList.PeekHead();
        if (ElNext != nullptr)
        {
            SetTimeout(ElNext->_Delay);
            _TimerStart = KNt::GetSystemTime();
        }
    }

    _Callout(El->_Msg);
    _delete(El);
}

VOID
_ObjectDelayEngine::SetTimeout(
    ULONG Delay
    )
/*++
 *
 * Routine Description:
 *      This routine starts or restarts the timer.
 *
 * Arguments:
 *      Delay - Specifies the timeout value in milliseconds.
 *
 * Return Value:
 *      None
 *
 * Note:
 *
-*/

{
    LARGE_INTEGER ticksToWait;
    ticksToWait.QuadPart = -1 * (((ULONGLONG)Delay) * ((LONGLONG)10000));     // To100NanoTicksFromMSecs()

#if KTL_USER_MODE
    FILETIME t;
    t.dwHighDateTime = ticksToWait.u.HighPart;
    t.dwLowDateTime = ticksToWait.u.LowPart;
    SetThreadpoolTimer(_TimerPtr, &t, 0, 0);
#else
    KeSetTimer(&_KTimer, ticksToWait, &_KDpc);
#endif

}
_ObjectDelayEngine::Element::Element(
    PVOID Msg,
    ULONG MillisecondsToDelay
    ) : _Msg(Msg),
        _Delay(MillisecondsToDelay)
{

}


