cmake_minimum_required(VERSION 3.20)

project(AutoKK)

set(BUILD_TRIAL OFF CACHE BOOL "build trial_kkr")
set(BUILD_COUNTERGEN OFF CACHE BOOL "build trial_kkr")

set(CMAKE_BUILD_TYPE MinSizeRel)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BINARY_DIR "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_INCLUDE_CURRENT_DIRS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 17)

set(LCMS_DIR "${CMAKE_SOURCE_DIR}/3rd-party/lcms2")

find_library(LCMS_LIBS
             NAMES lcms
             HINTS "${LCMS_DIR}/lib"
             REQUIRED )
message(STATUS "${LCMS_LIBS}")

find_path(LCMS_INCLUDES
             NAMES lcms2.h lcms2_plugin.h
             HINTS "${LCMS_DIR}/include"
             REQUIRED )
message(STATUS "${LCMS_INCLUDES}")


set(OpenCV_ARCH x64)
set(OpenCV_RUNTIME mingw)
set(OpenCV_STATIC ON)

find_package(OpenCV
             HINTS "${CMAKE_SOURCE_DIR}/3rd-party/opencv"
             REQUIRED )


set(OPENSSL_ROOT_DIR ${CMAKE_SOURCE_DIR}/3rd-party/openssl-3.5.2/lib/mingw/x64)
set(OPENSSL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rd-party/openssl-3.5.2/include)
set(OPENSSL_USE_STATIC_LIB ON)
find_package(OpenSSL)

set(OpenSSL_INCLUDES ${CMAKE_SOURCE_DIR}/3rd-party/openssl-3.5.2/include)

find_package(Qt6 COMPONENTS Widgets Test Concurrent AxContainer REQUIRED )

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

qt_standard_project_setup()

add_library(UsesOpenCV INTERFACE)
add_library(UsesLCMS2 INTERFACE)
add_library(UsesOpenSSL INTERFACE)
add_library(UsesQt6 INTERFACE)

target_include_directories(UsesOpenCV INTERFACE ${OpenCV_INCLUDE_DIRS})
target_include_directories(UsesLCMS2 INTERFACE ${LCMS_INCLUDES})
target_include_directories(UsesOpenSSL INTERFACE ${OPENSSL_INCLUDE_DIR})

# target_link_directories(UsesOpenSSL INTERFACE ${OpenSSL_INCLUDE_DIRECTORY})

target_link_libraries(UsesOpenCV INTERFACE ${OpenCV_LIBS} Qt::Test Qt::Concurrent)
target_link_libraries(UsesLCMS2 INTERFACE ${LCMS_LIBS})
target_link_libraries(UsesOpenSSL INTERFACE ${OPENSSL_LIBRARIES} ws2_32 advapi32 gdi32 user32 crypt32)
target_link_libraries(UsesQt6 INTERFACE Qt::Widgets Qt::AxContainer)

# add_executable(kkr src/test_kkr.cpp)

# add_executable(fineTune src/finetunedialog.h
                   # src/finetunedialog.cpp
                   # src/finetunedialog.ui )

# add_executable(grokTest src/grok_test.cpp)


set(KKR_CORE src/kkr/kkrmainwindow.h
             src/kkr/kkrmainwindow.cpp
             src/kkr/ui/kkrmainwindow.ui
             src/cv/cvhelper.hpp
             src/cv/cvhelper.cpp
             src/kkr/kkrprogressdialog.h
             src/kkr/kkrprogressdialog.cpp
             src/kkr/ui/kkrprogressdialog.ui
             src/kkr/notifier.h
             src/kkr/notifier.cpp
             src/kkr/runnablecoreltask.cpp
             src/kkr/runnablecoreltask.h
             src/kkr/cmykcolordialog.h
             src/kkr/cmykcolordialog.cpp
             src/kkr/ui/cmykcolordialog.ui
             src/kkr/colorconverter.h
             src/kkr/colorconverter.cpp
             src/kkr/colortemplatemodel.h
             src/kkr/colortemplatemodel.cpp
             src/kkr/aboutkkrdialog.h
             src/kkr/aboutkkrdialog.cpp
             src/kkr/polledsettings.h
             src/kkr/polledsettings.cpp
             src/kkr/tokenrequestdialog.h
             src/kkr/ui/tokenrequestdialog.ui
             src/kkr/tokenrequestdialog.cpp
             src/kkr/tokenrefilldialog.h
             src/kkr/tokenrefilldialog.cpp
             src/kkr/ui/tokenrefilldialog.ui
             # src/kkr_countergen/countergenerator.h
             src/kkr_countergen/countergenerator.cpp
             resources/colormanagement.qrc
             resources/eruces.qrc
             src/kkr/corelchooser.cpp
             src/kkr/corelchooser.h
             )


add_library(kkrcorelib OBJECT ${KKR_CORE})

target_include_directories(kkrcorelib PUBLIC src)

target_link_libraries(kkrcorelib PUBLIC UsesQt6 UsesOpenCV UsesLCMS2 UsesOpenSSL)

add_executable(kkrwindow WIN32 
                         src/kkr/main_kkr.cpp
                         src/kkr/winres.rc)

# add_executable(trial_kkrwindow WIN32
                         # src/kkr-trial/main_kkr_trial.cpp
                         # src/kkr/winres.rc)

add_subdirectory(src/kkr_countergen)

# target_link_libraries(kkr PRIVATE kkrcorelib)
# target_link_libraries(fineTune PRIVATE kkrcorelib)
# target_link_libraries(grokTest PRIVATE kkrcorelib)
target_link_libraries(kkrwindow PRIVATE kkrcorelib)
# target_link_libraries(trial_kkrwindow PRIVATE kkrcorelib)



# set(deploy_script "${CMAKE_CURRENT_BINARY_DIR}/deploy_MyApp.cmake")
# file(GENERATE OUTPUT ${deploy_script} CONTENT "

# set(QT_DEPLOY_PLUGINS_DIR \"plugins\")
# set(QT_DEPLOY_QML_DIR \"qml\")
# set(QT_DEPLOY_TRANSLATIONS_DIR \"i18n\")

# include(\"${QT_DEPLOY_SUPPORT}\")

# qt_deploy_runtime_dependencies(
    # EXECUTABLE \"\${QT_DEPLOY_BIN_DIR}/$<TARGET_FILE_NAME:MyApp>\"
# )")

# install(SCRIPT ${deploy_script})