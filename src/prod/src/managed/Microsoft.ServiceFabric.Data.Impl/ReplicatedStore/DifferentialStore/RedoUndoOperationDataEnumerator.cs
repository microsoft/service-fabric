// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections;
    using System.Collections.Generic;

    /// <summary>
    /// Operation data buffers enumerator.
    /// </summary>
    internal class RedoUndoOperationDataEnumerator : IEnumerator<ArraySegment<byte>>
    {
        #region Instance Members

        /// <summary>
        /// Operation buffer.
        /// </summary>
        private byte[] buffer;

        /// <summary>
        /// Position of the enumerator.
        /// </summary>
        private int position;

        #endregion

        /// <summary>
        /// Proper constructor.
        /// </summary>
        /// <param name="buffer">Single buffer to enumerate.</param>
        public RedoUndoOperationDataEnumerator(byte[] buffer)
        {
            this.buffer = buffer;
            this.position = 0;
        }

        #region IEnumerator<ArraySegment<byte>> Members

        /// <summary>
        /// Gets the element in the collection at the current position of the enumerator.
        /// </summary>
        public ArraySegment<byte> Current
        {
            get { return (0 == this.position) ? new ArraySegment<byte>() : new ArraySegment<byte>(this.buffer); }
        }

        /// <summary>
        /// Gets the element in the collection at the current position of the enumerator.
        /// </summary>
        object IEnumerator.Current
        {
            get { return (0 == this.position) ? new ArraySegment<byte>() : new ArraySegment<byte>(this.buffer); }
        }

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            // Nothing to do.
        }

        /// <summary>
        /// Advances the enumerator to the next element of the collection.
        /// </summary>
        /// <returns></returns>
        public bool MoveNext()
        {
            if (0 == this.position)
            {
                this.position++;
                return true;
            }

            return false;
        }

        /// <summary>
        /// Sets the enumerator to its initial position, which is before the first element in the collection.
        /// </summary>
        public void Reset()
        {
            this.position = 0;
        }

        #endregion
    }
}