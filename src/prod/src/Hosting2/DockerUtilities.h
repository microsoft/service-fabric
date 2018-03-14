// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class DockerUtilities
    {
        public:
            static void GetDecodedDockerStream(std::vector<unsigned char> &body, __out wstring & decodedDockerStreamStr);

        private:
            static DWORD GetDwordFromBytes(byte *b)
            {
                return (b[3]) | (b[2] << 8) | (b[1] << 16) | (b[0] << 24);
            }
    };
}
