// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Represents the base class for service initialization parameters that are passed to the <see cref="System.Fabric.IStatefulServiceReplica.Initialize(System.Fabric.StatefulServiceInitializationParameters)" /> method of 
    /// a service.</para>
    /// </summary>
    /// <remarks>
    /// <para>Derived types define initialization data that are specific to stateless and stateful services.</para>
    /// </remarks>
    public abstract class ServiceInitializationParameters
    {
        private readonly CodePackageActivationContext codePackageActivationContext;

        internal ServiceInitializationParameters() : this(null)
        {
        }

        internal ServiceInitializationParameters(CodePackageActivationContext activationContext)
        {
            this.codePackageActivationContext = activationContext;
        }

        /// <summary>
        /// <para>Specifies the activation context that is associated with the code package that contains the service.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.CodePackageActivationContext" />.</para>
        /// </value>
        public CodePackageActivationContext CodePackageActivationContext
        {
            get
            {
                if (this.codePackageActivationContext == null)
                {
                    throw new InvalidOperationException(StringResources.Error_ServiceInitializationParameters_ActivationContext_Not_Available);
                }

                return this.codePackageActivationContext;
            }
        }

        /// <summary>
        /// <para>Indicates the name of the type of the service.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string ServiceTypeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Indicates the Service Fabric name of the service.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Uri" />.</para>
        /// </value>
        public Uri ServiceName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Specifies custom initialization data that is provided by the creator of the service as part of the <see cref="System.Fabric.Description.ServiceDescription" /> class.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Byte" />.</para>
        /// </value>
        public byte[] InitializationData
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Specifies the unique identifier of the service partition.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Guid" />.</para>
        /// </value>
        public Guid PartitionId
        {
            get;
            internal set;
        }
    }
}