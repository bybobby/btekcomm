/********************************************************************************
** Form generated from reading UI file 'btekcomm.ui'
**
** Created by: Qt User Interface Compiler version 5.12.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BTEKCOMM_H
#define UI_BTEKCOMM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_bridgetekClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *bridgetekClass)
    {
        if (bridgetekClass->objectName().isEmpty())
            bridgetekClass->setObjectName(QString::fromUtf8("bridgetekClass"));
        bridgetekClass->resize(600, 400);
        menuBar = new QMenuBar(bridgetekClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        bridgetekClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(bridgetekClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        bridgetekClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(bridgetekClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        bridgetekClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(bridgetekClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        bridgetekClass->setStatusBar(statusBar);

        retranslateUi(bridgetekClass);

        QMetaObject::connectSlotsByName(bridgetekClass);
    } // setupUi

    void retranslateUi(QMainWindow *bridgetekClass)
    {
        bridgetekClass->setWindowTitle(QApplication::translate("bridgetekClass", "bridgetek", nullptr));
    } // retranslateUi

};

namespace Ui {
    class bridgetekClass: public Ui_bridgetekClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BTEKCOMM_H
