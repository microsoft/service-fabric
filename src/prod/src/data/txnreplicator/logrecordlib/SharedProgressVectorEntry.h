// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class SharedProgressVectorEntry final 
            : public KObject<SharedProgressVectorEntry>
        {

        public:

            SharedProgressVectorEntry(
                __in ULONG sourceindex,
                __in ProgressVectorEntry const & sourceProgressVector,
                __in ULONG targetIndex,
                __in ProgressVectorEntry const & targetProgressVector,
                __in FullCopyReason::Enum fullCopyReason);

            SharedProgressVectorEntry(
                __in ULONG sourceindex,
                __in ProgressVectorEntry const & sourceProgressVector,
                __in ULONG targetIndex,
                __in ProgressVectorEntry const & targetProgressVector,
                __in FullCopyReason::Enum fullCopyReason,
                __in std::string const & failedValidationMsg);

            SharedProgressVectorEntry(__in SharedProgressVectorEntry const & other);

            static SharedProgressVectorEntry CreateEmptySharedProgressVectorEntry();
            static SharedProgressVectorEntry CreateTrimmedSharedProgressVectorEntry();

            __declspec(property(get = get_SourceIndex)) ULONG SourceIndex;
            ULONG get_SourceIndex() const
            {
                return sourceIndex_;
            }

            __declspec(property(get = get_SourceProgressVectorEntry)) ProgressVectorEntry const & SourceProgressVectorEntry;
            ProgressVectorEntry const & get_SourceProgressVectorEntry() const
            {
                return sourceProgressEntry_;
            }

            __declspec(property(get = get_TargetIndex)) ULONG TargetIndex;
            ULONG get_TargetIndex() const
            {
                return targetIndex_;
            }

            __declspec(property(get = get_TargetProgressVectorEntry)) ProgressVectorEntry const & TargetProgressVectorEntry;
            ProgressVectorEntry const & get_TargetProgressVectorEntry() const
            {
                return targetProgressEntry_;
            }

            __declspec(property(get = get_FullCopyReason)) FullCopyReason::Enum FullCopyReason;
            FullCopyReason::Enum get_FullCopyReason() const
            {
                return fullCopyReason_;
            }

            __declspec(property(get = get_FailedValidationMsg)) std::string const & FailedValidationMsg;
            std::string const & get_FailedValidationMsg() const
            {
                return failedValidationMsg_;
            }

        private:
            SharedProgressVectorEntry();

            int const sourceIndex_;
            int const targetIndex_;
            ProgressVectorEntry const sourceProgressEntry_;
            ProgressVectorEntry const targetProgressEntry_;
            FullCopyReason::Enum fullCopyReason_;
            std::string failedValidationMsg_;
        };
    }
}
