/**************************************************************************
 *   This file is part of MICROKINETIC NETWORK EDITOR.                    *
 *                                                                        *
 *   Author: Ivo Filot <ivo@ivofilot.nl>                                  *
 *                                                                        *
 *   MICROKINETIC NETWORK EDITOR is free software:                        *
 *   you can redistribute it and/or modify it under the terms of the      *
 *   GNU General Public License as published by the Free Software         *
 *   Foundation, either version 3 of the License, or (at your option)     *
 *   any later version.                                                   *
 *                                                                        *
 *   MANAGLYPH is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include "logging.h"

#include <QDateTime>

#include <cstdlib>
#include <iostream>

std::shared_ptr<QStringList> g_log_messages = std::make_shared<QStringList>();

void message_output(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    Q_UNUSED(context);

    const QString local_msg = QString::fromLocal8Bit(msg.toLocal8Bit());
    const QString timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz");

    switch (type) {
    case QtDebugMsg:
        g_log_messages->append(timestamp + " [DEBUG] " + local_msg);
        std::cout << "[DEBUG] " << msg.toStdString() << std::endl;
        break;
    case QtInfoMsg:
        g_log_messages->append(timestamp + " [INFO] " + local_msg);
        std::cout << "[INFO] " << msg.toStdString() << std::endl;
        break;
    case QtWarningMsg:
        g_log_messages->append(timestamp + " [WARNING] " + local_msg);
        std::cout << "[WARNING] " << msg.toStdString() << std::endl;
        break;
    case QtCriticalMsg:
        g_log_messages->append(timestamp + " [CRITICAL] " + local_msg);
        std::cerr << "[CRITICAL] " << msg.toStdString() << std::endl;
        break;
    case QtFatalMsg:
        g_log_messages->append(timestamp + " [FATAL] " + local_msg);
        std::cerr << "[FATAL] " << msg.toStdString() << std::endl;
        abort();
    }
}
