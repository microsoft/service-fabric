// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace BSDockerVolumePlugin.Models
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
