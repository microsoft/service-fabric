//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class VolumesHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(VolumesHandler)

    public:
        VolumesHandler(HttpGatewayImpl & server);
        virtual ~VolumesHandler() {}

        Common::ErrorCode Initialize();

    private:
        void GetVolumeApiHandlers(std::vector<HandlerUriTemplate> & handlerUris);

        void PutVolume(Common::AsyncOperationSPtr const & thisSPtr);
        void OnPutVolumeComplete(Common::AsyncOperationSPtr const &, bool);

        void DeleteVolume(Common::AsyncOperationSPtr const & thisSPtr);
        void OnDeleteVolumeComplete(Common::AsyncOperationSPtr const &, bool);

        void GetAllVolumes(Common::AsyncOperationSPtr const &);
        void OnGetAllVolumesComplete(Common::AsyncOperationSPtr const &, bool);

        void GetVolumeByName(Common::AsyncOperationSPtr const &);
        void OnGetVolumeByNameComplete(Common::AsyncOperationSPtr const &, bool);

        Common::ErrorCode GetVolumeDescriptionSeaBreezePrivatePreview2(
            Management::ClusterManager::VolumeDescriptionSPtr & volumeDescriptionSPtr,
            HandlerAsyncOperation * handlerOperation);
    };
}
