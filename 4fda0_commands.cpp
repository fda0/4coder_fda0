
function void
f0_move_past_lead_whitespace(Application_Links *app, View_ID view, Buffer_ID buffer)
{
    i64 pos = view_get_cursor_pos(app, view);
    i64 new_pos = get_pos_past_lead_whitespace(app, buffer, pos);
    view_set_cursor(app, view, seek_pos(new_pos));
}

//


CUSTOM_COMMAND_SIG(f0_move_right_like_virtual)
CUSTOM_DOC("Moves the cursor one character to the right. Skips leading whitespace.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    view_set_cursor_by_character_delta(app, view, 1);
    no_mark_snap_to_cursor_if_shift(app, view);
    
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    f0_move_past_lead_whitespace(app, view, buffer);
}

CUSTOM_COMMAND_SIG(f0_move_up_like_virtual)
CUSTOM_DOC("Moves the cursor up one line. Skips leading whitespace.")
{
    move_vertical_lines(app, -1);
    
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    f0_move_past_lead_whitespace(app, view, buffer);
}



CUSTOM_COMMAND_SIG(f0_move_down_like_virtual)
CUSTOM_DOC("Moves the cursor down one line. Skips leading whitespace.")
{
    move_vertical_lines(app, 1);
    
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    f0_move_past_lead_whitespace(app, view, buffer);
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

CUSTOM_COMMAND_SIG(f0_scroll_up)
CUSTOM_DOC("Scroll the screen up a little bit.")
{
    f32 move_dist = -40.0f;

    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target = view_move_buffer_point(app, view, scroll.target, V2f32(0.f, move_dist));
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
}

CUSTOM_COMMAND_SIG(f0_scroll_down)
CUSTOM_DOC("Scroll the screen down a little bit.")
{
    f32 move_dist = 40.0f;

    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target = view_move_buffer_point(app, view, scroll.target, V2f32(0.f, move_dist));
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
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
            f0_move_past_lead_whitespace(app, view, buffer);
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
            f0_move_past_lead_whitespace(app, view, buffer);
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


CUSTOM_COMMAND_SIG(f0_auto_indent_two_lines_at_cursor)
CUSTOM_DOC("Auto-indents the line on which the cursor sits and the one above.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Range_i64 indent_range = Ii64(pos);
    
    i64 cur_line = get_line_number_from_pos(app, buffer, pos);
    i64 prev_line = cur_line - 1;
    if (prev_line > 0)
    {
        Range_i64 prev_line_range = get_line_pos_range(app, buffer, prev_line);
        indent_range.min = prev_line_range.max;
    }
    
    auto_indent_buffer(app, buffer, indent_range);
    f0_move_past_lead_whitespace(app, view, buffer);
}

