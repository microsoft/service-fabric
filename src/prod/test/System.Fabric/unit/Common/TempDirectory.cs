// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common
{
    using System;
    using System.IO;

    public class TempDirectory : IDisposable
    {
        public TempDirectory()
        {
            this.Info = Directory.CreateDirectory(System.IO.Path.Combine(System.IO.Path.GetTempPath(), System.IO.Path.GetRandomFileName()));
        }

        public DirectoryInfo Info { get; private set; }
        public string Path { get { return Info.FullName; } }

        void IDisposable.Dispose()
        {
            Delete();
        }

        public void Delete()
        {
            if (Info != null)
            {
                if (Info.Exists)
                {
                    Info.Delete(true);
                }
            }
        }
    }
}