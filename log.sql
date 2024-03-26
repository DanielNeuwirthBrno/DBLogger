
SELECT O.name AS ObjectName, Operation, [Transaction Name], [Transaction ID], [Begin Time], [End Time], Description, SUSER_SNAME([Transaction SID]) AS UserName
  FROM fn_dblog(null, null) AS L
  LEFT JOIN sys.system_internals_allocation_units AS AU
  ON L.AllocUnitId = AU.allocation_unit_id
  LEFT JOIN sys.partitions AS P
  ON P.partition_id = AU.container_id
  LEFT JOIN sys.objects AS O
  ON P.object_id = O.object_id
  WHERE [Transaction ID] = '0000:000003b1';
