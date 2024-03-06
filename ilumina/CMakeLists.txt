add_library(ilumina_lib STATIC types.h types.h debug.h ilumina.h types.cpp ilumina.cpp board.cpp board.h)
set_target_properties(ilumina_lib PROPERTIES OUTPUT_NAME ilumina)