// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    /// <summary>
    /// <para>Modifies the <see cref="System.Fabric.Description.ServiceGroupDescription" /> of an existing service group.</para>
    /// </summary>
    public sealed class ServiceGroupUpdateDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServiceGroupUpdateDescription" /> class.</para>
        /// </summary>
        public ServiceGroupUpdateDescription()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServiceGroupUpdateDescription" /> class from the specified 
        /// <see cref="System.Fabric.Description.ServiceUpdateDescription" />.</para>
        /// </summary>
        /// <param name="updateDescription">
        /// <para>The <see cref="System.Fabric.Description.ServiceUpdateDescription" /> that will be used to create the 
        /// <see cref="System.Fabric.Description.ServiceGroupUpdateDescription" /></para>
        /// </param>
        public ServiceGroupUpdateDescription(ServiceUpdateDescription updateDescription)
        {
            this.ServiceUpdateDescription = updateDescription;
        }

        /// <summary>
        /// <para>Gets or sets the description of a service update.</para>
        /// </summary>
        /// <value>
        /// <para>The description of a service update.</para>
        /// </value>
        public ServiceUpdateDescription ServiceUpdateDescription
        {
            get;
            set;
        }
    }
}