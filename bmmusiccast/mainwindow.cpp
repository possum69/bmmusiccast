#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "logic/communication.h"
#include <QJsonObject>
#include <QJsonArray>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    communication_ = new Communication();
    ui->setupUi(this);

    buildConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete communication_;
}

void MainWindow::buildConnections() {
    connect(this, &MainWindow::executeCmd, communication_, &Communication::executeCmd);

    connect(ui->scan_pushButton, &QPushButton::clicked, this, [this] {
        ui->scan_pushButton->setEnabled(false);
        this->resetDeviceList();
        this->communication_->scanForDevices();
    });

    connect(ui->scan_pushButton, &QPushButton::clicked, this, &MainWindow::resetDeviceList);
    connect(ui->devices_listWidget, &QListWidget::currentRowChanged, this, &MainWindow::onDeviceListSelectionChanged);
    connect(ui->devices_listWidget, &QListWidget::currentRowChanged, communication_, &Communication::selectDevice);
    connect(communication_, &Communication::message, this, &MainWindow::displayMessage);
    connect(communication_, &Communication::deviceFound, this, &MainWindow::addDeviceFound);
    connect(ui->on_radioButton, &QRadioButton::clicked, this, [this] {
        communication_->executeCmd("main/setPower?power=on");
    });
    connect(ui->off_radioButton, &QRadioButton::clicked, this, [this] {
        communication_->executeCmd("main/setPower?power=standby");
    });
    connect(communication_, &Communication::validFeedbackReceived, this, &MainWindow::onMessageReceived);
    connect(ui->volume_horizontalScrollBar, &QScrollBar::valueChanged, this, &MainWindow::onVolumeSliderChanged);

    connect(ui->volume_up_pushButton, &QPushButton::clicked, this, [this] {
        emit executeCmd("main/setVolume?volume=up");
    });
    connect(ui->volume_down_pushButton, &QPushButton::clicked, this, [this] {
        emit executeCmd("main/setVolume?volume=down");
    });
}

void MainWindow::displayMessage(const QString& message) {
    ui->statusbar->showMessage(message);
    //ui->devices_listWidget->addItem(message);
    if (message == "Scan finished.") {
        ui->scan_pushButton->setEnabled(true);
    }   
}

void MainWindow::addDeviceFound(const QJsonObject& deviceInfo, const QHostAddress& addr)
{
    auto displayName = QString("%1 (%2)").arg(deviceInfo.value("model_name").toString()).arg(addr.toString());
    for(auto row=0; row < ui->devices_listWidget->model()->rowCount(); ++row) {
        auto item = ui->devices_listWidget->model()->index(row, 0).data(Qt::DisplayRole).toString();
        if (item.contains(addr.toString())) {
            return; // already in list
        }
    }
    ui->devices_listWidget->addItem(displayName);
}

void MainWindow::resetDeviceList() {
    ui->devices_listWidget->clear();
}

void MainWindow::onDeviceListSelectionChanged(int currentRow)
{
    auto item = ui->devices_listWidget->model()->index(currentRow, 0).data(Qt::DisplayRole).toString();
    ui->name_lineEdit->setText(item);
    
}

void MainWindow::onMessageReceived(const QString& request, const QJsonObject& message) {
    if (request == "system/getFeatures") {
        QJsonObject features = message;
        if (features.contains("zone") && features.value("zone").isArray()) {
            QJsonArray zones = features.value("zone").toArray();
            if (zones.size() > 0) {
                QJsonObject zone = zones[0].toObject();
                if (zone.contains("range_step") && zone.value("range_step").isArray()) {
                    QJsonArray range_step = zone.value("range_step").toArray();
                    for (const auto& step : range_step) {
                        if (step.toObject().value("id").toString() == "volume") {
                            auto volumeMax = step.toObject().value("max").toInt();
                            auto volumeMin = step.toObject().value("min").toInt();
                            auto volumeStep = step.toObject().value("step").toInt();
                            ui->volume_horizontalScrollBar->setRange(volumeMin, volumeMax);
                            ui->volume_horizontalScrollBar->setSingleStep(volumeStep);
                        }
                    }
                }
            }
        }
    }
}

void MainWindow::onVolumeSliderChanged(int value) {
    emit executeCmd(QString("main/setVolume?volume=%1").arg(value));
}
