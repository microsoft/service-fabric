// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace BSDockerVolumePlugin.Models
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
