// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace AzureTableDownloader
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using Microsoft.WindowsAzure.Storage.Table;

    public static class AzureTableDownloader
    {
        internal const string PartitionKeyProperty = "PartitionKey";
        internal const string RowKeyProperty = "RowKey";
        public static void DownloadToFile(CloudTable table, string fileFullPath)
        {
            // Initialize column list
            Dictionary<int, string> columnNames = new Dictionary<int, string>();
            int columns = 2;
            columnNames[0] = PartitionKeyProperty;
            columnNames[1] = RowKeyProperty;

            // Get all entities
            TableQuery<DynamicTableEntity> tableQuery = new TableQuery<DynamicTableEntity>();
            IEnumerable<DynamicTableEntity> entities = table.ExecuteQuery(tableQuery);
            using (StreamWriter writer = new StreamWriter(fileFullPath))
            {
                foreach (DynamicTableEntity entity in entities)
                {
                    // Figure out what columns this entity has. If there are
                    // any new columns, add them to our list
                    foreach (string columnName in entity.Properties.Keys)
                    {
                        if (false == columnNames.Values.Contains(columnName))
                        {
                            columnNames[columns] = columnName;
                            columns++;
                        }
                    }

                    // Go through each column of this entity and get the string
                    // representation of the column value. Use commas to separate
                    // multiple columns
                    string line = String.Concat(entity.PartitionKey, ",", entity.RowKey);
                    for (int i = 2; i < columns; i++)
                    {
                        line = String.Concat(line, ",");
                        if (false == entity.Properties.ContainsKey(columnNames[i]))
                        {
                            continue;
                        }
                        string columnValue = String.Empty;
                        switch (entity.Properties[columnNames[i]].PropertyType)
                        {
                            case EdmType.Boolean:
                                {
                                    bool value = (bool)entity.Properties[columnNames[i]].BooleanValue;
                                    columnValue = value.ToString();
                                }
                                break;
                            case EdmType.Int32:
                                {
                                    int value = (int)entity.Properties[columnNames[i]].Int32Value;
                                    columnValue = value.ToString();
                                }
                                break;
                            case EdmType.Int64:
                                {
                                    long value = (long)entity.Properties[columnNames[i]].Int64Value;
                                    columnValue = value.ToString();
                                }
                                break;
                            case EdmType.String:
                                {
                                    columnValue = (string)entity.Properties[columnNames[i]].StringValue;
                                }
                                break;
                        }
                        line = String.Concat(line, columnValue);
                    }

                    // Write the entity to the file in CSV format
                    writer.WriteLine(line);
                }

                // Write the column names at the bottom of the file
                string columnNameLine = String.Empty;
                for (int i = 0; i < columns; i++)
                {
                    if (i > 0)
                    {
                        columnNameLine = String.Concat(columnNameLine, ",");
                    }
                    columnNameLine = String.Concat(columnNameLine, columnNames[i]);
                }
                writer.WriteLine(columnNameLine);
            }
        }
    }
}