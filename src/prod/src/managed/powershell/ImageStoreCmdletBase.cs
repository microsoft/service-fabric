// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Management.Automation;

    public abstract class ImageStoreCmdletBase : CommonCmdletBase
    {
        internal void OutputStoreFileInfo(ImageStoreFile fileInfo)
        {
            var filePSObj = new PSObject();
            filePSObj.Properties.Add(new PSNoteProperty(Constants.StoreRelativePathPropertyName, fileInfo.StoreRelativePath));
            filePSObj.Properties.Add(new PSNoteProperty("Type", "File"));
            filePSObj.Properties.Add(new PSNoteProperty(Constants.FileSizePropertyName, this.BytesToString(fileInfo.FileSize)));
            filePSObj.Properties.Add(new PSNoteProperty(Constants.ModifiedDatePropertyName, fileInfo.ModifiedDate.ToString()));
            this.OutputVersionInfo(fileInfo.VersionInfo, filePSObj);
            this.WriteObject(filePSObj, true);
        }

        internal void OutputStoreFolderInfo(ImageStoreFolder folderInfo)
        {
            var folderPSObj = new PSObject();
            folderPSObj.Properties.Add(new PSNoteProperty(Constants.StoreRelativePathPropertyName, folderInfo.StoreRelativePath));
            folderPSObj.Properties.Add(new PSNoteProperty("Type", string.Format("Folder [{0} files]", folderInfo.FileCount.ToString())));
            this.OutputVersionInfo(folderInfo.VersionInfo, folderPSObj);
            this.WriteObject(folderPSObj, true);
        }

        protected string BytesToString(long size)
        {
            uint kilo = 1024;
            if (size == 0)
            {
                return "0 KB";
            }

            if (size <= kilo)
            {
                return "1 KB";
            }

            string[] symbols = { "KB", "MB", "GB" };
            long scale = Convert.ToInt64(Math.Floor(Math.Log(size, kilo)));
            return (Math.Sign((long)size) * Math.Round(size / Math.Pow(kilo, scale), 1)).ToString() + " " + symbols[scale - 1];
        }

        protected bool VerifyAppVersion(IClusterConnection clusterConnection, string applicationTypeName, string applicationTypeVersion)
        {
            var queryDesc = new PagedApplicationTypeQueryDescription();
            queryDesc.ApplicationTypeNameFilter = applicationTypeName;

            bool exists = false;
            var queryResult = this.GetAllPages(clusterConnection, queryDesc);
            foreach (var item in queryResult)
            {
                if (string.Equals(item.ApplicationTypeVersion, applicationTypeVersion, StringComparison.OrdinalIgnoreCase))
                {
                    exists = true;
                    break;
                }
            }

            return exists;
        }

        protected List<string> GetAppVersions(IClusterConnection clusterConnection, string applicationTypeName)
        {
            var queryDesc = new PagedApplicationTypeQueryDescription();
            queryDesc.ApplicationTypeNameFilter = applicationTypeName;

            var versions = new List<string>();
            var appTypeList = this.GetAllPages(clusterConnection, queryDesc);
            foreach (var item in appTypeList)
            {
                versions.Add(item.ApplicationTypeVersion);
            }

            return versions;
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Reviewed. Suppression is OK here for psObj.")]
        private void OutputVersionInfo(ImageStoreVersionInfo version, PSObject psObj)
        {
            if (!string.IsNullOrEmpty(version.ServiceManifestName))
            {
                psObj.Properties.Add(new PSNoteProperty(Constants.ServiceManifestNamePropertyName, version.ServiceManifestName));
            }

            if (!string.IsNullOrEmpty(version.ServiceManifestVersion))
            {
                psObj.Properties.Add(new PSNoteProperty(Constants.ServiceManifestVersionPropertyName, version.ServiceManifestVersion));
            }

            string appVersion = version.ApplicationVersion;
            if (!string.IsNullOrEmpty(appVersion))
            {
                psObj.Properties.Add(new PSNoteProperty(Constants.ApplicationVersionPropertyName, appVersion));
            }
        }

        private ApplicationTypePagedList GetAllPages(IClusterConnection clusterConnection, PagedApplicationTypeQueryDescription queryDescription)
        {
            var allResults = new ApplicationTypePagedList();

            bool morePages = true;
            string currentContinuationToken = queryDescription.ContinuationToken;

            while (morePages)
            {
                // Update continuation token
                queryDescription.ContinuationToken = currentContinuationToken;

                var queryResult = clusterConnection.GetApplicationTypePagedListAsync(queryDescription, this.GetTimeout(), this.GetCancellationToken()).Result;

                foreach (var item in queryResult)
                {
                    allResults.Add(item);
                }

                morePages = Helpers.ResultHasMorePages(this, queryResult.ContinuationToken, out currentContinuationToken);
            }

            return allResults;
        }
    }
}