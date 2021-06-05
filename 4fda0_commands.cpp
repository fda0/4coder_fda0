
// TODO(f0): Comamnd_mode: M - add/remove enough spaces to align with mark position



#if 0
inline String_Const_u8
f0_string_advance_str(String_Const_u8 input, u64 distance)
{
    distance = clamp_top(distance, input.size);
    String_Const_u8 result = {};
    result.str = input.str + distance;
    result.size = input.size - distance;
    return result;
}
#endif


function void
f0_move_past_lead_whitespace_like_virtual(Application_Links *app, View_ID view, Buffer_ID buffer)
{
    i64 pos = view_get_cursor_pos(app, view);
    i64 new_pos = get_pos_past_lead_whitespace(app, buffer, pos);
    view_set_cursor(app, view, seek_pos(new_pos));
}

CUSTOM_COMMAND_SIG(f0_spaces_align)
CUSTOM_DOC("Align to spaces at cursor to mark column.")
{
    Scratch_Block scratch(app);
    
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    
    i64 cursor_abs_pos = view_get_cursor_pos(app, view);
    i64 mark_abs_pos = view_get_mark_pos(app, view);
    
    i64 cursor_line = get_line_number_from_pos(app, buffer, cursor_abs_pos);
    i64 mark_line = get_line_number_from_pos(app, buffer, mark_abs_pos);
    
    Range_i64 mark_line_range = get_line_pos_range(app, buffer, mark_line);
    Range_i64 cursor_line_range = get_line_pos_range(app, buffer, cursor_line);
    
    i64 mark_col = mark_abs_pos - mark_line_range.min;
    i64 cursor_col = cursor_abs_pos - cursor_line_range.min;
    
    String_Const_u8 cur_line_text = push_buffer_range(app, scratch, buffer, cursor_line_range);
    Range_i64 spaces_range = {cursor_abs_pos, cursor_abs_pos};
    
    
    // expand selection forward
    for (u64 i = cursor_col; i < cur_line_text.size; ++i)
    {
        u8 t = cur_line_text.str[i];
        if (t == ' ') spaces_range.max += 1;
        else break;
    }
    
    // expand selection backwards
    for (i64 i = cursor_col-1; i >= 0; --i)
    {
        u8 t = cur_line_text.str[i];
        if (t == ' ') spaces_range.min -= 1;
        else break;
    }
    
    
    i64 direction = mark_col - (spaces_range.max - cursor_line_range.min);
    if (direction > 0)
    {
        String_Const_u8 new_spaces = string_const_u8_push(scratch, direction);
        for (u64 i = 0; i < new_spaces.size; ++i)
        {
            new_spaces.str[i] = ' ';
        }
        
        Range_i64 cursor_range = {cursor_abs_pos, cursor_abs_pos};
        buffer_replace_range(app, buffer, cursor_range, new_spaces);
    }
    else if (direction < 0)
    {
        i64 spaces_len = spaces_range.max - spaces_range.min;
        i64 target_spaces_len = spaces_len + direction;
        
        b32 align_to_line_start = spaces_range.min > cursor_line_range.min;
        if (target_spaces_len < 1 && align_to_line_start)
        {
            target_spaces_len = 1;
        }
        else if (target_spaces_len < 0)
        {
            target_spaces_len = 0;
        }
        
        String_Const_u8 new_spaces = string_const_u8_push(scratch, target_spaces_len);
        for (u64 i = 0; i < new_spaces.size; ++i)
        {
            new_spaces.str[i] = ' ';
        }
        
        buffer_replace_range(app, buffer, spaces_range, new_spaces);
        
        i64 spaces_start_col = (spaces_range.min - cursor_line_range.min);
        i64 mark_or_start = clamp_bot(spaces_start_col, mark_col);
        i64 target_col = clamp_top(cursor_col, mark_or_start);
        
        i64 move = target_col - spaces_start_col;
        view_set_cursor_by_character_delta(app, view, move);
    }
}



CUSTOM_COMMAND_SIG(f0_banner_wrap_range)
CUSTOM_DOC("Adds symetric banner comment === formatting === to range. Works for single lines only.")
{
    u8 symbol = '=';
    i64 symbol_length = 64;
    Scratch_Block scratch(app);
    
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 mark_pos = view_get_mark_pos(app, view);
    i64 cursor_line = get_line_number_from_pos(app, buffer, cursor_pos);
    i64 mark_line = get_line_number_from_pos(app, buffer, mark_pos);
    
    if (cursor_line == mark_line)
    {
        Range_i64 range = {};
        if (cursor_pos > mark_pos)
        {
            range.min = mark_pos;
            range.max = cursor_pos;
        }
        else
        {
            range.min = cursor_pos;
            range.max = mark_pos;
        }
        Range_i64 original_range = range;
        
        
        String_Const_u8 full_text = push_buffer_range(app, scratch, buffer, range);
        b32 found_text = false;
        u64 skipped_from_left = 0;
        
        for (u64 i = 0; i < full_text.size; ++i)
        {
            u8 u = full_text.str[i];
            if (!(character_is_whitespace(u) || u == symbol))
            {
                range.min += i;
                skipped_from_left = i;
                found_text = true;
                break;
            }
        }
        
        if (found_text)
        {
            for (u64 i = 0; i < full_text.size; i++)
            {
                u8 u = full_text.str[full_text.size - 1 - i];
                if (!(character_is_whitespace(u) || u == symbol))
                {
                    range.max -= i;
                    break;
                }
            }
            
            
            Range_i64 line_range = get_line_pos_range(app, buffer, cursor_line);
            
            i64 len = (symbol_length - 2 - (range.min - line_range.min) -
                       (range.max - range.min) + (range.min - original_range.min));
            
            i64 symbols_per_side = len / 2;
            i64 padding_len = len % 2;
            
            if (symbols_per_side > 0)
            {
                String_Const_u8 text = push_buffer_range(app, scratch, buffer, range);
                String_Const_u8 symbols_l = string_const_u8_push(scratch, symbols_per_side);
                for (u64 i = 0; i < symbols_l.size; ++i)
                {
                    symbols_l.str[i] = symbol;
                }
                String_Const_u8 symbols_r = symbols_l;
                symbols_r.size -= 1;
                
                String_Const_u8 padding = symbols_l;
                padding.size = padding_len;
                
                String_Const_u8 final_text = push_stringf(scratch, "%.*s %.*s %.*s%.*s",
                                                          string_expand(symbols_l),
                                                          string_expand(text),
                                                          string_expand(padding),
                                                          string_expand(symbols_r));
                
                buffer_replace_range(app, buffer, original_range, final_text);
                view_set_cursor_by_character_delta(app, view, final_text.size);
            }
        }
    }
}

CUSTOM_COMMAND_SIG(f0_noop)
CUSTOM_DOC("Do nothing.")
{
}


CUSTOM_COMMAND_SIG(f0_insert_tilde)
CUSTOM_DOC("Inserts tilde character '~'.")
{
    write_string(app, string_u8_litexpr("~"));
}

CUSTOM_COMMAND_SIG(f0_insert_tick)
CUSTOM_DOC("Inserts tick character '`'.")
{
    write_string(app, string_u8_litexpr("`"));
}

CUSTOM_COMMAND_SIG(f0_insert_arrow)
CUSTOM_DOC("Inserts C++ arrow \"->\".")
{
    write_string(app, string_u8_litexpr("->"));
}


//~ "Camera" movement
CUSTOM_COMMAND_SIG(f0_center_view_around_top)
CUSTOM_DOC("Move screen so the cursor is at the top.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Rect_f32 region = view_get_buffer_region(app, view);
    i64 pos = view_get_cursor_pos(app, view);
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
    f32 view_height = rect_height(region);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target.line_number = cursor.line;
    scroll.target.pixel_shift.y = -view_height*0.15f;
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
    no_mark_snap_to_cursor(app, view);
}

CUSTOM_COMMAND_SIG(f0_small_scroll_up)
CUSTOM_DOC("Scroll the screen up a little bit.")
{
    f32 move_dist = -40.0f;
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target = view_move_buffer_point(app, view, scroll.target, V2f32(0.f, move_dist));
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
}


CUSTOM_COMMAND_SIG(f0_big_scroll_up)
CUSTOM_DOC("Scroll the screen up a little bit.")
{
    f32 move_dist = -40.0f*10.f;
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target = view_move_buffer_point(app, view, scroll.target, V2f32(0.f, move_dist));
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
}

CUSTOM_COMMAND_SIG(f0_small_scroll_down)
CUSTOM_DOC("Scroll the screen down a little bit.")
{
    f32 move_dist = 40.0f;
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target = view_move_buffer_point(app, view, scroll.target, V2f32(0.f, move_dist));
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
}

CUSTOM_COMMAND_SIG(f0_big_scroll_down)
CUSTOM_DOC("Scroll the screen down a little bit.")
{
    f32 move_dist = 40.0f*10.f;
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target = view_move_buffer_point(app, view, scroll.target, V2f32(0.f, move_dist));
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
}







//~ Like virtual cursor movement
CUSTOM_COMMAND_SIG(f0_move_right_like_virtual)
CUSTOM_DOC("Moves the cursor one character to the right. Skips leading whitespace.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    view_set_cursor_by_character_delta(app, view, 1);
    no_mark_snap_to_cursor_if_shift(app, view);
    
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    move_past_lead_whitespace(app, view, buffer);
}

CUSTOM_COMMAND_SIG(f0_move_up_like_virtual)
CUSTOM_DOC("Moves the cursor up one line. Skips leading whitespace.")
{
    move_vertical_lines(app, -1);
    
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    f0_move_past_lead_whitespace_like_virtual(app, view, buffer);
}



CUSTOM_COMMAND_SIG(f0_move_down_like_virtual)
CUSTOM_DOC("Moves the cursor down one line. Skips leading whitespace.")
{
    move_vertical_lines(app, 1);
    
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    f0_move_past_lead_whitespace_like_virtual(app, view, buffer);
}

CUSTOM_COMMAND_SIG(f0_lister_goto_position)
CUSTOM_DOC("Performs goto_jump_at_cursor.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer == 0){
        buffer = view_get_buffer(app, view, Access_ReadVisible);
        if (buffer != 0){
            goto_jump_at_cursor(app);
            lock_jump_buffer(app, buffer);
            f0_move_past_lead_whitespace_like_virtual(app, view, buffer);
        }
    }
}

CUSTOM_COMMAND_SIG(f0_lister_goto_position_same_panel)
CUSTOM_DOC("Performs goto_jump_at_cursor_same_panel.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer == 0){
        buffer = view_get_buffer(app, view, Access_ReadVisible);
        if (buffer != 0){
            goto_jump_at_cursor_same_panel(app);
            lock_jump_buffer(app, buffer);
            f0_move_past_lead_whitespace_like_virtual(app, view, buffer);
        }
    }
}

CUSTOM_COMMAND_SIG(f0_backspace_like_virtual)
CUSTOM_DOC("Deletes from the end of previous line to cursor if the range contained only whitespace.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    
    Range_i64 delete_range = {};
    delete_range.max = pos;
    
    i64 cur_line = get_line_number_from_pos(app, buffer, pos);
    Range_i64 cur_line_range = get_line_pos_range(app, buffer, cur_line);
    delete_range.min = cur_line_range.min;
    
    
    Scratch_Block scratch(app);
    String_Const_u8 delete_string = push_buffer_range(app, scratch, buffer, delete_range);
    b32 non_whitespace_found = false;
    for (u64 i = 0; i < delete_string.size; ++i)
    {
        non_whitespace_found = !character_is_whitespace(delete_string.str[i]);
        if (non_whitespace_found) { break; }
    }
    
    
    if (non_whitespace_found)
    {
        backspace_char(app);
    }
    else
    {
        i64 prev_line = cur_line-1;
        if (prev_line > 0)
        {
            Range_i64 prev_line_range = get_line_pos_range(app, buffer, prev_line);
            delete_range.min = prev_line_range.max;
        }
        
        buffer_replace_range(app, buffer, delete_range, string_u8_litexpr(""));
    }
}


CUSTOM_COMMAND_SIG(f0_auto_indent_three_lines_at_cursor)
CUSTOM_DOC("Auto-indents the line on which the cursor sits and the one above.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Range_i64 indent_range = Ii64(pos);
    
    i64 cur_line = get_line_number_from_pos(app, buffer, pos);
    i64 prev_line = cur_line - 2;
    if (prev_line > 0)
    {
        Range_i64 prev_line_range = get_line_pos_range(app, buffer, prev_line);
        indent_range.min = prev_line_range.max;
    }
    else
    {
        indent_range.min = 0;
    }
    
    auto_indent_buffer(app, buffer, indent_range);
    f0_move_past_lead_whitespace_like_virtual(app, view, buffer);
}
