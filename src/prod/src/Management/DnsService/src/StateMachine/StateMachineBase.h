// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "StateMachineQueue.h"

namespace DNS
{
    using ::_delete;

// Horrible bit of macro trickery necessary to build wide string literals out of macros
// that evaluate to a narrow string literal.
#define _DNS_WIDEN2(_x) L ## _x
#define _DNS_WIDEN(_x) _DNS_WIDEN2(_x)

// To turn the expansion of a macro into a string literal.
#define _DNS_STRINGIFY2(_x) #_x
#define _DNS_STRINGIFY(_x) _DNS_STRINGIFY2(_x)

#define DECLARE_STATE(STATE__, NUM__) \
const int _##STATE__ = NUM__; \
virtual void OnStateEnter_##STATE__(); \

#define DECLARE_STATES_2(S1__, S2__) \
    DECLARE_STATE(S1__, 1) \
    DECLARE_STATE(S2__, 2)

#define DECLARE_STATES_3(S1__, S2__, S3__) \
    DECLARE_STATES_2(S1__, S2__) \
    DECLARE_STATE(S3__, 3)

#define DECLARE_STATES_4(S1__, S2__, S3__, S4__) \
    DECLARE_STATES_3(S1__, S2__, S3__) \
    DECLARE_STATE(S4__, 4)

#define DECLARE_STATES_5(S1__, S2__, S3__, S4__, S5__) \
    DECLARE_STATES_4(S1__, S2__, S3__, S4__) \
    DECLARE_STATE(S5__, 5)

#define DECLARE_STATES_6(S1__, S2__, S3__, S4__, S5__, S6__) \
    DECLARE_STATES_5(S1__, S2__, S3__, S4__, S5__) \
    DECLARE_STATE(S6__, 6)

#define DECLARE_STATES_7(S1__, S2__, S3__, S4__, S5__, S6__, S7__) \
    DECLARE_STATES_6(S1__, S2__, S3__, S4__, S5__, S6__) \
    DECLARE_STATE(S7__, 7)

#define DECLARE_STATES_8(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__) \
    DECLARE_STATES_7(S1__, S2__, S3__, S4__, S5__, S6__, S7__) \
    DECLARE_STATE(S8__, 8)

#define DECLARE_STATES_9(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__) \
    DECLARE_STATES_8(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__) \
    DECLARE_STATE(S9__, 9)

#define DECLARE_STATES_10(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__) \
    DECLARE_STATES_9(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__) \
    DECLARE_STATE(S10__, 10)

#define DECLARE_STATES_11(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__) \
    DECLARE_STATES_10(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__) \
    DECLARE_STATE(S11__, 11)

#define DECLARE_STATES_12(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__, S12__) \
    DECLARE_STATES_11(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__) \
    DECLARE_STATE(S12__, 12)

#define DECLARE_STATES_13(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__, S12__, S13__) \
    DECLARE_STATES_12(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__, S12__) \
    DECLARE_STATE(S13__, 13)

#define DECLARE_STATES_14(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__, S12__, S13__, S14__) \
    DECLARE_STATES_13(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__, S12__, S13__) \
    DECLARE_STATE(S14__, 14)

#define DECLARE_STATES_15(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__, S12__, S13__, S14__, S15__) \
    DECLARE_STATES_14(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__, S12__, S13__, S14__) \
    DECLARE_STATE(S15__, 15)

#define DECLARE_STATES_16(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__, S12__, S13__, S14__, S15__, S16__) \
    DECLARE_STATES_15(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__, S12__, S13__, S14__, S15__) \
    DECLARE_STATE(S16__, 16)

#define DECLARE_STATES_17(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__, S12__, S13__, S14__, S15__, S16__, S17__) \
    DECLARE_STATES_16(S1__, S2__, S3__, S4__, S5__, S6__, S7__, S8__, S9__, S10__, S11__, S12__, S13__, S14__, S15__, S16__) \
    DECLARE_STATE(S17__, 17)

#define StartStateIndex -2
#define EndStateIndex -1

#define BEGIN_STATEMACHINE_DEFINITION \
private: \
    DECLARE_STATE(Start, StartStateIndex) \
    DECLARE_STATE(End, EndStateIndex) \

#define BEGIN_TRANSITIONS \
virtual void ChangeStateInternal(__in SmMessage msg) override { \

#define TRANSITION_BODY(STATE__, NEXT_STATE__) \


#define TRANSITION(STATE__, NEXT_STATE__) \
if (_currentState == _##STATE__) {\
if (msg != SmMsgTerminate) { \
_currentState = _##NEXT_STATE__; \
OnBeforeStateChange(_DNS_WIDEN(_DNS_STRINGIFY(STATE__)), _DNS_WIDEN(_DNS_STRINGIFY(NEXT_STATE__))); \
OnStateEnter_##NEXT_STATE__(); \
} else {\
_currentState = EndStateIndex; \
OnBeforeStateChange(_DNS_WIDEN(_DNS_STRINGIFY(STATE__)), L"End"); \
OnStateEnter_End(); \
}\
return; }\

#define TRANSITION_BOOL(STATE__, SUCCESSS_STATE__, FAILURE_STATE__) \
if (_currentState == _##STATE__) {\
if (msg != SmMsgTerminate) { \
_currentState = (msg == SmMsgSuccess) ? _##SUCCESSS_STATE__ : _##FAILURE_STATE__; \
OnBeforeStateChange(_DNS_WIDEN(_DNS_STRINGIFY(STATE__)), (msg == SmMsgSuccess) ? _DNS_WIDEN(_DNS_STRINGIFY(SUCCESSS_STATE__)) : _DNS_WIDEN(_DNS_STRINGIFY(FAILURE_STATE__))); \
(msg == SmMsgSuccess) ? OnStateEnter_##SUCCESSS_STATE__() : OnStateEnter_##FAILURE_STATE__(); \
} else {\
_currentState = EndStateIndex; \
OnBeforeStateChange(_DNS_WIDEN(_DNS_STRINGIFY(STATE__)), L"End"); \
OnStateEnter_End(); \
}\
return; }\

#define END_TRANSITIONS  }

#define END_STATEMACHINE_DEFINITION 

    class StateMachineBase :
        public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(StateMachineBase);

    protected:
        virtual void ChangeStateInternal(__in SmMessage msg) = 0;

        virtual void OnBeforeStateChange(
            __in LPCWSTR fromState,
            __in LPCWSTR toState
        ) = 0;

        virtual void OnStateEnter_Start() = 0;

        virtual void OnStateEnter_End() = 0;

    protected:
        // KAsyncContextBase
        virtual void OnStart() = 0;

    protected:
        void ActivateStateMachine();

        void DeactivateStateMachine();

        void ChangeStateAsync(
            __in bool fSuccess
        );

        void TerminateAsync();

        void OnDequeue(
            __in_opt KAsyncContextBase* const pParentContext,
            __in KAsyncContextBase& completedContext
        );

        void OnStateMachineQueueCompleted(
            __in_opt KAsyncContextBase* const pParentContext,
            __in KAsyncContextBase& completedContext
        );

    protected:
        int _currentState;

    private:
        KSharedPtr<StateMachineQueue> _spQueue;
        KSharedPtr<StateMachineQueue::DequeueOperation> _spDequeueOp;
    };
}
