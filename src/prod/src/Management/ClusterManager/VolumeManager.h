//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class VolumeManager :
            protected Common::TextTraceComponent<Common::TraceTaskCodes::ClusterManager>
        {
        public:
            VolumeManager(
                ClusterManagerReplica const & owner);

            Common::AsyncOperationSPtr BeginCreateVolume(
                VolumeDescriptionSPtr const &,
                ClientRequestSPtr && clientRequest,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & root);
            Common::ErrorCode EndCreateVolume(Common::AsyncOperationSPtr const & operation) { return EndVolumeRequest(operation); }

            Common::AsyncOperationSPtr BeginDeleteVolume(
                std::wstring const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndDeleteVolume(Common::AsyncOperationSPtr const & operation) { return EndVolumeRequest(operation); }

        private:
            class FinishVolumeRequestNoopAsyncOperation;
            class FinishVolumeRequestAsyncOperation;

            Common::ErrorCode EndVolumeRequest(Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr RejectRequest(
                ClientRequestSPtr &&,
                Common::ErrorCode &&,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            ClusterManagerReplica const & owner_;
        };
    }
}
