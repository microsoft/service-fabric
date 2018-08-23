// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents an application type.</para>
    /// </summary>
    /// <remarks>
    ///     <para>
    ///         An application type is a categorization of an application and consists of a bundle of service types.
    ///         Details are described in this <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-application-model">document</see>.
    ///     </para>
    ///     <para>
    ///         An application type contains all that is needed for
    ///         a functioning application, such as the code packages for the services it encompasses. It also contains
    ///         an application manifest. When an application is instantiated from an application type, the application manifest
    ///         associated with the application type can be overridden. The application type must be uploaded before an application
    ///         can be created.
    ///     </para>
    /// </remarks>
    public sealed class ApplicationType
    {
        internal ApplicationType(
            string applicationTypeName, 
            string applicationTypeVersion, 
            ApplicationTypeStatus status,
            string statusDetails,
            ApplicationParameterList defaultParamList)
        {
            this.ApplicationTypeName = applicationTypeName;
            this.ApplicationTypeVersion = applicationTypeVersion;
            this.Status = status;
            this.StatusDetails = statusDetails;

            // Initialize this to empty collection rather than null. native side serializer does not handle null sometimes.
            this.DefaultParameters = defaultParamList ?? new ApplicationParameterList();
        }

        private ApplicationType() : this(null, null, ApplicationTypeStatus.Invalid, null, null) { }

        /// <summary>
        /// <para>Gets the application type name.</para>
        /// </summary>
        /// <value>
        /// <para>The application type name which is defined in the application manifest. This value, in conjunction
        /// with the application type version create a unique identifier for the application type.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public string ApplicationTypeName
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the application type version.</para>
        /// </summary>
        /// <value>
        /// <para>The application type version which is defined in the application manifest. This value, in conjunction
        /// with the application type name create a unique identifier for the application type. This value need not
        /// be numerical in value.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Version)]
        public string ApplicationTypeVersion
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the application type status.</para>
        /// </summary>
        /// <value>
        /// <para>The application type status which contains one of the values defined in the
        /// enumeration <see cref="System.Fabric.Query.ApplicationTypeStatus()" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Status)]
        public ApplicationTypeStatus Status
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the application type status details.</para>
        /// </summary>
        /// <value>
        /// <para>The application type status details. This contains information pertaining to provisioning of this application type.
        /// During provision, this contains progress information. Should provisioning fail, this provides the error message.
        /// This field is left blank otherwise.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.StatusDetails)]
        public string StatusDetails
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the default application parameters.</para>
        /// </summary>
        /// <value>
        /// <para>The default application parameters defined in the application manifest. 
        /// When an application is instantiated from this application type, these are the parameters used
        /// unless they are overridden. The instantiated application may also have more application parameters defined in
        /// addition to the ones defined in this property.
        /// </para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.DefaultParameterList)]
        public ApplicationParameterList DefaultParameters
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the definition kind.</para>
        /// </summary>
        /// <value>
        /// <para>The definition kind which contains one of the values defined in the
        /// enumeration <see cref="ApplicationTypeDefinitionKind" />.</para>
        /// <para>Specifies the mechanism the user userd to define a Service Fabric application type.</para>
        /// </value>
        public ApplicationTypeDefinitionKind ApplicationTypeDefinitionKind
        {
            get;
            private set;
        }

        internal static unsafe ApplicationType CreateFromNative(NativeTypes.FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM native)
        {
            var managed = new ApplicationType();

            managed.ApplicationTypeName = NativeTypes.FromNativeString(native.ApplicationTypeName);
            managed.ApplicationTypeVersion = NativeTypes.FromNativeString(native.ApplicationTypeVersion);
            managed.DefaultParameters = ApplicationParameterList.FromNative(native.DefaultParameters);

            if (native.Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_EX1*)native.Reserved;

                managed.Status = (ApplicationTypeStatus)ex1->Status;
                managed.StatusDetails = NativeTypes.FromNativeString(ex1->StatusDetails);

                if (ex1->Reserved != IntPtr.Zero)
                {
                    var ex2 = (NativeTypes.FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_EX2*)ex1->Reserved;

                    managed.ApplicationTypeDefinitionKind = (ApplicationTypeDefinitionKind)ex2->ApplicationTypeDefinitionKind;
                }
            }

            return managed;
        }
    }
}