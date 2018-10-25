// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using Data;
    using System;
    using System.Globalization;

    /// <summary>
    /// Transaction State machine
    /// 
    /// The accepted state transitions are:
    /// 
    /// Active  ->Reading       ->Active
    /// 
    /// Active  ->Committing    ->Committed     (Happy path)
    /// 
    ///                         ->Active        (Can only happen for atomic or atomicredo operations)
    ///                     
    ///                         ->Faulted       (When commit fails due to terminal exception like object closed or not primary)
    ///                         
    ///                     
    /// Active  ->Aborting      ->Aborted       (Happy path for abort)
    ///       
    ///                         ->Faulted       (When abort fails due to terminal exception like object closed or not primary)
    ///                         
    /// </summary>
    public class TransactionStateMachine
    {
        /// <summary>
        /// State machine for the transaction state
        /// </summary>
        public TransactionStateMachine(long transactionId)
        {
            txId_ = transactionId;
            state_ = TransactionState.Active;
            stateLock_ = new object();
            abortReason_ = AbortReason.Invalid;
        }
        
        /// <summary>
        /// Gets the transaction state
        /// </summary>
        public TransactionState State
        {
            get
            {
                lock (stateLock_)
                {
                    return this.state_;
                }
            }
        }
        
        /// <summary>
        /// Add operation
        /// </summary>
        /// <param name="addOperationAction"></param>
        public void OnAddOperation(Action addOperationAction)
        {
            lock(stateLock_)
            {
                if (state_ != TransactionState.Active)
                {
                    ThrowExceptionCallerHoldsLock();
                }

                addOperationAction();
            }
        }
    
        /// <summary>
        /// Atomic and Atomic redo operation addasync
        /// </summary>
        public void OnBeginAtomicOperationAddAsync()
        {
            lock(stateLock_)
            {
                if (state_ != TransactionState.Active)
                {
                    ThrowExceptionCallerHoldsLock();
                }

                state_ = TransactionState.Committing;
            }
        }

        /// <summary>
        /// Atomic and Atomic redo operation addasync successful
        /// </summary>
        public void OnAtomicOperationSuccess()
        {
            lock(stateLock_)
            {
                Utility.Assert(
                    state_ == TransactionState.Committing,
                    "Transaction State during OnAtomicOperationSuccess must be Committing. It is {0} for transaction id {1}",
                    state_, txId_);

                state_ = TransactionState.Committed;
            }
        }

        /// <summary>
        /// Atomic and Atomic redo operation addasync faulted
        /// </summary>
        public void OnAtomicOperationFaulted()
        {
            lock(stateLock_)
            {
                Utility.Assert(
                    state_ == TransactionState.Committing,
                    "Transaction State during OnAtomicOperationFaulted must be Committing. It is {0} for transaction id {1}",
                    state_, txId_);

                state_ = TransactionState.Faulted;
            }
        }


        /// <summary>
        /// Atomic and Atomic redo operation addasync 
        /// </summary>
        public void OnAtomicOperationRetry()
        {
            lock(stateLock_)
            {
                Utility.Assert(
                    state_ == TransactionState.Committing,
                    "Transaction State during OnAtomicOperationRetry must be Committing. It is {0} for transaction id {1}",
                    state_, txId_);

                state_ = TransactionState.Active;
            }
        }
    
        /// <summary>
        /// On starting a read operation using GetVisibilitysequencenumber
        /// </summary>
        public void OnBeginRead()
        {
            lock(stateLock_)
            {
                if (state_ != TransactionState.Active)
                {
                    ThrowExceptionCallerHoldsLock();
                }

                state_ = TransactionState.Reading;
            }
        }

        /// <summary>
        /// On end a read operation using GetVisibilitysequencenumber
        /// </summary>
        public void OnEndRead()
        {
            lock(stateLock_)
            {
                Utility.Assert(
                    state_ == TransactionState.Reading,
                    "Transaction State during OnEndRead must be Reading. It is {0} for transaction id {1}",
                    state_, txId_);

                state_ = TransactionState.Active;
            }
        }

        /// <summary>
        /// Transaction is starting to commit
        /// </summary>
        public void OnBeginCommit()
        {
            lock(stateLock_)
            {
                if (state_ != TransactionState.Active)
                {
                    ThrowExceptionCallerHoldsLock();
                }

                state_ = TransactionState.Committing;
            }
        }

        /// <summary>
        /// Transaction committed faulted
        /// </summary>
        public void OnCommitFaulted()
        {
            lock(stateLock_)
            {
                Utility.Assert(
                    state_ == TransactionState.Committing,
                    "Transaction State during OnCommitFaulted must be Committing. It is {0} for transaction id {1}",
                    state_, txId_);

                state_ = TransactionState.Faulted;
            }
        }

        /// <summary>
        /// Transaction committed successfully
        /// </summary>
        public void OnCommitSuccessful()
        {
            lock(stateLock_)
            {
                Utility.Assert(
                    state_ == TransactionState.Committing,
                    "Transaction State during OnCommitSuccessful must be Committing. It is {0} for transaction id {1}",
                    state_, txId_);

                state_ = TransactionState.Committed;
            }
        }

        /// <summary>
        /// On user invoking abort
        /// </summary>
        public void OnUserBeginAbort()
        {
            lock(stateLock_)
            {
                if (state_ != TransactionState.Active)
                {
                    ThrowExceptionCallerHoldsLock();
                }

                state_ = TransactionState.Aborting;
                abortReason_ = AbortReason.UserAborted;
            }
        }

        /// <summary>
        /// On user transaction dispose
        /// </summary>
        /// <param name="unregisterCallback"></param>
        /// <returns>
        /// Boolean indicating whether the transaction must be aborted.
        /// True = transaction must be aborted, False = abort is not necessary
        /// </returns>
        public bool OnUserDisposeBeginAbort(Action unregisterCallback)
        {
            lock(stateLock_)
            {
                Utility.Assert(
                    state_ != TransactionState.Committing && state_ != TransactionState.Reading,
                    "Transaction State during OnUserDisposeBeginAbort cannot be Committing or reading for transaction id {0}. It is {1}",
                    txId_, state_);

                unregisterCallback(); // Do the unregister first

                if (state_ != TransactionState.Active)
                {
                    return false;
                }

                state_ = TransactionState.Aborting;
                abortReason_ = AbortReason.UserDisposed;

                // Transaction must be aborted
                return true;
            }
        }

        /// <summary>
        /// On system internally aborting transaction
        /// </summary>
        public void OnSystemInternallyAbort()
        {
            lock(stateLock_)
            {
                if (state_ != TransactionState.Active)
                {
                    ThrowExceptionCallerHoldsLock();
                }

                state_ = TransactionState.Aborting;
                abortReason_ = AbortReason.SystemAborted;
            }
        }

        /// <summary>
        /// On abort faulting due to non-retriable exception
        /// </summary>
        public void OnAbortFaulted()
        {
            lock(stateLock_)
            {
                Utility.Assert(
                    state_ == TransactionState.Aborting,
                    "Transaction State during OnAbortFaulted must be Aborting. It is {0} for transaction id {1}",
                    state_, txId_);

                state_ = TransactionState.Faulted;
            }
        }

        /// <summary>
        /// On abort successfully completing
        /// </summary>
        public void OnAbortSuccessful()
        {
            lock(stateLock_)
            {
                Utility.Assert(
                    state_ == TransactionState.Aborting,
                    "Transaction State during OnAbortSuccessful must be Aborting. It is {0} for transaction id {1}",
                    state_, txId_);

                state_ = TransactionState.Aborted;
            }
        }

        /// <summary>
        /// Gets the Reason for abort
        /// </summary>
        internal AbortReason AbortedReason
        {
            get
            {
                lock (stateLock_)
                {
                    return this.abortReason_;
                }
            }
        }

        static internal string GetTransactionIdFormattedString(string format, long transactionId)
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                format,
                transactionId);
        }


        private void ThrowExceptionCallerHoldsLock()
        {
            switch (state_)
            {
                case TransactionState.Active:
                    Utility.CodingError("Transaction is active. Cannot throw exception");
                    break;

                case TransactionState.Reading:
                    throw new InvalidOperationException(GetTransactionIdFormattedString(SR.Error_ReplicatorTransactionReadInProgress, txId_));

                case TransactionState.Committing:
                case TransactionState.Committed:
                    throw new InvalidOperationException(GetTransactionIdFormattedString(SR.Error_ReplicatorTransactionNotActive, txId_));

                case TransactionState.Aborting:
                case TransactionState.Aborted:
                case TransactionState.Faulted:

                    switch (abortReason_)
                    {
                        case AbortReason.Invalid: // Implies transaction got faulted without any abort
                        case AbortReason.UserAborted:
                        case AbortReason.UserDisposed:
                            throw new InvalidOperationException(GetTransactionIdFormattedString(SR.Error_ReplicatorTransactionNotActive, txId_));

                        case AbortReason.SystemAborted:
                            throw new System.Fabric.TransactionFaultedException(GetTransactionIdFormattedString(SR.Error_ReplicatorTransactionInternallyAborted, txId_));
                    }

                    Utility.CodingError("Unreachable codepath was reached");
                    break;
            }
        }

        internal enum AbortReason
        {
            Invalid,
            UserAborted,
            UserDisposed,
            SystemAborted
        }

        private readonly long txId_;
        private readonly object stateLock_;
        private TransactionState state_;
        AbortReason abortReason_;
    }
}