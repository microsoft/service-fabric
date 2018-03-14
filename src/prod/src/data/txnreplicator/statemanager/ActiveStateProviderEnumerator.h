// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        class ActiveStateProviderEnumerator :
            public IEnumerator<KeyValuePair<KUri::CSPtr, Metadata::SPtr>>
        {
            K_FORCE_SHARED(ActiveStateProviderEnumerator)

        public:
            static NTSTATUS Create(
                __in IEnumerator<KeyValuePair<KUri::CSPtr, Metadata::SPtr>> & enumerator,
                __in StateProviderFilterMode::FilterMode mode,
                __in KAllocator & allocator,
                __out SPtr & result);

            KeyValuePair<KUri::CSPtr, Metadata::SPtr> Current() override;

            bool MoveNext() override;

        private:
            NOFAIL ActiveStateProviderEnumerator(
                __in IEnumerator<KeyValuePair<KUri::CSPtr, Metadata::SPtr>> & enumerator,
                __in StateProviderFilterMode::FilterMode mode);

        private:
            KSharedPtr<IEnumerator<KeyValuePair<KUri::CSPtr, Metadata::SPtr>>> enumerator_;
            StateProviderFilterMode::FilterMode mode_;
        };
    }
}
