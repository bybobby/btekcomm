#include <stdio.h>
#include <iostream>
#include "btekcomm.h"
#include <QtWidgets/QApplication>
#include "btekcommwinhandle.h"
#include "cwinbtek.h"

using namespace std;

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  btekcomm w;
  w.show();
  getmainWidget(&w);

  return a.exec();
}
