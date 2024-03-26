/*******************************************************************************
 Copyright 2020 Daniel Neuwirth
 This program is distributed under the terms of the GNU General Public License.
*******************************************************************************/

#include <QSqlError>
#include "query.h"
#include "session.h"
#include "shared.h"

Session::Session(): _systemDatabase(new Database) {

     if (this->connectToSystemDatabase()) {

        if (!this->loadDatabases())
            this->_currentUserDatabaseID = QUuid();
     }
     else
         ErrorMessage::critical(QStringLiteral("Nepodařilo se připojit k systémové databázi."));
}

Session::~Session()
{
    for (auto it: _db)
        delete it;
    delete _systemDatabase;
}

Database * Session::db(const QUuid & ID) const {

    for (auto it: _db)
        if (it->ID() == ID)
            return it;

    return nullptr;
}

QUuid Session::addNewUserDatabase() {

    const QUuid ID = QUuid::createUuid();
    const QString connectionName =
        QStringLiteral("connection_") + ID.toString(QUuid::WithoutBraces);

    Database * const newDB = new Database(ID, int(-1), connectionName,
                                          DatabaseConnectionProps(sql::defaultPortNo));
    this->_db.push_back(newDB);

    return ID;
}

bool Session::removeUserDatabase(QUuid & currentIDAfterRemove) {

    // there’s no choosing from out of nothing
    if (this->noOfDatabases() > 0) {

        if (!this->isUserDbNew()) {

            // DB is not new => can be anywhere in the list of tracked DBs
            bool logTableDropped = false, recordRemoved = false;
            auto it = _db.begin();
            for (; it != _db.end(); ++it) {

                if ((*it)->ID() == _currentUserDatabaseID) {

                    QSqlDatabase::database(this->systemDatabase()->connectionName()).transaction();
                    // drop (this DB's) log table
                    // log table must be dropped before removing tracking record (key constraint)
                    logTableDropped = (*it)->
                        dropLogTableOfThisDB(this->systemDatabase()->dbConnection());

                    if (logTableDropped) {

                        // delete from tracking table
                        recordRemoved = (*it)->
                            removeRecordFromTrackingTable(this->_systemDatabase->dbConnection());

                        if (recordRemoved) {

                            QSqlDatabase::database(this->systemDatabase()->connectionName()).commit();
                            // remove from list of tracked databases
                            _db.erase(it);
                            break;
                        }
                        else QSqlDatabase::database(this->systemDatabase()->connectionName()).rollback();
                    }
                    else QSqlDatabase::database(this->systemDatabase()->connectionName()).rollback();
                }
            }
            if (!logTableDropped || !recordRemoved) { // if DB is not found => both values are false

                ErrorMessage::critical(QStringLiteral("Zvolenou databázi nebylo možno odebrat."));
                return false;
            }
        }
        else
            // DB is new => must be last in the list of tracked DBs
            this->_db.pop_back();

        // change current DB either to the first one
        if (this->noOfDatabases() > 0)
            currentIDAfterRemove = this->dbs().first()->ID();
        // or to null if nothing is left <=> no action
    }
    else
        return false;

    return true;
}

bool Session::switchUserDatabase(const Database::dbPosition dbLoc, QUuid & dbID) const {

    const int positionOfCurrentDbInVector = this->_db.indexOf(this->db(_currentUserDatabaseID));

    switch (dbLoc) {

        case Database::FIRST_DB: dbID = this->dbs().first()->ID(); break;
        case Database::PREVIOUS_DB: {
            if (positionOfCurrentDbInVector != 0)
                dbID = this->_db.at(positionOfCurrentDbInVector - 1)->ID();
            break;
        }
        case Database::NEXT_DB: {
            if (positionOfCurrentDbInVector != this->noOfDatabases())
               dbID = this->_db.at(positionOfCurrentDbInVector + 1)->ID();
            break;
        }
        case Database::LAST_DB: dbID = this->dbs().last()->ID(); break;
        default: dbID = QUuid(); return false;
    }

    return true;
}

bool Session::connectToUserDatabase() const {

    if (this->noOfDatabases() == 0 || this->db(_currentUserDatabaseID)->dbName().isEmpty()) {

        ErrorMessage::warning(QStringLiteral("Nepodařilo se připojit k uživatelské databázi."));
        return false;
    }

    QSqlError error;

    this->db(_currentUserDatabaseID)->connectionResult
        (this->db(_currentUserDatabaseID)->connectToServer(
             this->db(_currentUserDatabaseID)->connectionProperties(), error));

    if (!this->db(_currentUserDatabaseID)->connectionEstablished())
        ErrorMessage::critical(error.text());

    return (this->db(_currentUserDatabaseID)->connectionEstablished());
}

bool Session::connectToSystemDatabase() const {

    QSqlError error;

    this->_systemDatabase->connectionResult
        (this->_systemDatabase->connectToServer(_systemDatabase->connectionProperties(), error));

    if (!this->_systemDatabase->connectionEstablished())
        ErrorMessage::critical(error.text());

    return (this->_systemDatabase->connectionEstablished());
}

bool Session::saveUserDbConfiguration() {

    bool configurationSaved = false;

    if (this->isUserDbNew()) {

        Database * const newDatabase = this->db(_currentUserDatabaseID);

        // in case of (adding) new database, load its ID from master DB
        if (newDatabase->loadNewDatabaseID()) {

            // check if newly added database isn't already tracked
            const bool alreadyTracked =
                newDatabase->isDbAlreadyTracked(this->_systemDatabase->dbConnection());

            if (!alreadyTracked) {

                bool recordAdded = false, logTableCreated = false;

                QSqlDatabase::database(this->systemDatabase()->connectionName()).transaction();
                // add new record to tracking table
                recordAdded =
                    newDatabase->addRecordToTrackingTable(this->_systemDatabase->dbConnection());

                if (recordAdded)
                    // create log table (for this DB)
                    logTableCreated =
                        newDatabase->createLogTableForThisDB(this->_systemDatabase->dbConnection());

                if (logTableCreated)
                    QSqlDatabase::database(this->systemDatabase()->connectionName()).commit();

                configurationSaved = recordAdded && logTableCreated;
                if (!configurationSaved)
                    QSqlDatabase::database(this->systemDatabase()->connectionName()).rollback();
            }
            else {

                ErrorMessage::information(QStringLiteral("Tato databáze je již sledována."));
                // behave as new again
                newDatabase->deleteDbID();
            }
        }
    }
    else {

        configurationSaved = this->db(this->_currentUserDatabaseID)->
            saveConfiguration(this->_systemDatabase->dbConnection());
    };

    if (!configurationSaved)
        ErrorMessage::critical(QStringLiteral("Konfiguraci se nepodařilo uložit."));

    return configurationSaved;
}

bool Session::loadRecordsFromLog() {

    Database * const currentDatabase = this->db(_currentUserDatabaseID);

    // retrieve last tracked LSN
    const QString lastLSN = currentDatabase->retrieveLastLSNFromTrackingTable();

    // load data from log
    const bool logDataLoaded = currentDatabase->loadAllLogRecordsFromGivenLSN(lastLSN);

    // update tracking table with log data
    if (logDataLoaded) {

        currentDatabase->updateTrackingTableWithLogData(this->systemDatabase()->dbConnection());
    }

    return logDataLoaded;
}

bool Session::retrieveUserDbSettings(QMap<Database::dbSettings, QString> & dbSettings) const {

    return (this->db(this->_currentUserDatabaseID)->retrieveSettings(dbSettings));
}

bool Session::loadDatabases() {

    const QString resourceForQuery = QStringLiteral(":/query/sql/list_of_tracked_databases.sql");

    Query * queryToExecute = new Query(this->systemDatabase()->dbConnection());

    if (!queryToExecute->prepareQuery(resourceForQuery))
        return false;

    const bool queryProcessed = queryToExecute->processSelectQuery();

    if (queryProcessed) {

        const int noOfRecords = queryToExecute->noOfRowsInResults();
        if (noOfRecords < 1)
            return false;

        for (int i = 0; i < noOfRecords; ++i) {

            const QString connectionName =
                QStringLiteral("connection_") + queryToExecute->rowFromResults(i).at(0).toString();

            const QUuid ID = queryToExecute->rowFromResults(i).at(0).toUuid();
            const int dbID = queryToExecute->rowFromResults(i).at(3).toInt();

            const DatabaseConnectionProps properties = DatabaseConnectionProps(
                queryToExecute->rowFromResults(i).at(1).toString(), // serverName
                queryToExecute->rowFromResults(i).at(2).toString(), // port
                queryToExecute->rowFromResults(i).at(4).toString(), // dbName
                queryToExecute->rowFromResults(i).at(5).toString()); // userName

            Database * const userDB = new Database(ID, dbID, connectionName, properties);
            this->_db.push_back(userDB);

            // first database is set as current
            if (i == 0)
                this->_currentUserDatabaseID = ID;
        }
    }
    else
        ErrorMessage::warning(QStringLiteral("Nepodařilo se načíst údaje o sledovaných databázích."));

    delete queryToExecute;
    return queryProcessed;
}
