// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class StateMachine :
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Common>
    {
        DENY_COPY(StateMachine)
    
    public:
        static const uint64 Aborted = 1;
    
    public:
        virtual ~StateMachine();
        void Abort();
        uint64 GetState() const;
        void GetStateAndRefCount(__out uint64 & state, __out uint64 & refCount) const;
        virtual std::wstring StateToString(uint64 const state) const;

        Common::AsyncOperationSPtr BeginWaitForTransition(
            uint64 const targetStateMask,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) const;
        Common::AsyncOperationSPtr BeginWaitForTransition(
            uint64 const targetStateMask,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) const;
        Common::ErrorCode EndWaitForTransition(
            Common::AsyncOperationSPtr const & operation) const;

        void AbortAndWaitForTermination();

        void AbortAndWaitForTerminationAsync(
            Common::CompletionCallback const & completionCallback,
            Common::AsyncOperationSPtr const & parent);
       
        Common::AsyncOperationSPtr BeginAbortAndWaitForTermination(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
            void EndAbortAndWaitForTermination(
        Common::AsyncOperationSPtr const & operation);

    protected:
        StateMachine(uint64 initialState);

        Common::ErrorCode TransitionTo(uint64 target, uint64 entryMask);
        Common::ErrorCode TransitionTo(uint64 target, uint64 entryMask, __out uint64 & before);
        uint64 GetState_CallerHoldsLock() const;
        uint64 GetRefCount_CallerHoldsLock() const;
        virtual void OnAbort() = 0;
        virtual uint64 GetAbortEntryMask() const = 0;
        virtual uint64 GetTerminalStatesMask() const = 0;

        virtual uint64 GetRefCountedStatesMask();

         __declspec(property(get=get_Lock)) Common::RwLock & Lock;
        Common::RwLock & get_Lock() const { return this->lock_; }

    private:
        void TransitionToAborted();
        void CancelTransitionWaiters();
        
        bool IsRefCountedState(uint64 state);

        class TransitionWaitAsyncOperation;
        
    private:
        mutable Common::RwLock lock_;
        mutable std::map<std::wstring, Common::AsyncOperationSPtr> transitionWaiters_;
        uint64 current_;
        uint64 currentRefCount_;
        bool abortCalled_;
        std::wstring traceId_;
    };
}

#define STATEMACHINE_STATES_01(STATE1)                                                                                      \
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if (state == STATE1) return L ## #STATE1;                                                                           \
        else return StateMachine::StateToString(state);                                                                     \
    }                                                                                                                       \

#define STATEMACHINE_STATES_02(STATE1, STATE2)                                                                              \
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
    static const uint64 STATE2 = 4;                                                                                         \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if (state == STATE1) return L ## #STATE1;                                                                           \
        if (state == STATE2) return L ## #STATE2;                                                                           \
        else return StateMachine::StateToString(state);                                                                     \
    }                                                                                                                       \

#define STATEMACHINE_STATES_03(STATE1, STATE2, STATE3)                                                                      \
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
    static const uint64 STATE2 = 4;                                                                                         \
    static const uint64 STATE3 = 8;                                                                                         \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if (state == STATE1) return L ## #STATE1;                                                                           \
        if (state == STATE2) return L ## #STATE2;                                                                           \
        if (state == STATE3) return L ## #STATE3;                                                                           \
        else return StateMachine::StateToString(state);                                                                     \
    }                                                                                                                       \

#define STATEMACHINE_STATES_04(STATE1, STATE2, STATE3, STATE4)                                                              \
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
    static const uint64 STATE2 = 4;                                                                                         \
    static const uint64 STATE3 = 8;                                                                                         \
    static const uint64 STATE4 = 16;                                                                                        \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if (state == STATE1) return L ## #STATE1;                                                                           \
        if (state == STATE2) return L ## #STATE2;                                                                           \
        if (state == STATE3) return L ## #STATE3;                                                                           \
        if (state == STATE4) return L ## #STATE4;                                                                           \
        else return StateMachine::StateToString(state);                                                                     \
    }                                                                                                                       \

#define STATEMACHINE_STATES_05(STATE1, STATE2, STATE3, STATE4, STATE5)                                                      \
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
    static const uint64 STATE2 = 4;                                                                                         \
    static const uint64 STATE3 = 8;                                                                                         \
    static const uint64 STATE4 = 16;                                                                                        \
    static const uint64 STATE5 = 32;                                                                                        \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if (state == STATE1) return L ## #STATE1;                                                                           \
        if (state == STATE2) return L ## #STATE2;                                                                           \
        if (state == STATE3) return L ## #STATE3;                                                                           \
        if (state == STATE4) return L ## #STATE4;                                                                           \
        if (state == STATE5) return L ## #STATE5;                                                                           \
        else return StateMachine::StateToString(state);                                                                     \
    }                                                                                                                       \

#define STATEMACHINE_STATES_06(STATE1, STATE2, STATE3, STATE4, STATE5, STATE6)                                              \
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
    static const uint64 STATE2 = 4;                                                                                         \
    static const uint64 STATE3 = 8;                                                                                         \
    static const uint64 STATE4 = 16;                                                                                        \
    static const uint64 STATE5 = 32;                                                                                        \
    static const uint64 STATE6 = 64;                                                                                        \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if (state == STATE1) return L ## #STATE1;                                                                           \
        if (state == STATE2) return L ## #STATE2;                                                                           \
        if (state == STATE3) return L ## #STATE3;                                                                           \
        if (state == STATE4) return L ## #STATE4;                                                                           \
        if (state == STATE5) return L ## #STATE5;                                                                           \
        if (state == STATE6) return L ## #STATE6;                                                                           \
        else return StateMachine::StateToString(state);                                                                     \
    }                                                                                                                       \

#define STATEMACHINE_STATES_07(STATE1, STATE2, STATE3, STATE4, STATE5, STATE6, STATE7)                                      \
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
    static const uint64 STATE2 = 4;                                                                                         \
    static const uint64 STATE3 = 8;                                                                                         \
    static const uint64 STATE4 = 16;                                                                                        \
    static const uint64 STATE5 = 32;                                                                                        \
    static const uint64 STATE6 = 64;                                                                                        \
    static const uint64 STATE7 = 128;                                                                                       \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if (state == STATE1) return L ## #STATE1;                                                                           \
        if (state == STATE2) return L ## #STATE2;                                                                           \
        if (state == STATE3) return L ## #STATE3;                                                                           \
        if (state == STATE4) return L ## #STATE4;                                                                           \
        if (state == STATE5) return L ## #STATE5;                                                                           \
        if (state == STATE6) return L ## #STATE6;                                                                           \
        if (state == STATE7) return L ## #STATE7;                                                                           \
        else return StateMachine::StateToString(state);                                                                     \
    }             

#define STATEMACHINE_STATES_08(STATE1, STATE2, STATE3, STATE4, STATE5, STATE6, STATE7, STATE8)                              \
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
    static const uint64 STATE2 = 4;                                                                                         \
    static const uint64 STATE3 = 8;                                                                                         \
    static const uint64 STATE4 = 16;                                                                                        \
    static const uint64 STATE5 = 32;                                                                                        \
    static const uint64 STATE6 = 64;                                                                                        \
    static const uint64 STATE7 = 128;                                                                                       \
    static const uint64 STATE8 = 256;                                                                                       \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if (state == STATE1) return L ## #STATE1;                                                                           \
        if (state == STATE2) return L ## #STATE2;                                                                           \
        if (state == STATE3) return L ## #STATE3;                                                                           \
        if (state == STATE4) return L ## #STATE4;                                                                           \
        if (state == STATE5) return L ## #STATE5;                                                                           \
        if (state == STATE6) return L ## #STATE6;                                                                           \
        if (state == STATE7) return L ## #STATE7;                                                                           \
        if (state == STATE8) return L ## #STATE8;                                                                           \
        else return StateMachine::StateToString(state);                                                                     \
    }             

#define STATEMACHINE_STATES_09(STATE1, STATE2, STATE3, STATE4, STATE5, STATE6, STATE7, STATE8, STATE9)                      \
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
    static const uint64 STATE2 = 4;                                                                                         \
    static const uint64 STATE3 = 8;                                                                                         \
    static const uint64 STATE4 = 16;                                                                                        \
    static const uint64 STATE5 = 32;                                                                                        \
    static const uint64 STATE6 = 64;                                                                                        \
    static const uint64 STATE7 = 128;                                                                                       \
    static const uint64 STATE8 = 256;                                                                                       \
    static const uint64 STATE9 = 512;                                                                                       \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if (state == STATE1) return L ## #STATE1;                                                                           \
        if (state == STATE2) return L ## #STATE2;                                                                           \
        if (state == STATE3) return L ## #STATE3;                                                                           \
        if (state == STATE4) return L ## #STATE4;                                                                           \
        if (state == STATE5) return L ## #STATE5;                                                                           \
        if (state == STATE6) return L ## #STATE6;                                                                           \
        if (state == STATE7) return L ## #STATE7;                                                                           \
        if (state == STATE8) return L ## #STATE8;                                                                           \
        if (state == STATE9) return L ## #STATE9;                                                                           \
        else return StateMachine::StateToString(state);                                                                     \
    }             

    #define STATEMACHINE_STATES_10(STATE1, STATE2, STATE3, STATE4, STATE5, STATE6, STATE7, STATE8, STATE9, STATE10)         \
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
    static const uint64 STATE2 = 4;                                                                                         \
    static const uint64 STATE3 = 8;                                                                                         \
    static const uint64 STATE4 = 16;                                                                                        \
    static const uint64 STATE5 = 32;                                                                                        \
    static const uint64 STATE6 = 64;                                                                                        \
    static const uint64 STATE7 = 128;                                                                                       \
    static const uint64 STATE8 = 256;                                                                                       \
    static const uint64 STATE9 = 512;                                                                                       \
    static const uint64 STATE10 = 1024;                                                                                     \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if (state == STATE1) return L ## #STATE1;                                                                           \
        if (state == STATE2) return L ## #STATE2;                                                                           \
        if (state == STATE3) return L ## #STATE3;                                                                           \
        if (state == STATE4) return L ## #STATE4;                                                                           \
        if (state == STATE5) return L ## #STATE5;                                                                           \
        if (state == STATE6) return L ## #STATE6;                                                                           \
        if (state == STATE7) return L ## #STATE7;                                                                           \
        if (state == STATE8) return L ## #STATE8;                                                                           \
        if (state == STATE9) return L ## #STATE9;                                                                           \
        if (state == STATE10) return L ## #STATE10;                                                                         \
        else return StateMachine::StateToString(state);                                                                     \
    }

    #define STATEMACHINE_STATES_11(STATE1, STATE2, STATE3, STATE4, STATE5, STATE6, STATE7, STATE8, STATE9, STATE10, STATE11)\
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
    static const uint64 STATE2 = 4;                                                                                         \
    static const uint64 STATE3 = 8;                                                                                         \
    static const uint64 STATE4 = 16;                                                                                        \
    static const uint64 STATE5 = 32;                                                                                        \
    static const uint64 STATE6 = 64;                                                                                        \
    static const uint64 STATE7 = 128;                                                                                       \
    static const uint64 STATE8 = 256;                                                                                       \
    static const uint64 STATE9 = 512;                                                                                       \
    static const uint64 STATE10 = 1024;                                                                                     \
    static const uint64 STATE11 = 2048;                                                                                     \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if (state == STATE1) return L ## #STATE1;                                                                           \
        if (state == STATE2) return L ## #STATE2;                                                                           \
        if (state == STATE3) return L ## #STATE3;                                                                           \
        if (state == STATE4) return L ## #STATE4;                                                                           \
        if (state == STATE5) return L ## #STATE5;                                                                           \
        if (state == STATE6) return L ## #STATE6;                                                                           \
        if (state == STATE7) return L ## #STATE7;                                                                           \
        if (state == STATE8) return L ## #STATE8;                                                                           \
        if (state == STATE9) return L ## #STATE9;                                                                           \
        if (state == STATE10) return L ## #STATE10;                                                                         \
        if (state == STATE11) return L ## #STATE11;                                                                         \
        else return StateMachine::StateToString(state);                                                                     \
    }  

#define STATEMACHINE_STATES_12(STATE1, STATE2, STATE3, STATE4, STATE5, STATE6, STATE7, STATE8, STATE9, STATE10, STATE11, STATE12)\
public:                                                                                                                     \
    static const uint64 STATE1 = 2;                                                                                         \
    static const uint64 STATE2 = 4;                                                                                         \
    static const uint64 STATE3 = 8;                                                                                         \
    static const uint64 STATE4 = 16;                                                                                        \
    static const uint64 STATE5 = 32;                                                                                        \
    static const uint64 STATE6 = 64;                                                                                        \
    static const uint64 STATE7 = 128;                                                                                       \
    static const uint64 STATE8 = 256;                                                                                       \
    static const uint64 STATE9 = 512;                                                                                       \
    static const uint64 STATE10 = 1024;                                                                                     \
    static const uint64 STATE11 = 2048;                                                                                     \
    static const uint64 STATE12 = 4096;                                                                                     \
                                                                                                                            \
    virtual std::wstring StateToString(uint64 const state) const                                                            \
    {                                                                                                                       \
        if(state == STATE1) return L ## #STATE1;                                                                            \
        if(state == STATE2) return L ## #STATE2;                                                                            \
        if(state == STATE3) return L ## #STATE3;                                                                            \
        if(state == STATE4) return L ## #STATE4;                                                                            \
        if(state == STATE5) return L ## #STATE5;                                                                            \
        if(state == STATE6) return L ## #STATE6;                                                                            \
        if(state == STATE7) return L ## #STATE7;                                                                            \
        if(state == STATE8) return L ## #STATE8;                                                                            \
        if(state == STATE9) return L ## #STATE9;                                                                            \
        if(state == STATE10) return L ## #STATE10;                                                                          \
        if(state == STATE11) return L ## #STATE11;                                                                          \
        if(state == STATE12) return L ## #STATE12;                                                                          \
        else return StateMachine::StateToString(state);                                                                     \
    }

#define STATEMACHINE_TRANSITION(TARGET_STATE, ENTRY_MASK)                                                                   \
public:                                                                                                                     \
    Common::ErrorCode TransitionTo##TARGET_STATE()                                                                          \
    {                                                                                                                       \
        return TransitionTo(TARGET_STATE, ENTRY_MASK);                                                                      \
    }                                                                                                                       \
                                                                                                                            \
    Common::ErrorCode TransitionTo##TARGET_STATE(__out uint64 & before)                                                     \
    {                                                                                                                       \
        return TransitionTo(TARGET_STATE, ENTRY_MASK, before);                                                              \
    }                                                                                                                       \
                                                                                                                            \
    uint64 GetEntryMaskFor##TARGET_STATE()                                                                                  \
    {                                                                                                                       \
        return ENTRY_MASK;                                                                                                  \
    }


// Define the states from which direct transition is allowed to Aborted state. 
//   If an Abort is called whle the state machine is in any other state, Abort call
//   is queued and OnAbort is called when any other transition is attempted. This 
//   prevents running Abort in parallel with certain transient states.
#define STATEMACHINE_ABORTED_TRANSITION(ENTRY_MASK)                                                                         \
protected:                                                                                                                  \
    uint64 GetAbortEntryMask() const                                                                                        \
    {                                                                                                                       \
        return (ENTRY_MASK);                                                                                                \
    }


// Define the terminal states. This is used to create a transition waiter that waits for the 
// state machine to go to terminal state. Aborted is by default a terminal state. Any states 
// other than Aborted should be listed here.
#define STATEMACHINE_TERMINAL_STATES(TERMINAL_STATES_MASK)                                                                  \
protected:                                                                                                                  \
    uint64 GetTerminalStatesMask() const                                                                                    \
    {                                                                                                                       \
        return (TERMINAL_STATES_MASK);                                                                                      \
    }

#define STATEMACHINE_REF_COUNTED_STATES(REF_COUNTED_STATES_MASK)                                                            \
protected:                                                                                                                  \
    uint64 GetRefCountedStatesMask() override                                                                               \
    {                                                                                                                       \
        return (REF_COUNTED_STATES_MASK);                                                                                   \
    }                                                                                                                   \
