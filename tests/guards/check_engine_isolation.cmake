cmake_minimum_required(VERSION 3.25)

if (NOT DEFINED IMP_ROOT OR NOT IS_DIRECTORY "${IMP_ROOT}")
	message(FATAL_ERROR "check_engine_isolation: pass -DIMP_ROOT=<path to the imp/ directory>")
endif()

set(GAME_CONTENT_NAMES khr-sponza sanmiguel environment_test)

set(violation_report "")
set(violation_count 0)
set(scanned 0)

function(imp_check_pattern text regex message)
	string(REGEX MATCH "${regex}" match "${text}")
	if ("${match}" STREQUAL "")
		return()
	endif()

	string(FIND "${text}" "${match}" offset)
	string(SUBSTRING "${text}" 0 ${offset} prefix)
	string(REGEX MATCHALL "\n" newlines "${prefix}")
	list(LENGTH newlines line)
	math(EXPR line "${line} + 1")

	set(entry "  ${f}:${line}: ${message}")
	if ("SHOW_MATCH" IN_LIST ARGN)
		string(STRIP "${match}" shown)
		string(REPLACE ";" "," shown "${shown}")
		set(entry "${entry}  [${shown}]")
	endif()

	math(EXPR count "${violation_count} + 1")
	set(violation_count "${count}" PARENT_SCOPE)
	set(violation_report "${violation_report}\n${entry}" PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE engine_files LIST_DIRECTORIES false
	"${IMP_ROOT}/*.h" "${IMP_ROOT}/*.hpp" "${IMP_ROOT}/*.inl"
	"${IMP_ROOT}/*.c" "${IMP_ROOT}/*.cc" "${IMP_ROOT}/*.cpp" "${IMP_ROOT}/*.cxx" "${IMP_ROOT}/*.mm"
	"${IMP_ROOT}/*.cmake" "${IMP_ROOT}/CMakeLists.txt" "${IMP_ROOT}/*/CMakeLists.txt"
	"${IMP_ROOT}/*.glsl" "${IMP_ROOT}/*.vert" "${IMP_ROOT}/*.frag" "${IMP_ROOT}/*.comp"
	"${IMP_ROOT}/*.lua"
)

foreach (f IN LISTS engine_files)
	if (f MATCHES "/(third_party|vendor|build)/")
		continue()
	endif()

	file(READ "${f}" content)
	string(TOLOWER "${content}" content_lower)
	math(EXPR scanned "${scanned} + 1")

	# Rule 1: Includes of game headers.
	imp_check_pattern("${content}" "(^|\n)[ \t]*#[ \t]*include[ \t]*[<\"]game/[^\n]*"
		"engine code includes a game header" SHOW_MATCH)

	# Rule 2: Linking or referencing the game from the engine's build files.
	if (f MATCHES "CMakeLists\\.txt$" OR f MATCHES "\\.cmake$")
		imp_check_pattern("${content}" "(^|[^A-Za-z0-9_])game_lib([^A-Za-z0-9_]|$)"
			"engine build file references game_lib")
		imp_check_pattern("${content}" "CMAKE_SOURCE_DIR[}]/game[/\" )]"
			"engine build file reaches into the game directory")
	endif()

	# Rule 3: Game content (assets) names.
	foreach (name IN LISTS GAME_CONTENT_NAMES)
		imp_check_pattern("${content_lower}" "[^\n]*${name}[^\n]*"
			"engine code names game content '${name}'")
	endforeach()
endforeach()

# Rule 4: The game must not reach into engine internals from its build files.
if (DEFINED GAME_ROOT AND EXISTS "${GAME_ROOT}/CMakeLists.txt")
	set(f "${GAME_ROOT}/CMakeLists.txt")
	file(READ "${f}" content)
	imp_check_pattern("${content}" "IMP_ENGINE_SOURCE_DIR"
		"game CMake reaches into the engine source tree" SHOW_MATCH)
	imp_check_pattern("${content}" "IMP_SHADER_[A-Z_]+"
		"game CMake sets engine-private shader tooling definitions" SHOW_MATCH)
endif()

if (violation_count GREATER 0)
	message(FATAL_ERROR
		"Engine isolation violations (${violation_count}):${violation_report}\n\n"
		"The engine under imp/ must not depend on the game.")
endif()

message(STATUS "Engine isolation guard passed (${scanned} engine files scanned)")
