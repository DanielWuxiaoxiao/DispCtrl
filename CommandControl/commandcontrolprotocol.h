/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-09-11 19:13:20
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-11 22:04:54
 * @Description: 
 */
/*
 * 指控系统 UDP 协议的字节级编解码。
 *
 * 本文件不使用 C++ 结构体直接映射网络报文，所有字段按小端显式写入，避免填充、
 * 对齐和宿主字节序影响线上格式。
 */
#ifndef COMMANDCONTROL_PROTOCOL_H
#define COMMANDCONTROL_PROTOCOL_H

#include <QByteArray>
#include <QDateTime>
#include <QtGlobal>

namespace CommandControlProtocol {

constexpr quint16 kMagic = 0x5AA5;
constexpr quint8 kVersion = 0x10;
// 公共报头长度：2 + 1 + 4 + 4 + 1 + 2 + 1 + 3。
constexpr int kHeaderSize = 18;

enum MessageType : quint16 {
    ManagementNodeNotice = 0xDD31,  // 组播：总控公布单播端点与登录动作。
    LoginRequest = 0xDD33,          // 单播：本机登录或退出请求。
    LoginReply = 0xDD34,            // 单播：总控对 DD33 的业务结果。
    LinkCheck = 0xDD25,             // 组播：链路保活。
    SituationIntelligence = 0xDDA4, // 组播：航迹态势情报。
    EquipmentStatus = 0xDDA1        // 组播：雷达装备状态。
};

enum class LoginIdentity : quint8 {
    ManagementNodeNotice = 0x01, // 发现管理节点后执行登录。
    Relogin = 0x02,              // 即使端点未变化，也必须重新登录。
    Logout = 0x03                // 执行退出登录。
};

enum class LoginRequestType : quint8 {
    Default = 0x00, // 预留默认类型，本机不主动使用。
    Login = 0x01,   // 请求登录。
    Logout = 0x02   // 请求退出登录。
};

struct Header {
    quint16 magic = kMagic;       // 偏移 0：标识字，固定 0x5AA5，小端写为 A5 5A。
    quint8 version = kVersion;    // 偏移 2：版本，D7-D4=1、D3-D0=0，即 0x10。
    quint32 senderId = 0;         // 偏移 3：发端用户 ID；本机为 0x11474202。
    quint32 receiverId = 0;       // 偏移 7：收端用户 ID；组播无收端时填 0。
    quint8 flags = 0;             // 偏移 11：D7 回执请求；D4-D0 优先级；其余位为 0。
    quint16 messageType = 0;      // 偏移 12：报文类型字，取 MessageType。
    quint8 sequence = 0;          // 偏移 14：0 至 255 循环的发送流水号。
    quint32 dayTicks10Ms = 0;     // 偏移 15：北京时间当日零点起的 10 ms 计数，仅低 24 位在线上传输。
};

struct ManagementNode {
    quint32 controlId = 0; // DD31 偏移 18：总控用户 ID，非零才接受。
    LoginIdentity identity = LoginIdentity::ManagementNodeNotice; // 偏移 22：登录、重登或退出动作。
    quint32 ipv4LowFirst = 0; // 偏移 23：IPv4 的四个八位组按低字节到高字节存放。
    quint16 port = 0; // 偏移 27：总控单播 UDP 端口，小端。
    QByteArray reserve; // 偏移 29：7 字节备份字段；接收保留原值，仅用于联调日志。
};

struct LoginRequestPayload {
    quint32 userId = 0; // DD33 偏移 18：请求方用户 ID；本机固定为 0x11474202。
    quint32 ipv4LowFirst = 0; // 偏移 22：请求方 IPv4，四个八位组按低字节到高字节存放。
    quint16 port = 0; // 偏移 26：请求方 UDP 端口，小端。
    LoginRequestType type = LoginRequestType::Default; // 偏移 28：0 默认、1 登录、2 退出。
    quint16 year = 0; // 偏移 29：北京时间年份。
    quint8 month = 0; // 偏移 31：北京时间月份。
    quint8 day = 0; // 偏移 32：北京时间日期。
    quint8 hour = 0; // 偏移 33：北京时间时。
    quint8 minute = 0; // 偏移 34：北京时间分。
    quint8 second = 0; // 偏移 35：北京时间秒。
    quint16 millisecond = 0; // 偏移 36：北京时间毫秒。
};

struct LoginReplyPayload {
    quint8 result = 0;       // DD34 偏移 18：0x01 成功、0x02 失败、0x03 退出成功。
    quint8 rejectReason = 0; // 偏移 19：0x00 默认、0x01 未注册 ID；仅失败时有意义。
    quint32 controlId = 0;   // 偏移 20：回复方主控 ID，必须与当前 DD31 一致。
};

struct LinkCheckPayload {
    quint32 monthTimestampMs = 0; // DD25 偏移 18：北京时间本月首日零点起的毫秒数。
    quint8 cooperationStatus = 0; // 偏移 22：协同参与状态，定版默认 0x00。
};

struct Dda4Track {
    quint32 comprehensiveBatch = 0; // 偏移 18：目标综合批号，当前默认 0。
    quint32 localBatch = 0; // 偏移 22：本机管理批号，必须非零且同一源航迹保持稳定。
    quint32 deviceId = 0; // 偏移 26：产生该航迹的本机设备 ID。
    quint8 deviceType = 2; // 偏移 30：设备类型，定版默认 2。
    quint8 deviceNumber = 1; // 偏移 31：同类型设备编号，默认 1。
    quint16 rateCentiHz = 200; // 偏移 32：数据率，单位 0.01 Hz；200 表示 2 Hz。
    quint8 targetAttribute = 0; // 偏移 34：目标属性，当前所有位按定版填 0。
    quint8 targetType = 0; // 偏移 35：目标类型，当前默认 0x00。
    quint16 reserve = 0; // 偏移 36：2 字节备份字段；发送填 0，接收保留用于联调日志。
    quint8 trackQuality = 0xFF; // 偏移 38：航迹质量，当前默认 0xFF。
    qint32 longitudeE7 = 0; // 偏移 39：经度，1e-7 度，东为正。
    qint32 latitudeE7 = 0; // 偏移 43：纬度，1e-7 度，北为正。
    qint32 altitudeM = 0; // 偏移 47：WGS84 高度，单位 m。
    qint16 velocityEast = 0x7FFF; // 偏移 51：ENU X（东）速度，0.15 m/s；0x7FFF 无效。
    qint16 velocityNorth = 0x7FFF; // 偏移 53：ENU Y（北）速度，0.15 m/s；0x7FFF 无效。
    qint16 velocityUp = 0x7FFF; // 偏移 55：ENU Z（天）速度，0.15 m/s；0x7FFF 无效。
    quint16 rcsMilliSquareM = 0; // 偏移 57：RCS，0.001 m²；无值填 0。
    quint8 interferenceStatus = 0; // 偏移 59：干扰状态，当前默认 0。
    quint8 updateMethod = 0; // 偏移 60：高半字节更新状态、低半字节自动/手动上报方式。
    quint16 relativeDelayMs = 0; // 偏移 61：相对延时，单位 ms，当前默认 0。
    quint32 monthTimestampMs = 0; // 偏移 63：本月 1 日零点起的航迹生成/接收毫秒数。
};

struct Dda1Status {
    qint32 longitudeE7 = 0; // 偏移 18：雷达经度，1e-7 度，东为正。
    qint32 latitudeE7 = 0; // 偏移 22：雷达纬度，1e-7 度，北为正。
    qint32 altitudeM = 0; // 偏移 26：雷达高度，单位 m。
    quint8 workStatus = 1; // 偏移 30：工作状态低 4 位，1 表示战斗。
    quint8 healthStatus = 3; // 偏移 31：健康状态，0x03 表示正常。
    quint8 deviceCount = 1; // 偏移 32：本报文携带的设备数量，当前为 1。
    quint8 deviceType = 0x02; // 偏移 33：搜索雷达类型，定版为 0x02。
    quint8 deviceNumber = 1; // 偏移 34：搜索雷达设备编号。
    quint8 deviceStatus = 3; // 偏移 35：设备状态，0x03 表示正常。
    quint8 workMode = 0x22; // 偏移 36：工作模式，0x22 表示常规。
    quint8 radiationStatus = 0; // 偏移 37：辐射状态，0 关、1 开。
    quint16 azimuthStartDeg = 0; // 偏移 38：方位范围起始，正北为 0 度。
    quint16 azimuthEndDeg = 360; // 偏移 40：方位范围终止，单位度。
    qint8 elevationStartDeg = -10; // 偏移 42：俯仰范围起始，单位度，负值二补码。
    qint8 elevationEndDeg = 70; // 偏移 43：俯仰范围终止，单位度，负值二补码。
    quint16 reserve = 0; // 偏移 44：2 字节备份字段；发送填 0，接收保留用于联调日志。
};

// 解析并校验 18 字节公共报头；标识字或版本不匹配即失败。
bool parseHeader(const QByteArray& packet, Header& header);
// 解析 DD31，同时校验类型和完整报文长度；备份字段仅占位。
bool parseManagementNode(const QByteArray& packet, ManagementNode& node);
// 解析 DD33，同时校验类型、完整报文长度和登录类型。
bool parseLoginRequest(const QByteArray& packet, LoginRequestPayload& request);
// 解析 DD34，同时校验类型和完整报文长度。
bool parseLoginReply(const QByteArray& packet, LoginReplyPayload& reply);
// 解析 DD25，同时校验类型和完整报文长度。
bool parseLinkCheck(const QByteArray& packet, LinkCheckPayload& linkCheck);
// 解析 DDA4，同时校验类型和完整报文长度；备份字段保留用于联调日志。
bool parseDda4Track(const QByteArray& packet, Dda4Track& track);
// 解析 DDA1，同时校验类型和完整报文长度；俯仰按带符号 8 位数恢复。
bool parseDda1Status(const QByteArray& packet, Dda1Status& status);

// 生成 DD33：userId、ipv4LowFirst、port 分别为本机身份、本机 IPv4、小端 UDP 端口。
QByteArray makeLoginRequest(const Header& header, quint32 userId, quint32 ipv4LowFirst,
                            quint16 port, LoginRequestType type, const QDateTime& beijingNow);
// 生成 DD25：monthTimestampMs 为本月首日以来毫秒数，cooperationStatus 当前固定 0x00。
QByteArray makeLinkCheck(const Header& header, quint32 monthTimestampMs,
                         quint8 cooperationStatus = 0x00);
// 生成 DDA4：所有业务字段来自 Dda4Track，两个备份字节固定填 0。
QByteArray makeDda4Track(const Header& header, const Dda4Track& track);
// 生成 DDA1：所有业务字段来自 Dda1Status，两个备份字节固定填 0。
QByteArray makeDda1Status(const Header& header, const Dda1Status& status);

// 返回日志/界面使用的中文类型名；未知类型返回“未知报文”。
const char* messageTypeName(quint16 type);
// 返回包含公共报头的固定总长度；未知类型返回 -1，调用方只能做报头校验。
int expectedPacketSize(quint16 type);

}  // namespace CommandControlProtocol

#endif  // COMMANDCONTROL_PROTOCOL_H
