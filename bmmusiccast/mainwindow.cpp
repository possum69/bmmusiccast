#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "logic/communication.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    auto com = new communication();
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
