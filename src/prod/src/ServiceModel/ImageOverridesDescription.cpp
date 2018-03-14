// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ImageOverridesDescription::ImageOverridesDescription()
    : Images()
{
}

bool ImageOverridesDescription::operator == (ImageOverridesDescription const & other) const
{
    bool equals = true;
    for (auto i = 0; i < Images.size(); i++)
    {
        equals = Images[i] == other.Images[i];
        if (!equals) { return equals; }
    }
    return equals;
}

bool ImageOverridesDescription::operator != (ImageOverridesDescription const & other) const
{
    return !(*this == other);
}

void ImageOverridesDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are positioned on <ImageOverrides>
    xmlReader->StartElement(
        *SchemaNames::Element_ImageOverrides, 
        *SchemaNames::Namespace);


    if(xmlReader->IsEmptyElement())
    {
        // <ImageOverrides ... />
        xmlReader->ReadElement();
        return;
    }

    // <ImageOverrides ...>
    xmlReader->ReadStartElement();
    bool hasImages = true;
    while(hasImages)
    {
        // <Image ... >
        if(xmlReader->IsStartElement(
            *SchemaNames::Element_Image,
            *SchemaNames::Namespace,
            false))
        {
            ImageTypeDescription description;
            description.ReadFromXml(xmlReader);
            Images.push_back(description);
        }
        else
        {
            hasImages = false;
        }
    }
    
    // </ImageOverrides>
    xmlReader->ReadEndElement();
}

ErrorCode ImageOverridesDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    //<ImageOverrides>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ImageOverrides, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    for (auto i = 0; i < Images.size(); i++)
    {
        er = Images[i].WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    //</ImageOverrides>
    return xmlWriter->WriteEndElement();
}

void ImageOverridesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ImageOverrides { ");
    w.Write("Image = {");
    for (auto const& image : Images)
    {
        w.Write("{0},", image);
    }
    w.Write("}");

    w.Write("}");
}

void ImageOverridesDescription::clear()
{
    this->Images.clear();
}
