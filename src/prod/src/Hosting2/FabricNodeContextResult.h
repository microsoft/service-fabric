// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricNodeContextResult :
       public Common::ComponentRoot
    {
        DENY_COPY(FabricNodeContextResult)

    public:
        FabricNodeContextResult(
            std::wstring const & nodeId,
            uint64 nodeInstanceId,
            std::wstring const &nodeName,
            std::wstring const & nodeType,
            std::wstring const & ipAddressOrFQDN,
            std::map<wstring, wstring> const& logicalNodeDirectories);
        virtual ~FabricNodeContextResult();

         // 
        // IFabricNodeContextResult methods
        // 
        const FABRIC_NODE_CONTEXT *STDMETHODCALLTYPE get_NodeContext( void);

        //
        // IFabricNodeContextResult2 methods
        //
        Common::ErrorCode GetDirectory(std::wstring const& logicalDirectoryName, std::wstring &);

    private:
        FABRIC_NODE_CONTEXT nodeContext_;
        std::wstring nodeId_;
        uint64 nodeInstanceId_;
        std::wstring nodeName_;
        std::wstring nodeType_;
        std::wstring ipAddressOrFQDN_;
        std::map<wstring, wstring> logicalNodeDirectories_;
    };
}
