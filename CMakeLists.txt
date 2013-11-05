project(te-exporter)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

include_directories(SYSTEM ${QT_INCLUDES})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wswitch-default -Wswitch-enum -Wuninitialized -Winit-self -Wfloat-equal -Wwrite-strings -Wcast-qual -Wconversion -Waddress -Wlogical-op -Wno-sign-compare -Wzero-as-null-pointer-constant")

set(te-exporter-sources
	src/FlashcardsDatabase.cpp
	src/DatabaseExporter.cpp
	src/Util.cpp
	src/SceneryExecutor.cpp
	src/VariableStack.cpp
	src/main.cpp
	src/FlashcardUtilities.cpp
	src/FlashcardsDatabaseParser.cpp
	src/HistoricalFlashcards.cpp
	src/QuestionFlashcard.cpp
    src/WordSpellingFlashcard.cpp)

set(te-exporter-headers
	src/DatabaseExporter.h
	src/FlashcardsDatabase.h
	src/Util.h
	src/SceneryExecutor.h
	src/VariableStack.h
	src/FlashcardUtilities.h
	src/FlashcardsDatabaseParser.h
	src/HistoricalFlashcards.h
	src/QuestionFlashcard.h
	src/WordSpellingFlashcard.h)

qt4_wrap_cpp(moc-outfiles ${te-exporter-headers})

add_executable(te-exporter ${te-exporter-sources} ${moc-outfiles})

target_link_libraries(te-exporter ${QT_QTCORE_LIBRARY})
