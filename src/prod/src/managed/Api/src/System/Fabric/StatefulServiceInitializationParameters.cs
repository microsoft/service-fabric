// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>Defines service initialization parameters for a stateful service.</para>
    /// </summary>
    public sealed class StatefulServiceInitializationParameters : ServiceInitializationParameters
    {
        internal StatefulServiceInitializationParameters()
            : this(null)
        {
        }

        internal StatefulServiceInitializationParameters(CodePackageActivationContext activationContext) : base(activationContext)
        {
        }

        /// <summary>
        /// <para>Specifies the unique identifier for the stateful service replica.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Int64" />.</para>
        /// </value>
        public long ReplicaId
        {
            get;
            internal set;
        }
    }
}