// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>Defines service initialization parameters for a stateless service.</para>
    /// </summary>
    public sealed class StatelessServiceInitializationParameters : ServiceInitializationParameters
    {
        internal StatelessServiceInitializationParameters()
            : this(null)
        {
        }

        internal StatelessServiceInitializationParameters(CodePackageActivationContext activationContext)
            : base(activationContext)
        {
        }

        /// <summary>
        /// <para>Specifies the unique identifier for the stateless service instance.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Int64" />.</para>
        /// </value>
        public long InstanceId
        {
            get;
            internal set;
        }
    }
}