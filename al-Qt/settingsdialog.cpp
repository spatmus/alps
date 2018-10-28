#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setText(QString t)
{
    ui->plainTextEdit->setPlainText(t);
}

QString SettingsDialog::getText()
{
    return ui->plainTextEdit->toPlainText();
}
