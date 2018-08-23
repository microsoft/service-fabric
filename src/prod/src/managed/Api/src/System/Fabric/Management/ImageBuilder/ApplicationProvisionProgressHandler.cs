// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.IO;
    using System.Globalization;
    using System.Fabric.Strings;
    using System.Fabric.ImageStore;
    using System.Text;

    class ApplicationProvisionProgressHandler : IImageStoreProgressHandler
    {
        private readonly string operationName;
        private readonly string progressFileName;
        private readonly object updateLock;

        private bool isUpdating;

        public ApplicationProvisionProgressHandler(string operationName, string progressFileName)
        {
            this.operationName = operationName;
            this.progressFileName = progressFileName;
            this.updateLock = new object();
            this.isUpdating = false;
        }

        public void WriteStatusDownloading(string buildPackagePath)
        {
            WriteProgressStatus(string.Format(CultureInfo.CurrentCulture, 
                StringResources.ImageBuilder_Progress_Downloading, 
                buildPackagePath));
        }

        public void WriteStatusExtractingAndValidatingAppType(string downloadPath)
        {
            WriteProgressStatus(string.Format(CultureInfo.CurrentCulture,
                StringResources.ImageBuilder_Progress_ExtractingAndValidatingAppType,
                downloadPath));
        }

        public void WriteStatusExtractingApplicationPackage(string packageName)
        {
            WriteProgressStatus(string.Format(CultureInfo.CurrentCulture,
                StringResources.ImageBuilder_Progress_ExtractingAndValidatingAppType,
                packageName));
        }

        public void WriteStatusValidating()
        {
            WriteProgressStatus(string.Format(CultureInfo.CurrentCulture,
                StringResources.ImageBuilder_Progress_Validating));
        }

        public void WriteStatusDownloadingExternalPackageProgress(
            string sourcePath,
            long completed,
            long total,
            int percentage)
        {
            WriteProgressStatus(
                string.Format(CultureInfo.CurrentCulture,
                    StringResources.ImageBuilder_Progress_Downloading_ExternalSfpkg_Progress,
                    sourcePath,
                    completed,
                    total,
                    percentage / 100.0));
        }

        public void WriteStatusUploading(
            string sourcePath, 
            int completed,
            int total,
            bool shouldUpload)
        {
            WriteProgressStatus(string.Format(CultureInfo.CurrentCulture,
                (shouldUpload ? StringResources.ImageBuilder_Progress_Upload : StringResources.ImageBuilder_Progress_Copy),
                sourcePath,
                completed,
                total,
                (double)completed/total));
        }
        
        TimeSpan IImageStoreProgressHandler.GetUpdateInterval()
        {
            return TimeSpan.Zero; // Use cluster default
        }

        void IImageStoreProgressHandler.UpdateProgress(long completedItems, long totalItems, ProgressUnitType itemType)
        {
            this.WriteProgressStatus(string.Format(CultureInfo.CurrentCulture,
               StringResources.ImageBuilder_Progress_Downloading_Details,
                   completedItems,
                   totalItems,
                   itemType,
                   (double)completedItems/totalItems));
        }

        private void WriteProgressStatus(string status)
        {
            lock (this.updateLock)
            {
                if (this.isUpdating)
                {
                    return;
                }
                else
                {
                    this.isUpdating = true;
                }
            }

            // Only write out new progress information after
            // CM has consumed the previous progress information file
            //
            if (!File.Exists(this.progressFileName))
            {
                var tmpFileName = string.Format("{0}.tmp", this.progressFileName);

                ImageBuilderUtility.WriteStringToFile(tmpFileName, status, false, Encoding.Unicode);

                File.Move(tmpFileName, this.progressFileName);
            }

            lock (this.updateLock)
            {
                this.isUpdating = false;
            }
        }
    }
}