//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin
{
    using System;
    using System.IO;

    class LinuxMountManager : IMountManager
    {
        private const string MountExeName = "/bin/mount";
        private const string UnmountExeName = "/bin/umount";

        private readonly string mountPointBase;
        private readonly string workDirectory;

        internal LinuxMountManager(string mountPointBase, string workDirectory)
        {
            this.mountPointBase = mountPointBase;
            this.workDirectory = workDirectory;
        }

        public MountParams GetMountExeAndArgs(string name, VolumeEntry volumeEntry)
        {
            string mountPoint = Path.Combine(this.mountPointBase, name);
            Utilities.EnsureFolder(mountPoint);

            string shareName = volumeEntry.VolumeOptions[Constants.ShareNameOption].Trim();
            string storageAccountFQDN = Utilities.GetStorageAccountFQDN(volumeEntry.VolumeOptions);
            string storageAccountName = Utilities.GetStorageAccountName(volumeEntry.VolumeOptions);
            string key = Utilities.GetStorageAccountKey(volumeEntry.VolumeOptions);
            string mountArgs = $"-t cifs //{storageAccountFQDN}/{shareName} {mountPoint} -o vers=3.0,username={storageAccountName},password={key},dir_mode=0770,file_mode=0770,serverino";
            string argsForLogging = $"-t cifs //{storageAccountFQDN}/{shareName} {mountPoint} -o vers=3.0,username={storageAccountName},password=<hidden>,dir_mode=0770,file_mode=0770,serverino";

            return new MountParams()
            {
                Exe = MountExeName,
                Arguments = mountArgs,
                ArgumentsForLog = argsForLogging,
                MountPoint = mountPoint
            };
        }

        public UnmountParams GetUnmountExeAndArgs(VolumeEntry volumeEntry)
        {
            return new UnmountParams()
            {
                Exe = UnmountExeName,
                Arguments = volumeEntry.Mountpoint
            };
        }
    }
}
