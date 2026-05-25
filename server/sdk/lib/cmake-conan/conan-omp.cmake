function(conan_create_profile profile)
	conan_check(REQUIRED DETECT_QUIET)
	execute_process(
		COMMAND ${CONAN_CMD} profile new ${profile} --detect
		RESULT_VARIABLE return_code
		OUTPUT_VARIABLE fake_output
		ERROR_VARIABLE fake_output
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)
endfunction()

function(conan_update_profile profile setting)
	conan_check(REQUIRED DETECT_QUIET)
	execute_process(
		COMMAND ${CONAN_CMD} profile update ${setting} ${profile}
		RESULT_VARIABLE return_code
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)
	if(NOT "${return_code}" STREQUAL "0")
		message(FATAL_ERROR "Couldn't update build profile")
	endif()
endfunction()

if (NOT TARGET_BUILD_ARCH)
	if (MSVC_CXX_ARCHITECTURE_ID)
		string(TOLOWER ${MSVC_CXX_ARCHITECTURE_ID} LOWERCASE_CMAKE_SYSTEM_PROCESSOR)
		if (LOWERCASE_CMAKE_SYSTEM_PROCESSOR MATCHES "(x64|x86_64|amd64)")
			set(TARGET_BUILD_ARCH x86_64)
		elseif (LOWERCASE_CMAKE_SYSTEM_PROCESSOR MATCHES "(i[3-6]86|x86)")
			set(TARGET_BUILD_ARCH x86)
		else ()
			message(FATAL_ERROR "MSVC Arch ID: Unknown CPU '${LOWERCASE_CMAKE_SYSTEM_PROCESSOR}'")
		endif ()
	else ()
		if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x64|x86_64|amd64)")
			set(TARGET_BUILD_ARCH x86_64)
		elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "(i[3-6]86|x86)")
			set(TARGET_BUILD_ARCH x86)
			set(CMAKE_C_FLAGS "-m32 ${CMAKE_C_FLAGS}")
			set(CMAKE_CXX_FLAGS "-m32 ${CMAKE_CXX_FLAGS}")
			set(CMAKE_SIZEOF_VOID_P 4)
		else ()
			if (CMAKE_SYSTEM_PROCESSOR MATCHES "(arm64)")
				set(TARGET_BUILD_ARCH armv8)
			else ()
				set(TARGET_BUILD_ARCH x86)
			endif ()
		endif ()
	endif ()
endif()

if (NOT CONAN_OMP_PROFILE_NAME)
	set(CONAN_OMP_PROFILE_NAME omp_host)
endif()
if (NOT CONAN_OMP_BUILD_PROFILE_NAME)
	set(CONAN_OMP_BUILD_PROFILE_NAME omp_build)
endif()

conan_create_profile(${CONAN_OMP_PROFILE_NAME})
conan_update_profile(${CONAN_OMP_PROFILE_NAME} "options.*:shared=False")
conan_update_profile(${CONAN_OMP_PROFILE_NAME} "settings.arch=${TARGET_BUILD_ARCH}")

conan_create_profile(${CONAN_OMP_BUILD_PROFILE_NAME})
conan_update_profile(${CONAN_OMP_BUILD_PROFILE_NAME} "options.*:shared=False")

# Automatically download and set up paths to a library on Conan
# Works for multi-configuration
function(conan_omp_add_lib_opt pkg_name pkg_version pkg_options)
	set(CONAN_DISABLE_CHECK_COMPILER TRUE)
	conan_cmake_run(
		REQUIRES ${pkg_name}/${pkg_version}
		PROFILE_BUILD ${CONAN_OMP_BUILD_PROFILE_NAME}
		PROFILE ${CONAN_OMP_PROFILE_NAME}
		BASIC_SETUP CMAKE_TARGETS
		BUILD missing
		PROFILE_AUTO build_type compiler compiler.version compiler.runtime compiler.libcxx compiler.toolset
		OPTIONS ${pkg_options}
	)

	set_target_properties(CONAN_PKG::${pkg_name} PROPERTIES IMPORTED_GLOBAL TRUE)
endfunction()

function(conan_omp_add_lib pkg_name pkg_version)
	conan_omp_add_lib_opt(${pkg_name} ${pkg_version} "")
endfunction()
