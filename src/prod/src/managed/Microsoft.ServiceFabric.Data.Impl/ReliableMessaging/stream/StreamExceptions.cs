// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    internal class ReliableSessionQuotaExceededException : Exception
    {
    }

    internal class DuplicateStateProviderException : Exception
    {
        internal DuplicateStateProviderException(string message)
            : base(message)
        {
        }
    }

    /// <summary>
    /// 
    /// </summary>
    public class ReliableStreamQuotaExceededException : Exception
    {
    }

    /// <summary>
    /// 
    /// </summary>
    public class ReliableStreamInvalidOperationException : InvalidOperationException
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="message"></param>
        public ReliableStreamInvalidOperationException(string message)
            : base(message)
        {
        }
    }
}