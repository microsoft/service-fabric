// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.BackupRestore.Common;

    internal abstract class BaseOperation<TResult>
    {
        private const string PreviewSuffix = "-preview";

        /// <summary>
        /// Minimum API version supported by this operation
        /// </summary>
        protected double MinApiVersion { get; set; }

        /// <summary>
        /// Whether the operation is in preview
        /// </summary>
        protected bool IsInPreview { get; set; }

        /// <summary>
        /// The API version passed by user in this request
        /// </summary>
        protected string ApiVersion { get; }

        internal BaseOperation(string apiVersion)
        {
            this.ApiVersion = apiVersion;
            this.MinApiVersion = 6.4;
            this.IsInPreview = false;
        }

        internal abstract Task InitializeAsync();

        internal abstract Task<TResult> RunAsync(TimeSpan timeout, CancellationToken cancellationToken);

        internal async Task<TResult> RunAsyncWrappper(TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.ValidateApiVersion();
            await InitializeAsync();
            return await this.RunAsync(timeout, cancellationToken);
        }

        internal void ThrowInvalidArgumentIfNull<TValue>(TValue value)
        {
            if (value == null)
            {
                throw new ArgumentException(StringResources.InvalidArguments);
            }
        }

        /// <summary>
        /// Validates the API version passed in the request
        /// Can be overridden at the operation layer
        /// </summary>
        internal virtual void ValidateApiVersion()
        {
            var apiVersion = this.ApiVersion;

            // Validate preview and remove that from suffix for further validation
            if (this.IsInPreview)
            {
                if (this.ApiVersion.EndsWith(PreviewSuffix))
                {
                    var index = this.ApiVersion.LastIndexOf(PreviewSuffix);
                    apiVersion = this.ApiVersion.Substring(0, index);
                }
                else
                {
                    throw new ArgumentException(StringResources.InvalidApiVersion);
                }
            }

            // Convert the api version from string to double now
            if (Double.TryParse(apiVersion, out double doubleApiVersion))
            {
                if (doubleApiVersion >= MinApiVersion)
                {
                    // Valid API version
                    return;
                }
            }

            throw new ArgumentException(StringResources.InvalidApiVersion);
        }
    }
}