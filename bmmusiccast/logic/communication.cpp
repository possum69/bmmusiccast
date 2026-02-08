#include "communication.h"

Communication::Communication(/* args */)
{
    networkManager_ = new QNetworkAccessManager();
    connect(networkManager_, &QNetworkAccessManager::finished, this, &Communication::onFinished);
}

Communication::~Communication()
{
    networkManager_->deleteLater();
}
    
void Communication::onFinished(QNetworkReply* reply)
{
    lastResponseData_.clear();
    lastReply_ = reply;
    // Handle the finished network reply here
    if (reply->error() == QNetworkReply::NoError) {
        lastResponseData_ = reply->readAll();
        // Process the response data
    } else {
        qDebug() << "Network error:" << reply->errorString();
    }
    reply->deleteLater();
}
