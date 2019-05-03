// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.Globalization;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using System.Runtime.Serialization;

    /// <summary>
    /// State provider is faulted.
    /// </summary>
    [Serializable]
    public class StateProviderException : Exception
    {
        /// <summary>
        /// 
        /// </summary>
        public StateProviderException() : base() { }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="message"></param>
        public StateProviderException(string message) : base(message) { }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="format"></param>
        /// <param name="args"></param>
        public StateProviderException(string format, params object[] args) : 
            base(string.Format(CultureInfo.CurrentCulture, format, args)) { }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="message"></param>
        /// <param name="innerException"></param>
        public StateProviderException(string message, Exception innerException) : base(message, innerException) { }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="format"></param>
        /// <param name="innerException"></param>
        /// <param name="args"></param>
        public StateProviderException(string format, Exception innerException, params object[] args)
            : base(string.Format(CultureInfo.CurrentCulture, format, args), innerException) { }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="info"></param>
        /// <param name="context"></param>
        protected StateProviderException(SerializationInfo info, StreamingContext context) : base(info, context) { }
    }
    /// <summary>
    /// State provider transient exception. Can be retried.
    /// </summary>
    [Serializable]
    public class StateProviderTransientException : StateProviderException
    {
    }
    /// <summary>
    /// State provider permanent exception. Cannot be retried.
    /// </summary>
    [Serializable]
    public class StateProviderNonRetryableException : StateProviderException
    {
    }
    /// <summary>
    /// State provider is faulted. Cannot be retried.
    /// </summary>
    [Serializable]
    public class StateProviderFaultedException : StateProviderNonRetryableException
    {
    }
    /// <summary>
    /// State provider is not readable.
    /// </summary>
    [Serializable]
    public class StateProviderNotReadableException : StateProviderTransientException
    {
    }
    /// <summary>
    /// State provider is not writable.
    /// </summary>
    [Serializable]
    public class StateProviderNotWritableException : StateProviderTransientException
    {
    }
}