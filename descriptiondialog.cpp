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

#include <QLineEdit>

#include "descriptiondialog.h"
#include "ui_descriptiondialog.h"
#include "wsdb.h"

descriptionDialog::descriptionDialog(std::vector<std::string> desc, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::descriptionDialog),
    cur(desc) {
    ui->setupUi(this);

    for (size_t count = 0; count < cur.size(); count++)
        addRow();
}

std::vector<std::string> descriptionDialog::desc() {
    std::vector<std::string> vec;
    size_t num = edit.size();

    for (size_t count = 0; count < num; count++) {
        vec.push_back(std::string(edit[count]->text().toUtf8()));
    }

    return vec;
}

descriptionDialog::~descriptionDialog() {
    delete ui;
}

void descriptionDialog::on_addWorkstation_clicked() {
    addRow();

    //ui->scrollArea->adjustSize();
    //ui->scrollArea->ensureWidgetVisible(ui->addWorkstation);
}

void descriptionDialog::on_removeWorkstation_clicked() {
    removeRow();
}

void descriptionDialog::addRow() {
    QLineEdit *widget = new QLineEdit();

    size_t row = ui->workstationForm->rowCount();
    if (row < cur.size())
        widget->setText(QString::fromUtf8(cur[row].c_str()));

    edit.push_back(widget);
    ui->workstationForm->addRow(QString::fromUtf8(Wsdb::workstationName(row).c_str()), widget);
}

void descriptionDialog::removeRow() {
    int row = ui->workstationForm->rowCount();

    if (row > 1) {
        edit.pop_back();
        ui->workstationForm->removeRow(row - 1);
    }
}
