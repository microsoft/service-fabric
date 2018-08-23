// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Result;
    using System.Runtime.Serialization;
    using System.Text;

    /// <summary>
    /// Returns Invoke quorum loss progress object.
    /// </summary>
    /// <remarks>
    /// This class returns the TestCommandProgressState, Result (when in Completed or Faulted state), 
    /// and Exception (when in Faulted state) information for the invoke quorum loss action.
    /// </remarks>
    [DataContract]
    public sealed class PartitionQuorumLossProgress : TestCommandProgress
    {
        #region Constructors & Initialization
        internal PartitionQuorumLossProgress()
        { }

        internal PartitionQuorumLossProgress(TestCommandProgressState state, PartitionQuorumLossResult result)
        {
            this.State = state;
            this.Result = result;
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets the result of the invoke quorum loss action;
        /// this is avaliable only when the action is in Completed or Faulted state.
        /// </summary>
        /// <value>The PartitionQuorumLossResult object.</value>
        [JsonCustomization(PropertyName = "InvokeQuorumLossResult")]      
        public PartitionQuorumLossResult Result
        {
            get;
            internal set;
        }

        #endregion

        /// <summary>
        /// Returns a string representation of the contained information
        /// </summary>
        /// <returns>A string that has State, InvokeQuorumLossResult, and Exception information.
        /// State is always presnt; but depending on State, the Result and the Exception may not be present.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendFormat(base.ToString());

            if (this.Result != null)
            {
                sb.AppendFormat(", Result: {0}", this.Result);
            }

            return sb.ToString();
        }

        #region Interop Helpers
        internal unsafe IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeProgress = new NativeTypes.FABRIC_PARTITION_QUORUM_LOSS_PROGRESS();
            nativeProgress.State = TestCommandStateHelper.ToNative(this.State);
            nativeProgress.Result = this.Result.ToNative(pinCollection);
            return pinCollection.AddBlittable(nativeProgress);
        }

        internal static unsafe PartitionQuorumLossProgress FromNative(IntPtr pointer)
        {
            NativeTypes.FABRIC_PARTITION_QUORUM_LOSS_PROGRESS nativeProgress = *(NativeTypes.FABRIC_PARTITION_QUORUM_LOSS_PROGRESS*)pointer;
            var state = TestCommandStateHelper.FromNative(nativeProgress.State);

            PartitionQuorumLossResult result = null;
            if (nativeProgress.Result != IntPtr.Zero)
            {
                result = new PartitionQuorumLossResult();
                result.CreateFromNative(nativeProgress.Result);
            }

            return new PartitionQuorumLossProgress(state, result);
        }
        #endregion
    }
}