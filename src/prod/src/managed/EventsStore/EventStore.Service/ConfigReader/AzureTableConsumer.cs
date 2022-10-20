// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ConfigReader
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text;
    using ClusterAnalysis.Common.Util;
    using Microsoft.Win32;
    using Exceptions;

    // Code taken from DCA and Cluster Analysis service.
    internal class AzureTableConsumer
    {
        private const string TenantIdRegistryKey = "WATenantID";
        private const string RoleInstanceIdRegistryKey = "WARoleInstanceID";
        private const string RoleNameRegistryKey = "WARoleName";
        private const string BlobEndpoint = "BlobEndpoint=";
        private const string QueueEndpoint = "QueueEndpoint=";
        private const string TableEndpoint = "TableEndpoint=";
        private const string FileEndpoint = "FileEndpoint=";
        private const string IsEncrypted = "IsEncrypted";
        private const string XStoreConnectionStringIdentifier = @"xstore:";
        private const string DefaultEndpointsProtocol = "DefaultEndpointsProtocol=";
        private const string AccountName = "AccountName=";
        private const string AccountKey = "AccountKey=";
        private const string ProtectedAccountKeyName = "ProtectedAccountKeyName=";
        private static readonly string KeyName = Path.Combine(Registry.LocalMachine.Name, System.Fabric.FabricConstants.FabricRegistryKeyPath);

        private static Dictionary<TokenType, string> TokenPrefixes = new Dictionary<TokenType, string>
        {
            {TokenType.DefaultEndpointsProtocol, DefaultEndpointsProtocol},
            {TokenType.AccountName, AccountName},
            {TokenType.AccountKey, AccountKey},
            {TokenType.ProtectedAccountKeyName, ProtectedAccountKeyName},
            {TokenType.BlobEndpoint, BlobEndpoint},
            {TokenType.QueueEndpoint, QueueEndpoint},
            {TokenType.TableEndpoint, TableEndpoint},
            {TokenType.FileEndpoint, FileEndpoint}
        };

        private enum TokenType
        {
            DefaultEndpointsProtocol,
            AccountName,
            AccountKey,
            ProtectedAccountKeyName,
            BlobEndpoint,
            QueueEndpoint,
            TableEndpoint,
            FileEndpoint,

            // This is not a token type. It is a value representing the count
            // of token types.
            Count
        }

        public AzureTableConsumer(AzureConsumerType consumerType, string tablePrefix, string connectionString, string deploymentId)
        {
            Assert.IsNotEmptyOrNull(deploymentId, "deploymentId != null");
            this.Init(consumerType, tablePrefix, connectionString, deploymentId);
        }

        public AzureTableConsumer(AzureConsumerType consumerType, string tablePrefix, string connectionString)
        {
            this.Init(consumerType, tablePrefix, connectionString, null);
        }

        private void Init(AzureConsumerType consumerType, string tablePrefix, string connectionString, string deploymentId)
        {
            Assert.IsNotNull(tablePrefix, "tablePrefix != null");
            Assert.IsNotNull(connectionString, "connectionString != null");

            this.Type = consumerType;
            this.TablesPrefix = tablePrefix;
            this.Connection = new StorageConnection();
            this.ParseConnectionString(connectionString.ToCharArray());
            // TODO: We need to improve this Logic.
            if (deploymentId == null)
            {
                this.DeploymentId = this.GetAzureTableDeploymentId();
            }
        }

        public AzureConsumerType Type { private set; get; }

        public string TablesPrefix { private set; get; }

        public StorageConnection Connection { private set; get; }

        public string DeploymentId { private set; get; }

        private void ParseConnectionString(char[] connectionCharArray)
        {
            int charIndex = 0;

            // Remove optional xstore: prefix
            if (!XStoreConnectionStringIdentifier.ToCharArray().All(c => charIndex < connectionCharArray.Length && c == connectionCharArray[charIndex++]))
            {
                charIndex = 0;
            }

            // Save the connection string minus the xstore: prefix
            this.Connection.ConnectionString = HandyUtil.SecureStringFromCharArray(connectionCharArray, charIndex, connectionCharArray.Length - 1);

            // Parse the connection string to identify its tokens
            var tokenInfo = new Dictionary<TokenType, string>();
            for (int j = 0; (j < (int)TokenType.Count) && (charIndex < connectionCharArray.Length); j++)
            {
                TokenType tokenType;
                GetTokenType(connectionCharArray, charIndex, out charIndex, out tokenType);

                // If both an account key and a protected account key are specified and both of them are valid,
                // We would be using the account key that is defined in a token closer to the end of the connection string and the other one would be ignored.
                if (TokenType.AccountKey == tokenType)
                {
                    // Account key is handled in a special way. It is stored as a secure string
                    // instead of a regular string. Note that it is also not converted to a
                    // string even as an intermediate step,  because strings are immutable in
                    // .NET and there is no way to control how long the string object lives.
                    if ((charIndex >= connectionCharArray.Length) || (connectionCharArray[charIndex] == ';'))
                    {
                        throw new ConnectionParsingException(ErrorCodes.AzureAccountKeyMissing, "'AccountKey' is an empty string.");
                    }

                    int keyStart = charIndex;
                    for (; charIndex < connectionCharArray.Length && connectionCharArray[charIndex] != ';'; charIndex++)
                    {
                    }

                    int afterKeyEnd = charIndex;
                    charIndex++;

                    // Make sure the account key has only base-64 characters
                    try
                    {
                        byte[] byteArray = Convert.FromBase64CharArray(connectionCharArray, keyStart, afterKeyEnd - keyStart);

                        Array.Clear(byteArray, 0, byteArray.Length);
                    }
                    catch (FormatException)
                    {
                        throw new ConnectionParsingException(
                            ErrorCodes.AzureAccountKeyInvalid,
                            "The 'AccountKey' not a valid Base-64 string as it contains a non-base 64 character, more than two padding characters, or a non-white space character among the padding characters.");
                    }

                    this.Connection.AccountKey = HandyUtil.SecureStringFromCharArray(connectionCharArray, keyStart, afterKeyEnd - 1);
                }
                else if (TokenType.ProtectedAccountKeyName == tokenType)
                {
                    // Protected account key name is handled in a special way.
                    // Instead of a direct reference to the account key in connection string,
                    // it is a reference to a location on the machine where encrypted account key can be found.
                    // DCA is able to decrypt it to get the account key, which would be stored as a secure string directly.
                    // Note that this is required for Service Fabric RP managed clusters due to limitations that prevent us from using secret certs.
                    if ((charIndex >= connectionCharArray.Length) || (connectionCharArray[charIndex] == ';'))
                    {
                        throw new ConnectionParsingException(ErrorCodes.AzureAccountKeyMissing, "'ProtectedAccountKeyName' is an empty string.");
                    }

                    int keyStart = charIndex;
                    for (; charIndex < connectionCharArray.Length && connectionCharArray[charIndex] != ';'; charIndex++)
                    {
                    }

                    int afterKeyEnd = charIndex;
                    charIndex++;

                    char[] protectedAccountKeyNameCharArray = new char[afterKeyEnd - keyStart];
                    Array.Copy(connectionCharArray, keyStart, protectedAccountKeyNameCharArray, 0, afterKeyEnd - keyStart);
                    string protectedAccountKeyName = new string(protectedAccountKeyNameCharArray);

                    char[] accountKeyCharArray;
                    string decryptionErrorMessage;
                    if (!ProtectedAccountKeyHelper.TryDecryptProtectedAccountKey(protectedAccountKeyName, out accountKeyCharArray, out decryptionErrorMessage))
                    {
                        throw new ConnectionParsingException(ErrorCodes.AzureAccountKeyDecryptionFailed, decryptionErrorMessage);
                    }

                    this.Connection.AccountKey = HandyUtil.SecureStringFromCharArray(accountKeyCharArray, 0, accountKeyCharArray.Length - 1);

                    Array.Clear(accountKeyCharArray, 0, accountKeyCharArray.Length);
                }
                else
                {
                    StringBuilder tokenInfoBuilder = new StringBuilder();
                    while (charIndex < connectionCharArray.Length && connectionCharArray[charIndex] != ';')
                    {
                        tokenInfoBuilder.Append(connectionCharArray[charIndex++]);
                    }

                    charIndex++;

                    tokenInfo[tokenType] = tokenInfoBuilder.ToString();
                }
            }

            // Get AccountName
            if (!tokenInfo.ContainsKey(TokenType.AccountName))
            {
                throw new ConnectionParsingException(ErrorCodes.AzureAccountNameMissing, "The Connection string does not contain 'AccountName'.");
            }

            this.Connection.AccountName = tokenInfo[TokenType.AccountName];

            // Get AccountKey
            if (this.Connection.AccountKey == null)
            {
                throw new ConnectionParsingException(ErrorCodes.AzureAccountKeyMissing, "The Connection string does not contain 'AccountKey'.");
            }
        }

        private static void GetTokenType(char[] connectionCharArray, int startPos, out int endPos, out TokenType type)
        {
            endPos = startPos;
            type = TokenType.Count;
            for (int j = 0; j < (int)TokenType.Count; j++)
            {
                int i = startPos;
                if (TokenPrefixes[(TokenType)j].ToCharArray().All(c => i < connectionCharArray.Length && c == connectionCharArray[i++]))
                {
                    endPos = i;
                    type = (TokenType)j;
                    return;
                }
            }

            throw new ConnectionParsingException(ErrorCodes.AzureAccountKeyDecryptionFailed, "Unrecognized token in connection string.");
        }

        public string GetAzureTableDeploymentId()
        {
            if (IsAzureInterfaceAvailable())
            {
                return (string)Registry.GetValue(KeyName, TenantIdRegistryKey, null);
            }

            return string.Empty;
        }

        private bool IsAzureInterfaceAvailable()
        {
            var deploymentIdExists = !string.IsNullOrEmpty((string)Registry.GetValue(KeyName, TenantIdRegistryKey, null));
            var roleExists = !string.IsNullOrEmpty((string)Registry.GetValue(KeyName, RoleNameRegistryKey, null)) &&
                             !string.IsNullOrEmpty((string)Registry.GetValue(KeyName, RoleInstanceIdRegistryKey, null));

            return deploymentIdExists && roleExists;
        }
    }
}