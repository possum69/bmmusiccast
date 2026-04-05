#ifndef NEWSTATION_H
#define NEWSTATION_H

#include <QDialog>
#include <QObject>
#include "./ui_newstation.h"
#include <QNetworkAccessManager>

class newstation : public QDialog
{
    Q_OBJECT

public:
    explicit newstation(QWidget *parent = nullptr);
    ~newstation();
    QString selectedUrl;

private slots:
    void executeCmd(const QString& cmd);

private:
    Ui::NewStation ui;
    QNetworkAccessManager *networkManager_;
private: // constants
    QString baseUrl = "https://de1.api.radio-browser.info/json/";
    QString listTags = "tags";
    QString listByTag = "stations/bytag/"; // tag
    QString searchByName = "stations/search?name="; // name

    std::vector<QString> tags, urls;
};

#endif // NEWSTATION_H
