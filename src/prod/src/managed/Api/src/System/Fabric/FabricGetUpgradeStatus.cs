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
        #region Fields

        private NativeClient.IFabricGetUpgradeStatus nativeProgress;

        #endregion

        #region Constructors & Initialization

        internal FabricGetUpgradeStatus(NativeClient.IFabricGetUpgradeStatus nativeProgress)
        {
            this.nativeProgress = nativeProgress;
        }

        internal FabricGetUpgradeStatus()
        {
        }

        #endregion

        #region Properties

        /// <summary>
        /// <para>Gets the upgrade state for this  upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The upgrade state for this  upgrade.</para>
        /// </value>
        public FabricOrchestrationUpgradeState UpgradeState { get; internal set; }

        #endregion
    }
}