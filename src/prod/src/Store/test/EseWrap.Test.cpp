// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Store/test/Common.Test.h"


using namespace std;
using namespace Common;

namespace Store
{
    class TestEseWrap
    {
    };

    BOOST_FIXTURE_TEST_SUITE(TestEseWrapSuite, TestEseWrap)

        BOOST_AUTO_TEST_CASE(CreateTemporaryDatabase)
    {
        Config cfg;

        {
            TemporaryCurrentDirectory directory(std::wstring(L"FabricTestEseCreate") + StringUtility::ToWString<int64>(Stopwatch::Now().Ticks));
            TemporaryDatabase db(directory);
            db.InitializeCreate();
        }

        Trace.WriteInfo(TraceType, "Created and removed temporary database.");
    }

    BOOST_AUTO_TEST_CASE(InsertAndRead)
    {
        TemporaryCurrentDirectory directory(std::wstring(L"FabricTestEseReadWrite") + StringUtility::ToWString<int64>(Stopwatch::Now().Ticks));

        std::wstring tableName(L"SampleTable");
        std::wstring boolColumnName(L"Bool");
        std::wstring int32ColumnName(L"Int32");
        std::wstring int64ColumnName(L"Int64");
        std::wstring stringColumnName(L"String");

        std::wstring stringWrite(L"Windows Fabric Local Store");
        unsigned _int8 boolWrite = 1;
        _int32 int32Write = 42;

        // Validate that we can create and populate a row in a database
        {
            TemporaryDatabase db(directory);
            db.InitializeCreate();

            EseTableBuilder builder(tableName);
            auto tagged = JET_bitColumnTagged;
            auto autoInc = JET_bitColumnAutoincrement;
            auto fixed = JET_bitColumnFixed;
            auto notNull = JET_bitColumnNotNULL;

            auto boolIndex = builder.AddColumn<JET_coltypBit>(boolColumnName, fixed | notNull);
            auto int32Index = builder.AddColumn<JET_coltypLong>(int32ColumnName, fixed | notNull);
            builder.AddColumn<JET_coltypLongLong>(int64ColumnName, fixed | notNull | autoInc);
            auto stringIndex = builder.AddColumn<JET_coltypLongText>(
                stringColumnName,
                64 * 1024 * sizeof(wchar_t),
                tagged | notNull);

            auto cursor = db.CreateCursor();
            cursor->InitializeCreateTable(builder);

            auto boolColumn = builder.GetColumnId(boolIndex);
            auto int32Column = builder.GetColumnId(int32Index);
            auto stringColumn = builder.GetColumnId(stringIndex);

            {
                // Verify that BindToThread is re-entrant
                EseSession::BindToThread bind(db.Session, L"InsertAndRead1");
                EseSession::BindToThread bind2(db.Session, L"InsertAndRead2");
                db.Session.BeginTransaction();
                cursor->PrepareUpdate(JET_prepInsert);
                cursor->SetColumn<std::wstring>(stringColumn, stringWrite);
                cursor->SetColumn<unsigned _int8>(boolColumn, boolWrite);
                cursor->SetColumn<_int32>(int32Column, int32Write);
                cursor->Update();
                db.Session.CommitTransaction();
            }

            // Verify we can call methods on any thread and Commit on a different thread than Begin
            Common::ManualResetEvent wait;
            Common::Threadpool::Post(
                [&](){
                db.Session.BeginTransaction();
                cursor->PrepareUpdate(JET_prepInsert);
                cursor->SetColumn<std::wstring>(stringColumn, stringWrite);
                wait.Set(); });
            wait.WaitOne();

            cursor->SetColumn<unsigned _int8>(boolColumn, boolWrite);
            cursor->SetColumn<_int32>(int32Column, int32Write);
            cursor->Update();
            db.Session.CommitTransaction();

            cursor->Move(JET_MoveFirst);

            unsigned _int8 boolRead;
            cursor->RetrieveColumn<unsigned _int8>(boolColumn, boolRead);
            _int32 int32Read;
            cursor->RetrieveColumn<_int32>(int32Column, int32Read);
            std::wstring stringRead;
            cursor->RetrieveColumn<std::wstring>(stringColumn, stringRead);

            VERIFY_IS_TRUE((!boolRead) == (!boolWrite));
            VERIFY_IS_TRUE(int32Read == int32Write);
            VERIFY_IS_TRUE(stringRead == stringWrite);
        }

        // Validate that we can re-attach to the database and read back the same values
        {
            TemporaryDatabase db(directory);
            db.InitializeAttachAndOpen();

            auto cursor = db.CreateCursor();
            cursor->InitializeOpenTable(tableName);

            JET_COLUMNID boolColumn, int32Column, stringColumn;

            cursor->GetColumnId(boolColumnName, boolColumn);
            cursor->GetColumnId(int32ColumnName, int32Column);
            cursor->GetColumnId(stringColumnName, stringColumn);
            cursor->Move(JET_MoveFirst);

            unsigned _int8 boolRead;
            cursor->RetrieveColumn<unsigned _int8>(boolColumn, boolRead);
            _int32 int32Read;
            cursor->RetrieveColumn<_int32>(int32Column, int32Read);
            std::wstring stringRead;
            cursor->RetrieveColumn<std::wstring>(stringColumn, stringRead);

            VERIFY_IS_TRUE((!boolRead) == (!boolWrite));
            VERIFY_IS_TRUE(int32Read == int32Write);
            VERIFY_IS_TRUE(stringRead == stringWrite);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
