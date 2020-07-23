// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageStore
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.RetryPolicies;
    using Microsoft.WindowsAzure.Storage;

    /// <summary>
    /// Common static lib functions/constants for xstore file operations
    /// </summary>
    internal static class XStoreCommon
    {
        /// <summary>Manifest file to indicate a folder is complete </summary>
        public const string ManifestFilename = "serviceplatform.xstore.manifest";

        /// <summary>Parallel operation number for a single blob transfer (bloblist) </summary>
        public const int ParallelOperationThreadCount = 50;

        /// <summary>Default connection string for azure storage </summary>
        public const string DefaultConnectionStringFormat = "DefaultEndpointsProtocol={0};AccountName={1};AccountKey={2}";

        /// <summary>Default blob storage endpoint for azure blob storage </summary>
        public const string DefaultBlobEndpointFormat = "{0}://{1}.blob.core.windows.net/";

        /// <summary>Default blob storage endpoint compoisted by suffix for azure blob storage </summary>
        public const string DefaultBlobEndpointFormatWitSuffix = "{0}://{1}.blob.{2}/";

        /// <summary>Gets the default retry policy used in blob request option </summary>
        public static readonly ExponentialRetry DefaultRetryPolicy = new ExponentialRetry(System.TimeSpan.FromSeconds(3), 3);

        /// <summary>String in image uri to indicate default endpoint protocal </summary>
        private const string DefaultEndpointsProtocolField = "DefaultEndpointsProtocol";

        /// <summary>String in image uri to indicate account name </summary>
        private const string AccountNameField = "AccountName";

        /// <summary>String in image uri to indicate account key </summary>
        private const string AccountKeyField = "AccountKey";

        /// <summary>String in image uri to indicate endpoint suffix </summary>
        private const string EndpointSuffix = "EndpointSuffix";

        /// <summary>String in image uri to indicate account key </summary>
        private const string SecondaryAccountKeyField = "SecondaryAccountKey";

        /// <summary>String in image uri to indicate blob endpoint </summary>
        private const string BlobEndpointField = "BlobEndpoint";

        /// <summary>String in image uri to indicate queue endpoint </summary>
        private const string QueueEndpointField = "QueueEndpoint";

        /// <summary>String in image uri to indicate table endpoint </summary>
        private const string TableEndpointField = "TableEndpoint";

        /// <summary>The file extesion for partial downloaded file </summary>
        private const string PartialDownloadExtension = ".xspart.";

        /// <summary>
        /// String in image uri or code/data uri to specifiy that this is a xstore path.
        /// </summary>
        private const string XstoreSchemaTag = @"xstore:";

        /// <summary>The string in the imagestore uri that representation of container </summary>
        private const string ImageStoreUriContainerFieldName = "Container";

        private static ConcurrentDictionary<Guid, List<Tuple<string, string>>> blobContentMap;

        /// <summary>
        /// Retrieve the list of blob content operated 
        /// </summary>
        /// <param name="operationId">A unique ID represents a download operation used to retrieve blob contents.</param>
        /// <returns></returns>
        public static List<Tuple<string, string>> GetDownloadContentEntries(Guid operationId)
        {
            if (blobContentMap != null && blobContentMap.ContainsKey(operationId))
            {
                List<Tuple<string, string>> contents;
                if (!blobContentMap.TryRemove(operationId, out contents))
                {
                    return contents;
                }
            }

            return null;
        }

        /// <summary>
        /// Add the blob content to be operated into the map
        /// </summary>
        /// <param name="operationId">A unique ID represents a download operation used to retrieve blob contents.</param>
        /// <param name="srcUri">The source location of downloading operation.</param>
        /// <param name="dstUri">The destination locaiton of downloading operation.</param>
        public static void AddDownloadContentEntry(Guid operationId, string srcUri, string dstUri)
        {
            if (blobContentMap == null)
            {
                blobContentMap = new ConcurrentDictionary<Guid, List<Tuple<string, string>>>();
            }

            if (!blobContentMap.ContainsKey(operationId))
            {
                blobContentMap.TryAdd(operationId, new List<Tuple<string, string>>());
            }
            
            blobContentMap[operationId].Add(new Tuple<string, string>(srcUri, dstUri));
        }

        /// <summary>
        /// See if a uri point to xstore image store
        /// </summary>
        /// <param name="imageStoreUri">The uri to check whether it is for xstore or not.</param>
        /// <returns>True if the uri is XStore uri, false otherwise.</returns>
        public static bool IsXStoreUri(string imageStoreUri)
        {
            return imageStoreUri.Trim().StartsWith(XstoreSchemaTag, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Generate the manifest file name based on the xstore folder uri
        /// </summary>
        /// <param name="xstoreFolderUri">The uri of the folder.</param>
        /// <returns>
        /// Bool to indicate if the directory exist
        /// </returns>
        public static string GetXStoreFolderManifest(string xstoreFolderUri)
        {
            return FormatXStoreUri(xstoreFolderUri) + "/" + ManifestFilename;
        }

        /// <summary>
        /// Format a SMB uri
        /// </summary>
        /// <param name="smbUri">Smb uri that to be formatted</param>
        /// <returns>Formatted smb uri</returns>
        internal static string FormatSMBUri(string smbUri)
        {
            return smbUri;
        }

        /// <summary>
        /// Format a blob uri: trim leading/tailing white spaces and '/'
        /// </summary>
        /// <param name="blobUri">Blob uri that to be formatted</param>
        /// <returns>Formatted blob uri</returns>
        public static string FormatXStoreUri(string blobUri)
        {
            // format: RootFolder/SubFolder/blobname.extension
            return blobUri.Trim(new[] { '/', ' ', '\\', '\t' }).Replace('\\', '/');
        }

        /// <summary>
        /// XStore uri can starts with the xstore schema tag. Remove the tag and
        /// format it to be the standard uri
        /// </summary>
        /// <param name="xstoreUri">The uri that contains xstore schema tag</param>
        /// <returns>The uri without the xstore schema tag.</returns>
        public static string RemoveXStoreSchemaTag(string xstoreUri)
        {
            if (!IsXStoreUri(xstoreUri))
            {
                return FormatXStoreUri(xstoreUri);
            }
            else
            {
                return FormatXStoreUri(xstoreUri.TrimStart().Substring(XstoreSchemaTag.Length));
            }
        }

        /// <summary>
        /// The ImageStore Uri looks like "xstore:{connection string};Container={2}"
        /// </summary>
        /// <param name="imageStoreUri">The image store uri to be parsed.</param>
        /// <param name="connectionString">The connection string in the uri.</param>
        /// <param name="secondaryConnectionString">The secondary connection string in the uri.</param>
        /// <param name="blobEndpoint">The blob endpoint in the uri.</param>
        /// <param name="container">The container in the uri.</param>
        /// <param name="accountName">The Azure storage account name.</param>
        /// <param name="endpointSuffix">The Azure storage environment property of the endpoint suffix </param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Globalization", "CA1308:NormalizeStringsToUppercase", Justification = "Automatic suppression")]
        public static void TryParseImageStoreUri(
            string imageStoreUri,
            out string connectionString,
            out string secondaryConnectionString,
            out string blobEndpoint,
            out string container,
            out string accountName,
            out string endpointSuffix)
        {
            imageStoreUri = imageStoreUri.Replace("\\;", ";");
            imageStoreUri = imageStoreUri.Replace("\\=", "=");

            if (!imageStoreUri.StartsWith(XstoreSchemaTag, StringComparison.OrdinalIgnoreCase))
            {
                throw new ArgumentException(StringResources.XStore_InvalidConnectionStringPrefix, imageStoreUri);
            }
            string untaggedImageStoreUri = imageStoreUri.TrimStart().Substring(XstoreSchemaTag.Length).Trim();

            // validation that no whitespace exist in imagestore uri; otherwise it would be a problem to use it as
            // a parameter to loaderservice/servicemanager

            if (untaggedImageStoreUri.IndexOf(" ", StringComparison.OrdinalIgnoreCase) >= 0)
            {
                throw new ArgumentException(StringResources.XStore_UnexpectedWhitespaceCharInConnectionString, imageStoreUri);
            }            

            if (untaggedImageStoreUri.IndexOf(AccountNameField, StringComparison.OrdinalIgnoreCase) >= 0 &&
                untaggedImageStoreUri.IndexOf(AccountKeyField, StringComparison.OrdinalIgnoreCase) >= 0 &&
                untaggedImageStoreUri.IndexOf(ImageStoreUriContainerFieldName, StringComparison.OrdinalIgnoreCase) >= 0 &&
                untaggedImageStoreUri.IndexOf(BlobEndpointField, StringComparison.OrdinalIgnoreCase) < 0 &&
                untaggedImageStoreUri.IndexOf(QueueEndpointField, StringComparison.OrdinalIgnoreCase) < 0 &&
                untaggedImageStoreUri.IndexOf(TableEndpointField, StringComparison.OrdinalIgnoreCase) < 0 &&
                untaggedImageStoreUri.IndexOf(EndpointSuffix, StringComparison.OrdinalIgnoreCase) < 0)
            {
                // this is the old image store uri format; which use default storage endpoints. including specify DefaultEndpointProtocal field
                TryParseUntaggedDefaultImageStoreUri(untaggedImageStoreUri, out connectionString, out secondaryConnectionString, out blobEndpoint, out container, out accountName);
                endpointSuffix = string.Empty;
            }
            else
            {
                // this is the format that specify a customed connection string
                // it must have blobendpoint in it
                if (untaggedImageStoreUri.IndexOf(BlobEndpointField, StringComparison.OrdinalIgnoreCase) < 0 
                    && untaggedImageStoreUri.IndexOf(EndpointSuffix, StringComparison.OrdinalIgnoreCase) < 0)
                {
                    throw new ArgumentException(StringResources.XStore_MissingEndpoint, imageStoreUri);
                }

                TryParseUntaggedCustomImageStoreUri(untaggedImageStoreUri, out connectionString, out secondaryConnectionString, out blobEndpoint, out container, out accountName, out endpointSuffix);
            }

            if (string.IsNullOrEmpty(connectionString))
            {
                throw new ArgumentException(StringResources.XStore_MissingConnectionString, imageStoreUri);
            }

            if (string.IsNullOrEmpty(container))
            {
                throw new ArgumentException(StringResources.XStore_MissingContainer, imageStoreUri);
            }

            if (string.IsNullOrEmpty(blobEndpoint))
            {
                throw new ArgumentException(StringResources.XStore_MissingEndpoint, imageStoreUri);
            }

            connectionString = connectionString.Trim();
            container = container.Trim();

            // format the blobendpoint so it end with "/"
            blobEndpoint = blobEndpoint.Trim(new char[] { ' ', '/' }) + "/";
        }

        /// <summary>
        /// Check to see if the specified blob folder exists
        /// </summary>
        /// <param name="blobContainer">Container item</param>
        /// <param name="blobUri">Blob uri of the blob folder</param>
        /// <param name="checkXStoreFolderManifest">Indicates whether to check XStore folder manifest.</param>
        /// <param name="requestOptions">Represents the Azure blob request option.</param>
        /// <returns>
        /// Returns true if exists
        /// </returns>
        public static bool XStoreFolderExists(CloudBlobContainer blobContainer, string blobUri, bool checkXStoreFolderManifest, BlobRequestOptions requestOptions)
        {
            bool folderExists = false;
            CloudBlobDirectory cloudBlobDirectory = blobContainer.GetDirectoryReference(blobUri);
#if !DotNetCoreClr
            IEnumerable < IListBlobItem > blobItems = cloudBlobDirectory.ListBlobs(false, BlobListingDetails.None, requestOptions);
#else
            BlobContinuationToken continuationToken = null;
            do
            {
                BlobResultSegment resultSegment = cloudBlobDirectory.ListBlobsSegmentedAsync(continuationToken).Result;
                IEnumerable<IListBlobItem> blobItems = resultSegment.Results;
                continuationToken = resultSegment.ContinuationToken;
#endif

                // Windows Azure storage has strong consistency guarantees.
                // As soon the manifest file is ready, all users are guaranteed
                // to see the complete folder
                if ((!checkXStoreFolderManifest || XStoreFileExists(blobContainer, GetXStoreFolderManifest(blobUri), requestOptions)) &&
                    (blobItems.Count() > 0))
                {
                    folderExists = true;
                }
#if DotNetCoreClr
            } while (!folderExists && (continuationToken != null));
#endif
            return folderExists;
        }

        /// <summary>
        /// Get the partial downloaded file name
        /// </summary>
        /// <param name="dstFilePath">The path of the download file.</param>
        /// <param name="partialID">The index of the file.</param>
        /// <returns>The partial file name</returns>
        internal static string GetPartialFileName(string dstFilePath, int partialID)
        {
            return dstFilePath + PartialDownloadExtension + partialID;
        }

        /// <summary>
        /// Check if the specified file exists on xstore
        /// </summary>
        /// <param name="blobContainer">Container item</param>
        /// <param name="blobUri">The Blob uri to be checked.</param>
        /// <param name="blobRequestOptions">>Represents the Azure blob request option.</param>
        /// <returns>
        /// Returns true if exists
        /// </returns>
        public static bool XStoreFileExists(CloudBlobContainer blobContainer, string blobUri, BlobRequestOptions blobRequestOptions)
        {
            var blob = blobContainer.GetBlockBlobReference(blobUri);
#if !DotNetCoreClr
            return blob.Exists(blobRequestOptions);
#else
            return blob.ExistsAsync(blobRequestOptions, null).Result;
#endif
        }

        /// <summary>
        /// Check if the specified container exists on xstore
        /// </summary>
        /// <param name="blobContainer">The name of the container.</param>
        /// <param name="blobRequestOptions">Blob request options.</param>
        /// <returns>
        /// Returns true if exists
        /// </returns>
        public static bool XStoreContainerExists(CloudBlobContainer blobContainer, BlobRequestOptions blobRequestOptions)
        {
#if !DotNetCoreClr
            return blobContainer.Exists(blobRequestOptions);
#else
            return blobContainer.ExistsAsync(blobRequestOptions, null).Result;
#endif
        }

        /// <summary>
        /// The untagged ImageStore Uri looks like "AccountName={0};AccountKey={1};Container={2}"
        /// </summary>
        /// <param name="untaggedImageStoreUri">The untagged image store uri.</param>
        /// <param name="connectionString">The connectino string parsed from the uri.</param>
        /// <param name="secondaryConnectionString">The secondary connection string parsed from the uri.</param>
        /// <param name="blobEndpoint">The blob endpoint in the uri.</param>
        /// <param name="container">The container in the uri.</param>
        /// <param name="accountName">The Azure storage account name.</param>
        private static void TryParseUntaggedDefaultImageStoreUri(
            string untaggedImageStoreUri,
            out string connectionString,
            out string secondaryConnectionString,
            out string blobEndpoint,
            out string container,
            out string accountName)
        {
            secondaryConnectionString = null;

            // before calling this function we have checked the format of untagged imagestore uri
            Dictionary<string, string> azureXStoreTable = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            string[] xstoreParam = untaggedImageStoreUri.Split(';');
            foreach (string param in xstoreParam)
            {
                string[] values = param.Split(new[] { '=' }, 2);
                if (values.Length == 2)
                {
                    azureXStoreTable.Add(values[0].Trim(), values[1].Trim());
                }
            }

            string defaultEndpointProtocal = "https";
            if (azureXStoreTable.ContainsKey(DefaultEndpointsProtocolField))
            {
                defaultEndpointProtocal = azureXStoreTable[DefaultEndpointsProtocolField];
            }

            accountName = azureXStoreTable[AccountNameField];
            string accountKey = azureXStoreTable[AccountKeyField];
            container = azureXStoreTable[ImageStoreUriContainerFieldName];
            connectionString = string.Format(CultureInfo.InvariantCulture, DefaultConnectionStringFormat, defaultEndpointProtocal, accountName, accountKey);
            blobEndpoint = string.Format(CultureInfo.InvariantCulture, DefaultBlobEndpointFormat, defaultEndpointProtocal, accountName);

            if (azureXStoreTable.ContainsKey(SecondaryAccountKeyField))
            {
                secondaryConnectionString = string.Format(CultureInfo.InvariantCulture, DefaultConnectionStringFormat, defaultEndpointProtocal, accountName, azureXStoreTable[SecondaryAccountKeyField]);
            }
        }

        /// <summary>
        /// The untagged ImageStore Uri looks like "{connection string};Container={0}"
        /// </summary>
        /// <param name="untaggedImageStoreUri">The untagged image store uri.</param>
        /// <param name="connectionString">The connection string.</param>
        /// <param name="secondaryConnectionString">The secondary connection string.</param>
        /// <param name="blobEndpoint">The blob endpoint.</param>
        /// <param name="container">The blob container.</param>
        /// <param name="accountName">The Azure storage account name.</param>
        /// <param name="endpointSuffix">The Azure storage environment property of the endpoint suffix.</param>
        private static void TryParseUntaggedCustomImageStoreUri(
            string untaggedImageStoreUri,
            out string connectionString,
            out string secondaryConnectionString,
            out string blobEndpoint,
            out string container,
            out string accountName,
            out string endpointSuffix)
        {
            secondaryConnectionString = null;

            // before calling this function we have checked the format of untagged imagestore uri
            Dictionary<string, string> azureXStoreTable = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            // validate that the container field is at the tail of imagestore uri
            // note that we are sure no space would be between "Container" and "="
            int pos = untaggedImageStoreUri.LastIndexOf(ImageStoreUriContainerFieldName + "=", StringComparison.OrdinalIgnoreCase);
            if (pos <= 0)
            {
                throw new ArgumentException(StringResources.XStore_MalformedConnectionString, untaggedImageStoreUri);
            }

            connectionString = untaggedImageStoreUri.Substring(0, pos).Trim(new char[] { ' ', ';' });

            // search for blob endpoint and container
            string[] xstoreParam = untaggedImageStoreUri.Split(';');
            foreach (string param in xstoreParam)
            {
                string[] values = param.Split(new[] { '=' }, 2);
                if (values.Length == 2)
                {
                    azureXStoreTable.Add(values[0].Trim(), values[1].Trim());
                }
            }

            accountName = azureXStoreTable[AccountNameField];
            container = azureXStoreTable[ImageStoreUriContainerFieldName];
            endpointSuffix = azureXStoreTable.ContainsKey(EndpointSuffix) ? azureXStoreTable[EndpointSuffix] : string.Empty;
            blobEndpoint = azureXStoreTable.ContainsKey(BlobEndpointField) ? azureXStoreTable[BlobEndpointField] : string.Empty;
            if (!string.IsNullOrEmpty(endpointSuffix))
            {
                string defaultEndpointProtocal = "https";
                if (azureXStoreTable.ContainsKey(DefaultEndpointsProtocolField))
                {
                    defaultEndpointProtocal = azureXStoreTable[DefaultEndpointsProtocolField];
                }

                string blobEndpointFromSuffix = string.Format(CultureInfo.InvariantCulture, DefaultBlobEndpointFormatWitSuffix, defaultEndpointProtocal, accountName, endpointSuffix);
                if (string.IsNullOrEmpty(blobEndpoint))
                {
                    blobEndpoint = blobEndpointFromSuffix;
                }
                else
                {
                    if (string.Compare(blobEndpoint, blobEndpointFromSuffix, StringComparison.Ordinal) != 0)
                    {
                        throw new ArgumentException(StringResources.ImageStoreError_InvalidParameterSpecified, "BlobEndpoint");
                    }
                }
            }

            if (azureXStoreTable.ContainsKey(SecondaryAccountKeyField))
            {
                // Construct connection string primary and secondary.
                // Primary Connection string.
                List<string> connectionStringValues = new List<string>();
                foreach (var item in azureXStoreTable)
                {
                    if (!item.Key.Equals(ImageStoreUriContainerFieldName, StringComparison.OrdinalIgnoreCase) &&
                        !item.Key.Equals(SecondaryAccountKeyField, StringComparison.OrdinalIgnoreCase))
                    {
                        connectionStringValues.Add(string.Format(CultureInfo.InvariantCulture, "{0}={1}", item.Key, item.Value));
                    }
                }

                connectionString = string.Join(";", connectionStringValues.ToArray());

                // Secondary connection string.
                int accountKeyIndex = connectionStringValues.FindIndex((pair) => pair.StartsWith(AccountKeyField + "=", StringComparison.OrdinalIgnoreCase));

                if (accountKeyIndex < 0 || accountKeyIndex >= connectionStringValues.Count)
                {
                    throw new ArgumentException(StringResources.XStore_MissingAccountKey, untaggedImageStoreUri);
                }

                connectionStringValues[accountKeyIndex] = string.Format(CultureInfo.InvariantCulture, "{0}={1}", AccountKeyField, azureXStoreTable[SecondaryAccountKeyField]);
                secondaryConnectionString = string.Join(";", connectionStringValues.ToArray());
            }
        }
    }
}