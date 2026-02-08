#ifndef Communication_H
#define Communication_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonObject>

class Communication : public QObject
{
    Q_OBJECT
private: // members
    QNetworkAccessManager* networkManager_;
    int m_pending = 0;
    QList<QHostAddress> m_devices;
    int m_selectedDeviceIndex = -1;
    QString lastAlbumArtUrl = "";

public: // methods
    Communication(/* args */);
    ~Communication();

public slots:
    void scanForDevices();
    void selectDevice(int index);
    void executeCmd(const QString& cmd);
    void downloadAlbumArt(const QString &albumart_url);

private slots:
    void queryDevice(const QHostAddress &addr);

signals:
    void scanFinished();
    void message(const QString& msg);
    void deviceFound(const QJsonObject& deviceInfo, const QHostAddress& addr);
    void addressFound(const QHostAddress& addr);
    void validFeedbackReceived(const QString& request, const QJsonObject& message);
    void update(const QString& request);
    void albumArtReady(const QImage &image);

private: // methods
    QList<QHostAddress> getLocalIPAddresses();
};

#endif
