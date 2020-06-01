//////////////////////////////////////////////////////////////////////////////
// Copyright 2020 Paul Maurer
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#ifndef WORKSTATIONSCHEDULER_H
#define WORKSTATIONSCHEDULER_H

#include <list>

#include <QAction>
#include <QDate>
#include <QMainWindow>
#include <QPushButton>
#include <QString>
#include <QSettings>
#include <QTableWidget>

#include "threadeddb.h"
#include "wsdb.h"

namespace Ui {
class WorkstationScheduler;
}

class WorkstationScheduler : public QMainWindow {
    Q_OBJECT

public:
    static const QDate epoch;
    static const int slotsPerDay;
    static const int64_t refreshInterval;

    explicit WorkstationScheduler(QWidget *parent = nullptr);
    ~WorkstationScheduler();

    void refreshAll();
    void openDbFile(QString filename);
    void updateTable(std::list<DbSelectNamesCallback::Datum *> &data, bool isDaily);

private slots:
    void on_refresh_clicked();
    void on_book_clicked();
    void on_release_clicked();
    void on_dailyDate_dateChanged(const QDate &date);
    void on_workstationName_currentIndexChanged(int index);
    void on_workstationDate_dateChanged(const QDate &date);
    void on_foregroundButton_clicked();
    void on_backgroundButton_clicked();
    void on_actionWorkstationDescriptions_triggered();
    void on_actionAbout_triggered();
    void on_actionQuit_triggered();
    void on_actionOpenDatabase_triggered();
    void on_actionClearRecentDatabases_triggered();
    void on_bold_stateChanged(int arg1);
    void on_italic_stateChanged(int arg1);
    void on_defaultStyle_clicked();
    void on_dailyToday_clicked();
    void on_workstationToday_clicked();
    void on_takeFromCell_clicked();

private:
    void timerEvent(QTimerEvent *event);
    void saveSettings();
    void selectDbFile();
    void buildRecentDatabasesMenu();
    void setStyleToDefault();
    void setDailyToToday();
    void setWorkstationToToday();
    void setColor(QPushButton *button, QRgb color);
    QRgb getColor(QPushButton *button);
    void chooseColor(QPushButton *button, const QString &title);
    QString defaultBookAs();
    QDate workstationStartDate();
    void refreshInfo();
    void refreshDaily();
    void refreshWorkstation();
    void doBookRelease(bool isBooking);
    void book(int64_t workstation, QDate &date, int slotStart, int slotStop, QString &name, int64_t attr, DbInsertNameCallback *cb);
    void release(int64_t workstation, QDate &date, int slotStart, int slotStop);

    static void setupRows(QTableWidget *table);
    static QTableWidgetItem *newTableWidgetItem(const char *name, int64_t attr);

private:
    Ui::WorkstationScheduler *ui;
    QSettings settings;
    ThreadedDb tdb;
    bool isUpdating;
    int64_t lastRefresh;
    std::vector<int> dailyColumn;
    std::vector<int64_t> dailyStation;
    Wsdb::Limits limits;
    int ySaveOffset;
};

#endif // WORKSTATIONSCHEDULER_H
