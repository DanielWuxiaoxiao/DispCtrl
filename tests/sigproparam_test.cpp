#include "Basic/Protocol.h"
#include <QByteArray>
#include <QTest>

class SigProParamTest final : public QObject {
    Q_OBJECT
private slots:
    void defaultLayout() {
        const SigProParam param;
        const QByteArray wire(reinterpret_cast<const char*>(&param), sizeof(param));
        QCOMPARE(wire.size(), 32);
        QCOMPARE(wire.left(2), QByteArray::fromHex("06aa"));
        QCOMPARE(wire.mid(22, 4), QByteArray::fromHex("50140000"));
        QCOMPARE(wire.mid(26), QByteArray(6, '\0'));
        QCOMPARE(param.heightProcessLowerLimitM, short(0));
    }

    void signedHeight_data() {
        QTest::addColumn<int>("height");
        QTest::addColumn<QByteArray>("expectedBytes");
        QTest::newRow("default") << 0 << QByteArray::fromHex("0000");
        QTest::newRow("negative-one") << -1 << QByteArray::fromHex("ffff");
        QTest::newRow("negative-hundred") << -100 << QByteArray::fromHex("9cff");
        QTest::newRow("positive") << 100 << QByteArray::fromHex("6400");
        QTest::newRow("minimum") << -32768 << QByteArray::fromHex("0080");
        QTest::newRow("maximum") << 32767 << QByteArray::fromHex("ff7f");
    }

    void signedHeight() {
        QFETCH(int, height);
        QFETCH(QByteArray, expectedBytes);
        SigProParam param;
        const QByteArray baseline(reinterpret_cast<const char*>(&param), sizeof(param));
        param.heightProcessLowerLimitM = static_cast<short>(height);
        const QByteArray wire(reinterpret_cast<const char*>(&param), sizeof(param));
        QCOMPARE(wire.mid(24, 2), expectedBytes);
        QCOMPARE(wire.left(24), baseline.left(24));
        QCOMPARE(wire.mid(26), QByteArray(6, '\0'));
    }
};

QTEST_GUILESS_MAIN(SigProParamTest)
#include "sigproparam_test.moc"
