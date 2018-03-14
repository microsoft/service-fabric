// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace StoreBackupRequester
    {
        enum Enum : ULONG
        {
            User = 0,
            LogTruncation = 1,
            System = 2
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e);
        bool IsValid(Enum const & e);
    }

    class StoreBackupState
    {
    public:
        StoreBackupState() : backupState_(Inactive)
        {
        }

        bool TryAcquire(LONG desiredBackupState, LONG & backupStateIfNotAcquired)
        {
            backupStateIfNotAcquired = Inactive;
            return backupState_.compare_exchange_weak(backupStateIfNotAcquired, desiredBackupState);
        }

        void Release()
        {
            backupState_.store(Inactive);
        }

        __declspec(property(get = get_Value)) LONG Value;
        LONG get_Value() { return backupState_.load(); }

        static const LONG Inactive = 0x01;
        static const LONG ActiveByUser = 0x02;
        static const LONG ActiveByLogTruncation = 0x04;
        static const LONG ActiveBySystem = 0x08;

    private:
        Common::atomic_long backupState_;
    };

    class ScopedActiveBackupState;
    typedef std::shared_ptr<ScopedActiveBackupState> ScopedActiveBackupStateSPtr;

    class ScopedActiveBackupState
    {
        DENY_COPY(ScopedActiveBackupState);

    private:
        ScopedActiveBackupState(__in StoreBackupState & activeBackupState, LONG desiredBackupState);

    public:
        virtual ~ScopedActiveBackupState();

        static bool TryAcquire(
            __in StoreBackupState & activeBackupState,
            __in StoreBackupRequester::Enum const & backupRequester,
            __out ScopedActiveBackupStateSPtr & scopedBackupState);

        __declspec(property(get = get_IsOwnedByUser)) bool IsOwnedByUser;
        bool get_IsOwnedByUser() const { return (this->State == StoreBackupState::ActiveByUser); }

        __declspec(property(get = get_IsOwnedBySystem)) bool IsOwnedBySystem;
        bool get_IsOwnedBySystem() const { return (this->State == StoreBackupState::ActiveBySystem); }

        __declspec(property(get = get_IsOwnedByLogTruncation)) bool IsOwnedByLogTruncation;
        bool get_IsOwnedByLogTruncation() const { return (this->State == StoreBackupState::ActiveByLogTruncation); }

    private:
        __declspec(property(get = get_State)) LONG State;
        LONG get_State() const { return hasAcquired_ ? activeBackupState_.Value : backupStateIfNotAcquired_; }

        StoreBackupState & activeBackupState_;
        bool hasAcquired_;
        LONG backupStateIfNotAcquired_;
    };
}
