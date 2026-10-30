# SPDX-License-Identifier: GPL-3.0-only
find_package(Python3 REQUIRED COMPONENTS Interpreter)
install(PROGRAMS omarchy/wetypex-image-helper.py DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME wetypex-image-helper)
install(PROGRAMS omarchy/wetypex-remote-image.py DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME wetypex-remote-image)
install(PROGRAMS omarchy/wetypex-files DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY compat/ DESTINATION ${CMAKE_INSTALL_DATADIR}/fcitx5-wetypex/compat FILES_MATCHING PATTERN "*.py" PATTERN "LICENSE" PATTERN "NOTICE")
install(FILES LICENSES.md DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/fcitx5-wetypex)
configure_file(omarchy/wetypex-sync.service.in wetypex-sync.service @ONLY)
# Templates only: never enable services or change existing Fcitx configuration.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wetypex-sync.service omarchy/fcitx-sync-owner.conf
  DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/fcitx5-wetypex/omarchy)
option(WETYPE_OMARCHY_TESTS "Register offline Omarchy regression tests" ON)
if(WETYPE_OMARCHY_TESTS)
  enable_testing()
  foreach(test sync-stability image-stability pipe-images)
    add_test(NAME omarchy-${test}
      COMMAND ${CMAKE_COMMAND} -E env
        "WETYPE_NATIVE_CLIPBOARD_HELPER=$<TARGET_FILE:fcitx5-wetypex-clipboard-set>"
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check-wetypex-${test}.py)
  endforeach()
endif()

install(PROGRAMS omarchy/wetypex-omarchy-setup DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES VERSION DESTINATION ${CMAKE_INSTALL_DATADIR}/fcitx5-wetypex)

install(PROGRAMS omarchy/wetypex-omarchy-package DESTINATION ${CMAKE_INSTALL_BINDIR})

if(WETYPE_OMARCHY_TESTS)
  find_package(${WETYPE_QT_PACKAGE} REQUIRED COMPONENTS Test)
  foreach(gui settings transfer)
    add_executable(wetypex-${gui}-tests tests/${gui}-gui.cpp)
    set_target_properties(wetypex-${gui}-tests PROPERTIES AUTOMOC TRUE)
    get_target_property(gui_definitions fcitx5-wetypex-${gui} COMPILE_DEFINITIONS)
    get_target_property(gui_libraries fcitx5-wetypex-${gui} LINK_LIBRARIES)
    target_compile_definitions(wetypex-${gui}-tests PRIVATE ${gui_definitions})
    target_link_libraries(wetypex-${gui}-tests PRIVATE ${gui_libraries} ${WETYPE_QT_PACKAGE}::Test)
    add_test(NAME omarchy-${gui}-gui COMMAND wetypex-${gui}-tests)
    set_tests_properties(omarchy-${gui}-gui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DBUS_SESSION_BUS_ADDRESS=unix:path=/nonexistent-wetypex-test-bus")
  endforeach()
  foreach(test update-policy release-channel package-client account-preflight)
    add_test(NAME omarchy-${test} COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check-${test}.py)
  endforeach()
endif()

if(WETYPE_OMARCHY_TESTS)
  add_executable(wetypex-settings-window-test tests/settings-window.cpp)
  get_target_property(settings_definitions fcitx5-wetypex-settings COMPILE_DEFINITIONS)
  get_target_property(settings_libraries fcitx5-wetypex-settings LINK_LIBRARIES)
  target_compile_definitions(wetypex-settings-window-test PRIVATE ${settings_definitions})
  target_link_libraries(wetypex-settings-window-test PRIVATE ${settings_libraries})
  add_test(NAME omarchy-settings-window COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check-settings-window.py $<TARGET_FILE:wetypex-settings-window-test>)
endif()
install(FILES data/wetypex-omarchy-files.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
