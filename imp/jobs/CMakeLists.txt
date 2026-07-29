add_library(imp_jobs STATIC
	src/job_system.cpp
 "include/jobs/job_system.h")

target_include_directories(imp_jobs
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_jobs PUBLIC imp_core)
