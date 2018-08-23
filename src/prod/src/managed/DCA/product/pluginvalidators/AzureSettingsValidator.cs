// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Security;
    using System.Diagnostics;
    using System.Globalization;
    using System.Collections.Generic;
    using System.Linq;
    using System.Fabric.Dca.Validator;
    using System.Text.RegularExpressions;

    // This class implements validation for DCA Azure consumer settings in the cluster
    // manifest
    public abstract class ClusterManifestAzureSettingsValidator : ClusterManifestSettingsValidator
    {
        private const string DeploymentIdRegularExpression = "^[A-Za-z0-9]{1,16}$";
        protected bool connectionStringSpecified;

        protected string deploymentId;
        
        public SecureString ConnectionStringUsedWithAzureDiagnosticManager {get; protected set;}

        public int PerformanceCounterTransferIntervalUsedWithAzureDiagnosticManager { get; protected set; }
        
        public IEnumerable<AzureBlobContainerInfo> BlobContainerInfoList 
        {
            get
            {
                return this.blobContainerInfoList;
            }
        }
        protected List<AzureBlobContainerInfo> blobContainerInfoList;

        public IEnumerable<AzureTableInfo> TableInfoList 
        {
            get
            {
                return this.tableInfoList;
            }
        }
        protected List<AzureTableInfo> tableInfoList;

        internal ClusterManifestAzureSettingsValidator()
        {
            this.ConnectionStringUsedWithAzureDiagnosticManager = null;
            this.blobContainerInfoList = new List<AzureBlobContainerInfo>();
            this.tableInfoList = new List<AzureTableInfo>();
            return;
        }

        protected bool ConnectionStringHandler()
        {
            if (this.parameters.ContainsKey(AzureConstants.ConnectionStringParamName))
            {
                // Check to make sure that the connection string is not an empty string. 
                // This is mainly to catch the case where someone picks up a sample
                // manifest and tries to deploy without specifying their connection
                // string first. Therefore we make this check only if the connection
                // string is unencrypted. 
                string connectionString = this.parameters[AzureConstants.ConnectionStringParamName];
                StorageConnection unusedRef = new StorageConnection();
                if (!ConnectionStringParser.ParseConnectionString(
                    connectionString.ToCharArray(), 
                    message =>
                        {
                            this.traceSource.WriteError(
                                   this.logSourceId,
                                   "Section {0}, parameter {1}: {2}.",
                                   this.sectionName,
                                   AzureConstants.ConnectionStringParamName,
                                   message);
                        },
                    ref unusedRef))
                {
                    return false;
                }
            }
            this.connectionStringSpecified = true;
            return true;
        }

        protected bool GetConnectionString(string connectionStringParamName, out SecureString connectionString)
        {
            connectionString = null;
            if (false == this.connectionStringSpecified)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Consumer {0} is enabled, but the required parameter {1} for this consumer has not been provided.",
                    this.sectionName,
                    connectionStringParamName);
                return false;
            }

            if (this.parameters.ContainsKey(connectionStringParamName))
            {
                char[] connectionStringCharArray = this.parameters[connectionStringParamName].ToCharArray();
                connectionString = new SecureString();
                foreach (char c in connectionStringCharArray)
                {
                    connectionString.AppendChar(c);
                }
                connectionString.MakeReadOnly();
            }
            else
            {
                Debug.Assert(this.encryptedParameters.ContainsKey(connectionStringParamName));
                connectionString = this.encryptedParameters[connectionStringParamName];
            }
            return true;
        }

        protected bool DeploymentIdHandler()
        {
            this.deploymentId = this.parameters[AzureConstants.DeploymentId];
            Regex regEx = new Regex(DeploymentIdRegularExpression);

            if (false == regEx.IsMatch(deploymentId))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The value for parameter {0} in section {1} does not match the regular expression {2}.",
                    AzureConstants.DeploymentId,
                    this.sectionName,
                    DeploymentIdRegularExpression);
                return false;
            }
            return true;
        }
    }
}