

/*++
   (c) 2011 by Microsoft Corp.  All Rights Reserved.

   knetfaultinjector.h

   Description:

     Kernel Template Library (KTL)

     Defines a class which simulates network faults
     in order to exercise additional code paths for components
     which depend on KNetwork.

   History:
     raymcc        20-Apr-2011        KNetFaultInjector
     jhavens       20-Apr-2010        ObjectDelayEngine

--*/


#pragma once


class KNetFaultInjector : public KObject<KNetFaultInjector>, public KShared<KNetFaultInjector>
{
    friend class VNetwork;

public:
    typedef KSharedPtr<KNetFaultInjector> SPtr;

    // RegisterMessageDrop
    //
    // Drops a network message, preventing it from being delivered.
    //
    // Parameters:
    //    DestinationUri        The destination endpoint. If an empty string, then all
    //                          messages of the specified type are dropped, regardless of destination.
    //                          If this is an empty string, then MessageId cannot be null.
    //
    //    MessageId             Pointer to the id of the message to be dropped (the KTL Serialization GUID).
    //                          If NULL, all messages to the destination URI are subject to dropping.
     //                          If this is NULL, DestinationUri cannot be empty.
    //
    //    Ratio                 The ratio of delivered messages to dropped messages.  If this is 10,
    //                          10 messages will be delivered for every 1 dropped.  A value of 1 means
    //                          every other message is dropped.  A value of 0 means drop all occurrences.
    //
    //    This cannot be called while faults are 'active'.  You must first either let the activation specified in Activate() expire or
    //    call Deactivate() to register new faults.
    //
    //  Return values:
    //      STATUS_SUCCESS
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_LOCK_NOT_GRANTED             If faults are currently active.
    //
    //
    NTSTATUS
    RegisterMessageDrop(
        __in KWString& DestinationUri,
        __in GUID* MessageId,
        __in ULONG Ratio
        );


    // RegisterMessageDelay
    //
    // Delays delivery of a message.
    //
    // Parameters:
    //    DestinationUri        The destination endpoint. If an empty string, then all
    //                          messages of the specified type are delayed, regarldess of destination.
    //                          If this is an empty string, then MessageId cannot be null.
    //
    //    MessageId             Pointer to the id of the message to be delayed (the KTL Serialization GUID).
    //                          If NULL, a message of any type is randomly selected for delay.
    //                          If NULL, then DestinationUri cannot be an empty string.
    //
    //    Ratio                 The ratio of delivered messages to delayed messages.  If this is 10,
    //                          10 messages will be delivered immediately for every 1 delayed.  A value of 1 means
    //                          every other message is delayed  A value of 0 means delay all occurrences.
    //
    //    This cannot be called while faults are 'active'.  You must first either let the activation specified in Activate() expire or
    //    call Deactivate() to register new faults.
    //
    //  Return values:
    //      STATUS_SUCCESS
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_LOCK_NOT_GRANTED             If faults are currently active.
    //
    NTSTATUS
    RegisterMessageDelay(
        __in KWString& DestinationUri,
        __in GUID* MessageId,
        __in ULONG Milliseconds,
        __in ULONG Ratio
        );

    // Activate
    //
    // Activates all the currently registered faults, after the
    // specified delay.  Once active, it is not possible to register new faults.
    // New faults can be registered by allowing the active period to expire or else
    // by callilng Deactivate() at any time.
    //
    // Parameters:
    //      SecondsDelay                How many seconds to wait before the faults become active.
    //                                  If zero, faults begin immediately.
    //
    //      SecondsDuration             How many seconds the faults should be in force.  After this expires,
    //                                  normal delivery occurs. If -1, then faults continue until Deactivate() is called.
    //
    //
    VOID
    Activate(
        __in LONG SecondsDelay,
        __in LONG SecondDuration
        );


    // Randomize
    //
    // Introduces random drops/delays/corruptions using the specified ratio.
    //
    VOID
    Randomize(
        __in ULONG DropRatio,
        __in ULONG DelayRatio,
        __in ULONG DelayMilliseconds
        );

    // Deactivate
    //
    // Turns off all faults and resumes full speed normal delivery.
    //
    VOID
    Deactivate()
    {
        _StartTimestamp = 0;    // Special-case value disables injection
    }

    // UnregisterAll
    //
    // Removes all fault definitions.
    //
    VOID
    UnregisterAll();

    //
    // TestMessage
    //
    // (This is called internally by networking code that supports fault injection.)
    //
    // Tests message data to see if it is subject to a registered fault.
    //
    //
    // Parameters:
    //      DestUri         The destination URI of the message.
    //      MessageId       The Message Id type.
    //      Delay           Receives the message delay, in cases where STATUS_WAIT_1 is returned.
    //      CorruptIt       Receives a corruption instruction. If TRUE is returned, the caller should
    //                      corrupt the packet and try to deliver it.  If FALSE, then the packet
    //                      will be delayed or dropped.
    //
    // Return values:
    //      STATUS_SUCCESS    The message is not subject to a simulated fault and should be delivered.
    //      STATUS_WAIT_1     The message should be delayed by the number of milliseconds in <Delay>.
    //      STATUS_WAIT_2     The message should be intentionally corrupted and then delivered.
    //      STATUS_CANCELLED  The message should not be delivered (it should be dropped).
    //
    NTSTATUS
    TestMessage(
        __in  KWString& DestUri,
        __in  GUID& MessageId,
        __out ULONG& Delay
        );


    //  GetStatistics
    //
    //  Returns total messages dropped/delayed.
    //
    VOID GetStatistics(
        __in ULONGLONG& Dropped,
        __in ULONGLONG& Delayed
        )
    {
        Dropped = _Dropped;
        Delayed = _Delayed;
    }

    KNetFaultInjector(__in KAllocator& Alloc);      // Accessed by VNetwork
   ~KNetFaultInjector();

private:

    VOID
    Zero();

    // Data members
    class _FaultDef
    {
    public:
        ULONGLONG _Traffic;
        ULONG     _Ratio;
        BOOLEAN   _Drop;
        BOOLEAN   _Delay;
        ULONG     _DelayBy;
        KWString  _Uri;
        KGuid     _MessageId;

        _FaultDef(KAllocator& Allocator) 
            :   _Uri(Allocator)
        { 
            _Traffic = 0; 
            _Ratio = 0; 
            _Drop = FALSE; 
            _Delay = FALSE; 
            _DelayBy = 0; 
        }

    private:
        _FaultDef();
    };

    NTSTATUS
    DropOrDelayThis(
        __in  _FaultDef* Fault,
        __out ULONG& Delay
        );

    KHashTable<KWString,_FaultDef*>  _UriTable;
    KHashTable<GUID,_FaultDef*>      _MessageTable;
    KSpinLock                        _TableLock;
    KAllocator                      *_Alloc;

    // Statistics
    //
    ULONGLONG _AllTraffic;
    ULONGLONG _Dropped;
    ULONGLONG _Delayed;

    // Drop/delay all message types.
    //
    ULONG     _UniversalDelayRatio;
    ULONG     _UniversalDropRatio;
    ULONG     _UniversalDelayMSec;
    BOOLEAN   _UniversalDropInForce;
    BOOLEAN   _UniversalDelayInForce;

    LONGLONG  _StartTimestamp;
    LONGLONG  _StopTimestamp;
};


//  _ObjectDelayEngine
//
//  For internal use within networking core to help simulate faults by delaying messages.
//
class _ObjectDelayEngine : public KObject<_ObjectDelayEngine>
{
public:
    _ObjectDelayEngine(
		KAllocator& Allocator,
        KDelegate<VOID(PVOID Msg)> Callout
        );

    NTSTATUS
    DelayThis(
        PVOID Msg,
        ULONG MillisecondsToDelay
        );

    VOID
    StopAndDrain();

    ~_ObjectDelayEngine();

private:

#if KTL_USER_MODE
    static VOID CALLBACK
    TimerCallback(
        __inout PTP_CALLBACK_INSTANCE   Instance,
        __inout_opt PVOID               Context,
        __inout PTP_TIMER               Timer);
#else
    static KDEFERRED_ROUTINE DpcCallback;
#endif

    VOID
    SetTimeout(
        ULONG Delay
        );

    VOID
    ProcessTimeout();

    class Element{
        friend class _ObjectDelayEngine;

        Element(
            PVOID Msg,
            ULONG MillisecondsToDelay
            );

        KListEntry _Entry;
        PVOID _Msg;
        ULONG _Delay;

        static const ULONG EntryOffset;
    };

	KAllocator& _Allocator;
    BOOLEAN _Active;
    KSpinLock _Lock;
    LONGLONG _TimerStart;
    KNodeList<Element> _ElementList;
    KDelegate<VOID(PVOID Msg)> _Callout;

#if KTL_USER_MODE
    PTP_TIMER           _TimerPtr;
#else
    KTIMER              _KTimer;
    KDPC                _KDpc;
#endif
};



