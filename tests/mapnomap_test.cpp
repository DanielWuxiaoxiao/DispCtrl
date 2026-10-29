#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <memory>
#include "mapDisp/mapprox.h"

class MapNoMapTest final : public QObject {
    Q_OBJECT
private slots:
    void blackHtmlLoadsWithoutTiles() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        m_previousDirectory = QDir::currentPath();
        const QString source = QFINDTESTDATA("../htmls/black.html");
        QVERIFY(!source.isEmpty());
        m_htmlPath = QCoreApplication::applicationDirPath() + "/black.html";
        QVERIFY2(!QFile::exists(m_htmlPath), "Test executable must use an isolated fixture directory");
        QVERIFY(QFile::copy(source, m_htmlPath));
        m_fixtureCreated = true;
        for (const QString& name : {"mapNoL", "map16", "map16S", "indexNoL.html", "index3d.html"})
            QVERIFY(!QFile::exists(QCoreApplication::applicationDirPath() + '/' + name));
        QVERIFY(QDir::setCurrent(temp.path()));
        QFile config("config.toml");
        QVERIFY(config.open(QIODevice::WriteOnly));
        config.write("[map]\ndefault_type = 0\n");
        config.close();
        MapProxyWidget proxy;
        QWidget host;
        QVBoxLayout layout(&host); layout.setContentsMargins(0, 0, 0, 0);
        auto* view = proxy.getView();
        layout.addWidget(view);
        view->resize(320, 240);
        // Keep the host hidden: this checks Chromium HTML/JS, not GPU presentation.
        QSignalSpy loads(view, &QWebEngineView::loadFinished);
        proxy.chooseMap(0);
        QTRY_VERIFY_WITH_TIMEOUT(loads.count() > 0, 15000);
        QVERIFY(loads.last().first().toBool());
        QCOMPARE(view->url().toLocalFile(), m_htmlPath);
        QCOMPARE(view->page()->backgroundColor(), QColor(Qt::black));
        struct Result { bool received = false; QJsonObject dom; };
        const auto result = std::make_shared<Result>();
        view->page()->runJavaScript(QStringLiteral(
            "JSON.stringify({ready:document.readyState,"
            "html:getComputedStyle(document.documentElement).backgroundColor,"
            "body:getComputedStyle(document.body).backgroundColor})"),
            [result](const QVariant& value) {
                result->dom = QJsonDocument::fromJson(value.toString().toUtf8()).object();
                result->received = true;
            });
        QTRY_VERIFY_WITH_TIMEOUT(result->received, 5000);
        QCOMPARE(result->dom.value("ready").toString(), QString("complete"));
        QCOMPARE(result->dom.value("html").toString(), QString("rgb(0, 0, 0)"));
        QCOMPARE(result->dom.value("body").toString(), QString("rgb(0, 0, 0)"));

        for (int type : {1, 2, 3}) {
            const int completed = loads.count();
            proxy.chooseMap(type);
            QTRY_VERIFY_WITH_TIMEOUT(loads.count() > completed, 15000);
            QVERIFY(loads.last().first().toBool());
            QCOMPARE(view->url().toLocalFile(), m_htmlPath);
        }

        const QString appDir = QCoreApplication::applicationDirPath();
        // An empty selected tile directory also uses the real black HTML page.
        const QString satelliteDir = appDir + "/map16S";
        QVERIFY(QDir().mkdir(satelliteDir));
        m_fixtureDirectories << satelliteDir;
        int completed = loads.count();
        proxy.chooseMap(3);
        QTRY_VERIFY_WITH_TIMEOUT(loads.count() > completed, 15000);
        QCOMPARE(view->url().toLocalFile(), m_htmlPath);

        // Synthetic fixture checks page selection only, not real tile coverage/rendering.
        const QString roadDir = appDir + "/mapNoL";
        QVERIFY(QDir().mkdir(roadDir));
        m_fixtureDirectories << roadDir;
        const QString markerPath = roadDir + "/fixture";
        QFile marker(markerPath);
        QVERIFY(marker.open(QIODevice::WriteOnly));
        m_fixtureFiles << markerPath;
        marker.write("fixture"); marker.close();
        const QString roadHtml = appDir + "/indexNoL.html";
        QVERIFY(QFile::copy(source, roadHtml));
        m_fixtureFiles << roadHtml;
        completed = loads.count();
        proxy.chooseMap(1);
        QTRY_VERIFY_WITH_TIMEOUT(loads.count() > completed, 15000);
        QVERIFY(loads.last().first().toBool());
        QCOMPARE(view->url().toLocalFile(), roadHtml);

        // A different map's available tiles must not bypass the selected-map check.
        completed = loads.count();
        proxy.chooseMap(2);
        QTRY_VERIFY_WITH_TIMEOUT(loads.count() > completed, 15000);
        QVERIFY(loads.last().first().toBool());
        QCOMPARE(view->url().toLocalFile(), m_htmlPath);

        completed = loads.count();
        proxy.chooseMap(4); // No synthetic online page; exercise missing-HTML recovery.
        QCoreApplication::processEvents();
        QVERIFY(view->isHidden());
        QCOMPARE(loads.count(), completed);
        proxy.chooseMap(0);
        QTRY_VERIFY_WITH_TIMEOUT(loads.count() > completed, 15000);
        QVERIFY(loads.last().first().toBool());
        QCOMPARE(view->url().toLocalFile(), m_htmlPath);
    }
    void cleanupTestCase() {
        if (!m_previousDirectory.isEmpty()) QVERIFY(QDir::setCurrent(m_previousDirectory));
        for (const auto& path : m_fixtureFiles) QVERIFY(QFile::remove(path));
        for (const auto& path : m_fixtureDirectories) QVERIFY(QDir().rmdir(path));
        if (m_fixtureCreated) QVERIFY(QFile::remove(m_htmlPath));
    }
private:
    QString m_previousDirectory;
    QString m_htmlPath;
    QStringList m_fixtureFiles, m_fixtureDirectories;
    bool m_fixtureCreated = false;
};
int main(int argc, char** argv) {
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    MapNoMapTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "mapnomap_test.moc"
