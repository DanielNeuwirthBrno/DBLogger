/*******************************************************************************
 Copyright 2020 Daniel Neuwirth
 This program is distributed under the terms of the GNU General Public License.
*******************************************************************************/

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QMap>
#include <QString>

namespace shared {

    const static QString noValue = QStringLiteral("N/A");
    const static QString leftSqBr = QStringLiteral("[");
    const static QString rightSqBr = QStringLiteral("]");
}

namespace sql {

    const static QString noDatabasesToTrack =
        QStringLiteral("<b><font color=\"red\">Nejsou sledovány žádné databáze.</font></b>");

    const static QString defaultSqlDriver = QStringLiteral("QODBC3");
    const static QString defaultPortNo = QStringLiteral("1433");
    const static QString systemConnection = QStringLiteral("systemConnection");

    const static QString dbLog = QStringLiteral("fn_dblog");

namespace map {

    enum variable { SERVER, PORT, DBNAME, USERNAME, PASSWORD };

    const static QMap<QString, sql::map::variable> widgetToVariable =
        { { QStringLiteral("serverNameLineEdit"), sql::map::SERVER },
          { QStringLiteral("sqlPortNoLineEdit"), sql::map::PORT },
          { QStringLiteral("databaseNameLineEdit"), sql::map::DBNAME },
          { QStringLiteral("userNameLineEdit"), sql::map::USERNAME },
          { QStringLiteral("passwordLineEdit"), sql::map::PASSWORD } };
    }
}

#endif // CONSTANTS_H
