#include "Controller/errorloglimiter.h"

#include <QTest>
#include <QThread>
#include <QVector>

#include <atomic>

namespace {

ErrorInfo warning(const QString& code = QStringLiteral("DATA_INVALID_DETECTION"))
{
    ErrorInfo error{};
    error.code = code;
    error.message = QStringLiteral("Invalid detection point");
    error.severity = ErrorSeverity::Warning;
    error.category = ErrorCategory::DataProcessing;
    return error;
}

} // namespace

class ErrorLogLimiterTest : public QObject
{
    Q_OBJECT

private slots:
    void aggregatesDynamicPointDescriptions();
    void isolatesCodeSeverityAndCategory();
    void boundsStateAndEvictsLeastRecentlyUsed();
    void usesMonotonicTimeRatherThanErrorTimestamp();
    void neverSuppressesCriticalOrFatal();
    void aggregatesConcurrentCalls();
};

void ErrorLogLimiterTest::aggregatesDynamicPointDescriptions()
{
    ErrorLogInternal::BoundedLogLimiter limiter;
    ErrorInfo error = warning();
    const auto first = limiter.check(error, 0);
    QVERIFY(first.shouldLog);
    QCOMPARE(first.suppressedCount, quint64(0));

    for (int index = 0; index < 10000; ++index) {
        error.message = QStringLiteral("Invalid detection point: range=%1").arg(index);
        error.context.insert(QStringLiteral("range"), index);
        QVERIFY(!limiter.check(error, 1 + index % 4999).shouldLog);
    }
    QCOMPARE(limiter.stateCount(), 1);

    const auto nextWindow = limiter.check(error, 5000);
    QVERIFY(nextWindow.shouldLog);
    QCOMPARE(nextWindow.suppressedCount, quint64(10000));
    QVERIFY(!limiter.check(error, 5001).shouldLog);
    const auto thirdWindow = limiter.check(error, 10000);
    QVERIFY(thirdWindow.shouldLog);
    QCOMPARE(thirdWindow.suppressedCount, quint64(1));
}

void ErrorLogLimiterTest::isolatesCodeSeverityAndCategory()
{
    ErrorLogInternal::BoundedLogLimiter limiter;
    ErrorInfo error = warning();
    QVERIFY(limiter.check(error, 0).shouldLog);
    error.code = QStringLiteral("DATA_INVALID_TRACK");
    QVERIFY(limiter.check(error, 0).shouldLog);
    error.severity = ErrorSeverity::Error;
    QVERIFY(limiter.check(error, 0).shouldLog);
    error.category = ErrorCategory::Network;
    QVERIFY(limiter.check(error, 0).shouldLog);
    QCOMPARE(limiter.stateCount(), 4);
    QVERIFY(!limiter.check(error, 1).shouldLog);
}

void ErrorLogLimiterTest::boundsStateAndEvictsLeastRecentlyUsed()
{
    ErrorLogInternal::BoundedLogLimiter limiter;
    for (int index = 0; index < ErrorLogInternal::BoundedLogLimiter::kMaxStates; ++index) {
        QVERIFY(limiter.check(warning(QString::number(index)), index).shouldLog);
    }
    QVERIFY(!limiter.check(warning(QStringLiteral("0")), 500).shouldLog);
    QVERIFY(limiter.check(warning(QStringLiteral("new-code")), 501).shouldLog);
    QCOMPARE(limiter.stateCount(), ErrorLogInternal::BoundedLogLimiter::kMaxStates);
    QVERIFY(!limiter.check(warning(QStringLiteral("0")), 502).shouldLog);
    QVERIFY(limiter.check(warning(QStringLiteral("1")), 503).shouldLog);

    for (int index = 0; index < 10000; ++index) {
        QVERIFY(limiter.check(warning(QStringLiteral("dynamic-%1").arg(index)), 600 + index).shouldLog);
    }
    QCOMPARE(limiter.stateCount(), ErrorLogInternal::BoundedLogLimiter::kMaxStates);
}

void ErrorLogLimiterTest::usesMonotonicTimeRatherThanErrorTimestamp()
{
    ErrorLogInternal::BoundedLogLimiter limiter;
    ErrorInfo error = warning();
    error.timestamp = QDateTime::fromMSecsSinceEpoch(100000);
    QVERIFY(limiter.check(error, 0).shouldLog);
    error.timestamp = QDateTime::fromMSecsSinceEpoch(-100000);
    QVERIFY(!limiter.check(error, 4999).shouldLog);
    const auto decision = limiter.check(error, 5000);
    QVERIFY(decision.shouldLog);
    QCOMPARE(decision.suppressedCount, quint64(1));

    // An out-of-order clock sample from a concurrent caller must not reopen a window.
    QVERIFY(!limiter.check(error, 0).shouldLog);
    QVERIFY(limiter.check(error, 10000).shouldLog);
}

void ErrorLogLimiterTest::neverSuppressesCriticalOrFatal()
{
    ErrorLogInternal::BoundedLogLimiter limiter;
    ErrorInfo error = warning();
    error.severity = ErrorSeverity::Critical;
    QVERIFY(limiter.check(error, 0).shouldLog);
    QVERIFY(limiter.check(error, 0).shouldLog);
    error.severity = ErrorSeverity::Fatal;
    QVERIFY(limiter.check(error, 0).shouldLog);
    QVERIFY(limiter.check(error, 0).shouldLog);
    QCOMPARE(limiter.stateCount(), 0);
}

void ErrorLogLimiterTest::aggregatesConcurrentCalls()
{
    ErrorLogInternal::BoundedLogLimiter limiter;
    std::atomic<int> accepted{0};
    QVector<QThread*> threads;
    for (int worker = 0; worker < 4; ++worker) {
        QThread* thread = QThread::create([&limiter, &accepted]() {
            const ErrorInfo error = warning();
            for (int index = 0; index < 1000; ++index) {
                if (limiter.check(error, 0).shouldLog) {
                    ++accepted;
                }
            }
        });
        threads.append(thread);
        thread->start();
    }
    for (QThread* thread : threads) {
        thread->wait();
        delete thread;
    }
    QCOMPARE(accepted.load(), 1);
    const auto decision = limiter.check(warning(), 5000);
    QVERIFY(decision.shouldLog);
    QCOMPARE(decision.suppressedCount, quint64(3999));
    QCOMPARE(limiter.stateCount(), 1);
}

QTEST_APPLESS_MAIN(ErrorLogLimiterTest)

#include "errorloglimiter_test.moc"
