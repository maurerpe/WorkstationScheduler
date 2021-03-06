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

#ifndef DESCRIPTIONDIALOG_H
#define DESCRIPTIONDIALOG_H

#include <vector>

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>

#include "threadeddb.h"
#include "workstationscheduler.h"
#include "wsdb.h"

namespace Ui {
class DescriptionDialog;
}

class DescriptionDialog : public QDialog {
    Q_OBJECT

public:
    explicit DescriptionDialog(const std::vector<Wsdb::StationInfo> &info, const Wsdb::Limits &limits, WorkstationScheduler *ws, ThreadedDb *tdb);
    ~DescriptionDialog();

private slots:
    void on_addWorkstation_clicked();
    void on_removeWorkstation_clicked();
    void on_accepted();

private:
    std::vector<Wsdb::StationInfo> info();
    Wsdb::Limits limits();
    void addRow();
    void removeRow();

private:
    Ui::DescriptionDialog *ui;
    std::vector<Wsdb::StationInfo> cur;
    WorkstationScheduler *ws;
    ThreadedDb *tdb;
    std::vector<QLineEdit *> name;
    std::vector<QLineEdit *> desc;
    std::vector<QCheckBox *> exclude;
};

#endif // DESCRIPTIONDIALOG_H
