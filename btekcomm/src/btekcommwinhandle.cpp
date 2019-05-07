
#include "btekcomm.h"
#include "btekcommwinhandle.h"

btekcomm* getmainWidget(QWidget *p_Widget)
{
  static btekcomm* p_widget = NULL;

  if (p_Widget != Q_NULLPTR) {
    p_widget = (btekcomm*)p_Widget;
  }

  return p_widget;
}