// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.Validator
{
    using System.Collections.Generic;
    using System.Security;
    
    // Interface representing the validator of a DCA plugin's settings in the cluster manifest
    public interface IDcaSettingsValidator
    {
        // Validates a set of name-value pairs that are passed in as dictionaries.
        // There is a dictionary of strings for unencrypted parameters and another
        // dictionary of SecureStrings for encrypted parameters.
        bool Validate(
                ITraceWriter traceWriter,
                string sectionName,
                Dictionary<string, string> parameters,
                Dictionary<string, SecureString> encryptedParameters);
        
        // Informs the caller whether or not the plugin is enabled.
        bool Enabled {get;}
    }

    // Interface representing the validator of a DCA plugin's Azure-related settings in the cluster manifest
    public interface IDcaAzureSettingsValidator : IDcaSettingsValidator
    {
        // Returns the connection string that the plugin uses with the Azure Diagnostic Manager.
        // If the plugin does not use the Azure Diagnostic Manager, it returns null.
        SecureString ConnectionStringUsedWithAzureDiagnosticManager {get;}

        // Returns the transfer interval that the plugin uses for performance counter transfers with the
        // Azure Diagnostic Manager. If the plugin does not use performance counter transfers with the 
        // Azure Diagnostic Manager, it returns 0.
        int PerformanceCounterTransferIntervalUsedWithAzureDiagnosticManager { get; }

        // Returns information about Azure blob containers that need to be validated
        IEnumerable<AzureBlobContainerInfo> BlobContainerInfoList {get;}

        // Returns information about Azure tables that need to be validated
        IEnumerable<AzureTableInfo> TableInfoList {get;}
    }

    public struct AzureBlobContainerInfo
    {
        // Connection string for storage account
        public SecureString ConnectionString;

        // Name of the container to validate
        public string ContainerName;
    }

    // Represents a table in Azure table storage that the plugin needs to validate
    public struct AzureTableInfo
    {
        // Connection string for storage account
        public SecureString ConnectionString;

        // Name of the table to validate
        public string TableName;
    }
}