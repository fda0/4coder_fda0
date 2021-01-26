


//~ NOTE(fda0): Custom commands.


void set_current_mapid(Application_Links* app, Command_Map_ID mapid)
{
    // NOTE(mg): Function from https://4coder.handmade.network/wiki/7319-customization_layer_-_getting_started__4coder_4.1_
    
    View_ID view = get_active_view(app, 0);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Command_Map_ID *map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
    
    *map_id_ptr = mapid;
}

CUSTOM_COMMAND_SIG(fda0_toggle_editor_mode)
CUSTOM_DOC("Switch between editor modes (text/command)")
{
    fda0_command_mode = !fda0_command_mode;
    
    if (fda0_command_mode) { set_current_mapid(app, fda0_command_map_id); }
    else                   { set_current_mapid(app, fda0_code_map_id); }
}

CUSTOM_COMMAND_SIG(fda0_enter_command_mode)
CUSTOM_DOC("Switch editor to command mode")
{
    fda0_command_mode = true;
    set_current_mapid(app, fda0_command_map_id);
}

CUSTOM_COMMAND_SIG(fda0_enter_input_mode)
CUSTOM_DOC("Switch editor to text input mode")
{
    fda0_command_mode = false;
    set_current_mapid(app, fda0_code_map_id);
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

CUSTOM_COMMAND_SIG(fda0_toggle_function_helper)
CUSTOM_DOC("Toggle function helper (helps with function arguments)")
{
    fda0_function_helper = !fda0_function_helper;
}



#if 1
CUSTOM_COMMAND_SIG(fda0_debug_print)
CUSTOM_DOC("Print current _debug_ value")
{
    i64 current_map_id = -69;
    {    
        View_ID view = get_active_view(app, 0);
        Buffer_ID buffer = view_get_buffer(app, view, 0);
        Managed_Scope scope = buffer_get_managed_scope(app, buffer);
        Command_Map_ID *map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
        current_map_id = *map_id_ptr;
    }
    
    char buffer[256]; // TODO(fda0): Use push_stringf(...)
    auto len = snprintf(buffer, sizeof(buffer), 
                        "[keys] code: %lld, comamnd: %lld, current: %lld",
                        fda0_code_map_id, fda0_command_map_id, current_map_id);
    
    String_Const_u8 string;
    string.str = (u8 *)buffer;
    string.size = clamp(0, len, sizeof(buffer));
    write_string(app, string);
}
#endif

