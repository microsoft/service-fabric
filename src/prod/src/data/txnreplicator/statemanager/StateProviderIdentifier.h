// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        /// <summary>
        /// State  provider identifier.
        /// </summary>
        struct StateProviderIdentifier
        {
        public:
            StateProviderIdentifier()
                : nameSPtr_(nullptr)
                , stateProviderId_(Constants::InvalidStateProviderId)
            {
            }

            StateProviderIdentifier(
                __in KUri const & name, 
                __in FABRIC_STATE_PROVIDER_ID stateProviderId)
                : nameSPtr_(&name)
                , stateProviderId_(stateProviderId)
            {
            }

            StateProviderIdentifier & operator=(StateProviderIdentifier const & other)
            {
                if (&other == this)
                {
                    return *this;
                }

                nameSPtr_ = other.Name;
                stateProviderId_ = other.StateProviderId;

                return *this;
            }

            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID get_StateProviderId() const
            {
                return stateProviderId_;
            }

            __declspec(property(get = get_Name)) KUri::CSPtr Name;
            KUri::CSPtr get_Name() const
            {
                return nameSPtr_;
            }

        private:
            /// <summary>
            /// Gets or sets state provider name.
            /// </summary>
            KUri::CSPtr nameSPtr_;

            /// <summary>
            /// Gets or sets state provider id.
            /// </summary>
            FABRIC_STATE_PROVIDER_ID stateProviderId_;
        };
    }
}
