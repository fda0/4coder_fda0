
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
#if Fda0_Modal_Bindings
    String_ID shared_map_id = vars_save_string_lit("keys_shared");
    String_ID command_map_id = vars_save_string_lit("keys_command");
#endif
    
    if (dynamic_binding_load_from_file(app, &framework_mapping, bindings_file_name))
    {
#if Fda0_Modal_Bindings
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












////////////////////////////////

DELTA_RULE_SIG(F4_DeltaRule)
{
    // NOTE(fda0): Taken from Rayn Fleury's layer
    Vec2_f32 *velocity = (Vec2_f32*)data;
    if (velocity->x == 0.f)
    {
        velocity->x = 1.f;
        velocity->y = 1.f;
    }
    Smooth_Step step_x = smooth_camera_step(pending.x, velocity->x, 80.f, 1.f/4.f);
    Smooth_Step step_y = smooth_camera_step(pending.y, velocity->y, 80.f, 1.f/4.f);
    *velocity = V2f32(step_x.v, step_y.v);
    return(V2f32(step_x.p, step_y.p));
}




void
custom_layer_init(Application_Links *app)
{
    Thread_Context *tctx = get_thread_context(app);
    
    // NOTE(allen): setup for default framework
    default_framework_init(app);
    
    // NOTE(allen/fda0): default hooks and command maps
    set_all_default_hooks(app);
    mapping_init(tctx, &framework_mapping);
    String_ID global_map_id = vars_save_string_lit("keys_global");
    String_ID file_map_id = vars_save_string_lit("keys_file");
    fda0_code_map_id = vars_save_string_lit("keys_code");
    
#if Fda0_Modal_Bindings
    String_ID shared_map_id = vars_save_string_lit("keys_shared");
    fda0_command_map_id = vars_save_string_lit("keys_command");
#endif
    
    
#if OS_MAC
    setup_mac_mapping(&framework_mapping, global_map_id, file_map_id, fda0_code_map_id);
#else
    setup_default_mapping(&framework_mapping, global_map_id, file_map_id, fda0_code_map_id);
#endif
    
    
    
    
#if Fda0_Modal_Bindings
	fda0_setup_essential_mapping(&framework_mapping,
                                 global_map_id, file_map_id, fda0_code_map_id,
                                 shared_map_id, fda0_code_map_id);
#else
    setup_essential_mapping(&framework_mapping, global_map_id, file_map_id, fda0_code_map_id);
#endif
    
    
    // NOTE(fda0): Hooks
#if Fda0_Render
    set_custom_hook(app, HookID_RenderCaller, fda0_render);
#endif
#if Fda0_Delta_Rule
    set_custom_hook(app, HookID_DeltaRule, F4_DeltaRule);
#endif
    
    set_custom_hook(app, HookID_Tick, fda0_tick);
}





























