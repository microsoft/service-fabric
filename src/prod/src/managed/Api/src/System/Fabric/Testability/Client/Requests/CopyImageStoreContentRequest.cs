// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// This class derives FabricRequest class and my represent a request which is specific to copy image store content operation
    /// The derived classes needs to implement ToString() and PerformCoreAsync() methods defined in FabricRequest.
    /// </summary>
    public class CopyImageStoreContentRequest : FabricRequest
    {
        /// <summary>
        /// Initializes a new instance of the CopyImageStoreContentRequest
        /// </summary>
        /// <param name="fabricClient"></param>
        /// <param name="description"></param>
        /// <param name="timeout"></param>
        public CopyImageStoreContentRequest(IFabricClient fabricClient, WinFabricCopyImageStoreContentDescription description, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(description, "description");

            this.Description = description;
        }

        /// <summary>
        /// Get the instance of WinFabricCopyImageStoreContentDescription
        /// </summary>
        public WinFabricCopyImageStoreContentDescription Description
        {
            get;
            private set;
        }

        /// <summary>
        /// A string represent the CopyImageStoreContentRequest object including WinFabricCopyImageStoreContentDescription properties
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            if (this.Description.SkipFiles != null && this.Description.SkipFiles.Length > 0)
            {
                StringBuilder builder = new StringBuilder();
                foreach (var file in this.Description.SkipFiles)
                {
                    builder.Append("{" + file + "}");
                }

                return string.Format(
                CultureInfo.InvariantCulture,
                "Copy image store content from {0} to {1} with skipping files [{2}]",
                this.Description.RemoteSource,
                this.Description.RemoteDestination,
                builder.ToString());
            }
            else
            {
                return string.Format(
                CultureInfo.InvariantCulture,
                "Copy image store content from {0} to {1}",
                this.Description.RemoteSource,
                this.Description.RemoteDestination);
            }
        }

        /// <summary>
        /// Perform image store copying operation with async
        /// </summary>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.CopyImageStoreContentAsync(this.Description, this.Timeout, cancellationToken);
        }
    }
}