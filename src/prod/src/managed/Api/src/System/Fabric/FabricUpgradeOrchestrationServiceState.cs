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
    /// <para>Encapsulates the service state of Upgrade Orchestration Service.</para>
    /// </summary>
    public sealed class FabricUpgradeOrchestrationServiceState
    {
        #region Properties

        /// <summary>
        /// current code version
        /// </summary>
        public string CurrentCodeVersion { get; internal set; }

        /// <summary>
        /// current manifest version
        /// </summary>
        public string CurrentManifestVersion { get; internal set; }

        /// <summary>
        /// target code version
        /// </summary>
        public string TargetCodeVersion { get; internal set; }

        /// <summary>
        /// target manifest version
        /// </summary>
        public string TargetManifestVersion { get; internal set; }

        /// <summary>
        /// pending upgrade type
        /// </summary>
        public string PendingUpgradeType { get; internal set; }

        #endregion

        #region Interop Helpers

        internal static unsafe FabricUpgradeOrchestrationServiceState FromNative(IntPtr pointer)
        {
            if (pointer == IntPtr.Zero)
            {
                return new FabricUpgradeOrchestrationServiceState();
            }

            NativeTypes.FABRIC_UPGRADE_ORCHESTRATION_SERVICE_STATE* nativeState = (NativeTypes.FABRIC_UPGRADE_ORCHESTRATION_SERVICE_STATE*)pointer;
            return new FabricUpgradeOrchestrationServiceState()
            {
                CurrentCodeVersion = NativeTypes.FromNativeString(nativeState->CurrentCodeVersion),
                CurrentManifestVersion = NativeTypes.FromNativeString(nativeState->CurrentManifestVersion),
                TargetCodeVersion = NativeTypes.FromNativeString(nativeState->TargetCodeVersion),
                TargetManifestVersion = NativeTypes.FromNativeString(nativeState->TargetManifestVersion),
                PendingUpgradeType = NativeTypes.FromNativeString(nativeState->PendingUpgradeType)
            };
        }

        #endregion
    }
}
