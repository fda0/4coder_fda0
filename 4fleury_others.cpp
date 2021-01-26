


static f32
MinimumF32(f32 a, f32 b)
{
    return a < b ? a : b;
}

static f32
MaximumF32(f32 a, f32 b)
{
    return a > b ? a : b;
}



#if 0
static void
F4_PushTooltip(String_Const_u8 string, ARGB_Color color)
{
    if(global_tooltip_count < ArrayCount(global_tooltips))
    {
        String_Const_u8 string_copy = push_string_copy(&global_frame_arena, string);
        global_tooltips[global_tooltip_count].color = color;
        global_tooltips[global_tooltip_count].string = string_copy;
        global_tooltip_count += 1;
    }
}
#endif



//~ NOTE(rjf): Error annotations

static void
F4_RenderErrorAnnotations(Application_Links *app, Buffer_ID buffer,
                          Text_Layout_ID text_layout_id,
                          Buffer_ID jump_buffer)
{
    ProfileScope(app, "[Fleury] Error Annotations");
    
    Heap *heap = &global_heap;
    Scratch_Block scratch(app);
    
    Locked_Jump_State jump_state = {};
    {
        ProfileScope(app, "[Fleury] Error Annotations (Get Locked Jump State)");
        jump_state = get_locked_jump_state(app, heap);
    }
    
    Face_ID face = global_small_code_face;
    Face_Metrics metrics = get_face_metrics(app, face);
    
    if(jump_buffer != 0 && jump_state.view != 0)
    {
        Managed_Scope buffer_scopes[2];
        {
            ProfileScope(app, "[Fleury] Error Annotations (Buffer Get Managed Scope)");
            buffer_scopes[0] = buffer_get_managed_scope(app, jump_buffer);
            buffer_scopes[1] = buffer_get_managed_scope(app, buffer);
        }
        
        Managed_Scope comp_scope = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Get Managed Scope)");
            comp_scope = get_managed_scope_with_multiple_dependencies(app, buffer_scopes, ArrayCount(buffer_scopes));
        }
        
        Managed_Object *buffer_markers_object = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Scope Attachment)");
            buffer_markers_object = scope_attachment(app, comp_scope, sticky_jump_marker_handle, Managed_Object);
        }
        
        // NOTE(rjf): Get buffer markers (locations where jumps point at).
        i32 buffer_marker_count = 0;
        Marker *buffer_markers = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Load Managed Object Data)");
            buffer_marker_count = managed_object_get_item_count(app, *buffer_markers_object);
            buffer_markers = push_array(scratch, Marker, buffer_marker_count);
            managed_object_load_data(app, *buffer_markers_object, 0, buffer_marker_count, buffer_markers);
        }
        
        i64 last_line = -1;
        
        for(i32 i = 0; i < buffer_marker_count; i += 1)
        {
            ProfileScope(app, "[Fleury] Error Annotations (Buffer Loop)");
            
            i64 jump_line_number = get_line_from_list(app, jump_state.list, i);
            i64 code_line_number = get_line_number_from_pos(app, buffer, buffer_markers[i].pos);
            
            if(code_line_number != last_line)
            {
                ProfileScope(app, "[Fleury] Error Annotations (Jump Line)");
                
                String_Const_u8 jump_line = push_buffer_line(app, scratch, jump_buffer, jump_line_number);
                
                // NOTE(rjf): Remove file part of jump line.
                {
                    u64 index = string_find_first(jump_line, string_u8_litexpr("error"), StringMatch_CaseInsensitive);
                    if(index == jump_line.size)
                    {
                        index = string_find_first(jump_line, string_u8_litexpr("warning"), StringMatch_CaseInsensitive);
                        if(index == jump_line.size)
                        {
                            index = 0;
                        }
                    }
                    jump_line.str += index;
                    jump_line.size -= index;
                }
                
                // NOTE(rjf): Render annotation.
                {
                    Range_i64 line_range = Ii64(code_line_number);
                    Range_f32 y1 = text_layout_line_on_screen(app, text_layout_id, line_range.min);
                    Range_f32 y2 = text_layout_line_on_screen(app, text_layout_id, line_range.max);
                    Range_f32 y = range_union(y1, y2);
                    Rect_f32 last_character_on_line_rect =
                        text_layout_character_on_screen(app, text_layout_id, get_line_end_pos(app, buffer, code_line_number)-1);
                    
                    if(range_size(y) > 0.f)
                    {
                        Rect_f32 region = text_layout_region(app, text_layout_id);
                        Vec2_f32 draw_position =
                        {
                            region.x1 - metrics.max_advance*jump_line.size -
                                (y.max-y.min)/2 - metrics.line_height/2,
                            y.min + (y.max-y.min)/2 - metrics.line_height/2,
                        };
                        
                        if(draw_position.x < last_character_on_line_rect.x1 + 30)
                        {
                            draw_position.x = last_character_on_line_rect.x1 + 30;
                        }
                        
                        draw_string(app, face, jump_line, draw_position, 0xffff0000);
                        
#if 0                        
                        Mouse_State mouse_state = get_mouse_state(app);
                        if(mouse_state.x >= region.x0 && mouse_state.x <= region.x1 &&
                           mouse_state.y >= y.min && mouse_state.y <= y.max)
                        {
                            F4_PushTooltip(jump_line, 0xffff0000);
                        }
#endif
                    }
                }
            }
            
            last_line = code_line_number;
        }
    }
}


//~ NOTE(rjf): Divider Comments

static void
F4_RenderDividerComments(Application_Links *app, Buffer_ID buffer, View_ID view,
                         Text_Layout_ID text_layout_id)
{
    ProfileScope(app, "[Fleury] Divider Comments");
    
    String_Const_u8 divider_comment_signifier = string_u8_litexpr("//~");
    
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    Scratch_Block scratch(app);
    
    if(token_array.tokens != 0)
    {
        i64 first_index = token_index_from_pos(&token_array, visible_range.first);
        Token_Iterator_Array it = token_iterator_index(0, &token_array, first_index);
        
        Token *token = 0;
        for(;;)
        {
            token = token_it_read(&it);
            
            if(token->pos >= visible_range.one_past_last || !token || !token_it_inc_non_whitespace(&it))
            {
                break;
            }
            
            if(token->kind == TokenBaseKind_Comment)
            {
                Rect_f32 comment_first_char_rect =
                    text_layout_character_on_screen(app, text_layout_id, token->pos);
                
                Range_i64 token_range =
                {
                    token->pos,
                    token->pos + (token->size > (i64)divider_comment_signifier.size
                                  ? (i64)divider_comment_signifier.size
                                  : token->size),
                };
                
                u8 token_buffer[256] = {0};
                buffer_read_range(app, buffer, token_range, token_buffer);
                String_Const_u8 token_string = { token_buffer, (u64)(token_range.end - token_range.start) };
                
                if(string_match(token_string, divider_comment_signifier))
                {
                    // NOTE(rjf): Render divider line.
                    Rect_f32 rect =
                    {
                        comment_first_char_rect.x0,
                        comment_first_char_rect.y0-2,
                        10000,
                        comment_first_char_rect.y0,
                    };
                    f32 roundness = 4.f;
                    draw_rectangle(app, rect, roundness, fcolor_resolve(fcolor_id(defcolor_comment)));
                }
                
            }
            
        }
        
    }
    
}



//~


static Range_i64_Array
F4_GetLeftParens(Application_Links *app, Arena *arena, Buffer_ID buffer, i64 pos, u32 flags)
{
    Range_i64_Array array = {};
    i32 max = 100;
    array.ranges = push_array(arena, Range_i64, max);
    
    for(;;)
    {
        Range_i64 range = {};
        range.max = pos;
        if(find_nest_side(app, buffer, pos - 1, flags | FindNest_Balanced,
                          Scan_Backward, NestDelim_Open, &range.start))
        {
            array.ranges[array.count] = range;
            array.count += 1;
            pos = range.first;
            if (array.count >= max)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    return(array);
}

static String_Const_u8
F4_CopyStringButOnlyAllowOneSpace(Arena *arena, String_Const_u8 string)
{
    String_Const_u8 result = {0};
    
    u64 space_to_allocate = 0;
    u64 spaces_left_this_gap = 1;
    
    for(u64 i = 0; i < string.size; ++i)
    {
        if(string.str[i] <= 32)
        {
            if(spaces_left_this_gap > 0)
            {
                --spaces_left_this_gap;
                ++space_to_allocate;
            }
        }
        else
        {
            spaces_left_this_gap = 1;
            ++space_to_allocate;
        }
    }
    
    result.str = push_array(arena, u8, space_to_allocate);
    for(u64 i = 0; i < string.size; ++i)
    {
        if(string.str[i] <= 32)
        {
            if(spaces_left_this_gap > 0)
            {
                --spaces_left_this_gap;
                result.str[result.size++] = string.str[i];
            }
        }
        else
        {
            spaces_left_this_gap = 1;
            result.str[result.size++] = string.str[i];
        }
    }
    
    return result;
}























static void
F4_DrawTooltipRect(Application_Links *app, Rect_f32 rect)
{
    ARGB_Color background_color = fcolor_resolve(fcolor_id(defcolor_back));
    ARGB_Color border_color = fcolor_resolve(fcolor_id(defcolor_margin_active));
    
    background_color &= 0x00ffffff;
    background_color |= 0xd0000000;
    
    border_color &= 0x00ffffff;
    border_color |= 0xd0000000;
    
    draw_rectangle(app, rect, 4.f, background_color);
    draw_rectangle_outline(app, rect, 4.f, 3.f, border_color);
}



static void
F4_RenderRangeHighlight(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id,
                        Range_i64 range)
{
    Rect_f32 range_start_rect = text_layout_character_on_screen(app, text_layout_id, range.start);
    Rect_f32 range_end_rect = text_layout_character_on_screen(app, text_layout_id, range.end-1);
    Rect_f32 total_range_rect = {0};
    total_range_rect.x0 = MinimumF32(range_start_rect.x0, range_end_rect.x0);
    total_range_rect.y0 = MinimumF32(range_start_rect.y0, range_end_rect.y0);
    total_range_rect.x1 = MaximumF32(range_start_rect.x1, range_end_rect.x1);
    total_range_rect.y1 = MaximumF32(range_start_rect.y1, range_end_rect.y1);
    
#if 0
    ARGB_Color background_color = fcolor_resolve(fcolor_id(defcolor_pop2));
    float background_color_r = (float)((background_color & 0x00ff0000) >> 16) / 255.f;
    float background_color_g = (float)((background_color & 0x0000ff00) >>  8) / 255.f;
    float background_color_b = (float)((background_color & 0x000000ff) >>  0) / 255.f;
    background_color_r += (1.f - background_color_r) * 0.5f;
    background_color_g += (1.f - background_color_g) * 0.5f;
    background_color_b += (1.f - background_color_b) * 0.5f;
    ARGB_Color highlight_color = (0x55000000 |
                                  (((u32)(background_color_r * 255.f)) << 16) |
                                  (((u32)(background_color_g * 255.f)) <<  8) |
                                  (((u32)(background_color_b * 255.f)) <<  0));
#else // NOTE(mg): Token highlighting background color is now derived from variable.
    
    ARGB_Color highlight_color = finalize_color(defcolor_hover_background, 0);
    
#endif
    
    draw_rectangle(app, total_range_rect, 4.f, highlight_color);
}








static void
F4_RenderFunctionHelper(Application_Links *app, View_ID view, Buffer_ID buffer,
                        Text_Layout_ID text_layout_id, i64 pos)
{
    ProfileScope(app, "[Fleury] Function Helper");
    
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    Token_Iterator_Array it;
    Token *token = 0;
    
    Rect_f32 view_rect = view_get_screen_rect(app, view);
    
    Face_ID face = global_small_code_face;
    Face_Metrics metrics = get_face_metrics(app, face);
    
    if(token_array.tokens != 0)
    {
        it = token_iterator_pos(0, &token_array, pos);
        token = token_it_read(&it);
        
        if(token != 0 && token->kind == TokenBaseKind_ParentheticalOpen)
        {
            pos = token->pos + token->size;
        }
        else
        {
            if (token_it_dec_all(&it))
            {
                token = token_it_read(&it);
                if (token->kind == TokenBaseKind_ParentheticalClose &&
                    pos == token->pos + token->size)
                {
                    pos = token->pos;
                }
            }
        }
    }
    
    Scratch_Block scratch(app);
    Range_i64_Array ranges = F4_GetLeftParens(app, scratch, buffer, pos, FindNest_Paren);
    
    for(int range_index = 0; range_index < ranges.count; ++range_index)
    {
        it = token_iterator_pos(0, &token_array, ranges.ranges[range_index].min);
        token_it_dec_non_whitespace(&it);
        token = token_it_read(&it);
        if(token->kind == TokenBaseKind_Identifier)
        {
            Range_i64 function_name_range = Ii64(token->pos, token->pos+token->size);
            String_Const_u8 function_name = push_buffer_range(app, scratch, buffer, function_name_range);
            
            //F4_RenderRangeHighlight(app, view, text_layout_id, function_name_range);
            
            // NOTE(rjf): Find active parameter.
            int active_parameter_index = 0;
            {
                it = token_iterator_pos(0, &token_array, function_name_range.min);
                int paren_nest = 0;
                for(;token_it_inc_non_whitespace(&it);)
                {
                    token = token_it_read(&it);
                    if(token->pos + token->size > pos)
                    {
                        break;
                    }
                    
                    if(token->kind == TokenBaseKind_ParentheticalOpen)
                    {
                        ++paren_nest;
                    }
                    else if(token->kind == TokenBaseKind_StatementClose)
                    {
                        if(paren_nest == 1)
                        {
                            ++active_parameter_index;
                        }
                    }
                    else if(token->kind == TokenBaseKind_ParentheticalClose)
                    {
                        if(!--paren_nest)
                        {
                            break;
                        }
                    }
                }
            }
            
            
            
            
            
            for(Buffer_ID buffer_it = get_buffer_next(app, 0, Access_Always);
                buffer_it != 0; buffer_it = get_buffer_next(app, buffer_it, Access_Always))
            {
                Code_Index_File *file = code_index_get_file(buffer_it);
                if(file != 0)
                {
                    for(i32 i = 0; i < file->note_array.count; i += 1)
                    {
                        Code_Index_Note *note = file->note_array.ptrs[i];
                        
                        if((note->note_kind == CodeIndexNote_Function ||
                            note->note_kind == CodeIndexNote_Macro) &&
                           string_match(note->text, function_name))
                        {
                            Range_i64 function_def_range;
                            function_def_range.min = note->pos.min;
                            function_def_range.max = note->pos.max;
                            
                            Range_i64 highlight_parameter_range = {0};
                            
                            Token_Array find_token_array = get_token_array_from_buffer(app, buffer_it);
                            it = token_iterator_pos(0, &find_token_array, function_def_range.min);
                            
                            int paren_nest = 0;
                            int param_index = 0;
                            for(;token_it_inc_non_whitespace(&it);)
                            {
                                token = token_it_read(&it);
                                if(token->kind == TokenBaseKind_ParentheticalOpen)
                                {
                                    if(++paren_nest == 1)
                                    {
                                        if(active_parameter_index == param_index)
                                        {
                                            highlight_parameter_range.min = token->pos+1;
                                        }
                                    }
                                }
                                else if(token->kind == TokenBaseKind_ParentheticalClose)
                                {
                                    if(!--paren_nest)
                                    {
                                        function_def_range.max = token->pos + token->size;
                                        if(param_index == active_parameter_index)
                                        {
                                            highlight_parameter_range.max = token->pos;
                                        }
                                        break;
                                    }
                                }
                                else if(token->kind == TokenBaseKind_StatementClose)
                                {
                                    if(param_index == active_parameter_index)
                                    {
                                        highlight_parameter_range.max = token->pos;
                                    }
                                    
                                    if(paren_nest == 1)
                                    {
                                        ++param_index;
                                    }
                                    
                                    if(param_index == active_parameter_index)
                                    {
                                        highlight_parameter_range.min = token->pos+1;
                                    }
                                }
                            }
                            
                            if(highlight_parameter_range.min > function_def_range.min &&
                               function_def_range.max > highlight_parameter_range.max)
                            {
                                
                                String_Const_u8 function_def = push_buffer_range(app, scratch, buffer_it,
                                                                                 function_def_range);
                                String_Const_u8 highlight_param_untrimmed = push_buffer_range(app, scratch, buffer_it,
                                                                                              highlight_parameter_range);
                                
                                String_Const_u8 pre_highlight_def_untrimmed =
                                {
                                    function_def.str,
                                    (u64)(highlight_parameter_range.min - function_def_range.min),
                                };
                                
                                String_Const_u8 post_highlight_def_untrimmed =
                                {
                                    function_def.str + highlight_parameter_range.max - function_def_range.min,
                                    (u64)(function_def_range.max - highlight_parameter_range.max),
                                };
                                
                                String_Const_u8 highlight_param = F4_CopyStringButOnlyAllowOneSpace(scratch, highlight_param_untrimmed);
                                String_Const_u8 pre_highlight_def = F4_CopyStringButOnlyAllowOneSpace(scratch, pre_highlight_def_untrimmed);
                                String_Const_u8 post_highlight_def = F4_CopyStringButOnlyAllowOneSpace(scratch, post_highlight_def_untrimmed);
                                
                                Rect_f32 helper_rect =
                                {
                                    global_cursor_rect.x0 + 16,
                                    global_cursor_rect.y0 + 16,
                                    global_cursor_rect.x0 + 16,
                                    global_cursor_rect.y0 + metrics.line_height + 26,
                                };
                                
                                f32 padding = (helper_rect.y1 - helper_rect.y0)/2 -
                                    metrics.line_height/2;
                                
                                // NOTE(rjf): Size helper rect by how much text to draw.
                                {
                                    helper_rect.x1 += get_string_advance(app, face, highlight_param);
                                    helper_rect.x1 += get_string_advance(app, face, pre_highlight_def);
                                    helper_rect.x1 += get_string_advance(app, face, post_highlight_def);
                                    helper_rect.x1 += 2 * padding;
                                }
                                
                                if(helper_rect.x1 > view_get_screen_rect(app, view).x1)
                                {
                                    f32 difference = helper_rect.x1 - view_get_screen_rect(app, view).x1;
                                    helper_rect.x0 -= difference;
                                    helper_rect.x1 -= difference;
                                }
                                
                                Vec2_f32 text_position =
                                {
                                    helper_rect.x0 + padding,
                                    helper_rect.y0 + padding,
                                };
                                
                                F4_DrawTooltipRect(app, helper_rect);
                                
                                text_position = draw_string(app, face, pre_highlight_def,
                                                            text_position, finalize_color(defcolor_comment, 0));
                                
                                text_position = draw_string(app, face, highlight_param,
                                                            text_position, finalize_color(defcolor_comment_pop, 1));
                                text_position = draw_string(app, face, post_highlight_def,
                                                            text_position, finalize_color(defcolor_comment, 0));
                                
                                goto end_lookup;
                            }
                        }
                    }
                }
            }
            
            end_lookup:;
            break;
        }
    }
}

