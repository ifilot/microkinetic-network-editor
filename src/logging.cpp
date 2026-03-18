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

#include "logging.h"

#include <QDateTime>

#include <cstdlib>
#include <iostream>

std::shared_ptr<QStringList> g_log_messages = std::make_shared<QStringList>();

/**
 * @brief Route a Qt log message to both the in-memory log list and standard streams.
 *
 * This handler normalizes the incoming text to local 8-bit encoding and prefixes each
 * entry with a timestamp and severity tag. It appends the formatted line to the global
 * `g_log_messages` buffer so the debug window can display it later. It also mirrors output
 * to `std::cout`/`std::cerr`, and aborts the process for fatal messages.
 *
 * @param type Severity of the Qt message that determines tagging and output stream selection.
 * @param context Source-location metadata supplied by Qt (currently unused by the handler).
 * @param msg Original log payload emitted by Qt.
 */
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
