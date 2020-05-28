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

#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

#include <QBrush>
#include <QColor>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "dbcommand.h"
#include "descriptiondialog.h"
#include "workstationscheduler.h"
#include "ui_workstationscheduler.h"

static const size_t WsWorkstationComboRefresh = 1;
static const size_t WsDailyHeadersRefresh     = 2;
static const size_t WsDailyTableRefresh       = 3;
static const size_t WsWorkstationTableRefresh = 4;

const QDate WorkstationScheduler::epoch = QDate(2000,1,1);
const int WorkstationScheduler::slotsPerDay = 48;
const int64_t WorkstationScheduler::refreshInterval = 15 * 60; // seconds

class WsOpenCallback : public DbOpenCallback {
public:
    WsOpenCallback(QWidget *parent) : parent(parent) {}
    virtual void execute();

private:
    QWidget *parent;
};

void WsOpenCallback::execute() {
    QMessageBox::critical(parent, "Error opening database", QString::fromUtf8(errorMsg.c_str()));
}

WorkstationScheduler::WorkstationScheduler(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WorkstationScheduler),
    settings("maurerpe", "WorkstationScheduler"),
    isUpdatingCombo(false),
    lastDailyRefresh(0),
    lastWorkstationRefresh(0) {
    ui->setupUi(this);

    resize(settings.value("mainwindow/size", QSize(800, 600)).toSize());
    move(settings.value("mainwindow/pos", QPoint(200,200)).toPoint());

    QVariant db = settings.value("database/filename");
    if (db.isNull()) {
        selectDbFile();
    }  else {
        tdb.queueCommand(new DbOpenCommand(std::string(db.toString().toUtf8()), new WsOpenCallback(this)));
    }

    ui->bookAs->setText(defaultBookAs());
    setStyleColorToDefault();
    setDailyToToday();
    setWorkstationToToday();

    startTimer(15); // ms
}

WorkstationScheduler::~WorkstationScheduler() {
    settings.setValue("mainwindow/size", size());
    settings.setValue("mainwindow/pos", pos());
    settings.setValue("database/username", ui->bookAs->text());

    delete ui;
}

void WorkstationScheduler::refreshAll() {
    refreshDaily();
    refreshWorkstation();
}

void WorkstationScheduler::updateTable(std::list<DbSelectNamesCallback::Datum *> &data, bool isDaily) {
    QTableWidget *table = isDaily ? ui->dailyTable : ui->workstationTable;
    int cols = table->columnCount();

    for (int col = 0; col < cols; col++)
        for (int row = 0; row < slotsPerDay; row++)
            delete table->takeItem(row, col);

    QDate baseDay = isDaily ? ui->dailyDate->date() : workstationStartDate();
    int64_t baseSlot = epoch.daysTo(baseDay) * slotsPerDay;
    int64_t station = ui->workstationName->currentIndex();
    int64_t numBooked[slotsPerDay];

    for (int count = 0; count < slotsPerDay; count++)
         numBooked[count] = 0;

    while (!data.empty()) {
        std::unique_ptr<DbSelectNamesCallback::Datum> datum(data.front());
        data.pop_front();
        int64_t delta = datum->slot - baseSlot;
        int64_t col;
        int64_t row;
        if (isDaily) {
            if (delta < 0 || delta >= slotsPerDay || datum->station < 0)
                continue;
            col = datum->station + 1;
            row = delta;
        } else {
            if (datum->station != station)
                continue;
            col = delta / slotsPerDay;
            row = delta % slotsPerDay;
        }

        if (col < 0 || col >= cols)
            continue;

        QTableWidgetItem *item = WorkstationScheduler::newTableWidgetItem(datum->name.c_str(), datum->attr);
        table->setItem(static_cast<int> (row), static_cast<int> (col), item);
        numBooked[row]++;
    }

    if (isDaily) {
        for (int count = 0; count < slotsPerDay; count++) {
            std::stringstream ss;
            ss << numBooked[count];

            int64_t attr = 0xFFFFFF404040; // light gray text on white background
            if (numBooked[count] >= cols - 1)
                attr = 0xC00000FFFFFF; // white text on dark red background
            else if (numBooked[count] >= cols - 3)
                attr = 0xFFFF80000000; // black text on pale yellow background

            QTableWidgetItem *item = WorkstationScheduler::newTableWidgetItem(ss.str().c_str(), attr);
            item->setTextAlignment(Qt::AlignHCenter);
            table->setItem(count, 0, item);
        }
    }
}

void WorkstationScheduler::on_refresh_clicked() {
    refreshAll();
}

void WorkstationScheduler::on_book_clicked() {
    doBookRelease(true);

    refreshAll();
}

void WorkstationScheduler::on_release_clicked() {
    if (QMessageBox::warning(this, "Confirm Unbooking", "Are you sure you want to unbook the selected cells?\n\nThis cannot be undone.",
                             QMessageBox::Cancel | QMessageBox::Ok, QMessageBox::Cancel) != QMessageBox::Ok)
        return;

    doBookRelease(false);

    refreshAll();
}

void WorkstationScheduler::on_dailyDate_dateChanged(const QDate &) {
    refreshDaily();
}

void WorkstationScheduler::on_workstationName_currentIndexChanged(int) {
    if (!isUpdatingCombo)
        refreshWorkstation();
}

void WorkstationScheduler::on_workstationDate_dateChanged(const QDate &) {
    refreshWorkstation();
}

void WorkstationScheduler::on_foregroundButton_clicked() {
    chooseColor(ui->fgColor, QString::fromUtf8("Choose Foreground Color"));
}

void WorkstationScheduler::on_backgroundButton_clicked() {
    chooseColor(ui->bgColor, QString::fromUtf8("Choose Background Color"));
}

class WsDescriptionsCallback : public DbGetStationNamesCallback {
public:
    WsDescriptionsCallback(WorkstationScheduler *ws, ThreadedDb *tdb) : ws(ws), tdb(tdb) {}

    virtual void execute();

private:
    WorkstationScheduler *ws;
    ThreadedDb *tdb;
};

void WsDescriptionsCallback::execute() {
    descriptionDialog dlg(names, nullptr);

    if (!dlg.exec())
        return;

    tdb->queueCommand(new DbSetStationDescriptionsCommand(dlg.desc()));

    ws->refreshAll();
}

void WorkstationScheduler::on_actionWorkstationDescriptions_triggered() {
    tdb.queueCommand(new DbGetStationNamesCommand(true, false, new WsDescriptionsCallback(this, &tdb)));
}

void WorkstationScheduler::on_actionAbout_triggered() {
    QMessageBox::information(this, "About Workstation Scheduler",
                             "WorkstationScheduler version 0.2beta\n\n"
                             "www.github.com/maurerpe/WorkstationScheduler\n\n"
                             "Copyright 2020 Paul Maurer\n\n"
                             "Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n\n"
                             "The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n\n"
                             "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\n"
                             "This software uses Qt developed by The Qt Company licensed under LGPL v3. The source code is available from www.qt.io. You should have received a copy of the LGPL v3 with this software.");
}

void WorkstationScheduler::on_actionQuit_triggered() {
    QApplication::quit();
}

void WorkstationScheduler::on_actionOpen_Database_triggered() {
    selectDbFile();
}

void WorkstationScheduler::on_bold_stateChanged(int arg1) {
    QFont font = ui->bookAs->font();
    font.setBold(arg1);
    ui->bookAs->setFont(font);
}

void WorkstationScheduler::on_italic_stateChanged(int arg1) {
    QFont font = ui->bookAs->font();
    font.setItalic(arg1);
    ui->bookAs->setFont(font);
}

void WorkstationScheduler::on_fgColor_textChanged(const QString &arg1) {
    unsigned long long rgb = strtoull(arg1.toUtf8(), nullptr, 16);
    QPalette pallet = ui->bookAs->palette();
    pallet.setColor(QPalette::Text, QColor(static_cast<QRgb> (rgb)));
    ui->bookAs->setPalette(pallet);
}

void WorkstationScheduler::on_bgColor_textChanged(const QString &arg1) {
    unsigned long long rgb = strtoull(arg1.toUtf8(), nullptr, 16);
    QPalette pallet = ui->bookAs->palette();
    pallet.setColor(QPalette::Base, QColor(static_cast<QRgb> (rgb)));
    ui->bookAs->setPalette(pallet);
}

void WorkstationScheduler::on_defaultStyleColor_clicked() {
    setStyleColorToDefault();
}

void WorkstationScheduler::on_dailyToday_clicked() {
    setDailyToToday();
}

void WorkstationScheduler::on_workstationToday_clicked() {
    setWorkstationToToday();
}

void WorkstationScheduler::timerEvent(QTimerEvent *) {
    tdb.checkCallbacks();

    if (tdb.isProcessing())
        QApplication::setOverrideCursor(Qt::BusyCursor);
    else
        QApplication::restoreOverrideCursor();

    int64_t lastRefresh = lastDailyRefresh;
    if (lastRefresh > lastWorkstationRefresh)
        lastRefresh = lastWorkstationRefresh;
    if (QDateTime::currentSecsSinceEpoch() - lastRefresh >= refreshInterval)
        refreshAll();
}

void WorkstationScheduler::selectDbFile() {
    QFileDialog dlg(this);
    dlg.setWindowTitle("Open Database");
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setNameFilter("Database Files (*.db)");
    dlg.setViewMode(QFileDialog::Detail);
    dlg.setOptions(QFileDialog::DontConfirmOverwrite);
    dlg.setAcceptMode(QFileDialog::AcceptOpen);

    if (!dlg.exec())
        return;

    QStringList filenameList = dlg.selectedFiles();
    if (filenameList.size() != 1)
        return;

    QString filename = filenameList[0];

    if (!QFileInfo::exists(filename)) {
        if (QMessageBox::warning(this, "Confirm Database Creation", "Database " + filename + " does not exist.\n\nOk to create new database?", QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) != QMessageBox::Ok)
            return;
    }

    settings.setValue("database/filename", filename);
    tdb.queueCommand(new DbOpenCommand(std::string(filename.toUtf8()), new WsOpenCallback(this)));
    refreshAll();
}

void WorkstationScheduler::setStyleColorToDefault() {
    ui->bold->setChecked(false);
    ui->italic->setChecked(false);
    ui->fgColor->setText("000000");
    ui->bgColor->setText("FFFFFF");
}

void WorkstationScheduler::setDailyToToday() {
    ui->dailyDate->setDate(QDate::currentDate());
}

void WorkstationScheduler::setWorkstationToToday() {
    ui->workstationDate->setDate(QDate::currentDate());
}

void WorkstationScheduler::chooseColor(QLineEdit *text, const QString &title) {
    QColor color = QColorDialog::getColor(QColor(static_cast<QRgb> (strtoull(text->text().toUtf8(), nullptr, 16) | 0xFF000000)), this, title);

    if (color.isValid()) {
        std::stringstream str;

        str << std::setfill('0') << std::setw(6) << std::hex << ((static_cast<uint32_t> (color.rgb())) & 0xFFFFFF);
        text->setText(QString::fromUtf8(str.str().c_str()));
    }
}

class MakeTrue {
private:
    bool *pointer;
    bool orig;

public:
    MakeTrue(bool *ptr) {pointer = ptr; orig = *pointer; *pointer = true;}
    ~MakeTrue() {*pointer = orig;}
};

class WsBuildCombo : public DbGetStationNamesCallback {
public:
    WsBuildCombo(QComboBox *combo, bool *isUpdating) : combo(combo), isUpdating(isUpdating) {}

    virtual void execute();

protected:
    QComboBox *combo;
    bool *isUpdating;
};

void WsBuildCombo::execute() {
MakeTrue mt(isUpdating);
size_t len = static_cast<size_t> (combo->count());
size_t num = names.size();

if (len > num) {
    for (size_t count = num; count < len; count++)
        combo->removeItem(static_cast<int> (count));
    len = num;
}

for (size_t count = 0; count < len; count++)
    combo->setItemText(static_cast<int> (count), QString::fromUtf8(names[count].c_str()));

for (size_t count = len; count < num; count++)
    combo->addItem(QString::fromUtf8(names[count].c_str()));
}

void WorkstationScheduler::buildWorkstationCombo() {
    tdb.queueCommand(new DbGetStationNamesCommand(true, true, new WsBuildCombo(ui->workstationName, &isUpdatingCombo)), WsWorkstationComboRefresh);
}

QString WorkstationScheduler::defaultBookAs() {
    QString name = settings.value("database/username").toString();

    if (name.isEmpty())
        name = qgetenv("USER");
    if (name.isEmpty())
        name = qgetenv("USERNAME");

    return name;
}

QDate WorkstationScheduler::workstationStartDate() {
    QDate wd = ui->workstationDate->date();
    return wd.addDays(-(wd.dayOfWeek() % 7));
}

class WsSetColumnHeaders : public DbGetStationNamesCallback {
public:
    WsSetColumnHeaders(QTableWidget *table) : table(table) {}

    virtual void execute();

private:
    QTableWidget *table;
};

void WsSetColumnHeaders::execute() {
    table->setColumnCount(static_cast<int> (names.size() + 1));

    QTableWidgetItem *numItem = new QTableWidgetItem(QString::fromUtf8("Number booked"));
    table->setHorizontalHeaderItem(0, numItem);

    for (size_t count = 0; count < names.size(); count++) {
        delete table->takeHorizontalHeaderItem(static_cast<int> (count + 1));
        QTableWidgetItem *item = new QTableWidgetItem(QString::fromUtf8(names[count].c_str()));
        table->setHorizontalHeaderItem(static_cast<int> (count + 1), item);
    }
}

class WsUpdateTable : public DbSelectNamesCallback {
public:
    WsUpdateTable(WorkstationScheduler *ws, bool isDaily) : ws(ws), isDaily(isDaily) {}

    virtual void execute();

private:
    WorkstationScheduler *ws;
    bool isDaily;
};

void WsUpdateTable::execute()  {
    ws->updateTable(data, isDaily);
}

void WorkstationScheduler::refreshDaily() {
    setupRows(ui->dailyTable);

    tdb.queueCommand(new DbGetStationNamesCommand(false, true, new WsSetColumnHeaders(ui->dailyTable)), WsDailyHeadersRefresh);

    int64_t startSlot = epoch.daysTo(ui->dailyDate->date()) * slotsPerDay;
    tdb.queueCommand(new DbSelectNamesCommand(startSlot, startSlot + 47, 0, 0x7FFFFFFF, new WsUpdateTable(this, true)), WsDailyTableRefresh);

    lastDailyRefresh = QDateTime::currentSecsSinceEpoch();
}


void WorkstationScheduler::refreshWorkstation() {
    buildWorkstationCombo();

    setupRows(ui->workstationTable);
    QDate start = workstationStartDate();

    ui->workstationTable->setColumnCount(7);

    for (int count = 0; count < 7; count++) {
        delete ui->workstationTable->takeHorizontalHeaderItem(static_cast<int> (count));
        QTableWidgetItem *item = new QTableWidgetItem(start.addDays(count).toString(QString::fromUtf8("ddd yyyy-MM-dd")));
        ui->workstationTable->setHorizontalHeaderItem(count, item);
    }

    int64_t workstation = ui->workstationName->currentIndex();
    int64_t startSlot = epoch.daysTo(start) * slotsPerDay;
    tdb.queueCommand(new DbSelectNamesCommand(startSlot, startSlot + slotsPerDay * 7 - 1, workstation, workstation, new WsUpdateTable(this, false)), WsWorkstationTableRefresh);

    lastWorkstationRefresh = QDateTime::currentSecsSinceEpoch();
}

class WsBookCallback : public DbInsertNameCallback {
public:
    WsBookCallback(int64_t *bookCountA) : DbInsertNameCallback(bookCountA) {}

    virtual void execute();
};

void WsBookCallback::execute() {
}

class WsBookFinalCallback : public DbCallback {
public:
    WsBookFinalCallback(int64_t *bookCount, QStatusBar *status) :
        bookCount(bookCount), status(status) {}

    virtual void execute();

private:
    int64_t *bookCount;
    QStatusBar *status;
};

void WsBookFinalCallback::execute() {
   std::stringstream str;

   str << "Booked " << *bookCount << (*bookCount == 1 ? " slot" : " slots");
   status->showMessage(QString::fromUtf8(str.str().c_str()), 5000);

   delete bookCount;
}

void WorkstationScheduler::doBookRelease(bool isBooking) {
    QTableWidget *table;
    QDate date = QDate::currentDate();
    QDate wsd = workstationStartDate();
    int64_t workstation = 0;
    bool isDaily = ui->mainTab->currentIndex() == 0;
    QString name = ui->bookAs->text();
    int64_t attr =
            static_cast<int64_t> ((static_cast<uint64_t> (ui->italic->isChecked() & 1) << 49) |
                                  (static_cast<uint64_t> (ui->bold->isChecked() & 1) << 48) |
                                  (static_cast<uint64_t> (strtoull(ui->bgColor->text().toUtf8(), nullptr, 16) & 0xFFFFFF) << 24) |
                                  (static_cast<uint64_t> (strtoull(ui->fgColor->text().toUtf8(), nullptr, 16) & 0xFFFFFF)));
    int64_t *bookCount = nullptr;

    if (isDaily) {
        // Daily Tab
        table = ui->dailyTable;
        date = ui->dailyDate->date();
    } else {
        // Workstation Tab
        table = ui->workstationTable;
        workstation = ui->workstationName->currentIndex();
    }

    if (isBooking) {
        bookCount = new int64_t;
        *bookCount = 0;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    for (auto &range : ranges) {
        int rowStart = range.topRow();
        int rowStop  = range.bottomRow();
        int colStart = range.leftColumn();
        int colStop  = range.rightColumn();

        for (int col = colStart; col <= colStop; col++) {
            if (isDaily) {
                workstation = col - 1;
                if (workstation < 0)
                    continue;
            } else {
                date = wsd.addDays(col);
            }

            if (isBooking) {
                book(workstation, date, rowStart, rowStop, name, attr, new WsBookCallback(bookCount));
            } else {
                release(workstation, date, rowStart, rowStop);
            }
        }
    }

    if (isBooking)
        tdb.queueCommand(new DbNopCommand(new WsBookFinalCallback(bookCount, ui->statusBar)));
}

void WorkstationScheduler::book(int64_t workstation, QDate &date, int slotStart, int slotStop, QString &name, int64_t attr, DbInsertNameCallback *cb) {
    int64_t baseSlot = epoch.daysTo(date) * slotsPerDay;

    tdb.queueCommand(new DbInsertNameCommand(baseSlot + slotStart, baseSlot + slotStop, workstation, std::string(name.toUtf8()), attr, cb));
}

void WorkstationScheduler::release(int64_t workstation, QDate &date, int slotStart, int slotStop) {
    int64_t baseSlot = epoch.daysTo(date) * slotsPerDay;

    tdb.queueCommand(new DbRemoveNamesCommand(baseSlot + slotStart, baseSlot + slotStop, workstation));
}

void WorkstationScheduler::setupRows(QTableWidget *table) {
    table->setRowCount(slotsPerDay);

    for (int count = 0; count < slotsPerDay; count++) {
        delete table->takeVerticalHeaderItem(count);

        std::stringstream str;
        str << std::setfill('0') << std::setw(2) << (count >> 1);
        str << ((count & 1) ? ":30" : ":00");

        QTableWidgetItem *item = new QTableWidgetItem(QString::fromUtf8(str.str().c_str()));
        table->setVerticalHeaderItem(count, item);
    }
}

QTableWidgetItem *WorkstationScheduler::newTableWidgetItem(const char *name, int64_t attr) {
    uint64_t uattr = static_cast<uint64_t> (attr);
    QTableWidgetItem *item = new QTableWidgetItem(QString::fromUtf8(name));
    item->setForeground(QBrush(static_cast<QRgb> ((uattr & 0xFFFFFF) | 0xFF000000)));
    item->setBackground(QBrush(static_cast<QRgb> ((uattr >> 24) & 0xFFFFFF) | 0xFF000000));
    QFont font = item->font();
    font.setBold((uattr >> 48) & 1);
    font.setItalic((uattr >> 49) & 1);
    item->setFont(font);

    return item;
}
