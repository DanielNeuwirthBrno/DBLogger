/*******************************************************************************
 Copyright 2020 Daniel Neuwirth
 This program is distributed under the terms of the GNU General Public License.
*******************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include <QUuid>
#include <QVector>
#include "ui/ui_mainwindow.h"

class MainWindow: public QDialog {

    Q_OBJECT

    public:
        explicit MainWindow(Session *, QWidget * = nullptr);
        ~MainWindow();

        QString checkForValues(const QString & inputString)
            { return (inputString.isEmpty() ? shared::noValue : inputString); }

        Ui_MainWindow * ui;

    public slots:
        bool addDbButtonClicked();
        bool removeDbButtonClicked();
        bool previousDbButtonClicked();
        bool nextDbButtonClicked();
        bool saveButtonClicked();
        bool refreshButtonClicked();
        bool connectToServerButtonClicked();

    private:
        bool switchDbActionAfterButtonClicked(const Database::dbPosition);
        void saveValue();
        void refreshDbIDLabel(const QUuid ID, const int databaseID);
        void fillFormWithBasicData(const QUuid);
        void fillFormWithSettings(QMap<Database::dbSettings, QString> &);
        void fillLogTableContents();

        void clearWindowContents() const;
        void enableButtons(const QMap<buttonType, QPushButton *> &,
                 const QList<buttonType> & = QList<buttonType>(), const bool enable = true) const;
        void disableButtons(const QMap<buttonType, QPushButton *> &,
                 const QList<buttonType> & = QList<buttonType>()) const;
        void disableButtons(const QVector<QPair<const QMap<buttonType, QPushButton *> &,
                            const QList<buttonType> &>>) const;

        Session * _currentSession;
};

#endif // MAINWINDOW_H
