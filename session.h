/*******************************************************************************
 Copyright 2020 Daniel Neuwirth
 This program is distributed under the terms of the GNU General Public License.
*******************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include <QMap>
#include "database.h"

class Session {

    public:
        Session();
        ~Session();

        inline Database * systemDatabase() const { return _systemDatabase; }
        inline QUuid currentUserDatabaseID() const { return _currentUserDatabaseID; }
        QVector<Database *> dbs() const { return _db; }
        Database * db(const QUuid & ID) const;

        QUuid addNewUserDatabase();
        bool removeUserDatabase(QUuid &);
        bool switchUserDatabase(const Database::dbPosition, QUuid &) const;
        inline bool isUserDbNew() const { return (db(_currentUserDatabaseID)->databaseID() == -1); }
        inline void changeCurrentDbTo(QUuid ID) { _currentUserDatabaseID = ID; return; }
        bool connectToSystemDatabase() const;
        inline int noOfDatabases() const { return _db.size(); }
        bool connectToUserDatabase() const;
        bool saveUserDbConfiguration();
        bool loadRecordsFromLog();
        bool retrieveUserDbSettings(QMap<Database::dbSettings, QString> &) const;

    private:
        bool loadDatabases();

        Database * _systemDatabase;
        QUuid _currentUserDatabaseID;
        QVector<Database *> _db;
};

#endif // SESSION_H
