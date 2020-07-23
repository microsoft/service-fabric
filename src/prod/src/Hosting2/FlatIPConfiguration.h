// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // This class represents the decoded contents from the wireserver's report
    // on the network configuration established by NM agent for this node.
    // 
    // The overall structure of the configuration is:
    //  1..n: interfaces
    //      1..n: subnets
    //          1..n: IPAddresses
    //
    // This structure is reflected in the embedded classes below.
    //
    class FlatIPConfiguration :
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        // This class represents the data associated with a subnet.
        //
        class Subnet
        {
            DENY_COPY(Subnet)

        public:
            Subnet(wstring subnetCIDR, uint gateway, uint mask);
            virtual ~Subnet();

            // Adds an IP address to the subnet.  If there are multiple
            // addresses defined for this node, one will be reserved for use
            // by the node.  The rest can be used by workloads on the node,
            // such as containers or user processes.
            // 
            // Note that duplicate ips are not allowed, and will result in a
            // failure.
            //
            // The arguments are:
            //  address: ip address to add
            //  primary: true, if this ip is reserved for use by the node.
            //
            // Returns:
            //  true, if the address was added; false, if this ip ws to be
            //  a primary, and one already exists, or if the ip has already
            //  been added to this subnet.
            //
            bool AddAddress(uint address, bool primary);

            __declspec(property(get = get_Primary)) uint Primary;
            uint get_Primary() const { return this->primaryAddress_; }

            __declspec(property(get = get_GatewayIp)) uint GatewayIp;
            uint get_GatewayIp() const { return this->gatewayAddress_; }

            __declspec(property(get = get_SubnetCIDR)) wstring SubnetCIDR;
            wstring get_SubnetCIDR() const { return this->subnetCIDR_; }

            __declspec(property(get = get_Mask)) uint Mask;
            uint get_Mask() const { return this->addressMask_; }

            __declspec(property(get = get_SecondaryAddresses)) std::list<uint> SecondaryAddresses;
            std::list<uint> get_SecondaryAddresses() { return this->secondaryAddresses_; }

        private:
            // Holds the currently reserved ip address, or INVALID_IP if none
            // yet determined.
            //
            uint primaryAddress_;

            // Holds the ip address of the gateway for this subnet.  This is
            // always at the 'x.x.x.1' address determined by the subnet base
            // address and mask.
            //
            uint gatewayAddress_;

            // Holds the subnet information in CIDR format
            wstring subnetCIDR_;

            // Holds the mask for this subnet as a bitmask: e.g. the CIDR
            // notation /24 is converted into 0xFFFFFF00
            //
            uint addressMask_;

            // Contains the set of addresses available for assignment to user
            // workloads.
            //
            std::list<uint> secondaryAddresses_;
        };

        // This class represents the data associated with an interface.
        //
        class IPConfigInterface
        {
            DENY_COPY(IPConfigInterface)

        public:
            IPConfigInterface(bool isPrimary, uint64 macAddress);
            virtual ~IPConfigInterface();

            // Adds a subnet to the interface.  An attempt to add a subnet that
            // has the same gateway address as a subnet already in the
            // collection will fail.
            // 
            // The arguments are:
            //  newSubnet: a pointer to a subnet to add.  Note that if the add
            //             is successful, it is the responsibility of the
            //             containing interface object to delete it.
            // 
            // Returns:
            //  true, if the subnet was added; false, if it was not.
            //
            bool AddSubnet(Subnet *newSubnet);

            __declspec(property(get = get_Primary)) bool Primary;
            bool get_Primary() const { return this->isPrimary_; }

            __declspec(property(get = get_MacAddress)) uint64 MacAddress;
            uint64 get_MacAddress() const { return this->macAddress_; };

            __declspec(property(get = get_Subnets)) std::list<Subnet *> &Subnets;
            std::list<Subnet *> &get_Subnets() { return this->subnets_; };

        private:
            // Contains the flag indicating if this is the designated primary
            // interface for the node.
            //
            bool isPrimary_;

            // Holds the MAC address for this interface, if provided, or zero.
            //
            uint64 macAddress_;

            // Holds the set of subnets configured for this interface.
            //
            std::list<Subnet *> subnets_;
        };

        DENY_COPY(FlatIPConfiguration)

    public:
        FlatIPConfiguration(std::wstring const &contents);
        virtual ~FlatIPConfiguration();

        __declspec(property(get = get_Interfaces)) std::list<IPConfigInterface *> &Interfaces;
        std::list<IPConfigInterface *> &get_Interfaces() { return this->interfaces_; }

        static const uint INVALID_IP = 0xFFFFFFFF;

    private:
        // These are the helper classes that decode the XML elements that are
        // in the document from NM Agent.  Each method is responsible for a single
        // element, and calling through to others that handle further nested
        // elements.
        //
        // All take a reference to the common xml reader.  The more deeply
        // nested element methods also take a pointer to the enclosing object,
        // allowing them to add their element directly.
        //
        void ProcessInterfacesElement(Common::ComProxyXmlLiteReaderUPtr &proxy);
        IPConfigInterface *ProcessInterfaceElement(Common::ComProxyXmlLiteReaderUPtr &proxy);
        void ProcessIPSubnetElement(Common::ComProxyXmlLiteReaderUPtr &proxy, IPConfigInterface *subnetInterace);
        void ProcessIPAddressElement(Common::ComProxyXmlLiteReaderUPtr &proxy, Subnet *subnet);

        // These are two base helpers that process the sets of attributes and
        // the sets of contained elements, respectively.
        //
        void ProcessAttributes(Common::ComProxyXmlLiteReaderUPtr &proxy, std::function<bool(const std::wstring &, const std::wstring &)> OnAttr);
        void ProcessElements(Common::ComProxyXmlLiteReaderUPtr &proxy, std::function<bool(Common::ComProxyXmlLiteReaderUPtr &, const std::wstring &)> OnElement);

        // Helper methods that throw an XmlException if the supplied error code
        // is not success.
        //
        void ThrowIf(Common::ErrorCode const & error);
        void ThrowInvalidContent(Common::ComProxyXmlLiteReaderUPtr &proxy, std::wstring const &message);
        uint GetLine(Common::ComProxyXmlLiteReaderUPtr &proxy);
        uint GetColumn(Common::ComProxyXmlLiteReaderUPtr &proxy);

        // Holds the set of interfaces defined for this node's configuration.
        //
        std::list<IPConfigInterface *> interfaces_;
    };
}
