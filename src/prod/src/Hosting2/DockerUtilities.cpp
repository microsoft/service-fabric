// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

// https://docs.docker.com/engine/api/v1.31/#operation/ContainerAttach
//  Stream format
//      When the TTY setting is disabled in POST / containers / create, the stream over the hijacked connected is 
//      multiplexed to separate out stdout and stderr.The stream consists of a series of frames, each containing a 
//      header and a payload.
//      The header contains the information which the stream writes(stdout or stderr).It also contains the size of the 
//      associated frame encoded in the last four bytes(uint32).
//
//      It is encoded on the first eight bytes like this:
//          header: = [8]byte{ STREAM_TYPE, 0, 0, 0, SIZE1, SIZE2, SIZE3, SIZE4 }
//      STREAM_TYPE can be :
//
//          0 : stdin(is written on stdout)
//          1 : stdout
//          2 : stderr
//          SIZE1, SIZE2, SIZE3, SIZE4 are the four bytes of the uint32 size encoded as big endian.
//          Following the header is the payload, which is the specified number of bytes of STREAM_TYPE.
//          The simplest way to implement this protocol is the following :
//
//      Read 8 bytes.
//          Choose stdout or stderr depending on the first byte.
//          Extract the frame size from the last four bytes.
//          Read the extracted size and output it on the correct output.
//          Goto 1.

void DockerUtilities::GetDecodedDockerStream(std::vector<unsigned char> &body, __out wstring & decodedDockerStreamStr)
{
    if (body.size() > 0)
    {
        int index = 0;
        string str;

        while (index < body.size())
        {
            auto c = reinterpret_cast<CHAR *>(&body[index]);

            if (*c == '\x0' || *c == '\x1' || *c == '\x2' || *c == '\x3')
            {
                DWORD size = DockerUtilities::GetDwordFromBytes(&body[index + 4]);
                string result(reinterpret_cast<CHAR *>(&body[index + 8]), size);
                index = index + 8 + size;
                str += result;
            }
            else
            {
                index++;
            }
        }

        StringUtility::Utf8ToUtf16(str, decodedDockerStreamStr);
    }
}
