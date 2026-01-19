# Custom FindQt5LinguistTools.cmake for Debian/Ubuntu systems
# This file helps CMake find Qt5 linguist tools when the official Qt5LinguistTools package is not available

# Find lrelease executable
find_program(Qt5_LRELEASE_EXECUTABLE
    NAMES lrelease lrelease-qt5
    HINTS
    /usr/lib/x86_64-linux-gnu/qt5/bin
    /usr/lib/qt5/bin
    /usr/bin
    /usr/local/bin
)

# Find lupdate executable
find_program(Qt5_LUPDATE_EXECUTABLE
    NAMES lupdate lupdate-qt5
    HINTS
    /usr/lib/x86_64-linux-gnu/qt5/bin
    /usr/lib/qt5/bin
    /usr/bin
    /usr/local/bin
)

# Find linguist executable (optional)
find_program(Qt5_LINGUIST_EXECUTABLE
    NAMES linguist linguist-qt5
    HINTS
    /usr/lib/x86_64-linux-gnu/qt5/bin
    /usr/lib/qt5/bin
    /usr/bin
    /usr/local/bin
)

# Set package variables directly (no PARENT_SCOPE needed when variables are set directly)
if(Qt5_LRELEASE_EXECUTABLE AND Qt5_LUPDATE_EXECUTABLE)
    set(Qt5LinguistTools_FOUND TRUE)
    
    message(STATUS "Found Qt5 Linguist Tools:")
    message(STATUS "  lrelease: ${Qt5_LRELEASE_EXECUTABLE}")
    message(STATUS "  lupdate: ${Qt5_LUPDATE_EXECUTABLE}")
    if(Qt5_LINGUIST_EXECUTABLE)
        message(STATUS "  linguist: ${Qt5_LINGUIST_EXECUTABLE}")
    endif()
else()
    set(Qt5LinguistTools_FOUND FALSE)
    message(WARNING "Qt5 Linguist Tools not found - translations will be disabled")
endif()