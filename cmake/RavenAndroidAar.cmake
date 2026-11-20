# cmake -P script run by the raven_aar target (cmake/RavenAndroid.cmake): packs Raven's AAR.
#
#   AndroidManifest.xml     raven.android, minSdk
#   classes.jar             Raven's Java part
#   proguard.txt            the consumer rules: R8 keeps what libraven.so calls by name
#   R.txt                   empty: no resources
#   jni/<abi>/libraven.so   packed into the APK by the Android Gradle plugin
#   prefab/                 the same library with the headers, for an app's own native code:
#                           Gradle's "buildFeatures { prefab = true }", then in its CMake
#                           find_package(raven REQUIRED CONFIG) + target_link_libraries(app raven::raven)
#
# Inputs (-D): SOURCE_DIR STAGE AAR VERSION MIN_SDK ABI NDK_MAJOR LIBRARY STRIP CLASSES_JAR

foreach(var SOURCE_DIR STAGE AAR VERSION MIN_SDK ABI LIBRARY CLASSES_JAR)
	if(NOT ${var})
		message(FATAL_ERROR "RavenAndroidAar.cmake: ${var} is not set")
	endif()
endforeach()

file(REMOVE_RECURSE "${STAGE}")
file(MAKE_DIRECTORY "${STAGE}/jni/${ABI}")

configure_file("${SOURCE_DIR}/cmake/android/AndroidManifest.xml.in" "${STAGE}/AndroidManifest.xml" @ONLY)
configure_file("${SOURCE_DIR}/cmake/android/proguard-rules.pro" "${STAGE}/proguard.txt" COPYONLY)
configure_file("${CLASSES_JAR}" "${STAGE}/classes.jar" COPYONLY)
file(WRITE "${STAGE}/R.txt" "")

# libraven.so without its debug information (the build folder keeps the full one)
set(lib "${STAGE}/jni/${ABI}/libraven.so")
configure_file("${LIBRARY}" "${lib}" COPYONLY)
if(STRIP)
	execute_process(COMMAND "${STRIP}" --strip-unneeded "${lib}" RESULT_VARIABLE result)
	if(NOT result EQUAL 0)
		message(FATAL_ERROR "RavenAndroidAar.cmake: ${STRIP} failed on ${lib}")
	endif()
endif()

# prefab (schema 2): one module, raven, a shared library without an STL
set(module "${STAGE}/prefab/modules/raven")
file(WRITE "${STAGE}/prefab/prefab.json"
	"{\n  \"schema_version\": 2,\n  \"name\": \"raven\",\n  \"version\": \"${VERSION}\",\n  \"dependencies\": []\n}\n")
file(WRITE "${module}/module.json" "{\n  \"export_libraries\": [],\n  \"android\": {}\n}\n")
file(COPY "${SOURCE_DIR}/include/Raven.h" "${SOURCE_DIR}/include/raven" DESTINATION "${module}/include")
file(MAKE_DIRECTORY "${module}/libs/android.${ABI}")
file(WRITE "${module}/libs/android.${ABI}/abi.json"
	"{\n  \"abi\": \"${ABI}\",\n  \"api\": ${MIN_SDK},\n  \"ndk\": ${NDK_MAJOR},\n  \"stl\": \"none\",\n  \"static\": false\n}\n")
configure_file("${lib}" "${module}/libs/android.${ABI}/libraven.so" COPYONLY)

file(REMOVE "${AAR}")
execute_process(
	COMMAND "${CMAKE_COMMAND}" -E tar cf "${AAR}" --format=zip
		AndroidManifest.xml classes.jar proguard.txt R.txt jni prefab
	WORKING_DIRECTORY "${STAGE}"
	RESULT_VARIABLE result)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "RavenAndroidAar.cmake: could not write ${AAR}")
endif()
