# Based on https://github.com/percolator/percolator/blob/763d90c137f4ee6f0d4f19be4aeb7e59472b5ef5/CPack.txt

find_package(Git)

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # Parse the tag for version information if it contains it.
    if(${GIT_TAGS} MATCHES "^v[0-9]+\\.[0-9]+\\.([0-9]+).*")
        string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" GIT_VERSION_MAJOR "${GIT_TAGS}")
        string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" GIT_VERSION_MINOR "${GIT_TAGS}")
        string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" GIT_VERSION_PATCH "${GIT_TAGS}")
    endif()
endif()

# PACKAGING OPTIONS: GENERAL
if(WIN32)
    set(CPACK_PACKAGE_NAME "Vanilla-Conquer")
else()
    set(CPACK_PACKAGE_NAME "vanilla-conquer")
endif()

set(VANILLA_INSTALL_GUID "a054cfaa-a5bf-482a-bb2c-648ec58bf5e4")
set(CPACK_PACKAGE_FILE_NAME "vanilla-conquer-installer")
set(CPACK_SET_DESTDIR ON) # Enabling absolute paths for CPack (important!)
set(CPACK_SOURCE_GENERATOR "TGZ") # This file format is used to package source code ("make package_source")
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};${CMAKE_PROJECT_NAME};ALL;/")
set(CPACK_PACKAGE_VENDOR "Assembly Armada")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_PACKAGE_VERSION_MAJOR ${GIT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GIT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GIT_VERSION_PATCH})

if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")  
    set(CPACK_SET_DESTDIR OFF)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_COMPONENT_VANILLARUNTIME_DISPLAY_NAME "Runtime Libraries")
    
    if(BUILD_VANILLATD)
        set(CPACK_COMPONENT_VANILLATD_DEPENDS VanillaRuntime)
        set(CPACK_COMPONENT_VANILLATD_DISPLAY_NAME "Vanilla TD")
        set(CPACK_COMPONENT_VANILLATD_GROUP "Games")
        if(MSVC)
            set(CPACK_COMPONENT_VANILLATDDEBUG_DISPLAY_NAME "Vanilla TD Debug Info")
            set(CPACK_COMPONENT_VANILLATDDEBUG_GROUP "Debug Info")
            set(CPACK_COMPONENT_VANILLATDDEBUG_DISABLED ON)
        endif()
        list(APPEND CPACK_NSIS_CREATE_ICONS_EXTRA "  CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Vanilla TD.lnk' '$INSTDIR\\\\bin\\\\vanillatd.exe'")
        list(APPEND CPACK_NSIS_DELETE_ICONS_EXTRA "  Delete '$SMPROGRAMS\\\\$START_MENU\\\\Vanilla TD.lnk'")
    endif()
    
    if(BUILD_VANILLARA)
        set(CPACK_COMPONENT_VANILLARA_DEPENDS VanillaRuntime)
        set(CPACK_COMPONENT_VANILLARA_DISPLAY_NAME "Vanilla RA")
        set(CPACK_COMPONENT_VANILLARA_GROUP "Games")
        if(MSVC)
            set(CPACK_COMPONENT_VANILLARADEBUG_DISPLAY_NAME "Vanilla TD Debug Info")
            set(CPACK_COMPONENT_VANILLARADEBUG_GROUP "Debug Info")
            set(CPACK_COMPONENT_VANILLARADEBUG_DISABLED ON)
        endif()
        list(APPEND CPACK_NSIS_CREATE_ICONS_EXTRA "  CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Vanilla RA.lnk' '$INSTDIR\\\\bin\\\\vanillara.exe'")
        list(APPEND CPACK_NSIS_DELETE_ICONS_EXTRA "  Delete '$SMPROGRAMS\\\\$START_MENU\\\\Vanilla RA.lnk'")
    endif()
    
    # Fixup the icon directives with newlines in place of list separators.
    string (REPLACE ";" "\n" CPACK_NSIS_CREATE_ICONS_EXTRA "${CPACK_NSIS_CREATE_ICONS_EXTRA}")
    string (REPLACE ";" "\n" CPACK_NSIS_DELETE_ICONS_EXTRA "${CPACK_NSIS_DELETE_ICONS_EXTRA}")
    
    set(CPACK_COMPONENT_VANILLARUNTIME_HIDDEN ON)
    set(CPACK_COMPONENT_VANILLARUNTIME_REQUIRED ON)
    
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_DISPLAY_NAME "Vanilla Conquer")
    set(CPACK_NSIS_PACKAGE_NAME "Vanilla Conquer")
    
    # Use the TD icon for the uninstall unless we didn't build TD.
    if(BUILD_VANILLATD)
        set(CPACK_NSIS_INSTALLED_ICON_NAME bin\\\\vanillatd.exe)
    else()
        set(CPACK_NSIS_INSTALLED_ICON_NAME bin\\\\vanillara.exe)
    endif()
endif()

if(UNIX)
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(SPECIFIC_SYSTEM_VERSION_NAME "${CMAKE_SYSTEM_NAME}")
    set(CPACK_GENERATOR "TGZ")
    if(EXISTS "/etc/redhat-release")
      set(LINUX_NAME "")
      file(READ "/etc/redhat-release" LINUX_ISSUE)
    elseif(EXISTS "/etc/issue")
      set(LINUX_NAME "")
      file(READ "/etc/issue" LINUX_ISSUE)
    endif()
    if(DEFINED LINUX_ISSUE)
      # Fedora case
      if(LINUX_ISSUE MATCHES "Fedora")
        string(REGEX MATCH "release ([0-9]+)" FEDORA "${LINUX_ISSUE}")
        set(LINUX_NAME "FC${CMAKE_MATCH_1}")
        set(CPACK_GENERATOR "RPM")
      endif(LINUX_ISSUE MATCHES "Fedora")
      # Red Hat case
      if(LINUX_ISSUE MATCHES "Red")
        string(REGEX MATCH "release ([0-9]+\\.[0-9]+)" REDHAT "${LINUX_ISSUE}")
        set(LINUX_NAME "RHEL_${CMAKE_MATCH_1}")
        set(CPACK_GENERATOR "RPM")
      endif(LINUX_ISSUE MATCHES "Red")
      # CentOS case
      if(LINUX_ISSUE MATCHES "CentOS")
        string(REGEX MATCH "release ([0-9]+\\.[0-9]+)" CENTOS "${LINUX_ISSUE}")
        set(LINUX_NAME "CentOS_${CMAKE_MATCH_1}")
        set(CPACK_GENERATOR "RPM")
      endif(LINUX_ISSUE MATCHES "CentOS")
      # Ubuntu case
      if(LINUX_ISSUE MATCHES "Ubuntu")
        string(REGEX MATCH "buntu ([0-9]+\\.[0-9]+)" UBUNTU "${LINUX_ISSUE}")
        set(LINUX_NAME "Ubuntu_${CMAKE_MATCH_1}")
        set(CPACK_GENERATOR "DEB")
      endif(LINUX_ISSUE MATCHES "Ubuntu")
      # Debian case
      if(LINUX_ISSUE MATCHES "Debian")
        string(REGEX MATCH "Debian .*ux ([a-zA-Z]*/?[a-zA-Z]*) .*" DEBIAN "${LINUX_ISSUE}")
        set(LINUX_NAME "Debian_${CMAKE_MATCH_1}")
        string(REPLACE "/" "_" LINUX_NAME ${LINUX_NAME})
        set(CPACK_GENERATOR "DEB")
      endif(LINUX_ISSUE MATCHES "Debian")
      # Open SuSE case
      if(LINUX_ISSUE MATCHES "SUSE")
        string(REGEX MATCH "SUSE  ([0-9]+\\.[0-9]+)" SUSE "${LINUX_ISSUE}")
        set(LINUX_NAME "openSUSE_${CMAKE_MATCH_1}")
        string(REPLACE "/" "_" LINUX_NAME ${LINUX_NAME})
        set(CPACK_GENERATOR "RPM")
      endif(LINUX_ISSUE MATCHES "SUSE")
    endif(DEFINED LINUX_ISSUE)
  endif(CMAKE_SYSTEM_NAME MATCHES "Linux")
endif(UNIX)

set(CPACK_STRIP_FILES TRUE)

file(GLOB_RECURSE DOT_FILES_BEGIN ".*") # To be ignored by Cpack
file(GLOB_RECURSE TILD_FILES "*~*") # To be ignored by Cpack

set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/.svn/;/.swp$/;cscope.*;/.git/;${CMAKE_CURRENT_BINARY_DIR}/;/.bzr/;/.settings/;${DOT_FILES_BEGIN};${TILD_FILES}")

# PACKAGING OPTIONS: DEB
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Assembly Armada")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${TARGET_ARCH})
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0 (>= 2.0.5), libopenal1 (>= 1.17)")

# PACKAGING OPTIONS: RPM
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3 license")
set(CPACK_RPM_PACKAGE_VENDOR "Assembly Armada")

include(CPack)
