// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Defines a health policy used to evaluate the health of a Service Fabric application
    /// or one of its children entities.</para>
    /// </summary>
    public class ApplicationHealthPolicy
    {
        internal static readonly ApplicationHealthPolicy Default = new ApplicationHealthPolicy();

        private bool considerWarningAsError;
        private byte maxPercentUnhealthyDeployedApplications;
        private ServiceTypeHealthPolicy defaultServiceTypeHealthPolicy;
        private Dictionary<string, ServiceTypeHealthPolicy> serviceTypeHealthPolicyMap;

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> class.</para>
        /// </summary>
        /// <remarks>The default application health policy doesn't tolerate any errors or warnings.</remarks>
        public ApplicationHealthPolicy()
        {
            this.ConsiderWarningAsError = false;
            this.MaxPercentUnhealthyDeployedApplications = 0;
            this.defaultServiceTypeHealthPolicy = null;
            this.serviceTypeHealthPolicyMap = new Dictionary<string, ServiceTypeHealthPolicy>();
        }

        /// <summary>
        /// <para>Gets or sets a <see cref="System.Boolean" /> that determines whether�reports with warning state should be treated with the same severity as errors.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if reports with warning state should be treated as errors; <languageKeyword>false</languageKeyword> when 
        /// warnings should not be treated as errors.</para>
        /// </value>
        public bool ConsiderWarningAsError
        {
            get
            {
                return this.considerWarningAsError;
            }

            set
            {
                this.considerWarningAsError = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the maximum allowed percentage of unhealthy deployed applications.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of unhealthy deployed applications. Allowed values are <see cref="System.Byte" /> values from zero to 100.</para>
        /// </value>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// <para>The specified value was outside the range of integer values from zero to 100.</para>
        /// </exception>
        /// <remarks>
        /// <para>
        /// The percentage represents the maximum tolerated percentage of deployed applications that can be unhealthy 
        /// before the application is considered in error. 
        /// This is calculated by dividing the number of unhealthy deployed applications over the number of nodes
        /// that the applications are currently deployed on in the cluster.
        /// The computation rounds up to tolerate one failure on small numbers of nodes. Default percentage: zero.
        /// </para>
        /// </remarks>
        public byte MaxPercentUnhealthyDeployedApplications
        {
            get
            {
                return this.maxPercentUnhealthyDeployedApplications;
            }

            set
            {
                Requires.CheckPercentageArgument(value, "value");
                this.maxPercentUnhealthyDeployedApplications = value;
            }
        }

        /// <summary>
        /// Gets a string representation of the application health policy.
        /// </summary>
        /// <returns>A string representation of the application health policy.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "ApplicationHealthPolicy: [MaxPercentUnhealthyDeployedApplications={0}, ConsiderWarningAsError={1}",
                    this.MaxPercentUnhealthyDeployedApplications,
                    this.ConsiderWarningAsError));

            if (this.DefaultServiceTypeHealthPolicy != null)
            {
                sb.Append(
                    string.Format(
                        CultureInfo.CurrentCulture, 
                        ", Default{0}",
                        this.DefaultServiceTypeHealthPolicy));
            }

            if (this.ServiceTypeHealthPolicyMap.Count > 0)
            {
                sb.Append(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        ", ServiceTypeHealthPolicyMap: {0} entries",
                        this.ServiceTypeHealthPolicyMap.Count));
            }

            sb.Append("]");

            return sb.ToString();
        }

        /// <summary>
        /// <para>Gets or sets the health policy used by default to evaluate the health of a service type.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.ServiceTypeHealthPolicy" /> used to evaluate service type health if no service type policy is defined.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public ServiceTypeHealthPolicy DefaultServiceTypeHealthPolicy
        {
            get
            {
                return this.defaultServiceTypeHealthPolicy;
            }

            set
            {
                this.defaultServiceTypeHealthPolicy = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the map with <see cref="System.Fabric.Health.ServiceTypeHealthPolicy" /> per service type name. </para>
        /// </summary>
        /// <value>
        /// <para>The map with service type health policy per service type name.</para>
        /// </value>
        /// <remarks>
        /// <para>The entries in the map replace the default service type health policy for each specified service type.
        /// For example, in an application that contains both a stateless gateway service type and a stateful engine service type,
        /// the health policies for the stateless and stateful services can be configured differently.
        /// With policy per service type, there's more granular control of the health of the service.
        /// </para>
        /// <para>If no policy is 
        /// specified for a service type name, the <see cref="System.Fabric.Health.ApplicationHealthPolicy.DefaultServiceTypeHealthPolicy" />
        /// is used for evaluation.
        /// </para>
        /// </remarks>
        [JsonCustomization(IsIgnored = true)]
        public IDictionary<string, ServiceTypeHealthPolicy> ServiceTypeHealthPolicyMap
        {
            get
            {
                return this.serviceTypeHealthPolicyMap;
            }
        }

        internal static bool AreEqual(ApplicationHealthPolicy current, ApplicationHealthPolicy other)
        {
            if ((current != null) && (other != null))
            {
                if ((current.ConsiderWarningAsError != other.ConsiderWarningAsError) ||
                    (current.MaxPercentUnhealthyDeployedApplications != other.MaxPercentUnhealthyDeployedApplications) ||
                    (!ServiceTypeHealthPolicy.AreEqualWithNullAsDefault(current.DefaultServiceTypeHealthPolicy, other.DefaultServiceTypeHealthPolicy)) ||
                    (current.ServiceTypeHealthPolicyMap.Count != other.ServiceTypeHealthPolicyMap.Count))
                {
                    return false;
                }

                foreach (var currentItem in current.ServiceTypeHealthPolicyMap)
                {
                    ServiceTypeHealthPolicy otherValue;
                    var success = other.ServiceTypeHealthPolicyMap.TryGetValue(currentItem.Key, out otherValue);

                    if (!success)
                    {
                        return false;
                    }

                    if (!ServiceTypeHealthPolicy.AreEqual(currentItem.Value, otherValue))
                    {
                        return false;
                    }
                }

                return true;
            }
            else
            {
                return (current == null) && (other == null);
            }
        }

        internal static unsafe ApplicationHealthPolicy FromNative(IntPtr nativeApplicationHealthPolicyPtr)
        {
            var applicationHealthPolicy = new ApplicationHealthPolicy();
            if (nativeApplicationHealthPolicyPtr == IntPtr.Zero)
            {
                return applicationHealthPolicy;
            }

            var nativeApplicationHealthPolicy = (NativeTypes.FABRIC_APPLICATION_HEALTH_POLICY*)nativeApplicationHealthPolicyPtr;
            applicationHealthPolicy.ConsiderWarningAsError = NativeTypes.FromBOOLEAN(nativeApplicationHealthPolicy->ConsiderWarningAsError);

            applicationHealthPolicy.MaxPercentUnhealthyDeployedApplications = nativeApplicationHealthPolicy->MaxPercentUnhealthyDeployedApplications;

            applicationHealthPolicy.DefaultServiceTypeHealthPolicy =
                ServiceTypeHealthPolicy.FromNative(nativeApplicationHealthPolicy->DefaultServiceTypeHealthPolicy);

            ServiceTypeHealthPolicy.FromNativeMap(
                nativeApplicationHealthPolicy->ServiceTypeHealthPolicyMap,
                applicationHealthPolicy.ServiceTypeHealthPolicyMap);

            return applicationHealthPolicy;
        }

        internal unsafe IntPtr ToNative(PinCollection pin)
        {
            var nativeApplicationHealthPolicy = new NativeTypes.FABRIC_APPLICATION_HEALTH_POLICY();

            nativeApplicationHealthPolicy.ConsiderWarningAsError = NativeTypes.ToBOOLEAN(this.ConsiderWarningAsError);

            nativeApplicationHealthPolicy.MaxPercentUnhealthyDeployedApplications = this.MaxPercentUnhealthyDeployedApplications;

            if (this.DefaultServiceTypeHealthPolicy != null)
            {
                nativeApplicationHealthPolicy.DefaultServiceTypeHealthPolicy = this.DefaultServiceTypeHealthPolicy.ToNative(pin);
            }

            nativeApplicationHealthPolicy.ServiceTypeHealthPolicyMap = ServiceTypeHealthPolicy.ToNativeMap(pin, this.ServiceTypeHealthPolicyMap);

            return pin.AddBlittable(nativeApplicationHealthPolicy);
        }
    }
}