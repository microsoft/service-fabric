// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
namespace System.Fabric.Store
{
    internal class PostMergeMetadatableInformation
    {
        public PostMergeMetadatableInformation(List<uint> deletedFileIds, FileMetadata newMergedFile)
        {
            this.DeletedFileIds = deletedFileIds;
            this.NewMergedFile = newMergedFile;
        }

        public List<uint> DeletedFileIds { get; private set; }

        public FileMetadata NewMergedFile { get; private set; }
    }
}