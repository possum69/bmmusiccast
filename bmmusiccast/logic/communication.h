#ifndef Communication_H
#define Communication_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

class Communication : public QObject
{
    Q_OBJECT
private:
    QNetworkAccessManager* networkManager_;
    QString lastResponseData_;
    QNetworkReply* lastReply_;
public:
    Communication(/* args */);
    ~Communication();

public slots:
    void onFinished(QNetworkReply* reply);
    
};

#endif
