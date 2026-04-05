#include "newstation.h"

#include "newstation.h"
#include <QHostAddress>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

newstation::newstation(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    networkManager_ = new QNetworkAccessManager();

    connect(ui.search_pushButton, &QPushButton::clicked, [this](){
        if(ui.all_radioButton->isChecked()) {
            executeCmd(this->listTags);
        }
        else if(ui.selection_lineEdit->text().size() < 1) {
            return;
        }
        else if(ui.genre_radioButton->isChecked()) {
            executeCmd(QString("%1%2").arg(this->listByTag).arg(ui.selection_lineEdit->text()));
        }
        else if(ui.name_radioButton->isChecked()) {
            executeCmd(QString("%1%2").arg(this->searchByName).arg(ui.selection_lineEdit->text()));
        }
    });

    connect(ui.listWidget, &QListWidget::currentRowChanged, [this](int row) {
        qDebug() << "currentRowChanged=" << row;
        if(ui.all_radioButton->isChecked()) {
            if(row >=0 && row < this->tags.size()) {
                ui.selection_lineEdit->setText(this->tags.at(row));
            }
        }
        else if(ui.genre_radioButton->isChecked() || ui.name_radioButton->isChecked()) {
            if(row >=0 && row < this->urls.size()) {
                ui.url_lineEdit->setText(this->urls.at(row));
            }
        }
    });

    connect(ui.buttonBox, &QDialogButtonBox::accepted, [this](){
        selectedUrl = ui.url_lineEdit->text();
        qDebug() << "selectedUrl: " << selectedUrl;
        this->close();
    });
    connect(ui.buttonBox, &QDialogButtonBox::rejected, [this](){
        selectedUrl = "";
        qDebug() << "selectedUrl: " << selectedUrl;
        this->close();
    });
}

newstation::~newstation() {
    networkManager_->deleteLater();
}

void newstation::executeCmd(const QString& cmd) {
    qDebug() << "executeCmd " << cmd;

    QUrl url(QString("%1%2").arg(this->baseUrl).arg(cmd));

    QNetworkRequest request(url);
    auto *reply = networkManager_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, cmd]() {
        ui.listWidget->clear();
        if(cmd == this->listTags) {
            this->tags.clear();
        }
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            //qDebug() << "QJsonDocument: " << doc;
            if(doc.isArray()) {
                auto all = doc.array();
                int counter = 1;
                for(auto item: all) {
                    auto obj = item.toObject();
                    //qDebug() << "QObject: " << obj;
                    if(obj.contains("name")) {
                        auto name = obj.value("name").toString();
                        if(cmd == this->listTags) {
                            if(obj.contains("stationcount")) {
                                auto stationcount = obj.value("stationcount").toInt();
                                ui.listWidget->addItem(QString("%3=%1: %2").arg(name).arg(stationcount).arg(counter));
                                this->tags.push_back(name);
                            }
                        } else {
                            if(obj.contains("url_resolved")) {
                                auto url_resolved = obj.value("url_resolved").toString();
                                ui.listWidget->addItem(QString("%2=%1").arg(name).arg(counter));
                                this->urls.push_back(url_resolved);
                            }
                        }
                    }
                    counter++;
                }
            }
        }
        reply->deleteLater();
    });
}
