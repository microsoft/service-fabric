// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace Data::Utilities;

    class StatusConverterTests
    {
    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

    private:
        KtlSystem* ktlSystem_;
    };

    BOOST_FIXTURE_TEST_SUITE(StatusConverterTestSuite, StatusConverterTests)

    BOOST_AUTO_TEST_CASE(IsSFHResult_SFHResult_TRUE)
    {
        VERIFY_IS_TRUE(StatusConverter::IsSFHResult(FABRIC_E_COMMUNICATION_ERROR));

        VERIFY_IS_TRUE(StatusConverter::IsSFHResult(FABRIC_E_RECONFIGURATION_PENDING));

        VERIFY_IS_TRUE(StatusConverter::IsSFHResult(FABRIC_E_WRITE_CONFLICT));

        VERIFY_IS_TRUE(StatusConverter::IsSFHResult(FABRIC_E_UPLOAD_SESSION_ID_CONFLICT));
    }

    BOOST_AUTO_TEST_CASE(IsSFHResult_NotSF_FALSE)
    {
        VERIFY_IS_FALSE(StatusConverter::IsSFHResult(E_ABORT));

        VERIFY_IS_FALSE(StatusConverter::IsSFHResult(E_ACCESSDENIED));

        VERIFY_IS_FALSE(StatusConverter::IsSFHResult(E_OUTOFMEMORY));

        VERIFY_IS_FALSE(StatusConverter::IsSFHResult(E_BOUNDS));
    }

    BOOST_AUTO_TEST_CASE(TryConvertConvertToSFSTATUS_SFHResult_TRUE)
    {
        bool isConverted = false;
        NTSTATUS result;

        isConverted = StatusConverter::TryConvertToSFSTATUS(FABRIC_E_COMMUNICATION_ERROR, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_COMMUNICATION_ERROR);

        isConverted = StatusConverter::TryConvertToSFSTATUS(FABRIC_E_RECONFIGURATION_PENDING, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_RECONFIGURATION_PENDING);

        isConverted = StatusConverter::TryConvertToSFSTATUS(FABRIC_E_NOT_PRIMARY, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_NOT_PRIMARY);

        isConverted = StatusConverter::TryConvertToSFSTATUS(FABRIC_E_REPLICATION_QUEUE_FULL, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_REPLICATION_QUEUE_FULL);

        isConverted = StatusConverter::TryConvertToSFSTATUS(FABRIC_E_OBJECT_CLOSED, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_OBJECT_CLOSED);

        isConverted = StatusConverter::TryConvertToSFSTATUS(FABRIC_E_TIMEOUT, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_TIMEOUT);

        isConverted = StatusConverter::TryConvertToSFSTATUS(FABRIC_E_OBJECT_DISPOSED, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_OBJECT_DISPOSED);

        isConverted = StatusConverter::TryConvertToSFSTATUS(FABRIC_E_NOT_READABLE, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_NOT_READABLE);
    }

    BOOST_AUTO_TEST_CASE(TryConvertConvertToSFSTATUS_NotSF_FALSE)
    {
        bool isConverted = true;
        NTSTATUS result;

        isConverted = StatusConverter::TryConvertToSFSTATUS(E_ABORT, result);
        VERIFY_IS_FALSE(isConverted);

        isConverted = StatusConverter::TryConvertToSFSTATUS(E_ACCESSDENIED, result);
        VERIFY_IS_FALSE(isConverted);

        isConverted = StatusConverter::TryConvertToSFSTATUS(E_OUTOFMEMORY, result);
        VERIFY_IS_FALSE(isConverted);

        isConverted = StatusConverter::TryConvertToSFSTATUS(E_BOUNDS, result);
        VERIFY_IS_FALSE(isConverted);
    }

    BOOST_AUTO_TEST_CASE(TryConvert_SFHResult_TRUE)
    {
        bool isConverted = false;
        NTSTATUS result;

        isConverted = StatusConverter::TryConvert(FABRIC_E_COMMUNICATION_ERROR, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_COMMUNICATION_ERROR);

        isConverted = StatusConverter::TryConvert(FABRIC_E_RECONFIGURATION_PENDING, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_RECONFIGURATION_PENDING);

        isConverted = StatusConverter::TryConvert(FABRIC_E_NOT_PRIMARY, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_NOT_PRIMARY);

        isConverted = StatusConverter::TryConvert(FABRIC_E_REPLICATION_QUEUE_FULL, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_REPLICATION_QUEUE_FULL);

        isConverted = StatusConverter::TryConvert(FABRIC_E_OBJECT_CLOSED, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_OBJECT_CLOSED);

        isConverted = StatusConverter::TryConvert(FABRIC_E_TIMEOUT, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_TIMEOUT);

        isConverted = StatusConverter::TryConvert(FABRIC_E_OBJECT_DISPOSED, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_OBJECT_DISPOSED);

        isConverted = StatusConverter::TryConvert(FABRIC_E_NOT_READABLE, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_NOT_READABLE);
    }

    BOOST_AUTO_TEST_CASE(TryConvert_WellKnown_TRUE)
    {
        bool isConverted = false;
        NTSTATUS result;

        isConverted = StatusConverter::TryConvert(S_OK, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_SUCCESS);

        isConverted = StatusConverter::TryConvert(E_INVALIDARG, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_INVALID_PARAMETER);

        isConverted = StatusConverter::TryConvert(E_ACCESSDENIED, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_ACCESS_DENIED);

        isConverted = StatusConverter::TryConvert(E_POINTER, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == Common::K_STATUS_NULL_REF_POINTER);

        isConverted = StatusConverter::TryConvert(E_ABORT, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_CANCELLED);

        isConverted = StatusConverter::TryConvert(E_FAIL, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_UNSUCCESSFUL);

        isConverted = StatusConverter::TryConvert(E_OUTOFMEMORY, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_INSUFFICIENT_RESOURCES);

        isConverted = StatusConverter::TryConvert(E_NOTIMPL, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_NOT_IMPLEMENTED);
    }

    BOOST_AUTO_TEST_CASE(TryConvert_Unknown_TRUE)
    {
        bool isConverted = false;
        NTSTATUS status;
        HRESULT hResult;

        isConverted = StatusConverter::TryConvert(E_NOINTERFACE, status);
        VERIFY_IS_TRUE(isConverted);
        hResult = StatusConverter::ToHResult(status);
        VERIFY_IS_TRUE(hResult == E_NOINTERFACE);

        isConverted = StatusConverter::TryConvert(E_HANDLE, status);
        VERIFY_IS_TRUE(isConverted);
        hResult = StatusConverter::ToHResult(status);
        VERIFY_IS_TRUE(hResult == E_HANDLE);
    }

    BOOST_AUTO_TEST_CASE(IsSFStatus_SFStatus_TRUE)
    {
        VERIFY_IS_TRUE(StatusConverter::IsSFStatus(SF_STATUS_COMMUNICATION_ERROR));

        VERIFY_IS_TRUE(StatusConverter::IsSFStatus(SF_STATUS_TIMEOUT));

        VERIFY_IS_TRUE(StatusConverter::IsSFStatus(SF_STATUS_NAME_ALREADY_EXISTS));

        VERIFY_IS_TRUE(StatusConverter::IsSFStatus(SF_STATUS_NO_WRITE_QUORUM));
    }

    BOOST_AUTO_TEST_CASE(IsSFStatus_NotSFStatus_FALSE)
    {
        VERIFY_IS_FALSE(StatusConverter::IsSFStatus(STATUS_SUCCESS));

        VERIFY_IS_FALSE(StatusConverter::IsSFStatus(STATUS_INVALID_PARAMETER_1));

        VERIFY_IS_FALSE(StatusConverter::IsSFStatus(STATUS_INVALID_PARAMETER));

        VERIFY_IS_FALSE(StatusConverter::IsSFStatus(STATUS_INSUFFICIENT_RESOURCES));

        VERIFY_IS_FALSE(StatusConverter::IsSFStatus(SF_STATUS_COMMUNICATION_ERROR - 1));
    }

    BOOST_AUTO_TEST_CASE(TOSFHResult_SFStatus_TRUE)
    {
        bool isConverted = false;
        HRESULT result;

        isConverted = StatusConverter::ToSFHResult(SF_STATUS_COMMUNICATION_ERROR, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == FABRIC_E_COMMUNICATION_ERROR);

        isConverted = StatusConverter::ToSFHResult(SF_STATUS_RECONFIGURATION_PENDING, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == FABRIC_E_RECONFIGURATION_PENDING);

        isConverted = StatusConverter::ToSFHResult(SF_STATUS_NOT_PRIMARY, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == FABRIC_E_NOT_PRIMARY);

        isConverted = StatusConverter::ToSFHResult(SF_STATUS_REPLICATION_QUEUE_FULL, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == FABRIC_E_REPLICATION_QUEUE_FULL);

        isConverted = StatusConverter::ToSFHResult(SF_STATUS_OBJECT_CLOSED, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == FABRIC_E_OBJECT_CLOSED);

        isConverted = StatusConverter::ToSFHResult(SF_STATUS_TIMEOUT, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == FABRIC_E_TIMEOUT);

        isConverted = StatusConverter::ToSFHResult(SF_STATUS_OBJECT_DISPOSED, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == FABRIC_E_OBJECT_DISPOSED);

        isConverted = StatusConverter::ToSFHResult(SF_STATUS_NOT_READABLE, result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == FABRIC_E_NOT_READABLE);
    }

    BOOST_AUTO_TEST_CASE(ToSFHResult_NotSFStatus_FALSE)
    {
        bool isConverted = true;
        NTSTATUS result;

        isConverted = StatusConverter::ToSFHResult(STATUS_SUCCESS, result);
        VERIFY_IS_FALSE(isConverted);

        isConverted = StatusConverter::ToSFHResult(STATUS_INVALID_PARAMETER_1, result);
        VERIFY_IS_FALSE(isConverted);

        isConverted = StatusConverter::ToSFHResult(STATUS_INVALID_PARAMETER, result);
        VERIFY_IS_FALSE(isConverted);

        isConverted = StatusConverter::ToSFHResult(STATUS_INSUFFICIENT_RESOURCES, result);
        VERIFY_IS_FALSE(isConverted);

        isConverted = StatusConverter::ToSFHResult(SF_STATUS_COMMUNICATION_ERROR - 1, result);
        VERIFY_IS_FALSE(isConverted);
    }

    BOOST_AUTO_TEST_CASE(ToHResult_SFHResult_TRUE)
    {
        HRESULT result;

        result = StatusConverter::ToHResult(SF_STATUS_COMMUNICATION_ERROR);
        VERIFY_IS_TRUE(result == FABRIC_E_COMMUNICATION_ERROR);

        result = StatusConverter::ToHResult(SF_STATUS_RECONFIGURATION_PENDING);
        VERIFY_IS_TRUE(result == FABRIC_E_RECONFIGURATION_PENDING);

        result = StatusConverter::ToHResult(SF_STATUS_NOT_PRIMARY);
        VERIFY_IS_TRUE(result == FABRIC_E_NOT_PRIMARY);

        result = StatusConverter::ToHResult(SF_STATUS_REPLICATION_QUEUE_FULL);
        VERIFY_IS_TRUE(result == FABRIC_E_REPLICATION_QUEUE_FULL);

        result = StatusConverter::ToHResult(SF_STATUS_OBJECT_CLOSED);
        VERIFY_IS_TRUE(result == FABRIC_E_OBJECT_CLOSED);

        result = StatusConverter::ToHResult(SF_STATUS_TIMEOUT);
        VERIFY_IS_TRUE(result == FABRIC_E_TIMEOUT);

        result = StatusConverter::ToHResult(SF_STATUS_OBJECT_DISPOSED);
        VERIFY_IS_TRUE(result == FABRIC_E_OBJECT_DISPOSED);

        result = StatusConverter::ToHResult(SF_STATUS_NOT_READABLE);
        VERIFY_IS_TRUE(result == FABRIC_E_NOT_READABLE);
    }

    BOOST_AUTO_TEST_CASE(ToHResult_WellKnown_TRUE)
    {
        HRESULT result;

        result = StatusConverter::ToHResult(STATUS_SUCCESS);
        VERIFY_IS_TRUE(result == S_OK);

        result = StatusConverter::ToHResult(STATUS_INVALID_PARAMETER);
        VERIFY_IS_TRUE(result == E_INVALIDARG);

        result = StatusConverter::ToHResult(STATUS_ACCESS_DENIED);
        VERIFY_IS_TRUE(result == E_ACCESSDENIED);

        result = StatusConverter::ToHResult(Common::K_STATUS_NULL_REF_POINTER);
        VERIFY_IS_TRUE(result == E_POINTER);

        result = StatusConverter::ToHResult(STATUS_CANCELLED);
        VERIFY_IS_TRUE(result == E_ABORT);

        result = StatusConverter::ToHResult(STATUS_UNSUCCESSFUL);
        VERIFY_IS_TRUE(result == E_FAIL);

        result = StatusConverter::ToHResult(STATUS_INSUFFICIENT_RESOURCES);
        VERIFY_IS_TRUE(result == E_OUTOFMEMORY);

        result = StatusConverter::ToHResult(STATUS_NOT_IMPLEMENTED);
        VERIFY_IS_TRUE(result == E_NOTIMPL);
    }

    BOOST_AUTO_TEST_CASE(ToHResult_Unknown_TRUE)
    {
        HRESULT hResult;
        NTSTATUS status;

        hResult = StatusConverter::ToHResult(STATUS_ABIOS_INVALID_COMMAND);
        StatusConverter::TryConvert(hResult, status);
        VERIFY_IS_TRUE(status == STATUS_ABIOS_INVALID_COMMAND);

        hResult = StatusConverter::ToHResult(STATUS_NAME_TOO_LONG);
        StatusConverter::TryConvert(hResult, status);
        VERIFY_IS_TRUE(status == STATUS_NAME_TOO_LONG);

        hResult = StatusConverter::ToHResult(STATUS_OBJECT_NAME_EXISTS);
        StatusConverter::TryConvert(hResult, status);
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_EXISTS);

        hResult = StatusConverter::ToHResult(STATUS_UNSUCCESSFUL);
        StatusConverter::TryConvert(hResult, status);
        VERIFY_IS_TRUE(status == STATUS_UNSUCCESSFUL);

        hResult = StatusConverter::ToHResult(STATUS_INTERNAL_DB_CORRUPTION);
        StatusConverter::TryConvert(hResult, status);
        VERIFY_IS_TRUE(status == STATUS_INTERNAL_DB_CORRUPTION);

        hResult = StatusConverter::ToHResult(STATUS_INTERNAL_DB_ERROR);
        StatusConverter::TryConvert(hResult, status);
        VERIFY_IS_TRUE(status == STATUS_INTERNAL_DB_ERROR);
    }

    BOOST_AUTO_TEST_CASE(TryConvert_Win32Error_WellKnown_TRUE)
    {
        bool isConverted = true;
        NTSTATUS result;

        isConverted = StatusConverter::TryConvert(HRESULT_FROM_WIN32(ERROR_DISK_FULL), result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_DISK_FULL);

        isConverted = StatusConverter::TryConvert(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_ACCESS_DENIED);

        isConverted = StatusConverter::TryConvert(HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY), result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_INSUFFICIENT_RESOURCES);

        isConverted = StatusConverter::TryConvert(HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY), result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_INSUFFICIENT_RESOURCES);

        isConverted = StatusConverter::TryConvert(HRESULT_FROM_WIN32(ERROR_OPEN_FAILED), result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == STATUS_OPEN_FAILED);

        isConverted = StatusConverter::TryConvert(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_FILE_NOT_FOUND);

        isConverted = StatusConverter::TryConvert(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), result);
        VERIFY_IS_TRUE(isConverted);
        VERIFY_IS_TRUE(result == SF_STATUS_DIRECTORY_NOT_FOUND);
    }

    BOOST_AUTO_TEST_CASE(TryConvert_Win32Error_UnKnown_TRUE)
    {
        bool isConverted = true;
        NTSTATUS status;
        HRESULT hResult;

        isConverted = StatusConverter::TryConvert(HRESULT_FROM_WIN32(ERROR_CURRENT_DIRECTORY), status);
        VERIFY_IS_TRUE(isConverted);
        hResult = StatusConverter::ToHResult(status);
        VERIFY_IS_TRUE(hResult == HRESULT_FROM_WIN32(ERROR_CURRENT_DIRECTORY));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
