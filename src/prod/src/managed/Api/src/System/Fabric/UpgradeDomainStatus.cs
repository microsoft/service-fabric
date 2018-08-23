// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>Represents the status of an upgrade domain.</para>
    /// </summary>
    public class UpgradeDomainStatus
    {
        // To prevent auto generated default public constructor from being public
        internal UpgradeDomainStatus()
        {
        }

        /// <summary>
        /// <para>Specifies the name of the upgrade domain. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string Name { get; internal set; }

        /// <summary>
        /// <para>Specifies the state of the upgrade domain.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.UpgradeDomainState" />.</para>
        /// </value>
        public UpgradeDomainState State { get; internal set; }

        /// <summary>
        /// <para>
        /// Produces a string representation of the upgrade domain status.
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>String representing the upgrade domain status.</para>
        /// </returns>
        public override string ToString()
        {
            return string.Format("UpgradeDomainName={0}; State={1}; ", this.Name, this.State);
        }
    }
}