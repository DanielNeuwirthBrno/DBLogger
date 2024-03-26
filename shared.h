/*******************************************************************************
 Copyright 2020 Daniel Neuwirth
 This program is distributed under the terms of the GNU General Public License.
*******************************************************************************/

#ifndef SHARED_H
#define SHARED_H

#include <QMessageBox>
#include <QString>
#include "constants.h"

class Brackets {

    public:
        static QString squareBrackets(const QString & text)
            { return (shared::leftSqBr + text + shared::rightSqBr); }
};

class ErrorMessage {

    public:
        static void information(const QString & error) {

            QMessageBox::information(nullptr, QStringLiteral("Informace"), error);
            return;
        }

        static void warning(const QString & error) {

            QMessageBox::warning(nullptr, QStringLiteral("Upozornění"), error);
            return;
        }

        static void critical(const QString & error) {

            QMessageBox::critical(nullptr, QStringLiteral("Chyba"), error);
            return;
        }

        static QMessageBox::StandardButton question(const QString & question) {

            return (QMessageBox::question(nullptr, QStringLiteral("Dotaz"), question));
        }
};

#endif // SHARED_H
