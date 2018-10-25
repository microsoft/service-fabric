// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;
    using System.Text.RegularExpressions;
    using System.Security;
    using System.Text;
    using System.Threading.Tasks;

    internal static class ConnectionStringParser
    {
        private const string XStoreConnectionStringIdentifier = @"xstore:";
        private const string DefaultEndpointsProtocol = "DefaultEndpointsProtocol=";
        private const string AccountName = "AccountName=";
        private const string AccountKey = "AccountKey=";
        private const string ProtectedAccountKeyName = "ProtectedAccountKeyName=";
        private const string BlobEndpoint = "BlobEndpoint=";
        private const string QueueEndpoint = "QueueEndpoint=";
        private const string TableEndpoint = "TableEndpoint=";
        private const string FileEndpoint = "FileEndpoint=";
        private const string DsmsConnectionStringIdentifier = @"dsms:";
        private const string DsmsSourceLocation = "SourceLocation=";
        private const string DsmsEndpointSuffix = "EndpointSuffix=";
        private const string DsmsAutopilotServiceName = "AutopilotServiceName=";
        private const string Http = "Http";
        private const string Https = "Https";

        private const string DevelopmentStorageAccountName = "Azure Storage Emulator";

        // Dictionary of token prefixes
        private static Dictionary<TokenType, string> TokenPrefixes = new Dictionary<TokenType, string>()
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

        // Dictionary of Dsms connection string tokens
        private static Dictionary<DsmsTokenType, string> DsmsTokenPrefixes = new Dictionary<DsmsTokenType, string>
        {
            {DsmsTokenType.SourceLocation, DsmsSourceLocation},
            {DsmsTokenType.EndpointSuffix, DsmsEndpointSuffix},
            {DsmsTokenType.AutopilotServiceName, DsmsAutopilotServiceName}
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

        // tokens used for parsing Dsms Connection string
        private enum DsmsTokenType
        {
            SourceLocation,
            EndpointSuffix,

            // AutopilotServiceName is an optional parameter
            AutopilotServiceName,


            // This is not a token type. It is a value representing the count
            // of token types.
            Count,

            // Empty should be after Count.
            // This prevents GetDsmsTokenType to try to parse it from
            // the connection string
            Empty
        }

        internal static bool ParseConnectionString(char[] connectionCharArray, Action<string> onError, ref StorageConnection storageConnection)
        {
            int charIndex = 0;
            bool isDsms = DsmsConnectionStringIdentifier.ToCharArray().All(c => charIndex < connectionCharArray.Length && c == connectionCharArray[charIndex++]);

            if (isDsms)
            {
                // Dsms
                return ConnectionStringParser.ParseDsmsConnectionString(connectionCharArray, onError, ref storageConnection);
            }
            else
            {
                // xstore
                return ConnectionStringParser.ParseXStoreConnectionString(connectionCharArray, onError, ref storageConnection);
            }
        }

        // The expected format of connectionString is "dsms:SourceLocation=blahblabla;EndpointSuffix=blahblah;AutoPilotServiceName=bla"
        // Where dsms: SourceLocation= and EndpointSuffix= are required and case sensitive, and AutoPilotServiceName is optional
        private static bool ParseDsmsConnectionString(char[] connectionCharArray, Action<string> onError, ref StorageConnection storageConnection)
        {
            int charIndex = 0;

            // consuming dsms: from connectionstring
            DsmsConnectionStringIdentifier.ToCharArray().All(c => charIndex < connectionCharArray.Length && c == connectionCharArray[charIndex++]);

            // Parse the connection string to identify its tokens
            Dictionary<DsmsTokenType, string> tokenInfo = new Dictionary<DsmsTokenType, string>();
            for (int j = 0; ((j < (int)DsmsTokenType.Count) && (charIndex < connectionCharArray.Length)); j++)
            {
                // Look for SourceLocation, EndpointPrefix or AutopilotServiceName
                DsmsTokenType tokenType;
                try
                {
                    GetDsmsTokenType(connectionCharArray, charIndex, out charIndex, out tokenType);

                    // reached the end of the connection string
                    // this is needed for optional parameters
                    if (tokenType == DsmsTokenType.Empty)
                        break;
                }
                catch (ArgumentException ae)
                {
                    onError(ae.Message);
                    return false;
                }

                // Reading content of specified prefix
                StringBuilder tokenInfoBuilder = new StringBuilder();
                while (charIndex < connectionCharArray.Length && connectionCharArray[charIndex] != ';')
                {
                    tokenInfoBuilder.Append(connectionCharArray[charIndex++]);
                }

                // consuming ';'
                charIndex++;

                // Dsms expects one and only one of each prefix (SourceLocation, EndpointSuffix)
                try
                {
                    tokenInfo.Add(tokenType, tokenInfoBuilder.ToString());
                }
                catch (Exception e)
                {
                    onError($"Invalid Dsms connectionString. Exception {e}");
                    return false;
                }
            }

            try
            {
                string sourceLocation = tokenInfo.ContainsKey(DsmsTokenType.SourceLocation)? tokenInfo[DsmsTokenType.SourceLocation] : null;
                string endpointSuffix = tokenInfo.ContainsKey(DsmsTokenType.EndpointSuffix)? tokenInfo[DsmsTokenType.EndpointSuffix] : null;
                string autoPilotServiceName = tokenInfo.ContainsKey(DsmsTokenType.AutopilotServiceName) ? tokenInfo[DsmsTokenType.AutopilotServiceName] : null;

                storageConnection = new StorageConnection(
                    sourceLocation,
                    endpointSuffix,
                    autoPilotServiceName);
            }
            catch (Exception e)
            {
                onError($"Invalid Dsms connectionString. Empty parameter. Exception {e}");
                return false;
            }

            return true;
        }

        private static bool ParseXStoreConnectionString(char[] connectionCharArray, Action<string> onError, ref StorageConnection storageConnection)
        {
            StorageConnection connection = new StorageConnection();

            int charIndex = 0;

            // Remove optional xstore: prefix
            if (!XStoreConnectionStringIdentifier.ToCharArray().All(c => charIndex < connectionCharArray.Length && c == connectionCharArray[charIndex++]))
            {
                charIndex = 0;
            }

            char[] devStoreConnectionCharArray = AzureConstants.DevelopmentStorageConnectionString.ToCharArray();
            if (connectionCharArray.Skip(charIndex).SequenceEqual(devStoreConnectionCharArray))
            {
                connection.AccountName = DevelopmentStorageAccountName;
                connection.UseDevelopmentStorage = true;
                connection.ConnectionString = SecureStringFromCharArray(
                                                devStoreConnectionCharArray,
                                                charIndex,
                                                devStoreConnectionCharArray.Length - 1);
                storageConnection = connection;
                return true;
            }

            // Save the connection string minus the xstore: prefix
            connection.ConnectionString = SecureStringFromCharArray(
                                            connectionCharArray,
                                            charIndex,
                                            connectionCharArray.Length - 1);

            // Parse the connection string to identify its tokens
            Dictionary<TokenType, string> tokenInfo = new Dictionary<TokenType, string>();
            for (int j = 0; ((j < (int)TokenType.Count) && (charIndex < connectionCharArray.Length)); j++)
            {
                TokenType tokenType;
                try
                {
                    GetTokenType(connectionCharArray, charIndex, out charIndex, out tokenType);
                }
                catch (ArgumentException ae)
                {

                    onError(ae.Message);
                    return false;
                }

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
                        onError("'AccountKey' is an empty string.");
                        return false;
                    }

                    int keyStart = charIndex;
                    for (; charIndex < connectionCharArray.Length && connectionCharArray[charIndex] != ';'; charIndex++) ;
                    int afterKeyEnd = charIndex;
                    charIndex++;

                    // Make sure the account key has only base-64 characters
                    try
                    {
                        byte[] byteArray = Convert.FromBase64CharArray(
                                                connectionCharArray,
                                                keyStart,
                                                (afterKeyEnd - keyStart));

                        Array.Clear(byteArray, 0, byteArray.Length);
                    }
                    catch (FormatException)
                    {
                        onError("The 'AccountKey' not a valid Base-64 string as it contains a non-base 64 character, more than two padding characters, or a non-white space character among the padding characters.");
                        return false;
                    }

                    connection.AccountKey = SecureStringFromCharArray(
                                                connectionCharArray,
                                                keyStart,
                                                afterKeyEnd - 1);
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
                        onError("'ProtectedAccountKeyName' is an empty string.");
                        return false;
                    }

                    int keyStart = charIndex;
                    for (; charIndex < connectionCharArray.Length && connectionCharArray[charIndex] != ';'; charIndex++) ;
                    int afterKeyEnd = charIndex;
                    charIndex++;

                    char[] protectedAccountKeyNameCharArray = new char[afterKeyEnd - keyStart];
                    Array.Copy(connectionCharArray, keyStart, protectedAccountKeyNameCharArray, 0, afterKeyEnd - keyStart);
                    string protectedAccountKeyName = new string(protectedAccountKeyNameCharArray);

                    char[] accountKeyCharArray;
                    string decryptionErrorMessage;
                    if (!ProtectedAccountKeyHelper.TryDecryptProtectedAccountKey(protectedAccountKeyName, out accountKeyCharArray, out decryptionErrorMessage))
                    {
                        onError(decryptionErrorMessage);

                        return false;
                    }

                    connection.AccountKey = SecureStringFromCharArray(
                                                accountKeyCharArray,
                                                0,
                                                accountKeyCharArray.Length - 1);

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

            // Get DefaultEndpointsProtocol
            if (tokenInfo.ContainsKey(TokenType.DefaultEndpointsProtocol))
            {
                string protocol = tokenInfo[TokenType.DefaultEndpointsProtocol];
                if (!protocol.Equals(Http, StringComparison.OrdinalIgnoreCase) && !protocol.Equals(Https, StringComparison.OrdinalIgnoreCase))
                {
                    onError(
                        string.Format(
                            "The DefaultEndpointsProtocol specificed in the connection string is invalid. Valid values are {0} and {1}",
                            Http,
                            Https));
                    return false;
                }
                connection.IsHttps = protocol.Equals(Https, StringComparison.OrdinalIgnoreCase);
            }
            else
            {
                connection.IsHttps = true;
            }

            // Get AccountName
            if (!tokenInfo.ContainsKey(TokenType.AccountName))
            {
                onError("The Connection string does not contain 'AccountName'.");
                return false;
            }
            connection.AccountName = tokenInfo[TokenType.AccountName];

            // Get AccountKey
            if (null == connection.AccountKey)
            {
                onError("The Connection string does not contain 'AccountKey'.");
                return false;
            }

            string endpointTokenName = string.Empty;
            try
            {
                // Get endpoints
                foreach (var tokenType in new[] { TokenType.BlobEndpoint, TokenType.QueueEndpoint, TokenType.TableEndpoint, TokenType.FileEndpoint })
                {
                    if (tokenInfo.ContainsKey(tokenType))
                    {
                        endpointTokenName = TokenPrefixes[tokenType];
                        if (tokenType == TokenType.BlobEndpoint)
                        {
                            connection.BlobEndpoint = new Uri(tokenInfo[tokenType]);
                        }
                        else if(tokenType == TokenType.QueueEndpoint)
                        {
                            connection.QueueEndpoint = new Uri(tokenInfo[tokenType]);
                        }
                        else if (tokenType == TokenType.TableEndpoint)
                        {
                            connection.TableEndpoint = new Uri(tokenInfo[tokenType]);
                        }
                        else if (tokenType == TokenType.FileEndpoint)
                        {
                            connection.FileEndpoint = new Uri(tokenInfo[tokenType]);
                        }
                    }
                }

                storageConnection = connection;
            }
            catch (UriFormatException e)
            {
                onError(
                    string.Format(
                        "The value specified for endpoint '{0}' in connection string is not in the correct format. Exception: {1}",
                        endpointTokenName,
                        e));
                return false;
            }

            return true;
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

            throw new ArgumentException("Unrecognized token in connection string.");
        }

        private static void GetDsmsTokenType(char[] connectionCharArray, int startPos, out int endPos, out DsmsTokenType type)
        {
            endPos = startPos;
            type = DsmsTokenType.Empty;

            // DsmsTokeType.Empty in case it is the end of the string
            // this allows having optional parameters in the connection string
            // this assumes the connection string has no white space at the end
            if (startPos >= connectionCharArray.Length)
                return;

            for (int j = 0; j < (int)DsmsTokenType.Count; j++)
            {
                int i = startPos;
                if (DsmsTokenPrefixes[(DsmsTokenType)j].ToCharArray().All(c => i < connectionCharArray.Length && c == connectionCharArray[i++]))
                {
                    endPos = i;
                    type = (DsmsTokenType)j;
                    return;
                }
            }

            throw new ArgumentException("Unrecognized token in Dsms connection string.");
        }

        private static SecureString SecureStringFromCharArray(char[] charArray, int start, int end)
        {
            SecureString secureString = new SecureString();
            for (int i = start; i <= end; i++)
            {
                secureString.AppendChar(charArray[i]);
            }
            secureString.MakeReadOnly();
            return secureString;
        }
    }

}