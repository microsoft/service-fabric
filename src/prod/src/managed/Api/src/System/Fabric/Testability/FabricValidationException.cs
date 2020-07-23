// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    using System;
    using System.Fabric.Strings;
    using System.Runtime.Serialization;

    /// <summary>
    /// Exception thrown by validation APIs in testability
    /// </summary>
    [Serializable]
    public class FabricValidationException : FabricException
    {
        /// <summary>
        /// Default constructor for FabricValidationException
        /// </summary>
        public FabricValidationException()
            : base()
        {
        }

        /// <summary>
        /// Constructor for FabricValidationException which takes in FabricErrorCode
        /// </summary>
        /// <param name="errorCode">FabricErrorCode for the failure</param>
        public FabricValidationException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// Constructor for FabricValidationException which takes in a message
        /// </summary>
        /// <param name="message">Error message</param>
        public FabricValidationException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Constructor for FabricValidationException which takes in a message and FabricErrorCode
        /// </summary>
        /// <param name="message">Error message </param>
        /// <param name="errorCode">Fabric Error code</param>
        public FabricValidationException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// Constructor for FabricValidationException which takes in a message and innner exception
        /// </summary>
        /// <param name="message">Error message</param>
        /// <param name="inner">Inner exception</param>
        public FabricValidationException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// Constructor for FabricValidationException which takes in a message and innner exception and fabric error code
        /// </summary>
        /// <param name="message">Error message</param>
        /// <param name="inner">Inner exception</param>
        /// <param name="errorCode">Fabric error code</param>
        public FabricValidationException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// FabricValidationException constructor
        /// </summary>
        /// <param name="info">info</param>
        /// <param name="context">context</param>
        protected FabricValidationException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// FabricValidationException constructor
        /// </summary>
        /// <param name="info">info</param>
        /// <param name="context">context</param>
        /// <param name="errorCode">errorCode</param>
        protected FabricValidationException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
    }
}

#pragma warning restore 1591