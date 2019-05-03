// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes an application to be created by using 
    /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.CreateApplicationAsync(System.Fabric.Description.ApplicationDescription)" />.</para>
    /// </summary>
    public sealed class ApplicationDescription
    {
        /// <summary>
        /// <para>Instantiates an instance of <see cref="System.Fabric.Description.ApplicationDescription" /> class.</para>
        /// </summary>
        public ApplicationDescription() : this(null /* applicationName */, null /* applicationTypeName */, null /* applicationTypeVersion */)
        {
        }

        /// <summary>
        /// <para>Instantiates an instance of <see cref="System.Fabric.Description.ApplicationDescription" /> with the application instance name, the application 
        /// type name, and the application type version.</para>
        /// </summary>
        /// <param name="applicationName">
        /// <para>URI of the application instance name.</para>
        /// </param>
        /// <param name="applicationTypeName">
        /// <para>Name of the application type.</para>
        /// </param>
        /// <param name="applicationTypeVersion">
        /// <para>Version of the application type.</para>
        /// </param>
        public ApplicationDescription(Uri applicationName, string applicationTypeName, string applicationTypeVersion)
            : this(applicationName, applicationTypeName, applicationTypeVersion, new NameValueCollection())
        {
        }

        /// <summary>
        /// <para>Instantiates an instance of <see cref="System.Fabric.Description.ApplicationDescription" /> with the application instance name, the application 
        /// type name, the application type version, and the collection of application parameters.</para>
        /// </summary>
        /// <param name="applicationName">
        /// <para>URI of the application instance name.</para>
        /// </param>
        /// <param name="applicationTypeName">
        /// <para>Name of the application type.</para>
        /// </param>
        /// <param name="applicationTypeVersion">
        /// <para>Version of the application type.</para>
        /// </param>
        /// <param name="applicationParameters">
        /// <para>Collection of name-value pairs for the parameters that are specified in the ApplicationManifest.xml.</para>
        /// </param>
        public ApplicationDescription(Uri applicationName, string applicationTypeName, string applicationTypeVersion, NameValueCollection applicationParameters)
        {
            this.ApplicationName = applicationName;
            this.ApplicationTypeName = applicationTypeName;
            this.ApplicationTypeVersion = applicationTypeVersion;
            this.ApplicationParameters = applicationParameters ?? new NameValueCollection();
            // Scaleout parameters are initialized so that no scaleout is present
            this.MaximumNodes = 0;
            this.MinimumNodes = 0;
            this.Metrics = new List<ApplicationMetricDescription>();
        }

        /// <summary>
        /// <para>Gets or sets the URI name of the application instance.</para>
        /// </summary>
        /// <value>
        /// <para>The application name.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public Uri ApplicationName { get; set; }

        /// <summary>
        /// <para>Gets or sets the name of the Service Fabric application type.</para>
        /// </summary>
        /// <value>
        /// <para>The application type name.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.TypeName)]
        public string ApplicationTypeName { get; set; }

        /// <summary>
        /// <para>Gets or sets the version of the application type.</para>
        /// </summary>
        /// <value>
        /// <para>The application type version.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.TypeVersion)]
        public string ApplicationTypeVersion { get; set; }

        /// <summary>
        /// <para>Gets the collection of name-value pairs for the parameters that are specified in the ApplicationManifest.xml.</para>
        /// </summary>
        /// <value>
        /// <para>The collection of name-value pairs for the parameters that are specified in the ApplicationManifest.xml.</para>
        /// </value>
        /// <remarks>
        /// The maximum allowed length of a parameter value is 1024*1024 characters (including the terminating null character).
        /// </remarks>
        [JsonCustomization(IsIgnored = true)]
        public NameValueCollection ApplicationParameters { get; private set; }

        /// Wrapper of property ApplicationParameters. Needed for serialization.
        [JsonCustomization(ReCreateMember = true)]
        private ApplicationParameterList ParameterList
        {
            get { return new ApplicationParameterList(this.ApplicationParameters); }
            set { this.ApplicationParameters = value.AsNameValueCollection(); }
        }

        /// <summary>
        /// Gets or sets the maximum number of nodes where this application can be instantiated.
        /// </summary>
        /// <value>
        /// <para>
        /// Number of nodes this application is allowed to span. Default value is zero.
        /// If it is zero, Application can span any number of nodes in the cluster.
        /// </para>
        /// <para>
        /// If this parameter is smaller than <see cref="System.Fabric.Description.ApplicationDescription.MinimumNodes"/> an
        /// <see cref="System.ArgumentException"/> will be thrown when <see cref="System.Fabric.FabricClient.ApplicationManagementClient.CreateApplicationAsync(System.Fabric.Description.ApplicationDescription)" />
        /// is called.
        /// </para>
        /// </value>
        public long MaximumNodes { get; set; }

        /// <summary>
        /// Gets or sets the minimum number of nodes where Service Fabric will reserve capacity for this application.
        /// </summary>
        /// <value>
        /// <para>
        /// Number of nodes where Service Fabric will reserve Capacity in the cluster for this Application to be placed.
        /// Note that this does not mean that the Application is guaranteed to have replicas on all those nodes.
        /// </para>
        /// <para>
        /// If this parameter is set to zero, no capacity will be reserved.
        /// </para>
        /// <para>
        /// If this parameters is greater than <see cref="System.Fabric.Description.ApplicationDescription.MaximumNodes"/> an
        /// <see cref="System.ArgumentException"/> will be thrown when <see cref="System.Fabric.FabricClient.ApplicationManagementClient.CreateApplicationAsync(System.Fabric.Description.ApplicationDescription)" />
        /// is called.
        /// </para>
        /// </value>
        public long MinimumNodes { get; set; }

        /// <summary>
        /// Gets or sets the list of metrics for which the application capacity is defined.
        /// </summary>
        /// <value>
        /// Specifies the metric Capacity of the Application. Capacity is specified for each metric by using
        /// <see cref="System.Fabric.Description.ApplicationMetricDescription"/>.
        /// </value>
        public IList<ApplicationMetricDescription> Metrics;

        internal static void Validate(ApplicationDescription description)
        {
            Requires.Argument<Uri>("ApplicationName", description.ApplicationName).NotNull();
            Requires.Argument<string>("ApplicationTypeName", description.ApplicationTypeName).NotNullOrWhiteSpace();
            Requires.Argument<string>("ApplicationTypeVersion", description.ApplicationTypeVersion).NotNullOrWhiteSpace();

            // This check is needed because managed type is long, and native type is UINT
            Requires.CheckUInt32ArgumentLimits(description.MaximumNodes, "MaximumNodes");
            Requires.CheckUInt32ArgumentLimits(description.MinimumNodes, "MinimumNodes");

            if (description.MinimumNodes > description.MaximumNodes)
            {
                throw new ArgumentException(
                    String.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_MinimumNodesGreaterThanMaximumNodes,
                        description.MinimumNodes,
                        description.MaximumNodes
                    ));
            }
            foreach (var metric in description.Metrics)
            {
                ApplicationMetricDescription.Validate(metric, description.MaximumNodes);
            }
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_APPLICATION_DESCRIPTION();

            nativeDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDescription.ApplicationTypeName = pinCollection.AddObject(this.ApplicationTypeName);
            nativeDescription.ApplicationTypeVersion = pinCollection.AddObject(this.ApplicationTypeVersion);

            if (this.ApplicationParameters.Count != 0)
            {
                var applicationParameterList = new ApplicationParameterList(this.ApplicationParameters);
                nativeDescription.ApplicationParameters = applicationParameterList.ToNative(pinCollection);
            }
            else
            {
                nativeDescription.ApplicationParameters = IntPtr.Zero;
            }

            if (this.MaximumNodes > 0 || this.MinimumNodes > 0 || this.Metrics != null)
            {
                var nativeDescriptionEx1 = new NativeTypes.FABRIC_APPLICATION_DESCRIPTION_EX1();
                var nativeApplicationCapacityDescription = new NativeTypes.FABRIC_APPLICATION_CAPACITY_DESCRIPTION();
                nativeApplicationCapacityDescription.MaximumNodes = checked((uint)this.MaximumNodes);
                nativeApplicationCapacityDescription.MinimumNodes = checked((uint)this.MinimumNodes);

                var nativeMetricsList = new NativeTypes.FABRIC_APPLICATION_METRIC_LIST();
                if (null != this.Metrics)
                {
                    nativeMetricsList.Count = (uint)this.Metrics.Count;
                    if (this.Metrics.Count > 0)
                    {
                        var nativeMetricsArray = new NativeTypes.FABRIC_APPLICATION_METRIC_DESCRIPTION[this.Metrics.Count];
                        for (int i = 0; i < this.Metrics.Count; i++)
                        {
                             Metrics[i].ToNative(pinCollection, ref nativeMetricsArray[i]);
                        }

                        nativeMetricsList.Metrics = pinCollection.AddBlittable(nativeMetricsArray);
                    }
                    else
                    {
                        nativeMetricsList.Metrics = IntPtr.Zero;
                    }
                }
                else
                {
                    nativeMetricsList.Count = 0;
                    nativeMetricsList.Metrics = IntPtr.Zero;
                }
                nativeApplicationCapacityDescription.Metrics = pinCollection.AddBlittable(nativeMetricsList);

                nativeDescriptionEx1.ApplicationCapacity = pinCollection.AddBlittable(nativeApplicationCapacityDescription);

                nativeDescription.Reserved = pinCollection.AddBlittable(nativeDescriptionEx1);
            }         

            return pinCollection.AddBlittable(nativeDescription);
        }
    }    
}