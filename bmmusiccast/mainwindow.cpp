#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "logic/communication.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    communication_ = new Communication();
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete communication_;
}
