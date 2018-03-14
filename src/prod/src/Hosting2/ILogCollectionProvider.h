// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ILogCollectionProvider : 
        public Common::RootedObject,
        public Common::AsyncFabricComponent
    {
    public:
        ILogCollectionProvider(
            Common::ComponentRoot const & root, 
            std::wstring const & nodePath)
            : Common::RootedObject(root),
            Common::AsyncFabricComponent(),
            nodePath_(nodePath)
        {
        }

        virtual ~ILogCollectionProvider() {}

        __declspec(property(get=get_NodePath)) std::wstring const & NodePath;
        std::wstring const & get_NodePath() const { return this->nodePath_; }
        
        // Add folders from where logs can be collected.
        // All folders should be children of the rootPath.
        virtual Common::AsyncOperationSPtr BeginAddLogPaths(
            std::wstring const & applicationId,
            std::vector<std::wstring> const & paths,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndAddLogPaths(Common::AsyncOperationSPtr const & operation) = 0;

        // Remove log collection from the paths specified inside the context
        virtual Common::AsyncOperationSPtr BeginRemoveLogPaths(
            std::wstring const & rootPath,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndRemoveLogPaths(Common::AsyncOperationSPtr const & operation) = 0;

        virtual void CleanupLogPaths(std::wstring const & rootPath) = 0;
    private:
        std::wstring const nodePath_;
    };
}
