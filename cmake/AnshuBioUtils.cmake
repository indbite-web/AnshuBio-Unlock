# AnshuBio Unlock CMake Utilities & Compiler Hardening
# Publisher: AnshuCore

function(anshubio_apply_security_flags target)
    target_compile_definitions(${target} PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )

    if (MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /utf-8
            /GS
            /guard:cf
            /EHsc
        )
        target_link_options(${target} PRIVATE
            /GUARD:CF
            /DYNAMICBASE
            /NXCOMPAT
            /HIGHENTROPYVA
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -fstack-protector-strong
        )
        target_link_options(${target} PRIVATE
            -Wl,--dynamicbase
            -Wl,--nxcompat
            -Wl,--high-entropy-va
        )
    endif()
endfunction()
