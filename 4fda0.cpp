#include <stdlib.h>
#include <stdio.h>
#include <string.h>



//~ NOTE(fda0): Configuration switches
#define Fda0_Bookmark 1 // A way to toggle my changes in default files.

#define Fda0_Modal_Bindings 1 // Use to enable "modal mode" inspired by Casey's config
// Adds keys_shared (new) and keys_command (new)
// keys_shared becomes a parent for keys_command and keys_file (and therefore also a grandparent for keys_code)
//
// Commands for changing keyboard map: fda0_toggle_editor_mode, fda0_enter_input_mode, fda0_enter_command_mode
//
// Keyboard map is enforced on every tick in fda0_tick. You may want to change this behavior

#define Fda0_Render 1 // 1 = custom render rules, 0 = default 4coder rules
#define Fda0_Delta_Rule 1 // 1 = custom Fleury's scrolling behavior, 0 = default



//~////////////////////
#include "4coder_default_include.cpp"
#include "4fda0_global_data.cpp"
#include "generated/managed_id_metadata.cpp"



//~ NOTE(fda0): Forward declarations
function void fda0_render_buffer(Application_Links *app, View_ID view_id, Face_ID face_id, Buffer_ID buffer, Text_Layout_ID text_layout_id, Rect_f32 rect);
function void fda0_render(Application_Links *app, Frame_Info frame_info, View_ID view_id);
function void fda0_tick(Application_Links *app, Frame_Info frame_info);
function void fda0_setup_essential_mapping(Mapping *mapping, i64 global_id, i64 file_id, i64 code_id, i64 shared_id, i64 command_id);



//~////////////////////
#include "4fleury_cursor.cpp"
#include "4fleury_brace.cpp"
#include "4fleury_others.cpp"

#include "4fda0_utilities.cpp"
#include "4fda0_commands.cpp"
#include "4fda0_definitions_lister.cpp"

#include "4fda0_startup.cpp"




//~////////////////////
// TODO(fda0): Show whitespace shouldn't color '\n' or '\r'
// TODO(fda0): Color "\ " (escaped spaces) to catch bugs in #define











//~//////////////////////////////////////////////////////////////////

function void
fda0_setup_essential_mapping(Mapping *mapping,
                             i64 global_id, i64 file_id, i64 code_id,
                             i64 shared_id, i64 command_id)
{
    MappingScope();
    SelectMapping(mapping);
    
    SelectMap(global_id);
    BindCore(fda0_startup, CoreCode_Startup);
    BindCore(default_try_exit, CoreCode_TryExit);
    BindCore(clipboard_record_clip, CoreCode_NewClipboardContents);
    BindMouseWheel(mouse_wheel_scroll);
    BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);
    
    
    SelectMap(shared_id);
    ParentMap(global_id);
    BindMouse(click_set_cursor_and_mark, MouseCode_Left);
    BindMouseRelease(click_set_cursor, MouseCode_Left);
    BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
    BindMouseMove(click_set_cursor_if_lbutton);
    
    SelectMap(file_id);
    ParentMap(shared_id);
    BindTextInput(write_text_input);
    
    SelectMap(code_id);
    ParentMap(file_id);
    BindTextInput(write_text_and_auto_indent);
    
    SelectMap(command_id);
    ParentMap(shared_id);
}


function void
fda0_tick(Application_Links *app, Frame_Info frame_info)
{
    default_tick(app, frame_info);
    
#if Fda0_Modal_Bindings
    // NOTE(fda0): Update keyboard profile (hacky?)
    set_current_mapid(app, fda0_command_mode ? fda0_command_map_id : fda0_code_map_id);
#endif
}










function void
fda0_render_buffer(Application_Links *app, View_ID view_id, Face_ID face_id,
                   Buffer_ID buffer, Text_Layout_ID text_layout_id,
                   Rect_f32 rect, Frame_Info frame_info)
{
    // NOTE(fda0): Stuff with "NOTE(rfj)" comes from Ryan's layer
    
    ProfileScope(app, "fda0 render buffer");
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    Rect_f32 prev_clip = draw_set_clip(app, rect);
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    // NOTE(allen): Cursor shape
    Face_Metrics metrics = get_face_metrics(app, face_id);
    u64 cursor_roundness_100 = def_get_config_u64(app, vars_save_string_lit("cursor_roundness"));
    f32 cursor_roundness = metrics.normal_advance*cursor_roundness_100*0.01f;
    f32 mark_thickness = (f32)def_get_config_u64(app, vars_save_string_lit("mark_thickness"));
    
    
    
    
    
    
    i64 cursor_pos = view_correct_cursor(app, view_id);
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    
    
    // NOTE(fda0): Searches for currently hovered string.
    if (token_array.tokens != 0)
    {
        ProfileScope(app, "[Fleury] Token Highlight / saving token's hash");
        Scratch_Block fda0_scratch(app);
        
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, cursor_pos);
        Token *token = token_it_read(&it);
        if (token && token->kind == TokenBaseKind_Identifier)
        {
            // NOTE(fda0): Saving current highlight.
            String_Const_u8 hover_string = push_buffer_range(app, fda0_scratch, buffer, Ii64(token->pos, token->pos + token->size));
            fda0_current_hover_hash = fda0_get_hash(hover_string);
        }
        else
        {
            fda0_current_hover_hash = 18446744073058989232;
        }
    }
    else
    {
        fda0_current_hover_hash = 18446744073058989232;
    }
    
    
    
    
    
    
    // NOTE(allen): Token colorizing
    if (token_array.tokens != 0)
    {
        draw_cpp_token_colors(app, text_layout_id, &token_array);
        
        // NOTE(allen): Scan for TODOs and NOTEs
        b32 use_comment_keyword = def_get_config_b32(vars_save_string_lit("use_comment_keywords"));
        if (use_comment_keyword)
        {
            Comment_Highlight_Pair pairs[] = {
                {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
                {string_u8_litexpr("IMPORTANT"), finalize_color(defcolor_comment_pop, 2)},
            };
            draw_comment_highlights(app, buffer, text_layout_id, &token_array, pairs, ArrayCount(pairs));
        }
        
#if 1
        // TODO(allen): Put in 4coder_draw.cpp
        // NOTE(allen): Color functions
        
        Scratch_Block scratch(app);
        
        ARGB_Color color_type = finalize_color(defcolor_type, 0);
        ARGB_Color color_function = fcolor_resolve(fcolor_id(defcolor_function));
        ARGB_Color color_macro = fcolor_resolve(fcolor_id(defcolor_macro));
        ARGB_Color color_4coder_command = fcolor_resolve(fcolor_id(defcolor_4coder_command));
        
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, visible_range.first);
        for (;;)
        {
            if (!token_it_inc_non_whitespace(&it)){
                break;
            }
            
            Token *token = token_it_read(&it);
            String_Const_u8 lexeme = push_token_lexeme(app, scratch, buffer, token);
            Code_Index_Note *note = code_index_note_from_string(lexeme);
            if (note != 0)
            {
                if (note->note_kind == CodeIndexNote_Type)
                {
                    paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), color_type);
                }
                else if (note->note_kind == CodeIndexNote_Function)
                {
                    paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), color_function);
                }
                else if (note->note_kind == CodeIndexNote_Macro)
                {
                    paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), color_macro);
                }
                else if (note->note_kind == CodeIndexNote_4coderCommand)
                {
                    paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), color_4coder_command);
                }
                
            }
            
            u64 hash = fda0_get_hash(lexeme);
            b32 background_highlight = (hash == fda0_current_hover_hash);
            if (background_highlight)
            {
                F4_RenderRangeHighlight(app, view_id, text_layout_id, Ii64(token->pos, token->pos + token->size));
            }
        }
#endif
    }
    else{
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    }
    
    
    
    
    
    
    
    
    
    view_correct_mark(app, view_id);
    
    // NOTE(allen): Scope highlight
    b32 use_scope_highlight = def_get_config_b32(vars_save_string_lit("use_scope_highlight"));
    if (use_scope_highlight){
        Color_Array colors = finalize_color_array(defcolor_back_cycle);
        draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    // NOTE(rjf): Brace highlight
    {
        Color_Array brace_colors = finalize_color_array(defcolor_brace_highlight);
        F4_RenderBraceHighlight(app, buffer, text_layout_id, cursor_pos, brace_colors.vals, brace_colors.count);
    }
    
    
    
    b32 use_error_highlight = def_get_config_b32(vars_save_string_lit("use_error_highlight"));
    b32 use_jump_highlight = def_get_config_b32(vars_save_string_lit("use_jump_highlight"));
    if (use_error_highlight || use_jump_highlight)
    {
        // NOTE(allen): Error highlight
        String_Const_u8 name = string_u8_litexpr("*compilation*");
        Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
        if (use_error_highlight)
        {
            draw_jump_highlights(app, buffer, text_layout_id, compilation_buffer,
                                 fcolor_id(defcolor_highlight_junk));
        }
        
        // NOTE(allen): Search highlight
        if (use_jump_highlight)
        {
            Buffer_ID jump_buffer = get_locked_jump_buffer(app);
            if (jump_buffer != compilation_buffer){
                draw_jump_highlights(app, buffer, text_layout_id, jump_buffer,
                                     fcolor_id(defcolor_highlight_white));
            }
        }
    }
    
    // NOTE(rjf): Error annotations
    {
        String_Const_u8 name = string_u8_litexpr("*compilation*");
        Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
        F4_RenderErrorAnnotations(app, buffer, text_layout_id, compilation_buffer);
    }
    
    // NOTE(allen): Color parens
    b32 use_paren_helper = def_get_config_b32(vars_save_string_lit("use_paren_helper"));
    if (use_paren_helper)
    {
        Color_Array colors = finalize_color_array(defcolor_text_cycle);
        draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    // NOTE(rjf): Divider Comments
    {
        F4_RenderDividerComments(app, buffer, view_id, text_layout_id);
    }
    
    // NOTE(allen): Line highlight
    b32 highlight_line_at_cursor = def_get_config_b32(vars_save_string_lit("highlight_line_at_cursor"));
    if (highlight_line_at_cursor && is_active_view){
        i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
        draw_line_highlight(app, text_layout_id, line_number, fcolor_id(defcolor_highlight_cursor_line));
    }
    
    // NOTE(allen): Whitespace highlight
    b64 show_whitespace = false;
    view_get_setting(app, view_id, ViewSetting_ShowWhitespace, &show_whitespace);
    if (show_whitespace){
        if (token_array.tokens == 0){
            draw_whitespace_highlight(app, buffer, text_layout_id, cursor_roundness);
        }
        else{
            draw_whitespace_highlight(app, text_layout_id, &token_array, cursor_roundness);
        }
    }
    
    // NOTE(allen): Cursor
    switch (fcoder_mode){
        case FCoderMode_Original:
        {
#if 0
            draw_original_4coder_style_cursor_mark_highlight(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
#else
            fda0_render_cursor(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness, frame_info);
#endif
        }break;
        case FCoderMode_NotepadLike:
        {
            draw_notepad_style_cursor_highlight(app, view_id, buffer, text_layout_id, cursor_roundness);
        }break;
    }
    
    
    // NOTE(rjf): Brace annotations
    {
        F4_RenderCloseBraceAnnotation(app, buffer, text_layout_id, cursor_pos);
    }
    
    // NOTE(rjf): Brace lines
    {
        F4_RenderBraceLines(app, buffer, view_id, text_layout_id, cursor_pos);
    }
    
    // NOTE(allen): Fade ranges
    paint_fade_ranges(app, text_layout_id, buffer);
    
    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);
    
    draw_set_clip(app, prev_clip);
    
    
    
    
    
    
    // NOTE(rjf): Draw tooltips and stuff.
    if(active_view == view_id)
    {
        // NOTE(rjf): Function helper
        if (fda0_function_helper)
        {
            F4_RenderFunctionHelper(app, view_id, buffer, text_layout_id, cursor_pos);
        }
        
    }
    
}














function void
fda0_render(Application_Links *app, Frame_Info frame_info, View_ID view_id)
{
    ProfileScope(app, "fda0 render caller");
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    
    Rect_f32 region = draw_background_and_margin(app, view_id, is_active_view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) &&
        showing_file_bar)
    {
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        draw_file_bar(app, view_id, buffer, face_id, pair.min);
        region = pair.max;
    }
    
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    
    Buffer_Point_Delta_Result delta = delta_apply(app, view_id, frame_info.animation_dt, scroll);
    if (!block_match_struct(&scroll.position, &delta.point))
    {
        block_copy_struct(&scroll.position, &delta.point);
        view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
    }
    if (delta.still_animating)
    {
        fda0_buffer_is_scrolling = true;
        animate_in_n_milliseconds(app, 0);
    }
    else
    {
        fda0_buffer_is_scrolling = false;
    }
    
    // NOTE(allen): query bars
    region = default_draw_query_bars(app, region, view_id, face_id);
    
    // NOTE(allen): FPS hud
    if (show_fps_hud)
    {
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        draw_fps_hud(app, frame_info, face_id, pair.max);
        region = pair.min;
        animate_in_n_milliseconds(app, 1000);
    }
    
    // NOTE(allen): layout line numbers
    b32 show_line_number_margins = def_get_config_b32(vars_save_string_lit("show_line_number_margins"));
    Rect_f32 line_number_rect = {};
    if (show_line_number_margins)
    {
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        line_number_rect = pair.min;
        region = pair.max;
    }
    
    // NOTE(allen): begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
    
    // NOTE(allen): draw line numbers
    if (show_line_number_margins)
    {
        draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
    }
    
    // NOTE(allen): draw the buffer
    fda0_render_buffer(app, view_id, face_id, buffer, text_layout_id, region, frame_info);
    
    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}








