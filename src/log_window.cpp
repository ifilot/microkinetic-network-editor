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

#include "log_window.h"

#include <QIcon>
#include <QPlainTextEdit>
#include <QStringList>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

LogWindow::LogWindow(const std::shared_ptr<QStringList>& log_messages, QWidget* parent)
    : QWidget(parent)
    , log_messages_(log_messages) {
    setWindowFlag(Qt::Window, true);
    setWindowFlag(Qt::WindowCloseButtonHint, true);
    setWindowFlag(Qt::WindowMinMaxButtonsHint, true);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowIcon(QIcon(QStringLiteral(":/icons/mng-icon.png")));
    setWindowTitle("Debug log");

    auto* layout = new QVBoxLayout(this);
    auto* scroll_area = new QScrollArea(this);
    layout->addWidget(scroll_area);

    auto* scroll_widget = new QWidget(scroll_area);
    auto* scroll_layout = new QVBoxLayout(scroll_widget);
    text_box_ = new QPlainTextEdit(scroll_widget);
    text_box_->setReadOnly(true);
    text_box_->setOverwriteMode(false);
    scroll_layout->addWidget(text_box_);
    scroll_area->setWidget(scroll_widget);
    scroll_area->setWidgetResizable(true);

    for (const auto& line : *log_messages_) {
        text_box_->appendPlainText(line);
    }
    lines_read_ = log_messages_->size();

    resize(1024, 420);
    setMinimumHeight(256);
    setMinimumWidth(900);
    hide();

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &LogWindow::update_log);
    timer->start(1000);
}

void LogWindow::update_log() {
    const int new_size = log_messages_->size();
    for (int i = lines_read_; i < new_size; ++i) {
        text_box_->appendPlainText(log_messages_->at(i));
    }
    lines_read_ = new_size;
}
