cmake_minimum_required(VERSION 3.16)
project(what_to_eat VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Network Multimedia)

set(SOURCES
    main.cpp
    mainwindow.cpp
    mealpage.cpp
    dishdata.cpp
    fuzzysearch.cpp
    searchwidget.cpp
    recommend.cpp
    gachawidget.cpp
    radarchartwidget.cpp
    historywindow.cpp
    chartwidgets.cpp
    decopainter.cpp
    sketchyui.cpp
    mapview.cpp
    settingsdialog.cpp
    zonemanager.cpp
    zoneeditor.cpp
    distancedb.cpp
    userdata.cpp
    calendarwindow.cpp
    statisticswindow.cpp
    aireportdialog.cpp
    achievementmanager.cpp
    achievementpage.cpp
    manualwindow.cpp
)

set(HEADERS
    mainwindow.h
    mealpage.h
    dishdata.h
    fuzzysearch.h
    searchwidget.h
    recommend.h
    gachawidget.h
    radarchartwidget.h
    historywindow.h
    chartwidgets.h
    decopainter.h
    sketchyui.h
    mapview.h
    settingsdialog.h
    zonemanager.h
    zoneeditor.h
    zonecommon.h
    distancedb.h
    userdata.h
    calendarwindow.h
    statisticswindow.h
    aireportdialog.h
    achievementdata.h
    achievementmanager.h
    achievementpage.h
    manualwindow.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::Multimedia
)

# Windows: ensure GUI app (no console window), link qtmain entry point
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
)
