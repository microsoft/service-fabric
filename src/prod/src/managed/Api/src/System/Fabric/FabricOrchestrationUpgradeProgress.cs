// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Encapsulates the progress of a Service Fabric upgrade.</para>
    /// </summary>
    public sealed class FabricOrchestrationUpgradeProgress
    {
        #region Properties

        /// <summary>
        /// <para>Gets the upgrade state for this  upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The upgrade state for this  upgrade.</para>
        /// </value>
        public FabricUpgradeState UpgradeState { get; internal set; }

        /// <summary>
        /// <para>Gets the upgrade state for this  upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The upgrade state for this  upgrade.</para>
        /// </value>
        /// 
#if !DotNetCoreClr
        [CLSCompliant(false)]
#endif
        public UInt32 ProgressStatus { get; internal set; }

        /// <summary>
        /// Gets the JSON config version
        /// </summary>
        public string ConfigVersion { get; internal set; }

        /// <summary>
        /// Upgrade details
        /// </summary>
        public string Details { get; internal set; }

        #endregion

        #region Interop Helpers
        internal unsafe IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeProgress = new NativeTypes.FABRIC_ORCHESTRATION_UPGRADE_PROGRESS();
            nativeProgress.UpgradeState = (NativeTypes.FABRIC_UPGRADE_STATE)this.UpgradeState;
            nativeProgress.ProgressStatus = this.ProgressStatus;

            var ex2 = new NativeTypes.FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX2();
            ex2.Details = pinCollection.AddObject(this.Details);

            var ex1 = new NativeTypes.FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX1();
            ex1.ConfigVersion = pinCollection.AddObject(this.ConfigVersion);
            ex1.Reserved = pinCollection.AddBlittable(ex2);

            nativeProgress.Reserved = pinCollection.AddBlittable(ex1);

            return pinCollection.AddBlittable(nativeProgress);
        }

        internal static unsafe FabricOrchestrationUpgradeProgress FromNative(IntPtr pointer)
        {
            NativeTypes.FABRIC_ORCHESTRATION_UPGRADE_PROGRESS* nativeProgress = (NativeTypes.FABRIC_ORCHESTRATION_UPGRADE_PROGRESS*)pointer;

            FabricOrchestrationUpgradeProgress result = new FabricOrchestrationUpgradeProgress();
            result.UpgradeState = (FabricUpgradeState)nativeProgress->UpgradeState;
            result.ProgressStatus = nativeProgress->ProgressStatus;

            if (nativeProgress->Reserved != IntPtr.Zero)
            {
                NativeTypes.FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX1* ex1Ptr = (NativeTypes.FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX1*)nativeProgress->Reserved;
                result.ConfigVersion = NativeTypes.FromNativeString(ex1Ptr->ConfigVersion);

                if (ex1Ptr->Reserved != IntPtr.Zero)
                {
                    NativeTypes.FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX2* ex2Ptr = (NativeTypes.FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX2*)ex1Ptr->Reserved;
                    result.Details = NativeTypes.FromNativeString(ex2Ptr->Details);
                }
            }

            return result;
        }
#endregion
    }
}