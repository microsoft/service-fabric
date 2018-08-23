// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;
    using System.Collections.Generic;
    using Common.Serialization;

    /// <summary>
    /// <para>Represents the upgrade progress details of nodes in the upgrade domain.</para>
    /// </summary>
    public sealed class UpgradeDomainProgress
    {
        internal UpgradeDomainProgress()
        {
        }

        /// <summary>
        /// <para>Gets or sets the name of the upgrade domain going through upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the upgrade domain going through upgrade.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.DomainName)]
        public string UpgradeDomainName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the list of <see cref="System.Fabric.NodeUpgradeProgress" /> that indicate upgrade progress details of nodes in the current upgrade domain.</para>
        /// </summary>
        /// <value>
        /// <para>The list of <see cref="System.Fabric.NodeUpgradeProgress" /> that indicate upgrade progress details of nodes in the current upgrade domain.</para>
        /// </value>
        public IList<NodeUpgradeProgress> NodeProgressList
        {
            get;
            internal set;
        }

        internal static unsafe UpgradeDomainProgress FromNative(IntPtr nativePtr)
        {
            if (nativePtr == IntPtr.Zero)
            {
                return null;
            }
            else
            {
                return FromNative((NativeTypes.FABRIC_UPGRADE_DOMAIN_PROGRESS*)nativePtr);
            }
        }

        internal static unsafe UpgradeDomainProgress FromNative(NativeTypes.FABRIC_UPGRADE_DOMAIN_PROGRESS* nativePtr)
        {
            IList<NodeUpgradeProgress> nodeProgressList;
            if (nativePtr->NodeProgressList != IntPtr.Zero)
            {
                nodeProgressList = NodeUpgradeProgress.FromNativeList(
                    (NativeTypes.FABRIC_NODE_UPGRADE_PROGRESS_LIST*)nativePtr->NodeProgressList);
            }
            else
            {
                nodeProgressList = new List<NodeUpgradeProgress>();
            }

            return new UpgradeDomainProgress()
            {
                UpgradeDomainName = NativeTypes.FromNativeString(nativePtr->UpgradeDomainName),
                NodeProgressList = nodeProgressList
            };
        }
    }
}