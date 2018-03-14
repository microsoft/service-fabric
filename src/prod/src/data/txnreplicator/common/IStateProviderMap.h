// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface IStateProviderMap
    {
        K_SHARED_INTERFACE(IStateProviderMap)

    public:
        /// <summary>
        /// Gets the given state provider registered with the state manager.
        /// </summary>
        /// <returns>
        /// If the state provider exists, return STATUS_SUCCESS and a valid SPtr to the requested ISP2 when get is successful.
        /// If not, return SF_STATUS_NAME_DOES_NOT_EXIST NTSTATUS error code and null SPtr to the reqeusted ISP2.
        /// </returns>
        virtual NTSTATUS Get(
            __in KUriView const & stateProviderName,
            __out IStateProvider2::SPtr & stateProvider) noexcept = 0;

        /// <summary>
        /// Adds a state provider from the state manager.
        /// </summary>
        /// If the state provider exists, return SF_STATUS_NAME_ALREADY_EXISTS NTSTATUS error code.
        /// If not, return STATUS_SUCCESS when add is successful.
        virtual ktl::Awaitable<NTSTATUS> AddAsync(
            __in Transaction & transaction,
            __in KUriView const & stateProviderName,
            __in KStringView const & stateProvider,
            __in_opt Data::Utilities::OperationData const * const initializationParameters = nullptr,
            __in_opt Common::TimeSpan const & timeout = Common::TimeSpan::MaxValue,
            __in_opt ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) noexcept = 0;

        /// <summary>
        /// Removes a state provider from the state manager.
        /// </summary>
        /// If the state provider exists, return STATUS_SUCCESS when remove is successful.
        /// If not, return SF_STATUS_NAME_DOES_NOT_EXIST NTSTATUS error code.
        virtual ktl::Awaitable<NTSTATUS> RemoveAsync(
            __in Transaction & transaction,
            __in KUriView const & stateProviderName,
            __in_opt Common::TimeSpan const & timeout = Common::TimeSpan::MaxValue,
            __in_opt ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) noexcept = 0;

        /// <summary>
        /// Gets state providers registered with the state manager.
        /// </summary>
        virtual NTSTATUS CreateEnumerator(
            __in bool parentsOnly,
            __out Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr & outEnumerator) noexcept = 0;

        /// <summary>
        /// Get the state provider from the state manager if it exists, otherwise add the state provider to
        /// the state manager. Function gets the read lock to check state provider existence. If not exists, release the read lock 
        /// and acquire the write lock to add the state provider. The read is repeatable.
        /// </summary>
        /// <returns>
        /// If the state provider exists, set a valid SPtr to the requested ISP2 when get is successful, return STATUS_SUCCESS
        /// and set out parameter alreadyExist as true.
        /// If not, set added SPtr to the reqeusted ISP2 when add is successful, return STATUS_SUCCESS and set out parameter
        /// alreadyExist as false
        /// If any exception is caught, return a NTSTATUS error code.
        /// </returns>
        virtual ktl::Awaitable<NTSTATUS> GetOrAddAsync(
            __in Transaction & transaction,
            __in KUriView const & stateProviderName,
            __in KStringView const & stateProviderTypeName,
            __out IStateProvider2::SPtr & stateProvider,
            __out bool & alreadyExist,
            __in_opt Data::Utilities::OperationData const * const initializationParameters = nullptr,
            __in_opt Common::TimeSpan const & timeout = Common::TimeSpan::MaxValue,
            __in_opt ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) noexcept = 0;
    };
}
