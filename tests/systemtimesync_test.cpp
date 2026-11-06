#include <QTest>
#include "Basic/Protocol.h"

class SystemTimeSyncTest final : public QObject {
    Q_OBJECT
private slots:
    void commandGoldenFrame()
    {
        SystemTimeSyncControl command;
        char* frame = packData(reinterpret_cast<char*>(&command), sizeof(command), 1, 2, 3);
        QVERIFY(frame);
        const QByteArray bytes(frame, sizeof(ProtocolFrame) + sizeof(command) + sizeof(ProtocolEnd));
        free(frame);
        QCOMPARE(bytes.toHex(), QByteArray("55fa55fa0100020003000000140010aa01000000affa55fa55"));
    }

    void replySuccessAndFailure()
    {
        const auto success = QByteArray::fromHex("55fa55fa0100020003000000140003de01000000c8fa55fa55");
        const auto failure = QByteArray::fromHex("55fa55fa0100020003000000140003de00000000c9fa55fa55");
        bool succeeded = false;
        QVERIFY(decodeSystemTimeSyncReply(success.constData(), success.size(), 1, 2, succeeded));
        QVERIFY(succeeded);
        QVERIFY(decodeSystemTimeSyncReply(failure.constData(), failure.size(), 1, 2, succeeded));
        QVERIFY(!succeeded);
    }

    void rejectsMalformedReplies()
    {
        const auto success = QByteArray::fromHex("55fa55fa0100020003000000140003de01000000c8fa55fa55");
        bool succeeded = false;
        auto decode = [&](const QByteArray& frame) {
            return decodeSystemTimeSyncReply(frame.constData(), frame.size(), 1, 2, succeeded);
        };
        QVERIFY(!decode(success.left(success.size() - 1)));
        QVERIFY(!decode(success + QByteArray(1, '\0')));
        QVERIFY(!decodeSystemTimeSyncReply(success.constData(), success.size(), 2, 1, succeeded));
        for (int offset : {0, 12, 14, 20, 24}) {
            auto corrupted = success;
            corrupted[offset] = char(success.at(offset) ^ 0x01);
            if (offset < 20) corrupted[20] = char(calculateXOR(corrupted.constData(), 20));
            QVERIFY(!decode(corrupted));
        }
        auto invalidResult = success;
        invalidResult[16] = 2;
        invalidResult[20] = char(calculateXOR(invalidResult.constData(), 20));
        QVERIFY(!decode(invalidResult));
        QVERIFY(!succeeded);
    }
};

QTEST_APPLESS_MAIN(SystemTimeSyncTest)
#include "systemtimesync_test.moc"
