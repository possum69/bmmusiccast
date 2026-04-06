#ifndef HELPABOUT_H
#define HELPABOUT_H

#include <QDialog>

namespace Ui {
class HelpAbout;
}

class HelpAbout : public QDialog
{
    Q_OBJECT

public:
    explicit HelpAbout(QWidget *parent = nullptr);
    ~HelpAbout();

private:
    Ui::HelpAbout *ui;
};

#endif // HELPABOUT_H
