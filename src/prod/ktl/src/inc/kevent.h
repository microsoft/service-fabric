/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kevent.h

Abstract:

    This file defines a notification event class.

Author:

    Norbert P. Kusters (norbertk) 28-Sep-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

#pragma once

class KEvent : public KObject<KEvent> {

    public:

        static const ULONG Infinite = 0xFFFFFFFF;

        ~KEvent();

        FAILABLE
        KEvent(
            __in BOOLEAN IsManualReset = TRUE,
            __in BOOLEAN InitialState = FALSE
            );

        VOID
        SetEvent(
            );

        VOID
        ResetEvent(
            );

        BOOLEAN
        WaitUntilSet(
            __in_opt ULONG TimeoutInMilliseconds = Infinite
            );

    private:

        K_DENY_COPY(KEvent);

        VOID
        Zero(
            );

        VOID
        Cleanup(
            );

        NTSTATUS
        Initialize(
            __in BOOLEAN IsManualReset,
            __in BOOLEAN InitialState
            );

#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX)
        HANDLE _EventHandle;
#else
        bool _manual;
        pthread_cond_t _cond;
        pthread_mutex_t _mutex;
        bool _signaled;
        bool _closed;
#endif
#else
        KEVENT _Event;
#endif

};


class KAutoResetEvent : public KEvent
{
public:
    KAutoResetEvent() : KEvent(FALSE) {}
};
