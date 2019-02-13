//By Monica Moniot
#include "basic.h"
#include "types.h"


void init(State* state, Output* output) {
	memzero(state, sizeof(State));

	byte* text_tape = 0;
	tape_reserve(text_tape, GIGABYTE/2);
	state->text_pool = cast(Pool*, text_tape);
	pool_init(state->text_pool, GIGABYTE/2);
}

static TextBuffer* get_buffer_from_cursor(Cursor* cursor, uint32* ret_buffer_i) {
	LineStart* cur_line = cursor->line;
	TextBuffer* buffer = cur_line->buffer;
	uint32 buffer_i = min(cursor->line_i, cur_line->line_size - 1) + cur_line->buffer_i;
	while(buffer_i >= buffer->text_size) {
		buffer_i -= buffer->text_size;
		buffer = buffer->next;
	}
	*ret_buffer_i = buffer_i;
	return buffer;
}

static LineStart* push_text_right_(TextBuffer* buffer, uint32 buffer_i, LineStart* line, uint32 line_i) {
	assert(buffer_i < buffer->text_size);
	//line must be the line that buffer_i is on (newlines are on the line before the one they start)
	Cursor* head_cursor = line->head_cursor;
	Cursor* cursor = head_cursor->next;
	while(cursor != head_cursor) {
		assert(cursor->line = line);
		//NOTE: does nothing if only one cursor in list
		if(cursor->line_i > line_i) {
			//NOTE: will miss the insert cursor
			cursor->line_i += 1;
		}
	}
	line = line->next;
	while(line->buffer == buffer) {
		line->buffer_i += 1;
		line = line->next;
	}
	if(buffer->text_size < TEXT_CAPACITY) {
		buffer->text_size = buffer->text_size + 1;
		line = 0;
	}
	for(uint i = buffer->text_size; i > buffer_i, i -= 1) {
		buffer->text[i] = buffer->text[i - 1];
	}
	return line;
}

static void insert_char_at(Pane* pane, Cursor* cursor, char ch) {
	LineStart* cur_line = cursor->line;
	if(!cur_line) return;//invalid cursor

	uint32 line_i = min(cursor->line_i, cur_line->line_size - 1);
	uint32 buffer_i;
	TextBuffer* cur_buffer = get_buffer_from_cursor(cursor, &buffer_i);

	//push_text into text list
	char overflow_ch = cur_buffer->text[TEXT_CAPACITY - 1];
	LineStart* oveflow_line = push_text_right_(cur_buffer, buffer_i, cur_line);
	if(oveflow_line) {
		TextBuffer* next_buffer = cur_buffer->next;
		if((next_buffer->text_size < TEXT_CAPACITY) && (next_buffer != pane->head_text_buffer)) {
			//next has space and does not wrap to the beginning
			//push text in next
			push_text_right_(next_buffer, 0, oveflow_line);
			next_buffer->text[0] = overflow_ch;
		} else {
			//append new page, the next one is full, or the current one is at the end of the document
			TextBuffer* new_buffer = pool_reserve(TextBuffer, pane->text_pool);
			cur_buffer->next = new_buffer;
			next_buffer->pre = new_buffer;
			new_buffer->next = next_buffer;
			new_buffer->pre = cur_buffer;
			new_buffer->text_size = 1;
			new_buffer->text[0] = overflow_ch;
		}
	}
	cur_buffer->text[buffer_i] = ch;
	//correct line list
	if(ch == '\n') {
		//add new_line to the line list
		//NOTE: cursors auto update line
		LineStart* new_line;//TODO: Allocate
		LineStart* next = cur_line->next;
		next->pre = new_line;
		cur_line->next = new_line;
		new_line->next = next;
		new_line->pre = pre;
		new_line->line_size = cur_line->line_size - line_i;
		cur_line->line_size = line_i + 1;
		if(buffer_i + 1 < cur_buffer->text_size) {
			new_line->buffer_i = buffer_i + 1;
			new_line->buffer = cur_buffer;
		} else {
			new_line->buffer_i = 0;
			new_line->buffer = cur_buffer->next;
		}
	} else {
		cur_line->line_size += 1;
	}
}

static bool push_text_left_(TextBuffer* buffer, uint32 buffer_i, LineStart* line, uint32 line_i) {
	assert(buffer_i < buffer->text_size);
	assert(buffer->text_size > 0);
	//line must be the line that buffer_i is on
	//returns if the push would empty the buffer
	Cursor* head_cursor = line->head_cursor;
	if(buffer->text[buffer_i] == '\n') {
		assert(line_i == line->line_size - 1);
		//remove cur_line
		LineStart* pre_line = line;
		LineStart* cur_line = line->next;
		LineStart* next_line = cur_line->next;
		pre_line->next = next_line;
		next_line->pre = pre_line;
		pre_line->line_size += cur_line->line_size;
		//relink cur_line's cursor list to pre_line's
		Cursor* next_cursor0 = cur_line->head_cursor;
		if(next_cursor0) {
			if(next_cursor1) {
				Cursor* next_cursor1 = pre_line->head_cursor;
				Cursor* pre_cursor1 = next_cursor1->pre;
				Cursor* pre_cursor0 = next_cursor0->pre;
				pre_cursor0->next = next_cursor1;
				next_cursor1->pre = pre_cursor0;
				next_cursor0->pre = pre_cursor1;
				pre_cursor1->next = next_cursor0;
			} else {
				pre_line->head_cursor = next_cursor1;
			}
		}
		//TODO: free cur_line here
	} else {
		Cursor* head_cursor = line->head_cursor;
		Cursor* cursor = head_cursor->next;
		while(cursor != head_cursor) {
			assert(cursor->line == line);
			//NOTE: does nothing if only one cursor in list
			if(cursor->line_i > line_i) {
				//NOTE: will miss the insert cursor
				cursor->line_i -= 1;
			}
		}
	}
	line = line->next;
	while(line->buffer == buffer) {
		//NOTE: some cursors auto update line_i
		line->buffer_i -= 1;
		line = line->next;
	}
	for(uint i = buffer_i; i + 1 < buffer->text_size, i += 1) {
		buffer->text[i] = buffer->text[i + 1];
	}
	buffer->text_size -= 1;
	if(buffer->text_size == 0) {
		return 1;
	} else {
		return 0;
	}
}
static void delete_char_at(Pane* pane, Cursor* cursor) {
	LineStart* cur_line = cursor->line;
	if(!cur_line) return;//invalid cursor

	uint32 line_i = min(cursor->line_i, cur_line->line_size - 1);
	uint32 buffer_i;
	TextBuffer* cur_buffer = get_buffer_from_cursor(cursor, &buffer_i);

	cur_line->line_size -= 1;
	bool is_empty = push_text_left_(cur_buffer, buffer_i, cur_line);
	if(is_empty) {
		//remove cur_buffer from text list
		TextBuffer* next_buffer = cur_buffer->next;
		TextBuffer* pre_buffer = cur_buffer->pre;
		next_buffer->pre = pre_buffer;
		pre_buffer->next = next_buffer;
		//TODO: free cur_buffer here
	}
}

static void cursor_set_line(Cursor* cursor, LineStart* new_line) {
	//relink cursor from line to new_line
	LineStart* line = cursor->line;

	Cursor* next_cursor = cursor->next;
	if(cursor == next_cursor) {
		line->head_cursor = 0;
	} else {
		Cursor* pre_cursor = cursor->pre;
		pre_cursor->next = next_cursor;
		next_cursor->pre = pre_cursor;
		if(line->head_cursor == cursor) {
			line->head_cursor = next_cursor;
		}
	}
	next_cursor = new_line->head_cursor;
	if(next_cursor) {//could optimize case by case
		Cursor* pre_cursor = next_cursor->pre;
		next_cursor->pre = cursor;
		cursor->next = next_cursor;
		pre_cursor->next = cursor;
		cursor->pre = pre_cursor;
	} else {
		new_line->head_cursor = cursor;
		cursor->next = cursor;
		cursor->pre = cursor;
	}
	cursor->line = new_line;
}

static void move_cursor_right(Cursor* cursor, LineStart* head_line) {
	LineStart* line = cursor->line;
	LineStart* next_line = line->next;
	if(cursor->line_i + 1 < line->line_size) {
		cursor->line_i += 1;
	} else if(next_line != head_line) {
		cursor->line_i = 0;
		cursor_set_line(cursor, next_line);
	}
}
static void move_cursor_left (Cursor* cursor, LineStart* head_line) {
	LineStart* line = cursor->line;
	if(cursor->line_i - 1 >= 0) {
		cursor->line_i -= 1;
	} else if(line != head_line) {
		LineStart pre = line->pre;
		cursor->line_i = pre->line_size - 1;
		cursor_set_line(cursor, pre);
	}
}
static void move_cursor_down (Cursor* cursor, LineStart* head_line) {
	LineStart next = cursor->line->next;
	if(next != head_line) {
		cursor_set_line(cursor, next);
	}
}
static void move_cursor_up   (Cursor* cursor, LineStart* head_line) {
	LineStart* line = cursor->line;
	if(line != head_line) {
		cursor_set_line(cursor, line->pre);
	}
}


static void undo_record_delete(Pane* pane, Cursor* cursor) {
	UndoData* undo;//--<--
	
}


void update(State* state, KeyCode* keys, uint32 keys_size, Output* output) {
	UserData user = &state->user;
	for_each_in(key_event, keys, keys_size) {
		uint32 key;
		bool is_down = key_event > 0;
		if(is_down) {
			key = key_event;
			if(key >= 'a' && key <= 'z') {
				if(user->is_down_shift) {
					key += 'A' - 'a';
				}
				insert_char_at_cursor(state, user->cursor, cast(char, key));
			}
		} else {
			key = -key_event;
		}
	}
}
