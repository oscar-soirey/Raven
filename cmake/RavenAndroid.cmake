# Android packaging (included by the top CMakeLists.txt when the platform is android).
#
#   raven_java  (built by default)  Raven's Java part, src/platform/android/java (the
#               InputConnection of the on-screen keyboard), compiled with javac and d8:
#                 <build>/android/classes.jar   for an AAR or a Gradle module
#                 <build>/android/classes.dex   for an APK packed by hand
#   raven_aar   (cmake --build <build> --target raven_aar, needs RAVEN_BUILD_SHARED=ON)
#                 <build>/raven-<version>-<abi>.aar  with libraven.so of this build's ABI
#               tools/mobile/build-android-aar.sh builds one AAR with several ABIs.
#
# The tools come from the Android SDK: ANDROID_HOME, ANDROID_SDK_ROOT, or the SDK that
# holds the NDK of the toolchain (<sdk>/ndk/<version>); the newest build tools (d8) and
# the newest platform (android.jar) found there; javac from JAVA_HOME or the PATH.
# RAVEN_ANDROID_SDK, RAVEN_ANDROID_BUILD_TOOLS and RAVEN_ANDROID_JAR override them.
# When one is missing, the Java part and the AAR are skipped with a warning: libraven
# still builds, and an app without the Java part types through key events only (no
# suggestions, ASCII only). RAVEN_ANDROID_JAVA=OFF skips them without a warning.

include(GNUInstallDirs)

option(RAVEN_ANDROID_JAVA "Android: compile Raven's Java part (classes.jar, classes.dex) and offer the raven_aar target" ON)
set(RAVEN_ANDROID_SDK "" CACHE PATH "Android: the SDK (default: ANDROID_HOME, ANDROID_SDK_ROOT, or the SDK holding the NDK)")
set(RAVEN_ANDROID_BUILD_TOOLS "" CACHE PATH "Android: the build tools folder with d8 (default: the newest of the SDK)")
set(RAVEN_ANDROID_JAR "" CACHE FILEPATH "Android: the android.jar compiled against (default: the newest platform of the SDK)")

# the API level of the NDK build (ANDROID_PLATFORM): the minSdk of the AAR and of d8
if(ANDROID_PLATFORM_LEVEL)
	set(RAVEN_ANDROID_MIN_SDK "${ANDROID_PLATFORM_LEVEL}")
elseif(CMAKE_C_COMPILER_TARGET MATCHES "android([0-9]+)$")
	set(RAVEN_ANDROID_MIN_SDK "${CMAKE_MATCH_1}")
elseif(CMAKE_SYSTEM_VERSION GREATER 1)
	set(RAVEN_ANDROID_MIN_SDK "${CMAKE_SYSTEM_VERSION}")
else()
	set(RAVEN_ANDROID_MIN_SDK 29)
endif()
if(CMAKE_ANDROID_ARCH_ABI)
	set(RAVEN_ANDROID_ABI "${CMAKE_ANDROID_ARCH_ABI}")
else()
	set(RAVEN_ANDROID_ABI "${ANDROID_ABI}")
endif()

# libraven.so: 16 KB pages (Android 15 devices with 16 KB pages; Google Play wants it since
# November 2025). NDK r28 does it by default, older NDKs need the flag.
if(RAVEN_BUILD_SHARED)
	target_link_options(raven PRIVATE "LINKER:-z,max-page-size=16384")
endif()

if(NOT RAVEN_ANDROID_JAVA)
	return()
endif()

# --- the tools ---------------------------------------------------------------

# the newest "<dir>/<prefix><version>" holding <file>, by version
function(raven_android_newest out dir prefix file)
	set(best "")
	set(best_version "")
	file(GLOB candidates LIST_DIRECTORIES true "${dir}/${prefix}*")
	foreach(candidate ${candidates})
		get_filename_component(name "${candidate}" NAME)
		string(REPLACE "${prefix}" "" version "${name}")
		if(EXISTS "${candidate}/${file}" AND version MATCHES "^[0-9.]+$" AND
			(best STREQUAL "" OR version VERSION_GREATER best_version))
			set(best "${candidate}")
			set(best_version "${version}")
		endif()
	endforeach()
	set(${out} "${best}" PARENT_SCOPE)
endfunction()

set(raven_sdk "${RAVEN_ANDROID_SDK}")
if(NOT raven_sdk)
	if(DEFINED ENV{ANDROID_HOME} AND EXISTS "$ENV{ANDROID_HOME}")
		set(raven_sdk "$ENV{ANDROID_HOME}")
	elseif(DEFINED ENV{ANDROID_SDK_ROOT} AND EXISTS "$ENV{ANDROID_SDK_ROOT}")
		set(raven_sdk "$ENV{ANDROID_SDK_ROOT}")
	elseif(CMAKE_ANDROID_NDK AND EXISTS "${CMAKE_ANDROID_NDK}/../../build-tools")
		get_filename_component(raven_sdk "${CMAKE_ANDROID_NDK}/../.." ABSOLUTE)
	endif()
endif()

set(raven_build_tools "${RAVEN_ANDROID_BUILD_TOOLS}")
if(NOT raven_build_tools AND raven_sdk)
	raven_android_newest(raven_build_tools "${raven_sdk}/build-tools" "" "d8")
endif()
set(raven_android_jar "${RAVEN_ANDROID_JAR}")
if(NOT raven_android_jar AND raven_sdk)
	raven_android_newest(raven_platform "${raven_sdk}/platforms" "android-" "android.jar")
	if(raven_platform)
		set(raven_android_jar "${raven_platform}/android.jar")
	endif()
endif()
if(CMAKE_HOST_WIN32)
	set(raven_d8 "${raven_build_tools}/d8.bat")
else()
	set(raven_d8 "${raven_build_tools}/d8")
endif()
# javac runs on the host (the NDK toolchain looks for programs on the host too)
find_program(RAVEN_JAVAC javac HINTS "$ENV{JAVA_HOME}/bin" NO_CMAKE_FIND_ROOT_PATH)
if(RAVEN_JAVAC)
	# macOS has a /usr/bin/javac that only says that no JDK is installed
	execute_process(COMMAND "${RAVEN_JAVAC}" -version RESULT_VARIABLE raven_javac_result OUTPUT_QUIET ERROR_QUIET)
	if(NOT raven_javac_result EQUAL 0)
		set(RAVEN_JAVAC "RAVEN_JAVAC-NOTFOUND" CACHE FILEPATH "javac" FORCE)
	endif()
endif()

set(raven_missing "")
if(NOT RAVEN_JAVAC)
	list(APPEND raven_missing "javac (a JDK: JAVA_HOME or the PATH)")
endif()
if(NOT raven_build_tools OR NOT EXISTS "${raven_d8}")
	list(APPEND raven_missing "d8 (the SDK's build tools: ANDROID_HOME or RAVEN_ANDROID_BUILD_TOOLS)")
endif()
if(NOT raven_android_jar OR NOT EXISTS "${raven_android_jar}")
	list(APPEND raven_missing "android.jar (an SDK platform: ANDROID_HOME or RAVEN_ANDROID_JAR)")
endif()
if(raven_missing)
	string(REPLACE ";" ", " raven_missing "${raven_missing}")
	message(WARNING "Raven: not found: ${raven_missing}. Raven's Java part (classes.jar / classes.dex) "
		"and the raven_aar target are skipped: an app without the Java part types through key events "
		"only (no suggestions, ASCII only). -DRAVEN_ANDROID_JAVA=OFF skips them without this warning.")
	return()
endif()
message(STATUS "Raven: Android Java part (minSdk ${RAVEN_ANDROID_MIN_SDK}) with ${RAVEN_JAVAC}, ${raven_d8}, ${raven_android_jar}")

# --- raven_java: classes.jar + classes.dex -----------------------------------

set(RAVEN_ANDROID_OUT "${CMAKE_BINARY_DIR}/android")
file(GLOB_RECURSE RAVEN_JAVA_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/android/java/*.java")
set(RAVEN_JAVA_JAR "${RAVEN_ANDROID_OUT}/classes.jar")
set(RAVEN_JAVA_DEX "${RAVEN_ANDROID_OUT}/classes.dex")
# Java 8 class files: what d8 and every Android Gradle plugin read
add_custom_command(OUTPUT "${RAVEN_JAVA_JAR}"
	COMMAND "${CMAKE_COMMAND}" -E remove_directory "${RAVEN_ANDROID_OUT}/classes"
	COMMAND "${CMAKE_COMMAND}" -E make_directory "${RAVEN_ANDROID_OUT}/classes"
	COMMAND "${RAVEN_JAVAC}" -source 8 -target 8 -bootclasspath "${raven_android_jar}" -Xlint:-options
		-encoding UTF-8 -d "${RAVEN_ANDROID_OUT}/classes" ${RAVEN_JAVA_SOURCES}
	COMMAND "${CMAKE_COMMAND}" -E chdir "${RAVEN_ANDROID_OUT}/classes"
		"${CMAKE_COMMAND}" -E tar cf "${RAVEN_JAVA_JAR}" --format=zip .
	DEPENDS ${RAVEN_JAVA_SOURCES}
	COMMENT "Raven: compiling the Java part (classes.jar)"
	VERBATIM)
add_custom_command(OUTPUT "${RAVEN_JAVA_DEX}"
	COMMAND "${raven_d8}" --release --min-api ${RAVEN_ANDROID_MIN_SDK} --lib "${raven_android_jar}"
		--output "${RAVEN_ANDROID_OUT}" "${RAVEN_JAVA_JAR}"
	DEPENDS "${RAVEN_JAVA_JAR}"
	COMMENT "Raven: dexing the Java part (classes.dex)"
	VERBATIM)
add_custom_target(raven_java ALL DEPENDS "${RAVEN_JAVA_JAR}" "${RAVEN_JAVA_DEX}")
install(FILES "${RAVEN_JAVA_JAR}" DESTINATION ${CMAKE_INSTALL_DATADIR}/raven/android RENAME raven-android.jar)
install(FILES "${RAVEN_JAVA_DEX}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/android/proguard-rules.pro"
	DESTINATION ${CMAKE_INSTALL_DATADIR}/raven/android)

# --- raven_aar: the AAR of this build's ABI -----------------------------------

if(NOT RAVEN_BUILD_SHARED)
	message(STATUS "Raven: the raven_aar target needs -DRAVEN_BUILD_SHARED=ON (an AAR holds libraven.so)")
	return()
endif()
if(CMAKE_ANDROID_NDK_VERSION)
	string(REGEX MATCH "^[0-9]+" RAVEN_ANDROID_NDK_MAJOR "${CMAKE_ANDROID_NDK_VERSION}")
else()
	set(RAVEN_ANDROID_NDK_MAJOR 0)
endif()
set(RAVEN_AAR "${CMAKE_BINARY_DIR}/raven-${PROJECT_VERSION}-${RAVEN_ANDROID_ABI}.aar")
add_custom_command(OUTPUT "${RAVEN_AAR}"
	COMMAND "${CMAKE_COMMAND}"
		-D "SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
		-D "STAGE=${RAVEN_ANDROID_OUT}/aar"
		-D "AAR=${RAVEN_AAR}"
		-D "VERSION=${PROJECT_VERSION}"
		-D "MIN_SDK=${RAVEN_ANDROID_MIN_SDK}"
		-D "ABI=${RAVEN_ANDROID_ABI}"
		-D "NDK_MAJOR=${RAVEN_ANDROID_NDK_MAJOR}"
		-D "LIBRARY=$<TARGET_FILE:raven>"
		-D "STRIP=${CMAKE_STRIP}"
		-D "CLASSES_JAR=${RAVEN_JAVA_JAR}"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RavenAndroidAar.cmake"
	DEPENDS raven "${RAVEN_JAVA_JAR}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RavenAndroidAar.cmake"
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/android/AndroidManifest.xml.in"
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/android/proguard-rules.pro"
		${RAVEN_PUBLIC_HEADERS}
	COMMENT "Raven: packing ${RAVEN_AAR}"
	VERBATIM)
add_custom_target(raven_aar DEPENDS "${RAVEN_AAR}")
add_dependencies(raven_aar raven_java)
