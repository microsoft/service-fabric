// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class VolumeMapper :
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
    public:
        static Common::AsyncOperationSPtr BeginMapAllVolumes(
            std::wstring const & containerName,
            std::vector<ServiceModel::ContainerVolumeDescription> && volumeDescriptions,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode EndMapAllVolumes(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out std::vector<ServiceModel::ContainerVolumeDescription> & mappedVolumeDescriptions);

        static Common::AsyncOperationSPtr BeginUnmapAllVolumes(
            std::wstring const & containerName,
            std::vector<ServiceModel::ContainerVolumeDescription> && volumeDescriptions,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode EndUnmapAllVolumes(
            Common::AsyncOperationSPtr const & asyncOperation);

    private:
        class PluginCommunication;
        class VolumeCreateRequest;
        class VolumeMountOrUnmountRequest;
        class VolumeMountResponse;
        class VolumeAsyncOperation;
        class CreateVolumeAsyncOperation;
        class MountVolumeAsyncOperation;
        class UnmountVolumeAsyncOperation;
        class MapOrUnmapAllVolumesAsyncOperation;
        class MapAllVolumesAsyncOperation;
        class UnmapAllVolumesAsyncOperation;
        class MapSingleVolumeAsyncOperation;
    };
}
