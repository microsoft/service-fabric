// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Test
{
    using System;
    using System.Runtime.InteropServices;
    using System.Security;

    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Provides unit tests for ConnectionStringParser class.
    /// </summary>
    [TestClass]
    public class ConnectionStringParserTest
    {
        private const string Prefix = "xstore:";
        private const string AccountName = "testAccountName";
        private const string AccountKey = "abcd";
        private const string DevelopmentStorageAccountName = "Azure Storage Emulator";

        private const string BlobEndpoint = "https://test20160124231703.blob.core.windows.net/";
        private const string QueueEndpoint = "https://test20160124231703.queue.core.windows.net/";
        private const string TableEndpoint = "https://test20160124231703.table.core.windows.net/";
        private const string FileEndpoint = "https://test20160124231703.file.core.windows.net/";

        private const string DsmsSourceLocation = @"https://uswest-dsms.dsms.core.windows.net/dsms/winfab/dca/test/storageaccounts/unmeshkdcastorage";
        private const string DsmsEndpointSuffix = @"core.windows.net";
        private const string DsmsAutopilotServiceName = @"AutopilotService";

        /// <summary>
        /// Tests simple account name and key connection string.
        /// </summary>
        [TestMethod]
        public void ParseAccountNameAccountKeyTest()
        {
            var testString = string.Format("{0}AccountName={1};AccountKey={2}", Prefix, AccountName, AccountKey);
            var connection = ValidateParse(testString, AccountName);
            Assert.AreEqual(AccountKey, ConvertToUnsecureString(connection.AccountKey));
        }

        /// <summary>
        /// Tests that the connection string doesn't have to start with prefix.
        /// </summary>
        [TestMethod]
        public void ParseWithoutPrefixTest()
        {
            var testString = string.Format("AccountName={0};AccountKey={1}", AccountName, AccountKey);
            var connection = ValidateParse(testString, AccountName);
            Assert.AreEqual(AccountKey, ConvertToUnsecureString(connection.AccountKey));
        }

        [TestMethod]
        public void ParseDevelopmentStorageWithPrefixTest()
        {
            var testString = string.Format("{0}UseDevelopmentStorage=true", Prefix);
            var cs = ValidateParse(testString, DevelopmentStorageAccountName);
            Assert.IsTrue(cs.UseDevelopmentStorage);
        }

        [TestMethod]
        public void ParseDevelopmentStorageTest()
        {
            var testString = "UseDevelopmentStorage=true";
            var cs = ValidateParse(testString, DevelopmentStorageAccountName);
            Assert.IsTrue(cs.UseDevelopmentStorage);
        }

        /// <summary>
        /// Tests that the dsms connection string is parsed as expected.
        /// </summary>
        [TestMethod]
        public void ParseDsmsConnectionStringTest()
        {
            // Test cases with expected results in parsing
            var connectionStrings = new[] {
                // Testing parsing success case with SourceLocation before EndpointSuffix
                new {
                    ConnectionString = $"dsms:SourceLocation={DsmsSourceLocation};EndpointSuffix={DsmsEndpointSuffix}",
                    StorageConnection = new StorageConnection(DsmsSourceLocation, DsmsEndpointSuffix, null),
                    Valid = true
                },
                // Testing parsing success case with EndpointSuffix before SourceLocation
                new {
                    ConnectionString = $"dsms:EndpointSuffix={DsmsEndpointSuffix};SourceLocation={DsmsSourceLocation}",
                    StorageConnection = new StorageConnection(DsmsSourceLocation, DsmsEndpointSuffix, null),
                    Valid = true
                },
                // Testing parsing success case with AutopilotServiceName after the required parameters
                new {
                    ConnectionString = $"dsms:EndpointSuffix={DsmsEndpointSuffix};SourceLocation={DsmsSourceLocation};AutopilotServiceName={DsmsAutopilotServiceName}",
                    StorageConnection = new StorageConnection(DsmsSourceLocation, DsmsEndpointSuffix, DsmsAutopilotServiceName),
                    Valid = true
                },
                // Testing parsing success case with AutopilotServiceName before the required parameters
                new {
                    ConnectionString = $"dsms:AutopilotServiceName={DsmsAutopilotServiceName};EndpointSuffix={DsmsEndpointSuffix};SourceLocation={DsmsSourceLocation}",
                    StorageConnection = new StorageConnection(DsmsSourceLocation, DsmsEndpointSuffix, DsmsAutopilotServiceName),
                    Valid = true
                },
                // Testing parsing failure case missing EndpointSuffix
                new {
                    ConnectionString = $"dsms:SourceLocation={DsmsSourceLocation}",
                    StorageConnection = new StorageConnection(),
                    Valid = false
                },
                // Testing parsing failure case missing SourceLocation
                new {
                    ConnectionString = $"dsms:EndpointSuffix={DsmsEndpointSuffix}",
                    StorageConnection = new StorageConnection(),
                    Valid = false
                },
                // Testing parsing failure case missing both EndpointSuffix and SourceLocation
                new {
                    ConnectionString = $"dsms:",
                    StorageConnection = new StorageConnection(),
                    Valid = false
                },
                // Testing parsing failure case repeated SourceLocation
                new {
                    ConnectionString = $"dsms:SourceLocation={DsmsSourceLocation};SourceLocation={DsmsSourceLocation}",
                    StorageConnection = new StorageConnection(),
                    Valid = false
                },
                // Testing parsing failure case repeated EndpointSuffix
                new {
                    ConnectionString = $"dsms:EndpointSuffix={DsmsEndpointSuffix};EndpointSuffix={DsmsEndpointSuffix}",
                    StorageConnection = new StorageConnection(),
                    Valid = false
                },
                // Testing parsing failure case where SourceLocation is empty
                new {
                    ConnectionString = $"dsms:SourceLocation=;EndpointSuffix={DsmsEndpointSuffix}",
                    StorageConnection = new StorageConnection(),
                    Valid = false
                },
                // Testing parsing failure case where EndpointSuffix is empty
                new {
                    ConnectionString = $"dsms:SourceLocation={DsmsSourceLocation};EndpointSuffix=",
                    StorageConnection = new StorageConnection(),
                    Valid = false
                },
                // Testing parsing failure case where both SourceLocation and EndpointSuffix are empty
                new {
                    ConnectionString = $"dsms:SourceLocation=;EndpointSuffix=",
                    StorageConnection = new StorageConnection(),
                    Valid = false
                },
                // Testing parsing failure case AutopilotServiceName is empty
                new {
                    ConnectionString = $"dsms:SourceLocation={DsmsSourceLocation};EndpointSuffix={DsmsEndpointSuffix};AutopilotServiceName=",
                    StorageConnection = new StorageConnection(),
                    Valid = false
                }
            };

            bool testSuccess = true;
            string parseErrorMsg = string.Empty;

            // go over each test case and append error message in case test fails
            foreach (var connection in connectionStrings)
            {
                try
                {
                    StorageConnection storageConnection = new StorageConnection();

                    Action<string> errorLogger = (connection.Valid) ? new Action<string>(error => { Console.WriteLine(error); }) : new Action<string>(error => { });
                    bool parseRes = ConnectionStringParser.ParseConnectionString(
                        connection.ConnectionString.ToCharArray(),
                        errorLogger,
                        ref storageConnection);

                    if (
                        parseRes != connection.Valid ||
                        storageConnection.IsDsms != connection.Valid ||
                        storageConnection.DsmsEndpointSuffix != connection.StorageConnection.DsmsEndpointSuffix ||
                        storageConnection.DsmsSourceLocation != connection.StorageConnection.DsmsSourceLocation ||
                        storageConnection.DsmsAutopilotServiceName != connection.StorageConnection.DsmsAutopilotServiceName)
                    {
                        parseErrorMsg += $"TestCase with ConnectionString=\"{connection.ConnectionString}\" failed to parse.\n" +
                            $"Parsing Result = {parseRes}\n" +
                            $"Expected:\n\tParseResult = {connection.Valid}\n\tIsDsms = { connection.StorageConnection.IsDsms},\n\tSourceLocation = { connection.StorageConnection.DsmsSourceLocation}\n\tEndpointSuffix = { connection.StorageConnection.DsmsEndpointSuffix}\n\tAutopilotServiceName = {connection.StorageConnection.DsmsAutopilotServiceName}\n" +
                            $"Read    :\n\tParseResult = {parseRes        }\n\tIsDsms = { storageConnection.IsDsms           },\n\tSourceLocation = { storageConnection.DsmsSourceLocation           }\n\tEndpointSuffix = { storageConnection.DsmsEndpointSuffix           }\n\tAutopilotServiceName = {storageConnection.DsmsAutopilotServiceName}\n";

                        testSuccess = false;
                    }
                }
                catch (Exception e)
                {
                    // test failed
                    parseErrorMsg += $"TestCase with ConnectionString=\"{connection.ConnectionString}\" failed with Exception: {e}";
                    testSuccess = false;
                }
            }

            Assert.IsTrue(testSuccess, parseErrorMsg);
        }

        /// <summary>
        /// Tests custom endpoints connection string.
        /// </summary>
        [TestMethod]
        public void ParseCustomEndpointsTest()
        {
            var testString = string.Format(
                "{0}BlobEndpoint={1};QueueEndpoint={2};TableEndpoint={3};FileEndpoint={4};AccountName={5};AccountKey={6}", 
                Prefix, 
                BlobEndpoint,
                QueueEndpoint,
                TableEndpoint,
                FileEndpoint,
                AccountName,
                AccountKey);
            var connection = ValidateParse(testString, AccountName);

            Assert.IsTrue(ConvertToUnsecureString(connection.ConnectionString).StartsWith("BlobEndpoint"));
            Assert.AreEqual(AccountKey, ConvertToUnsecureString(connection.AccountKey));
            Assert.AreEqual(BlobEndpoint, connection.BlobEndpoint.ToString());
            Assert.AreEqual(QueueEndpoint, connection.QueueEndpoint.ToString());
            Assert.AreEqual(TableEndpoint, connection.TableEndpoint.ToString());
            Assert.AreEqual(FileEndpoint, connection.FileEndpoint.ToString());
        }

        private static StorageConnection ValidateParse(string testString, string expectedAccountName)
        {
            var parseErrorMessage = string.Empty;
            var connection = new StorageConnection();
            var result = ConnectionStringParser.ParseConnectionString(testString.ToCharArray(), (error) => { parseErrorMessage = error; }, ref connection);

            Assert.IsTrue(result, parseErrorMessage);
            Assert.IsNotNull(connection);
            Assert.AreEqual(expectedAccountName, connection.AccountName);
            return connection;
        }

        private static string ConvertToUnsecureString(SecureString securePassword)
        {
            IntPtr unmanagedString = IntPtr.Zero;
            try
            {
                unmanagedString = Marshal.SecureStringToGlobalAllocUnicode(securePassword);
                return Marshal.PtrToStringUni(unmanagedString);
            }
            finally
            {
                Marshal.ZeroFreeGlobalAllocUnicode(unmanagedString);
            }
        }
    }
}