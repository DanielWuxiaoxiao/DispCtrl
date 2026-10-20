#include "cusWidgets/windowgeometry.h"
#include <QTest>

class WindowGeometryTest : public QObject {
    Q_OBJECT
private slots:
    void maximizesOnRightExtensionDespiteStalePrimaryScreen()
    {
        const QVector<CusWindowGeometry::Screen> screens{
            {QRect(0, 0, 1920, 1080), QRect(0, 0, 1920, 1040)},
            {QRect(1920, 0, 2560, 1440), QRect(1920, 0, 2560, 1400)}
        };
        QCOMPARE(CusWindowGeometry::maximizedGeometry(QRect(2200, 100, 800, 600), screens, 0),
                 screens[1].availableGeometry);
    }

    void supportsLeftAndUpperExtensions()
    {
        const QVector<CusWindowGeometry::Screen> screens{
            {QRect(0, 0, 1920, 1080), QRect(0, 0, 1920, 1040)},
            {QRect(-1920, 0, 1920, 1080), QRect(-1920, 0, 1920, 1040)},
            {QRect(0, -1200, 1920, 1200), QRect(0, -1170, 1920, 1170)}
        };
        QCOMPARE(CusWindowGeometry::maximizedGeometry(QRect(-1500, 100, 500, 400), screens),
                 screens[1].availableGeometry);
        QCOMPARE(CusWindowGeometry::maximizedGeometry(QRect(200, -900, 500, 400), screens),
                 screens[2].availableGeometry);
    }

    void choosesLargestOverlapAcrossScreens()
    {
        const QVector<CusWindowGeometry::Screen> screens{
            {QRect(0, 0, 1920, 1080), QRect(0, 0, 1920, 1040)},
            {QRect(1920, 0, 1920, 1080), QRect(1920, 0, 1920, 1040)}
        };
        QCOMPARE(CusWindowGeometry::maximizedGeometry(QRect(1700, 100, 1000, 600), screens, 0),
                 screens[1].availableGeometry);
        QCOMPARE(CusWindowGeometry::maximizedGeometry(QRect(1300, 100, 1000, 600), screens, 1),
                 screens[0].availableGeometry);
        QCOMPARE(CusWindowGeometry::maximizedGeometry(QRect(1420, 100, 1000, 600), screens, 1),
                 screens[1].availableGeometry);
    }

    void usesWorkAreaWithoutAdditionalDpiScaling()
    {
        const QVector<CusWindowGeometry::Screen> screens{
            {QRect(0, 0, 1920, 1080), QRect(0, 0, 1920, 1040)},
            {QRect(1920, 200, 1707, 960), QRect(1960, 200, 1667, 960)}
        };
        QCOMPARE(CusWindowGeometry::maximizedGeometry(QRect(2200, 300, 700, 500), screens),
                 QRect(1960, 200, 1667, 960));
    }

    void fallsBackSafelyForMissingOrOffscreenWindow()
    {
        const QVector<CusWindowGeometry::Screen> screens{
            {QRect(0, 0, 1920, 1080), QRect(0, 0, 1920, 1040)},
            {QRect(-1920, 0, 1920, 1080), QRect()}
        };
        QCOMPARE(CusWindowGeometry::maximizedGeometry(QRect(8000, 8000, 400, 300), screens, 1),
                 screens[1].geometry);
        QCOMPARE(CusWindowGeometry::maximizedGeometry(QRect(8000, 8000, 400, 300), screens, -1),
                 screens[0].availableGeometry);
        QVERIFY(!CusWindowGeometry::maximizedGeometry(QRect(0, 0, 400, 300), {}).isValid());
    }
};

QTEST_GUILESS_MAIN(WindowGeometryTest)
#include "windowgeometry_test.moc"
