//By Monica Moniot
#include "mchar.h"
#include "basic.h"


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


static void cursor_write_line(Cursor* cursor, LineStart* new_line) {
	//link cursor line to new_line
	Cursor* next_cursor = new_line->head_cursor;
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
	cursor_write_line(cursor, new_line);
}

void new_cursor(Pane* pane, Cursor* ret_cursor) {
	ret_cursor->line_i = 0;
	cursor_write_line(ret_cursor, pane->head_line);
	return ret_cursor;
}

TCursor to_tcursor_from_cursor(Cursor* cursor) {

}
void to_cursor_from_tcursor(TCursor cursor, Cursor* ret_cursor) {

}

void clone_cursor(Cursor* cursor, Cursor* ret_cursor) {
	ret_cursor->line_i = cursor->line_i;
	cursor_write_line(ret_cursor, cursor->line);
	return ret_cursor;
}


bool move_cursor_right(const Pane* pane, Cursor* cursor) {
	LineStart* line = cursor->line;
	LineStart* next_line = line->next;
	if(cursor->line_i + 1 < line->line_size) {
		cursor->line_i += 1;
		return 1;
	} else if(next_line != pane->head_line) {
		cursor->line_i = 0;
		cursor_set_line(cursor, next_line);
		return 1;
	} else {
		return 0;
	}
}
bool move_cursor_left (const Pane* pane, Cursor* cursor) {
	LineStart* line = cursor->line;
	if(cursor->line_i > 0) {
		cursor->line_i = min(cursor->line_i, line->line_size - 1) - 1;
		return 1;
	} else if(line != pane->head_line) {
		LineStart* pre = line->pre;
		cursor->line_i = pre->line_size - 1;
		cursor_set_line(cursor, pre);
		return 1;
	} else {
		return 0;
	}
}
bool move_cursor_down (const Pane* pane, Cursor* cursor) {
	LineStart* next = cursor->line->next;
	if(next != pane->head_line) {
		cursor_set_line(cursor, next);
		return 1;
	} else {
		return 0;
	}
}
bool move_cursor_up   (const Pane* pane, Cursor* cursor) {
	LineStart* line = cursor->line;
	if(line != pane->head_line) {
		cursor_set_line(cursor, line->pre);
		return 1;
	} else {
		return 0;
	}
}
void move_cursor_end  (Cursor* cursor) {
	LineStart* line = cursor->line;
	cursor->line_i = line->line_size - 1;
}
void move_cursor_start(Cursor* cursor) {
	cursor->line_i = 0;
}

static LineStart* push_text_right_(Pane* pane, TextBuffer* buffer, uint32 buffer_i, LineStart* line, uint32 line_i) {
	assert(buffer_i < buffer->text_size);
	//line must be the line that buffer_i is on (newlines are on the line before the one they start)
	LineStart* line0 = line;
	line = line->next;
	while(line->buffer == buffer && line != line0 && line != pane->head_line) {
		line->buffer_i += 1;
		line = line->next;
	}
	if(buffer->text_size < TEXT_CAPACITY) {
		buffer->text_size = buffer->text_size + 1;
		line = 0;
	}
	for(uint i = buffer->text_size - 1; i > buffer_i; i -= 1) {
		buffer->text[i] = buffer->text[i - 1];
	}
	mam_check(buffer, sizeof(TextBuffer));
	return line;
}

void insert_char_at(Pane* pane, Cursor* cursor, char ch) {
	LineStart* cur_line = cursor->line;
	if(!cur_line) return;//invalid cursor

	uint32 line_i = min(cursor->line_i, cur_line->line_size - 1);
	uint32 buffer_i;
	TextBuffer* cur_buffer = get_buffer_from_cursor(cursor, &buffer_i);

	mam_check(cur_buffer, sizeof(TextBuffer));
	//push_text into text list
	char overflow_ch = cur_buffer->text[TEXT_CAPACITY - 1];
	LineStart* oveflow_line = push_text_right_(pane, cur_buffer, buffer_i, cur_line, line_i);
	if(oveflow_line) {
		TextBuffer* next_buffer = cur_buffer->next;
		if((next_buffer->text_size < TEXT_CAPACITY) && (next_buffer != pane->head_text_buffer)) {
			//next has space and does not wrap to the beginning
			//push text in next
			push_text_right_(pane, next_buffer, 0, oveflow_line, 0);
			next_buffer->text[0] = overflow_ch;
		} else {
			//append new page, the next one is full, or the current one is at the end of the document
			TextBuffer* new_buffer = mam_pool_alloc(TextBuffer, pane->text_pool);
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
		LineStart* new_line = mam_pool_alloc(LineStart, pane->line_pool);
		LineStart* next = cur_line->next;
		cur_line->next = new_line;
		new_line->pre = cur_line;
		next->pre = new_line;
		new_line->next = next;
		new_line->line_size = cur_line->line_size - line_i;
		cur_line->line_size = line_i + 1;
		if(buffer_i + 1 < cur_buffer->text_size) {
			new_line->buffer_i = buffer_i + 1;
			new_line->buffer = cur_buffer;
		} else {
			new_line->buffer_i = 0;
			new_line->buffer = cur_buffer->next;
		}
		//relink cursor list to new_line
		Cursor* cur_cursor = cur_line->head_cursor;//NOTE: assumes that the cursor list isn't empty
		Cursor* last_cursor = cur_cursor->pre;
		while(cur_cursor != last_cursor) {
			Cursor* cur_cursor1 = cur_cursor->next;
			if(cur_cursor->line_i >= line_i) {
				//NOTE: will move the insert cursor to new_line
				cursor_set_line(cur_cursor, new_line);
				cur_cursor->line_i -= line_i;
			}
			cur_cursor = cur_cursor1;
		}
		if(cur_cursor->line_i >= line_i) {
			//NOTE: will move the insert cursor to new_line
			cursor_set_line(cur_cursor, new_line);
			cur_cursor->line_i -= line_i;
		}
	} else {
		cur_line->line_size += 1;
		//move cursors on this line right
		Cursor* head_cursor = cur_line->head_cursor;
		Cursor* cur_cursor = head_cursor;//NOTE: assumes that the cursor list isn't empty
		do {
			if(cur_cursor->line_i >= line_i) {
				//NOTE: will move the insert cursor right
				cur_cursor->line_i += 1;
			}
			cur_cursor = cur_cursor->next;
		} while(cur_cursor != head_cursor);
	}
	mam_check(cur_buffer, sizeof(TextBuffer));
}

static bool push_text_left_(Pane* pane, TextBuffer* buffer, uint32 buffer_i, LineStart* line, uint32 line_i) {
	mam_check(buffer, sizeof(TextBuffer));
	assert(buffer_i < buffer->text_size);
	assert(buffer->text_size > 0);
	//line must be the line that buffer_i is on
	//returns if the push would empty the buffer
	mam_check(buffer, sizeof(TextBuffer));
	LineStart* line0 = line;
	line = line->next;
	while(line->buffer == buffer && line != line0) {
		//NOTE: some cursors auto update line_i
		line->buffer_i -= 1;
		line = line->next;
	}
	for(uint i = buffer_i + 1; i < buffer->text_size; i += 1) {
		buffer->text[i - 1] = buffer->text[i];
	}
	buffer->text_size -= 1;
	mam_check(buffer, sizeof(TextBuffer));
	if(buffer->text_size == 0) {
		return 1;
	} else {
		return 0;
	}
}

void delete_char_at(Pane* pane, Cursor* cursor) {
	LineStart* cur_line = cursor->line;
	if(!cur_line) return;//invalid cursor
	//NOTE: make sure the absolute last character is not deleted
	//also do nothing if attempting to delete the first line
	if(cur_line == pane->head_line && cursor->line_i == 0) return;

	uint32 line_i = min(cursor->line_i, cur_line->line_size - 1);

	Cursor* head_cursor = cur_line->head_cursor;
	if(line_i == 0) {
		//remove cur_line
		LineStart* pre_line = cur_line->pre;
		LineStart* next_line = cur_line->next;
		pre_line->next = next_line;
		next_line->pre = pre_line;
		pre_line->line_size -= 1;
		//relink cur_line's cursor list to pre_line's
		do {
			Cursor* cur_cursor = cur_line->head_cursor;//NOTE: assumes that the cursor list isn't empty
			//NOTE: will move the delete cursor to pre_line
			cursor_set_line(cur_cursor, pre_line);
			cur_cursor->line_i += pre_line->line_size;
		} while(cur_line->head_cursor);
		pre_line->line_size += cur_line->line_size;
		mam_pool_free(pane->line_pool, cur_line);
	} else {
		cur_line->line_size -= 1;
		//move cursors on this line left
		Cursor* head_cursor = cur_line->head_cursor;
		Cursor* cur_cursor = head_cursor;//NOTE: assumes that the cursor list isn't empty
		do {
			if(cur_cursor->line_i >= line_i) {
				//NOTE: will move the delete cursor left
				cur_cursor->line_i -= 1;
			}
			cur_cursor = cur_cursor->next;
		} while(cur_cursor != head_cursor);
	}

	uint32 buffer_i;
	TextBuffer* cur_buffer = get_buffer_from_cursor(cursor, &buffer_i);
	mam_check(cur_buffer, sizeof(TextBuffer));

	bool is_empty = push_text_left_(pane, cur_buffer, buffer_i, cur_line, line_i);
	mam_check(cur_buffer, sizeof(TextBuffer));
	if(is_empty) {
		//remove cur_buffer from text list
		TextBuffer* next_buffer = cur_buffer->next;
		TextBuffer* pre_buffer = cur_buffer->pre;
		next_buffer->pre = pre_buffer;
		pre_buffer->next = next_buffer;
		if(pane->head_text_buffer == cur_buffer) {
			//NOTE: It is not possible to delete the last character out of the last buffer
			pane->head_text_buffer = next_buffer;
		}
		mam_pool_free(pane->text_pool, cur_buffer);
	}
}

uint32 get_size_of_text_between(Pane* pane, Cursor* cursor0, Cursor* cursor1) {
	uint32 end_buffer_i;
	TextBuffer* end_buffer = get_buffer_from_cursor(cursor1, &end_buffer_i);

	uint32 buffer_i;
	TextBuffer* cur_buffer = get_buffer_from_cursor(cursor0, &buffer_i);

	uint32 raw_text_size = 0;

	if(cur_buffer != end_buffer) {
		raw_text_size += cur_buffer->text_size;
		cur_buffer = cur_buffer->next;
		while(cur_buffer != end_buffer) {
			raw_text_size += cur_buffer->text_size;
			cur_buffer = cur_buffer->next;
		}
		buffer_i = 0;
	}
	raw_text_size += end_buffer_i - buffer_i;
	return raw_text_size + 1;
}
void get_text_between(Pane* pane, Cursor* cursor0, Cursor* cursor1, char* ret_raw_text) {
	uint32 end_buffer_i;
	TextBuffer* end_buffer = get_buffer_from_cursor(cursor1, &end_buffer_i);

	uint32 buffer_i;
	TextBuffer* cur_buffer = get_buffer_from_cursor(cursor0, &buffer_i);

	char* cur_text = ret_raw_text;

	if(cur_buffer != end_buffer) {
		memcpy(cur_text, &cur_buffer->text[buffer_i], cur_buffer->text_size);
		cur_text += cur_buffer->text_size;
		cur_buffer = cur_buffer->next;
		while(cur_buffer != end_buffer) {
			memcpy(cur_text, &cur_buffer->text, cur_buffer->text_size);
			cur_text += cur_buffer->text_size;
			cur_buffer = cur_buffer->next;
		}
		buffer_i = 0;
	}
	memcpy(cur_text, &cur_buffer->text, end_buffer_i - buffer_i);
	//add a null terminator for the user
	cur_text += end_buffer_i - buffer_i;
	*cur_text = 0;
}


int main() {
	Pane pane = {0};
	pane.text_pool = mam_pool_init(TextBuffer, malloc(MEGABYTE), MEGABYTE);
	pane.line_pool = mam_pool_init(LineStart, malloc(MEGABYTE), MEGABYTE);
	mam_pool_freei(pane.text_pool, mam_pool_alloci(pane.text_pool));

	pane.head_text_buffer = mam_pool_alloc(TextBuffer, pane.text_pool);
	pane.head_text_buffer->next = pane.head_text_buffer;
	pane.head_text_buffer->pre = pane.head_text_buffer;
	pane.head_text_buffer->text_size = 1;
	memzero(pane.head_text_buffer->text, sizeof(pane.head_text_buffer->text));

	pane.head_line = mam_pool_alloc(LineStart, pane.line_pool);
	pane.head_line->next = pane.head_line;
	pane.head_line->pre = pane.head_line;
	pane.head_line->head_cursor = 0;
	pane.head_line->line_size = 1;
	pane.head_line->buffer_i = 0;
	pane.head_line->buffer = pane.head_text_buffer;

	Cursor cursor;
	Cursor cursor0;
	new_cursor(&pane, &cursor);
	new_cursor(&pane, &cursor0);
	// Cursor cursor0 = new_cursor(&pane);
	// insert_char_at(&pane, &cursor, 'A');

	insert_char_at(&pane, &cursor, '\n');
	for_each_lt(i, 3*26) {
		insert_char_at(&pane, &cursor, 'A' + (i%26));
	}
	insert_char_at(&pane, &cursor, '\n');
	// insert_char_at(&pane, &cursor, '\n');
	for_each_lt(i, 2*26) {
		delete_char_at(&pane, &cursor);
	}
	// insert_char_at(&pane, &cursor, '\n');
	// insert_char_at(&pane, &cursor, 'C');
	// insert_char_at(&pane, &cursor, 'C');
	return 0;
}
