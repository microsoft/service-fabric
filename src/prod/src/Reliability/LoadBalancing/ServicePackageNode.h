// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once


namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServicePackageNode
        {
            DENY_COPY(ServicePackageNode);

        public:
            ServicePackageNode();
            ServicePackageNode(ServicePackageNode && other);
            ServicePackageNode & operator = (ServicePackageNode && other);

            __declspec(property(get = get_SharedServiceRegularReplicaCount, put=put_SharedServiceRegularReplicaCount)) int SharedServiceRegularReplicaCount;
            int get_SharedServiceRegularReplicaCount() const { return sharedServiceRegularReplicaCount_; }
            void put_SharedServiceRegularReplicaCount(int sharedServiceRegularReplicaCount) {sharedServiceRegularReplicaCount_ = sharedServiceRegularReplicaCount;}

            __declspec(property(get = get_SharedServiceDisappearReplicaCount, put=put_SharedServiceDisappearReplicaCount)) int SharedServiceDisappearReplicaCount;
            int get_SharedServiceDisappearReplicaCount() const { return sharedServiceDisappearReplicaCount_; }
            void put_SharedServiceDisappearReplicaCount(int sharedServiceDisappearReplicaCount) {sharedServiceDisappearReplicaCount_ = sharedServiceDisappearReplicaCount;}

            __declspec(property(get = get_ExclusiveServiceTotalReplicaCount, put=put_ExclusiveServiceTotalReplicaCount)) int ExclusiveServiceTotalReplicaCount;
            int get_ExclusiveServiceTotalReplicaCount() const { return exclusiveServiceTotalReplicaCount_; }
            void put_ExclusiveServiceTotalReplicaCount(int exclusiveServiceTotalReplicaCount) {exclusiveServiceTotalReplicaCount_ = exclusiveServiceTotalReplicaCount;}

            //in case there is any replica of the service package on the node
            //here we account for shared and exclusive, regular and should disappear ones
            bool HasAnyReplica() const;

            //here we account only for shared ones, considering regular and should disappear
            int GetSharedServiceReplicaCount() const;

            void MergeServicePackages(ServicePackageNode const & other);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;
        private:
            int sharedServiceRegularReplicaCount_;
            int sharedServiceDisappearReplicaCount_;
            int exclusiveServiceTotalReplicaCount_;
        };
    }
}
