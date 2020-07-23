//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin.Models
{
    public class VolumeMountResponse : Response
    {
        public VolumeMountResponse(string mountpoint, string err = "")
            : base(err)
        {
            this.Mountpoint = mountpoint;
        }

        public string Mountpoint;
    }
}
