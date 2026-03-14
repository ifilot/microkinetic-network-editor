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

#pragma once

#include <QMessageLogContext>
#include <QStringList>

#include <memory>

extern std::shared_ptr<QStringList> g_log_messages;

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
void message_output(QtMsgType type, const QMessageLogContext& context, const QString& msg);
