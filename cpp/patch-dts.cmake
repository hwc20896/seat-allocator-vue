if(NOT DEFINED DTS_FILE)
    message(FATAL_ERROR "DTS_FILE is required")
endif()

file(READ "${DTS_FILE}" _content)

string(REGEX REPLACE
    "setAnnealingConfigDynamic\\([^)]*\\): void;"
    "setAnnealingConfigDynamic(func: (gridSize: number) => AnnealingConfig): void;"
    _content "${_content}")

if(NOT _content MATCHES "setAnnealingConfigDynamic\\(func: \\(gridSize: number\\) => AnnealingConfig\\)")
    message(WARNING "alloc_algo.d.ts: setAnnealingConfigDynamic not patched; tsgen output may have changed")
endif()

file(WRITE "${DTS_FILE}" "${_content}")