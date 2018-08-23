//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class DeleteVolumeMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        DeleteVolumeMessageBody() = default;

        DeleteVolumeMessageBody(
            std::wstring const & volumeName)
            : volumeName_(volumeName)
        {
        }

        __declspec(property(get = get_VolumeName)) std::wstring const & VolumeName;
        std::wstring const & get_VolumeName() const { return volumeName_; }

        FABRIC_FIELDS_01(volumeName_);

    private:
        std::wstring volumeName_;
    };
}
