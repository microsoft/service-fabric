// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Security;

    /// <summary>
    /// <para>Specifies a configuration setting and its value that can be used to configure a service or application.</para> 
    /// <para>The settings are specified in the settings.xml file in the service manifest. For more information see https://docs.microsoft.com/azure/service-fabric/service-fabric-application-model</para>
    /// </summary>
    public sealed class ConfigurationProperty
    {
        internal ConfigurationProperty()
        {
        }

        /// <summary>
        /// <para>Gets the name of the setting as specified in the settings.xml file in the service manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the setting.</para>
        /// </value>
        public string Name { get; internal set; }

        /// <summary>
        /// <para>Gets the value of the setting.</para>
        /// </summary>
        /// <value>
        /// <para>The value of the setting.</para>
        /// </value>
        public string Value { get; internal set; }

        /// <summary>
        /// <para>Gets a flag indicating whether the setting must be overridden in the application manifest.</para>
        /// </summary>
        /// <value>
        /// <para>Flag indicating whether the setting must be overridden in the application manifest.</para>
        /// </value>
        public bool MustOverride { get; internal set; }

        /// <summary>
        /// <para>Gets a flag indicating whether the configuration is encrypted. </para>
        /// </summary>
        /// <value>
        /// <para>Returns true if the configuration is encrypted; false, otherwise.</para>
        /// </value>
        public bool IsEncrypted { get; internal set; }

        /// <summary>
        /// <para>Gets the type of the setting "Encrypted/SecretsStore/Inline.</para>
        /// </summary>
        /// <value>
        /// <para>The value of the setting.</para>
        /// </value>
        public string Type { get; internal set; }

        /// <summary>
        /// <para>Decrypts an encrypted value and returns it as a SecureString.</para>       
        /// </summary>
        /// <returns>
        /// <para>Returns <see cref="System.Security.SecureString" />.</para>
        /// </returns>
        /// <exception cref="System.InvalidOperationException">
        /// <para>The value of the <see cref="System.Fabric.Description.ConfigurationProperty" /> is not encrypted.</para>
        /// </exception>
        /// <remarks>See https://docs.microsoft.com/azure/service-fabric/service-fabric-application-secret-management for an example on how to encrypt a secret and store it in the configuration and how to use this method to decrypt the value at runtime.</remarks>
        public SecureString DecryptValue()
        {
            if (this.IsEncrypted)
            {
                if (this.Value != null)
                {
                    return Utility.WrapNativeSyncInvokeInMTA(() => this.DecryptValueHelper(this.Value), "FabricDecryptText");
                }
                else
                {
                    SecureString secureString = new SecureString();
                    secureString.MakeReadOnly();
                    return secureString;
                }
            }
            else
            {
                throw new InvalidOperationException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_PropertyNotEncrypted_Formatted,
                        this.Name));
            }
        }

        internal static unsafe ConfigurationProperty CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_CONFIGURATION_PARAMETER native = *(NativeTypes.FABRIC_CONFIGURATION_PARAMETER*)nativeRaw;

            string name = NativeTypes.FromNativeString(native.Name);
            if (string.IsNullOrEmpty(name))
            {
                throw new ArgumentException(
                    StringResources.Error_ConfigParameterNameNullOrEmpty,
                    "nativeRaw");
            }

            string value = NativeTypes.FromNativeString(native.Value);

            string type= "";

            if (native.Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_CONFIGURATION_PARAMETER_EX1*)native.Reserved;
                type = NativeTypes.FromNativeString(ex1->Type);
            }

            return new ConfigurationProperty
            {
                Name = name,
                Value = NativeTypes.FromNativeString(native.Value),
                MustOverride = NativeTypes.FromBOOLEAN(native.MustOverride),
                IsEncrypted = NativeTypes.FromBOOLEAN(native.IsEncrypted),
                Type = type
            };
        }

        private SecureString DecryptValueHelper(string encryptedValue)
        {
            using (var pin = new PinCollection())
            {
                return StringResult.FromNativeToSecureString(NativeCommon.FabricDecryptText(
                    pin.AddBlittable(encryptedValue),
                    NativeTypes.FABRIC_X509_STORE_LOCATION.FABRIC_X509_STORE_LOCATION_LOCALMACHINE));
            }
        }
    }
}