// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Data.Common;

    internal class FileCountMergeConfiguration
    {
        /// <summary>
        /// Default number of files
        /// </summary>
        private const int DefaultMergeThreshold = 16;

        /// <summary>
        /// File size threshold for a file to be considered very small: 1 MB
        /// </summary>
        public const long DefaultVerySmallFileSizeThreshold = 1024 * 1024;

        /// <summary>
        /// File size threshold for a file to be considered small: 16 MB
        /// </summary>
        public const long DefaultSmallFileSizeThreshold = DefaultVerySmallFileSizeThreshold * 16;

        /// <summary>
        /// File size threshold for a file to be considered medium: 256 MB
        /// </summary>
        public const long DefaultMediumFileSizeThreshold = DefaultSmallFileSizeThreshold * 16;

        /// <summary>
        /// File size threshold for a file to be considered large: 4 GB
        /// </summary>
        public const long DefaultLargeFileSizeThreshold = DefaultMediumFileSizeThreshold * 16;

        /// <summary>
        /// Number of files in a given file type that will trigger a merge.
        /// </summary>
        private readonly ushort fileCountMergeThreshold;

        /// <summary>
        /// Mapping between file type and its largest file size.
        /// </summary>
        private readonly SortedList<ushort, long> fileSizeThresholdDictionary;

        private readonly string traceType;

        /// <summary>
        /// Initializes a new instance of the FileCountMergeConfiguration struct.
        /// </summary>
        public FileCountMergeConfiguration(string traceType) : this(DefaultMergeThreshold)
        {
            this.traceType = traceType;
        }

        /// <summary>
        /// Initializes a new instance of the FileCountMergeConfiguration struct.
        /// </summary>
        /// <param name="mergeTheshold"></param>
        internal FileCountMergeConfiguration(ushort mergeTheshold): this(mergeTheshold, GenerateDefaultFileTypeToSizeMap())
        {
        }

        /// <summary>
        /// Initializes a new instance of the FileCountMergeConfiguration struct.
        /// </summary>
        /// <param name="fileCountMergeThreshold"></param>
        /// <param name="fileSizeThresholdDictionary"></param>
        internal FileCountMergeConfiguration(ushort fileCountMergeThreshold, SortedList<ushort, long> fileSizeThresholdDictionary)
        {
            Diagnostics.Assert(fileCountMergeThreshold > 1, this.traceType, "FileCountMergeThreshold");
            this.AssertIfInvalidFileTypeToSizeMap(fileSizeThresholdDictionary);

            this.fileCountMergeThreshold = fileCountMergeThreshold;
            this.fileSizeThresholdDictionary = fileSizeThresholdDictionary;
        }


        public ushort FileCountMergeThreshold
        {
            get
            {
                return this.fileCountMergeThreshold;
            }
        }

        public ushort GetFileType(long fileSize)
        {
            ushort fileType = checked((ushort)this.fileSizeThresholdDictionary.Count);
            ushort fileTypeCount = checked((ushort)this.fileSizeThresholdDictionary.Count);
            for (ushort i = 0; i < fileTypeCount; i++)
            {
                if (fileSize < this.GetFileSizeThreshold(i))
                {
                    fileType = i;
                    break;
                }
            }

            return fileType;
        }

        public int GetFileTypeCount()
        {
            return this.fileSizeThresholdDictionary.Count;
        }

        public long GetFileSizeThreshold(int typeId)
        {
            return this.fileSizeThresholdDictionary.Values[typeId];
        }

        private static SortedList<ushort, long> GenerateDefaultFileTypeToSizeMap()
        {
            var list = new SortedList<ushort, long>();
            list.Add(0, DefaultVerySmallFileSizeThreshold);
            list.Add(1, DefaultSmallFileSizeThreshold);
            list.Add(2, DefaultMediumFileSizeThreshold);
            list.Add(3, DefaultLargeFileSizeThreshold);

            return list;
        }

        private void AssertIfInvalidFileTypeToSizeMap(SortedList<ushort, long> fileSizeThresholdDictionary)
        {
            long lastSize = long.MinValue;
            foreach (var row in fileSizeThresholdDictionary)
            {
                Diagnostics.Assert(row.Value > 0, this.traceType, "value {0} for key {1} must be larger than 0", row.Value, row.Key);
                if (lastSize != long.MinValue)
                {
                    Diagnostics.Assert(row.Value > lastSize, this.traceType, "Values must be growing in size {0} > {1}", row.Value, lastSize);
                }

                lastSize = row.Value;
            }
        }
    }
}