// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.FileUploader 
{
    using System.Fabric.Common;
    using System.IO;

    // Class that implements helper methods that are used in trimming
    // local map folders
    internal class LocalMapTrimmingHelper
    {
        // Source folder associated with the local map
        private readonly string sourceFolder;

        internal LocalMapTrimmingHelper(string sourceFolder)
        {
            this.sourceFolder = sourceFolder;
        }

        internal void SetFolderDeletionContext(FolderTrimmer.FolderDeletionInfo folderInfo, FolderTrimmer.FolderDeletionInfo parentFolderInfo, string folderNameWithoutPath)
        {
            if (null == parentFolderInfo)
            {
                folderInfo.Context = string.Empty;
            }
            else
            {
                string parentRelativePath = (string)parentFolderInfo.Context;
                if (string.IsNullOrEmpty(parentRelativePath))
                {
                    folderInfo.Context = folderNameWithoutPath;
                }
                else
                {
                    folderInfo.Context = Path.Combine(parentRelativePath, folderNameWithoutPath);
                }
            }
        }

        internal bool OkToDeleteFile(FolderTrimmer.FolderDeletionInfo folderInfo, string fileName)
        {
            string relativePath = (string)folderInfo.Context;
            string pathOnLocalNode;
            if (string.IsNullOrEmpty(relativePath))
            {
                pathOnLocalNode = Path.Combine(this.sourceFolder, fileName);
            }
            else
            {
                pathOnLocalNode = Path.Combine(
                                        this.sourceFolder,
                                        relativePath,
                                        fileName);
            }

            return false == FabricFile.Exists(pathOnLocalNode);
        }
    }
}