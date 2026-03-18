file(READ "${LOGFILE}" CONTENT)

string(REGEX MATCH "warning:" HAS_WARNING "${CONTENT}")

if(HAS_WARNING)
    message(FATAL_ERROR "Documentation lint failed:\n${CONTENT}")
else()
    message(STATUS "Documentation lint passed")
endif()