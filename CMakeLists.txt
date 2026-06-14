cmake_minimum_required(VERSION 3.21)

project(NexoraAirways VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# argon2 / sqlite3 are compiled from source under third_party/ with the SAME
# MinGW-Builds 13.1.0 (MSVCRT) toolchain as Qt. Do NOT link the MSYS2 ucrt64
# archives here: those are UCRT / newer-GCC (sqlite 15.2, argon2 13.2) builds,
# and mixing them with Qt's MSVCRT binaries makes the app fail to load with
# STATUS_ENTRYPOINT_NOT_FOUND (0xC0000139) at startup, before main() runs.
set(THIRD_PARTY "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Gui Network)

qt_standard_project_setup(REQUIRES 6.5)

if(POLICY QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

# ---- Core reservation logic (converted from the CLI C++20 modules) ----
add_library(reservation_core STATIC
    core/seats.cpp
    core/crypto.cpp
    core/sqlite3db.cpp
    core/flights.cpp
    core/users.cpp
)

target_include_directories(reservation_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${THIRD_PARTY}/include
)

# Link argon2 and sqlite3 as STATIC archives. These are pure C libraries built
# from source with Qt's MinGW 13.1 kit (see third_party/build_thirdparty.sh),
# so they embed cleanly into the MSVCRT binary with no runtime-DLL conflict.
target_link_libraries(reservation_core PUBLIC
    ${THIRD_PARTY}/lib/libsqlite3.a
    ${THIRD_PARTY}/lib/libargon2.a
)

# ---- GUI application ----
qt_add_executable(NexoraAirways
    backend/main.cpp
    backend/Backend.cpp
    backend/Backend.h
    backend/FlightListModel.cpp
    backend/FlightListModel.h
    backend/SeatGridModel.cpp
    backend/SeatGridModel.h
    backend/PaystackClient.cpp
    backend/PaystackClient.h
)

# ---- Application icon ----
# Use icons/nexora.ico as the app icon when present. On Windows the .exe /
# taskbar icon comes from a compiled .rc resource; the runtime window icon is
# also set in main.cpp from the copy deployed next to the executable.
# (CMake evaluates EXISTS at configure time, so re-run cmake after adding the
# icon for the first time.)
set(NEXORA_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/nexora.ico")
if(EXISTS "${NEXORA_ICON}")
    enable_language(RC)
    target_sources(NexoraAirways PRIVATE icons/nexora.rc)
    set(NEXORA_HAS_ICON TRUE)
endif()

# Theme.qml is a singleton — must be marked before qt_add_qml_module
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(NexoraAirways
    URI NexoraAirways
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/LoginPage.qml
        qml/HomePage.qml
        qml/FlightsView.qml
        qml/SeatMapDialog.qml
        qml/BookingsView.qml
        qml/WalletView.qml
        qml/AccountView.qml
        qml/ManageFlightsView.qml
        qml/AdminListView.qml
        qml/PaymentsView.qml
        qml/SuperAdminView.qml
        qml/Theme.qml
        qml/components/GlassCard.qml
        qml/components/PrimaryButton.qml
        qml/components/StyledField.qml
        qml/components/Toast.qml
    RESOURCES
        icons/nexora.png
)

target_include_directories(NexoraAirways PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

target_link_libraries(NexoraAirways PRIVATE Qt6::Quick Qt6::Gui Qt6::Network reservation_core)

# Statically link the MinGW C/C++ runtime so the exe does NOT depend on
# libgcc_s_seh-1.dll, libstdc++-6.dll or libwinpthread-1.dll. The Qt6 DLLs
# still need to be deployed alongside the exe (see windeployqt POST_BUILD
# below) because only a dynamic Qt build is installed.
target_link_options(NexoraAirways PRIVATE
    -static-libgcc
    -static-libstdc++
    -static
)

# Copy runtime data (database) next to the executable
add_custom_command(TARGET NexoraAirways POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:NexoraAirways>/data
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/data/flight_reservation.db
            $<TARGET_FILE_DIR:NexoraAirways>/data/flight_reservation.db
    # Copy the Paystack secret config next to the executable
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:NexoraAirways>/config
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/config/paystack_secret.txt
            $<TARGET_FILE_DIR:NexoraAirways>/config/paystack_secret.txt
)

# Deploy the app icon next to the executable so main.cpp can set the window icon.
if(NEXORA_HAS_ICON)
    add_custom_command(TARGET NexoraAirways POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:NexoraAirways>/icons
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${NEXORA_ICON}
                $<TARGET_FILE_DIR:NexoraAirways>/icons/nexora.ico
    # Add nexora.png to the deployed app folder too since it's used at runtime by the QML UI (Image in LoginPage.qml).
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/icons/nexora.png
                $<TARGET_FILE_DIR:NexoraAirways>/icons/nexora.png
        COMMENT "Deploying app icon next to executable"
    )
endif()

# ---- Standalone deployment ----
# Run windeployqt after the build to copy the Qt6 DLLs, platform/style plugins
# and the QML runtime next to the exe, producing a folder that runs on a clean
# Windows machine with no Qt installed. Combined with the static MinGW runtime
# above, the only remaining external dependencies are system DLLs (kernel32,
# msvcrt) that ship with Windows.
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT6_INSTALL_PREFIX}/bin" "${CMAKE_PREFIX_PATH}/bin")
if(WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET NexoraAirways POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                # This MinGW Qt kit ships only release-mode Qt binaries/plugins
                # (no qwindowsd.dll etc.), so always deploy the release set even
                # for a Debug build of the app — windeployqt's auto-detection
                # would otherwise look for non-existent debug plugins and fail.
                --release
                "$<TARGET_FILE:NexoraAirways>"
        COMMENT "Deploying Qt runtime next to NexoraAirways.exe (standalone bundle)"
        VERBATIM
    )
endif()
