// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the set of possible isolation levels for a <see cref="System.Fabric.Transaction" />.</para>
    /// </summary>
    public enum TransactionIsolationLevel
    {
        /// <summary>
        /// <para>Indicates the default isolation level of the store.</para>
        /// </summary>
        Default = NativeTypes.FABRIC_TRANSACTION_ISOLATION_LEVEL.FABRIC_TRANSACTION_ISOLATION_LEVEL_DEFAULT,

        /// <summary>
        /// <para>
        /// Indicates that volatile data can be read during the transaction. 
        /// </para>
        /// </summary>
        ReadUncommitted = NativeTypes.FABRIC_TRANSACTION_ISOLATION_LEVEL.FABRIC_TRANSACTION_ISOLATION_LEVEL_READ_UNCOMMITTED,

        /// <summary>
        /// <para>Indicates that volatile data cannot be read during the transaction, but can be modified. Shared locks are held while data is being 
        /// read to avoid dirty reads, but data can be changed before the end of the transaction that results in non-repeatable reads or phantom data.</para>
        /// </summary>
        ReadCommitted = NativeTypes.FABRIC_TRANSACTION_ISOLATION_LEVEL.FABRIC_TRANSACTION_ISOLATION_LEVEL_READ_COMMITTED,
        
        /// <summary>
        /// <para>Indicates that volatile data can be read but not modified during the transaction. Locks are placed on all data that is used in a 
        /// query to prevent other users from updating data. New rows can be inserted into data sets and are included in later reads in the current transaction.</para>
        /// </summary>
        RepeatableRead = NativeTypes.FABRIC_TRANSACTION_ISOLATION_LEVEL.FABRIC_TRANSACTION_ISOLATION_LEVEL_REPEATABLE_READ,
        
        /// <summary>
        /// <para>Indicates the snapshot level where volatile data can be read. Any data that is read will be a transaction-consistent version of the 
        /// data that existed at start of the transaction.</para>
        /// </summary>
        Snapshot = NativeTypes.FABRIC_TRANSACTION_ISOLATION_LEVEL.FABRIC_TRANSACTION_ISOLATION_LEVEL_SNAPSHOT,
        
        /// <summary>
        /// <para>Indicates that volatile data are serializable. Volatile data can be read but not modified, and no new data can be added during the 
        /// transaction. Indicates that range locks will be put on data sets. The locks prevent updates or inserts in data sets until the transaction finishes.</para>
        /// </summary>
        Serializable = NativeTypes.FABRIC_TRANSACTION_ISOLATION_LEVEL.FABRIC_TRANSACTION_ISOLATION_LEVEL_SERIALIZABLE
    }
}