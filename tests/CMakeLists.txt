
function(nuru_add_test name)
    qt_add_executable(${name} ${name}.cpp TestMedia.h LocalHttpServer.h)
    target_link_libraries(${name} PRIVATE nuru_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=nuru.*.info=false"
        TIMEOUT 120)
endfunction()

nuru_add_test(tst_database)
nuru_add_test(tst_playqueue)
nuru_add_test(tst_library)
nuru_add_test(tst_playback)
nuru_add_test(tst_streams)

# Launches the real application, visits every page and fails on any QML warning.
add_test(NAME app_smoke COMMAND nuru --smoke-test)
set_tests_properties(app_smoke PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;NURU_VLC_ARGS=--aout=dummy;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/smoke/data;XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/smoke/config;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/smoke/cache"
    TIMEOUT 60)
