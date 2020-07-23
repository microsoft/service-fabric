//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin.Models
{
    public class VolumeGetResponse : Response
    {
        public VolumeGetResponse(string err)
            : base(err)
        {
        }

        public VolumeGetResponse(VolumeMountDescription volume, string err = "")
            : base(err)
        {
            this.Volume = volume;
        }

        public VolumeMountDescription Volume;
    }
}
