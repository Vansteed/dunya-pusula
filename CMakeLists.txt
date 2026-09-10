cmake_minimum_required(VERSION 3.21)
project(DunyaPusula CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Qml Gui)

qt_standard_project_setup(REQUIRES 6.5)

if(MSVC)
    # Kaynak ve calisma zamani kodlamasi UTF-8 - Turkce karakterler bozulmasin.
    add_compile_options(/utf-8)
endif()

set(CORE_SOURCES
    src/core/SysInfo.cpp
    src/core/CpuInfo.cpp
    src/core/SensorAjani.cpp
    src/core/HealthScore.cpp
    src/core/MemoryCleaner.cpp
    src/core/ProcessList.cpp
    src/core/DiskCleaner.cpp
    src/core/DiskHealth.cpp
    src/core/FolderScanner.cpp
    src/core/DosyaIslem.cpp
    src/core/Settings.cpp
    src/core/RestorePoint.cpp
    src/core/SingleInstance.cpp
)

# Arka plan isci nesneleri - saf QObject, Widgets bagimliligi YOK.
set(WORKER_SOURCES
    src/ui/CleanWorker.cpp
    src/ui/DiskScanWorker.cpp
    src/ui/DiskTreeScanWorker.cpp
)

set(BACKEND_SOURCES
    src/backend/MemoryBackend.cpp
    src/backend/ProcessModel.cpp
    src/backend/CpuBackend.cpp
    src/backend/HealthBackend.cpp
    src/backend/FolderTreeModel.cpp
)

qt_add_executable(DunyaPusula WIN32
    src/main.cpp
    ${CORE_SOURCES}
    ${WORKER_SOURCES}
    ${BACKEND_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/res/DunyaPusula.rc
)

target_include_directories(DunyaPusula PRIVATE src)

# RT_QML_DISK=1 ile calistirildiginda QML kaynak agacindan okunur
# (canli yenileme). Yol derleme aninda gomulur.
target_compile_definitions(DunyaPusula PRIVATE
    RT_QML_KAYNAK="${CMAKE_CURRENT_SOURCE_DIR}/qml")

# CPU sicakligi icin yonetici izni sart - ozel manifest linker'a gomulur
# (MSVC'nin urettigi ortak kontroller manifestiyle otomatik birlestirilir).
if(MSVC)
    target_link_options(DunyaPusula PRIVATE
        "/MANIFEST:EMBED"
        "/MANIFESTUAC:NO"
        "/MANIFESTINPUT:${CMAKE_SOURCE_DIR}/uygulama.manifest")
endif()

set_source_files_properties(qml/Tema.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(DunyaPusula
    URI RamTemizleyici
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Tema.qml
        qml/components/Card.qml
        qml/components/GhostButton.qml
        qml/components/PrimaryButton.qml
        qml/components/StatCard.qml
        qml/components/Kadran.qml
        qml/components/TikKutusu.qml
        qml/components/Ipucu.qml
        qml/components/Logo.qml
        qml/components/TitleBar.qml
        qml/pages/PanelPage.qml
        qml/pages/MemoryPage.qml
        qml/pages/CpuPage.qml
        qml/pages/HealthPage.qml
        qml/pages/FolderTreePage.qml
)

target_link_libraries(DunyaPusula PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    psapi
    version
    shell32
    gdi32
    dwmapi
    powrprof
    pdh
    wbemuuid
    opengl32
    d3d11
)

# Calisma zamani DLL/eklenti/QML modulleri exe yanina kopyala.
# windeployqt yok (qtbase[windeployqt] istenmedi - eklemek qtbase'i bastan
# derletir), bu yuzden elle kopyalanir.
get_target_property(QT_CORE_DLL Qt6::Core IMPORTED_LOCATION_RELEASE)
get_filename_component(QT_BIN_DIR "${QT_CORE_DLL}" DIRECTORY)
get_filename_component(QT_PREFIX "${QT_BIN_DIR}" DIRECTORY)

set(QT_RUNTIME_DLLS
    Qt6Core.dll Qt6Gui.dll Qt6Network.dll Qt6OpenGL.dll
    Qt6Qml.dll Qt6QmlMeta.dll Qt6QmlModels.dll Qt6QmlWorkerScript.dll
    Qt6Quick.dll Qt6QuickControls2.dll Qt6QuickControls2Basic.dll
    Qt6QuickControls2Impl.dll Qt6QuickControls2BasicStyleImpl.dll
    Qt6QuickLayouts.dll Qt6QuickTemplates2.dll
    double-conversion.dll md4c.dll pcre2-16.dll z.dll zstd.dll
)
foreach(dll ${QT_RUNTIME_DLLS})
    if(EXISTS "${QT_BIN_DIR}/${dll}")
        add_custom_command(TARGET DunyaPusula POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT_BIN_DIR}/${dll}" "$<TARGET_FILE_DIR:DunyaPusula>/${dll}")
    endif()
endforeach()

# Platform eklentisi + Qt'nin kendi QML modul agaci (QtQuick, Controls, ...).
add_custom_command(TARGET DunyaPusula POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${QT_PREFIX}/Qt6/plugins/platforms"
        "$<TARGET_FILE_DIR:DunyaPusula>/platforms"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${QT_PREFIX}/Qt6/qml"
        "$<TARGET_FILE_DIR:DunyaPusula>/qml")

# Donanim sensor ajani (LibreHardwareMonitor sarmalayici, ayri publish adimi
# gerektirir - bkz. sensor/yayinla.bat). Henuz yayinlanmamissa derleme
# kirilmasin diye EXISTS ile korunur.
set(SENSOR_AJANI_EXE "${CMAKE_SOURCE_DIR}/sensor/cikti/SensorAjani.exe")
if(EXISTS "${SENSOR_AJANI_EXE}")
    add_custom_command(TARGET DunyaPusula POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SENSOR_AJANI_EXE}" "$<TARGET_FILE_DIR:DunyaPusula>/SensorAjani.exe")
endif()
