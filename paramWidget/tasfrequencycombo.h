#ifndef TASFREQUENCYCOMBO_H
#define TASFREQUENCYCOMBO_H
#pragma pack(push, 8)
#include <QComboBox>
#include "Basic/tascontrolformat.h"
#pragma pack(8)

namespace TasControl {
inline void setupFrequencyCombo(QComboBox* combo) {
    combo->clear();
    combo->setEditable(false);
    for (int code = 0; code <= 80; code += 10)
        combo->addItem(QString::number(9.0 + code * 0.01, 'f', 1), code);
    combo->addItem(QStringLiteral("跳频"), 90);
    combo->setCurrentIndex(0);
}

inline void restoreFrequencyCombo(QComboBox* combo, int savedCode) {
    const int index = combo->findData(savedCode);
    // 旧配置可能保存了临时频点。留空，避免误把另一频点保存或下发。
    combo->setCurrentIndex(TasControl::frequencyIndex(savedCode) >= 0 ? index : 0);
}
}
#pragma pack(pop)
#endif
