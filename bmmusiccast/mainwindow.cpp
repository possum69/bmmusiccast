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
    emit ui->scan_pushButton->clicked(); // trigger initial scan
}

MainWindow::~MainWindow()
{
    delete ui;
    delete communication_;
}

void MainWindow::buildConnections() {
    // General
    connect(this, &MainWindow::executeCmd, communication_, &Communication::executeCmd);
   
    // Device
    connect(ui->scan_pushButton, &QPushButton::clicked, this, &MainWindow::resetDeviceList);
    connect(ui->scan_pushButton, &QPushButton::clicked, this, [this] {
        ui->scan_pushButton->setEnabled(false);
        this->resetDeviceList();
        this->communication_->scanForDevices();
    });
    connect(communication_, &Communication::deviceFound, this, &MainWindow::addDeviceFound);

    connect(ui->checkBox_album_image, &QCheckBox::checkStateChanged, [this](Qt::CheckState state) {
        disconnect(ui->album_lineEdit, &QLineEdit::textChanged,communication_,&Communication::searchArtistImage);
        disconnect(ui->track_lineEdit, &QLineEdit::textChanged,communication_,&Communication::searchTrackInfo);

        if(state == Qt::Checked) {
            ui->checkBox_track_image->setCheckState(Qt::Unchecked);
            // Artist
            connect(ui->album_lineEdit, &QLineEdit::textChanged,communication_,&Communication::searchArtistImage);
            emit ui->album_lineEdit->textChanged(ui->album_lineEdit->text());
        }
    });
    connect(ui->checkBox_track_image, &QCheckBox::checkStateChanged, [this](Qt::CheckState state) {
        disconnect(ui->album_lineEdit, &QLineEdit::textChanged,communication_,&Communication::searchArtistImage);
        disconnect(ui->track_lineEdit, &QLineEdit::textChanged,communication_,&Communication::searchTrackInfo);
        if(state == Qt::Checked) {
            ui->checkBox_album_image->setCheckState(Qt::Unchecked);
            // Artist
            connect(ui->track_lineEdit, &QLineEdit::textChanged,communication_,&Communication::searchArtistImage);
            emit ui->track_lineEdit->textChanged(ui->track_lineEdit->text());
        }
    });

    // Select device from list
    connect(ui->devices_listWidget, &QListWidget::currentRowChanged, this, &MainWindow::onDeviceListSelectionChanged);
    connect(ui->devices_listWidget, &QListWidget::currentRowChanged, communication_, &Communication::selectDevice);
    
    // Power
    connect(ui->on_radioButton, &QRadioButton::clicked, this, [this] {
        communication_->executeCmd("main/setPower?power=on");
    });
    connect(ui->off_radioButton, &QRadioButton::clicked, this, [this] {
        communication_->executeCmd("main/setPower?power=standby");
    });

    // Feedback from device
    connect(communication_, &Communication::validFeedbackReceived, this, &MainWindow::onMessageReceived);
    connect(communication_, &Communication::message, this, &MainWindow::displayMessage);

    // Volume
    connect(ui->volume_horizontalScrollBar, &QScrollBar::valueChanged, this, &MainWindow::onVolumeSliderChanged);

    connect(ui->volume_up_pushButton, &QPushButton::clicked, this, [this] {
        emit executeCmd(QString("main/setVolume?volume=up&step=%1").arg(ui->step_spinBox->value()));
    });
    connect(ui->volume_down_pushButton, &QPushButton::clicked, this, [this] {
        emit executeCmd(QString("main/setVolume?volume=down&step=%1").arg(ui->step_spinBox->value()));
    });
    // MUTE
    connect(ui->unmute_radioButton, &QRadioButton::clicked, this, [this] {
        emit executeCmd("main/setMute?enable=false");
    });
    connect(ui->mute_radioButton, &QRadioButton::clicked, this, [this] {
        emit executeCmd("main/setMute?enable=true");
    });

    // Presets
    connect(ui->preset_listWidget, &QListWidget::currentRowChanged, this, [this](int currentRow) {        
        ui->albumArtLabel->setText("Album:");
        ui->album_lineEdit->setText("");
        ui->track_lineEdit->setText("");
        emit executeCmd(QString("netusb/recallPreset?zone=%1&num=%2").arg(m_currentzone).arg(currentRow+1));
    });

    // Album art
    connect(this, &MainWindow::fetch, communication_, &Communication::downloadAlbumArt);
    connect(communication_, &Communication::albumArtReady, this, [this](const QImage &img) {
        QPixmap pixmap = QPixmap::fromImage(img);
        ui->albumArtLabel->setPixmap(pixmap.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    });


    // Abstract
    connect(communication_, &Communication::albumAbstract, ui->album_plainTextEdit, &QPlainTextEdit::setPlainText);
    connect(communication_, &Communication::trackAbstract, ui->album_plainTextEdit, &QPlainTextEdit::setPlainText);
}

void MainWindow::displayMessage(const QString& message) {
    if (message == "Scan finished.") {
        ui->scan_pushButton->setEnabled(true);
        ui->statusbar->showMessage(message);
    } else if(message.startsWith("Error")) {
        ui->statusbar->showMessage(message);
    } else {
        //ui->statusbar->showMessage(message);
        ui->statusbar->showMessage("");
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
    } else if(request == "netusb/getPresetInfo") {
        if(message.contains("preset_info") && message.value("preset_info").isArray()) {
            ui->preset_listWidget->clear();
            QJsonArray presetInfo = message.value("preset_info").toArray();
            for (const auto& preset : presetInfo) {
                ui->preset_listWidget->addItem(preset.toObject().value("text").toString());                
            }
        }
    } else if(request == "netusb/getPlayInfo") {
        ui->artist_lineEdit->setText(message.value("artist").toString());
        auto album = message.value("album").toString();
        if(album.size() > 0) {
            ui->album_lineEdit->setText(album);
            ui->track_lineEdit->setText(message.value("track").toString());
        } else {
            auto track = message.value("track").toString();
            if(track.contains(" - ")) {
                auto splitted = track.split("-", Qt::SkipEmptyParts);
                if(splitted.size() > 1) {
                    ui->album_lineEdit->setText(splitted.at(0).trimmed());
                    ui->track_lineEdit->setText(splitted.at(1).trimmed());
                } else {
                    ui->track_lineEdit->setText(track.trimmed());
                }
            }

        }
        emit fetch(message.value("albumart_url").toString());
    } else if(request == "system/getLocationInfo") {
        auto zoneList = message.value("zone_list");    
    } else if(request == "main/getStatus") {
        if(message.value("power").toString() == "on") {
            ui->on_radioButton->setChecked(true);
        } else {
            ui->off_radioButton->setChecked(true);
        }
        ui->volume_horizontalScrollBar->setMaximum(message.value("max_volume").toInt());
        ui->volume_horizontalScrollBar->setValue(message.value("volume").toInt());
        if(message.value("mute").toBool()) {
            ui->mute_radioButton->setChecked(true);
        } else {
            ui->unmute_radioButton->setChecked(true);
        }
    }
    else 
    {
        //qDebug() << "Received response for" << request << ":" << message;
    }
}

void MainWindow::onVolumeSliderChanged(int value) {
    emit executeCmd(QString("main/setVolume?volume=%1").arg(value));
}
