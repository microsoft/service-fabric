// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Exception that indicates a failure due to the transaction being faulted internally by the system.</para>
    /// </summary>
    [Serializable]
    public class TransactionFaultedException : InvalidOperationException
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.TransactionFaultedException" /> class with appropriate message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for this exception.</para>
        /// </param>
        public TransactionFaultedException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.TransactionFaultedException" /> class with appropriate message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for this exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The Inner Exception that provides detailed information.</para>
        /// </param>
        public TransactionFaultedException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.TransactionFaultedException" /> class from serialized state.</para>
        /// </summary>
        /// <param name="info">
        /// <para>Contains serialized object data about the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>Contains contextual information about the source or destination.</para>
        /// </param>
        protected TransactionFaultedException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }
    }
}