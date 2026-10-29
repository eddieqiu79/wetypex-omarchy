# SPDX-License-Identifier: GPL-3.0-only
find_package(Python3 REQUIRED COMPONENTS Interpreter)
install(PROGRAMS omarchy/wetypex-image-helper.py DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME wetypex-image-helper)
install(PROGRAMS omarchy/wetypex-remote-image.py DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME wetypex-remote-image)
install(PROGRAMS omarchy/wetypex-files DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY compat/ DESTINATION compat FILES_MATCHING PATTERN "*.py" PATTERN "LICENSE" PATTERN "NOTICE")
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
