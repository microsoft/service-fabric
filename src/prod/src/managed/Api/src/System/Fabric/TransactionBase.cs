// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.ObjectModel;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>Provides an abstract base class for a transaction.</para>
    /// </summary>
    public abstract class TransactionBase : IDisposable
    {
        private bool disposed;
        private Collection<IDisposable> linkedDisposables;

        internal TransactionBase(NativeRuntime.IFabricTransactionBase nativeTransactionBase)
        {
            this.NativeTransactionBase = nativeTransactionBase;
            this.Id = this.NativeTransactionBase.get_Id();
            this.IsolationLevel = (TransactionIsolationLevel)this.NativeTransactionBase.get_IsolationLevel();
            this.linkedDisposables = new Collection<IDisposable>();
        }

        /// <summary>
        /// <para>Enables an object to try to free resources and perform other cleanup operations before it is reclaimed by garbage collection.</para>
        /// </summary>
        ~TransactionBase()
        {
            this.Dispose(false);
        }

        /// <summary>
        /// <para>Gets the ID of the transaction as a <see cref="System.Guid" />.</para>
        /// </summary>
        /// <value>
        /// <para>The transaction ID as a <see cref="System.Guid" />.</para>
        /// </value>
        public Guid Id { get; private set; }

        /// <summary>
        /// <para>Gets the isolation level of the transaction as a <see cref="System.Fabric.TransactionIsolationLevel" />.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.Fabric.TransactionIsolationLevel" /> object that represents the isolation level of the transaction.</para>
        /// </value>
        /// <remarks>
        /// <para>The isolation level of a transaction determines what level of access to volatile data other transactions have before a transaction finishes. 
        /// For more information about isolation levels, see the documentation for the <see cref="System.Fabric.TransactionIsolationLevel" /> enumeration.</para>
        /// </remarks>
        public TransactionIsolationLevel IsolationLevel { get; private set; }

        internal NativeRuntime.IFabricTransactionBase NativeTransactionBase { get; private set; }

        /// <summary>
        /// <para>Performs application-defined tasks that are associated with freeing, releasing, or resetting unmanaged resources.</para>
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        internal void Link(IDisposable linkedObject)
        {
            this.linkedDisposables.Add(linkedObject);
        }

        /// <summary>
        /// <para>The dispose event occurs when the transaction is disposed of through the <languageKeyword>OnDispose</languageKeyword> method.</para>
        /// </summary>
        /// <remarks>
        /// <para>When overriding <see cref="System.Fabric.Transaction.OnDispose" />, be sure to call the <languageKeyword>OnDispose</languageKeyword> 
        /// method on the base class.</para>
        /// </remarks>
        protected internal virtual void OnDispose()
        {
            foreach (var disposable in this.linkedDisposables)
            {
                disposable.Dispose();
            }

            if (this.NativeTransactionBase != null)
            {
                Marshal.FinalReleaseComObject(this.NativeTransactionBase);
                this.NativeTransactionBase = null;
            }
        }

        /// <summary>
        /// <para>Gets a value that indicates whether the object has been disposed.</para>
        /// </summary>
        /// <returns>
        /// <para>Returns <languageKeyword>true</languageKeyword> if the object has been disposed; otherwise, returns <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        protected bool IsDisposed()
        {
            return this.disposed;
        }

        private void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    this.OnDispose();
                }

                this.disposed = true;
            }
        }
    }
}