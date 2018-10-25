//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

GetImagesReply::GetImagesReply()
    : error_(ErrorCodeValue::Success)
{
}

GetImagesReply::GetImagesReply(std::vector<wstring> const & images, Common::ErrorCode error)
    : images_(images),
    error_(error)
{
}

void GetImagesReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("GetImagesReply { ");
    w.Write("Images {");
    for (auto const & image : images_)
    {
        w.Write(" {0} ", image);
    }
    w.Write("}");
    w.Write("Error = {0}", error_);
    w.Write("}");
}