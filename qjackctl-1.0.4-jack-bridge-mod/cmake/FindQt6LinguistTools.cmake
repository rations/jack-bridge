# Custom FindQt6LinguistTools.cmake for Debian/Ubuntu systems
# This file helps CMake find Qt6 linguist tools when the official Qt6LinguistTools package is not available

# Find lrelease executable
find_program(Qt6_LRELEASE_EXECUTABLE
    NAMES lrelease lrelease-qt6
    HINTS
    /usr/lib/x86_64-linux-gnu/qt6/bin
    /usr/lib/qt6/bin
    /usr/bin
    /usr/local/bin
)

# Find lupdate executable
find_program(Qt6_LUPDATE_EXECUTABLE
    NAMES lupdate lupdate-qt6
    HINTS
    /usr/lib/x86_64-linux-gnu/qt6/bin
    /usr/lib/qt6/bin
    /usr/bin
    /usr/local/bin
)

# Find linguist executable (optional)
find_program(Qt6_LINGUIST_EXECUTABLE
    NAMES linguist linguist-qt6
    HINTS
    /usr/lib/x86_64-linux-gnu/qt6/bin
    /usr/lib/qt6/bin
    /usr/bin
    /usr/local/bin
)

# Set package variables directly (no PARENT_SCOPE needed when variables are set directly)
if(Qt6_LRELEASE_EXECUTABLE AND Qt6_LUPDATE_EXECUTABLE)
    set(Qt6LinguistTools_FOUND TRUE)

    message(STATUS "Found Qt6 Linguist Tools:")
    message(STATUS "  lrelease: ${Qt6_LRELEASE_EXECUTABLE}")
    message(STATUS "  lupdate: ${Qt6_LUPDATE_EXECUTABLE}")
    if(Qt6_LINGUIST_EXECUTABLE)
        message(STATUS "  linguist: ${Qt6_LINGUIST_EXECUTABLE}")
    endif()
else()
    set(Qt6LinguistTools_FOUND FALSE)
    message(WARNING "Qt6 Linguist Tools not found - translations will be disabled")
endif()