// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Runtime.Serialization;

    /// <summary>
    /// The exception class for all exceptions arising from the management protocol related code.
    /// A general guideline is to not continue execution on hitting this exception.
    /// </summary>    
#if !DotNetCoreClr
    [Serializable]
#endif
    internal class ManagementException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ManagementException"/> class.
        /// </summary>
        public ManagementException()
        {            
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ManagementException"/> class.
        /// </summary>
        /// <param name="message">The message that describes the exception.</param>
        public ManagementException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Constructor. Initializes a new instance of the <see cref="ManagementException"/> class
        /// with the specified error message and optionally, an inner exception. 
        /// </summary>
        /// <param name="message">The message that describes the exception.</param>
        /// <param name="innerException">
        /// The exception that is the cause of the current exception. 
        /// Optional parameter that defaults to null.
        /// </param>
        public ManagementException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ManagementException"/> class by
        /// deserializing it from a persistent data source.
        /// </summary>
        /// <param name="info">Serialization information object that stores all information regarding serialization.</param>
        /// <param name="context">Object that describes the context for serialization/deserialization.</param>
#if !DotNetCoreClr
        protected ManagementException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }
#endif
    }
}