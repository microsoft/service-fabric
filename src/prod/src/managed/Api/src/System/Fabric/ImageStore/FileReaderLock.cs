// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    internal class FileReaderLock : FileLock
    {
        public FileReaderLock(string path) 
            : base(path, true) 
        { 
        }
    }
}