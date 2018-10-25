// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Specifies the batch that contains the results from the <see cref="System.Fabric.FabricClient.PropertyManagementClient.SubmitPropertyBatchAsync(System.Uri,System.Collections.Generic.ICollection{System.Fabric.PropertyBatchOperation},System.TimeSpan,System.Threading.CancellationToken)" /> method call.</para>
    /// </summary>
    public sealed class PropertyBatchResult
    {
        private readonly NativeClient.IFabricPropertyBatchResult innerBatch;
        private readonly FabricClient fabricClient;

        private PropertyBatchResult(NativeClient.IFabricPropertyBatchResult innerBatch, int failedOperationIndex, Exception failedOperationException, FabricClient client)
        {
            this.innerBatch = innerBatch;
            this.fabricClient = client;
            this.FailedOperationIndex = failedOperationIndex;
            this.FailedOperationException = failedOperationException;
        }

        /// <summary>
        /// <para>Gets the failed operation index. This parameter contains the index of the 
        /// unsuccessful <see cref="System.Fabric.PropertyBatchOperation" /> in the batch.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Int32" />.</para>
        /// </value>
        /// <remarks>
        /// <para>Note that if none of the operations in the batch fail, this property will be set to -1.</para>
        /// </remarks>
        public int FailedOperationIndex { get; private set; }

        /// <summary>
        /// <para>Gets the failed operation exception. This parameter contains the exception thrown due to the first 
        /// unsuccessful <see cref="System.Fabric.PropertyBatchOperation" /> object in the batch.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Exception" />.</para>
        /// </value>
        public Exception FailedOperationException { get; private set; }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.NamedProperty" /> object that is returned by the <see cref="System.Fabric.PropertyBatchOperation" /> in the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>An <see cref="System.Int32" /> that indicates the index in the batch that was submitted.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.NamedProperty" />.</para>
        /// </returns>
        /// <remarks>
        /// <para>Note that whether <see cref="System.Fabric.NamedPropertyMetadata" /> is returned is dependent on the <languagekeyword>includeValues</languagekeyword> 
        /// argument to the <see cref="System.Fabric.GetPropertyOperation" />. Returns an error when the operation has a different type than specified.</para>
        /// </remarks>
        public NamedProperty GetProperty(int index)
        {
            if (index < 0)
            {
                throw new ArgumentOutOfRangeException("index", index, StringResources.Error_NegativeIndex);
            }

            if (this.FailedOperationException != null)
            {
                throw this.FailedOperationException;
            }

            return Utility.WrapNativeSyncInvokeInMTA(() => this.InternalGetProperty(index), "PropertyBatchResult.GetProperty");
        }

        internal static PropertyBatchResult FromNative(NativeClient.IFabricPropertyBatchResult innerBatch, FabricClient fabricClient)
        {
            if (innerBatch == null)
            {
                return null;
            }
            else
            {
                var result = new PropertyBatchResult(innerBatch, -1, null, fabricClient);
                return result;
            }
        }

        internal static PropertyBatchResult CreateFailedResult(uint failedOperationIndex, Exception failedOperationException)
        {
            return new PropertyBatchResult(null, (int)failedOperationIndex, failedOperationException, null);
        }

        private NamedProperty InternalGetProperty(int index)
        {
            NativeClient.IFabricPropertyValueResult nativeProperty = this.innerBatch.GetProperty((uint)index);
            return NamedProperty.FromNative(nativeProperty, true /* includesValue */);
        }
    }
}