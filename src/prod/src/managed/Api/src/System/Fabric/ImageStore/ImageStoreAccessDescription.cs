// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System.Fabric.Common;
    using System.IO;
    using System.Security.Principal;

    /// <summary>
    /// <para>Provides information about the permissions or access rights to the specific user or group.</para>
    /// </summary>
    internal class ImageStoreAccessDescription
    {
        /// <summary>
        /// <para>A Windows user or group represented by the specified Windows account token or authentication.</para>
        /// </summary>
#if !DotNetCoreClrLinux
        public WindowsIdentity WindowsIdentity { get; private set; }
#endif

        /// <summary>
        /// <para>Indicates whether the specific user or group has read access.</para>
        /// </summary>
        public bool HasReadAccess { get; private set; }

        /// <summary>
        /// <para>Indicates whether the specific user or group has write access.</para>        
        /// </summary>
        public bool HasWriteAccess { get; private set; }

        /// <summary>
        /// Initializes a new instance of ImageStoreAccessDescription class
        /// </summary>
        /// <param name="imageStoreAccessWindowsIdentity">A Windows user or group represented by the specified Windows account token or authentication.</param>
        /// <param name="localRoot">The local path of the working folder.</param>
#if !DotNetCoreClrLinux
        public ImageStoreAccessDescription(WindowsIdentity imageStoreAccessWindowsIdentity, string localRoot)
        {
            if (imageStoreAccessWindowsIdentity == null)
            {
                throw new ArgumentNullException("imageStoreAccessWindowsIdentity");
            }

            this.WindowsIdentity = imageStoreAccessWindowsIdentity;
            this.HasReadAccess = CheckAccess(localRoot, FileAccess.Read);
            this.HasWriteAccess = CheckAccess(localRoot, FileAccess.Write);
        }
#endif

        private bool CheckAccess(string localRoot, FileAccess access)
        {
            if (string.IsNullOrEmpty(localRoot))
            {
                return false;
            }

            string tempFileName = Path.Combine(localRoot, Path.GetRandomFileName());

            using (FileStream fileStream = FabricFile.Open(tempFileName, FileMode.Create, FileAccess.ReadWrite, FileShare.ReadWrite)) {}

            try
            {
                // Issue : https://github.com/dotnet/corefx/issues/17164
#if !DotNetCoreClr
                using (WindowsImpersonationContext impersonationContext = this.WindowsIdentity.Impersonate())
#endif
                {
                    FileStream accessCheckStream = null;
                    try
                    {
                        FileShare fileShareOption = FileShare.None;
                        switch (access)
                        {
                            case FileAccess.Read:
                                fileShareOption = FileShare.Read;
                                break;

                            case FileAccess.Write:
                                fileShareOption = FileShare.Write;
                                break;

                            case FileAccess.ReadWrite:
                                fileShareOption = FileShare.ReadWrite;
                                break;
                        }

                        accessCheckStream = FabricFile.Open(tempFileName, FileMode.Open, access, fileShareOption);
                    }
                    catch (UnauthorizedAccessException)
                    {
                        return false;
                    }
                    finally
                    {
                        if (accessCheckStream != null) { accessCheckStream.Dispose(); }
                    }
                }
            }
            finally
            {
                if (FabricFile.Exists(tempFileName)) { FabricFile.Delete(tempFileName, deleteReadonly: true); };
            }

            return true;
        }
    }
}