#include "communication.h"

#include <QNetworkInterface>
#include <QList>
#include <QHostAddress>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QThread>

Communication::Communication(/* args */)
{
    networkManager_ = new QNetworkAccessManager();
}

Communication::~Communication()
{
    networkManager_->deleteLater();
}
    
void Communication::scanForDevices()
{
    m_devices.clear();
    emit message("Scanning for devices...");
    auto port = 80;
    QTcpSocket socket;

    const auto interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp) ||
            !(iface.flags() & QNetworkInterface::IsRunning) ||
            (iface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        connect(this, &Communication::addressFound, this, &Communication::queryDevice);
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol)
                continue;

            quint32 ip = entry.ip().toIPv4Address();
            quint32 mask = entry.netmask().toIPv4Address();
            quint32 network = ip & mask;
            quint32 broadcast = network | ~mask;

            for (quint32 addr = network + 1; addr < broadcast; ++addr) {
                QThread::create([this, addr, port]() -> void{
                    QTcpSocket* testSocket = new QTcpSocket();
                    testSocket->connectToHost(QHostAddress(addr), port);
                    if (testSocket->waitForConnected(100)) {
                        testSocket->disconnectFromHost();                        
                        //queryDevice(QHostAddress(addr));
                        emit addressFound(QHostAddress(addr));
                    } else {
                        emit message(QString("No response from %1").arg(QHostAddress(addr).toString()));
                    }
                    testSocket->deleteLater();
                    }
                )->start();
            }
        }
    }
}

void Communication::queryDevice(const QHostAddress &addr)
{
    emit message(QString("Querying %1...").arg(QHostAddress(addr).toString()));
    m_pending++;
    QUrl url(QString("http://%1/YamahaExtendedControl/v1/system/getDeviceInfo").arg(addr.toString()));
    QNetworkRequest req(url);   
    QNetworkReply *reply = networkManager_->get(req);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, addr]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            auto data = reply->readAll();
    
            QJsonDocument doc = QJsonDocument::fromJson(data);  
            if (doc.isObject()) {
                // Valid Yamaha response
                if (!m_devices.contains(addr)) {
                    m_devices.append(addr);
                }                
                emit deviceFound(doc.object(), addr);
            } else {
                //emit message(QString("Yamaha device found at %1 (invalid response) %2").arg(addr.toString()).arg(QString(data)));                
            }
        } else {
            //qDebug() << "Status code for" << addr.toString() << ":" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            //emit message(QString("Error querying %1: %2").arg(addr.toString()).arg(reply->errorString()));
        }

        if (--m_pending == 0) {
            emit message(QString("Scan finished."));
        } else {
            emit message(QString("Pending queries: %1").arg(m_pending));
        }
    });
}

void Communication::selectDevice(int index) {
    m_selectedDeviceIndex = index;
    qDebug() << "Selected device index:" << index << "of" << m_devices.size();
    executeCmd("system/getFeatures");
}


void Communication::executeCmd(const QString& cmd)
{
    if (m_selectedDeviceIndex < 0 || m_selectedDeviceIndex >= m_devices.size()) {
        emit message("No device selected.");
        return;
    }
    emit message(QString("%2 %1...").arg(QHostAddress(m_devices[m_selectedDeviceIndex]).toString()).arg(cmd));
    QUrl url(QString("http://%1/YamahaExtendedControl/v1/%2").arg(m_devices[m_selectedDeviceIndex].toString()).arg(cmd));
    QNetworkRequest req(url);   
    QNetworkReply *reply = networkManager_->get(req);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, cmd]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            auto data = reply->readAll();
    
            QJsonDocument doc = QJsonDocument::fromJson(data);  
            if (doc.isObject()) {
                qDebug() << "Response from" << QHostAddress(m_devices[m_selectedDeviceIndex]).toString() << "for command" << cmd << ":" << doc.toJson(QJsonDocument::Compact);
                // Valid Yamaha response
                if (doc.object().contains("response_code") && doc.object().value("response_code").toInt() == 0) {
                    emit message(QString("Command %2 executed successfully on %1").arg(QHostAddress(m_devices[m_selectedDeviceIndex]).toString()).arg(cmd));
                    emit validFeedbackReceived(cmd, doc.object());
                } else {
                    emit message(QString("Command %2 failed on %1: %3").arg(QHostAddress(m_devices[m_selectedDeviceIndex]).toString()).arg(cmd).arg(doc.object().value("response_code").toInt()));
                }
            } else {
                emit message(QString("Invalid response from %1 for command %2: %3").arg(QHostAddress(m_devices[m_selectedDeviceIndex]).toString()).arg(cmd).arg(QString(data)));
            }
        } else {
            qDebug() << "Status code for" << m_devices[m_selectedDeviceIndex].toString() << ":" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        }

    });
}
