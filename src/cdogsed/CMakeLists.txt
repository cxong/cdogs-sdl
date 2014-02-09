set(CDOGSED_SOURCES
	charsed.c
	editor_brush.c
	editor_ui.c
	editor_ui_common.c
	editor_ui_static.c
	ui_object.c)
set(CDOGSED_HEADERS
	charsed.h
	editor_brush.h
	editor_ui.h
	editor_ui_common.h
	editor_ui_static.h
	ui_object.h)
add_library(cdogsedlib STATIC ${CDOGSED_SOURCES} ${CDOGSED_HEADERS})
