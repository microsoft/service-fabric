// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LoggingReplicatorTests
{
    using namespace std;
    using namespace ktl;
    using namespace Data::LoggingReplicator;
    using namespace TxnReplicator;
    using namespace Data::Utilities;
    using namespace Common;

    StringLiteral const TraceComponent = "TransactionStateMachineTests";

    class TransactionStateMachineTests
    {
    protected:

        void EndTest();

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    void TransactionStateMachineTests::EndTest()
    {
        prId_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(TransactionStateMachineTestsSuite, TransactionStateMachineTests)

    BOOST_AUTO_TEST_CASE(DefaultState)
    {
        TEST_TRACE_BEGIN("DefaultState")
        {
            TransactionStateMachine::SPtr stateMachine = TransactionStateMachine::Create(1, allocator);
            VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Active);
        }
    }

    BOOST_AUTO_TEST_CASE(Reading)
    {
        TEST_TRACE_BEGIN("Reading")
        {
            TransactionStateMachine::SPtr stateMachine = TransactionStateMachine::Create(1, allocator);
            
            status = stateMachine->OnBeginRead();
            VERIFY_ARE_EQUAL(status, STATUS_SUCCESS);
            VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Reading);

            status = stateMachine->OnBeginRead();
            VERIFY_ARE_EQUAL(status, SF_STATUS_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED);

            status = stateMachine->OnBeginCommit();
            VERIFY_ARE_EQUAL(status, SF_STATUS_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED);

            status = stateMachine->OnSystemInternallyAbort();
            VERIFY_ARE_EQUAL(status, SF_STATUS_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED);

            status = stateMachine->OnUserBeginAbort();
            VERIFY_ARE_EQUAL(status, SF_STATUS_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED);

            status = stateMachine->OnAddOperation([] { return STATUS_SUCCESS; });
            VERIFY_ARE_EQUAL(status, SF_STATUS_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED);

            stateMachine->OnEndRead();
            VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Active);
        }
    }

    BOOST_AUTO_TEST_CASE(Commit)
    {
        TEST_TRACE_BEGIN("Commit")
        {
            {
                TransactionStateMachine::SPtr stateMachine = TransactionStateMachine::Create(1, allocator);

                status = stateMachine->OnBeginCommit();
                VERIFY_ARE_EQUAL(status, STATUS_SUCCESS);
                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Committing);

                status = stateMachine->OnBeginRead();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnSystemInternallyAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnUserBeginAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnAddOperation([] { return STATUS_SUCCESS; });
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                try
                {
                    stateMachine->OnBeginCommit();
                }
                catch (Exception &ex)
                {
                    VERIFY_ARE_EQUAL(ex.GetStatus(), SF_STATUS_TRANSACTION_NOT_ACTIVE);
                }

                stateMachine->OnCommitSuccessful();

                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Committed);
            }

            {
                TransactionStateMachine::SPtr stateMachine = TransactionStateMachine::Create(1, allocator);

                stateMachine->OnBeginCommit();
                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Committing);

                status = stateMachine->OnBeginRead();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnSystemInternallyAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnUserBeginAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnAddOperation([] { return STATUS_SUCCESS; });
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnBeginCommit();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                stateMachine->OnCommitFaulted();
                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Faulted);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(UserAbort)
    {
        TEST_TRACE_BEGIN("UserAbort")
        {
            {
                TransactionStateMachine::SPtr stateMachine = TransactionStateMachine::Create(1, allocator);

                status = stateMachine->OnUserBeginAbort();
                VERIFY_ARE_EQUAL(status, STATUS_SUCCESS);
                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Aborting);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::UserAborted);

                status = stateMachine->OnBeginRead();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnBeginCommit();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnAddOperation([] { return STATUS_SUCCESS; });
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnUserBeginAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnSystemInternallyAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnUserDisposeBeginAbort([] {});
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                stateMachine->OnAbortSuccessful();

                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Aborted);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::UserAborted);
            }

            {
                TransactionStateMachine::SPtr stateMachine = TransactionStateMachine::Create(1, allocator);

                status = stateMachine->OnUserBeginAbort();
                VERIFY_ARE_EQUAL(status, STATUS_SUCCESS);
                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Aborting);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::UserAborted);

                status = stateMachine->OnBeginRead();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnBeginCommit();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnAddOperation([] { return STATUS_SUCCESS; });
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnUserBeginAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnSystemInternallyAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnUserDisposeBeginAbort([] {});
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                stateMachine->OnAbortFaulted();

                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Faulted);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::UserAborted);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(DisposeAbort)
    {
        TEST_TRACE_BEGIN("DisposeAbort")
        {
            {
                TransactionStateMachine::SPtr stateMachine = TransactionStateMachine::Create(1, allocator);

                status = stateMachine->OnUserDisposeBeginAbort([] {});
                VERIFY_ARE_EQUAL(status, STATUS_SUCCESS);
                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Aborting);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::UserDisposed);

                status = stateMachine->OnBeginRead();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnBeginCommit();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnAddOperation([] { return STATUS_SUCCESS; });
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnUserBeginAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnSystemInternallyAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnUserDisposeBeginAbort([] {});
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                stateMachine->OnAbortSuccessful();

                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Aborted);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::UserDisposed);
            }

            {
                TransactionStateMachine::SPtr stateMachine = TransactionStateMachine::Create(1, allocator);

                status = stateMachine->OnUserDisposeBeginAbort([] {});
                VERIFY_ARE_EQUAL(status, STATUS_SUCCESS);
                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Aborting);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::UserDisposed);

                status = stateMachine->OnBeginRead();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnBeginCommit();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnAddOperation([] { return STATUS_SUCCESS; });
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnUserBeginAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnSystemInternallyAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                status = stateMachine->OnUserDisposeBeginAbort([] {});
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);

                stateMachine->OnAbortFaulted();

                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Faulted);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::UserDisposed);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(SystemAbort)
    {
        TEST_TRACE_BEGIN("SystemAbort")
        {
            {
                TransactionStateMachine::SPtr stateMachine = TransactionStateMachine::Create(1, allocator);

                status = stateMachine->OnSystemInternallyAbort();
                VERIFY_ARE_EQUAL(status, STATUS_SUCCESS);
                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Aborting);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::SystemAborted);

                status = stateMachine->OnBeginRead();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                status = stateMachine->OnBeginCommit();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                status = stateMachine->OnAddOperation([] { return STATUS_SUCCESS; });
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                status = stateMachine->OnUserBeginAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                status = stateMachine->OnSystemInternallyAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                status = stateMachine->OnUserDisposeBeginAbort([] {});
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                stateMachine->OnAbortSuccessful();

                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Aborted);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::SystemAborted);
            }

            {
                TransactionStateMachine::SPtr stateMachine = TransactionStateMachine::Create(1, allocator);

                status = stateMachine->OnSystemInternallyAbort();
                VERIFY_ARE_EQUAL(status, STATUS_SUCCESS);
                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Aborting);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::SystemAborted);

                status = stateMachine->OnBeginRead();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                status = stateMachine->OnBeginCommit();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                status = stateMachine->OnAddOperation([] { return STATUS_SUCCESS; });
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                status = stateMachine->OnUserBeginAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                status = stateMachine->OnSystemInternallyAbort();
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                status = stateMachine->OnUserDisposeBeginAbort([] {});
                VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

                stateMachine->OnAbortFaulted();

                VERIFY_ARE_EQUAL(stateMachine->State, TransactionState::Enum::Faulted);
                VERIFY_ARE_EQUAL(stateMachine->AbortReason, AbortReason::Enum::SystemAborted);
            }
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
