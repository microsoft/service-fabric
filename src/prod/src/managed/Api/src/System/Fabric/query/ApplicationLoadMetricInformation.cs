// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Text;

    /// <summary>
    /// Represents the information about capacity and current load for one metric that services of the application are using.
    /// </summary>
    public sealed class ApplicationLoadMetricInformation
    {
              
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.ApplicationLoadMetricInformation" /> class.</para>
        /// </summary>
        public ApplicationLoadMetricInformation()
        {
        }

        /// <summary>
        /// Gets the name of the metric.
        /// </summary>
        /// <value>
        /// The name of the metric.
        /// </value>
        public string Name { get; internal set; }

        /// <summary>
        /// Gets the reserved capacity for this metric.
        /// </summary>
        /// <value>
        /// The ammount of capacity that is reserved in the cluster for this application.
        /// </value>
        public long ReservationCapacity { get; internal set; }

        /// <summary>
        /// Gets the total capacity for this metric.
        /// </summary>
        /// <value>
        /// The total capacity that is available for this metric that the services of this application can use.
        /// </value>
        public long ApplicationCapacity { get; internal set; }

        /// <summary>
        /// Gets the load for this metric.
        /// </summary>
        /// <value>
        /// The total load that the services of this application are using.
        /// </value>
        public long ApplicationLoad { get; internal set; }

        internal static unsafe ApplicationLoadMetricInformation CreateFromNative(NativeTypes.FABRIC_APPLICATION_LOAD_METRIC_INFORMATION nativeResultItem)
        {
            return new ApplicationLoadMetricInformation
            {
                Name = NativeTypes.FromNativeString(nativeResultItem.Name),
                ReservationCapacity = nativeResultItem.ReservationCapacity,
                ApplicationCapacity = nativeResultItem.ApplicationCapacity,
                ApplicationLoad = nativeResultItem.ApplicationLoad
            };
        }

        internal static unsafe IList<ApplicationLoadMetricInformation> CreateFromNativeList(NativeTypes.FABRIC_APPLICATION_LOAD_METRIC_INFORMATION_LIST* nativeResultList)
        {
            var rv = new List<ApplicationLoadMetricInformation>();

            var nativeArray = (NativeTypes.FABRIC_APPLICATION_LOAD_METRIC_INFORMATION*)nativeResultList->LoadMetrics;
            for (int i = 0; i < nativeResultList->Count; i++)
            {
                var nativeItem = *(nativeArray + i);
                rv.Add(ApplicationLoadMetricInformation.CreateFromNative(nativeItem));
            }

            return rv;
        }

        /// <summary>
        /// <para>
        /// Pretty print out details of <see cref="System.Fabric.Query.ApplicationLoadMetricInformation" />.
        /// </para>
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Query.ApplicationLoadMetricInformation"/>.</returns>
        /// <example>
        /// LoadMetricName        : Metric1
        /// ReservationCapacity   : 10
        /// ApplicationCapacity   : 10
        /// ApplicationLoad       : 2
        /// </example>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendFormat("Name: {0}\n", Name);
            sb.AppendFormat("ReservationCapacity: {0}\n", ReservationCapacity);
            sb.AppendFormat("ApplicationCapacity: {0}\n", ApplicationCapacity);
            sb.AppendFormat("ApplicationLoad: {0}\n", ApplicationLoad);

            return sb.ToString();
        }
    }
}