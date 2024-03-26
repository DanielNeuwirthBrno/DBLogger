/*******************************************************************************
 Copyright 2020 Daniel Neuwirth
 This program is distributed under the terms of the GNU General Public License.
*******************************************************************************/

#include <QFile>
#include <QSqlQuery>
#include "database.h"
#include "query.h"
#include "shared.h"

// system database connection settings
DatabaseConnectionProps::DatabaseConnectionProps():
    _serverName(QStringLiteral(".")), _portNo(sql::defaultPortNo),
    _dbName(QStringLiteral("DBLogger")), _userName(QStringLiteral("web")),
    _password(QStringLiteral("web")) {}

// user database connection settings (for new database)
DatabaseConnectionProps::DatabaseConnectionProps(const QString & port):
     _serverName(QString()), _portNo(port), _dbName(QString()), _userName(QString()) {}

// user database connection settings
DatabaseConnectionProps::DatabaseConnectionProps(const QString & serverName, const QString & port,
    const QString & dbName, const QString & userName):
    _serverName(serverName), _portNo(port), _dbName(dbName), _userName(userName) {}

void DatabaseConnectionProps::setValueToVariable(const sql::map::variable variable,
                                                 const QString &newValue) {
    switch (variable) {

        case sql::map::SERVER: _serverName = newValue; break;
        case sql::map::PORT: _portNo = newValue; break;
        case sql::map::DBNAME: _dbName = newValue; break;
        case sql::map::USERNAME: _userName = newValue; break;
        case sql::map::PASSWORD: _password = newValue; break;
    }
    return;
}

DatabaseLog::DatabaseLog(const QString & objectName, const QString & operation,
    const QString & transactionName, const QString & transactionID, const QDateTime & beginTime,
    const QDateTime & endTime, const QString & description, const QString & userName):
    _objectName(objectName), _operation(operation), _transactionName(transactionName),
    _transactionID(transactionID), _beginTime(beginTime), _endTime(endTime),
    _description(description), _userName(userName) {}

// system database
Database::Database():
    _ID(QUuid::createUuid()), _databaseID(0), _connectionName(sql::systemConnection),
    _driverName(sql::defaultSqlDriver), _connectionEstablished(false), _connectionProperties(new
    DatabaseConnectionProps), _dbConnection(new QSqlDatabase), _logContents(nullptr),
    _logTable(nullptr) {

    *(_dbConnection) = QSqlDatabase::addDatabase(this->_driverName, this->_connectionName);
}

// user database
Database::Database(const QUuid ID, const int dbID, const QString & connectionName,
                   const DatabaseConnectionProps & properties):
    _ID(ID), _databaseID(dbID), _connectionName(connectionName), _driverName(sql::defaultSqlDriver),
    _connectionEstablished(false), _connectionProperties(new DatabaseConnectionProps),
     _dbConnection(new QSqlDatabase), _logContents(new QMap<QString, QVector<DatabaseLog>>) {

    *(_connectionProperties) = properties;
    *(_dbConnection) = QSqlDatabase::addDatabase(this->_driverName, this->_connectionName);
    _logTable = new QSqlTableModel(nullptr, QSqlDatabase::database(sql::systemConnection));
}

Database::Database(const Database & rhs):
    _ID(rhs._ID), _databaseID(rhs._databaseID), _connectionName(rhs._connectionName),
    _driverName(rhs._driverName), _connectionEstablished(rhs._connectionEstablished) {

    _connectionProperties = new DatabaseConnectionProps;
    *(_connectionProperties) = *(rhs._connectionProperties);
    _logContents = new QMap<QString, QVector<DatabaseLog>>;
    *(_logContents) = *(rhs._logContents);
    _dbConnection = new QSqlDatabase;
    *(_dbConnection) = *(rhs._dbConnection);
    _logTable = new QSqlTableModel(nullptr, QSqlDatabase::database(sql::systemConnection));
}

Database::~Database() {

    delete _dbConnection;
    delete _connectionProperties;
    QSqlDatabase::removeDatabase(this->_connectionName);
    delete _logTable;
}

bool Database::connectToServer(const DatabaseConnectionProps * const props, QSqlError & error) const {

    const QString connectionDataSet =
        QStringLiteral("DRIVER={SQL Server};Server=") + props->serverName() +
        QStringLiteral(";Database=") + props->dbName() + QStringLiteral(";Uid=") +
        props->userName() + QStringLiteral(";Port=") + props->portNo() + QStringLiteral(";Pwd=") +
        props->password() + QStringLiteral(";");

    if (this->_dbConnection->isOpen())
        this->_dbConnection->close();

    _dbConnection->setDatabaseName(connectionDataSet);

    const bool connectionEstablished = _dbConnection->open();
    if (!connectionEstablished)
        error = _dbConnection->lastError();

    return connectionEstablished;
}

bool Database::checkIfNameMatchesID() const {

    const QString resourceForQuery =
        QStringLiteral(":/query/sql/master/derive_name_from_database_id.sql");
    bool dataAcquired = false;

    Query * const queryToExecute = new Query(this->dbConnection());
    if (queryToExecute->prepareQuery(resourceForQuery)) {

        queryToExecute->setBinding(QStringLiteral(":id"), QString::number(this->databaseID()));

        if (queryToExecute->processSelectQuery() && queryToExecute->noOfRowsInResults() != 0) {

            const QString name = queryToExecute->rowFromResults(0).at(0).toString();
            dataAcquired = (name == dbName());
        }
    }
    delete queryToExecute;
    return dataAcquired;
}

bool Database::loadNewDatabaseID() {

    const QString resourceForQuery =
        QStringLiteral(":/query/sql/master/derive_id_from_database_name.sql");
    bool dataAcquired = false;

    Query * const queryToExecute = new Query(this->dbConnection());
    if (queryToExecute->prepareQuery(resourceForQuery)) {

        queryToExecute->setBinding(QStringLiteral(":dbName"), this->dbName());
        if (queryToExecute->processSelectQuery() && queryToExecute->noOfRowsInResults() != 0) {

            const int databaseID = queryToExecute->rowFromResults(0).at(0).toInt();
            this->_databaseID = databaseID;
            dataAcquired = true;
        }
    }
    delete queryToExecute;
    return dataAcquired;
}

bool Database::isDbAlreadyTracked(const QSqlDatabase * systemConnection) const {

    const QString resourceForQuery =
        QStringLiteral(":/query/sql/check_if_db_is_already_tracked.sql");
    bool isTracked = false;

    Query * const queryToExecute = new Query(systemConnection);
    if (queryToExecute->prepareQuery(resourceForQuery)) {

        queryToExecute->setBinding(QStringLiteral(":databaseid"),
                                   QString::number(this->databaseID()));
        queryToExecute->setBinding(QStringLiteral(":dbName"), this->dbName());

        if (queryToExecute->processSelectQuery() && queryToExecute->noOfRowsInResults() != 0)
            isTracked = queryToExecute->rowFromResults(0).at(0).toBool();
    }
    delete queryToExecute;
    return isTracked;
}

bool Database::addRecordToTrackingTable(const QSqlDatabase * systemConnection) {

    const QString resourceForQuery = QStringLiteral(":/query/sql/track_new_database.sql");
    bool dataModified = false;

    Query * const queryToExecute = new Query(systemConnection);
    if (queryToExecute->prepareQuery(resourceForQuery)) {

        queryToExecute->setAllBindingsForProps(this->connectionProperties());
        queryToExecute->setBinding(QStringLiteral(":id"), this->ID().toString(QUuid::WithoutBraces));
        queryToExecute->setBinding(QStringLiteral(":databaseid"), QString::number(this->databaseID()));
        dataModified = queryToExecute->processModifyQuery();
    }
    delete queryToExecute;
    return dataModified;
}

bool Database::removeRecordFromTrackingTable(const QSqlDatabase * systemConnection) {

    const QString resourceForQuery = QStringLiteral(":/query/sql/stop_tracking_of_database.sql");
    bool dataModified = false;

    Query * const queryToExecute = new Query(systemConnection);
    if (queryToExecute->prepareQuery(resourceForQuery)) {

        queryToExecute->setBinding(QStringLiteral(":id"), this->ID().toString(QUuid::WithoutBraces));
        dataModified = queryToExecute->processModifyQuery();
    }
    delete queryToExecute;
    return dataModified;
}

const QString Database::retrieveLastLSNFromTrackingTable() const {

    QString lastLSN = QString();

    const QString resourceForQuery =
        QStringLiteral(":/query/sql/master/retrieve_last_lsn_from_log.sql");

    // set custom bindings
    const QVector<QPair<QString, QString>> customBindings
      { qMakePair<QString, QString>(QStringLiteral(":dbName"), this->dbName()) };

    Query * const queryToExecute = new Query(this->_dbConnection, customBindings);

    if (queryToExecute->prepareQuery(resourceForQuery)) {

        if (queryToExecute->processSelectQuery() && queryToExecute->noOfRowsInResults() != 0)
            lastLSN = queryToExecute->rowFromResults(0).at(0).toString();
    }
    delete queryToExecute;
    return lastLSN;
}

bool Database::loadAllLogRecordsFromGivenLSN(const QString & fromLSN) {

    const QString resourceForQuery =
        QStringLiteral(":/query/sql/master/retrieve_data_from_log.sql");
    bool dataAcquired = false;

    // set custom bindings
    const QVector<QPair<QString, QString>> customBindings
      { qMakePair<QString, QString>(QStringLiteral(":dbName"), this->dbName()) };

    Query * const queryToExecute = new Query(this->_dbConnection, customBindings);

    if (queryToExecute->prepareQuery(resourceForQuery)) {

        queryToExecute->setBinding(QStringLiteral(":fromLSN"), fromLSN);
        const bool queryProcessed = queryToExecute->processSelectQuery();

        if (queryProcessed) {

            const int noOfRecords = queryToExecute->noOfRowsInResults();
            if (noOfRecords < 1)
                return false;

            this->_logContents->clear();

            for (int i = 0; i < noOfRecords; ++i) {

                const QString transactionID = queryToExecute->rowFromResults(i).at(3).toString();
                const DatabaseLog databaseLogRow(
                    queryToExecute->rowFromResults(i).at(0).toString(),
                    queryToExecute->rowFromResults(i).at(1).toString(),
                    queryToExecute->rowFromResults(i).at(2).toString(), transactionID,
                    queryToExecute->rowFromResults(i).at(4).toDateTime(),
                    queryToExecute->rowFromResults(i).at(5).toDateTime(),
                    queryToExecute->rowFromResults(i).at(6).toString(),
                    queryToExecute->rowFromResults(i).at(7).toString());

                // if map already contains key, add record to vector
                if (this->_logContents->contains(transactionID)) {

                    auto transactionRecord = this->_logContents->find(transactionID);
                    transactionRecord.value().push_back(databaseLogRow);
                }
                else
                    this->_logContents->insert(transactionID, QVector<DatabaseLog>{databaseLogRow});
            }
            dataAcquired = true;
        }
    }
    delete queryToExecute;
    return dataAcquired;
}

bool Database::updateTrackingTableWithLogData(const QSqlDatabase * systemConnection) {

    const QString resourceForQuery = QString(":/query/sql/update_log_table_with_new_data.sql");
    bool dataModified = false;

    for (auto it: *(this->_logContents)) {

        Query * const queryToExecute = new Query(systemConnection);

        queryToExecute->setBinding(QStringLiteral(":tableName"), this->logTableName());
        queryToExecute->setBinding(QStringLiteral(":databaseid"), QString::number(this->databaseID()));
        queryToExecute->setBinding(QStringLiteral(":objectName"), "");
        queryToExecute->setBinding(QStringLiteral(":operation"), "");
        queryToExecute->setBinding(QStringLiteral(":transID"), it[0].transactionID());
        queryToExecute->setBinding(QStringLiteral(":beginTime"), it[0].beginTime().toString(Qt::ISODate));
        queryToExecute->setBinding(QStringLiteral(":endTime"), it[0].endTime().toString(Qt::ISODate));
        queryToExecute->setBinding(QStringLiteral(":userName"), it[0].userName());
        queryToExecute->setBinding(QStringLiteral(":beginLSN"), "");
        queryToExecute->setBinding(QStringLiteral(":endLSN"), "");

        if (queryToExecute->prepareQuery(resourceForQuery)) {

            dataModified = queryToExecute->processModifyQuery();
        }

    delete queryToExecute;
    return dataModified;

    }
    return true;
}

bool Database::createLogTableForThisDB(const QSqlDatabase * systemConnection) {

    const QString resourceForQuery = QString(":/query/sql/create_new_log_table.sql");
    bool dataModified = false;

    // set custom bindings
    const QVector<QPair<QString, QString>> customBindings
      { { qMakePair<QString, QString>(QStringLiteral(":tableName"), this->logTableName()) },
        { qMakePair<QString, QString>(QStringLiteral(":foreignKeyName"), this->logTableForeignKey()) } };

    Query * const queryToExecute = new Query(systemConnection, customBindings);

    if (queryToExecute->prepareQuery(resourceForQuery)) {

        dataModified = queryToExecute->processModifyQuery();
    }
    delete queryToExecute;
    return dataModified;
}

bool Database::dropLogTableOfThisDB(const QSqlDatabase * systemConnection) {

    const QString resourceForQuery = QString(":/query/sql/drop_log_table.sql");
    bool dataModified = false;

    // set custom bindings
    const QVector<QPair<QString, QString>> customBindings
      { { qMakePair<QString, QString>(QStringLiteral(":tableName"), this->logTableName()) } };

    Query * const queryToExecute = new Query(systemConnection, customBindings);

    if (queryToExecute->prepareQuery(resourceForQuery)) {

        dataModified = queryToExecute->processModifyQuery();
    }
    delete queryToExecute;
    return dataModified;
}

bool Database::saveConfiguration(const QSqlDatabase * systemConnection) {

    // check if database DB corresponds to its name
    if (!this->checkIfNameMatchesID())
        return false;

    const QString resourceForQuery = QStringLiteral(":/query/sql/update_db_connection_settings.sql");
    bool dataModified = false;

    Query * const queryToExecute = new Query(systemConnection);
    if (queryToExecute->prepareQuery(resourceForQuery)) {

        queryToExecute->setAllBindingsForProps(this->connectionProperties());
        queryToExecute->setBinding(QStringLiteral(":id"), this->ID().toString(QUuid::WithoutBraces));
        dataModified = queryToExecute->processModifyQuery();
    }
    delete queryToExecute;
    return dataModified;
}

bool Database::retrieveSettings(QMap<dbSettings, QString> & dbSettings) const {

    const QString resourceForQuery = QStringLiteral(":/query/sql/master/dbstatus.sql");
    bool dataAcquired = false;

    Query * const queryToExecute = new Query(this->_dbConnection);
    if (queryToExecute->prepareQuery(resourceForQuery)) {

        queryToExecute->setBinding(QStringLiteral(":dbName"), this->dbName());
        if (queryToExecute->processSelectQuery() && queryToExecute->noOfRowsInResults() != 0) {

            for (int i = (int)LAST_FULL_BACKUP; i != int(END_OF_SETTINGS); ++i)
                dbSettings.insert((Database::dbSettings)i, queryToExecute->rowFromResults(0).at(i).toString());
            dataAcquired = true;
        }
    }
    delete queryToExecute;
    return dataAcquired;
}
