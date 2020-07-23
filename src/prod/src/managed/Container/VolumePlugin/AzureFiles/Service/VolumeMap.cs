//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Fabric;
    using System.Diagnostics;
    using System.IO;
    using Microsoft.ServiceFabric.Diagnostics.Tracing;
    using AzureFilesVolumePlugin.Models;
    using Newtonsoft.Json;

    public class VolumeMap : IDisposable
    {
        private const string TraceType = "AzureFilesVolMap";
        private const string volumesMetadata = "volumesMetadata";
        private const string mountMetadata = "mountMetadata";
        private const int defaultProcessRuntimeMs = 30000; // 30 seconds

        // TODO: Coarse lock - improve.
        private readonly ReaderWriterLockSlim rwLock;
        private readonly Dictionary<string, VolumeEntry> knownVolumeMappings;
        private readonly ServiceContext serviceContext;
        private readonly string mountPointBase;
        private readonly IMountManager mountManager;
        private readonly string volumesMetadataPath; // ../work/volumesMetadata/
        private readonly string mountMetadataPath;   // ../work/mountMetadata/

        public VolumeMap(ServiceContext serviceContext)
        {
            this.rwLock = new ReaderWriterLockSlim();
            this.knownVolumeMappings = new Dictionary<string, VolumeEntry>();
            this.serviceContext = serviceContext;
            this.mountPointBase = Path.Combine(this.serviceContext.CodePackageActivationContext.WorkDirectory, Constants.Mounts);
            Utilities.EnsureFolder(this.mountPointBase);
            this.volumesMetadataPath = Path.Combine(this.serviceContext.CodePackageActivationContext.WorkDirectory, volumesMetadata);
            Utilities.EnsureFolder(volumesMetadataPath);
            this.mountMetadataPath = Path.Combine(this.serviceContext.CodePackageActivationContext.WorkDirectory, mountMetadata);
            Utilities.EnsureFolder(mountMetadataPath);

            var os = Utilities.GetOperatingSystem(this.serviceContext.CodePackageActivationContext);
            this.mountManager = (os == AzureFilesVolumePluginSupportedOs.Linux) ?
                (IMountManager) (new LinuxMountManager(
                                        this.mountPointBase,
                                        this.serviceContext.CodePackageActivationContext.WorkDirectory)) :
                (IMountManager) (new WindowsMountManager(this.mountPointBase));

            this.Initialize();
        }

        public Response CreateVolume(VolumeCreateRequest request)
        {
            if (this.VolumeExists(request.Name))
            {
                TraceWriter.WriteInfoWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"volume ${request.Name} already exists");
                return new Response($"volume ${request.Name} already exists");
            }

            this.rwLock.EnterWriteLock();
            try
            {
                if (knownVolumeMappings.ContainsKey(request.Name))
                {
                    return new Response("Already exists");
                }

                if (!request.Opts.ContainsKey(Constants.ShareNameOption))
                {
                    return new Response("Required argument shareName not present");
                }

                if (!request.Opts.ContainsKey(Constants.StorageAccountNameOption))
                {
                    return new Response("Required argument storageAccountName not present");
                }

                
                if (!request.Opts.ContainsKey(Constants.StorageAccountFQDNOption))
                {
                    // TODO: Require storageAccountFQDN and fail the request at some timepoint
                    TraceWriter.WriteInfoWithId(
                        TraceType,
                        this.serviceContext.TraceId,
                        $"{Constants.StorageAccountFQDNOption} not present");
                }

                string mountMetadataDirectoryPath = Path.Combine(mountMetadataPath, request.Name);
                Utilities.EnsureFolder(mountMetadataDirectoryPath);

                string volumesMetadataFilename = Path.Combine(volumesMetadataPath, request.Name);
                if (!this.WriteMetadataFile(volumesMetadataFilename, request))
                {
                    return new Response($"Unable to persist metadata for creating volume {request.Name}");
                }

                this.knownVolumeMappings[request.Name] = new VolumeEntry() { VolumeOptions = request.Opts };

                return new Response();
            }
            catch (Exception e)
            {
                TraceWriter.WriteErrorWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"Create volume {request.Name} failed with exception {e}.");

                return new Response($"Create volume {request.Name} failed with exception {e}.");
            }
            finally
            {
                this.rwLock.ExitWriteLock();
            }
        }

        public Response MountVolume(VolumeMountRequest request)
        {
            if (!this.VolumeExists(request.Name))
            {
                return new Response("volume not found");
            }

            this.rwLock.EnterWriteLock();
            try
            {
                if (!knownVolumeMappings.ContainsKey(request.Name))
                {
                    return new Response("volume not found");
                }

                if (string.IsNullOrEmpty(this.knownVolumeMappings[request.Name].Mountpoint))
                {
                    // TODO: Holding the lock during network call - improve.
                    var item = this.knownVolumeMappings[request.Name];
                    if (!this.DoMount(request.Name, ref item))
                    {
                        return new Response("mount failed");
                    }
                }

                string mountMetadataFilename = Path.Combine(mountMetadataPath, request.Name, request.ID);
                // Using just File.Create will leave the file open, dispose immediately
                System.IO.File.Create(mountMetadataFilename).Dispose();

                this.knownVolumeMappings[request.Name].MountIDs.Add(request.ID);

                return new VolumeMountResponse(this.knownVolumeMappings[request.Name].Mountpoint);
            }
            catch (Exception e)
            {
                TraceWriter.WriteErrorWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"Mount volume {request.Name}:{request.ID} failed with exception {e}.");

                return new Response($"Mount volume {request.Name}:{request.ID} failed with exception {e}.");
            }
            finally
            {
                this.rwLock.ExitWriteLock();
            }
        }

        public Response UnmountVolume(VolumeUnmountRequest request)
        {
            if (!this.VolumeExists(request.Name))
            {
                return new Response("volume not found");
            }

            this.rwLock.EnterWriteLock();
            try
            {
                if (!knownVolumeMappings.ContainsKey(request.Name))
                {
                    return new Response("volume not found");
                }

                string mountMetadataFilename = Path.Combine(mountMetadataPath, request.Name, request.ID);

                bool mountpointNotExist = string.IsNullOrEmpty(this.knownVolumeMappings[request.Name].Mountpoint);
                bool mountIDNotExist = !this.knownVolumeMappings[request.Name].MountIDs.Contains(request.ID);
                bool mountMetadataFileNotExist = !System.IO.File.Exists(mountMetadataFilename);
                if (mountpointNotExist || mountIDNotExist || mountMetadataFileNotExist)
                {
                    TraceWriter.WriteWarningWithId(
                        TraceType,
                        this.serviceContext.TraceId,
                        $"Unmount volume {request.Name}:{request.ID} unexpected state: mountpointNotExist-{mountpointNotExist} mountIDNotExist-{mountIDNotExist}:mountMetadataFileNotExist-{mountMetadataFileNotExist}");

                    // We are not tracking this volume mount.
                    return new Response();
                }

                System.IO.File.Delete(mountMetadataFilename);

                this.knownVolumeMappings[request.Name].MountIDs.Remove(request.ID);

                return new Response();
            }
            catch (Exception e)
            {
                TraceWriter.WriteErrorWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"Unmount volume {request.Name} failed with exception {e}.");

                return new Response($"Unmount volume {request.Name}:{request.ID} failed with exception {e}.");
            }
            finally
            {
                this.rwLock.ExitWriteLock();
            }

        }

        public Response ListVolumes()
        {
            this.rwLock.EnterReadLock();
            try
            {
                var volumesListResponse = new VolumeListResponse();
                foreach (var item in this.knownVolumeMappings)
                {
                    volumesListResponse.Volumes.Add(new VolumeMountDescription()
                    {
                        Name = item.Key,
                        Mountpoint = item.Value.Mountpoint
                    });
                }

                return volumesListResponse;
            }
            finally
            {
                this.rwLock.ExitReadLock();
            }
        }

        public Response GetVolume(VolumeName request)
        {
            this.rwLock.EnterReadLock();
            try
            {
                if (!knownVolumeMappings.ContainsKey(request.Name))
                {
                    return new VolumeGetResponse("volume not found");
                }

                return new VolumeGetResponse(
                        new VolumeMountDescription()
                        {
                            Name = request.Name,
                            Mountpoint = knownVolumeMappings[request.Name].Mountpoint
                        });
            }
            finally
            {
                this.rwLock.ExitReadLock();
            }
        }

        public Response GetVolumeMountPoint(VolumeName request)
        {
            this.rwLock.EnterReadLock();
            try
            {
                if (!knownVolumeMappings.ContainsKey(request.Name))
                {
                    return new Response("volume not found");
                }

                return new VolumeMountResponse(this.knownVolumeMappings[request.Name].Mountpoint);
            }
            finally
            {
                this.rwLock.ExitReadLock();
            }
        }

        public Response RemoveVolume(VolumeName request)
        {
            if (!this.VolumeExists(request.Name))
            {
                return new Response("volume not found");
            }

            this.rwLock.EnterWriteLock();
            try
            {
                if (!knownVolumeMappings.ContainsKey(request.Name))
                {
                    return new Response("volume not found");
                }

                if (this.knownVolumeMappings[request.Name].MountIDs.Count != 0)
                {
                    return new Response("volume in use");
                }

                if (!this.DoUnmount(this.knownVolumeMappings[request.Name]))
                {
                    TraceWriter.WriteErrorWithId(
                        TraceType,
                        this.serviceContext.TraceId,
                        "unmount during remove failed");
                    return new Response("unmount failed");
                }

                string volumesMetadataFilename = Path.Combine(volumesMetadataPath, request.Name);
                if (System.IO.File.Exists(volumesMetadataFilename))
                {
                    System.IO.File.Delete(volumesMetadataFilename);
                }
                else
                {
                    TraceWriter.WriteWarningWithId(
                        TraceType,
                        this.serviceContext.TraceId,
                        $"Remove volume {request.Name} unexpected state: volumesMetadataFilename {volumesMetadataFilename} not exists");
                }

                string mountMetadataDirectoryPath = Path.Combine(mountMetadataPath, request.Name);
                if (System.IO.Directory.Exists(mountMetadataDirectoryPath))
                {
                    System.IO.Directory.Delete(mountMetadataDirectoryPath);
                }
                else
                {
                    TraceWriter.WriteWarningWithId(
                        TraceType,
                        this.serviceContext.TraceId,
                        $"Remove volume {request.Name} unexpected state: mountMetadataDirectoryPath {mountMetadataDirectoryPath} not exists");
                }

                this.knownVolumeMappings.Remove(request.Name);

                return new Response();
            }
            catch (Exception e)
            {
                TraceWriter.WriteErrorWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"Remove volume {request.Name} failed with exception {e}.");

                return new Response($"Remove volume {request.Name} failed with exception {e}.");
            }
            finally
            {
                this.rwLock.ExitWriteLock();
            }

        }

        #region Helpers
        bool VolumeExists(string name)
        {
            this.rwLock.EnterReadLock();
            try
            {
                if (knownVolumeMappings.ContainsKey(name))
                {
                    return true;
                }

                return false;
            }
            finally
            {
                this.rwLock.ExitReadLock();
            }
        }

        //
        // The Mount and Unmount methods are platform specific.
        //
        bool DoMount(string name, ref VolumeEntry volumeEntry)
        {
            string mountPoint = null;
            try
            {
                var mountParams = this.mountManager.GetMountExeAndArgs(name, volumeEntry);
                mountPoint = mountParams.MountPoint;

                //
                // TODO: Change to sys call instead of process create for better diagnostics
                //
                TraceWriter.WriteInfoWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"{mountParams.Exe} {mountParams.ArgumentsForLog}");

                using (Process exeProcess = new Process())
                {
                    exeProcess.StartInfo.UseShellExecute = false;
                    exeProcess.StartInfo.RedirectStandardOutput = true;
                    exeProcess.StartInfo.RedirectStandardError = true;
                    exeProcess.StartInfo.FileName = mountParams.Exe;
                    exeProcess.StartInfo.Arguments = mountParams.Arguments;
                    exeProcess.Start();
                    if (!exeProcess.WaitForExit(defaultProcessRuntimeMs))
                    {
                        TraceWriter.WriteErrorWithId(
                            TraceType,
                            this.serviceContext.TraceId,
                            $"mount didnt complete in the required time. Output:{exeProcess.StandardOutput.ReadToEnd()} Error:{exeProcess.StandardError.ReadToEnd()}");
                        exeProcess.Kill();
                        return false;
                    }
                    if (exeProcess.ExitCode != 0)
                    {
                        TraceWriter.WriteErrorWithId(
                            TraceType,
                            this.serviceContext.TraceId,
                            $"Error {exeProcess.ExitCode} encountered during mount. Output:{exeProcess.StandardOutput.ReadToEnd()} Error:{exeProcess.StandardError.ReadToEnd()}");
                        return false;
                    }
                    else
                    {
                        TraceWriter.WriteInfoWithId(
                           TraceType,
                           this.serviceContext.TraceId,
                           $"mount complete. Output:{exeProcess.StandardOutput.ReadToEnd()} Error:{exeProcess.StandardError.ReadToEnd()}");
                    }
                }
            }
            catch (Exception e)
            {
                TraceWriter.WriteErrorWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"mount failed with exception {e}.");
                return false;
            }

            volumeEntry.Mountpoint = mountPoint;
            return true;
        }

        bool DoUnmount(VolumeEntry volumeEntry)
        {
            try
            {
                //
                // TODO: Change to syscall instead of process create for better diagnostics.
                //
                var unmountParams = this.mountManager.GetUnmountExeAndArgs(volumeEntry);
                TraceWriter.WriteInfoWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"{unmountParams.Exe} {unmountParams.Arguments}");
                using (Process exeProcess = new Process())
                {
                    exeProcess.StartInfo.UseShellExecute = false;
                    exeProcess.StartInfo.RedirectStandardOutput = true;
                    exeProcess.StartInfo.RedirectStandardError = true;
                    exeProcess.StartInfo.FileName = unmountParams.Exe;
                    exeProcess.StartInfo.Arguments = unmountParams.Arguments;
                    exeProcess.Start();
                    if (!exeProcess.WaitForExit(defaultProcessRuntimeMs))
                    {
                        TraceWriter.WriteErrorWithId(
                            TraceType,
                            this.serviceContext.TraceId,
                            $"unmount didnt complete in the required time. Output:{exeProcess.StandardOutput.ReadToEnd()} Error:{exeProcess.StandardError.ReadToEnd()}");
                        exeProcess.Kill();
                        return false;
                    }
                    if (exeProcess.ExitCode != 0)
                    {
                        TraceWriter.WriteErrorWithId(
                            TraceType,
                            this.serviceContext.TraceId,
                            $"Error {exeProcess.ExitCode} encountered during unmount. Output:{exeProcess.StandardOutput.ReadToEnd()} Error:{exeProcess.StandardError.ReadToEnd()}");
                        return false;
                    }
                    else
                    {
                        TraceWriter.WriteInfoWithId(
                           TraceType,
                           this.serviceContext.TraceId,
                           $"unmount complete. Output:{exeProcess.StandardOutput.ReadToEnd()} Error:{exeProcess.StandardError.ReadToEnd()}");
                    }
                }
            }
            catch (Exception e)
            {
                TraceWriter.WriteErrorWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"unmount failed with exception {e}.");
                return false;
            }

            return true;
        }

        //
        // Initialize information about any known volume mounts.
        //
        private void Initialize()
        {
            var knownVolumes = Directory.GetFiles(volumesMetadataPath);

            foreach (var metadataFile in knownVolumes)
            {
                VolumeCreateRequest volume;
                if (ReadMetadataFile(metadataFile, out volume))
                {
                    this.knownVolumeMappings[volume.Name] = new VolumeEntry() { VolumeOptions = volume.Opts};
                }
            }

            var volumeDirectories = Directory.GetDirectories(mountMetadataPath);

            foreach (var volumeDirectory in volumeDirectories)
            {
                // Create a reference to the directory.
                DirectoryInfo volume = new DirectoryInfo(volumeDirectory);
                // Create an array representing the files in the directory.
                FileInfo[] mountIDs = volume.GetFiles();
                foreach (FileInfo mountID in mountIDs)
                {
                    if (this.knownVolumeMappings.ContainsKey(volume.Name))
                    {
                        this.knownVolumeMappings[volume.Name].MountIDs.Add(mountID.Name);

                        if (string.IsNullOrEmpty(this.knownVolumeMappings[volume.Name].Mountpoint))
                        {
                            this.knownVolumeMappings[volume.Name].Mountpoint = Path.Combine(this.mountPointBase, volume.Name);
                        }
                    }
                }
            }

            TraceWriter.WriteInfoWithId(
                TraceType,
                this.serviceContext.TraceId,
                "Volume map initialized");
        }

        private bool ReadMetadataFile(string filename, out VolumeCreateRequest volume)
        {
            volume = null;

            try
            {
                using (StreamReader reader = File.OpenText(filename))
                {
                    JsonSerializer serializer = new JsonSerializer();
                    volume = (VolumeCreateRequest)serializer.Deserialize(reader, typeof(VolumeCreateRequest));
                }
            }
            catch (Exception e)
            {
                TraceWriter.WriteErrorWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"Reading metadata file {filename} failed with exception {e}.");
                return false;
            }

            return true;
        }

        private bool WriteMetadataFile(string filename, VolumeCreateRequest volume)
        {
            try
            {
                using (StreamWriter writer = File.CreateText(filename))
                {
                    JsonSerializer serializer = new JsonSerializer();
                    serializer.Serialize(writer, volume);
                }
            }
            catch (Exception e)
            {
                TraceWriter.WriteErrorWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"Writing metadata file {filename} failed with exception {e}.");
                return false;
            }

            return true;
        }

        #endregion

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                    this.rwLock.Dispose();
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.

                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        // ~VolumeMap() {
        //   // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
        //   Dispose(false);
        // }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            // GC.SuppressFinalize(this);
        }
        #endregion
    }
}
