// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using System.Linq;

    /// <summary>
    /// <para>Creates a service group factory that is used to create actual service groups from the provided type factories at runtime.</para>
    /// </summary>
    public sealed class ServiceGroupFactory
    {
        private readonly Dictionary<string, IStatefulServiceFactory> statefulFactories = new Dictionary<string, IStatefulServiceFactory>();
        private readonly Dictionary<string, IStatelessServiceFactory> statelessFactories = new Dictionary<string, IStatelessServiceFactory>();

        /// <summary>
        /// <para>Creates an empty <see cref="System.Fabric.ServiceGroupFactory" /> object.</para>
        /// </summary>
        public ServiceGroupFactory()
        {
        }

        internal IEnumerable<Tuple<string, IStatefulServiceFactory>> StatefulServiceFactories
        {
            get
            {
                return this.statefulFactories.Select(entry => Tuple.Create(entry.Key, entry.Value));
            }
        }

        internal IEnumerable<Tuple<string, IStatelessServiceFactory>> StatelessServiceFactories
        {
            get
            {
                return this.statelessFactories.Select(entry => Tuple.Create(entry.Key, entry.Value));
            }
        }

        /// <summary>
        /// <para>Registers a particular stateful or stateless service type with the service group factory so that it can be created as 
        /// a member of the service group.</para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The service type name as a string. It should match the service type that is specified in the manifest or 
        /// the <see cref="System.Fabric.Description.ServiceGroupMemberDescription" /> of the <see cref="System.Fabric.Description.ServiceGroupDescription" /> 
        /// that is provided during the <see cref="System.Fabric.FabricClient.ServiceGroupManagementClient.CreateServiceGroupAsync(System.Fabric.Description.ServiceGroupDescription)" /> call.</para>
        /// </param>
        /// <param name="serviceTypeImplementation">
        /// <para>The fully qualified C# type of the service that implements the Service Fabric service.</para>
        /// </param>
        public void AddServiceType(string serviceTypeName, Type serviceTypeImplementation)
        {
            if (serviceTypeName == null)
            {
                throw new ArgumentNullException("serviceTypeName");
            }
            if (string.IsNullOrWhiteSpace(serviceTypeName))
            {
                throw new ArgumentOutOfRangeException(
                    "serviceTypeName",
                    StringResources.Error_InvalidServiceType);
            }
            if (serviceTypeImplementation == null)
            {
                throw new ArgumentNullException("serviceTypeImplementation");
            }

            bool? isStateful = Helpers.IsStatefulService(serviceTypeImplementation);

            if (!isStateful.HasValue)
            {
                AppTrace.TraceSource.WriteError("ServiceGroupFactory.AddServiceType", "Service Type Implementation {0} is invalid", serviceTypeImplementation.FullName);
                throw new ArgumentException(StringResources.Error_InvalidServiceTypeImplementation);
            }

            DefaultServiceFactory factory = new DefaultServiceFactory(serviceTypeImplementation);

            if (isStateful.Value)
            {
                this.AddStatefulServiceFactory(serviceTypeName, factory);
            }
            else
            {
                this.AddStatelessServiceFactory(serviceTypeName, factory);
            }
        }

        /// <summary>
        /// <para>Adds the specified stateless service factory to the service group factory.</para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The service type name as a string. It should match the service type that is specified in the manifest 
        /// or the <see cref="System.Fabric.Description.ServiceGroupMemberDescription" /> of the <see cref="System.Fabric.Description.ServiceGroupDescription" /> 
        /// that is provided during the <see cref="System.Fabric.FabricClient.ServiceGroupManagementClient.CreateServiceGroupAsync(System.Fabric.Description.ServiceGroupDescription)" /> call.</para>
        /// </param>
        /// <param name="factory">
        /// <para>The stateless service factory to add.</para>
        /// </param>
        public void AddStatelessServiceFactory(string serviceTypeName, IStatelessServiceFactory factory)
        {
            Requires.Argument("serviceTypeName", serviceTypeName).NotNullOrWhiteSpace();
            Requires.Argument("factory", factory).NotNull();

            this.VerifyServiceTypeNameIsUnique(serviceTypeName);
            this.VerifyIsEmpty(this.statefulFactories.Keys);

            this.statelessFactories.Add(serviceTypeName, factory);
        }

        /// <summary>
        /// <para>Adds the specified stateful service factory to the service group factory.</para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>Indicates the service type name as a string. It should match the service type that is specified in the manifest or 
        /// the <see cref="System.Fabric.Description.ServiceGroupMemberDescription" /> of the <see cref="System.Fabric.Description.ServiceGroupDescription" /> that 
        /// is provided during the <see cref="System.Fabric.FabricClient.ServiceGroupManagementClient.CreateServiceGroupAsync(System.Fabric.Description.ServiceGroupDescription)" /> call.</para>
        /// </param>
        /// <param name="factory">
        /// <para>The stateful service factory to add.</para>
        /// </param>
        public void AddStatefulServiceFactory(string serviceTypeName, IStatefulServiceFactory factory)
        {
            Requires.Argument("serviceTypeName", serviceTypeName).NotNullOrWhiteSpace();
            Requires.Argument("factory", factory).NotNull();

            this.VerifyServiceTypeNameIsUnique(serviceTypeName);
            this.VerifyIsEmpty(this.statelessFactories.Keys);

            this.statefulFactories.Add(serviceTypeName, factory);
        }

        /// <summary>
        /// <para>Removes the service factory.</para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The service type name as a string. It should match the service type that was specified when the factory was registered. </para>
        /// </param>
        public void RemoveServiceFactory(string serviceTypeName)
        {
            Requires.Argument("serviceTypeName", serviceTypeName).NotNullOrWhiteSpace();

            if (this.statefulFactories.ContainsKey(serviceTypeName))
            {
                this.statefulFactories.Remove(serviceTypeName);
            }

            if (this.statelessFactories.ContainsKey(serviceTypeName))
            {
                this.statelessFactories.Remove(serviceTypeName);
            }
        }

        private void VerifyServiceTypeNameIsUnique(string serviceTypeName)
        {
            if (this.statelessFactories.ContainsKey(serviceTypeName) || this.statefulFactories.ContainsKey(serviceTypeName))
            {
                throw new ArgumentException(StringResources.Error_ServiceGroupType_Existing_ServiceFactory);
            }
        }

        private void VerifyIsEmpty(IEnumerable<string> serviceTypeNames)
        {
            if (serviceTypeNames.Count() > 0)
            {
                throw new InvalidOperationException(StringResources.Error_ServiceGroupType_Not_Supported);
            }
        }
    }
}