################# dependencies #################
### Qt/KDE
find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED CONFIG COMPONENTS Widgets)

################# breezestyle target #################
set(breezechameleoncommon_LIB_SRCS
    breezeboxshadowrenderer.cpp
)

add_library(breezechameleoncommon6 ${breezechameleoncommon_LIB_SRCS})

generate_export_header(breezechameleoncommon6
    BASE_NAME breezecommon
    EXPORT_FILE_NAME breezecommon_export.h)

target_link_libraries(breezechameleoncommon6
    PUBLIC
        Qt::Core
        Qt::Gui)

set_target_properties(breezechameleoncommon6 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

install(TARGETS breezechameleoncommon6 ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
