// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;
    using System.Collections.Generic;
    using System.Text;

    /// <summary>
    /// <para> Describes the load of an application instance that is retrieved using
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationLoadInformationAsync(string)" />
    /// </para>
    /// </summary>
    public class ApplicationLoadInformation
    {
              
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.ApplicationLoadInformation" /> class.</para>
        /// </summary>
        public ApplicationLoadInformation()
        {
        }

        /// <summary>
        /// Gets or sets the name of the application.
        /// </summary>
        /// <value>
        /// The name of the application.
        /// </value>
        public string Name { get; internal set; }

        /// <summary>
        /// Gets the minimum number of nodes for this application.
        /// </summary>
        /// <value>
        /// <para>
        /// Number of nodes where Service Fabric will reserve Capacity in the cluster for this Application instance.
        /// This value is equal to <see cref="System.Fabric.Description.ApplicationDescription.MinimumNodes"/> that was set when the application was created or updated.
        /// </para>
        /// </value>
        public long MinimumNodes { get; internal set; }

        /// <summary>
        /// Gets the maximum number of nodes where this application can be instantiated.
        /// </summary>
        /// <value>
        /// <para>
        /// Number of nodes this application is allowed to span.
        /// This value is equal to <see cref="System.Fabric.Description.ApplicationDescription.MaximumNodes" /> that was set when application was created or updated.
        /// </para>
        /// </value>
        /// <remarks>
        /// For applications that do not have application capacity defined this value will be zero.
        /// </remarks>
        public long MaximumNodes { get; internal set; }

        /// <summary>
        /// Gets the number of nodes on which this application is instantiated.
        /// </summary>
        /// <value>
        /// <para>
        /// The number of nodes on which this application is currently instantiated.
        /// </para>
        /// </value>
        /// <remarks>
        /// For applications that do not have application capacity defined this value will be zero.
        /// </remarks>
        public long NodeCount { get; internal set; }

        /// <summary>
        /// Gets the list of loads per metric for this application instance.
        /// </summary>
        /// <value>
        /// The list of loads per metric for for this application instance. For each metric that was defined in 
        /// <see cref="System.Fabric.Description.ApplicationDescription.Metrics"/> when application was created or updated there will be one instance of
        /// <see cref="System.Fabric.Query.ApplicationLoadMetricInformation"/> in this list.
        /// </value>
        /// <remarks>
        /// For applications that do not have application capacity defined this list will be empty.
        /// </remarks>
        public IList<ApplicationLoadMetricInformation> ApplicationLoadMetricInformation { get; private set; }

        internal static unsafe ApplicationLoadInformation CreateFromNative(
           NativeClient.IFabricGetApplicationLoadInformationResult nativeApplicationLoadInformation)
        {
            var retval = CreateFromNative(*(NativeTypes.FABRIC_APPLICATION_LOAD_INFORMATION*)nativeApplicationLoadInformation.get_ApplicationLoadInformation());
            GC.KeepAlive(nativeApplicationLoadInformation);

            return retval;
        }

        internal static unsafe ApplicationLoadInformation CreateFromNative(
            NativeTypes.FABRIC_APPLICATION_LOAD_INFORMATION nativeLoadInformation)
        {
            IList<ApplicationLoadMetricInformation> applicationLoadMetricInformationList;

            if (nativeLoadInformation.ApplicationLoadMetricInformation == IntPtr.Zero)
            {
                applicationLoadMetricInformationList = new List<ApplicationLoadMetricInformation>();
            }
            else
            {
                applicationLoadMetricInformationList = System.Fabric.Query.ApplicationLoadMetricInformation.CreateFromNativeList(
                    (NativeTypes.FABRIC_APPLICATION_LOAD_METRIC_INFORMATION_LIST*)nativeLoadInformation.ApplicationLoadMetricInformation);
            }

            return new ApplicationLoadInformation
            {
                Name = NativeTypes.FromNativeString(nativeLoadInformation.Name),
                MinimumNodes = checked((long)nativeLoadInformation.MinimumNodes),
                MaximumNodes = checked((long)nativeLoadInformation.MaximumNodes),
                NodeCount = checked((long)nativeLoadInformation.NodeCount),
                ApplicationLoadMetricInformation = applicationLoadMetricInformationList
            };
        }

        /// <summary>
        /// <para>
        /// Pretty print out details of <see cref="System.Fabric.Query.ApplicationLoadInformation" />.
        /// </para>
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Query.ApplicationLoadInformation"/>.</returns>
        /// <example>
        /// "ApplicationName": "fabric:/LoadBalancingAppType",
        /// "MinimumNodes": 0,
        /// "MaximumNodes": 0,
        /// "NodeCount": 0,
        /// "ApplicationLoadMetricInformation": [],
        /// </example>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendFormat("Application Name: {0}\n", Name);
            sb.AppendFormat("MinimumNodes: {0}\n", MinimumNodes);
            sb.AppendFormat("MaximumNodes: {0}\n", MaximumNodes);
            sb.AppendFormat("NodeCount: {0}\n", NodeCount);
            foreach (ApplicationLoadMetricInformation loadMetricInfo in ApplicationLoadMetricInformation)
            {
                sb.AppendFormat("{0}\n", loadMetricInfo.ToString());
            }

            return sb.ToString();
        }
    }
}