// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Common.Serialization;

    /// <summary>
    /// <para>Enumerates the possible kinds of upgrade.</para>
    /// </summary>
    public enum UpgradeKind
    {
        /// <summary>
        /// <para>Indicates that this is an invalid upgrade. All Service Fabric enumerations have a reserved "Invalid" member.</para>
        /// </summary>
        Invalid = 0,
        /// <summary>
        /// <para>Indicates that this is a rolling upgrade.</para>
        /// </summary>
        Rolling = 1,
        /// <summary>
        /// <para>Indicates that this is a rolling upgrade and the service host restarts.</para>
        /// </summary>
        Rolling_ForceRestart = 2,
        /// <summary>
        /// <para>Indicates that this is a rolling upgrade and replicas are not restarted.</para>
        /// </summary>
        Rolling_NotificationOnly = 3
    }

    /// <summary>
    /// <para>Describes the upgrade policy of the service.</para>
    /// </summary>
    public abstract class UpgradePolicyDescription
    {
        /// <summary>
        /// <para>Initializes an instance of the <see cref="System.Fabric.Description.UpgradePolicyDescription" /> class with the upgrade kind.</para>
        /// </summary>
        /// <param name="kind">
        /// <para>
        ///     <see cref="System.Fabric.Description.UpgradeKind" />: Describes the kind of upgrade.</para>
        /// </param>
        protected UpgradePolicyDescription(UpgradeKind kind)
        {
            this.Kind = kind;
        }

        /// <summary>
        /// <para>Gets the kind of application upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Description.UpgradeKind" /> of the application upgrade.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.UpgradeKind, AppearanceOrder = -2)]
        public UpgradeKind Kind { get; private set; }

        internal abstract void Validate();
    }
}