#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDir>
#include <QFile>
#include <QIcon>

#include <filesystem>

int main(int argc, char* argv[]) {
    fprintf(stderr, "[main] entered\n");
    fflush(stderr);
    QGuiApplication app(argc, argv);
    app.setApplicationName("Nexora Airways");
    app.setOrganizationName("Nexora");

    // Use icons/nexora.ico as the window/taskbar icon when it is present.
    // Look next to the executable first (deployed by CMake), then the CWD.
    for (const QString& iconPath : {
             QDir(QCoreApplication::applicationDirPath()).filePath("icons/nexora.ico"),
             QStringLiteral("icons/nexora.ico") }) {
        if (QFile::exists(iconPath)) {
            app.setWindowIcon(QIcon(iconPath));
            break;
        }
    }

    // Ensure the data directory exists relative to the working directory,
    // matching the original CLI behaviour.
    if (!std::filesystem::is_directory("data"))
        std::filesystem::create_directory("data");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("NexoraAirways", "Main");

    return app.exec();
}
