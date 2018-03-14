// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class CopyModeResult final 
            : public KObject<CopyModeResult>
        {

        public:

            static CopyModeResult CreateCopyNoneResult(__in SharedProgressVectorEntry const & sharedProgressVectorEntry);

            static CopyModeResult CreateFullCopyResult(
                __in SharedProgressVectorEntry const & sharedProgressVectorEntry,
                __in FullCopyReason::Enum fullCopyReason);

            static CopyModeResult CreatePartialCopyResult(
                __in SharedProgressVectorEntry const & sharedProgressVectorEntry,
                __in LONG64 sourceStartingLsn,
                __in LONG64 targetStartingLsn);

            static CopyModeResult CreateFalseProgressCopyResult(
                __in LONG64 lastRecoveredAtomicRedoOperationLsnAtTarget,
                __in SharedProgressVectorEntry const & sharedProgressVectorEntry,
                __in LONG64 sourceStartingLsn,
                __in LONG64 targetStartingLsn);

            CopyModeResult(__in CopyModeResult const & other);

            __declspec(property(get = get_CopyMode)) int CopyModeEnum;
            int get_CopyMode() const
            {
                return copyMode_;
            }

            __declspec(property(get = get_FullCopyReason)) FullCopyReason::Enum FullCopyReasonEnum;
            FullCopyReason::Enum get_FullCopyReason() const
            {
                return fullCopyReason_;
            }

            __declspec(property(get = get_SharedProgressVectorEntry)) SharedProgressVectorEntry const & SharedProgressVector;
            SharedProgressVectorEntry const & get_SharedProgressVectorEntry() const
            {
                return sharedProgressVectorEntry_;
            }

            __declspec(property(get = get_SourceStartingLsn)) LONG64 SourceStartingLsn;
            LONG64 get_SourceStartingLsn() const
            {
                return sourceStartingLsn_;
            }

            __declspec(property(get = get_TargetStartingLsn)) LONG64 TargetStartingLsn;
            LONG64 get_TargetStartingLsn() const
            {
                return targetStartingLsn_;
            }

        private:

            CopyModeResult();

            CopyModeResult(
                __in int copyMode,
                __in FullCopyReason::Enum fullCopyReason,
                __in SharedProgressVectorEntry const & sharedProgressVectorEntry,
                __in LONG64 sourceStartingLsn,
                __in LONG64 targetStartingLsn);

            int const copyMode_;
            FullCopyReason::Enum const fullCopyReason_;
            SharedProgressVectorEntry const sharedProgressVectorEntry_;
            LONG64 const sourceStartingLsn_;
            LONG64 const targetStartingLsn_;
        };
    }
}
