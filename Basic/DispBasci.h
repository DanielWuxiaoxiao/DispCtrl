#ifndef DISPBASCI_H
#define DISPBASCI_H

#include <QColor>
#include <qnamespace.h>


//COLOR
constexpr int DET_COLOR = Qt::green;
constexpr int TRA_COLOR = Qt::red;

//STRING
constexpr char APP_NAME[] = "雷达控制平台";
constexpr char APP_NAME_E[] = "Radar Control Platform";
constexpr char DET_LABEL[] = "检测点";
constexpr char TRA_LABEL[] = "跟踪点";

//RANGE
constexpr float MIN_RANGE = 0.0;
constexpr float MAX_RANGE = 5000.0;  //单位m

//ZVALUE
constexpr int TOOL_TIP_Z = 1000;
constexpr int POINT_Z = 10;
constexpr int INFO_Z = 50;
constexpr int LINE_Z = 9;
constexpr int MAP_Z = -5;

//font
constexpr int MAIN_FONT_SIZE = 9;

#endif // DISPBASCI_H
