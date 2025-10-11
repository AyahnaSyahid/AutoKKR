add_library(counterObject OBJECT
            countergenerator.cpp
            ${CMAKE_SOURCE_DIR}/resources/dev-eruces.qrc )

target_link_libraries(counterObject PUBLIC UsesOpenSSL Qt::Core)

add_executable(CounterGenerator
                    $<TARGET_OBJECTS:counterObject>
                    countergenerator_main.cpp
                    countergenwindow.h
                    countergenwindow.cpp
                    requestparser.h
                    requestparser.cpp
                    ui/requestparser.ui
                    ui/countergenwindow.ui )
                    
target_link_libraries(CounterGenerator PUBLIC 
                        UsesQt6 UsesOpenSSL )

add_executable(Test_CounterGenerator countergenerator_test.cpp)
target_link_libraries(Test_CounterGenerator PUBLIC UsesQt6 counterObject)

set(CNTROBJ <TARGET_OBJECTS:counterObject> PARENT_SCOPE)