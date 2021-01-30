

CUSTOM_COMMAND_SIG(fda0_noop)
CUSTOM_DOC("Do nothing")
{
}


CUSTOM_COMMAND_SIG(fda0_insert_tilde)
CUSTOM_DOC("Insert tilde '~'.")
{
    write_string(app, string_u8_litexpr("~"));
}

CUSTOM_COMMAND_SIG(fda0_insert_arrow)
CUSTOM_DOC("Insert C++ arrow '->'")
{
    write_string(app, string_u8_litexpr("->"));
}

CUSTOM_COMMAND_SIG(fda0_center_view_around_top)
CUSTOM_DOC("Move screen so the cursor is at the top")
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

CUSTOM_COMMAND_SIG(fda0_scroll_up)
CUSTOM_DOC("Scroll the screen up")
{
    f32 move_dist = -40.0f;
    
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target = view_move_buffer_point(app, view, scroll.target, V2f32(0.f, move_dist));
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
}

CUSTOM_COMMAND_SIG(fda0_scroll_down)
CUSTOM_DOC("Scroll the screen down")
{
    f32 move_dist = 40.0f;
    
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target = view_move_buffer_point(app, view, scroll.target, V2f32(0.f, move_dist));
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
}
