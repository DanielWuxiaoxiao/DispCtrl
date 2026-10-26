#include "CommandControl/commandcontrolmodule.h"
#include <QSignalSpy>
#include <QTest>
#include <memory>

class CommandControlLifecycleTest final : public QObject {
    Q_OBJECT
private slots:
    void destroyingInactiveModuleDoesNotEmitReady() {
        // No init(): no UDP, NTP, ICMP, recording or live radar resources.
        auto module = std::make_unique<CommandControlModule>();
        QSignalSpy ready(module.get(), &CommandControlModule::readyChanged);
        module.reset();
        QCOMPARE(ready.count(), 0);
    }
    void receiverLifetimeDoesNotFollowParentLifetime() {
        auto owner = std::make_unique<QObject>();
        auto* overlay = new QObject(owner.get());
        auto* module = new CommandControlModule(owner.get());
        int callbacks = 0;
        connect(module, &CommandControlModule::readyChanged, overlay, [&](bool) { ++callbacks; });
        module->readyChanged(true);
        QCOMPARE(callbacks, 1);
        delete overlay;
        module->readyChanged(false);
        QCOMPARE(callbacks, 1);
        owner.reset();
        QCOMPARE(callbacks, 1);
    }
};
QTEST_GUILESS_MAIN(CommandControlLifecycleTest)
#include "commandcontrol_lifecycle_test.moc"
