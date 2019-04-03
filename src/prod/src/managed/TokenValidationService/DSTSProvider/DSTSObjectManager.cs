// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.TokenValidationService
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;

    using Microsoft.IdentityModel.Claims;
    using Microsoft.WindowsAzure.Security.Authentication;
    using Microsoft.WindowsAzure.Security.Authentication.Contracts;
    using System.ServiceModel.Channels;

    using Microsoft.Win32;

    using Collections.Generic;

    internal sealed class DSTSObjectManager : IConfigStoreUpdateHandler
    {
        DSTSSettings settings;
        private IServerHomeDsts serverHomeDsts;
        private IServiceIdentity serviceIdentity;
        WebSecurityTokenAuthenticator tokenAuthenticator;
        ServerAuthenticationProvider authenticationProvider;
        ReaderWriterLockSlim reloadLock;
        private const int LockTimeout = 1000 * 60; // 1 minute timeout
        NativeConfigStore configStore;
        
        private static readonly string TraceType = "DSTSObjectManager";
        private static readonly string SecuritySection = "Security";
        private static readonly string ClaimsAuthEnabledKey = "ClientClaimAuthEnabled";
        private string configSection;

        private bool claimsAuthEnabled;
        public DSTSObjectManager(string configSection)
        {
            this.configSection = configSection;
            this.reloadLock = new ReaderWriterLockSlim();
            this.configStore = NativeConfigStore.FabricGetConfigStore(this);
            this.claimsAuthEnabled = false;

            try
            {
                this.InitializeDSTSObjects();
            }
            catch (Exception e)
            {
                TokenValidationServiceFactory.TraceSource.WriteError(DSTSObjectManager.TraceType,
                        "Failed to update DSTSObjects due to exception {0}, {1}",
                        e.GetType(),
                        e.Message);
                throw;
            }
        }

        private bool InitializeDSTSObjects()
        {
            //initialize objects first and then update member variables under lock to reduce lock duration
            string claimsAuth = this.configStore.ReadUnencryptedString(SecuritySection, ClaimsAuthEnabledKey);

            if (Boolean.TryParse(claimsAuth, out this.claimsAuthEnabled) &&
                this.claimsAuthEnabled &&
                this.configStore.GetSections(this.configSection).Count > 0)
            {
                var settings = new DSTSSettings(configSection);
                settings.LoadSettings(this.configStore);
                IServerHomeDsts serverHomeDsts = new ServerHomeDsts(new Uri(settings.DSTSRealm), settings.DSTSDnsName);

                IServiceIdentity serviceIdentity = new ServiceIdentity(settings.ServiceDnsName, settings.ServiceName);
                var configuration = new DatacenterServiceConfiguration(serverHomeDsts, serviceIdentity);
                WebSecurityTokenAuthenticator tokenAuthenticator = new WebSecurityTokenAuthenticator(configuration);

                ServerAuthenticationProvider authenticationProvider = new ServerAuthenticationProvider(configuration);
                if (!this.reloadLock.TryEnterWriteLock(DSTSObjectManager.LockTimeout))
                {
                    TokenValidationServiceFactory.TraceSource.WriteWarning(
                        DSTSObjectManager.TraceType,
                        "Failed to obtain write lock on InitializeDSTSObjects");
                    return false;
                }
                bool retval = false;
                try
                {
                    this.settings = settings;
                    this.serverHomeDsts = serverHomeDsts;
                    this.serviceIdentity = serviceIdentity;
                    this.tokenAuthenticator = tokenAuthenticator;
                    this.authenticationProvider = authenticationProvider;

                    retval = true;
                    TokenValidationServiceFactory.TraceSource.WriteInfo(
                        DSTSObjectManager.TraceType,
                        "DSTSSettings and DSTSObjects initialized for ServiceName {0}",
                        this.settings.ServiceName);

                }
                finally
                {
                    this.reloadLock.ExitWriteLock();
                }
                return retval;
            }
            else
            {
                TokenValidationServiceFactory.TraceSource.WriteWarning(
                        DSTSObjectManager.TraceType,
                        "TVS is in disabled mode because section {0} key {1} has value {2}",
                        SecuritySection,
                        ClaimsAuthEnabledKey,
                        claimsAuth);
                return true;
            }
        }

        // Return value:
        //
        // true = Dynamic config update success, process should not restart.
        // false = Dynamic config update failure, process should restart.
        //
        public bool OnUpdate(string section, string key)
        {
            bool success = false;
            if (String.Equals(section, this.configSection))
            {
                TokenValidationServiceFactory.TraceSource.WriteInfo(
                    DSTSObjectManager.TraceType,
                    "Update config notification for section {0} key {1}, reloading DSTSSettings",
                    section,
                    key);

                try
                {
                    success = this.InitializeDSTSObjects();
                }
                catch (Exception e)
                {
                    TokenValidationServiceFactory.TraceSource.WriteError(DSTSObjectManager.TraceType,
                        "Failed to update DSTSObjects due to exception {0}, {1} return {2}",
                        e.GetType(),
                        e.Message,
                        success);
                }
            }
            else
            {
                success = true;
            }

            return success;
        }

        public bool CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted)
        {
            throw new InvalidOperationException(System.Fabric.Strings.StringResources.Error_InvalidOperation);
        }

        internal ClaimsIdentityCollection ValidateToken(string authToken)
        {
            if (this.claimsAuthEnabled)
            {
                if (!this.reloadLock.TryEnterReadLock(DSTSObjectManager.LockTimeout))
                {
                    TokenValidationServiceFactory.TraceSource.WriteWarning(
                        DSTSObjectManager.TraceType,
                        "Failed to obtain read lock on ValidateToken");

                    throw new InvalidOperationException("Failed to get read lock within required time for validating token");
                }
                try
                {
                    return this.tokenAuthenticator.Authenticate(authToken);
                }
                catch (Exception e)
                {
                    TokenValidationServiceFactory.TraceSource.WriteWarning(
                        DSTSObjectManager.TraceType,
                        "Exception {0} with Message {1} occured while validating dsts token", e.GetType(), e.Message);
                    throw;
                }
                finally
                {
                    this.reloadLock.ExitReadLock();
                }
            }
            else
            {
                TokenValidationServiceFactory.TraceSource.WriteWarning(
                        DSTSObjectManager.TraceType,
                        "ValidateToken method called when TVS is disabled");
                throw new InvalidOperationException(System.Fabric.Strings.StringResources.Error_InvalidOperation);
            }
        }

        internal TokenServiceMetadata GetDSTSMetadata()
        {
            if (this.claimsAuthEnabled)
            {
                if (!this.reloadLock.TryEnterReadLock(DSTSObjectManager.LockTimeout))
                {
                    TokenValidationServiceFactory.TraceSource.WriteWarning(
                            DSTSObjectManager.TraceType,
                            "Failed to obtain read lock on GetDSTSMetadata");
                    throw new InvalidOperationException("Failed to get read lock within required time for getting token metadata");
                }
                try
                {
                    TokenValidationServiceFactory.TraceSource.WriteNoise(
                        DSTSObjectManager.TraceType,
                        "GetDSTSMetadata invoked for service {0} with DNS name {1}",
                         settings.ServiceName,
                         settings.ServiceDnsName);
                    object metadata = this.authenticationProvider.CreateAuthenticationMetadata();
                    using (Message metadataMessage = Message.CreateMessage(MessageVersion.Default, "", metadata))
                    {
                        return new TokenServiceMetadata(
                            TVSSerializerUtility.Serialize(metadataMessage), 
                            settings.ServiceName, 
                            settings.ServiceDnsName);
                    }
                }
                finally
                {
                    this.reloadLock.ExitReadLock();
                }
            }
            else
            {
                TokenValidationServiceFactory.TraceSource.WriteWarning(
                       DSTSObjectManager.TraceType,
                       "GetDSTSMetadata method called when TVS is disabled");
                throw new InvalidOperationException(System.Fabric.Strings.StringResources.Error_InvalidOperation);
            }
        }
    }
}