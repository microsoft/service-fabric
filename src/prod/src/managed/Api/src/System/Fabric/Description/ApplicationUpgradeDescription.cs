// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Collections.Specialized;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// Describes the upgrade policy and the application to be upgraded.
    /// </para>
    /// </summary>
    /// <seealso>
    /// <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-application-upgrade">Service Fabric application upgrade</see>.
    /// </seealso>
    public sealed class ApplicationUpgradeDescription
    {
        private ApplicationParameterList parameters = new ApplicationParameterList();

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ApplicationUpgradeDescription" /> class.</para>
        /// </summary>
        public ApplicationUpgradeDescription()
        {
            this.ApplicationParameters = new NameValueCollection();
        }

        /// <summary>
        /// <para>Gets or sets the URI name of the application instance that needs to be upgraded.</para>
        /// </summary>
        /// <value>
        /// <para>The URI name of the application instance that needs to be upgraded.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public Uri ApplicationName { get; set; }

        /// <summary>
        /// <para>Gets or sets the version of the application type to which the application instance is upgrading.</para>
        /// </summary>
        /// <value>
        /// <para>The version of the application type to which the application instance is upgrading.</para>
        /// </value>
        public string TargetApplicationTypeVersion { get; set; }

        /// <summary>
        /// <para>Gets or sets the description of the policy to be used for upgrading this application instance.</para>
        /// </summary>
        /// <value>
        /// <para>The description of the policy to be used for upgrading this application instance.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public UpgradePolicyDescription UpgradePolicyDescription { get; set; }

        /// <summary>
        /// <para>Gets or sets the collection of name-value pairs for the parameters that are specified in the ApplicationManifest.xml.</para>
        /// </summary>
        /// <value>
        /// <para>The collection of name-value pairs for the parameters that are specified in the ApplicationManifest.xml.</para>
        /// </value>
        /// <remarks>
        /// The maximum allowed length of a parameter value is 1024*1024 characters (including the terminating null character).
        /// </remarks>
        [JsonCustomization(IsIgnored = true)]
        public NameValueCollection ApplicationParameters { get; private set; }

        // Needed for serialization
        // ReCreateMember will cause serializer to call setter of the property
        // on deserialization rather than adding elements to the existing ParameterList.
        // This will allow property "ApplicationParameters" to be populated on deserialization.
        [JsonCustomization(ReCreateMember = true)]
        private ApplicationParameterList Parameters
        {
            get { return new ApplicationParameterList(this.ApplicationParameters); }
            set { this.ApplicationParameters = value.AsNameValueCollection(); }
        }

        internal static void Validate(ApplicationUpgradeDescription description)
        {
            Requires.Argument<Uri>("ApplicationName", description.ApplicationName).NotNullOrWhiteSpace();
            Requires.Argument<string>("TargetApplicationTypeVersion", description.TargetApplicationTypeVersion).NotNullOrWhiteSpace();
            Requires.Argument<UpgradePolicyDescription>("UpgradePolicyDescription", description.UpgradePolicyDescription).NotNull();

            description.UpgradePolicyDescription.Validate();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_APPLICATION_UPGRADE_DESCRIPTION();

            nativeDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDescription.TargetApplicationTypeVersion = pinCollection.AddBlittable(this.TargetApplicationTypeVersion);

            if (this.ApplicationParameters.Count != 0)
            {
                var applicationParameterList = new ApplicationParameterList(this.ApplicationParameters);
                nativeDescription.ApplicationParameters = applicationParameterList.ToNative(pinCollection);
            }
            else
            {
                nativeDescription.ApplicationParameters = IntPtr.Zero;
            }

            var rollingUpgradePolicyDescription = this.UpgradePolicyDescription as RollingUpgradePolicyDescription;
            if (rollingUpgradePolicyDescription != null)
            {
                nativeDescription.UpgradeKind = NativeTypes.FABRIC_APPLICATION_UPGRADE_KIND.FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
                nativeDescription.UpgradePolicyDescription = rollingUpgradePolicyDescription.ToNative(pinCollection);
            }
            else
            {
                throw new ArgumentException("description.UpgradePolicyDescription");
            }

            return pinCollection.AddBlittable(nativeDescription);
        }

        internal static unsafe ApplicationUpgradeDescription FromNative(IntPtr descriptionPtr)
        {
            if (descriptionPtr == IntPtr.Zero) { return null; }

            var castedPtr = (NativeTypes.FABRIC_APPLICATION_UPGRADE_DESCRIPTION*)descriptionPtr;

            var description = new ApplicationUpgradeDescription();

            description.ApplicationName = NativeTypes.FromNativeUri(castedPtr->ApplicationName);
            description.TargetApplicationTypeVersion = NativeTypes.FromNativeString(castedPtr->TargetApplicationTypeVersion);

            if (castedPtr->ApplicationParameters != IntPtr.Zero)
            {
                var castedParamsPtr = (NativeTypes.FABRIC_APPLICATION_PARAMETER_LIST*)castedPtr->ApplicationParameters;
                var parametersList = ApplicationParameterList.FromNative(castedParamsPtr);
                description.ApplicationParameters = parametersList.AsNameValueCollection();
            }

            if (castedPtr->UpgradeKind == NativeTypes.FABRIC_APPLICATION_UPGRADE_KIND.FABRIC_APPLICATION_UPGRADE_KIND_ROLLING &&
                castedPtr->UpgradePolicyDescription != IntPtr.Zero)
            {
                RollingUpgradePolicyDescription policy = null;

                var castedPolicyPtr = (NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION*)castedPtr->UpgradePolicyDescription;

                if (castedPolicyPtr->RollingUpgradeMode == NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_MONITORED)
                {
                    policy = MonitoredRollingApplicationUpgradePolicyDescription.FromNative(castedPtr->UpgradePolicyDescription);
                }
                else
                {
                    policy = RollingUpgradePolicyDescription.FromNative(castedPtr->UpgradePolicyDescription);
                }

                description.UpgradePolicyDescription = policy;
            }

            return description;
        }
    }
}