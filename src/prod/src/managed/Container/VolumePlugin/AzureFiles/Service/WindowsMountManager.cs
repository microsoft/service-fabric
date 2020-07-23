//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin
{
    using System;
    using System.IO;
    using System.Reflection;

    class WindowsMountManager : IMountManager
    {
        private const string ExeName = "powershell.exe";
        private const string MapScriptName = "MapRemotePath.ps1";
        private const string UnmapScriptName = "UnmapRemotePath.ps1";

        private readonly string mountPointBase;
        private readonly string mapScriptPath;
        private readonly string unmapScriptPath;

        internal WindowsMountManager(string mountPointBase)
        {
            this.mountPointBase = mountPointBase;

            string scriptPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            this.mapScriptPath = Path.Combine(scriptPath, MapScriptName);
            this.unmapScriptPath = Path.Combine(scriptPath, UnmapScriptName);
        }

        public MountParams GetMountExeAndArgs(string name, VolumeEntry volumeEntry)
        {
            string mountPoint = Path.Combine(this.mountPointBase, name);

            string shareName = volumeEntry.VolumeOptions[Constants.ShareNameOption].Trim();
            string storageAccountFQDN = Utilities.GetStorageAccountFQDN(volumeEntry.VolumeOptions);
            string storageAccountName = Utilities.GetStorageAccountName(volumeEntry.VolumeOptions);
            string key = Utilities.GetStorageAccountKey(volumeEntry.VolumeOptions);
            string remoteShare = $"\\\\{storageAccountFQDN}\\{shareName}";
            string scriptArgs = $"{this.mapScriptPath} -RemotePath {remoteShare} -StorageAccountName {storageAccountName} -StorageAccountKey {key} -LocalPath {mountPoint}";
            string argsForLogging = $"{this.mapScriptPath} -RemotePath {remoteShare} -StorageAccountName {storageAccountName} -StorageAccountKey <hidden> -LocalPath {mountPoint}";

            return new MountParams()
            {
                Exe = ExeName,
                Arguments = scriptArgs,
                ArgumentsForLog = argsForLogging,
                MountPoint = mountPoint
            };
        }

        public UnmountParams GetUnmountExeAndArgs(VolumeEntry volumeEntry)
        {
            string mountPoint = volumeEntry.Mountpoint;
            string shareName = volumeEntry.VolumeOptions[Constants.ShareNameOption].Trim();
            string storageAccountFQDN = Utilities.GetStorageAccountFQDN(volumeEntry.VolumeOptions);
            string storageAccountName = Utilities.GetStorageAccountName(volumeEntry.VolumeOptions);
            string remoteShare = $"\\\\{storageAccountFQDN}\\{shareName}";
            string scriptArgs = $"{this.unmapScriptPath} -RemotePath {remoteShare} -LocalPath {mountPoint}";

            return new UnmountParams()
            {
                Exe = ExeName,
                Arguments = scriptArgs
            };
        }
    }
}
