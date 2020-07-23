// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Store
{
    using System;

    /// <summary>
    /// Define size based retention policy
    /// </summary>
    public class SizeBasedRetentionPolicy : DataRetentionPolicy
    {
        /// <summary>
        /// One Mega Byte
        /// </summary>
        public static readonly double OneMegaByte = 1e+6;

        // One Giga Byte
        public static readonly double OneGigaByte = 1e+9;

        /// <summary>
        /// Default Retention Policy
        /// </summary>
        public static readonly SizeBasedRetentionPolicy DefaultPolicy = new SizeBasedRetentionPolicy(OneGigaByte, TimeSpan.FromHours(4));

        /// <summary>
        /// Create an instance of <see cref="SizeBasedRetentionPolicy"/>
        /// </summary>
        /// <param name="maxStorageSizeInBytes"></param>
        /// <param name="scrubDuration"></param>
        public SizeBasedRetentionPolicy(double maxStorageSizeInBytes, TimeSpan scrubDuration)
        {
            this.MaxSizeOfStorageInBytes = maxStorageSizeInBytes;
            this.GapBetweenScrubs = scrubDuration;
        }

        /// <summary>
        /// Max size of the collection. Once this size is reached, data scrubbing can kick in. This
        /// size doesn't include any metadata that storage may be using.
        /// </summary>
        public double MaxSizeOfStorageInBytes { get; private set; }
    }
}