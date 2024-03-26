/*******************************************************************************
 Copyright 2020 Daniel Neuwirth
 This program is distributed under the terms of the GNU General Public License.
*******************************************************************************/

/* Application:     DB Log Inspection and Maintenance Tool (dblogger.exe)
 * Version:         0.10
 * Worktime:        54 hrs
 *
 * Author:          Daniel Neuwirth
 * E-mail:          d.neuwirth@tiscali.cz
 * Created:         2020-06-06
 * Modified:        2020-08-04
 *
 * IDE/framework:   Qt 5.14.0
 * Compiler:        MinGW 7.3.0 32-bit
 * Language:        C++11
 */

#include <QApplication>
#include "mainwindow.h"
#include "session.h"

int main(int argc, char * argv[])
{
    Q_INIT_RESOURCE(resource);

    QApplication app(argc, argv);

    int exitValue = 0;

    Session * appSession = new Session;
    if (appSession->systemDatabase()->connectionEstablished()) {

        MainWindow mainWindow(appSession);
        mainWindow.show();

        exitValue = app.exec();
    }

    delete appSession;
    return exitValue;
}
