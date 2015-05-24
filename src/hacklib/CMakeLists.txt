SET(PROJ_NAME hacklib)
PROJECT(${PROJ_NAME})

SET(FILES_CPP
    src/Main.cpp
    src/Hooker.cpp
    src/Patch.cpp
    )
SET(FILES_H
    include/hacklib/MessageBox.h
    include/hacklib/Main.h
    include/hacklib/Injector.h
    include/hacklib/Hooker.h
    include/hacklib/ForeignClass.h
    include/hacklib/PatternScanner.h
    include/hacklib/ImplementMember.h
    include/hacklib/Rng.h
    include/hacklib/Timer.h
    include/hacklib/PageAllocator.h
    include/hacklib/Patch.h
    )

# windows specific
IF(WIN32)
    SET(FILES_CPP ${FILES_CPP}
        src/MessageBox.cpp
        src/D3DDeviceFetcher.cpp
        src/Drawer.cpp
        src/GfxOverlay.cpp
        src/WindowOverlay.cpp
        src/PatternScanner.cpp  # TODO this is not really windows specific
        src/ConsoleEx.cpp
        src/PageAllocator_WIN32.cpp
        src/Hooker_VEH.cpp
        src/Injector.cpp
        src/Input.cpp
        )
    SET(FILES_H ${FILES_H}
        include/hacklib/D3DDeviceFetcher.h
        include/hacklib/Drawer.h
        include/hacklib/GfxOverlay.h
        include/hacklib/WindowOverlay.h
        include/hacklib/ConsoleEx.h
        include/hacklib/Input.h
        )
ELSEIF(UNIX)
    SET(FILES_CPP ${FILES_CPP}
        src/PageAllocator_UNIX.cpp
        )
ENDIF()

ADD_LIBRARY(${PROJ_NAME} STATIC
    ${FILES_CPP}
    ${FILES_H})

SET_TARGET_PROPERTIES(${PROJ_NAME} PROPERTIES FOLDER ${PROJ_NAME})

TARGET_INCLUDE_DIRECTORIES(${PROJ_NAME} PUBLIC include)

IF(WIN32)
    TARGET_LINK_LIBRARIES(${PROJ_NAME} d3d9.lib d3dx9.lib d3d11.lib dwmapi.lib)
ENDIF()