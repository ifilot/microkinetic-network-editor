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

#include <QWidget>

#include <memory>

class QPlainTextEdit;
class QStringList;

/**
 * @brief Display and live-update the collected debug log messages in a separate window.
 *
 * The widget owns a read-only text box and periodically appends any newly emitted log lines
 * from the shared message list. It is configured as a non-modal top-level window so it can
 * stay open independently while the editor remains active.
 */
class LogWindow : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Construct the log window and initialize it with existing log lines.
     *
     * The constructor builds the scrollable text view, copies the current contents of the
     * shared message buffer, and starts a timer that polls for new entries every second.
     * It also applies window flags/icons used by the main application.
     *
     * @param log_messages Shared list that accumulates formatted log strings.
     * @param parent Optional parent widget used for Qt ownership.
     */
    explicit LogWindow(const std::shared_ptr<QStringList>& log_messages, QWidget* parent = nullptr);

public slots:
    /**
     * @brief Append any unread log lines from the shared buffer to the text view.
     *
     * The method compares the current list size against `lines_read_`, inserts only the new
     * lines, and advances the read cursor. This avoids rewriting the full log on each timer
     * tick while keeping the UI synchronized with background logging.
     */
    void update_log();

private:
    std::shared_ptr<QStringList> log_messages_;
    QPlainTextEdit* text_box_{nullptr};
    int lines_read_{0};
};
