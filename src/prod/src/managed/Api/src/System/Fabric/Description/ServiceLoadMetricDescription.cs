// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Globalization;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;

    /// <summary>
    /// <para>Specifies a metric to load balance a service during runtime.</para>
    /// </summary>
    /// <remarks>
    /// <para>Note that to provide metrics for services is optional, because Service Fabric uses default metrics. Provide metrics only if the service 
    /// requires advanced load balancing features, such as balancing that is based on specific node characteristics and resources.</para>
    /// </remarks>

    public class ServiceLoadMetricDescription
    {
        private string name;

        private int primaryDefaultLoad;
        private int secondaryDefaultLoad;

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServiceLoadMetricDescription" /> class.</para>
        /// </summary>
        public ServiceLoadMetricDescription()
        {
        }

        internal ServiceLoadMetricDescription(string name, ServiceLoadMetricWeight loadMetricWeight = 0)
        {
            this.name = name;
            this.primaryDefaultLoad = 0;
            this.secondaryDefaultLoad = 0;
            this.Weight = loadMetricWeight;
        }

        internal ServiceLoadMetricDescription(string name, int primaryDefaultLoad, int secondaryDefaultLoad = 0, ServiceLoadMetricWeight loadMetricWeight = 0)
        {
            this.name = name;
            this.primaryDefaultLoad = primaryDefaultLoad;
            this.secondaryDefaultLoad = secondaryDefaultLoad;
            this.Weight = loadMetricWeight;
        }

        // Copy ctor
        internal ServiceLoadMetricDescription(ServiceLoadMetricDescription other)
            : this(other.Name, other.primaryDefaultLoad, other.secondaryDefaultLoad, other.Weight)
        {
        }

        /// <summary>
        /// <para>Gets or sets the name of the metric. </para>
        /// </summary>
        /// <value>
        /// <para>The name of the metric.</para>
        /// </value>
        /// <remarks>
        /// <para>If the service chooses to <see cref="System.Fabric.IServicePartition.ReportLoad(System.Collections.Generic.IEnumerable{System.Fabric.LoadMetric})" /> 
        /// during runtime, the name that is provided via the <see cref="System.Fabric.LoadMetric" /> at that time should match the name that is specified in 
        /// <see cref="System.Fabric.Description.ServiceLoadMetricDescription.Name" /> exactly.</para>
        /// <para>Note that metric names are case sensitive.</para>
        /// </remarks>
        public string Name
        {
            get
            {
                return this.name;
            }

            set
            {
                this.name = value;
            }
        }

        /// <summary>
        /// <para>Determines the metric weight relative to the other metrics that are configured for this service. During runtime, if two metrics end up in 
        /// conflict, the Cluster Resource Manager prefers the metric with the higher weight.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Description.ServiceLoadMetricWeight" />.</para>
        /// </value>
        public ServiceLoadMetricWeight Weight
        {
            get;
            set;
        }

        /// <summary>  
        /// <para>Please refer to the derived class <see cref="System.Fabric.Description.StatefulServiceLoadMetricDescription"/> or 
        /// <see cref="System.Fabric.Description.StatelessServiceLoadMetricDescription"/> for usage.</para>
        /// </summary>  
        /// <remarks>
        /// <para>This property is deprecated, please use the corresponding property in the derived class.</para>
        /// </remarks>
        [Obsolete("PrimaryDefaultLoad in ServiceLoadMetricDescription is deprecated, please use StatefulServiceLoadMetricDescription instead.", false)]
        public int PrimaryDefaultLoad
        {
            get
            {
                return this.primaryDefaultLoad;
            }
            set
            {
                this.primaryDefaultLoad = value;
            }
        }

        /// <summary>  
        /// <para>Please refer to the derived class <see cref="System.Fabric.Description.StatefulServiceLoadMetricDescription"/> or 
        /// <see cref="System.Fabric.Description.StatelessServiceLoadMetricDescription"/> for usage.</para>
        /// </summary>  
        /// <remarks>
        /// <para>This property is deprecated, please use the corresponding property in the derived class.</para>
        /// </remarks>
        [Obsolete("SecondaryDefaultLoad in ServiceLoadMetricDescription is deprecated, please use StatefulServiceLoadMetricDescription instead.", false)]
        public int SecondaryDefaultLoad
        {
            get
            {
                return this.secondaryDefaultLoad;
            }
            set
            {
                this.secondaryDefaultLoad = value;
            }
        }

        internal static void Validate(ServiceLoadMetricDescription serviceLoadMetricDescription)
        {
            Requires.Argument<ServiceLoadMetricDescription>("serviceLoadMetricDescription", serviceLoadMetricDescription).NotNull();
            Requires.Argument<string>("serviceLoadMetricDescription.Name", serviceLoadMetricDescription.Name).NotNull();
        }

        internal static void CopyFrom(KeyedCollection<string, ServiceLoadMetricDescription> source, KeyedCollection<string, ServiceLoadMetricDescription> target)
        {
            if (source != null)
            {
                target.Clear();

                source.ForEach(m =>
                {
                    ServiceLoadMetricDescription casted;

                    if (m is StatefulServiceLoadMetricDescription)
                    {
                        casted = new StatefulServiceLoadMetricDescription(m as StatefulServiceLoadMetricDescription);
                    }
                    else if (m is StatelessServiceLoadMetricDescription)
                    {
                        casted = new StatelessServiceLoadMetricDescription(m as StatelessServiceLoadMetricDescription);
                    }
                    else
                    {
                        casted = new ServiceLoadMetricDescription(m);
                    }

                    target.Add(casted);
                });
            }
        }

#pragma warning disable 618

        internal unsafe void ToNative(PinCollection pin, ref NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION native)
        {
            native.Name = pin.AddBlittable(this.Name);
            native.PrimaryDefaultLoad = checked((uint)this.PrimaryDefaultLoad);
            native.SecondaryDefaultLoad = checked((uint)this.SecondaryDefaultLoad);

            switch (this.Weight)
            {
                case ServiceLoadMetricWeight.Zero:
                    native.Weight = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO;
                    break;
                case ServiceLoadMetricWeight.Low:
                    native.Weight = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW;
                    break;
                case ServiceLoadMetricWeight.Medium:
                    native.Weight = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM;
                    break;
                case ServiceLoadMetricWeight.High:
                    native.Weight = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH;
                    break;
                default:
                    ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_UnknownWeight_Formatted, this.Weight));
                    break;
            }
        }

        internal static unsafe ServiceLoadMetricDescription CreateFromNative(IntPtr nativeRaw, bool isStateful)
        {
            NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION nativeMetric = *(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION*)nativeRaw;

            ServiceLoadMetricDescription managed;

            if (isStateful)
            {
                managed = new StatefulServiceLoadMetricDescription();
            }
            else
            {
                managed = new StatelessServiceLoadMetricDescription();
            }

            managed.Name = NativeTypes.FromNativeString(nativeMetric.Name);

            switch (nativeMetric.Weight)
            {
                case NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO:
                    managed.Weight = ServiceLoadMetricWeight.Zero;
                    break;
                case NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW:
                    managed.Weight = ServiceLoadMetricWeight.Low;
                    break;
                case NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM:
                    managed.Weight = ServiceLoadMetricWeight.Medium;
                    break;
                case NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH:
                    managed.Weight = ServiceLoadMetricWeight.High;
                    break;
                default:
                    ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_UnknownWeight_Formatted, nativeMetric.Weight));
                    throw new ArgumentException(StringResources.Error_UnknownWeight);
            }

            managed.PrimaryDefaultLoad = checked((int)nativeMetric.PrimaryDefaultLoad);
            managed.SecondaryDefaultLoad = checked((int)nativeMetric.SecondaryDefaultLoad);

            return managed;
        }

        /// <summary>
        /// Pretty print out details of <see cref="System.Fabric.Description.StatefulServiceLoadMetricDescription"/>
        /// or <see cref="System.Fabric.Description.StatelessServiceLoadMetricDescription"/>.
        /// </summary>
        /// <returns>A string representation of <see cref="System.Fabric.Description.StatefulServiceLoadMetricDescription"/>
        /// or <see cref="System.Fabric.Description.StatelessServiceLoadMetricDescription"/>.</returns>
        public string ToString(bool isStateful)
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendFormat("{0},", this.Name);
            sb.AppendFormat("{0},", this.Weight);
            sb.AppendFormat("{0}", this.PrimaryDefaultLoad);

            if (isStateful)
            {
                sb.AppendFormat(",{0}", this.SecondaryDefaultLoad);
            }

            return sb.ToString();
        }

#pragma warning restore 618        
    }
}