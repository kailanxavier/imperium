enable_testing()

add_test(
	NAME engine_game_isolation_guard
	COMMAND ${CMAKE_COMMAND}
		-DIMP_ROOT=${CMAKE_SOURCE_DIR}/imp
		-DGAME_ROOT=${CMAKE_SOURCE_DIR}/game
		-P ${CMAKE_CURRENT_SOURCE_DIR}/check_engine_isolation.cmake
)
