// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AsyncFabricComponent;
    class FabricComponent;

    struct FabricComponentState
    {
        DENY_COPY(FabricComponentState)

    public:
        FabricComponentState();
        ~FabricComponentState();

        __declspec(property(get=get_Value)) LONG Value;

        inline LONG get_Value() const
        {
            AcquireReadLock grab(stateLock_);
            return state_;   
        }

        inline LONG get_Value_CallerHoldsLock() const
        {
            return state_;   
        }

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        static std::wstring ToString(LONG state);
    
        static const LONG Created  = 1;
        static const LONG Opening  = 2;
        static const LONG Opened   = 4;
        static const LONG Closing  = 8;
        static const LONG Closed   = 16;
        static const LONG Failed   = 32;
        static const LONG Aborted  = 64;

    private:
        Common::ErrorCode TransitionToOpening();
        Common::ErrorCode TransitionToOpened();
        Common::ErrorCode TransitionToClosing();
        Common::ErrorCode TransitionToClosed();
        void TransitionToFailed();
        void TransitionToAborted(bool & shouldCallOnAbort);
        void SetStateToAborted();

        mutable Common::RwLock stateLock_;
        LONG state_;
        bool abortCalled_;
        bool closeCalled_;

        static const LONG DestructEntryMask = Created | Closed | Aborted;
        static const LONG FailedEntryMask = Opening | Closing;
        static const LONG AbortedEntryMask = Opened | Failed;

        friend AsyncFabricComponent;
        friend FabricComponent;
    };
}
