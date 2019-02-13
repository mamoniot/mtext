//By Monica Moniot
#include "basic.h"


//negative keycodes represent a key being let up
//repeat keys always come between a key being pushed down event and being let up
typedef int32 KeyCode;


typedef struct {

} Output;


#ifndef __cplusplus
typedef int8 bool;
#endif

#define TEXT_CAPACITY (256 - 2*sizeof(TextPage*) - sizeof(BufferPos))
typedef struct {
	TextBuffer* next;
	TextBuffer* pre;
	BufferPos text_size;
	char text[TEXT_CAPACITY];
} TextBuffer;
typedef struct {
	LineStart* next;
	LineStart* pre;
	Cursor* head_cursor;
	uint32 line_size;//NOTE: does include the newline
	uint32 buffer_i;
	TextBuffer* buffer;
} LineStart;

typedef struct {
	LineStart* line;
	uint32 line_i;//NOTE: can be any value, regardless of the current line's size. min(line_i, line->line_size - 1)
	Cursor* next;
	Cursor* pre;
} Cursor;


typedef struct {
	Cursor cursor;
	bool is_down_shift;
} UserData;

typedef union {
	relptr next;
	relptr pre;
} UndoItem;
typedef struct {
	Stack* branch[2];
} UndoData;

typedef struct {
	LineStart* head_line;
	TextBuffer* head_text_buffer;
} Pane;

typedef struct {
	Pool* text_pool;
	TextPage* head_text_page;
} State;
