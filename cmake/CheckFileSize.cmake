# CheckFileSize.cmake — warn on files > 3000 lines, error on > 5000
# Usage: cmake -DSOURCE_LIST=<file> -P CheckFileSize.cmake

cmake_policy(SET CMP0007 NEW)

if(NOT DEFINED SOURCE_LIST)
    message(FATAL_ERROR "SOURCE_LIST not defined")
endif()

file(STRINGS "${SOURCE_LIST}" sources)

set(failed FALSE)
set(warn_count 0)
set(error_count 0)

foreach(src ${sources})
    if(EXISTS "${src}")
        file(READ "${src}" content)
        string(REGEX MATCHALL "\n" newlines "${content}")
        list(LENGTH newlines n)
        if(n GREATER 5000)
            message("  ERROR: ${src}: ${n} lines (limit 5000)")
            set(failed TRUE)
            math(EXPR error_count "${error_count} + 1")
        elseif(n GREATER 3000)
            message("  WARN:  ${src}: ${n} lines (recommended max 3000)")
            math(EXPR warn_count "${warn_count} + 1")
        endif()
    endif()
endforeach()

if(error_count GREATER 0)
    message(STATUS "${error_count} file(s) exceed 5000-line limit, ${warn_count} file(s) exceed 3000-line recommendation")
    message(FATAL_ERROR "File size check failed")
elseif(warn_count GREATER 0)
    message(STATUS "${warn_count} file(s) exceed 3000-line recommendation (no errors)")
endif()
