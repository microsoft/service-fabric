// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Description;

    /// <summary>
    /// Represents activation context for the Service Fabric activated service.
    /// </summary>
    /// <remarks>Includes information from the service manifest as well as information
    /// about the currently activated code package like work directory, context id etc.</remarks>
    public interface ICodePackageActivationContext2 : ICodePackageActivationContext
    {
        /// <summary>
        /// <para>The address at which the service should start the communication listener.</para>
        /// </summary>
        /// <value>
        /// <para>The address at which the service should start the communication listener.</para>
        /// </value>
        string ServiceListenAddress { get; }

        /// <summary>
        /// <para>The address which the service should publish as the listen address.</para>
        /// </summary>
        /// <value>
        /// <para>The address which the service should publish as the listen address.</para>
        /// </value>
        string ServicePublishAddress { get; }
    }
}