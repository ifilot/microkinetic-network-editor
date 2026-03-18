/**************************************************************************
 *   This file is part of MICROKINETIC NETWORK EDITOR.                    *
 *                                                                        *
 *   Author: Ivo Filot <ivo@ivofilot.nl>                                  *
 *                                                                        *
 *   MICROKINETIC NETWORK EDITOR (MNE) is free software:                  *
 *   you can redistribute it and/or modify it under the terms of the      *
 *   GNU General Public License as published by the Free Software         *
 *   Foundation, either version 3 of the License, or (at your option)     *
 *   any later version.                                                   *
 *                                                                        *
 *   MNE is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QIcon>
#include <QSurfaceFormat>

#include "config.h"
#include "logging.h"
#include "main_window.h"

int main(int argc, char* argv[]) {
    qInstallMessageHandler(message_output);
    QSurfaceFormat format;
    format.setSamples(8);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Microkinetic Network Editor");
    QCoreApplication::setOrganizationDomain("mne.local");
    app.setApplicationName(kProgramName);
    app.setApplicationVersion(kProgramVersion);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/mng-icon.png")));

    qInfo() << "Starting" << kProgramName << kProgramVersion;

    MainWindow window;
    window.show();
    return app.exec();
}
