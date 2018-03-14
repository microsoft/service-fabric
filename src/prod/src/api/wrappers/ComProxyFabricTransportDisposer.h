// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    typedef std::vector<IFabricTransportMessage*> ComFabricTransportMessageRawPtrVector;
    class ComProxyFabricTransportDisposer
    {
        DENY_COPY(ComProxyFabricTransportDisposer);

    public:
        ComProxyFabricTransportDisposer()
        :comImpl_(){

        }

        ComProxyFabricTransportDisposer(Common::ComPointer<IFabricTransportMessageDisposer> const & comImpl)
            :comImpl_(comImpl)
        {
        }

                
        bool ProcessJob(vector<ComPointer<IFabricTransportMessage>> & items,ComponentRoot &)
        {
            ComFabricTransportMessageRawPtrVector rawitems;
			auto size = items.size();
			rawitems.reserve(size);            
            for (uint i = 0; i < size; ++i)
            {
                rawitems.push_back(items[i].GetRawPointer());
            }
            if (size > 0)
            {
                comImpl_->Dispose(static_cast<ULONG>(size),rawitems.data());
            }
            return true;
        }

    private:
        
        Common::ComPointer<IFabricTransportMessageDisposer> comImpl_;
    };
}
