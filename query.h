/*******************************************************************************
 Copyright 2020 Daniel Neuwirth
 This program is distributed under the terms of the GNU General Public License.
*******************************************************************************/

#ifndef QUERY_H
#define QUERY_H

#include <QPair>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <QVector>
#include "database.h"

class Query {

    public:
        Query(const QSqlDatabase * db, const QVector<QPair<QString, QString>> & customBindings
              = QVector<QPair<QString, QString>>()):
              _customBindings(customBindings), _query(QSqlQuery(*db)) {}
        ~Query() {}

        inline int noOfRowsInResults() const { return _results.size(); }
        QVector<QVariant> rowFromResults(int row) const { return _results.at(row); }
        void setResults(const QVector<QVariant> & newRow) { _results.push_back(newRow); return; }

        bool prepareQuery(const QString &);
        void setAllBindingsForProps(const DatabaseConnectionProps * const);
        inline void setBinding(const QString & placeholder, const QString & value)
            { this->_query.bindValue(placeholder, value); return; };
        bool processSelectQuery();
        bool processModifyQuery();

    private:
        void setCustomBinding(const QString &, const QString &);

        QVector<QPair<QString, QString>> _customBindings;
        QVector<QVector<QVariant>> _results;
        QString _queryString;
        QSqlQuery _query;
};

#endif // QUERY_H
