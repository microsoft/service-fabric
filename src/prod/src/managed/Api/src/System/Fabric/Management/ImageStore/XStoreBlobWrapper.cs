// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageStore
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Fabric.Common;
    using System.Fabric.ImageStore;
    using System.Fabric.Strings;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.RetryPolicies;

    /// <summary>
    /// Wrapper class for blob operations that can stuck without timeout
    /// </summary>
    internal class XStoreBlobWrapper
    {
        /// <summary>Min time out value  </summary>
        private const int MinTimeoutSeconds = 60;

        /// <summary>Buffer size of the download </summary>
        private const int PartialDownloadBufferLen = 4 * 1024 * 1024;

        /// <summary>Wrapped blob object </summary>
        private readonly CloudBlockBlob blob;

        /// <summary>Request option for blob operations  </summary>
        private readonly BlobRequestOptions reqOption;

        /// <summary>
        /// Initializes a new instance of the XStoreBlobWrapper class.
        /// </summary>
        /// <param name="blob">The cloud blob object.</param>
        public XStoreBlobWrapper(CloudBlockBlob blob)
        {
            this.blob = blob;
            //ExponentialRetry with 3 max retry attempts and 3 second back off interval each.
            this.reqOption = new BlobRequestOptions { RetryPolicy = XStoreCommon.DefaultRetryPolicy }; 
        }

        /// <summary>
        /// Wrapped function of FetchAttributes with timeout
        /// </summary>
        public void FetchAttributes()
        {
            this.reqOption.MaximumExecutionTime = new TimeSpan(0, 0, MinTimeoutSeconds);
#if !DotNetCoreClr
            this.blob.FetchAttributes(null, this.reqOption);
#else
            this.blob.FetchAttributesAsync(null, this.reqOption, null).Wait();
#endif
        }

        /// <summary>
        /// Wrapped function of uploading file with timeout
        /// </summary>
        /// <param name="filename">The file name to upload.</param>
        /// <param name="length">The part of the file to upload.</param>
        /// <param name="timeout">The timeout for performing the uploading operation.</param>
        public void UploadFromFile(string filename, int length, TimeSpan timeout)
        {
            UpDownloadParams uploadParam = new UpDownloadParams(this.blob, filename, 0, length);
            int caluatedTimeout = GetTimeOutInSeconds(uploadParam.Length);
            this.reqOption.MaximumExecutionTime = (timeout == TimeSpan.MaxValue || timeout.Seconds <caluatedTimeout)
                ? new TimeSpan(0, 0, caluatedTimeout)
                : timeout;
            using (var stream = FabricFile.Open(uploadParam.Filename, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
#if !DotNetCoreClr
                uploadParam.Blob.UploadFromStream(stream, null, this.reqOption);
#else
                uploadParam.Blob.UploadFromStreamAsync(stream, null, this.reqOption, null).Wait();
#endif
            }
        }

        /// <summary>
        /// Wrapped function of copying file with timeout
        /// </summary>
        /// <param name="sourceBlob">The CloubBlob referencing the source.</param>
        /// <param name="length">The size of the file to be copied.</param>
        /// <param name="timeout">The timeout for performing the copying operation.</param>
        public void CopyFromXStore(CloudBlockBlob sourceBlob, int length, TimeSpan timeout)
        {
            CopyParams copyParam = new CopyParams(this.blob, sourceBlob, length);
            int caluatedTimeout = GetTimeOutInSeconds(copyParam.Length);
            this.reqOption.MaximumExecutionTime = (timeout == TimeSpan.MaxValue || timeout.Seconds < caluatedTimeout)
                ? new TimeSpan(0,0, caluatedTimeout)
                : timeout;
#if !DotNetCoreClr
            copyParam.Blob.StartCopy(copyParam.SourceBlob, null, null, this.reqOption);
#else
            copyParam.Blob.StartCopyAsync(copyParam.SourceBlob, null, null, this.reqOption, null).Wait();
#endif
        }

        /// <summary>
        /// Wrapped function of downloading file with timeout
        /// </summary>
        /// <param name="filename">The filename to which to download.</param>
        /// <param name="length">The length of the content to be downloaded</param>
        /// <param name="timeout">The timeout for performing the download operation.</param>
        /// <param name="operContext">Represents the context for a request operation.</param>
        public void DownloadToFile(string filename, int length, TimeSpan timeout, OperationContext operContext)
        {
            UpDownloadParams downloadParam = new UpDownloadParams(this.blob, filename, 0, length);
            int caluatedTimeout = GetTimeOutInSeconds(downloadParam.Length);
            this.reqOption.MaximumExecutionTime = (timeout == TimeSpan.MaxValue || timeout.Seconds < caluatedTimeout)
                ? new TimeSpan(0, 0, caluatedTimeout)
                : timeout;
          
#if !DotNetCoreClr
            downloadParam.Blob.DownloadToFile(downloadParam.Filename, FileMode.CreateNew, null, this.reqOption, operContext);
#else
            downloadParam.Blob.DownloadToFileAsync(downloadParam.Filename, FileMode.CreateNew, null, this.reqOption, operContext).Wait();
#endif

        }

        /// <summary>
        /// Wrapped function of downloading partial file with timeout
        /// </summary>
        /// <param name="filename">The file name to download to.</param>
        /// <param name="offset">Start offset of the download.</param>
        /// <param name="length">The lenth of content to download.</param>
        /// <param name="timeout">The timeout for performing the download operation.</param>
        public void DownloadPartToFile(string filename, int offset, int length, TimeSpan timeout)
        {
            UpDownloadParams downloadParam = new UpDownloadParams(this.blob, filename, offset, length) as UpDownloadParams;
            int caluatedTimeout = GetTimeOutInSeconds(downloadParam.Length);
            this.reqOption.MaximumExecutionTime = (timeout == TimeSpan.MaxValue || timeout.Seconds < caluatedTimeout)
                ? new TimeSpan(0, 0, caluatedTimeout)
                : timeout;
            using (FileStream fs = FabricFile.Open(downloadParam.Filename, FileMode.OpenOrCreate, FileAccess.Write, FileShare.None))
            {
                // Create the Blob and upload the file
#if !DotNetCoreClr
                using (Stream blobStream = downloadParam.Blob.OpenRead(null, this.reqOption))
#else
                using (Stream blobStream = downloadParam.Blob.OpenReadAsync(null, this.reqOption, null).Result)
#endif
                {
                    int len = 0;
                    byte[] buffer = new byte[PartialDownloadBufferLen];
                    while (len < downloadParam.Length)
                    {
                        blobStream.Position = downloadParam.Offset + len;
                        int count = blobStream.Read(buffer, 0, Math.Min(PartialDownloadBufferLen, downloadParam.Length - len));
                        fs.Write(buffer, 0, count);
                        len += count;
                    }
                }
            }

        }

        /// <summary>
        /// Get timeout setting for each blob operation
        /// </summary>
        /// <param name="size">
        /// Size in bytes
        /// </param>
        /// <returns>The timeout value in seconds.</returns>
        private static int GetTimeOutInSeconds(int size)
        {
            int timeout = Math.Max(size / ImageStoreConfig.XStoreMinTransferBPS, MinTimeoutSeconds);
            if (timeout < 0)
            {
                return int.MaxValue / 1000;
            }

            return Math.Min(int.MaxValue / 1000, timeout);
        }

        /// <summary>
        /// Class to wrap parameters for blob upload/download
        /// </summary>
        internal class UpDownloadParams
        {
            /// <summary>
            /// Initializes a new instance of the UpDownloadParams class.
            /// </summary>
            /// <param name="blob">Blob object for this upload/download</param>
            /// <param name="filename">Local file name to be uploaded/downloaded</param>
            /// <param name="offset">The offset position for partial upload/download</param>
            /// <param name="length">The length of this upload/download</param>
            public UpDownloadParams(CloudBlockBlob blob, string filename, int offset, int length)
            {
                this.Blob = blob;
                this.Filename = filename;
                this.Offset = offset;
                this.Length = length;
            }

            /// <summary>Gets or sets the Wrapped CloudBlockBlob object</summary>
            public CloudBlockBlob Blob { get; set; }

            /// <summary>Gets or sets the The file name</summary>
            public string Filename { get; set; }

            /// <summary>Gets or sets the Offset in the file </summary>
            public int Offset { get; set; }

            /// <summary>Gets or sets the length to be upload/download </summary>
            public int Length { get; set; }

            public CloudBlockBlob SourceBlob { get; set; }
        }
    }

    /// <summary>
    /// Class to wrap parameters for blob copy
    /// </summary>
    internal class CopyParams
    {
        public CopyParams(CloudBlockBlob blob, CloudBlockBlob sourceBlob, int length)
        {
            this.Blob = blob;
            this.SourceBlob = sourceBlob;
            this.Length = length;
        }

        /// <summary>Gets or sets the Wrapped CloudBlockBlob object</summary>
        public CloudBlockBlob Blob { get; set; }

        /// <summary>Gets or sets the length to be upload/download </summary>
        public int Length { get; set; }

        /// <summary>Gets or sets the source CloudBlockBlob object</summary>
        public CloudBlockBlob SourceBlob { get; set; }
    }
}