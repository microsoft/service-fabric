// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes an application instance which is characterized by application-name, application-type, application-parameters, 
    /// health-state etc.</para>
    /// </summary>
    public sealed class Application
    {
        internal Application()
        {
        }

        /// <summary>
        /// <para>Gets the name of the application as a URI.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the application.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public Uri ApplicationName { get; internal set; }

        /// <summary>
        /// <para>Gets the application type name as specified in the Application Manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The application type name.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.TypeName)]
        public string ApplicationTypeName { get; internal set; }

        /// <summary>
        /// <para>Gets the application type version as specified in the Application Manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The application type version.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.TypeVersion)]
        public string ApplicationTypeVersion { get; internal set; }

        /// <summary>
        /// <para>Gets the status of the application as <see cref="System.Fabric.Query.ApplicationStatus" />.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the application.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Status)]
        public ApplicationStatus ApplicationStatus { get; internal set; }

        /// <summary>
        /// <para>Gets the aggregated health state of the application as <see cref="System.Fabric.Health.HealthState" />. </para>
        /// </summary>
        /// <value>
        /// <para>The aggregated health of the application.</para>
        /// </value>
        public HealthState HealthState { get; internal set; }

        /// <summary>
        /// <para>Gets the parameters of the application, which is a collection of key and corresponding values.</para>
        /// </summary>
        /// <value>
        /// <para>The parameters of the application.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Parameters)]
        public ApplicationParameterList ApplicationParameters { get; internal set; }

        /// <summary>
        /// <para>Gets the definition kind.</para>
        /// </summary>
        /// <value>
        /// <para>The definition kind which contains one of the values defined in the
        /// enumeration <see cref="ApplicationDefinitionKind" />.</para>
        /// <para>Specifies the mechanism the user used to define a Service Fabric application.</para>
        /// </value>
        public ApplicationDefinitionKind ApplicationDefinitionKind
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Deprecated: Do not use. See <see cref="System.Fabric.ApplicationUpgradeProgress" /> instead.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        [Obsolete("Use ApplicationUpgradeProgress.")]
        [JsonCustomization(IsIgnored = true)]
        public string UpgradeTypeVersion { get; internal set; }

        /// <summary>
        /// <para>Deprecated: Do not use. See <see cref="System.Fabric.ApplicationUpgradeProgress" /> instead.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Description.ApplicationParameterList" />.</para>
        /// </value>
        [Obsolete("Use ApplicationUpgradeProgress.")]
        [JsonCustomization(IsIgnored = true)]
        public ApplicationParameterList UpgradeParameters { get; internal set; }

        internal static unsafe Application CreateFromNative(
            NativeTypes.FABRIC_APPLICATION_QUERY_RESULT_ITEM nativeResultItem)
        {
            var item = new Application();

            item.ApplicationName = new Uri(NativeTypes.FromNativeString(nativeResultItem.ApplicationName));
            item.ApplicationTypeName = NativeTypes.FromNativeString(nativeResultItem.ApplicationTypeName);
            item.ApplicationTypeVersion = NativeTypes.FromNativeString(nativeResultItem.ApplicationTypeVersion);
            item.ApplicationStatus = (ApplicationStatus)nativeResultItem.Status;
            item.HealthState = (HealthState)nativeResultItem.HealthState;
            item.ApplicationParameters = ApplicationParameterList.FromNative(nativeResultItem.ApplicationParameters);
            
            if (nativeResultItem.Reserved != IntPtr.Zero)
            {
                // Deprecated
                var ex1 = (NativeTypes.FABRIC_APPLICATION_QUERY_RESULT_ITEM_EX1*)nativeResultItem.Reserved;
                if (ex1->Reserved != IntPtr.Zero)
                {
                    var ex2 = (NativeTypes.FABRIC_APPLICATION_QUERY_RESULT_ITEM_EX2*)ex1->Reserved;
                    item.ApplicationDefinitionKind = (ApplicationDefinitionKind)ex2->ApplicationDefinitionKind;
                }
            }
            
            return item;
        }
    }
}