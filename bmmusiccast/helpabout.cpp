#include "helpabout.h"
#include "ui_helpabout.h"

HelpAbout::HelpAbout(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::HelpAbout)
{
    ui->setupUi(this);
}

HelpAbout::~HelpAbout()
{
    delete ui;
}
