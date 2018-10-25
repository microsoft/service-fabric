// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Disposable base class.
    /// </summary>
    internal abstract class Disposable : IDisposable
    {
        /// <summary>
        /// True if the object has been disposed.
        /// </summary>
        internal bool Disposed;

        /// <summary>
        /// Implementation of disposable call.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Derived method for performing custom disposable code.
        /// </summary>
        /// <param name="disposing">False, if destructor.</param>
        protected abstract void Dispose(bool disposing);
    }
}