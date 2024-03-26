
CREATE TABLE test (ID uniqueidentifier NOT NULL PRIMARY KEY, name nvarchar(50) NOT NULL);
INSERT INTO test VALUES (NEWID(), 'test1');
INSERT INTO test VALUES (NEWID(), 'test2');
INSERT INTO test VALUES (NEWID(), 'test3');
