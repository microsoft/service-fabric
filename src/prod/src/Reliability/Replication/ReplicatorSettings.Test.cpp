// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
 #include "ComTestStatefulServicePartition.h"
#include "ComTestStateProvider.h"
#include "ComProxyTestReplicator.h"
#include "ComReplicatorFactory.h"
#include "ComReplicator.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;
    

    static StringLiteral const PublicTestSource("TESTReplicatorSettings");

    class TestReplicatorSettings
    {
    protected:
        TestReplicatorSettings() { BOOST_REQUIRE(Setup()); }
        TEST_CLASS_SETUP(Setup)

        class DummyRoot : public ComponentRoot
        {
        public:
            ~DummyRoot() {}
        };

        static ComponentRoot const & GetRoot();

        static void CreateReplicator(
            ComReplicatorFactory & factory,
            __in_opt FABRIC_REPLICATOR_SETTINGS *settings,
            __out ComPointer<IFabricStateReplicator> & replicator)
        {
            return CreateReplicator(factory, settings, S_OK, replicator);
        }

        static void CreateReplicator(
            ComReplicatorFactory & factory,
            __in_opt FABRIC_REPLICATOR_SETTINGS *settings,
            HRESULT expectedHr,
            __out ComPointer<IFabricStateReplicator> & replicator)
        {
            VERIFY_IS_TRUE(factory.Open(L"0").IsSuccess(), L"ComReplicatorFactory opened");
            Guid partitionId = Guid::NewGuid();
            IReplicatorHealthClientSPtr temp;
            ComTestStatefulServicePartition * partition = new ComTestStatefulServicePartition(partitionId);
            auto hr = factory.CreateReplicator(
                0LL,
                partition,
                new ComTestStateProvider(0, -1, TestReplicatorSettings::GetRoot()),
                settings,
                true,
                move(temp),
                replicator.InitializationAddress());

            VERIFY_ARE_EQUAL(expectedHr, hr, L"Create replicator");

            if (FAILED(hr))
            {
                return;
            }

            IFabricReplicator *replicatorObj;
            VERIFY_SUCCEEDED(
                replicator->QueryInterface(
                IID_IFabricReplicator,
                reinterpret_cast<void**>(&replicatorObj)),
                L"Query for IFabricReplicator");

            IFabricStateReplicator *replicatorStateObj;
            VERIFY_SUCCEEDED(
                replicator->QueryInterface(
                IID_IFabricStateReplicator2,
                reinterpret_cast<void**>(&replicatorStateObj)),
                L"Query for IFabricStateReplicator2");
        }

        void ResetSettingsStructures()
        {
            reSettings_.Flags = 0;
            reSettings_.Reserved = &reSettingsEx1_;
            reSettingsEx1_.Reserved = &reSettingsEx2_;
            reSettingsEx2_.Reserved = &reSettingsEx3_;
            reSettingsEx3_.Reserved = &reSettingsEx4_;
            reSettingsEx4_.Reserved = nullptr;
        }

        void UpdateConfig(wstring name, wstring newvalue, ConfigSettings & settings, shared_ptr<ConfigSettingsConfigStore> & store)
        {
            ConfigSection section;
            section.Name = L"Replication";

            ConfigParameter d1;
            d1.Name = name;
            d1.Value = newvalue;

            section.Parameters[d1.Name] = d1;
            settings.Sections[section.Name] = section;

            store->Update(settings);
        }

        void VerifySettingsForFlags(DWORD flags, FABRIC_REPLICATOR_SETTINGS const * actual, FABRIC_REPLICATOR_SETTINGS const * expected)
        {
            FABRIC_REPLICATOR_SETTINGS_EX1 const * actualEx1 = (FABRIC_REPLICATOR_SETTINGS_EX1*)actual->Reserved;
            FABRIC_REPLICATOR_SETTINGS_EX2 const * actualEx2 = (FABRIC_REPLICATOR_SETTINGS_EX2*)actualEx1->Reserved;
            FABRIC_REPLICATOR_SETTINGS_EX3 const * actualEx3 = (FABRIC_REPLICATOR_SETTINGS_EX3*)actualEx2->Reserved;
            FABRIC_REPLICATOR_SETTINGS_EX4 const * actualEx4 = (FABRIC_REPLICATOR_SETTINGS_EX4*)actualEx3->Reserved;

            FABRIC_REPLICATOR_SETTINGS_EX1 const * expectedEx1 = (FABRIC_REPLICATOR_SETTINGS_EX1*)expected->Reserved;
            FABRIC_REPLICATOR_SETTINGS_EX2 const * expectedEx2 = (FABRIC_REPLICATOR_SETTINGS_EX2*)expectedEx1->Reserved;
            FABRIC_REPLICATOR_SETTINGS_EX3 const * expectedEx3 = (FABRIC_REPLICATOR_SETTINGS_EX3*)expectedEx2->Reserved;
            FABRIC_REPLICATOR_SETTINGS_EX4 const * expectedEx4 = (FABRIC_REPLICATOR_SETTINGS_EX4*)expectedEx3->Reserved;

            // Do not compare flags values intentionally in the 2 structures
            if ((flags & FABRIC_REPLICATOR_ADDRESS) != 0)
            {
                wstring actualStr(actual->ReplicatorAddress);
                wstring expectedStr(expected->ReplicatorAddress);
                VERIFY_ARE_EQUAL(expectedStr, actualStr, L"ReplicatorAddress");
                // The address is specified
            }

            if ((flags & FABRIC_REPLICATOR_RETRY_INTERVAL) != 0)
            {
                VERIFY_ARE_EQUAL(expected->RetryIntervalMilliseconds, actual->RetryIntervalMilliseconds, L"RetryIntervalMilliseconds");
            }

            if ((flags & FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL) != 0)
            {
                VERIFY_ARE_EQUAL(expected->BatchAcknowledgementIntervalMilliseconds, actual->BatchAcknowledgementIntervalMilliseconds, L"BatchAcknowledgementIntervalMilliseconds");
            }

            if ((flags & FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK) != 0)
            {
                VERIFY_ARE_EQUAL(expected->RequireServiceAck, actual->RequireServiceAck, L"RequireServiceAck");
            }

            if ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE) != 0)
            {
                VERIFY_ARE_EQUAL(expected->InitialReplicationQueueSize, actual->InitialReplicationQueueSize, L"InitialReplicationQueueSize");

                bool checkPrimary = true;
                bool checkSecondary = true;

                if ((flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0)
                {
                    checkPrimary = false; // since this flag overrides the value
                }

                if ((flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0)
                {
                    checkSecondary = false; // since this flag overrides the value
                }

                if (checkPrimary)
                    VERIFY_ARE_EQUAL(expected->InitialReplicationQueueSize, actualEx3->InitialPrimaryReplicationQueueSize, L"InitialReplicationQueueSize");
                if (checkSecondary)
                    VERIFY_ARE_EQUAL(expected->InitialReplicationQueueSize, actualEx3->InitialSecondaryReplicationQueueSize, L"InitialReplicationQueueSize");
            }
            if ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE) != 0)
            {
                VERIFY_ARE_EQUAL(expected->MaxReplicationQueueSize, actual->MaxReplicationQueueSize, L"MaxReplicationQueueSize");

                bool checkPrimary = true;
                bool checkSecondary = true;

                if ((flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE) != 0)
                {
                    checkPrimary = false; // since this flag overrides the value
                }

                if ((flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE) != 0)
                {
                    checkSecondary = false; // since this flag overrides the value
                }

                if (checkPrimary)
                    VERIFY_ARE_EQUAL(expected->MaxReplicationQueueSize, actualEx3->MaxPrimaryReplicationQueueSize, L"MaxReplicationQueueSize");
                if (checkSecondary)
                    VERIFY_ARE_EQUAL(expected->MaxReplicationQueueSize, actualEx3->MaxSecondaryReplicationQueueSize, L"MaxReplicationQueueSize");
            }
            if ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
            {
                VERIFY_ARE_EQUAL(expectedEx1->MaxReplicationQueueMemorySize, actualEx1->MaxReplicationQueueMemorySize, L"MaxReplicationQueueMemorySize");

                bool checkPrimary = true;
                bool checkSecondary = true;

                if ((flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
                {
                    checkPrimary = false; // since this flag overrides the value
                }

                if ((flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
                {
                    checkSecondary = false; // since this flag overrides the value
                }

                if (checkPrimary)
                    VERIFY_ARE_EQUAL(expectedEx1->MaxReplicationQueueMemorySize, actualEx3->MaxPrimaryReplicationQueueMemorySize, L"MaxReplicationQueueMemorySize");
                if (checkSecondary)
                    VERIFY_ARE_EQUAL(expectedEx1->MaxReplicationQueueMemorySize, actualEx3->MaxSecondaryReplicationQueueMemorySize, L"MaxReplicationQueueMemorySize");
            }
            if ((flags & FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE) != 0)
            {
                VERIFY_ARE_EQUAL(expected->InitialCopyQueueSize, actual->InitialCopyQueueSize, L"InitialCopyQueueSize");
            }
            if ((flags & FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE) != 0)
            {
                VERIFY_ARE_EQUAL(expected->MaxCopyQueueSize, actual->MaxCopyQueueSize, L"MaxCopyQueueSize");
            }
            if ((flags & FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS) != 0)
            {
                VERIFY_ARE_EQUAL(expectedEx1->SecondaryClearAcknowledgedOperations, actualEx1->SecondaryClearAcknowledgedOperations, L"SecondaryClearAcknowledgedOperations");
            }
            if ((flags & FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE) != 0)
            {
                VERIFY_ARE_EQUAL(expectedEx1->MaxReplicationMessageSize, actualEx1->MaxReplicationMessageSize, L"MaxReplicationMessageSize");
            }
            if ((flags & FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK) != 0)
            {
                VERIFY_ARE_EQUAL(expectedEx2->UseStreamFaultsAndEndOfStreamOperationAck, actualEx2->UseStreamFaultsAndEndOfStreamOperationAck, L"UseStreamFaultsAndEndOfStreamOperationAck");
            }
            if ((flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0 ||
                (flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE) != 0 ||
                (flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0 ||
                (flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0 ||
                (flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE) != 0 ||
                (flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
            {
                if ((flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0)
                {
                    VERIFY_ARE_EQUAL(expectedEx3->InitialSecondaryReplicationQueueSize, actualEx3->InitialSecondaryReplicationQueueSize, L"InitialSecondaryReplicationQueueSize");
                }
                if ((flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0)
                {
                    VERIFY_ARE_EQUAL(expectedEx3->InitialPrimaryReplicationQueueSize, actualEx3->InitialPrimaryReplicationQueueSize, L"InitialPrimaryReplicationQueueSize");
                }
                if ((flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE) != 0)
                {
                    VERIFY_ARE_EQUAL(expectedEx3->MaxSecondaryReplicationQueueSize, actualEx3->MaxSecondaryReplicationQueueSize, L"MaxSecondaryReplicationQueueSize");
                }
                if ((flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
                {
                    VERIFY_ARE_EQUAL(expectedEx3->MaxSecondaryReplicationQueueMemorySize, actualEx3->MaxSecondaryReplicationQueueMemorySize, L"MaxSecondaryReplicationQueueMemorySize");
                }
                if ((flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE) != 0)
                {
                    VERIFY_ARE_EQUAL(expectedEx3->MaxPrimaryReplicationQueueSize, actualEx3->MaxPrimaryReplicationQueueSize, L"MaxPrimaryReplicationQueueSize");
                }
                if ((flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
                {
                    VERIFY_ARE_EQUAL(expectedEx3->MaxPrimaryReplicationQueueMemorySize, actualEx3->MaxPrimaryReplicationQueueMemorySize, L"MaxPrimaryReplicationQueueMemorySize");
                }
            }
            if ((flags & FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT) != 0)
            {
                VERIFY_ARE_EQUAL(expectedEx3->PrimaryWaitForPendingQuorumsTimeoutMilliseconds, actualEx3->PrimaryWaitForPendingQuorumsTimeoutMilliseconds, L"PrimaryWaitForPendingQuorumsTimeoutMilliseconds");
            }
            if ((flags & FABRIC_REPLICATOR_LISTEN_ADDRESS) != 0 ||
                (flags & FABRIC_REPLICATOR_PUBLISH_ADDRESS) != 0)
            {
                if ((flags & FABRIC_REPLICATOR_LISTEN_ADDRESS) != 0)
                {
                    wstring actualStr(actualEx4->ReplicatorListenAddress);
                    wstring expectedStr(expectedEx4->ReplicatorListenAddress);
                    VERIFY_ARE_EQUAL(expectedStr, actualStr, L"ReplicatorListenAddress");
                }
                if ((flags & FABRIC_REPLICATOR_PUBLISH_ADDRESS) != 0)
                {
                    wstring actualStr(actualEx4->ReplicatorPublishAddress);
                    wstring expectedStr(expectedEx4->ReplicatorPublishAddress);
                    VERIFY_ARE_EQUAL(expectedStr, actualStr, L"ReplicatorPublishAddress");
                }
            }
        }

        FABRIC_REPLICATOR_SETTINGS reSettings_;
        FABRIC_REPLICATOR_SETTINGS_EX1 reSettingsEx1_;
        FABRIC_REPLICATOR_SETTINGS_EX2 reSettingsEx2_;
        FABRIC_REPLICATOR_SETTINGS_EX3 reSettingsEx3_;
        FABRIC_REPLICATOR_SETTINGS_EX4 reSettingsEx4_;
    };
    


    BOOST_FIXTURE_TEST_SUITE(TestReplicatorSettingsSuite,TestReplicatorSettings)

    BOOST_AUTO_TEST_CASE(TestOldQueueConfigsUsage)
    {
        ComReplicatorFactory factory(TestReplicatorSettings::GetRoot());
        ComPointer<IFabricStateReplicator> replicator;
        ResetSettingsStructures();

        DWORD initialQueueSize = 4096;
        DWORD maxQueueSize = 8192;
        DWORD maxQueueMemorySize = 123*1024*1024;

        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettings_.InitialReplicationQueueSize = initialQueueSize;
        reSettings_.MaxReplicationQueueSize = maxQueueSize;
        reSettingsEx1_.MaxReplicationQueueMemorySize = maxQueueMemorySize;

        TestReplicatorSettings::CreateReplicator(factory, &reSettings_, replicator);

        ComPointer<IFabricStateReplicator2> replicator2;
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;

        replicator->QueryInterface(IID_IFabricStateReplicator2, reinterpret_cast<void**>(&replicator2));
        replicator2->GetReplicatorSettings(settingsResult.InitializationAddress());

        FABRIC_REPLICATOR_SETTINGS const * outputResult = settingsResult->get_ReplicatorSettings();
        VerifySettingsForFlags(reSettings_.Flags, outputResult, &reSettings_);
        factory.Close();
    }

    BOOST_AUTO_TEST_CASE(TestNewPrimaryQueueConfigsUsage)
    {
        ComReplicatorFactory factory(TestReplicatorSettings::GetRoot());
        ComPointer<IFabricStateReplicator> replicator;
        ResetSettingsStructures();

        DWORD initialQueueSize = 512;
        DWORD maxQueueSize = 2048;
        DWORD maxQueueMemorySize = 126*1024*1024;

        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettingsEx3_.InitialPrimaryReplicationQueueSize = initialQueueSize;
        reSettingsEx3_.MaxPrimaryReplicationQueueSize = maxQueueSize;
        reSettingsEx3_.MaxPrimaryReplicationQueueMemorySize = maxQueueMemorySize;

        TestReplicatorSettings::CreateReplicator(factory, &reSettings_, replicator);

        ComPointer<IFabricStateReplicator2> replicator2;
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;

        replicator->QueryInterface(IID_IFabricStateReplicator2, reinterpret_cast<void**>(&replicator2));
        replicator2->GetReplicatorSettings(settingsResult.InitializationAddress());

        FABRIC_REPLICATOR_SETTINGS const * outputResult = settingsResult->get_ReplicatorSettings();
        VerifySettingsForFlags(reSettings_.Flags, outputResult, &reSettings_);
        factory.Close();
    }

    BOOST_AUTO_TEST_CASE(TestNewSecondaryQueueConfigsUsage)
    {
        ComReplicatorFactory factory(TestReplicatorSettings::GetRoot());
        ComPointer<IFabricStateReplicator> replicator;
        ResetSettingsStructures();

        DWORD initialQueueSize = 32;
        DWORD maxQueueSize = 64;
        DWORD maxQueueMemorySize = 626*1024*1024;

        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettingsEx3_.InitialSecondaryReplicationQueueSize = initialQueueSize;
        reSettingsEx3_.MaxSecondaryReplicationQueueSize = maxQueueSize;
        reSettingsEx3_.MaxSecondaryReplicationQueueMemorySize = maxQueueMemorySize;

        TestReplicatorSettings::CreateReplicator(factory, &reSettings_, replicator);

        ComPointer<IFabricStateReplicator2> replicator2;
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;

        replicator->QueryInterface(IID_IFabricStateReplicator2, reinterpret_cast<void**>(&replicator2));
        replicator2->GetReplicatorSettings(settingsResult.InitializationAddress());

        FABRIC_REPLICATOR_SETTINGS const * outputResult = settingsResult->get_ReplicatorSettings();
        VerifySettingsForFlags(reSettings_.Flags, outputResult, &reSettings_);
        factory.Close();
    }

    BOOST_AUTO_TEST_CASE(TestNewPrimaryAndSecondaryQueueConfigsUsage)
    {
        ComReplicatorFactory factory(TestReplicatorSettings::GetRoot());
        ComPointer<IFabricStateReplicator> replicator;
        ResetSettingsStructures();

        DWORD initialQueueSize = 8;
        DWORD maxQueueSize = 32;
        DWORD maxQueueMemorySize = 63*1024*1024;

        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettingsEx3_.InitialSecondaryReplicationQueueSize = initialQueueSize*2;
        reSettingsEx3_.MaxSecondaryReplicationQueueSize = maxQueueSize*2;
        reSettingsEx3_.MaxSecondaryReplicationQueueMemorySize = maxQueueMemorySize*2;

        reSettingsEx3_.InitialPrimaryReplicationQueueSize = initialQueueSize;
        reSettingsEx3_.MaxPrimaryReplicationQueueSize = maxQueueSize;
        reSettingsEx3_.MaxPrimaryReplicationQueueMemorySize = maxQueueMemorySize;

        TestReplicatorSettings::CreateReplicator(factory, &reSettings_, replicator);

        ComPointer<IFabricStateReplicator2> replicator2;
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;

        replicator->QueryInterface(IID_IFabricStateReplicator2, reinterpret_cast<void**>(&replicator2));
        replicator2->GetReplicatorSettings(settingsResult.InitializationAddress());

        FABRIC_REPLICATOR_SETTINGS const * outputResult = settingsResult->get_ReplicatorSettings();
        VerifySettingsForFlags(reSettings_.Flags, outputResult, &reSettings_);
        factory.Close();
    }

    BOOST_AUTO_TEST_CASE(TestPrimaryQueueConfigsUsageWithConflicts)
    {
        ComReplicatorFactory factory(TestReplicatorSettings::GetRoot());
        ComPointer<IFabricStateReplicator> replicator;
        ResetSettingsStructures();

        DWORD initialQueueSize = 8;
        DWORD maxQueueSize = 32;
        DWORD maxQueueMemorySize = 63*1024*1024;

        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettingsEx3_.InitialPrimaryReplicationQueueSize = initialQueueSize;
        reSettingsEx3_.MaxPrimaryReplicationQueueSize = maxQueueSize;
        reSettingsEx3_.MaxPrimaryReplicationQueueMemorySize = maxQueueMemorySize;

        reSettings_.InitialReplicationQueueSize = 128;
        reSettings_.MaxReplicationQueueSize = 256;
        reSettingsEx1_.MaxReplicationQueueMemorySize = maxQueueMemorySize*2;

        TestReplicatorSettings::CreateReplicator(factory, &reSettings_, replicator);

        ComPointer<IFabricStateReplicator2> replicator2;
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;

        replicator->QueryInterface(IID_IFabricStateReplicator2, reinterpret_cast<void**>(&replicator2));
        replicator2->GetReplicatorSettings(settingsResult.InitializationAddress());

        FABRIC_REPLICATOR_SETTINGS const * outputResult = settingsResult->get_ReplicatorSettings();
        VerifySettingsForFlags(reSettings_.Flags, outputResult, &reSettings_);
        factory.Close();
    }

    BOOST_AUTO_TEST_CASE(TestSecondaryQueueConfigsUsageWithConflicts)
    {
        ComReplicatorFactory factory(TestReplicatorSettings::GetRoot());
        ComPointer<IFabricStateReplicator> replicator;
        ResetSettingsStructures();

        DWORD initialQueueSize = 32;
        DWORD maxQueueSize = 64;
        DWORD maxQueueMemorySize = 626*1024*1024;

        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettingsEx3_.InitialSecondaryReplicationQueueSize = initialQueueSize;
        reSettingsEx3_.MaxSecondaryReplicationQueueSize = maxQueueSize;
        reSettingsEx3_.MaxSecondaryReplicationQueueMemorySize = maxQueueMemorySize;

        reSettings_.InitialReplicationQueueSize = 128;
        reSettings_.MaxReplicationQueueSize = 256;
        reSettingsEx1_.MaxReplicationQueueMemorySize = maxQueueMemorySize*2;

        TestReplicatorSettings::CreateReplicator(factory, &reSettings_, replicator);

        ComPointer<IFabricStateReplicator2> replicator2;
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;

        replicator->QueryInterface(IID_IFabricStateReplicator2, reinterpret_cast<void**>(&replicator2));
        replicator2->GetReplicatorSettings(settingsResult.InitializationAddress());

        FABRIC_REPLICATOR_SETTINGS const * outputResult = settingsResult->get_ReplicatorSettings();
        VerifySettingsForFlags(reSettings_.Flags, outputResult, &reSettings_);
        factory.Close();
    }

    BOOST_AUTO_TEST_CASE(TestReplicatorListenAndPublishAddressIsDefaultedToOldReplicatorAddress)
    {
        ComReplicatorFactory factory(TestReplicatorSettings::GetRoot());
        ComPointer<IFabricStateReplicator> replicator;
        ResetSettingsStructures();

        reSettings_.Flags |= FABRIC_REPLICATOR_ADDRESS;
        reSettings_.ReplicatorAddress = L"localhost:10";

        TestReplicatorSettings::CreateReplicator(factory, &reSettings_, replicator);

        // Set this after calling create replicator
        reSettings_.Flags |= FABRIC_REPLICATOR_LISTEN_ADDRESS;
        reSettings_.Flags |= FABRIC_REPLICATOR_PUBLISH_ADDRESS;
        reSettingsEx4_.ReplicatorPublishAddress = L"localhost:10";
        reSettingsEx4_.ReplicatorListenAddress = L"localhost:10";

        ComPointer<IFabricStateReplicator2> replicator2;
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;

        replicator->QueryInterface(IID_IFabricStateReplicator2, reinterpret_cast<void**>(&replicator2));
        replicator2->GetReplicatorSettings(settingsResult.InitializationAddress());

        FABRIC_REPLICATOR_SETTINGS const * outputResult = settingsResult->get_ReplicatorSettings();
        VerifySettingsForFlags(reSettings_.Flags, outputResult, &reSettings_);
        factory.Close();
    }

    BOOST_AUTO_TEST_CASE(TestReplicatorListenAndPublishAddress)
    {
        ComReplicatorFactory factory(TestReplicatorSettings::GetRoot());
        ComPointer<IFabricStateReplicator> replicator;
        ResetSettingsStructures();

        reSettings_.Flags |= FABRIC_REPLICATOR_LISTEN_ADDRESS;
        reSettings_.Flags |= FABRIC_REPLICATOR_PUBLISH_ADDRESS;
        reSettings_.Flags |= FABRIC_REPLICATOR_ADDRESS;
        reSettings_.ReplicatorAddress = L"localhost:10";
        reSettingsEx4_.ReplicatorPublishAddress = L"localhost:11";
        reSettingsEx4_.ReplicatorListenAddress = L"localhost:12";

        TestReplicatorSettings::CreateReplicator(factory, &reSettings_, replicator);

        ComPointer<IFabricStateReplicator2> replicator2;
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;

        replicator->QueryInterface(IID_IFabricStateReplicator2, reinterpret_cast<void**>(&replicator2));
        replicator2->GetReplicatorSettings(settingsResult.InitializationAddress());

        FABRIC_REPLICATOR_SETTINGS const * outputResult = settingsResult->get_ReplicatorSettings();
        VerifySettingsForFlags(reSettings_.Flags, outputResult, &reSettings_);
        factory.Close();
    }

// Cannot enable this test as there is no way to set the replicator address cleanly as part of REConfig
/*
    BOOST_AUTO_TEST_CASE(TestReplicatorListenAndPublishAddressWhenRESettingsIsNull)
    {
        //
        // WORK AROUND ALERT - Config subsystem replaces "0" in ReplicatorAddress if provided in cluster manifest to "IP:Port".
        // The Listen and Publish Address must be initialized to this to ensure customers using "0" are not broken if the default values are specified for the listen and publish addresses
        // 
        // Disable this workaround once the config subsystem logic moves into deployer
        //
        // RD: RDBug 10589649: Config subsystem loads ipaddress:port if config name ends with "Address" - Move this logic into deployer
        //

        wstring replicatorAddress = L"localhost:10123"; // This is set in the Replication.Test.exe.cfg

        ComReplicatorFactory factory(TestReplicatorSettings::GetRoot());
        ComPointer<IFabricStateReplicator> replicator;
        ResetSettingsStructures();

        TestReplicatorSettings::CreateReplicator(factory, nullptr, replicator);

        ComPointer<IFabricStateReplicator2> replicator2;
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;

        replicator->QueryInterface(IID_IFabricStateReplicator2, reinterpret_cast<void**>(&replicator2));
        replicator2->GetReplicatorSettings(settingsResult.InitializationAddress());

        FABRIC_REPLICATOR_SETTINGS const * actual = settingsResult->get_ReplicatorSettings();
        FABRIC_REPLICATOR_SETTINGS_EX1 const * actualEx1 = (FABRIC_REPLICATOR_SETTINGS_EX1*)actual->Reserved;
        FABRIC_REPLICATOR_SETTINGS_EX2 const * actualEx2 = (FABRIC_REPLICATOR_SETTINGS_EX2*)actualEx1->Reserved;
        FABRIC_REPLICATOR_SETTINGS_EX3 const * actualEx3 = (FABRIC_REPLICATOR_SETTINGS_EX3*)actualEx2->Reserved;
        FABRIC_REPLICATOR_SETTINGS_EX4 const * actualEx4 = (FABRIC_REPLICATOR_SETTINGS_EX4*)actualEx3->Reserved;

        wstring actualListenAddress(actualEx4->ReplicatorListenAddress);
        wstring actualPublishAddress(actualEx4->ReplicatorPublishAddress);

        VERIFY_ARE_EQUAL(replicatorAddress, actualListenAddress, L"ReplicatorListenAddress");
        VERIFY_ARE_EQUAL(replicatorAddress, actualPublishAddress, L"ReplicatorPublishAddress");

        factory.Close();
    }
*/

    BOOST_AUTO_TEST_CASE(TestNewPrimaryAndSecondaryQueueConfigsUsageWithConflicts)
    {
        ComReplicatorFactory factory(TestReplicatorSettings::GetRoot());
        ComPointer<IFabricStateReplicator> replicator;
        ResetSettingsStructures();

        DWORD initialQueueSize = 8;
        DWORD maxQueueSize = 32;
        DWORD maxQueueMemorySize = 63*1024*1024;

        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettingsEx3_.InitialSecondaryReplicationQueueSize = initialQueueSize*2;
        reSettingsEx3_.MaxSecondaryReplicationQueueSize = maxQueueSize*2;
        reSettingsEx3_.MaxSecondaryReplicationQueueMemorySize = maxQueueMemorySize*2;

        reSettingsEx3_.InitialPrimaryReplicationQueueSize = initialQueueSize;
        reSettingsEx3_.MaxPrimaryReplicationQueueSize = maxQueueSize;
        reSettingsEx3_.MaxPrimaryReplicationQueueMemorySize = maxQueueMemorySize;

        reSettings_.InitialReplicationQueueSize = initialQueueSize*4;
        reSettings_.MaxReplicationQueueSize = maxQueueSize*8;
        reSettingsEx1_.MaxReplicationQueueMemorySize = maxQueueMemorySize*2;

        TestReplicatorSettings::CreateReplicator(factory, &reSettings_, replicator);

        ComPointer<IFabricStateReplicator2> replicator2;
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;

        replicator->QueryInterface(IID_IFabricStateReplicator2, reinterpret_cast<void**>(&replicator2));
        replicator2->GetReplicatorSettings(settingsResult.InitializationAddress());

        FABRIC_REPLICATOR_SETTINGS const * outputResult = settingsResult->get_ReplicatorSettings();
        VerifySettingsForFlags(reSettings_.Flags, outputResult, &reSettings_);
        factory.Close();
    }

    BOOST_AUTO_TEST_CASE(TestDynamicUpdates)
    {
        ConfigSettings settings;
        auto configStore = make_shared<ConfigSettingsConfigStore>(settings);
        Config::SetConfigStore(configStore);

        ComReplicatorFactory factory(TestReplicatorSettings::GetRoot());
        ComPointer<IFabricStateReplicator> replicator;
        ResetSettingsStructures();

        DWORD initialQueueSize = 8;
        DWORD maxQueueSize = 32;
        DWORD maxQueueMemorySize = 63*1024*1024;

        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
        reSettings_.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

        reSettingsEx3_.InitialSecondaryReplicationQueueSize = initialQueueSize*2;
        reSettingsEx3_.MaxSecondaryReplicationQueueSize = maxQueueSize*2;
        reSettingsEx3_.MaxSecondaryReplicationQueueMemorySize = maxQueueMemorySize*2;

        reSettingsEx3_.InitialPrimaryReplicationQueueSize = initialQueueSize;
        reSettingsEx3_.MaxPrimaryReplicationQueueSize = maxQueueSize;
        reSettingsEx3_.MaxPrimaryReplicationQueueMemorySize = maxQueueMemorySize;

        reSettings_.InitialReplicationQueueSize = initialQueueSize*4;
        reSettings_.MaxReplicationQueueSize = maxQueueSize*8;
        reSettingsEx1_.MaxReplicationQueueMemorySize = maxQueueMemorySize*2;

        TestReplicatorSettings::CreateReplicator(factory, &reSettings_, replicator);

        ComPointer<IFabricStateReplicator2> replicator2;
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;

        replicator->QueryInterface(IID_IFabricStateReplicator2, reinterpret_cast<void**>(&replicator2));
        replicator2->GetReplicatorSettings(settingsResult.InitializationAddress());

        FABRIC_REPLICATOR_SETTINGS const * outputResult = settingsResult->get_ReplicatorSettings();
        VerifySettingsForFlags(reSettings_.Flags, outputResult, &reSettings_);
        
        ComReplicator * comReplicator = reinterpret_cast<ComReplicator*>(replicator.GetRawPointer());
        auto configObject = comReplicator->ReplicatorEngine.Config;

        // Now, update some dynamic settings
        {
            UpdateConfig(L"MaxPendingAcknowledgements", L"2", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->MaxPendingAcknowledgements, 2, L"MaxPendingAcknowledgementsDynamic_Update");
        }

        {
            UpdateConfig(L"CompleteReplicateThreadCount", L"2", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->CompleteReplicateThreadCount, 2, L"CompleteReplicateThreadCountDynamic_Update");
        }

        {
            UpdateConfig(L"AllowMultipleQuorumSet", L"false", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->AllowMultipleQuorumSet, false, L"AllowMultipleQuorumSetDynamic_Update");
        }

        {
            UpdateConfig(L"TraceInterval", L"0.01", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->TraceInterval, TimeSpan::FromSeconds(0.01), L"TraceIntervalDynamic_Update");
        }
        
        {
            UpdateConfig(L"QueueFullTraceInterval", L"0.01", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->QueueFullTraceInterval, TimeSpan::FromSeconds(0.01), L"QueueFullTraceInterval_Update");
        }

        {
            UpdateConfig(L"EnableSlowIdleRestartForVolatile", L"true", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->EnableSlowIdleRestartForVolatile, true, L"EnableSlowIdleRestartForVolatile_Update");
        }

        {
            UpdateConfig(L"EnableSlowIdleRestartForPersisted", L"false", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->EnableSlowIdleRestartForPersisted, false, L"EnableSlowIdleRestartForPersisted_Update1");
            UpdateConfig(L"EnableSlowIdleRestartForPersisted", L"true", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->EnableSlowIdleRestartForPersisted, true, L"EnableSlowIdleRestartForPersisted_Update2");
        }

        {
            UpdateConfig(L"SlowIdleRestartAtQueueUsagePercent", L"99", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->SlowIdleRestartAtQueueUsagePercent, 99, L"SlowIdleRestartAtQueueUsagePercent_Update1");
        }

        {
            UpdateConfig(L"EnableSlowActiveSecondaryRestartForVolatile", L"true", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->EnableSlowActiveSecondaryRestartForVolatile, true, L"EnableSlowActiveSecondaryRestartForVolatile_Update");
        }

        {
            UpdateConfig(L"EnableSlowActiveSecondaryRestartForPersisted", L"false", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->EnableSlowActiveSecondaryRestartForPersisted, false, L"EnableSlowActiveSecondaryRestartForPersisted_Update1");
            UpdateConfig(L"EnableSlowActiveSecondaryRestartForPersisted", L"true", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->EnableSlowActiveSecondaryRestartForPersisted, true, L"EnableSlowActiveSecondaryRestartForPersisted_Update2");
        }

        {
            UpdateConfig(L"SlowActiveSecondaryRestartAtQueueUsagePercent", L"10", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->SlowActiveSecondaryRestartAtQueueUsagePercent, 10, L"SlowActiveSecondaryRestartAtQueueUsagePercent_Update1");
        }

        {
            UpdateConfig(L"ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness", L"50", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness, 50, L"ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness_Update");
        }

        {
            UpdateConfig(L"SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation", L"50", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation, TimeSpan::FromSeconds(50), L"SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation_Update");
        }

        {
            UpdateConfig(L"SecondaryProgressRateDecayFactor", L"0.75", settings, configStore);
            Sleep(2000);
            VERIFY_ARE_EQUAL(configObject->SecondaryProgressRateDecayFactor, 0.75, L"SecondaryProgressRateDecayFactor_Update");
        }

        factory.Close();
    }

    BOOST_AUTO_TEST_SUITE_END()

    ComponentRoot const & TestReplicatorSettings::GetRoot()
    {
        static shared_ptr<DummyRoot> root = make_shared<DummyRoot>();
        return *root;
    }

    bool TestReplicatorSettings::Setup()
    {
        return true;
    }

}
