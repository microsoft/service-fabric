// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>This wrapper contains the updated <see cref="System.Fabric.ResolvedServicePartition" />. </para>
    /// </summary>
    /// <remarks>
    /// <para>If there was an exception thrown while the newer <see cref="System.Fabric.ResolvedServicePartition" /> is acquired, then the 
    /// <see cref="System.Fabric.ServicePartitionResolutionChange" /> also contains the exception. Note that if the <see cref="System.Fabric.ServicePartitionResolutionChange.Exception" /> property 
    /// is not null, then the <see cref="System.Fabric.ServicePartitionResolutionChange.Result" /> property is null.</para>
    /// </remarks>
    public sealed class ServicePartitionResolutionChange
    {
        private readonly ResolvedServicePartition result;

        private ServicePartitionResolutionChange(ResolvedServicePartition result, Exception exception)
        {
            this.Exception = exception;
            this.result = result;
        }

        /// <summary>
        /// <para>Gets the exception that was thrown while the relevant <see cref="System.Fabric.ResolvedServicePartition" /> was being acquired or updated.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Exception" />.</para>
        /// </value>
        public Exception Exception { get; private set; }

        /// <summary>
        /// <para>Contains the new <see cref="System.Fabric.ResolvedServicePartition" /> that is relevant for the registered service partition.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.ResolvedServicePartition" />.</para>
        /// </value>
        public ResolvedServicePartition Result
        {
            get
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                return this.result;
            }
        }

        /// <summary>
        /// <para>Indicates whether there was an exception. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Boolean" />.</para>
        /// </value>
        /// <remarks>
        /// <para>If so, the <see cref="System.Fabric.ServicePartitionResolutionChange.Result" /> parameter is null and the <see cref="System.Fabric.ServicePartitionResolutionChange.Exception" /> 
        /// parameter is set.</para>
        /// </remarks>
        public bool HasException
        {
            get
            {
                return this.Exception != null;
            }
        }

        internal static unsafe ServicePartitionResolutionChange FromNative(NativeClient.IFabricResolvedServicePartitionResult nativeResult, int error)
        {
            Exception exception = null;
            ResolvedServicePartition result = null;
            if (nativeResult == null)
            {
                exception = Utility.TranslateCOMExceptionToManaged(new ExceptionWrapper(error), "ServicePartitionResolutionChange");
            }
            else
            {
                result = ResolvedServicePartition.FromNative(nativeResult);
            }

            var args = new ServicePartitionResolutionChange(result, exception);
            return args;
        }

        private sealed class ExceptionWrapper : COMException
        {
            public ExceptionWrapper(int errorCode)
                : base(StringResources.Error_AddressDetectionFailure, errorCode)
            {
            }
        }
    }
}