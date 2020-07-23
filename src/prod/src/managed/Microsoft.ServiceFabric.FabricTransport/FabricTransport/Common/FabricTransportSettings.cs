// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Linq;
    using System.Security.Cryptography.X509Certificates;

    /// <summary>
    /// Settings that configures the  FabricTransport communication.
    /// </summary>
    internal class FabricTransportSettings
    {
        internal const string DefaultSectionName = "TransportSettings";
        private const uint DefaultQueueSize = 10000;
        //Got these values from experimenting 
        private static readonly int DefaultConcurrentCalls = 16*Environment.ProcessorCount;
        private const string TraceType = "FabricTransportSettings";
        private const string RemoteSecurityPrincipalName = "RemoteSecurityPrincipalName";
        private const string CertificateFindType = "CertificateFindType";
        private const string CertificateFindValue = "CertificateFindValue";
        private const string CertificateStoreLocation = "CertificateStoreLocation";
        private const string CertificateStoreName = "CertificateStoreName";
        private const string CertificateRemoteCommonNames = "CertificateRemoteCommonNames";
        private const string CertificateRemoteThumbprints = "CertificateRemoteThumbprints";
        private const string CertificateIssuerThumbprints = "CertificateIssuerThumbprints";
        private const string CertificateFindValuebySecondary = "CertificateFindValuebySecondary";
        private const string CertificateProtectionLevel = "CertificateProtectionLevel";
        private const string CertificateApplicationIssuerStorePrefix = "CertificateApplicationIssuerStore/";
        private const string SecurityCredentialsType = "SecurityCredentialsType ";
        private const string MaxQueueSizeSettingName = "MaxQueueSize";
        private const string MaxMessageSizeSettingName = "MaxMessageSize";
        private const string MaxConcurrentCallsSettingName = "MaxConcurrentCalls";
        private const string OperationTimeoutInSecondsSettingName = "OperationTimeoutInSeconds";
        private const string KeepAliveTimeoutInSecondsSettingName = "KeepAliveTimeoutInSeconds";
        private const string ConnectTimeoutInMillisecondsSettingName = "ConnectTimeoutInMilliseconds";
        internal static readonly uint DefaultMaxReceivedMessageSize = 4*1024*1024;
        internal static readonly TimeSpan DefaultConnectTimeout = TimeSpan.FromSeconds(5);
        internal static readonly TimeSpan DefaultOperationTimeout = TimeSpan.FromMinutes(5);
        internal static readonly TimeSpan DefaultKeepAliveTimeout = TimeSpan.Zero;

        /// <summary>
        /// Creates a new FabricTransportSettings with default Values.
        /// </summary>
        public FabricTransportSettings()
        {
            this.SecurityCredentials = new NoneSecurityCredentials();
            this.OperationTimeout = DefaultOperationTimeout;
            this.KeepAliveTimeout = DefaultKeepAliveTimeout;
            this.MaxMessageSize = DefaultMaxReceivedMessageSize;
            this.MaxQueueSize = DefaultQueueSize;
            this.MaxConcurrentCalls = DefaultConcurrentCalls;
            this.ConnectTimeout = DefaultConnectTimeout;
        }

        /// <summary>
        /// Operation Timeout  which governs the whole process of sending a message, including receiving a reply message for a request/reply service operation.
        ///  This timeout also applies when sending reply messages from a callback contract method.
        /// </summary>
        /// <value>OperationTimeout as <see cref="System.TimeSpan"/></value>
        /// <remarks>Default Value for Operation Timeout is set as 5 mins</remarks>
        public TimeSpan OperationTimeout { get; set; }

        /// <summary>
        /// KeepAliveTimeout is provides a way to configure  Tcp keep-alive option.
        /// </summary>
        /// <value>KeepAliveTimeout as <see cref="System.TimeSpan"/></value>
        /// <remarks>Default Value for KeepAliveTimeout Timeout is set as TimeSpan.Zero. which indicates we disable the tcp keepalive option.
        /// If you are using loadbalancer , you may need to configure this in order to avoid  the loadbalancer to close the connection after certain time </remarks>
        public TimeSpan KeepAliveTimeout { get; set; }

        /// <summary>
        /// Connect timeout specifies the maximum time allowed for the connection to be established successfully.
        /// </summary>
        /// <value>ConnectTimeout as <see cref="System.TimeSpan"/></value>
        /// <remarks>Default Value for ConnectTimeout Timeout is set as 5 seconds.</remarks>
        public TimeSpan ConnectTimeout { get; set; }

        /// <summary>
        /// MaxMessageSize represents  the maximum size for a message that can be received on a channel configured with this setting.
        /// </summary>
        /// <value>Maximum size of the message in bytes.
        /// </value>
        /// <remarks>
        /// Default Value for MaxMessageSize used is 4194304 bytes
        /// </remarks>
        public long MaxMessageSize { get; set; }

        /// <summary>
        /// The maximum size, of a queue that stores messages while they are processed for an endpoint configured with this setting. 
        /// </summary>
        /// <value> Max Size for a Queue that receives messages from the channel 
        /// </value>
        /// <remarks>
        /// Default value is 10,000 messages</remarks>
        public long MaxQueueSize { get; set; }

        ///<summary>
        ///MaxConcurrentCalls represents maximum number of messages actively service processes at one time.
        /// </summary>
        /// <value>
        ///MaxConcurrentCalls is  the upper limit of active messages in the service.
        /// </value>
        /// <remarks>
        ///    Default  value for the MaxConcurrentCalls is 16*Number of processors.
        /// </remarks>
        public long MaxConcurrentCalls { get; set; }

        /// <summary>
        /// Security credentials for securing the communication 
        /// </summary>
        /// <value>SecurityCredentials as  <see cref=" System.Fabric.SecurityCredentials"/>
        /// </value>
        /// <remarks>
        /// Default Value for SecurityCredentials is None
        /// SecurityCredential can be of type x509SecurityCredentail <seealso cref="System.Fabric.X509Credentials"/>or
        ///  WindowsCredentials <seealso cref="System.Fabric.WindowsCredentials"/>
        ///</remarks>
        public SecurityCredentials SecurityCredentials { get; set; }

        internal FabricServiceConfigSection ConfigSection { get; private set; }

        /// <summary>
        /// Loads the FabricTransport settings from a sectionName specified in the configuration file 
        /// Configuration File can be specified using the filePath or using the name of the configuration package specified in the service manifest .
        /// It will first try to load config using configPackageName . if configPackageName is not specified then try to load  from filePath.
        /// </summary>
        /// <param name="sectionName">Name of the section within the configuration file. If not found section in configuration file, it will throw ArgumentException</param>
        /// <param name="filepath"> Full path of the file where the settings will be loaded from. 
        ///  If not specified , it will first try to load from default Config Package"Config" , if not found then load from Settings "ClientExeName.Settings.xml" present in Client Exe directory. </param>
        ///  <param name="configPackageName"> Name of the configuration package.If its null or empty,it will check for file in filePath</param>
        /// <returns>FabricTransportSettings</returns>
        /// <remarks>
        /// The following are the parameter names that should be provided in the configuration file,to be recognizable by service fabric to load the transport settings.
        ///     
        ///     1. MaxQueueSize - <see cref="MaxQueueSize"/>value in long.
        ///     2. MaxMessageSize - <see cref="MaxMessageSize"/>value in bytes.
        ///     3. MaxConcurrentCalls - <see cref="MaxConcurrentCalls"/>value in long.
        ///     4. SecurityCredentials - <see cref="SecurityCredentials"/> value.
        ///     5. OperationTimeoutInSeconds - <see cref="OperationTimeout"/> value in seconds.
        ///     6. KeepAliveTimeoutInSeconds - <see cref="KeepAliveTimeout"/> value in seconds.
        ///     7. ConnectTimeoutInMilliseconds - <see cref="ConnectTimeout"/> value in milliseconds.
        /// </remarks>
        public static FabricTransportSettings LoadFrom(
            string sectionName,
            string filepath = null,
            string configPackageName = null)
        {
            bool isInitialized;
            var settings = new FabricTransportSettings();

            if (!string.IsNullOrEmpty(configPackageName))
            {
                isInitialized = settings.InitializeConfigFileFromConfigPackage(configPackageName);

                if (!isInitialized)
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            SR.ErrorConfigPackageNotFound,
                            configPackageName));
                }
            }
            else if (!string.IsNullOrEmpty(filepath))
            {
                isInitialized = settings.InitializeConfigFile(filepath);

                if (!isInitialized)
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            SR.ErrorConfigFileNotFound,
                            filepath));
                }
            }

            isInitialized = settings.InitializeSettingsFromConfig(sectionName);
            if (!isInitialized)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        SR.ErrorSectionNameNotFound,
                        sectionName));
            }

            AppTrace.TraceSource.WriteInfo(
                TraceType,
                "MaxMessageSize: {0} , MaxConcurrentCalls: {1} , MaxQueueSize: {2} , OperationTimeoutInSeconds: {3} KeepAliveTimeoutInSeconds : {4} , SecurityCredentials {5} , ConnectTimeoutInMilliseconds {6}",
                settings.MaxMessageSize,
                settings.MaxConcurrentCalls,
                settings.MaxQueueSize,
                settings.OperationTimeout.TotalSeconds,
                settings.KeepAliveTimeout.TotalSeconds,
                settings.SecurityCredentials.CredentialType,
                settings.ConnectTimeout.TotalMilliseconds);

            return settings;
        }

        /// <summary>
        /// Try to load the FabricTransport settings from a sectionName specified in the configuration file.
        /// Configuration File can be specified using the filePath or using the name of the configuration package specified in the service manifest .
        /// It will first try to load config using configPackageName . if configPackageName is not specified then try to load  from filePath.
        /// </summary>
        /// <param name="sectionName">Name of the section within the configuration file. If not found section in configuration file, it return false</param>
        /// <param name="filepath"> Full path of the file where the settings will be loaded from. 
        ///  If not specified , it will first try to load from default Config Package"Config" , if not found then load from Settings "ClientExeName.Settings.xml" present in Client Exe directory. </param>
        ///  <param name="configPackageName"> Name of the configuration package. If its null or empty,it will check for file in filePath</param>
        /// <param name="settings">When this method returns it sets the <see cref="FabricTransportSettings"/> settings if load from Config succeeded. If fails ,its sets settings to null/> </param>
        /// <returns><see cref="bool"/> specifies whether the settings get loaded successfully from Config.
        /// It returns true when load from Config succeeded, else return false. </returns>
        /// <remarks>
        /// The following are the parameter names that should be provided in the configuration file,to be recognizable by service fabric to load the transport settings.
        ///     
        ///     1. MaxQueueSize - <see cref="MaxQueueSize"/>value in long.
        ///     2. MaxMessageSize - <see cref="MaxMessageSize"/>value in bytes.
        ///     3. MaxConcurrentCalls - <see cref="MaxConcurrentCalls"/>value in long.
        ///     4. SecurityCredentials - <see cref="SecurityCredentials"/> value.
        ///     5. OperationTimeoutInSeconds - <see cref="OperationTimeout"/> value in seconds.
        ///     6. KeepAliveTimeoutInSeconds - <see cref="KeepAliveTimeout"/> value in seconds.
        ///     7. ConnectTimeoutInMilliseconds - <see cref="ConnectTimeout"/> value in milliseconds.
        /// </remarks>
        public static bool TryLoadFrom(string sectionName, out FabricTransportSettings settings, string filepath = null,
            string configPackageName = null)
        {
            try
            {
                bool isInitialized;
                var fabricTransportSettings = new FabricTransportSettings();

                if (!string.IsNullOrEmpty(configPackageName))
                {
                    isInitialized = fabricTransportSettings.InitializeConfigFileFromConfigPackage(configPackageName);

                    if (!isInitialized)
                    {
                        settings = null;
                        return false;
                    }
                }
                else if (!string.IsNullOrEmpty(filepath))
                {
                    isInitialized = fabricTransportSettings.InitializeConfigFile(filepath);

                    if (!isInitialized)
                    {
                        settings = null;
                        return false;
                    }
                }

                isInitialized = fabricTransportSettings.InitializeSettingsFromConfig(sectionName);
                if (!isInitialized)
                {
                    settings = null;
                    return false;
                }

                settings = fabricTransportSettings;

                AppTrace.TraceSource.WriteInfo(
                    TraceType,
                    "MaxMessageSize: {0} , MaxConcurrentCalls: {1} , MaxQueueSize: {2} , OperationTimeoutInSeconds: {3} KeepAliveTimeoutInSeconds : {4} , SecurityCredentials {5} , ConnectTimeoutInMilliseconds {6}",
                    settings.MaxMessageSize,
                    settings.MaxConcurrentCalls,
                    settings.MaxQueueSize,
                    settings.OperationTimeout.TotalSeconds,
                    settings.KeepAliveTimeout.TotalSeconds,
                    settings.SecurityCredentials.CredentialType,
                    settings.ConnectTimeout.TotalMilliseconds);


                return true;
            }
            catch (Exception ex)
            {
                // return false if load from fails
                AppTrace.TraceSource.WriteWarning(TraceType, "Exception thrown while loading from Config {0}", ex);
                settings = null;
                return false;
            }
        }

        /// <summary>
        /// FabricTransportSettings returns the default Settings .Loads the configuration file from default Config Package"Config" , if not found then try to load from  default config file "ClientExeName.Settings.xml"  from Client Exe directory.
        ///</summary>
        /// <param name="sectionName">Name of the section within the configuration file. If not found section in configuration file, it will return the default Settings</param>
        /// <returns></returns>
        internal static FabricTransportSettings GetDefault(string sectionName = DefaultSectionName)
        {
            FabricTransportSettings settings = null;
            if (!TryLoadFrom(sectionName, out settings))
            {
                settings = new FabricTransportSettings();
                AppTrace.TraceSource.WriteInfo(
                    TraceType,
                    "Loading Default Settings , MaxMessageSize: {0} , MaxConcurrentCalls: {1} , MaxQueueSize: {2} , OperationTimeoutInSeconds: {3} KeepAliveTimeoutInSeconds : {4} , SecurityCredentials {5}",
                    settings.MaxMessageSize,
                    settings.MaxConcurrentCalls,
                    settings.MaxQueueSize,
                    settings.OperationTimeout.TotalSeconds,
                    settings.KeepAliveTimeout.TotalSeconds,
                    settings.SecurityCredentials.CredentialType);
            }
            return settings;
        }

        internal bool InitializeConfigFileFromConfigPackage(string configPackageName)
        {
            return FabricServiceConfig.InitializeFromConfigPackage(configPackageName);
        }

        internal bool InitializeConfigFile(string filePath)
        {
            return FabricServiceConfig.Initialize(filePath);
        }

        internal bool InitializeSettingsFromConfig(string sectionName)
        {
            this.ConfigSection = new FabricServiceConfigSection((sectionName ?? DefaultSectionName), this.OnInitialize);
            return this.ConfigSection.Initialize();
        }

        internal virtual void OnInitialize()
        {
            this.MaxConcurrentCalls = this.ConfigSection.GetSetting<long>(MaxConcurrentCallsSettingName,
                DefaultConcurrentCalls);
            this.MaxMessageSize = this.ConfigSection.GetSetting<long>(MaxMessageSizeSettingName,
                DefaultMaxReceivedMessageSize);
            this.MaxQueueSize = this.ConfigSection.GetSetting<long>(MaxQueueSizeSettingName, DefaultQueueSize);
            var operationTimeoutInSeconds = this.ConfigSection.GetSetting<double>(OperationTimeoutInSecondsSettingName,
                0);
            if (operationTimeoutInSeconds == 0)
            {
                this.OperationTimeout = DefaultOperationTimeout;
            }
            else
            {
                this.OperationTimeout = TimeSpan.FromSeconds(operationTimeoutInSeconds);
            }
            var keepAliveTimeoutInSeconds = this.ConfigSection.GetSetting<double>(KeepAliveTimeoutInSecondsSettingName,
                0);

            if (keepAliveTimeoutInSeconds == 0)
            {
                this.KeepAliveTimeout = DefaultKeepAliveTimeout;
            }
            else
            {
                this.KeepAliveTimeout = TimeSpan.FromSeconds(keepAliveTimeoutInSeconds);
            }

            var ConnectTimeoutInMilliSeconds =
                this.ConfigSection.GetSetting<double>(ConnectTimeoutInMillisecondsSettingName, 0);

            if (ConnectTimeoutInMilliSeconds == 0)
            {
                this.ConnectTimeout = DefaultConnectTimeout;
            }
            else
            {
                this.ConnectTimeout = TimeSpan.FromMilliseconds(ConnectTimeoutInMilliSeconds);
            }

            this.SecurityCredentials = this.LoadSecurityCredential();
        }


        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeObj = new NativeTypes.FABRIC_SERVICE_TRANSPORT_SETTINGS();
            nativeObj.Reserved = IntPtr.Zero;

            if (this.SecurityCredentials != null)
            {
                nativeObj.SecurityCredentials = this.SecurityCredentials.ToNative(pin);
            }
            else
            {
                nativeObj.SecurityCredentials = IntPtr.Zero;
            }

            if (this.OperationTimeout.TotalSeconds < 0)
            {
                nativeObj.OperationTimeoutInSeconds = 0;
            }
            else
            {
                nativeObj.OperationTimeoutInSeconds = (uint) this.OperationTimeout.TotalSeconds;
            }

            if (this.KeepAliveTimeout.TotalSeconds < 0)
            {
                nativeObj.KeepAliveTimeoutInSeconds = 0;
            }
            else
            {
                nativeObj.KeepAliveTimeoutInSeconds = (uint) this.KeepAliveTimeout.TotalSeconds;
            }

            var ex1settings = new NativeTypes.FABRIC_SERVICE_TRANSPORT_SETTINGS_EX1();

            if (this.ConnectTimeout.TotalMilliseconds < 0)
            {
                ex1settings.ConnectTimeoutInMilliseconds = (uint) DefaultConnectTimeout.TotalMilliseconds;
            }
            else
            {
                ex1settings.ConnectTimeoutInMilliseconds = (uint) this.ConnectTimeout.TotalMilliseconds;
            }

            nativeObj.Reserved = pin.AddBlittable(ex1settings);

            Helper.ThrowIfValueOutOfBounds(this.MaxMessageSize, MaxMessageSizeSettingName);

            nativeObj.MaxMessageSize = (uint) this.MaxMessageSize;

            Helper.ThrowIfValueOutOfBounds(this.MaxConcurrentCalls, MaxConcurrentCallsSettingName);
            nativeObj.MaxConcurrentCalls = (uint) this.MaxConcurrentCalls;

            Helper.ThrowIfValueOutOfBounds(this.MaxQueueSize, MaxQueueSizeSettingName);

            nativeObj.MaxQueueSize = (uint) this.MaxQueueSize;

            return pin.AddBlittable(nativeObj);
        }

        private SecurityCredentials LoadSecurityCredential()
        {
            var credentialType = this.ConfigSection.GetSetting<CredentialType>(SecurityCredentialsType,
                CredentialType.None);
            switch (credentialType)
            {
                case CredentialType.X509:
                    return this.X509SecurityCredentialsBuilder();
                case CredentialType.Windows:
                    return this.WindowsSecurityCredentialsBuilder();
            }

            return new NoneSecurityCredentials();
        }

        private SecurityCredentials WindowsSecurityCredentialsBuilder()
        {
            var windowsCredentials = new WindowsCredentials();
            windowsCredentials.RemoteSpn = this.ConfigSection.GetSetting<string>(RemoteSecurityPrincipalName, null);
            return windowsCredentials;
        }

        private SecurityCredentials X509SecurityCredentialsBuilder()
        {
            var x509SecurityCredential = new X509Credentials();
            x509SecurityCredential.FindType = this.ConfigSection.GetSetting<X509FindType>(CertificateFindType,
                x509SecurityCredential.FindType);
            x509SecurityCredential.ProtectionLevel = this.ConfigSection.GetSetting<ProtectionLevel>(
                CertificateProtectionLevel,
                x509SecurityCredential.ProtectionLevel);
            x509SecurityCredential.FindValue = this.ConfigSection.GetSetting<object>(CertificateFindValue,
                x509SecurityCredential.FindValue);

            x509SecurityCredential.StoreLocation =
                this.ConfigSection.GetSetting<StoreLocation>(CertificateStoreLocation,
                    x509SecurityCredential.StoreLocation);

            x509SecurityCredential.StoreName = this.ConfigSection.GetSetting<string>(CertificateStoreName,
                x509SecurityCredential.StoreName);
            var remoteCommonNames = this.ConfigSection.GetSettingsList<string>(CertificateRemoteCommonNames);

            foreach (var name in remoteCommonNames)
            {
                x509SecurityCredential.RemoteCommonNames.Add(name);
            }

            var remoteCertThumbPrints = this.ConfigSection.GetSettingsList<string>(CertificateRemoteThumbprints);
            foreach (var name in remoteCertThumbPrints)
            {
                x509SecurityCredential.RemoteCertThumbprints.Add(name);
            }

            var issuerCertThumbPrints = this.ConfigSection.GetSettingsList<string>(CertificateIssuerThumbprints);
            foreach (var name in issuerCertThumbPrints)
            {
                x509SecurityCredential.IssuerThumbprints.Add(name);
            }

            x509SecurityCredential.FindValueSecondary =
                this.ConfigSection.GetSetting<object>(CertificateFindValuebySecondary,
                    x509SecurityCredential.FindValueSecondary);

            var issuerCertStores = this.ConfigSection.GetSettingsMapFromPrefix(CertificateApplicationIssuerStorePrefix);
            var remoteCertIssuers = new List<X509IssuerStore>();
            foreach (var issuerCertStore in issuerCertStores)
            {
                var issuerStoreLocations = issuerCertStore.Value.Split(',').ToList();
                remoteCertIssuers.Add(new X509IssuerStore(issuerCertStore.Key, issuerStoreLocations));
            }
            if (remoteCertIssuers.Count !=0)
            {
                x509SecurityCredential.RemoteCertIssuers = remoteCertIssuers;
            }

            return x509SecurityCredential;
        }
    }
}