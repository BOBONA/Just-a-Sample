if(NOT DEFINED REPO_DIR OR NOT DEFINED PATCH_FILE OR NOT DEFINED NAME)
    message(FATAL_ERROR "Missing variables: REPO_DIR PATCH_FILE NAME required")
endif()

execute_process(
        COMMAND git apply --reverse --check --ignore-whitespace -p0 "${PATCH_FILE}"
        WORKING_DIRECTORY "${REPO_DIR}"
        RESULT_VARIABLE reverse_result
        OUTPUT_QUIET ERROR_QUIET
)

if(reverse_result EQUAL 0)
    message(STATUS "${NAME}: patch already applied")
    return()
endif()

execute_process(
        COMMAND git apply --check --ignore-whitespace -p0 "${PATCH_FILE}"
        WORKING_DIRECTORY "${REPO_DIR}"
        RESULT_VARIABLE forward_result
        OUTPUT_QUIET ERROR_QUIET
)

if(forward_result EQUAL 0)
    message(STATUS "${NAME}: applying patch")
    execute_process(
            COMMAND git apply --ignore-whitespace -p0 "${PATCH_FILE}"
            WORKING_DIRECTORY "${REPO_DIR}"
            RESULT_VARIABLE apply_result
    )

    if(NOT apply_result EQUAL 0)
        message(FATAL_ERROR "${NAME}: patch failed while applying")
    endif()
else()
    message(FATAL_ERROR "${NAME}: patch cannot be applied (source differs)")
endif()