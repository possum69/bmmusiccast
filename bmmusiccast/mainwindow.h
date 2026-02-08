#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include <QHostAddress>
#include "logic/communication.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void displayMessage(const QString& message);
    void addDeviceFound(const QJsonObject& deviceInfo, const QHostAddress& addr);
    void onMessageReceived(const QString& request, const QJsonObject& message);

private slots:
    void resetDeviceList();
    void onDeviceListSelectionChanged(int currentRow);
    void onVolumeSliderChanged(int value);
    
signals:
    void executeCmd(const QString& cmd);

private:
    Ui::MainWindow *ui;
    Communication* communication_;
    void buildConnections();
    QString m_currentzone = "main";
};
#endif // MAINWINDOW_H
