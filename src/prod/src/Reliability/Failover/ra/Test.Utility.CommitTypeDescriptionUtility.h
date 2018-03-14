// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class CommitTypeDescriptionUtility
            {
            public:
                CommitTypeDescriptionUtility() :
                wasNoOp_(true)
                {
                }

                CommitTypeDescriptionUtility(Infrastructure::CommitTypeDescription const & commitType) :
                    wasNoOp_(false),
                    commitType_(commitType)
                {
                }

                __declspec(property(get = get_WasProcessed)) bool WasProcessed;
                bool get_WasProcessed() const { return !wasNoOp_; }


                void VerifyInMemoryCommit() const
                {
                    VerifyUpdate(true);
                }

                void VerifyDiskCommit() const
                {
                    VerifyUpdate(false);
                }

                void VerifyNoOp() const
                {
                    Verify::AreEqualLogOnError(true, wasNoOp_, L"Expected no-op");
                }

            private:
                void VerifyUpdate(bool isInMemory) const
                {
                    Verify::AreEqualLogOnError(false, wasNoOp_, L"Expected update but was no-op");
                    Verify::AreEqualLogOnError(isInMemory, commitType_.IsInMemoryOperation, L"InMemoryOperation");
                    Verify::AreEqualLogOnError(Storage::Api::OperationType::Update, commitType_.StoreOperationType, L"Operation Type");
                }

                Infrastructure::CommitTypeDescription commitType_;
                bool wasNoOp_;
            };
        }
    }
}
