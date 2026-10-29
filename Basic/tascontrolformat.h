#ifndef TASCONTROLFORMAT_H
#define TASCONTROLFORMAT_H

#include "Basic/Protocol.h"
#include <QString>

namespace TasControl {
// All TAS options use index*10; index 9 is random hopping (90/0x5A).
inline int frequencyCode(int index) { return index >= 0 && index <= 9 ? index * 10 : -1; }
inline int frequencyIndex(int code) { return code >= 0 && code <= 90 && code % 10 == 0 ? code / 10 : -1; }
inline QString frequencyText(int code) {
    if (code == 90) return QStringLiteral("随机跳频(freqID=90/0x5A)");
    if (frequencyIndex(code) >= 0)
        return QStringLiteral("%1GHz(freqID=%2)").arg(9.0 + code * 0.01, 0, 'f', 1).arg(code);
    return QStringLiteral("未知频点(freqID=%1)").arg(code);
}
inline QString scanDetails(const ScanRange& param) {
    return QStringLiteral("AA04 工作模式=%1(raw=%2)")
        .arg(param.workMode == 1 ? "TAS" : param.workMode == 0 ? "TWS" : "未知").arg(param.workMode);
}
inline QString beamDetails(const BeamControl& p) {
    QString text = QStringLiteral("AA05 频率=%1；类型=%2(raw=%3)；方位=%4~%5°，步进=%6°；场景=%7(sceneRaw=%8)；有效波形数=%9；积累脉冲=%10")
        .arg(frequencyText(p.freqID), p.type == 1 ? QStringLiteral("线性负调频") : p.type == 2 ? QStringLiteral("线性正调频") : QStringLiteral("未知"))
        .arg(p.type).arg(p.aziStart * 0.01, 0, 'f', 2).arg(p.aziEnd * 0.01, 0, 'f', 2)
        .arg(p.aziStep * 0.01, 0, 'f', 2).arg(int(p.scene) + 1).arg(p.scene).arg(p.flagNum).arg(p.pulseNum);
    const auto wave = [](int number, int enabled, int code, int start, int end, int elStart, int elEnd, int elStep) {
        return QStringLiteral("；波形%1[使能=%2，编码=%3，采样=%4~%5us，俯仰=%6~%7°，间隔=%8°]")
            .arg(number).arg(enabled).arg(code).arg(start * 0.1, 0, 'f', 1).arg(end * 0.1, 0, 'f', 1)
            .arg(elStart * 0.01, 0, 'f', 2).arg(elEnd * 0.01, 0, 'f', 2).arg(elStep * 0.01, 0, 'f', 2);
    };
    text += wave(1, p.beam1Flag, p.beam1Code, p.sampleStart1, p.sampleEnd1, p.elestart1, p.eleend1, p.elestep1);
    text += wave(2, p.beam2Flag, p.beam2Code, p.sampleStart2, p.sampleEnd2, p.elestart2, p.eleend2, p.elestep2);
    text += wave(3, p.beam3Flag, p.beam3Code, p.sampleStart3, p.sampleEnd3, p.elestart3, p.eleend3, p.elestep3);
    return text;
}
inline QString servoDetails(const ServoControlParam& p) {
    const QString cmd = p.cmd == 2 ? QStringLiteral("方位寻位") : p.cmd == 3 ? QStringLiteral("方位归北")
        : p.cmd == 1 ? QStringLiteral("方位转动") : p.cmd == 0 ? QStringLiteral("方位停转") : QStringLiteral("未知");
    return QStringLiteral("AA03 指令=%1(raw=%2)；速度=%3秒/转；方位=%4°(azRaw=%5)")
        .arg(cmd).arg(p.cmd).arg(p.speed).arg(p.az * 0.01, 0, 'f', 2).arg(p.az);
}
}
#endif
