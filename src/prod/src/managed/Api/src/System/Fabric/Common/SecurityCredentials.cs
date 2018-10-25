// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    ///   <para>An abstract base class for types that represent security credentials.</para>
    /// </summary>
    public abstract class SecurityCredentials
    {
        internal SecurityCredentials(CredentialType type)
        {
            this.CredentialType = type;
        }

        /// <summary>
        ///   <para>Indicates the type of security credentials to use in order to secure the cluster – valid values are "none", "x509", "Windows".</para>
        /// </summary>
        /// <value>
        ///   <para>The type of security credentials to use in order to secure the cluster.</para>
        /// </value>
        public CredentialType CredentialType { get; protected set; }

        /// <summary>
        /// <para>Instantiate <see cref="System.Fabric.SecurityCredentials" /> object from service configuration settings file</para>
        /// </summary>
        /// <param name="codePackageActivationContext"><para>The current code package activation context <see cref="System.Fabric.CodePackageActivationContext" />.</para></param>
        /// <param name="configPackageName"><para>The current configuration package name.</para></param>
        /// <param name="sectionName"><para>The section within the configuration file that defines all the security settings.</para></param>
        /// <returns>The security credentials.</returns>
        /// <remarks>
        /// <para> The configuration settings file (settings.xml) within the service configuration folder should contain all the security settings that is needed to create
        /// <see cref="System.Fabric.SecurityCredentials" /> object and pass to the <see cref="System.Fabric.IStatefulServicePartition.CreateReplicator" /> method.
        /// Typically, the onus is on the service author to read the settings.xml file, parse the values and appropriately construct the <see cref="System.Fabric.SecurityCredentials" /> object.</para>
        /// <para>With the current helper method, the service author can bypass the above process.</para>
        /// <para>The following are the parameter names that should be provided in the service configuration "settings.xml", to be recognizable by windows fabric to perform the above parsing automatically:</para>
        /// <list type="number">
        /// <item>
        /// <description>
        /// <para>CredentialType–type of credentials to use to secure communication channel: X509 (X509 certificate credentials) or Windows (Windows credentials, requires active directory)</para>
        /// </description>
        /// </item>
        /// </list>
        /// <para> CredentialType=X509</para>
        /// <list type="number">
        /// <item>
        /// <description>
        /// <para>StoreLocation-Store location to find the certificate: CurrentUser or LocalMachine</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>StoreName-name of the certificate store where the certificate should be searched</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>FindType-Identifies the type of value provided by in the FindValue parameter: FindBySubjectName or FindByThumbPrint</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>FindValue-Search target for finding the certificate</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>AllowedCommonNames-A comma separated list of certificate common names/dns names. This list should include all certificates used by replicators, it is used to validate incoming certificate.</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>IssuerThumbprints-A comma separated list of issuer certificate thumbprints. When specified, the incoming certificate is validated if it is issued by one of the entries in the list, in addition to chain validation.</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>ApplicationIssuerStore/[IssuerCommonName]-A comma separated list of store names where issuer certificate corresponding to IssuerCommonName can be found. When specified, the incoming certificate is validated if it is issued by one of the entries in the list, in addition to chain validation.</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>RemoteCertThumbprints-A comma separated list of certificate thumbprints. This list should include all certificates used by replicators, it is used to validate incoming certificate.</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>ProtectionLevel-Indicates how the data is protected: Sign or EncryptAndSign or None.</para>
        /// </description>
        /// </item>
        /// </list>
        /// <para> CredentialType=Windows</para>
        /// <list type="number">
        /// <item>
        /// <description>
        /// <para>ServicePrincipalName-Service Principal name registered for the service. Can be empty if the service/actor host processes runs as a machine account (e.g: NetworkService, LocalSystem etc.)</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>WindowsIdentities-A comma separated list of windows identities of all service/actor host processes.</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>ProtectionLevel-Indicates how the data is protected: Sign or EncryptAndSign or None.</para>
        /// </description>
        /// </item>
        /// </list>
        /// <para>X509 configuration snippet sample</para>
        /// <code>
        /// &lt;Section Name="SecurityConfig"&gt;
        ///   &lt;Parameter Name="CredentialType" Value="X509" /&gt;
        ///   &lt;Parameter Name="FindType" Value="FindByThumbprint" /&gt;
        ///   &lt;Parameter Name="FindValue" Value="9d c9 06 b1 69 dc 4f af fd 16 97 ac 78 1e 80 67 90 74 9d 2f" /&gt;
        ///   &lt;Parameter Name="StoreLocation" Value="LocalMachine" /&gt;
        ///   &lt;Parameter Name="StoreName" Value="My" /&gt;
        ///   &lt;Parameter Name="ProtectionLevel" Value="EncryptAndSign" /&gt;
        ///   &lt;Parameter Name="AllowedCommonNames" Value="My-Test-SAN1-Alice,My-Test-SAN1-Bob" /&gt;
        ///   &lt;Parameter Name="ApplicationIssuerStore/WinFabric-Test-TA-CA" Value="Root" /&gt;
        /// &lt;/Section&gt;
        /// </code>
        /// <para>Windows configuration snippet sample 1: all the service/actor host processes run as NetworkService or LocalSystem.</para>
        /// <code>&lt;Section Name="SecurityConfig"&gt;
        ///   &lt;Parameter Name="CredentialType" Value="Windows" /&gt;
        ///   &lt;Parameter Name="ServicePrincipalName" Value="" /&gt;
        ///   &lt;!--This machine group contains all machines in a cluster--&gt;
        ///   &lt;Parameter Name="WindowsIdentities" Value="redmond\ClusterMachineGroup" /&gt;
        ///   &lt;Parameter Name="ProtectionLevel" Value="EncryptAndSign" /&gt;
        /// &lt;/Section&gt;</code>
        /// <para>Windows configuration snippet sample 1: all service/actor host processes run as a group managed service account.</para>
        /// <code>&lt;Section Name="SecurityConfig"&gt;
        ///   &lt;Parameter Name="CredentialType" Value="Windows" /&gt;
        ///   &lt;Parameter Name="ServicePrincipalName" Value="servicefabric/cluster.microsoft.com" /&gt;
        ///   &lt;--All actor/service host processes run as redmond\GroupManagedServiceAccount--&gt;
        ///   &lt;Parameter Name="WindowsIdentities" Value="redmond\GroupManagedServiceAccount" /&gt;
        ///   &lt;Parameter Name="ProtectionLevel" Value="EncryptAndSign" /&gt;
        /// &lt;/Section&gt;</code>
        /// </remarks>
        public static SecurityCredentials LoadFrom(CodePackageActivationContext codePackageActivationContext, string configPackageName, string sectionName)
        {
            return Utility.WrapNativeSyncInvokeInMTA<SecurityCredentials>(
                () => LoadFromPrivate(codePackageActivationContext, configPackageName, sectionName), "NativeFabricRuntime::FabricLoadSecurityCredentials");
        }

        private static SecurityCredentials LoadFromPrivate(CodePackageActivationContext codePackageActivationContext, string configPackageName, string sectionName)
        {
            using (var pin = new PinCollection())
            {
                var nativeSectionName = pin.AddBlittable(sectionName);
                var nativeConfigPackageName = pin.AddBlittable(configPackageName);
                NativeRuntime.IFabricSecurityCredentialsResult securityCredentialsResult =
                    NativeRuntime.FabricLoadSecurityCredentials(
                        codePackageActivationContext.NativeActivationContext,
                        nativeConfigPackageName,
                        nativeSectionName);

                return SecurityCredentials.CreateFromNative(securityCredentialsResult);
            }
        }
        
        /// <summary>
        /// <para>Instantiate <see cref="System.Fabric.SecurityCredentials" /> object from service configuration settings file</para>
        /// </summary>
        /// <returns>The security credentials.</returns>
        internal static SecurityCredentials LoadClusterSettings()
        {
            return Utility.WrapNativeSyncInvokeInMTA<SecurityCredentials>(
                () => LoadClusterSettingsFromPrivate(), "NativeFabricRuntime::FabricLoadClusterSecurityCredentials");
        }

        private static SecurityCredentials LoadClusterSettingsFromPrivate()
        {
            using (var pin = new PinCollection())
            {
                NativeRuntime.IFabricSecurityCredentialsResult securityCredentialsResult = NativeRuntimeInternal.FabricLoadClusterSecurityCredentials();         

                return SecurityCredentials.CreateFromNative(securityCredentialsResult);
            }
        }        

        internal static unsafe SecurityCredentials CreateFromNative(NativeRuntime.IFabricSecurityCredentialsResult securityCredentialsResult)
        {
            ReleaseAssert.AssertIfNot(securityCredentialsResult != null, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "SecurityCredentials"));
            ReleaseAssert.AssertIfNot(securityCredentialsResult.get_SecurityCredentials() != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "SecurityCredentialsResult.get_SecurityCredentials()"));

            NativeTypes.FABRIC_SECURITY_CREDENTIALS* nativeCredentials = (NativeTypes.FABRIC_SECURITY_CREDENTIALS*)securityCredentialsResult.get_SecurityCredentials();
            SecurityCredentials managedCredentials = CreateFromNative(nativeCredentials);
            
            GC.KeepAlive(securityCredentialsResult);
            return managedCredentials;
        }

        internal static unsafe SecurityCredentials CreateFromNative(NativeTypes.FABRIC_SECURITY_CREDENTIALS* nativeCredentials)
        {
            SecurityCredentials managedCredentials = null;

            switch(nativeCredentials->Kind)
            {
                case NativeTypes.FABRIC_SECURITY_CREDENTIAL_KIND.FABRIC_SECURITY_CREDENTIAL_KIND_CLAIMS:
                    managedCredentials = ClaimsCredentials.CreateFromNative((NativeTypes.FABRIC_CLAIMS_CREDENTIALS*)nativeCredentials->Value);
                    break;

                case NativeTypes.FABRIC_SECURITY_CREDENTIAL_KIND.FABRIC_SECURITY_CREDENTIAL_KIND_WINDOWS:
                    managedCredentials = WindowsCredentials.CreateFromNative((NativeTypes.FABRIC_WINDOWS_CREDENTIALS*)nativeCredentials->Value);
                    break;

                case NativeTypes.FABRIC_SECURITY_CREDENTIAL_KIND.FABRIC_SECURITY_CREDENTIAL_KIND_X509:
                    managedCredentials = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*)nativeCredentials->Value);
                    break;

                case NativeTypes.FABRIC_SECURITY_CREDENTIAL_KIND.FABRIC_SECURITY_CREDENTIAL_KIND_NONE:
                    managedCredentials = NoneSecurityCredentials.CreateFromNative();
                    break;

                default:
                    AppTrace.TraceSource.WriteError("SecurityCredentials.FromNative", "Unknown credential type: {0}", nativeCredentials->Kind);
                    ReleaseAssert.Failfast("Unknown credential type: {0}", nativeCredentials->Kind);
                    break;
            }

            return managedCredentials;
        }

        internal static unsafe ProtectionLevel CreateFromNative(NativeTypes.FABRIC_PROTECTION_LEVEL protectionLevel)
        {
            switch(protectionLevel)
            {
                case NativeTypes.FABRIC_PROTECTION_LEVEL.FABRIC_PROTECTION_LEVEL_ENCRYPTANDSIGN:
                    return ProtectionLevel.EncryptAndSign;
                case NativeTypes.FABRIC_PROTECTION_LEVEL.FABRIC_PROTECTION_LEVEL_NONE:
                    return ProtectionLevel.None;
                case NativeTypes.FABRIC_PROTECTION_LEVEL.FABRIC_PROTECTION_LEVEL_SIGN:
                    return ProtectionLevel.Sign;
                default:
                    throw new InvalidOperationException(StringResources.Error_InvalidProtectionLevel);
            }
        }
    }
}

