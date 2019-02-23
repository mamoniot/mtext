//By Monica Moniot
#include "basic.h"
#define MAM_ALLOC_IMPLEMENTATION
#define MAM_ALLOC_DEBUG
#include "mam_alloc.h"


//negative keycodes represent a key being let up
//repeat keys always come between a key being pushed down event and being let up
// typedef int32 KeyCode;


// typedef struct {
// 	char* i;
// } Output;


#ifndef __cplusplus
typedef int8 bool;
#endif
//NOTE: All of the linked lists here are circular
#define TEXT_CAPACITY 64
typedef struct TextBuffer {
	struct TextBuffer* next;
	struct TextBuffer* pre;
	uint32 text_size;
	char text[TEXT_CAPACITY];
} TextBuffer;

typedef struct LineStart {
	struct LineStart* next;
	struct LineStart* pre;
	struct Cursor* head_cursor;
	uint32 line_size;//NOTE: does include the newline
	uint32 buffer_i;
	TextBuffer* buffer;
} LineStart;

typedef struct TCursor {
	LineStart* line;
	uint32 line_i;//NOTE: can be any value, regardless of the current line's size. min(line_i, line->line_size - 1)
} TCursor;

typedef union Cursor {
	TCursor t;
	struct {
		LineStart* line;
		uint32 line_i;
		struct Cursor* next;
		struct Cursor* pre;
	}
} Cursor;

typedef struct {
	LineStart* head_line;
	TextBuffer* head_text_buffer;
	MamPool* text_pool;
	MamPool* line_pool;
} Pane;

// typedef union {
// 	relptr next;
// 	relptr pre;
// } UndoItem;
// typedef struct {
// 	MamStack* branch[2];
// } UndoData;
//NOTE: must always contain at least one character every line
