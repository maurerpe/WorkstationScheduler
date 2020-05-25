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

#include <QDate>
#include <QMainWindow>
#include <QString>
#include <QTableWidget>

#include "threadeddb.h"

namespace Ui {
class WorkstationScheduler;
}

class WorkstationScheduler : public QMainWindow {
    Q_OBJECT

public:
    static const QDate epoch;
    static const int slotsPerDay;
    static const int64_t refreshInterval;

    explicit WorkstationScheduler(QWidget *parent = 0);
    ~WorkstationScheduler();

    void refreshAll();
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

private:
    void timerEvent(QTimerEvent *event);
    void chooseColor(QLineEdit *text, const QString &title);
    void buildWorkstationCombo();
    QString defaultBookAs();
    QDate workstationStartDate();
    void refreshDaily();
    void refreshWorkstation();
    void doBookRelease(bool isBooking);
    void book(int workstation, QDate &date, int slotStart, int slotStop, QString &name, int64_t attr, DbInsertNameCallback *cb);
    void release(int workstation, QDate &date, int slotStart, int slotStop);

    static void setupRows(QTableWidget *table);
    static QTableWidgetItem *newTableWidgetItem(const char *name, uint64_t attr);

private:
    Ui::WorkstationScheduler *ui;
    ThreadedDb tdb;
    bool isUpdatingCombo;
    int64_t lastDailyRefresh;
    int64_t lastWorkstationRefresh;
};

#endif // WORKSTATIONSCHEDULER_H
