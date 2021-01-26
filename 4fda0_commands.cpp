
// NOTE(fda0): Initialization stuff!

function void
fda0_4coder_initialize(Application_Links *app, String_Const_u8_Array file_names, i32 override_font_size, b32 override_hinting){
#define M \
"Welcome to " VERSION "\n" \
"If you're new to 4coder there is a built in tutorial\n" \
"Use the key combination [ X Alt ] (on mac [ X Control ])\n" \
"Type in 'hms_demo_tutorial' and press enter\n" \
"\n" \
"Direct bug reports and feature requests to https://github.com/4coder-editor/4coder/issues\n" \
"\n" \
"Other questions and discussion can be directed to editor@4coder.net or 4coder.handmade.network\n" \
"\n" \
"The change log can be found in CHANGES.txt\n" \
"\n"
    print_message(app, string_u8_litexpr(M));
#undef M
    
    Scratch_Block scratch(app);
    
    load_config_and_apply(app, &global_config_arena, override_font_size, override_hinting);
    
    String_Const_u8 bindings_file_name = string_u8_litexpr("bindings.4coder");
    String_Const_u8 mapping = def_get_config_string(scratch, vars_save_string_lit("mapping"));
    
    if (string_match(mapping, string_u8_litexpr("mac-default")))
    {
        bindings_file_name = string_u8_litexpr("mac-bindings.4coder");
    }
    else if (OS_MAC && string_match(mapping, string_u8_litexpr("choose")))
    {
        bindings_file_name = string_u8_litexpr("mac-bindings.4coder");
    }
    
    // TODO(allen): cleanup
    String_ID global_map_id = vars_save_string_lit("keys_global");
    String_ID file_map_id = vars_save_string_lit("keys_file");
    String_ID code_map_id = vars_save_string_lit("keys_code");
    String_ID shared_map_id = vars_save_string_lit("keys_shared");
    String_ID command_map_id = vars_save_string_lit("keys_command");
    
    if (dynamic_binding_load_from_file(app, &framework_mapping, bindings_file_name))
    {
#if Enable_Modal_Bindings
        fda0_setup_essential_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id, shared_map_id, command_map_id);
#else
        setup_essential_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
#endif
    }
    else
    {
        setup_built_in_mapping(app, mapping, &framework_mapping, global_map_id, file_map_id, code_map_id);
    }
    
    // open command line files
    String_Const_u8 hot_directory = push_hot_directory(app, scratch);
    for (i32 i = 0; i < file_names.count; i += 1){
        Temp_Memory_Block temp(scratch);
        String_Const_u8 input_name = file_names.vals[i];
        String_Const_u8 full_name = push_u8_stringf(scratch, "%.*s/%.*s",
                                                    string_expand(hot_directory),
                                                    string_expand(input_name));
        Buffer_ID new_buffer = create_buffer(app, full_name, BufferCreate_NeverNew|BufferCreate_MustAttachToFile);
        if (new_buffer == 0){
            create_buffer(app, input_name, 0);
        }
    }
}



CUSTOM_COMMAND_SIG(fda0_startup)
CUSTOM_DOC("fda0 command for responding to a startup event")
{
    ProfileScope(app, "default startup");
    User_Input input = get_current_input(app);
    if (match_core_code(&input, CoreCode_Startup))
    {
        String_Const_u8_Array file_names = input.event.core.file_names;
        load_themes_default_folder(app);
        {
            Face_Description description = get_face_description(app, 0);
            fda0_4coder_initialize(app, file_names, description.parameters.pt_size, description.parameters.hinting);
        }
        default_4coder_side_by_side_panels(app, file_names);
        b32 auto_load = def_get_config_b32(vars_save_string_lit("automatically_load_project"));
        if (auto_load)
        {
            load_project(app);
        }
    }
    
#if 0
    {
        def_audio_init();
        
        Scratch_Block scratch(app);
        FILE *file = def_search_normal_fopen(scratch, "audio_test/raygun_zap.wav", "rb");
        if (file != 0)
        {
            Audio_Clip test_clip = audio_clip_from_wav_FILE(&global_permanent_arena, file);
            fclose(file);
            
            local_persist Audio_Control test_control = {};
            test_control.channel_volume[0] = 1.f;
            test_control.channel_volume[1] = 1.f;
            def_audio_play_clip(test_clip, &test_control);
        }
    }
#endif
    
    {
        def_enable_virtual_whitespace = def_get_config_b32(vars_save_string_lit("enable_virtual_whitespace"));
        clear_all_layouts(app);
    }
    
    
    // NOTE(rjf): Initialize stylish fonts.
    {
        // NOTE(rjf): Small code font.
        Scratch_Block scratch(app);
        String_Const_u8 bin_path = system_get_path(scratch, SystemPath_Binary);
        
        Face_ID normal_font = get_face_id(app, 0);
        Face_Description normal_font_desc = get_face_description(app, normal_font);
        
        Face_Description desc = {0};
        {
            char *small_font_font_file_name = "%.*sfonts/Inconsolata-Regular.ttf";
            //char *small_font_font_file_name = "%.*sfonts/DroidSansMono.ttf";
            
            desc.font.file_name = push_u8_stringf(scratch, small_font_font_file_name, string_expand(bin_path));
            desc.parameters.pt_size = normal_font_desc.parameters.pt_size - 1;
            desc.parameters.bold = 1;
            desc.parameters.italic = 1;
            desc.parameters.hinting = 0;
        }
        
        global_small_code_face = try_create_new_face(app, &desc);
        if (!global_small_code_face)
        {
            global_small_code_face = normal_font;
        }
    }
}



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

