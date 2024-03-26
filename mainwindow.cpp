/*******************************************************************************
 Copyright 2020 Daniel Neuwirth
 This program is distributed under the terms of the GNU General Public License.
*******************************************************************************/

#include <QApplication>
#include <QDialog>
#include "mainwindow.h"
#include "shared.h"
#include "ui/ui_mainwindow.h"

MainWindow::MainWindow(Session * session, QWidget * parent):
    QDialog(parent), ui(new Ui_MainWindow), _currentSession(session) {

    ui->setupUi(this);

    // enable state buttons
    const int noOfDatabases = session->noOfDatabases();
    QList<buttonType> buttonsToEnable { buttonType::ADD_DB };
    if (noOfDatabases > 0)
        buttonsToEnable << buttonType::REMOVE_DB;
    if (noOfDatabases > 1)
        buttonsToEnable << buttonType::NEXT_DB;
    this->enableButtons(ui->switchDbButtons, buttonsToEnable);

    if (noOfDatabases > 0) {

        fillFormWithBasicData(session->currentUserDatabaseID());
        fillLogTableContents();
    }
    else {

        ui->dbIDLabel->setText(sql::noDatabasesToTrack);
        this->enableButtons(ui->handleDbButtons);
    }

    connect(ui->switchDbButtons[buttonType::ADD_DB], &QPushButton::clicked,
            this, &MainWindow::addDbButtonClicked);
    connect(ui->switchDbButtons[buttonType::REMOVE_DB], &QPushButton::clicked,
            this, &MainWindow::removeDbButtonClicked);
    connect(ui->switchDbButtons[buttonType::PREVIOUS_DB], &QPushButton::clicked,
            this, &MainWindow::previousDbButtonClicked);
    connect(ui->switchDbButtons[buttonType::NEXT_DB], &QPushButton::clicked,
            this, &MainWindow::nextDbButtonClicked);

    connect(ui->serverNameLineEdit, &QLineEdit::editingFinished, this,
            [this]() -> void { if (this->_currentSession->noOfDatabases() > 0) saveValue(); } );
    connect(ui->sqlPortNoLineEdit, &QLineEdit::editingFinished, this,
            [this]() -> void { if (this->_currentSession->noOfDatabases() > 0) saveValue(); } );
    connect(ui->databaseNameLineEdit, &QLineEdit::editingFinished, this,
            [this]() -> void { if (this->_currentSession->noOfDatabases() > 0) saveValue(); } );
    connect(ui->userNameLineEdit, &QLineEdit::editingFinished, this,
            [this]() -> void { if (this->_currentSession->noOfDatabases() > 0) saveValue(); } );
    connect(ui->passwordLineEdit, &QLineEdit::editingFinished, this,
            [this]() -> void { if (this->_currentSession->noOfDatabases() > 0) saveValue(); } );

    connect(ui->handleDbButtons[buttonType::SAVE], &QPushButton::clicked,
            this, &MainWindow::saveButtonClicked);
    connect(ui->handleDbButtons[buttonType::REFRESH], &QPushButton::clicked,
            this, &MainWindow::refreshButtonClicked);
    connect(ui->connectToServerButton, &QPushButton::clicked, this, &MainWindow::connectToServerButtonClicked);
    connect(ui->quitButton, &QPushButton::clicked, this, &QApplication::quit);
}

MainWindow::~MainWindow() {

    delete ui;
}

bool MainWindow::switchDbActionAfterButtonClicked(const Database::dbPosition switchToDbPos) {

    QUuid dbAfterSwitchID = QUuid();
    const bool dbSwitched =
        _currentSession->switchUserDatabase(switchToDbPos, dbAfterSwitchID);

    if (dbSwitched) {

        this->clearWindowContents();
        this->fillFormWithBasicData(dbAfterSwitchID);
        _currentSession->changeCurrentDbTo(dbAfterSwitchID);

        this->fillLogTableContents();

        // if current (after switch) db is connected => display its settings
        Database * const newDB = _currentSession->db(_currentSession->currentUserDatabaseID());
        if (newDB->connectionEstablished()) {

            QMap<Database::dbSettings, QString> dbSettings;
            if (_currentSession->retrieveUserDbSettings(dbSettings))
                this->fillFormWithSettings(dbSettings);
        }

        this->enableButtons(ui->switchDbButtons, { buttonType::PREVIOUS_DB, buttonType::NEXT_DB });
        if (newDB == _currentSession->dbs().first())
            this->disableButtons(ui->switchDbButtons, { buttonType::PREVIOUS_DB });
        if (newDB == _currentSession->dbs().last())
            this->disableButtons(ui->switchDbButtons, { buttonType::NEXT_DB });
    }

    return dbSwitched;
}

void MainWindow::saveValue() {

    const QString currentWidget = QObject::sender()->objectName();
    const QString newValue = static_cast<QLineEdit *>(QObject::sender())->text();
    DatabaseConnectionProps * properties =
        _currentSession->db(_currentSession->currentUserDatabaseID())->connectionProperties();

    properties->setValueToVariable(sql::map::widgetToVariable[currentWidget], newValue);

    return;
}

void MainWindow::refreshDbIDLabel(const QUuid ID, const int databaseID) {

    const QString databaseID_str = (databaseID == -1) ? QStringLiteral("nová databáze")
                  : QStringLiteral("databaseID: ") + QString::number(databaseID);

    const QString ID_str = QStringLiteral("<b>") + ID.toString(QUuid::WithoutBraces) +
        QStringLiteral("</b> (") + databaseID_str + QStringLiteral(")");

    ui->dbIDLabel->setText(ID_str);

    return;
}

void MainWindow::fillFormWithBasicData(const QUuid currentDbID) {

    Database * const currentDB = _currentSession->db(currentDbID);

    this->refreshDbIDLabel(currentDbID, currentDB->databaseID());
    ui->serverNameLineEdit->setText(currentDB->connectionProperties()->serverName());
    ui->sqlPortNoLineEdit->setText(currentDB->connectionProperties()->portNo());
    ui->databaseNameLineEdit->setText(currentDB->dbName());
    ui->userNameLineEdit->setText(currentDB->connectionProperties()->userName());

    return;
}

void MainWindow::fillFormWithSettings(QMap<Database::dbSettings, QString> & settings) {

    ui->lastDatabaseBackupLineEdit->setText(checkForValues(settings[Database::LAST_FULL_BACKUP]));
    ui->lastDiffDatabaseBackupLineEdit->setText(checkForValues(settings[Database::LAST_DIFF_BACKUP]));
    ui->lastLogBackupLineEdit->setText(checkForValues(settings[Database::LAST_LOG_BACKUP]));
    ui->recoveryModelLineEdit->setText(settings[Database::RECOVERY_MODEL]);
    ui->dbStatusLineEdit->setText(settings[Database::STATE]);

    return;
}

void MainWindow::fillLogTableContents() {

    Database * currentDB = _currentSession->db(_currentSession->currentUserDatabaseID());
    currentDB->initializeLogTable(currentDB->logTableName());

    delete ui->logTableView;
    ui->logTableView = new QTableView();
    ui->logTableView->setModel(currentDB->logTable());
    ui->windowLayout->insertWidget(3, ui->logTableView);
    return;
}

void MainWindow::clearWindowContents() const {

    const QList<QLineEdit *> allQLineEditFields = this->findChildren<QLineEdit *>();

    for (auto it: allQLineEditFields)
        it->clear();

    return;
}

void MainWindow::enableButtons(const QMap<buttonType, QPushButton *> & groupOfButtons,
                               const QList<buttonType> & listOfButtons, const bool enable) const {

    // enable all buttons from list which belong to particular group
    for (auto it: groupOfButtons.keys()) {

        if (!listOfButtons.isEmpty() && listOfButtons.contains(it))
            groupOfButtons[it]->setEnabled(enable);
        else
            groupOfButtons[it]->setEnabled(!enable);
    }
    return;
}

void MainWindow::disableButtons(const QMap<buttonType, QPushButton *> & groupOfButtons,
                                const QList<buttonType> & listOfButtons) const {

    enableButtons(groupOfButtons, listOfButtons, false);
    return;
}

void MainWindow::disableButtons(const QVector<QPair<const QMap<buttonType, QPushButton *> &,
                                                    const QList<buttonType> &>> buttonCategories) const {
    for (auto it : buttonCategories)
        disableButtons(it.first, it.second);
    return;
}

// [slot]
bool MainWindow::addDbButtonClicked() {

    this->clearWindowContents();
    const QList<buttonType> buttonsToDisable
        { buttonType::ADD_DB, buttonType::PREVIOUS_DB, buttonType::NEXT_DB };
    this->disableButtons(ui->switchDbButtons, buttonsToDisable);
    this->enableButtons(ui->handleDbButtons);

    const QUuid newDbID = _currentSession->addNewUserDatabase();

    if (!newDbID.isNull()) {

        fillFormWithBasicData(newDbID);
        _currentSession->changeCurrentDbTo(newDbID);
        return true;
    }

    ErrorMessage::warning(QStringLiteral("Nelze přidat novou databázi."));
    return false;
}

// [slot]
bool MainWindow::removeDbButtonClicked() {

    if (!_currentSession->isUserDbNew()) {

        const QMessageBox::StandardButton removeDb =
            ErrorMessage::question(QStringLiteral("Přejete si vybranou databázi opravdu smazat?"));
        if (removeDb != QMessageBox::Yes)
            return false;
    }

    // erase from list of user DBs
    QUuid currentDatabaseID = QUuid();
    _currentSession->removeUserDatabase(currentDatabaseID);

    // display first DB from list of tracked DBs (if exists)
    if (_currentSession->noOfDatabases() > 0) {

        this->fillFormWithBasicData(currentDatabaseID);
        this->refreshDbIDLabel(currentDatabaseID, _currentSession->db(currentDatabaseID)->databaseID());

        QList<buttonType> buttonsToEnable { buttonType::ADD_DB };
        if (_currentSession->noOfDatabases() > 0)
            buttonsToEnable << buttonType::REMOVE_DB;
        if (_currentSession->noOfDatabases() > 1)
            buttonsToEnable << buttonType::NEXT_DB;
        this->enableButtons(ui->switchDbButtons, buttonsToEnable);
        this->disableButtons(ui->handleDbButtons);
    }
    else {

        this->clearWindowContents();
        ui->dbIDLabel->setText(sql::noDatabasesToTrack);
        QList<buttonType> buttonsToEnable { buttonType::ADD_DB };
        this->enableButtons(ui->switchDbButtons, buttonsToEnable);
        this->enableButtons(ui->handleDbButtons);
    }

    _currentSession->changeCurrentDbTo(currentDatabaseID);
    return true;
}

// [slot]
bool MainWindow::previousDbButtonClicked() {

    // if current db is the first one => no previous db
    if (_currentSession->db(_currentSession->currentUserDatabaseID()) == _currentSession->dbs().first())
        return false;

    return (this->switchDbActionAfterButtonClicked(Database::PREVIOUS_DB));
}

// [slot]
bool MainWindow::nextDbButtonClicked() {

    // if current db is the last one => no next db
    if (_currentSession->db(_currentSession->currentUserDatabaseID()) == _currentSession->dbs().last())
        return false;

    return (this->switchDbActionAfterButtonClicked(Database::NEXT_DB));
}

// [slot]
bool MainWindow::saveButtonClicked() {

    if (_currentSession->db(_currentSession->currentUserDatabaseID())->connectionEstablished()) {

        bool dbConfigurationSaved = false;

        // test is valid only before the configuration is saved
        if (_currentSession->isUserDbNew()) {

            // save current (new) configuration
            dbConfigurationSaved = _currentSession->saveUserDbConfiguration();

            if (dbConfigurationSaved) {

                // display database ID
                refreshDbIDLabel(_currentSession->currentUserDatabaseID(),
                     _currentSession->db(_currentSession->currentUserDatabaseID())->databaseID());
                // enable state buttons
                QList<buttonType> buttonsToEnable { buttonType::ADD_DB, buttonType::REMOVE_DB };
                if (_currentSession->noOfDatabases() > 1)
                    buttonsToEnable << buttonType::PREVIOUS_DB;
                this->enableButtons(ui->switchDbButtons, buttonsToEnable);
                // display message box
                ErrorMessage::information(QStringLiteral("Sledování změn v databázi nastaveno."));
            }
        }
        else
            dbConfigurationSaved = _currentSession->saveUserDbConfiguration();

        return dbConfigurationSaved;
    }

    ErrorMessage::warning(QStringLiteral("Databáze není připojená."));
    return false;
}

bool MainWindow::refreshButtonClicked() {

    if (_currentSession->db(_currentSession->currentUserDatabaseID())->connectionEstablished()) {

        bool dbTrackingRefreshed = false;

        dbTrackingRefreshed = _currentSession->loadRecordsFromLog();
        // _currentSession->loadRecordsFromLogBackup();

        // display message box
        ErrorMessage::information(QStringLiteral("Záznamy o provedených změnách v databázi byly aktualizovány."));

        return dbTrackingRefreshed;
    }

    ErrorMessage::warning(QStringLiteral("Databáze není připojená."));
    return false;
}

// [slot]
bool MainWindow::connectToServerButtonClicked() {

    if (_currentSession->connectToUserDatabase()) {

        QMap<Database::dbSettings, QString> dbSettings;

        if (_currentSession->retrieveUserDbSettings(dbSettings)) {

            this->fillFormWithSettings(dbSettings);
            if (_currentSession->isUserDbNew()) {

                QList<buttonType> buttonsToEnable { buttonType::SAVE };
                this->enableButtons(ui->handleDbButtons, buttonsToEnable);
            }
            return true;
        }
    }
    return false;
}
