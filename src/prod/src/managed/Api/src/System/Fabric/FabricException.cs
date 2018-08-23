// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>The base Service Fabric exception class.</para>
    /// </summary>
    /// <remarks>
    /// <para>Defines an error code property that is used to indicate the precise circumstance that caused the exception, in addition to properties defined by the base <see cref="System.Exception" /> class.</para>
    /// </remarks>
    [Serializable]
    public class FabricException : Exception
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.</para>
        /// </summary>
        public FabricException() : this(FabricErrorCode.Unknown)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricException" /> class with a specified error code.</para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricException(FabricErrorCode errorCode) : this(string.Empty, errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricException" /> class with error code <see cref="FabricErrorCode.Unknown"/> and a specified error message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricException(string message) : this(message, FabricErrorCode.Unknown)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricException" /> class with specified error message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricException(string message, FabricErrorCode errorCode) : this(message, null, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricException(string message, Exception inner) : this(message, inner, FabricErrorCode.Unknown)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricException" /> class
        /// with a specified error message, a reference to the inner exception that is the cause of this exception, and a specified error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricException(string message, Exception inner, FabricErrorCode errorCode) : base(message, inner)
        {
            this.SetErrorCode(errorCode);
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricException" /> class with specified message and Hresult.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="hresult">The HResult that the exception is wrapping around</param>
        public FabricException(string message, int hresult) : this(message, null, hresult)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricException" /> class with specified message, inner exception and HResult.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The inner exception.</para>
        /// </param>
        /// <param name="hresult">The HResult that the exception is wrapping around</param>
        public FabricException(string message, Exception inner, int hresult) : base(message, inner)
        {
            this.SetErrorCode(hresult);
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricException" /> class from a serialized object data, with a specified context.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricException(SerializationInfo info, StreamingContext context) : this(info, context, FabricErrorCode.Unknown)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricException" /> class from a serialized object data, with specified context and error code.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        protected FabricException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode) : base(info, context)
        {
            this.SetErrorCode(errorCode);
        }

        /// <summary>
        /// <para>Gets the error code parameter.</para>
        /// </summary>
        /// <value>
        /// <para>The error code associated with the <see cref="System.Fabric.FabricException" /> exception.</para>
        /// </value>
        public FabricErrorCode ErrorCode { get; private set; }

        private void SetErrorCode(int hresult)
        {
            this.SetErrorCode((FabricErrorCode)hresult);
        }

        private void SetErrorCode(FabricErrorCode errorCode)
        {
            this.ErrorCode = errorCode;

            // Exception class throws SerializationException during de-serialization if HResult is zero.
            if (errorCode != 0)
            {
                this.HResult = (int)errorCode;
            }
        }
    }

    /// <summary>
    /// <para>The exception that indicates failure due to the use of a service partition key that is not valid.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricInvalidPartitionKeyException : FabricException
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidPartitionKeyException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.</para>
        /// </summary>
        public FabricInvalidPartitionKeyException()
            : base()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidPartitionKeyException" /> class with a specified error code.</para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricInvalidPartitionKeyException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidPartitionKeyException" /> class with error code <see cref="FabricErrorCode.Unknown"/> and a specified error message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricInvalidPartitionKeyException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidPartitionKeyException" /> class with specified error message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricInvalidPartitionKeyException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidPartitionKeyException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricInvalidPartitionKeyException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidPartitionKeyException" /> class
        /// with a specified error message, a reference to the inner exception that is the cause of this exception, and a specified error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricInvalidPartitionKeyException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidPartitionKeyException" /> class from a serialized object data, with a specified context.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricInvalidPartitionKeyException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidPartitionKeyException" /> class from a serialized object data, with specified context and error code.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        protected FabricInvalidPartitionKeyException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
    }

    /// <summary>
    /// <para>The exception that indicates failure due to the existence of a conflicting entity.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricElementAlreadyExistsException : FabricException
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementAlreadyExistsException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.</para>
        /// </summary>
        public FabricElementAlreadyExistsException() :
            base()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementAlreadyExistsException" /> class with a specified error code.</para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricElementAlreadyExistsException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementAlreadyExistsException" /> class with error code <see cref="FabricErrorCode.Unknown"/> and a specified error message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricElementAlreadyExistsException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementAlreadyExistsException" /> class with specified error message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricElementAlreadyExistsException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricElementAlreadyExistsException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricElementAlreadyExistsException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricElementAlreadyExistsException" /> class
        /// with a specified error message, a reference to the inner exception that is the cause of this exception, and a specified error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricElementAlreadyExistsException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementAlreadyExistsException" /> class from the serialized object data, with a specified context.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricElementAlreadyExistsException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementAlreadyExistsException" /> class with a specified error code, with specified context and error code.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        protected FabricElementAlreadyExistsException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
    }


    /// <summary>
    /// <para>The exception that is thrown when a Service Fabric element is not available.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricElementNotFoundException : FabricException
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementNotFoundException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.</para>
        /// </summary>
        public FabricElementNotFoundException() :
            base()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementNotFoundException" /> class with a specified error code.</para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricElementNotFoundException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Runtime.Serialization.StreamingContext" /> class with error code <see cref="FabricErrorCode.Unknown"/> and a specified error message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricElementNotFoundException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementNotFoundException" /> class with specified message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricElementNotFoundException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Runtime.Serialization.StreamingContext" /> class with specified error message and inner exception.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricElementNotFoundException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementNotFoundException" /> class with specified message, inner exception and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricElementNotFoundException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementNotFoundException" /> class with specified info, context.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>

        protected FabricElementNotFoundException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricElementNotFoundException" /> class with specified info, context and error code.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        protected FabricElementNotFoundException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when the callee is not a primary.</para>
    /// </summary>
    /// <remarks>
    /// <para>The <see cref="System.Fabric.FabricNotPrimaryException" /> indicates that the operation cannot be performed because the callee is currently not a primary.
    /// For example, this exception can be observed if a secondary replica attempted to replicate an operation via 
    /// <see cref="System.Fabric.IStateReplicator.ReplicateAsync(System.Fabric.OperationData,System.Threading.CancellationToken,out System.Int64)" />. A likely scenario is that the replica is no longer the primary.</para>
    /// <para>
    /// Handling <see cref="System.Fabric.FabricNotPrimaryException"/> for <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-reliable-services-reliable-collections">Reliable Collections</see> :
    ///     1. If the service sees <see cref="System.Fabric.FabricNotPrimaryException" /> in <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.services.runtime.statefulservicebase.runasync?viewFallbackFrom=servicefabricsvcs#Microsoft_ServiceFabric_Services_Runtime_StatefulServiceBase_RunAsync_System_Threading_CancellationToken_">RunAsync</see>, 
    ///         it should catch the exception, complete all tasks and return from <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.services.runtime.statefulservicebase.runasync?viewFallbackFrom=servicefabricsvcs#Microsoft_ServiceFabric_Services_Runtime_StatefulServiceBase_RunAsync_System_Threading_CancellationToken_">RunAsync</see>. 
    ///         The <see cref="CancellationToken"/> passed to <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.services.runtime.statefulservicebase.runasync?viewFallbackFrom=servicefabricsvcs#Microsoft_ServiceFabric_Services_Runtime_StatefulServiceBase_RunAsync_System_Threading_CancellationToken_">RunAsync</see> would be signalled. All background tasks should complete execution when this cancellation is signalled.
    ///     2. If the service sees <see cref="System.Fabric.FabricNotPrimaryException" /> while processing a client request (e.g. via their communication listener), the service should throw the exception to the client to signal the client that it should re-resolve the service in order to locate the new Primary.
    /// </para>
    /// </remarks>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricNotPrimaryException : FabricException
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricNotPrimaryException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.</para>
        /// </summary>
        public FabricNotPrimaryException() :
            base()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricNotPrimaryException" /> class with a specified error code.</para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricNotPrimaryException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricNotPrimaryException" /> class with a specified message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricNotPrimaryException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricNotPrimaryException" /> class with specified message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricNotPrimaryException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricNotPrimaryException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricNotPrimaryException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricNotPrimaryException" /> class
        /// with a specified error message, a reference to the inner exception that is the cause of this exception, and a specified error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricNotPrimaryException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricNotPrimaryException" /> class with specified info, context.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricNotPrimaryException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricNotPrimaryException" /> class with specified info, context and error code.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        protected FabricNotPrimaryException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
    }

    /// <summary>
    /// <para>The exception that indicates failure of an operation due to a transient environmental or runtime circumstance.</para>
    /// <para>Handling <see cref="FabricTransientException"/> for <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-reliable-services-reliable-collections">Reliable Collections</see> :
    ///     The user is suggested to catch this exception, abort the transaction and retry all operations with a new Transaction
    /// </para>
    /// </summary>
    /// <remarks>
    /// <para>For example, an operation may fail because a quorum of replicas is temporarily not reachable. The <see cref="System.Fabric.FabricTransientException" /> corresponds to failed operations that can be tried again.</para>
    /// </remarks>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricTransientException : FabricException
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricTransientException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.</para>
        /// </summary>
        public FabricTransientException() :
            base()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricTransientException" /> class with a specified error code.</para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricTransientException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricTransientException" /> class with a specified message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricTransientException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricTransientException" /> class with specified message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricTransientException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricTransientException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricTransientException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricTransientException" /> class
        /// with a specified error message, a reference to the inner exception that is the cause of this exception, and a specified error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricTransientException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricTransientException" /> class with specified info, context.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricTransientException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricTransientException" /> class with specified info, context and error code.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        protected FabricTransientException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
    }

    /// <summary>
    /// <para>
    /// The exception that is thrown when the Service Fabric object is currently in a closed state due to one of the following conditions:
    ///     1. The Service Fabric object is being deleted.
    ///     2. The Service Fabric object is not reachable due to a failover.
    /// </para>
    /// </summary>
    /// <remarks>
    /// <para>For example, this exception can be observed when a service attempts to perform an operation on a Service Fabric or <see cref="System.Fabric.FabricReplicator" /> object while it is in the closed state. Another example is when an API is invoked on a <see cref="System.Fabric.FabricClient" /> object when it is in the closed state.</para>
    /// <para>
    /// Handling <see cref="System.Fabric.FabricObjectClosedException" /> for <see cref="System.Fabric.FabricClient"/> calls:
    ///     If a FabricClient call sees <see cref="System.Fabric.FabricObjectClosedException" />, see <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions">FabricClient Exception Handling</see> for handling common FabricClient failures.
    /// </para>
    /// <para>
    /// Handling <see cref="System.Fabric.FabricObjectClosedException"/> for <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-reliable-services-reliable-collections">Reliable Collections</see> :
    ///     1. If the service sees <see cref="System.Fabric.FabricObjectClosedException" /> in <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.services.runtime.statefulservicebase.runasync?viewFallbackFrom=servicefabricsvcs#Microsoft_ServiceFabric_Services_Runtime_StatefulServiceBase_RunAsync_System_Threading_CancellationToken_">RunAsync</see>, 
    ///         it should catch the exception and return from <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.services.runtime.statefulservicebase.runasync?viewFallbackFrom=servicefabricsvcs#Microsoft_ServiceFabric_Services_Runtime_StatefulServiceBase_RunAsync_System_Threading_CancellationToken_">RunAsync</see>.
    ///         The <see cref="CancellationToken"/> passed to <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.services.runtime.statefulservicebase.runasync?viewFallbackFrom=servicefabricsvcs#Microsoft_ServiceFabric_Services_Runtime_StatefulServiceBase_RunAsync_System_Threading_CancellationToken_">RunAsync</see> would be signalled. All background tasks should complete execution when this cancellation is signalled.
    ///     2. If the service sees <see cref="System.Fabric.FabricObjectClosedException" /> while processing a client request (e.g. via their communication listener), the service should throw the exception to the client to signal the client that it should re-resolve the service in order to locate the new Primary.
    /// 
    /// [NOTE] If an <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.data.ireliablestate?view=azure-dotnet">IReliableState</see> was removed via <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.data.ireliablestatemanager.removeasync?view=azure-dotnet#Microsoft_ServiceFabric_Data_IReliableStateManager_RemoveAsync_Microsoft_ServiceFabric_Data_ITransaction_System_Uri_System_TimeSpan_">IReliableStateManager.RemoveAsync()</see>,
    /// any calls trying to access this <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.data.ireliablestate?view=azure-dotnet">IReliableState</see> would see <see cref="System.Fabric.FabricObjectClosedException"/>. These calls needs to be synchronized with the <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.data.ireliablestatemanager.removeasync?view=azure-dotnet#Microsoft_ServiceFabric_Data_IReliableStateManager_RemoveAsync_Microsoft_ServiceFabric_Data_ITransaction_System_Uri_System_TimeSpan_">IReliableStateManager.RemoveAsync()</see> call and should know that the <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.data.ireliablestate?view=azure-dotnet">IReliableState</see> has been removed.
    /// Possible ways to handle this case are:
    ///     1. Recreate the <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.data.ireliablestate?view=azure-dotnet">IReliableState</see> if it was removed and retry the operation.
    ///     2. Ignore the <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.data.ireliablestate?view=azure-dotnet">IReliableState</see> and process other <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.data.ireliablestate?view=azure-dotnet">IReliableState</see> in the service.
    ///     3. Use locks to avoid the race. Hence if a remove call comes in, the user can stop processing the <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.data.ireliablestate?view=azure-dotnet">IReliableState</see> further.
    /// </para>
    /// </remarks>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricObjectClosedException : FabricException
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricObjectClosedException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.</para>
        /// </summary>
        public FabricObjectClosedException() :
            base()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricObjectClosedException" /> class with a specified error code.</para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricObjectClosedException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricObjectClosedException" /> class with error code <see cref="FabricErrorCode.Unknown"/> and a specified error message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricObjectClosedException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricObjectClosedException" /> class.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricObjectClosedException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricObjectClosedException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricObjectClosedException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricObjectClosedException" /> class
        /// with a specified error message, a reference to the inner exception that is the cause of this exception, and a specified error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricObjectClosedException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricObjectClosedException" /> class from a serialized object data, with a specified context.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>

        protected FabricObjectClosedException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricObjectClosedException" /> class from a serialized object data, with specified context and error code.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        protected FabricObjectClosedException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when a connection request is denied by Service Fabric cluster or server.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricConnectionDeniedException : FabricException
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricConnectionDeniedException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.
        /// </para>
        /// </summary>
        public FabricConnectionDeniedException() :
            base()
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricConnectionDeniedException" /> class with a specified error code.
        /// </para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricConnectionDeniedException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricConnectionDeniedException" /> class with error code <see cref="FabricErrorCode.Unknown"/> and a specified error message.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricConnectionDeniedException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricConnectionDeniedException" /> class with specified error message and error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricConnectionDeniedException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricConnectionDeniedException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricConnectionDeniedException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricConnectionDeniedException" /> class
        /// with a specified error message, a reference to the inner exception that is the cause of this exception, and a specified error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricConnectionDeniedException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricConnectionDeniedException" /> class from a serialized object data, with a specified context.
        /// </para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricConnectionDeniedException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricConnectionDeniedException" /> class from a serialized object data, with specified context and error code.
        /// </para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        protected FabricConnectionDeniedException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
    }

    /// <summary>
    /// <para>The exception that indicates a failed authentication of cluster or server identity.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricServerAuthenticationFailedException : FabricException
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricServerAuthenticationFailedException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.
        /// </para>
        /// </summary>
        public FabricServerAuthenticationFailedException() :
            base()
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricServerAuthenticationFailedException" /> class with a specified error code.
        /// </para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricServerAuthenticationFailedException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricServerAuthenticationFailedException" /> class with error code <see cref="FabricErrorCode.Unknown"/> and a specified error message.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricServerAuthenticationFailedException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricServerAuthenticationFailedException" /> class with specified error message and error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricServerAuthenticationFailedException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricServerAuthenticationFailedException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricServerAuthenticationFailedException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricServerAuthenticationFailedException" /> class
        /// with a specified error message, a reference to the inner exception that is the cause of this exception, and a specified error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricServerAuthenticationFailedException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricServerAuthenticationFailedException" /> class from a serialized object data, with a specified context.
        /// </para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricServerAuthenticationFailedException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricServerAuthenticationFailedException" /> class from a serialized object data, with specified context and error code.
        /// </para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        protected FabricServerAuthenticationFailedException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when an address is not recognized by Service Fabric.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public sealed class FabricInvalidAddressException : FabricException
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidAddressException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.
        /// </para>
        /// </summary>
        public FabricInvalidAddressException()
            : base()
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidAddressException" /> class with a specified error code.
        /// </para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricInvalidAddressException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidAddressException" /> class with error code <see cref="FabricErrorCode.Unknown"/> and a specified error message.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricInvalidAddressException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidAddressException" /> class with specified error message and error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricInvalidAddressException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidAddressException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricInvalidAddressException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidAddressException" /> class
        /// with a specified error message, a reference to the inner exception that is the cause of this exception, and a specified error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricInvalidAddressException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

#pragma warning disable 0628 // protected member in sealed type. protected ctor is required by the serialization
        /// <summary>
        /// Initializes a new instance of <see cref="FabricInvalidAddressException"/> class from a serialized object data, with a specified context.
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        private FabricInvalidAddressException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }
#pragma warning restore 0628

#pragma warning disable 0628 // protected member in sealed type. protected ctor is required by the serialization
        /// <summary>
        /// Initializes a new instance of <see cref="FabricInvalidAddressException"/> class from a serialized object data, with specified context and error code.
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        private FabricInvalidAddressException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
#pragma warning restore 0628
    }

    /// <summary>
    /// <para>The exception that is thrown when the Service Fabric atomic group is invalid.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricInvalidAtomicGroupException : FabricException
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidAtomicGroupException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.</para>
        /// </summary>
        public FabricInvalidAtomicGroupException() :
            base()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidAtomicGroupException" /> class with a specified error code.</para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricInvalidAtomicGroupException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidAtomicGroupException" /> with a specified error message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricInvalidAtomicGroupException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidAtomicGroupException" /> class with specified message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricInvalidAtomicGroupException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidAtomicGroupException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricInvalidAtomicGroupException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidAtomicGroupException" /> class
        /// with a specified error message, a reference to the inner exception that is the cause of this exception, and a specified error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricInvalidAtomicGroupException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidAtomicGroupException" /> class from a serialized object data, with a specified context.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricInvalidAtomicGroupException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidAtomicGroupException" /> class from a serialized object data, with specified context and error code.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        protected FabricInvalidAtomicGroupException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
    }

    /// <summary>
    /// <para>
    /// The exception that is thrown when an attempt is made to create an incremental backup of the key-value store
    /// before an initial full backup is created.
    /// </para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricMissingFullBackupException : FabricException
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricMissingFullBackupException" /> class with error code <see cref="FabricErrorCode.MissingFullBackup"/>.
        /// </para>
        /// </summary>
        public FabricMissingFullBackupException() : base(FabricErrorCode.MissingFullBackup)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricMissingFullBackupException" /> class with error code <see cref="FabricErrorCode.MissingFullBackup"/> and a specified error message.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricMissingFullBackupException(string message)
            : base(message, FabricErrorCode.MissingFullBackup)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricMissingFullBackupException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricMissingFullBackupException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.MissingFullBackup)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricMissingFullBackupException" /> class from a serialized object data, with a specified context.
        /// </para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricMissingFullBackupException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.MissingFullBackup)
        {
        }
    }

    /// <summary>
    /// <para>
    /// The exception that is thrown when a service partition or a replica cannot accept reads.
    /// </para>
    /// </summary>
    /// <remarks>
    /// <para>
    /// The exception can be seen in the following 2 scenarios :
    ///     1. The partition does not have a read quorum.
    ///     2. The service is trying to read from an <see href="https://msdn.microsoft.com/library/azure/dn707635.aspx">IdleSecondary replica</see>.
    /// </para>
    /// <para>
    /// Handling <see cref="System.Fabric.FabricNotReadableException"/> for <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-reliable-services-reliable-collections">Reliable Collections</see> :
    /// If <see cref="System.Fabric.FabricNotReadableException" /> is seen by the service or a client call, the exception should be caught, current transaction should be disposed and all the operations should be retried with a new transaction object.
    /// Read status will eventually be granted or a non-retriable exception will be thrown. An optional backoff can be added before retrying.
    /// </para>
    /// </remarks>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricNotReadableException : FabricTransientException
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricNotReadableException" /> class with error code <see cref="FabricErrorCode.Unknown"/>.</para>
        /// </summary>
        public FabricNotReadableException()
            : base(FabricErrorCode.NotReadable)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricNotReadableException" /> class with error code <see cref="FabricErrorCode.Unknown"/> and a specified error message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricNotReadableException(string message)
            : base(message, FabricErrorCode.NotReadable)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricNotReadableException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricNotReadableException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.NotReadable)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricNotReadableException" /> class from a serialized object data, with a specified context.</para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricNotReadableException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.NotReadable)
        {
        }
    }

    /// <summary>
    /// <para>
    /// The exception that is thrown when perform ImageStore operations.
    /// </para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricImageStoreException : FabricException
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricImageStoreException" /> class with error code <see cref="FabricErrorCode.ImageStoreIOException"/>.
        /// </para>
        /// </summary>
        public FabricImageStoreException()
            : this(FabricErrorCode.ImageStoreIOException)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricImageStoreException" /> class with a specified error code.
        /// </para>
        /// </summary>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricImageStoreException(FabricErrorCode errorCode)
            : base(errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricImageStoreException" /> class with error code <see cref="FabricErrorCode.ImageStoreIOException"/> and a specified error message.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricImageStoreException(string message)
            : this(message, FabricErrorCode.ImageStoreIOException)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricImageStoreException" /> class with specified error message and error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricImageStoreException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricImageStoreException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricImageStoreException(string message, Exception inner)
            : this(message, inner, FabricErrorCode.ImageStoreIOException)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricImageStoreException" /> class
        /// with a specified error message, a reference to the inner exception that is the cause of this exception, and a specified error code.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricImageStoreException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricImageStoreException" /> class from a serialized object data, with a specified context.
        /// </para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricImageStoreException(SerializationInfo info, StreamingContext context)
            : this(info, context, FabricErrorCode.ImageStoreIOException)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricImageStoreException" /> class from a serialized object data, with specified context and error code.
        /// </para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        protected FabricImageStoreException(SerializationInfo info, StreamingContext context, FabricErrorCode errorCode)
            : base(info, context, errorCode)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when an attempt is made to initiate a backup while a previously initiated backup is still in progress.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricBackupInProgressException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of <see cref="FabricBackupInProgressException"/> class with error code <see cref="FabricErrorCode.BackupInProgress"/>.
        /// </summary>
        public FabricBackupInProgressException()
            : base(FabricErrorCode.BackupInProgress)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the FabricBackupInProgressException class with error code <see cref="FabricErrorCode.BackupInProgress"/> and a specified error message.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricBackupInProgressException(string message)
            : base(message, FabricErrorCode.BackupInProgress)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of the <see cref="FabricBackupInProgressException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricBackupInProgressException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.BackupInProgress)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="FabricBackupInProgressException" /> class from a serialized object data, with a specified context.
        /// </para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricBackupInProgressException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.BackupInProgress)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when a user-provided backup directory is not empty.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricBackupDirectoryNotEmptyException : FabricException
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricBackupDirectoryNotEmptyException" /> class with error code <see cref="FabricErrorCode.BackupDirectoryNotEmpty"/>.
        /// </para>
        /// </summary>
        public FabricBackupDirectoryNotEmptyException()
            : base(FabricErrorCode.BackupDirectoryNotEmpty)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricBackupDirectoryNotEmptyException" /> class with error code <see cref="FabricErrorCode.BackupDirectoryNotEmpty"/> and a specified error message.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricBackupDirectoryNotEmptyException(string message)
            : base(message, FabricErrorCode.BackupDirectoryNotEmpty)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricBackupDirectoryNotEmptyException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricBackupDirectoryNotEmptyException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.BackupDirectoryNotEmpty)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricBackupDirectoryNotEmptyException" /> class from a serialized object data, with a specified context.
        /// </para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricBackupDirectoryNotEmptyException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.BackupDirectoryNotEmpty)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when the replication operation is larger than the configured limit.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricReplicationOperationTooLargeException : FabricException
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricReplicationOperationTooLargeException" /> class with error code <see cref="FabricErrorCode.ReplicationOperationTooLarge"/>.
        /// </para>
        /// </summary>
        public FabricReplicationOperationTooLargeException()
            : base(FabricErrorCode.ReplicationOperationTooLarge)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricReplicationOperationTooLargeException" /> class with error code <see cref="FabricErrorCode.ReplicationOperationTooLarge"/> and a specified error message.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricReplicationOperationTooLargeException(string message)
            : base(message, FabricErrorCode.ReplicationOperationTooLarge)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricReplicationOperationTooLargeException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricReplicationOperationTooLargeException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.ReplicationOperationTooLarge)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricReplicationOperationTooLargeException" /> class from a serialized object data, with a specified context.
        /// </para>
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricReplicationOperationTooLargeException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.ReplicationOperationTooLarge)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when there is no service found by the specified name.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public sealed class FabricServiceNotFoundException : FabricElementNotFoundException
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricServiceNotFoundException" /> class with error code <see cref="FabricErrorCode.ServiceNotFound"/>.
        /// </para>
        /// </summary>
        public FabricServiceNotFoundException()
            : base(FabricErrorCode.ServiceNotFound)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricServiceNotFoundException" /> class with error code <see cref="FabricErrorCode.ServiceNotFound"/> and a specified error message.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricServiceNotFoundException(string message)
            : base(message, FabricErrorCode.ServiceNotFound)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricServiceNotFoundException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricServiceNotFoundException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.ServiceNotFound)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="FabricServiceNotFoundException"/> class from a serialized object data, with a specified context.
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        private FabricServiceNotFoundException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.ServiceNotFound)
        {
        }
    }

    /// <summary>
    /// <para>The exception that indicates that there is CannotConnect Error.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public sealed class FabricCannotConnectException : FabricException
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricCannotConnectException" /> class with error code <see cref="FabricErrorCode.FabricCannotConnect"/>.
        /// </para>
        /// </summary>
        public FabricCannotConnectException()
            : base(FabricErrorCode.FabricCannotConnect)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricCannotConnectException" /> class with error code <see cref="FabricErrorCode.FabricCannotConnect"/> and a specified error message.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricCannotConnectException(string message)
            : base(message, FabricErrorCode.FabricCannotConnect)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricCannotConnectException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>The error code associated with the exception.</para>
        /// </param>
        public FabricCannotConnectException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricCannotConnectException"/> class from a serialized object data, with a specified context.
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        private FabricCannotConnectException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.FabricCannotConnect)
        {
        }
    }

    /// <summary>
    /// <para>The exception that indicates the message is too large.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public sealed class FabricMessageTooLargeException : FabricException
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricMessageTooLargeException" /> class with error code <see cref="FabricErrorCode.FabricMessageTooLarge"/>.
        /// </para>
        /// </summary>
        public FabricMessageTooLargeException()
            : base(FabricErrorCode.FabricMessageTooLarge)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricMessageTooLargeException" /> class with error code <see cref="FabricErrorCode.FabricMessageTooLarge"/> and a specified error message.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricMessageTooLargeException(string message)
            : base(message, FabricErrorCode.FabricMessageTooLarge)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricMessageTooLargeException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricMessageTooLargeException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.FabricMessageTooLarge)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricMessageTooLargeException"/> class from a serialized object data, with a specified context.
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        private FabricMessageTooLargeException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.FabricMessageTooLarge)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when the specified endpoint is not found.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public sealed class FabricEndpointNotFoundException : FabricException
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricEndpointNotFoundException" /> class with error code <see cref="FabricErrorCode.FabricEndpointNotFound"/>.
        /// </para>
        /// </summary>
        public FabricEndpointNotFoundException()
            : base(FabricErrorCode.FabricEndpointNotFound)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricEndpointNotFoundException" /> class with error code <see cref="FabricErrorCode.FabricEndpointNotFound"/> and a specified error message.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricEndpointNotFoundException(string message)
            : base(message, FabricErrorCode.FabricEndpointNotFound)
        {
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of <see cref="System.Fabric.FabricEndpointNotFoundException" /> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricEndpointNotFoundException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.FabricEndpointNotFound)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricEndpointNotFoundException"/> class from a serialized object data, with a specified context.
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        private FabricEndpointNotFoundException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.FabricEndpointNotFound)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when deletion of a file or a directory fails during backup.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricDeleteBackupFileFailedException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricDeleteBackupFileFailedException"/> class with error code <see cref="FabricErrorCode.DeleteBackupFileFailed"/>.
        /// </summary>
        public FabricDeleteBackupFileFailedException() : base(FabricErrorCode.DeleteBackupFileFailed)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricDeleteBackupFileFailedException"/> class with error code <see cref="FabricErrorCode.DeleteBackupFileFailed"/> and a specified error message.
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricDeleteBackupFileFailedException(string message)
            : base(message, FabricErrorCode.DeleteBackupFileFailed)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricDeleteBackupFileFailedException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricDeleteBackupFileFailedException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.DeleteBackupFileFailed)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricDeleteBackupFileFailedException"/> class from a serialized object data, with a specified context.
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricDeleteBackupFileFailedException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.DeleteBackupFileFailed)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when an operation is not valid for a test command in a particular state.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricInvalidTestCommandStateException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidTestCommandStateException"/> class with error code <see cref="FabricErrorCode.InvalidTestCommandState"/>.
        /// </summary>
        public FabricInvalidTestCommandStateException()
            : base(FabricErrorCode.InvalidTestCommandState)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidTestCommandStateException"/> class with error code <see cref="FabricErrorCode.InvalidTestCommandState"/> and a specified error message.
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricInvalidTestCommandStateException(string message)
            : base(message, FabricErrorCode.InvalidTestCommandState)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidTestCommandStateException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricInvalidTestCommandStateException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.InvalidTestCommandState)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricInvalidTestCommandStateException"/> class from a serialized object data, with a specified context.
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricInvalidTestCommandStateException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.InvalidTestCommandState)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when a test command already exists, i.e. when there is a duplicate operation identifier.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricTestCommandOperationIdAlreadyExistsException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricTestCommandOperationIdAlreadyExistsException"/> class with error code <see cref="FabricErrorCode.TestCommandOperationIdAlreadyExists"/>.
        /// </summary>
        public FabricTestCommandOperationIdAlreadyExistsException()
            : base(FabricErrorCode.TestCommandOperationIdAlreadyExists)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricTestCommandOperationIdAlreadyExistsException"/> class with error code <see cref="FabricErrorCode.TestCommandOperationIdAlreadyExists"/> and a specified error message.
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricTestCommandOperationIdAlreadyExistsException(string message)
            : base(message, FabricErrorCode.TestCommandOperationIdAlreadyExists)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricTestCommandOperationIdAlreadyExistsException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricTestCommandOperationIdAlreadyExistsException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.TestCommandOperationIdAlreadyExists)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricTestCommandOperationIdAlreadyExistsException"/> class from a serialized object data, with a specified context.
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricTestCommandOperationIdAlreadyExistsException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.TestCommandOperationIdAlreadyExists)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when there is an attempt to create a new instance of Service Fabric's built-in Chaos Test service, while the service is already running in the cluster.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricChaosAlreadyRunningException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricChaosAlreadyRunningException"/> class with error code <see cref="FabricErrorCode.ChaosAlreadyRunning"/>.
        /// </summary>
        public FabricChaosAlreadyRunningException()
            : base(FabricErrorCode.ChaosAlreadyRunning)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricChaosAlreadyRunningException"/> class with error code <see cref="FabricErrorCode.ChaosAlreadyRunning"/> and a specified error message.
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricChaosAlreadyRunningException(string message)
            : base(message, FabricErrorCode.ChaosAlreadyRunning)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricChaosAlreadyRunningException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricChaosAlreadyRunningException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.ChaosAlreadyRunning)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricChaosAlreadyRunningException"/> class from a serialized object data, with a specified context.
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricChaosAlreadyRunningException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.ChaosAlreadyRunning)
        {
        }
    }

    /// <summary>
    /// <para>The exception that is thrown when Service Fabric's built-in Chaos Test service has encountered something unexpected.</para>
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricChaosEngineException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricChaosEngineException"/> class with error code <see cref="FabricErrorCode.ChaosAlreadyRunning"/>.
        /// </summary>
        public FabricChaosEngineException()
            : base(FabricErrorCode.ChaosAlreadyRunning)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricChaosEngineException"/> class with error code <see cref="FabricErrorCode.ChaosAlreadyRunning"/> and a specified error message.
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        public FabricChaosEngineException(string message)
            : base(message, FabricErrorCode.ChaosAlreadyRunning)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricChaosEngineException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="inner">
        /// <para>The exception that is the cause of the current exception or null if no inner exception is specified. The <see cref="System.Exception" /> class provides more details about the inner exception.</para>
        /// </param>
        public FabricChaosEngineException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.ChaosAlreadyRunning)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="System.Fabric.FabricChaosEngineException"/> class from a serialized object data, with a specified context.
        /// </summary>
        /// <param name="info">
        /// <para>The <see cref="System.Runtime.Serialization.SerializationInfo" /> object that contains serialized object data of the exception being thrown.</para>
        /// </param>
        /// <param name="context">
        /// <para>The <see cref="System.Runtime.Serialization.StreamingContext" /> object that contains contextual information about the source or destination. The context parameter is reserved for future use and can be null.</para>
        /// </param>
        protected FabricChaosEngineException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.ChaosAlreadyRunning)
        {
        }
    }

    /// <summary>
    /// The exception that is thrown when Backup is too old to be used for restore
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricRestoreSafeCheckFailedException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricRestoreSafeCheckFailedException"/> class.
        /// </summary>
        public FabricRestoreSafeCheckFailedException()
            : base(FabricErrorCode.RestoreSafeCheckFailed)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricRestoreSafeCheckFailedException"/> class with a specified error message.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        public FabricRestoreSafeCheckFailedException(string message)
            : base(message, FabricErrorCode.RestoreSafeCheckFailed)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricRestoreSafeCheckFailedException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="inner">The exception that is the cause of the current exception, or a null reference if no inner exception is specified.</param>
        public FabricRestoreSafeCheckFailedException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.RestoreSafeCheckFailed)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricRestoreSafeCheckFailedException"/> class with serialized data.
        /// </summary>
        /// <param name="info">The <see cref="System.Runtime.Serialization.SerializationInfo"/> that holds the serialized object data about the exception being thrown. </param>
        /// <param name="context">The <see cref="System.Runtime.Serialization.StreamingContext"/> that contains contextual information about the source or destination.</param>
        protected FabricRestoreSafeCheckFailedException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.RestoreSafeCheckFailed)
        {
        }
    }

    /// <summary>
    /// The exception that is thrown when a PartitionSelector is invalid.
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricInvalidPartitionSelectorException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidPartitionSelectorException"/> class.
        /// </summary>
        public FabricInvalidPartitionSelectorException() : base(FabricErrorCode.InvalidPartitionSelector)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidPartitionSelectorException"/> class with a specified error message.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        public FabricInvalidPartitionSelectorException(string message)
            : base(message, FabricErrorCode.InvalidPartitionSelector)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidPartitionSelectorException" /> with specified message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> defines the error code that the exception is wrapping around.</para>
        /// </param>
        public FabricInvalidPartitionSelectorException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidPartitionSelectorException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="inner">The exception that is the cause of the current exception, or a null reference if no inner exception is specified.</param>
        public FabricInvalidPartitionSelectorException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.InvalidPartitionSelector)
        {
        }
    }

    /// <summary>
    /// The exception that is thrown when a ReplicaSelector is invalid.
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricInvalidReplicaSelectorException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidReplicaSelectorException"/> class.
        /// </summary>
        public FabricInvalidReplicaSelectorException() : base(FabricErrorCode.InvalidReplicaSelector)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidReplicaSelectorException"/> class with a specified error message.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        public FabricInvalidReplicaSelectorException(string message)
            : base(message, FabricErrorCode.InvalidReplicaSelector)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidReplicaSelectorException" /> with specified message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> defines the error code that the exception is wrapping around.</para>
        /// </param>
        public FabricInvalidReplicaSelectorException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidReplicaSelectorException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="inner">The exception that is the cause of the current exception, or a null reference if no inner exception is specified.</param>
        public FabricInvalidReplicaSelectorException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.InvalidReplicaSelector)
        {
        }
    }


    /// <summary>
    /// The exception that is thrown when an operation is only valid for stateless services.
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricInvalidForStatefulServicesException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidForStatefulServicesException"/> class.
        /// </summary>
        public FabricInvalidForStatefulServicesException() : base(FabricErrorCode.InvalidForStatefulServices)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidForStatefulServicesException"/> class with a specified error message.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        public FabricInvalidForStatefulServicesException(string message)
            : base(message, FabricErrorCode.InvalidForStatefulServices)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidForStatefulServicesException" /> with specified message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> defines the error code that the exception is wrapping around.</para>
        /// </param>
        public FabricInvalidForStatefulServicesException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidForStatefulServicesException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="inner">The exception that is the cause of the current exception, or a null reference if no inner exception is specified.</param>
        public FabricInvalidForStatefulServicesException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.InvalidForStatefulServices)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidForStatefulServicesException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="inner">The exception that is the cause of the current exception, or a null reference if no inner exception is specified.</param>
        /// <param name="errorCode">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> defines the error code that the exception is wrapping around.</para>
        /// </param>
        public FabricInvalidForStatefulServicesException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }
    }

    /// <summary>
    /// The exception that is thrown when an operation is only valid for stateful services.
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricInvalidForStatelessServicesException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidForStatelessServicesException"/> class.
        /// </summary>
        public FabricInvalidForStatelessServicesException() : base(FabricErrorCode.InvalidForStatelessServices)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidForStatelessServicesException"/> class with a specified error message.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        public FabricInvalidForStatelessServicesException(string message)
            : base(message, FabricErrorCode.InvalidForStatelessServices)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricInvalidForStatelessServicesException" /> with specified message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> defines the error code that the exception is wrapping around.</para>
        /// </param>
        public FabricInvalidForStatelessServicesException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidForStatelessServicesException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="inner">The exception that is the cause of the current exception, or a null reference if no inner exception is specified.</param>
        public FabricInvalidForStatelessServicesException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.InvalidForStatelessServices)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricInvalidForStatelessServicesException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="inner">The exception that is the cause of the current exception, or a null reference if no inner exception is specified.</param>
        /// <param name="errorCode">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> defines the error code that the exception is wrapping around.</para>
        /// </param>
        public FabricInvalidForStatelessServicesException(string message, Exception inner, FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }
    }

    /// <summary>
    /// The exception that is thrown when an operation is only valid for stateful persistent services.
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricOnlyValidForStatefulPersistentServicesException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricOnlyValidForStatefulPersistentServicesException"/> class.
        /// </summary>
        public FabricOnlyValidForStatefulPersistentServicesException()
            : base(FabricErrorCode.OnlyValidForStatefulPersistentServices)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricOnlyValidForStatefulPersistentServicesException"/> class with a specified error message.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        public FabricOnlyValidForStatefulPersistentServicesException(string message)
            : base(message, FabricErrorCode.OnlyValidForStatefulPersistentServices)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.FabricOnlyValidForStatefulPersistentServicesException" /> with specified message and error code.</para>
        /// </summary>
        /// <param name="message">
        /// <para>The error message that explains the reason for the exception.</para>
        /// </param>
        /// <param name="errorCode">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> defines the error code that the exception is wrapping around.</para>
        /// </param>
        public FabricOnlyValidForStatefulPersistentServicesException(string message, FabricErrorCode errorCode)
            : base(message, errorCode)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricOnlyValidForStatefulPersistentServicesException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="inner">The exception that is the cause of the current exception, or a null reference if no inner exception is specified.</param>
        public FabricOnlyValidForStatefulPersistentServicesException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.OnlyValidForStatefulPersistentServices)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricOnlyValidForStatefulPersistentServicesException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="inner">The exception that is the cause of the current exception, or a null reference if no inner exception is specified.</param>
        /// <param name="errorCode">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" />defines the error code that the exception is wrapping around.</para>
        /// </param>
        public FabricOnlyValidForStatefulPersistentServicesException(string message, Exception inner,
            FabricErrorCode errorCode)
            : base(message, inner, errorCode)
        {
        }
    }

    /// <summary>
    /// The exception that is thrown trying to get backup scheduling policy for a partition for which backup is not enabled
    /// </summary>
    [Serializable]
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Exception")]
    public class FabricPeriodicBackupNotEnabledException : FabricException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FabricPeriodicBackupNotEnabledException"/> class.
        /// </summary>
        public FabricPeriodicBackupNotEnabledException()
            : base(StringResources.Error_BackupNotEnabled, FabricErrorCode.BackupNotEnabled)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FabricPeriodicBackupNotEnabledException"/> class with a specified error message.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        public FabricPeriodicBackupNotEnabledException(string message)
            : base(message, FabricErrorCode.BackupNotEnabled)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FabricPeriodicBackupNotEnabledException"/> class
        /// with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="inner">The exception that is the cause of the current exception, or a null reference if no inner exception is specified.</param>
        public FabricPeriodicBackupNotEnabledException(string message, Exception inner)
            : base(message, inner, FabricErrorCode.BackupNotEnabled)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FabricPeriodicBackupNotEnabledException"/> class with serialized data.
        /// </summary>
        /// <param name="info">The <see cref="SerializationInfo"/> that holds the serialized object data about the exception being thrown. </param>
        /// <param name="context">The <see cref="StreamingContext"/> that contains contextual information about the source or destination.</param>
        protected FabricPeriodicBackupNotEnabledException(SerializationInfo info, StreamingContext context)
            : base(info, context, FabricErrorCode.BackupNotEnabled)
        {
        }
    }

#if !DotNetCoreClr
    [Serializable]
#endif
    internal class FabricImageBuilderReservedDirectoryException : FabricException
    {
        private string fileName;

        public FabricImageBuilderReservedDirectoryException()
        {
        }

        public FabricImageBuilderReservedDirectoryException(string message, FabricErrorCode errorCode = FabricErrorCode.ImageBuilderReservedDirectoryError)
            : this(message, string.Empty, errorCode)
        {
        }

        public FabricImageBuilderReservedDirectoryException(string message, string fileName, FabricErrorCode errorCode = FabricErrorCode.ImageBuilderReservedDirectoryError)
            : base(message, errorCode)
        {
            this.fileName = fileName;
        }

        public FabricImageBuilderReservedDirectoryException(string message, Exception inner, FabricErrorCode errorCode = FabricErrorCode.ImageBuilderReservedDirectoryError)
            : this(message, string.Empty, inner, errorCode)
        {
        }

        public FabricImageBuilderReservedDirectoryException(string message, string fileName, Exception inner, FabricErrorCode errorCode = FabricErrorCode.ImageBuilderReservedDirectoryError)
            : base(message, inner, errorCode)
        {
            this.fileName = fileName;
        }

#if !DotNetCoreClr
        protected FabricImageBuilderReservedDirectoryException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
            this.fileName = info.GetString("FileName");
        }
#endif

        public override string Message
        {
            get
            {
                string message = base.Message;
                if (!string.IsNullOrEmpty(this.fileName))
                {
                    message = message + Environment.NewLine + string.Format(System.Globalization.CultureInfo.InvariantCulture, "FileName: {0}", this.fileName);
                }

                return message;
            }
        }
    }
}