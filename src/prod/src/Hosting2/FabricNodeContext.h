// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class FabricNodeContext
    {    
    public:     
        FabricNodeContext();
        FabricNodeContext(
            std::wstring const & nodeName,
            std::wstring const & nodeId,
            uint64 nodeInstanceId,
            std::wstring const & nodeType,
            std::wstring const & clientConnectionAddress, 
            std::wstring const & runtimeConnectionAddress,
            std::wstring const & deploymentDirectory,
            std::wstring const & ipAddressOrFQDN,
            std::wstring const & nodeWorkFolder,
            std::map<wstring, wstring> const& logicalApplicationDirectories,
            std::map<wstring, wstring> const& logicalNodeDirectories);
        FabricNodeContext(FabricNodeContext const & other);
        FabricNodeContext(FabricNodeContext && other);

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        inline std::wstring const & get_NodeId() const { return nodeId_; };

        __declspec(property(get=get_ClientConnectionAddress)) std::wstring const & ClientConnectionAddress;
        inline std::wstring const & get_ClientConnectionAddress() const { return clientConnectionAddress_; };

        __declspec(property(get=get_RuntimeConntectionAddress)) std::wstring const & RuntimeConnectionAddress;
        inline std::wstring const & get_RuntimeConntectionAddress() const { return runtimeConnectionAddress_; };

        __declspec(property(get=get_DeploymentDirectory)) std::wstring const & DeploymentDirectory;
        inline std::wstring const & get_DeploymentDirectory() const { return deploymentDirectory_; };

        __declspec(property(get=get_NodeInstanceId)) uint64 NodeInstanceId;
        inline uint64 const & get_NodeInstanceId() const { return nodeInstanceId_; };        

        __declspec(property(get=get_NodeName)) std::wstring const& NodeName;
        inline std::wstring const & get_NodeName() const { return nodeName_; };        

        __declspec(property(get=get_NodeType)) std::wstring const& NodeType;
        inline std::wstring const & get_NodeType() const { return nodeType_; };        

        __declspec(property(get=get_IPAddressOrFQDN)) std::wstring const& IPAddressOrFQDN;
        inline std::wstring const & get_IPAddressOrFQDN() const { return ipAddressOrFQDN_; };        

        __declspec(property(get=get_NodeWorkFolder)) std::wstring const& NodeWorkFolder;
        inline std::wstring const & get_NodeWorkFolder() const { return nodeWorkFolder_; };

        __declspec(property(get = get_LogicalApplicationDirectories)) std::map<wstring, wstring> LogicalApplicationDirectories;
        inline std::map<wstring, wstring> const & get_LogicalApplicationDirectories() const { return logicalApplicationDirectories_; };

        __declspec(property(get = get_LogicalNodeDirectories)) std::map<wstring, wstring> LogicalNodeDirectories;
        inline std::map<wstring, wstring> const & get_LogicalNodeDirectories() const { return logicalNodeDirectories_; };

        FabricNodeContext const & operator = (FabricNodeContext const & other);

        FabricNodeContext const & operator = (FabricNodeContext && other);

        bool operator == (FabricNodeContext const & other) const;
        bool operator != (FabricNodeContext const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void Test_SetLogicalApplicationDirectories(std::map<wstring, wstring> && logicalApplicationDirectories);

    private:
        std::wstring nodeId_;
        uint64 nodeInstanceId_;
        std::wstring nodeName_;
        std::wstring nodeType_;
        std::wstring clientConnectionAddress_;
        std::wstring runtimeConnectionAddress_;
        std::wstring deploymentDirectory_;
        std::wstring ipAddressOrFQDN_;
        std::wstring nodeWorkFolder_;
        std::map<wstring, wstring> logicalApplicationDirectories_;
        std::map<wstring, wstring> logicalNodeDirectories_;
    };
}
