#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_btekcomm.h"

class btekcomm : public QMainWindow
{
    Q_OBJECT

public:
    btekcomm(QWidget *parent = Q_NULLPTR);

private:
    Ui::bridgetekClass ui;
};
