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

#include "descriptiondialog.h"
#include "ui_descriptiondialog.h"
#include "wsdb.h"

DescriptionDialog::DescriptionDialog(const std::vector<Wsdb::StationInfo> &info, const Wsdb::Limits &limits, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DescriptionDialog),
    cur(info) {
    ui->setupUi(this);
    setWindowTitle("Worksapce Info");

    for (size_t count = 0; count < cur.size(); count++)
        addRow();

    int yellow = limits.yellow > INT_MAX ? INT_MAX : limits.yellow < INT_MIN ? INT_MIN : static_cast<int>(limits.yellow);
    int red = limits.red > INT_MAX ? INT_MAX : limits.red < INT_MIN ? INT_MIN : static_cast<int>(limits.red);

    ui->yellowLimit->setValue(yellow);
    ui->redLimit->setValue(red);
}

std::vector<Wsdb::StationInfo> DescriptionDialog::info() {
    std::vector<Wsdb::StationInfo> vec;
    size_t num = name.size();

    for (size_t count = 0; count < num; count++) {
        vec.push_back(Wsdb::StationInfo(std::string(name[count]->text().toUtf8()),
                                        std::string(desc[count]->text().toUtf8()),
                                        exclude[count]->isChecked() ? 1 : 0));
    }

    return vec;
}

Wsdb::Limits DescriptionDialog::limits() {
    return Wsdb::Limits(ui->yellowLimit->value(), ui->redLimit->value());
}

DescriptionDialog::~DescriptionDialog() {
    delete ui;
}

void DescriptionDialog::on_addWorkstation_clicked() {
    addRow();
}

void DescriptionDialog::on_removeWorkstation_clicked() {
    removeRow();
}

void DescriptionDialog::addRow() {
    QLineEdit *nameWidget = new QLineEdit();
    QLineEdit *descWidget = new QLineEdit();
    QCheckBox *excludeWidget = new QCheckBox(QString::fromUtf8("Exclude"));

    int rowInt = ui->workstationGrid->count() / ui->workstationGrid->columnCount();
    if (rowInt < 1)
        return;
    size_t row = static_cast<size_t>(rowInt) - 1;

    if (row < cur.size()) {
        nameWidget->setText(QString::fromUtf8(cur[row].name.c_str()));
        descWidget->setText(QString::fromUtf8(cur[row].desc.c_str()));
        excludeWidget->setChecked(cur[row].flags & 1);
    } else {
        nameWidget->setText(QString::fromUtf8(Wsdb::defaultWorkstationName(row).c_str()));
    }

    name.push_back(nameWidget);
    desc.push_back(descWidget);
    exclude.push_back(excludeWidget);
    ui->workstationGrid->addWidget(nameWidget, rowInt, 0);
    ui->workstationGrid->addWidget(descWidget, rowInt, 1);
    ui->workstationGrid->addWidget(excludeWidget, rowInt, 2);
    ui->scrollArea->adjustSize();
}

void DescriptionDialog::removeRow() {
    int rowInt = ui->workstationGrid->count() / ui->workstationGrid->columnCount();

    if (rowInt > 2) {
        ui->workstationGrid->removeWidget(name.back());
        ui->workstationGrid->removeWidget(desc.back());
        ui->workstationGrid->removeWidget(exclude.back());
        delete name.back();
        delete desc.back();
        delete exclude.back();
        name.pop_back();
        desc.pop_back();
        exclude.pop_back();
    }
}
