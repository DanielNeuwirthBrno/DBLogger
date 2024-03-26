/*******************************************************************************
 Copyright 2020 Daniel Neuwirth
 This program is distributed under the terms of the GNU General Public License.
*******************************************************************************/

#include <QFile>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlRecord>
#include <QString>
#include <QVariant>
#include "query.h"
#include "shared.h"

void Query::setAllBindingsForProps(const DatabaseConnectionProps * const properties) {

    this->setBinding(QStringLiteral(":serverName"), properties->serverName());
    this->setBinding(QStringLiteral(":port"), properties->portNo());
    this->setBinding(QStringLiteral(":dbName"), properties->dbName());
    this->setBinding(QStringLiteral(":userName"), properties->userName());
}

void Query::setCustomBinding(const QString & placeholder, const QString & value) {

    const QString currentValue =
        (value.contains(QRegularExpression(QStringLiteral("[^\\w]"))))
            ? shared::leftSqBr + value + shared::rightSqBr : value;
    this->_queryString.replace(placeholder, currentValue);
    return;
}

bool Query::prepareQuery(const QString & resourcePath) {

    QFile resource(resourcePath);
    resource.open(QIODevice::ReadOnly | QIODevice::Text);
    const QByteArray queryFromResource = resource.readAll();
    QString queryString(queryFromResource);

    if (queryString.isEmpty())
        return false;
    this->_queryString = queryString;

    // set custom bindings
    for (auto it : this->_customBindings)
        setCustomBinding(it.first, it.second);

    return (this->_query.prepare(this->_queryString));
}

bool Query::processSelectQuery() {

    if (this->_query.exec()) {

        bool recordRetrieved = this->_query.first();
        if (!recordRetrieved)
            return true; // OK but empty

        // table rows
        while (recordRetrieved) {

            // table columns
            QVector<QVariant> values;

            for (int column = 0; column < _query.record().count(); ++column)
                if (this->_query.value(column).isValid())
                    values.push_back(this->_query.value(column));

            this->setResults(values);
            recordRetrieved = this->_query.next();
        }
    }
    else {

        if (this->_query.lastError().isValid())
            ErrorMessage::critical(_query.lastError().text());
        return false;
    }
    return true;
}

bool Query::processModifyQuery() {

    if (!this->_query.exec()) {

        if (this->_query.lastError().isValid())
            ErrorMessage::critical(_query.lastError().text());
        return false;
    }
    return true;
}
