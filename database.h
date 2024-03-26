/*******************************************************************************
 Copyright 2020 Daniel Neuwirth
 This program is distributed under the terms of the GNU General Public License.
*******************************************************************************/

#ifndef DATABASE_H
#define DATABASE_H

#include <QDateTime>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlTableModel>
#include <QString>
#include <QUuid>
#include <QVariant>
#include <QVector>
#include "constants.h"

static struct LogTableLabels {

    const QString _prefix = QStringLiteral("Track_DB_");
    const QString _foreignKeyName =
        QStringLiteral("FK_[tableName]_DatabaseID_TrackedDatabases_ID");

} logTableLabels;

class DatabaseLog {

    public:
        DatabaseLog();
        DatabaseLog(const QString &, const QString &, const QString &, const QString &,
                    const QDateTime &, const QDateTime &, const QString &, const QString &);
        ~DatabaseLog() {}

        QString transactionName() const { return _transactionName; }
        QString transactionID() const { return _transactionID; }
        QDateTime beginTime() const { return _beginTime; }
        QDateTime endTime() const { return _endTime; }
        QString userName() const { return _userName; }

    private:
        QString _objectName;
        QString _operation;
        QString _transactionName;
        QString _transactionID;
        QDateTime _beginTime;
        QDateTime _endTime;
        QString _description;
        QString _userName;
};

class DatabaseConnectionProps {

    public:
        DatabaseConnectionProps();
        DatabaseConnectionProps(const QString &);
        DatabaseConnectionProps(const QString &, const QString &, const QString &, const QString &);
        DatabaseConnectionProps(const DatabaseConnectionProps &);
        ~DatabaseConnectionProps() {}

        inline QString serverName() const { return _serverName; }
        inline QString portNo() const { return _portNo; }
        inline QString dbName() const { return _dbName; }
        inline QString userName() const { return _userName; }
        inline QString password() const { return _password; }

        void setValueToVariable(const sql::map::variable, const QString &);

    private:
        QString _serverName;
        QString _portNo;
        QString _dbName;
        QString _userName;
        QString _password;
};

class Database {

    public:
        Database();
        Database(const QUuid, const int, const QString &, const DatabaseConnectionProps &);
        Database(const Database &);
        ~Database();

        enum dbSettings { LAST_FULL_BACKUP, LAST_DIFF_BACKUP, LAST_LOG_BACKUP,
                          RECOVERY_MODEL, STATE, END_OF_SETTINGS };
        enum dbPosition { NO_DB = 0, FIRST_DB, PREVIOUS_DB, NEXT_DB, LAST_DB };

        inline void initializeLogTable(const QString & tableName)
            { this->_logTable->setTable(tableName); this->_logTable->select(); return; }

        inline QUuid ID() const { return _ID; }
        inline int databaseID() const { return _databaseID; }
        inline QString connectionName() const { return _connectionName; }
        inline bool connectionEstablished() const { return _connectionEstablished; }
        inline QString dbName() const { return _connectionProperties->dbName(); }
        inline DatabaseConnectionProps * connectionProperties() const { return _connectionProperties; }
        inline QSqlDatabase * dbConnection() const { return _dbConnection; }
        inline QSqlTableModel * logTable() { return _logTable; }

        inline QString logTableName() const
            { return (logTableLabels._prefix + _ID.toString(QUuid::WithoutBraces)); }
        inline QString logTableForeignKey() const
            {  QString foreignKeyTemplate = logTableLabels._foreignKeyName;
               return (foreignKeyTemplate.replace("[tableName]", this->logTableName())); }
        inline void deleteDbID() { _databaseID = -1 /* behaves as new */; return; }

        bool loadNewDatabaseID();
        bool isDbAlreadyTracked(const QSqlDatabase *) const;
        bool addRecordToTrackingTable(const QSqlDatabase *);
        bool removeRecordFromTrackingTable(const QSqlDatabase *);
        const QString retrieveLastLSNFromTrackingTable() const;
        bool loadAllLogRecordsFromGivenLSN(const QString &);
        bool updateTrackingTableWithLogData(const QSqlDatabase *);
        bool createLogTableForThisDB(const QSqlDatabase *);
        bool dropLogTableOfThisDB(const QSqlDatabase *);
        void connectionResult(const bool result) { _connectionEstablished = result; return; }

        bool connectToServer(const DatabaseConnectionProps * const, QSqlError &) const;
        bool saveConfiguration(const QSqlDatabase *);
        bool retrieveSettings(QMap<dbSettings, QString> &) const;

    private:
        bool checkIfNameMatchesID() const;

        const QUuid _ID;
        int _databaseID;
        const QString _connectionName;
        const QString _driverName;
        bool _connectionEstablished;
        DatabaseConnectionProps * _connectionProperties;
        QSqlDatabase * _dbConnection;
        QMap<QString, QVector<DatabaseLog>> * _logContents;
        QSqlTableModel * _logTable;
};

#endif // DATABASE_H
