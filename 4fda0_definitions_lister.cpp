
//~ NOTE(fda0): This file should be self contained enough to "just work" in 4coder 4.1.7
//
// Define following macros before including this file to change colors:
#ifndef Fda0_Color_Type
#define Fda0_Color_Type defcolor_type
#endif
#ifndef Fda0_Color_Function
#define Fda0_Color_Function defcolor_function
#endif
#ifndef Fda0_Color_Fordward_Declaration
#define Fda0_Color_Fordward_Declaration defcolor_declaration
#endif
#ifndef Fda0_Color_Macro
#define Fda0_Color_Macro defcolor_macro
#endif



//~ NOTE(fda0): Forward declarations
function Lister_Result run_lister_with_custom_render(Application_Links *app, Lister *lister, Render_Caller_Function *custom_render_caller);
function void fda0_lister_render(Application_Links *app, Frame_Info frame_info, View_ID view);



//~ NOTE(fda0): Helper functions
internal void
fda0_trim_string_to_single_spaces_in_place(String_Const_u8 *string)
{
    b32 was_space = true;
    u64 dest_index = 0;
    
    u64 original_size = string->size;
    for (u64 source_index = 0;
         source_index< original_size;
         ++source_index)
    {
        u8 c = string->str[source_index];
        
        if (c <= ' ')
        {
            if (was_space)
            {
                continue;
            }
            else
            {
                was_space = true;
                string->str[source_index] = ' ';
            }
        }
        else
        {
            was_space = false;
        }
        
        
        if (dest_index != source_index)
        {
            string->str[dest_index] = string->str[source_index];
        }
        
        
        ++dest_index;
    }
    
    if (string->str[dest_index-1] == ' ')
    {
        --dest_index;
    }
    
    string->size = dest_index;
}


enum Fda0_Coded_Description : u8
{
    Fda0_CodedDesc_None,
    Fda0_CodedDesc_Type,
    Fda0_CodedDesc_Macro,
    Fda0_CodedDesc_Function,
    Fda0_CodedDesc_Forward,
    
    Fda0_CodedDesc_Count
};

struct Fda0_Decode_Result
{
    String_Const_u8 string;
    Fda0_Coded_Description code;
};

inline Fda0_Decode_Result
fda0_decode_description(String_Const_u8 string)
{
    Fda0_Decode_Result result = {};
    result.string = string;
    
    if (string.size > 1 &&
        string.str[string.size-1] < Fda0_CodedDesc_Count)
    {
        result.code = (Fda0_Coded_Description)string.str[string.size-1];
        --result.string.size;
    }
    
    return result;
}

function String_Const_u8
fda0_push_buffer_range_plus_bonus_space(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 range,
                                        i64 bonus_spaces_count){
    String_Const_u8 result = {};
    i64 length = range_size(range);
    if (length > 0)
    {
        Temp_Memory restore_point = begin_temp(arena);
        u8 *memory = push_array(arena, u8, length + bonus_spaces_count);
        if (buffer_read_range(app, buffer, range, memory))
        {
            result = SCu8(memory, length + bonus_spaces_count);
        }
        else
        {
            end_temp(restore_point);
        }
    }
    return(result);
}



struct Fda0_Lister_Item
{
    Fda0_Lister_Item *next;
    
    String_Const_u8 primary;
    String_Const_u8 description;
    Tiny_Jump *jump;
    Fda0_Coded_Description code;
};

inline Fda0_Lister_Item *
fda0_push_lister_item(Arena *arena, Fda0_Lister_Item *tail,
                      String_Const_u8 primary, String_Const_u8 description, Tiny_Jump *jump, Fda0_Coded_Description code)
{
    Fda0_Lister_Item *new_item = push_array(arena, Fda0_Lister_Item, 1);
    if (tail)
    {
        tail->next = new_item;
    }
    
    new_item->next = nullptr;
    new_item->primary = primary;
    new_item->description = description;
    new_item->jump = jump;
    new_item->code = code;
    
    return new_item;
}


struct Fda0_Li_Split_Result
{
    Fda0_Lister_Item *a;
    Fda0_Lister_Item *b;
};

internal Fda0_Li_Split_Result
fda0_li_split(Fda0_Lister_Item* source)
{
    // NOTE(fda0): Taken and modified from: https://www.geeksforgeeks.org/merge-sort-for-linked-list/
    // NOTE(fda0): I have heavy suspicion that tracking lengths/indexes would be faster
    
    Fda0_Lister_Item* fast = source->next;
    Fda0_Lister_Item* slow = source;
    /* Advance 'fast' two nodes, and advance 'slow' one node */
    while (fast != nullptr)
    {
        fast = fast->next;
        if (fast != nullptr)
        {
            slow = slow->next;
            fast = fast->next;
        }
    }
    
    /* 'slow' is before the midpoint in the list, so split it in two at that point. */
    Fda0_Li_Split_Result result = {
        source,
        slow->next
    };
    
    slow->next = nullptr;
    
    return result;
}


internal Fda0_Lister_Item *
fda0_li_sorted_merge(Fda0_Lister_Item *a, Fda0_Lister_Item *b)
{
    // NOTE(fda0): Taken and modified from: https://www.geeksforgeeks.org/merge-sort-for-linked-list/
    
    /* Base cases */
    if (a == nullptr) { return b; }
    else if (b == nullptr) { return a; }
    
    
    // NOTE(fda0): Sorting critera
    b32 a_goes_first = false;
    if (a->code == b->code)
    {
        if (a->primary.size == b->primary.size)
        {
            i32 primary_compare = string_compare(a->primary, b->primary);
            if (primary_compare == 0)
            {
                if (a->description.size < b->description.size) 
                { 
                    a_goes_first = true; 
                }
            }
            else if (primary_compare < 0) 
            { 
                a_goes_first = true; 
            }
        }
        else if (a->primary.size < b->primary.size)
        {
            a_goes_first = true;
        }
    }
    else if (a->code < b->code)
    {
        a_goes_first = true;
    }
    
    
    
    // NOTE(fda0): Further sorting
    /* Pick either a or b, and recur */
    Fda0_Lister_Item *result = nullptr;
    if (a_goes_first)
    {
        result = a;
        result->next = fda0_li_sorted_merge(a->next, b);
    }
    else
    {
        result = b;
        result->next = fda0_li_sorted_merge(a, b->next);
    }
    
    return result;
}


internal void
fda0_li_merge_sort(Fda0_Lister_Item **head_ptr)
{
    Fda0_Lister_Item *head = *head_ptr;
    if (head == nullptr || head->next == nullptr) { return; }
    
    
    Fda0_Li_Split_Result split = fda0_li_split(head);
    fda0_li_merge_sort(&split.a);
    fda0_li_merge_sort(&split.b);
    
    
    *head_ptr = fda0_li_sorted_merge(split.a, split.b);
}



//~ NOTE(fda0): The main dish
CUSTOM_UI_COMMAND_SIG(fda0_jump_to_definition)
CUSTOM_DOC("List, sort and preview all definitions in the code index and jump to one.")
{
    Scratch_Block scratch(app);
    
    String_Const_u8 str_type     = push_stringf(scratch, "type%c", Fda0_CodedDesc_Type);
    String_Const_u8 str_func     = push_stringf(scratch, "function%c", Fda0_CodedDesc_Function);
    String_Const_u8 str_func_dec = push_stringf(scratch, "declaration%c", Fda0_CodedDesc_Forward);
    String_Const_u8 str_macro    = push_stringf(scratch, "macro%c", Fda0_CodedDesc_Macro);
    
    Fda0_Lister_Item *head = nullptr;
    Fda0_Lister_Item *tail = nullptr;
    
    Lister_Block lister(app, scratch);
    char *query = "Definition:";
    lister_set_query(lister, query);
    lister_set_default_handlers(lister);
    code_index_lock();
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
         buffer != 0;
         buffer = get_buffer_next(app, buffer, Access_Always))
    {
        Token_Array token_array = get_token_array_from_buffer(app, buffer);
        
        Code_Index_File *file = code_index_get_file(buffer);
        if (file != 0)
        {
            for (i32 i = 0;
                 i < file->note_array.count;
                 i += 1)
            {
                Code_Index_Note *note = file->note_array.ptrs[i];
                Tiny_Jump *jump = push_array(scratch, Tiny_Jump, 1);
                jump->buffer = buffer;
                jump->pos = note->pos.first;
                
                
                String_Const_u8 description = {};
                Fda0_Coded_Description code = {};
                
                
                switch (note->note_kind)
                {
                    case CodeIndexNote_Type:
                    {
                        description = str_type;
                        code = Fda0_CodedDesc_Type;
                    } break;
                    
                    
                    case CodeIndexNote_Function:
                    {
                        code = Fda0_CodedDesc_Function;
                        String_Const_u8 arguments_string = {};
                        
                        if(token_array.tokens != 0)
                        {
                            Range_i64 function_range = note->pos;
                            Token_Iterator_Array it = token_iterator_pos(0, &token_array, function_range.min);
                            
                            for (;token_it_inc_non_whitespace(&it);)
                            {
                                Token *token = token_it_read(&it);
                                
                                if(token->kind == TokenBaseKind_ParentheticalOpen)
                                {
                                    function_range.min = token->pos;
                                }
                                else if (token->kind == TokenBaseKind_ParentheticalClose)
                                {
                                    function_range.max = token->pos + token->size;
                                    
                                    // NOTE(fda0): Allocate 3 bytes more for "{}" + code
                                    arguments_string = fda0_push_buffer_range_plus_bonus_space(app, scratch, buffer, function_range, 3);
                                    
                                    token_it_inc_non_whitespace(&it);
                                    token = token_it_read(&it);
                                    
                                    if (arguments_string.size)
                                    {
                                        if (token->kind == TokenBaseKind_ScopeOpen)
                                        {
                                            arguments_string.str[arguments_string.size - 3] = '{';
                                            arguments_string.str[arguments_string.size - 2] = '}';
                                        }
                                        else
                                        {
                                            code = Fda0_CodedDesc_Forward;
                                            arguments_string.str[arguments_string.size - 3] = ';';
                                            --arguments_string.size;
                                        }
                                        
                                        --arguments_string.size;
                                        fda0_trim_string_to_single_spaces_in_place(&arguments_string);
                                        ++arguments_string.size;
                                        arguments_string.str[arguments_string.size-1] = code;
                                        
                                        description = arguments_string; 
                                    }
                                    else
                                    {
                                        description = str_func; 
                                    }
                                    
                                    break;
                                }
                            }
                        }
                        
                    } break;
                    
                    
                    case CodeIndexNote_Macro:
                    {
                        description = str_macro;
                        code = Fda0_CodedDesc_Macro;
                    } break;
                }
                
                
                tail = fda0_push_lister_item(scratch, tail, note->text, description, jump, code);
                if (head == nullptr)
                {
                    head = tail;
                }
                
            }
        }
    }
    code_index_unlock();
    
    
    // NOTE(fda0): Merge sort
    fda0_li_merge_sort(&head);
    
    
    // NOTE(fda0): Add items to lister
    for (Fda0_Lister_Item *item = head;
         item;
         item = item->next)
    {
        if (item->code == Fda0_CodedDesc_Function ||
            item->code == Fda0_CodedDesc_Forward)
        {
            char code_char = (char)item->code == Fda0_CodedDesc_Function;
            
            item->primary = push_stringf(scratch, "%.*s %.*s",
                                         item->primary.size, (char *)item->primary.str, 
                                         item->description.size-1, (char *)item->description.str,
                                         code_char);
        }
        
        
        lister_add_item(lister, item->primary, item->description, item->jump, 0);
    }
    
    
    
    // NOTE(fda0): Run lister
    Lister_Result l_result = run_lister_with_custom_render(app, lister, fda0_lister_render);
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0)
    {
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    }
    
    if (result.buffer != 0)
    {
        View_ID view = get_this_ctx_view(app, Access_Always);
        point_stack_push_view_cursor(app, view);
        jump_to_location(app, view, result.buffer, result.pos);
    }
}






//~ NOTE(fda0): Custom lister render - modified at the end
// It decodes color info hidden in node->status
function void
fda0_lister_render(Application_Links *app, Frame_Info frame_info, View_ID view)
{
    Scratch_Block scratch(app);
    
    Lister *lister = view_get_lister(app, view);
    if (lister == 0){
        return;
    }
    
    Rect_f32 region = draw_background_and_margin(app, view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Face_ID face_id = get_face_id(app, 0);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 line_height = metrics.line_height;
    f32 block_height = lister_get_block_height(line_height);
    f32 text_field_height = lister_get_text_field_height(line_height);
    
    // NOTE(allen): file bar
    // TODO(allen): What's going on with 'showing_file_bar'? I found it like this.
    b64 showing_file_bar = false;
    b32 hide_file_bar_in_ui = def_get_config_b32(vars_save_string_lit("hide_file_bar_in_ui"));
    if (view_get_setting(app, view, ViewSetting_ShowFileBar, &showing_file_bar) &&
        showing_file_bar && !hide_file_bar_in_ui){
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        draw_file_bar(app, view, buffer, face_id, pair.min);
        region = pair.max;
    }
    
    Mouse_State mouse = get_mouse_state(app);
    Vec2_f32 m_p = V2f32(mouse.p);
    
    lister->visible_count = (i32)((rect_height(region)/block_height)) - 3;
    lister->visible_count = clamp_bot(1, lister->visible_count);
    
    
    Rect_f32 text_field_rect = {};
    Rect_f32 list_rect = {};
    {
        Rect_f32_Pair pair = lister_get_top_level_layout(region, text_field_height);
        text_field_rect = pair.min;
        list_rect = pair.max;
    }
    
    {
        Vec2_f32 p = V2f32(text_field_rect.x0 + 3.f, text_field_rect.y0);
        Fancy_Line text_field = {};
        push_fancy_string(scratch, &text_field, fcolor_id(defcolor_pop1),
                          lister->query.string);
        push_fancy_stringf(scratch, &text_field, " ");
        p = draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);
        
        // TODO(allen): This is a bit of a hack. Maybe an upgrade to fancy to focus
        // more on being good at this and less on overriding everything 10 ways to sunday
        // would be good.
        block_zero_struct(&text_field);
        push_fancy_string(scratch, &text_field, fcolor_id(defcolor_text_default),
                          lister->text_field.string);
        f32 width = get_fancy_line_width(app, face_id, &text_field);
        f32 cap_width = text_field_rect.x1 - p.x - 6.f;
        if (cap_width < width){
            Rect_f32 prect = draw_set_clip(app, Rf32(p.x, text_field_rect.y0, p.x + cap_width, text_field_rect.y1));
            p.x += cap_width - width;
            draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);
            draw_set_clip(app, prect);
        }
        else{
            draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);
        }
    }
    
    
    Range_f32 x = rect_range_x(list_rect);
    draw_set_clip(app, list_rect);
    
    // NOTE(allen): auto scroll to the item if the flag is set.
    f32 scroll_y = lister->scroll.position.y;
    
    if (lister->set_vertical_focus_to_item){
        lister->set_vertical_focus_to_item = false;
        Range_f32 item_y = If32_size(lister->item_index*block_height, block_height);
        f32 view_h = rect_height(list_rect);
        Range_f32 view_y = If32_size(scroll_y, view_h);
        if (view_y.min > item_y.min || item_y.max > view_y.max){
            f32 item_center = (item_y.min + item_y.max)*0.5f;
            f32 view_center = (view_y.min + view_y.max)*0.5f;
            f32 margin = view_h*.3f;
            margin = clamp_top(margin, block_height*3.f);
            if (item_center < view_center){
                lister->scroll.target.y = item_y.min - margin;
            }
            else{
                f32 target_bot = item_y.max + margin;
                lister->scroll.target.y = target_bot - view_h;
            }
        }
    }
    
    // NOTE(allen): clamp scroll target and position; smooth scroll rule
    i32 count = lister->filtered.count;
    Range_f32 scroll_range = If32(0.f, clamp_bot(0.f, count*block_height - block_height));
    lister->scroll.target.y = clamp_range(scroll_range, lister->scroll.target.y);
    lister->scroll.target.x = 0.f;
    
    Vec2_f32_Delta_Result delta = delta_apply(app, view,
                                              frame_info.animation_dt, lister->scroll);
    lister->scroll.position = delta.p;
    if (delta.still_animating){
        animate_in_n_milliseconds(app, 0);
    }
    
    lister->scroll.position.y = clamp_range(scroll_range, lister->scroll.position.y);
    lister->scroll.position.x = 0.f;
    
    scroll_y = lister->scroll.position.y;
    f32 y_pos = list_rect.y0 - scroll_y;
    
    i32 first_index = (i32)(scroll_y/block_height);
    y_pos += first_index*block_height;
    
    //
    // NOTE(fda0): Bugfix for default lister_render in 4coder 4.1.7
    //
    i32 max_count = first_index + lister->visible_count + 6;
    count = clamp_top(lister->filtered.count, max_count);
    //
    //
    //
    
    for (i32 i = first_index; i < count; i += 1){
        Lister_Node *node = lister->filtered.node_ptrs[i];
        
        Range_f32 y = If32(y_pos, y_pos + block_height);
        y_pos = y.max;
        
        Rect_f32 item_rect = Rf32(x, y);
        Rect_f32 item_inner = rect_inner(item_rect, 3.f);
        
        b32 hovered = rect_contains_point(item_rect, m_p);
        UI_Highlight_Level highlight = UIHighlight_None;
        if (node == lister->highlighted_node){
            highlight = UIHighlight_Active;
        }
        else if (node->user_data == lister->hot_user_data){
            if (hovered){
                highlight = UIHighlight_Active;
            }
            else{
                highlight = UIHighlight_Hover;
            }
        }
        else if (hovered){
            highlight = UIHighlight_Hover;
        }
        
        u64 lister_roundness_100 = def_get_config_u64(app, vars_save_string_lit("lister_roundness"));
        f32 roundness = block_height*lister_roundness_100*0.01f;
        draw_rectangle_fcolor(app, item_rect, roundness, get_item_margin_color(highlight));
        draw_rectangle_fcolor(app, item_inner, roundness, get_item_margin_color(highlight, 1));
        
        Fancy_Line line = {};
        
        // NOTE(fda0): Modified part - custom colors and description decoding
        FColor color_primary = fcolor_id(defcolor_text_default);
        FColor color_secondary = fcolor_id(defcolor_pop2);
        
        Fda0_Decode_Result decode = fda0_decode_description(node->status);
        switch (decode.code)
        {
            case Fda0_CodedDesc_Type: {
                color_primary = fcolor_id(Fda0_Color_Type);
            } break;
            
            case Fda0_CodedDesc_Function: {
                color_primary = fcolor_id(Fda0_Color_Function);
                color_secondary = fcolor_id(defcolor_text_default);
            } break;
            
            case Fda0_CodedDesc_Forward: {
                color_primary = fcolor_id(Fda0_Color_Fordward_Declaration);
                color_secondary = fcolor_id(defcolor_comment);
            } break;
            
            case Fda0_CodedDesc_Macro: {
                color_primary = fcolor_id(Fda0_Color_Macro);
            } break;
        }
        
        
        String_Const_u8 primary = node->string;
        
        if (decode.code == Fda0_CodedDesc_Function ||
            decode.code == Fda0_CodedDesc_Forward)
        {
            i64 split_pos = (i64)string_find_first(node->string, 0, ' ');
            primary.size = split_pos;
        }
        
        
        push_fancy_string(scratch, &line, color_primary, primary);
        push_fancy_stringf(scratch, &line, " ");
        push_fancy_string(scratch, &line, color_secondary, decode.string);
        
        Vec2_f32 p = item_inner.p0 + V2f32(3.f, (block_height - line_height)*0.5f);
        draw_fancy_line(app, face_id, fcolor_zero(), &line, p);
    }
    
    draw_set_clip(app, prev_clip);
}





//~ NOTE(fda0): Barely modified version of default run_lister that allows to specify custom render_caller function
function Lister_Result
run_lister_with_custom_render(Application_Links *app, Lister *lister, Render_Caller_Function *custom_render_caller)
{
    lister->filter_restore_point = begin_temp(lister->arena);
    lister_update_filtered_list(app, lister);
    
    View_ID view = get_this_ctx_view(app, Access_Always);
    View_Context ctx = view_current_context(app, view);
    ctx.render_caller = custom_render_caller;
    ctx.hides_buffer = true;
    View_Context_Block ctx_block(app, view, &ctx);
    
    for (;;){
        User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort){
            block_zero_struct(&lister->out);
            lister->out.canceled = true;
            break;
        }
        
        Lister_Activation_Code result = ListerActivation_Continue;
        b32 handled = true;
        switch (in.event.kind){
            case InputEventKind_TextInsert:
            {
                if (lister->handlers.write_character != 0){
                    result = lister->handlers.write_character(app);
                }
            }break;
            
            case InputEventKind_KeyStroke:
            {
                switch (in.event.key.code){
                    case KeyCode_Return:
                    case KeyCode_Tab:
                    {
                        void *user_data = 0;
                        if (0 <= lister->raw_item_index &&
                            lister->raw_item_index < lister->options.count){
                            user_data = lister_get_user_data(lister, lister->raw_item_index);
                        }
                        lister_activate(app, lister, user_data, false);
                        result = ListerActivation_Finished;
                    }break;
                    
                    case KeyCode_Backspace:
                    {
                        if (lister->handlers.backspace != 0){
                            lister->handlers.backspace(app);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_Up:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister, -1);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_Down:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister, 1);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_PageUp:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister,
                                                      -lister->visible_count);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_PageDown:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister,
                                                      lister->visible_count);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    default:
                    {
                        if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                }
            }break;
            
            case InputEventKind_MouseButton:
            {
                switch (in.event.mouse.code){
                    case MouseCode_Left:
                    {
                        Vec2_f32 p = V2f32(in.event.mouse.p);
                        void *clicked = lister_user_data_at_p(app, view, lister, p);
                        lister->hot_user_data = clicked;
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            case InputEventKind_MouseButtonRelease:
            {
                switch (in.event.mouse.code){
                    case MouseCode_Left:
                    {
                        if (lister->hot_user_data != 0){
                            Vec2_f32 p = V2f32(in.event.mouse.p);
                            void *clicked = lister_user_data_at_p(app, view, lister, p);
                            if (lister->hot_user_data == clicked){
                                lister_activate(app, lister, clicked, true);
                                result = ListerActivation_Finished;
                            }
                        }
                        lister->hot_user_data = 0;
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            case InputEventKind_MouseWheel:
            {
                Mouse_State mouse = get_mouse_state(app);
                lister->scroll.target.y += mouse.wheel;
                lister_update_filtered_list(app, lister);
            }break;
            
            case InputEventKind_MouseMove:
            {
                lister_update_filtered_list(app, lister);
            }break;
            
            case InputEventKind_Core:
            {
                switch (in.event.core.code){
                    case CoreCode_Animate:
                    {
                        lister_update_filtered_list(app, lister);
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            default:
            {
                handled = false;
            }break;
        }
        
        if (result == ListerActivation_Finished){
            break;
        }
        
        if (!handled){
            Mapping *mapping = lister->mapping;
            Command_Map *map = lister->map;
            
            Fallback_Dispatch_Result disp_result =
                fallback_command_dispatch(app, mapping, map, &in);
            if (disp_result.code == FallbackDispatch_DelayedUICall){
                call_after_ctx_shutdown(app, view, disp_result.func);
                break;
            }
            if (disp_result.code == FallbackDispatch_Unhandled){
                leave_current_input_unhandled(app);
            }
            else{
                lister_call_refresh_handler(app, lister);
            }
        }
    }
    
    return(lister->out);
}





