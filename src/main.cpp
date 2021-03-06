#include "launchercore.h"
#include "launcherwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <keychain.h>

#include "sapphirelauncher.h"
#include "squareboot.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("redstrate");
    QCoreApplication::setOrganizationDomain("redstrate.com");

#ifdef NDEBUG
    QCoreApplication::setApplicationName("xivlauncher");
#else
    QCoreApplication::setApplicationName("xivlauncher-debug");
#endif

    LauncherCore c;

    QCommandLineParser parser;
    parser.setApplicationDescription("Cross-platform FFXIV Launcher");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption noguiOption("nogui", "Don't open a main window.");
    parser.addOption(noguiOption);

    QCommandLineOption autologinOption("autologin", "Auto-login with the default profile. This requires the profile to have remember username/password enabled!");
    parser.addOption(autologinOption);

    QCommandLineOption profileOption("default-profile", "Profile to use for default profile and autologin.", "profile");
    parser.addOption(profileOption);

    parser.process(app);

    if(parser.isSet(profileOption)) {
        c.defaultProfileIndex = c.getProfileIndex(parser.value(profileOption));

        if(c.defaultProfileIndex == -1) {
            qInfo() << "The profile \"" << parser.value(profileOption) << "\" does not exist!";
            return 0;
        }
    }
    if(parser.isSet(autologinOption)) {
        const auto profile = c.getProfile(c.defaultProfileIndex);

        if(!profile.rememberUsername || !profile.rememberPassword) {
            qInfo() << "Profile does not have a username and/or password saved, autologin disabled.";

            return 0;
        }

        auto loop = new QEventLoop();
        QString username, password;

        auto usernameJob = new QKeychain::ReadPasswordJob("LauncherWindow");
        usernameJob->setKey(profile.name + "-username");
        usernameJob->start();

        c.connect(usernameJob, &QKeychain::ReadPasswordJob::finished, [loop, usernameJob, &username](QKeychain::Job* j) {
            username = usernameJob->textData();
            loop->quit();
        });

        loop->exec();

        auto passwordJob = new QKeychain::ReadPasswordJob("LauncherWindow");
        passwordJob->setKey(profile.name + "-password");
        passwordJob->start();

        c.connect(passwordJob, &QKeychain::ReadPasswordJob::finished, [loop, passwordJob, &password](QKeychain::Job* j) {
            password = passwordJob->textData();
            loop->quit();
        });

        loop->exec();

        auto info = LoginInformation{&profile, username, password, ""};

        if(profile.isSapphire) {
            c.sapphireLauncher->login(profile.lobbyURL, info);
        } else {
            c.squareBoot->bootCheck(info);
        }
    }

    LauncherWindow w(c);
    if(!parser.isSet(noguiOption)) {
        w.show();
    }

    return app.exec();
}
