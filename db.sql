
CREATE TABLE TrackedDatabases
  (ID uniqueidentifier PRIMARY KEY, Create_Date datetime, ServerName nvarchar(100) not null, DatabaseID int not null,
   DatabaseName nvarchar(100) not null, PortNo nvarchar(10), UserName nvarchar(100));

INSERT INTO TrackedDatabases
  VALUES (NEWID(), CURRENT_TIMESTAMP, '.', 11, 'S5_System_Etalon_test_F', '1433', 'web'); 

CREATE TABLE [Track_DB_73F72078-01D8-4FDC-B617-AF70460B0DF1]
  (ID uniqueidentifier PRIMARY KEY, DatabaseID uniqueidentifier not null, Create_Date datetime, CurrentLSN nvarchar not null, Operation nvarchar not null, Context nvarchar not null,
   TransactionID nvarchar not null, LogRecordLength smallint not null, PreviousLSN nvarchar not null, TransactionSID varbinary null, LogRecord varbinary not null,
   CONSTRAINT FK_DatabaseID_TrackedDatabases_ID FOREIGN KEY (DatabaseID) REFERENCES TrackedDatabases(ID));
