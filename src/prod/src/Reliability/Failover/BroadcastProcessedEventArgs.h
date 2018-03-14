// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class BroadcastProcessedEventArgs : public Common::EventArgs
    {
    public:
        BroadcastProcessedEventArgs(ServiceCuidMap && updated, ServiceCuidMap && removed)
          : updated_(std::move(updated))
          , removed_(std::move(removed))
        {
        }

        __declspec (property(get=get_UpdatedCuids)) ServiceCuidMap const & UpdatedCuids;
        ServiceCuidMap const & get_UpdatedCuids() const { return updated_; }

        __declspec (property(get=get_RemovedCuids)) ServiceCuidMap const & RemovedCuids;
        ServiceCuidMap const & get_RemovedCuids() const { return removed_; }

    private:
        ServiceCuidMap updated_;
        ServiceCuidMap removed_;
    };
}
