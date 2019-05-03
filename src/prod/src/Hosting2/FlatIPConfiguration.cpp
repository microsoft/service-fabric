// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const FlatIPConfigurationProvider("FlatIPConfiguration");

FlatIPConfiguration::Subnet::Subnet(wstring subnetCIDR, uint gateway, uint mask) :
    subnetCIDR_(subnetCIDR),
    gatewayAddress_(gateway),
    addressMask_(mask),
    primaryAddress_(INVALID_IP),
    secondaryAddresses_()
{
}

FlatIPConfiguration::Subnet::~Subnet()
{
    this->secondaryAddresses_.clear();
}

bool FlatIPConfiguration::Subnet::AddAddress(uint address, bool primary)
{
    // Need to check for duplicate first.
    //
    if ((find(this->secondaryAddresses_.begin(), this->secondaryAddresses_.end(), address) != this->secondaryAddresses_.end()) ||
        (address == this->primaryAddress_))
    {
        return false;
    }

    if (primary)
    {
        if (this->primaryAddress_ == INVALID_IP)
        {
            this->primaryAddress_ = address;
        }
        else
        {
            return false;
        }
    }
    else
    {
        this->secondaryAddresses_.push_back(address);
    }

    return true;
}

FlatIPConfiguration::IPConfigInterface::IPConfigInterface(bool isPrimary, uint64 macAddress) :
    isPrimary_(isPrimary),
    macAddress_(macAddress),
    subnets_()
{
}

FlatIPConfiguration::IPConfigInterface::~IPConfigInterface()
{
    for (auto & item : this->subnets_)
    {
        delete item;
    }

    this->subnets_.clear();
}

bool FlatIPConfiguration::IPConfigInterface::AddSubnet(Subnet *newSubnet)
{
    // Need to check for duplicate gateway IPs.
    //
    for (auto & item : this->subnets_)
    {
        if (item->GatewayIp == newSubnet->GatewayIp)
        {
            return false;
        }
    }

    this->subnets_.push_back(newSubnet);
    return true;
}

FlatIPConfiguration::FlatIPConfiguration(wstring const & contents) :
    interfaces_()
{
    ComProxyXmlLiteReaderUPtr proxy;

    WriteInfo(
        FlatIPConfigurationProvider,
        "Xml passed in: {0}",
        contents);

#if defined(PLATFORM_UNIX)
    auto xmlUtf8 = StringUtility::Utf16ToUtf8(contents);
    const char *xmlString = xmlUtf8.c_str();
    ThrowIf(ComProxyXmlLiteReader::Create(xmlString, proxy));
#else
    IStream *baseStream = ::SHCreateMemStream((const BYTE *)contents.c_str(), (uint)(contents.size() * sizeof(WCHAR)));
    ThrowIf(ComProxyXmlLiteReader::Create(L"IPConfiguration", baseStream, proxy));
#endif

    ProcessElements(
        proxy,
        [&](ComProxyXmlLiteReaderUPtr &proxy, const wstring &localName)
        {
            if (localName == L"Interfaces")
            {
                ProcessInterfacesElement(proxy);
            }

            return true;
        });
}

FlatIPConfiguration::~FlatIPConfiguration()
{
    for (auto & item : this->interfaces_)
    {
        delete item;
    }

    this->interfaces_.clear();
}

void FlatIPConfiguration::ProcessInterfacesElement(Common::ComProxyXmlLiteReaderUPtr &proxy)
{
    ProcessElements(
        proxy,
        [&](ComProxyXmlLiteReaderUPtr &proxy, const wstring &localName)
        {
            if (localName == L"Interface")
            {
                auto subnetInterface = ProcessInterfaceElement(proxy);
                this->interfaces_.push_back(subnetInterface);
            }

            return true;
        });
}

FlatIPConfiguration::IPConfigInterface *FlatIPConfiguration::ProcessInterfaceElement(Common::ComProxyXmlLiteReaderUPtr &proxy)
{
    bool isPrimary = false;
    uint64 macAddress = 0;

    // First, get the attributes associated with the Interface element
    //
    ProcessAttributes(
        proxy,
        [&](const wstring &attrName, const wstring &attrValue)
        {
            if (attrName == L"IsPrimary")
            {
                return StringUtility::ParseBool::Try(attrValue, isPrimary);
            }
            else if (attrName == L"MacAddress")
            {
                return StringUtility::ParseIntegralString<uint64, false>::Try(attrValue, macAddress, 16U);
            }

            return true;
        });

    // Create the new working item
    IPConfigInterface *subnetInterace = new IPConfigInterface(isPrimary, macAddress);

    ProcessElements(
        proxy,
        [&](ComProxyXmlLiteReaderUPtr &proxy, const wstring &localName)
        {
            if (localName == L"IPSubnet")
            {
                ProcessIPSubnetElement(proxy, subnetInterace);
            }

            return true;
        });

    return subnetInterace;
}

void FlatIPConfiguration::ProcessIPSubnetElement(Common::ComProxyXmlLiteReaderUPtr &proxy, IPConfigInterface *subnetInterace)
{
    uint gatewayIP = INVALID_IP;
    uint mask = 0;
    wstring subnetCIDR = L"";

    // First, get the attributes associated with the Interface element
    //
    ProcessAttributes(
        proxy,
        [&](const wstring &attrName, const wstring &attrValue)
        {
            if (attrName == L"Prefix")
            {
                uint baseIP;
                int maskNum;
                if (!IPHelper::TryParseCIDR(attrValue, baseIP, maskNum))
                {
                    return false;
                }

                subnetCIDR = attrValue;

                // Convert the mask number to a bitmask;
                for (int i = 0; i < maskNum; i++)
                {
                    mask >>= 1;
                    mask |= 0x80000000;
                }

                // apply & calculate the gateway IP.
                gatewayIP = (baseIP & mask) + 1;
            }

            return true;
        });

    auto subnet = new Subnet(subnetCIDR, gatewayIP, mask);
    if (!subnetInterace->AddSubnet(subnet))
    {
        ThrowInvalidContent(
            proxy, 
            wformatString(
                L"Failed adding a new subnet with a gateway IP {0}", 
                IPHelper::Format(gatewayIP)));
    }

    ProcessElements(
        proxy,
        [&](ComProxyXmlLiteReaderUPtr &proxy, const wstring &localName)
        {
            if (localName == L"IPAddress")
            {
                ProcessIPAddressElement(proxy, subnet);
            }

            return true;
        });
}

void FlatIPConfiguration::ProcessIPAddressElement(Common::ComProxyXmlLiteReaderUPtr &proxy, Subnet *subnet)
{
    uint ip = INVALID_IP;
    bool isPrimary = false;

    // First, get the attributes associated with the Interface element
    //
    ProcessAttributes(
        proxy,
        [&](const wstring &attrName, const wstring &attrValue)
        {
            if (attrName == L"IsPrimary")
            {
                return StringUtility::ParseBool::Try(attrValue, isPrimary);
            }
            else if (attrName == L"Address")
            {
                return IPHelper::TryParse(attrValue, ip);
            }

            return true;
        });

    if (ip == INVALID_IP)
    {
        ThrowInvalidContent(proxy, L"No IP identified");
    }

    if (!subnet->AddAddress(ip, isPrimary))
    {
        ThrowInvalidContent(
            proxy, 
            wformatString(
                L"Failed to add an IP address entry: IP={0}, IsPrimary={1}",
                IPHelper::Format(ip),
                isPrimary));
    }

    ProcessElements(
        proxy,
        [&](ComProxyXmlLiteReaderUPtr &proxy, const wstring &localName)
        {
            UNREFERENCED_PARAMETER(proxy);
            UNREFERENCED_PARAMETER(localName);

            return true;
        });
}

void FlatIPConfiguration::ProcessAttributes(
    ComProxyXmlLiteReaderUPtr &proxy, 
    function<bool(const wstring &, const wstring &)> OnAttr)
{
    bool success;

    // First, get the attributes associated with the Interface element
    //
    for (ThrowIf(proxy->MoveToFirstAttribute(success));
         success;
         ThrowIf(proxy->MoveToNextAttribute(success)))
    {
        wstring attrName;
        wstring attrValue;

        ThrowIf(proxy->GetLocalName(attrName));
        ThrowIf(proxy->GetValue(attrValue));

        if (!OnAttr(attrName, attrValue))
        {
            ThrowInvalidContent(
                proxy, 
                wformatString(
                    L"Failed to process attribute: {0} = {1}",
                    attrName,
                    attrValue));
        }
    }

    // Done with the attributes, reposition back on the node.
    proxy->MoveToElement();
}

void FlatIPConfiguration::ProcessElements(
    ComProxyXmlLiteReaderUPtr &proxy,
    function<bool(ComProxyXmlLiteReaderUPtr &, const wstring &)> OnElement)
{
    bool isEof;
    XmlNodeType nodeType;

#if defined(PLATFORM_UNIX)
    // skip check when starting to read since the current node 
    // has not been initialized yet
    uint line = GetLine(proxy);
    uint column = GetColumn(proxy);
    if (proxy->IsEmptyElement() && !(line == 1 && column == 1))
    {
        return;
    }
#else
    if (proxy->IsEmptyElement())
    {
        return;
    }
#endif

    for (ThrowIf(proxy->Read(nodeType, isEof));
        !isEof;
        ThrowIf(proxy->Read(nodeType, isEof)))
    {
        wstring localName;

        switch (nodeType)
        {
        case XmlNodeType_Element:
            ThrowIf(proxy->GetLocalName(localName));

            if (!OnElement(proxy, localName))
            {
                ThrowInvalidContent(
                    proxy,
                    wformatString(
                        L"Failed to process element '{0}'",
                        localName));
            }
            break;

        case XmlNodeType_EndElement:
            return;

        default:
            break;
        }
    }
}

void FlatIPConfiguration::ThrowIf(ErrorCode const & error)
{
    if (!error.IsSuccess())
    {
        throw XmlException(error);
    }
}

void FlatIPConfiguration::ThrowInvalidContent(
    ComProxyXmlLiteReaderUPtr &proxy, 
    wstring const &message)
{
        WriteError(
            FlatIPConfigurationProvider,
            "{0}, at line {1}, column {2}",
            message, 
            (uint32) GetLine(proxy),
            (uint32) GetColumn(proxy));

        throw XmlException(ErrorCode(ErrorCodeValue::XmlInvalidContent));
}

uint FlatIPConfiguration::GetLine(ComProxyXmlLiteReaderUPtr &proxy)
{
    uint line;
    proxy->GetLineNumber(line);

    return line;
}

uint FlatIPConfiguration::GetColumn(ComProxyXmlLiteReaderUPtr &proxy)
{

    uint column;
    proxy->GetLinePosition(column);

    return column;
}
