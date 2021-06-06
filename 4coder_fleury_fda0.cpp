

// TODO(fda0): Delete/rename it in the future?
CUSTOM_ID(colors, defcolor_type);
CUSTOM_ID(colors, defcolor_function);
CUSTOM_ID(colors, defcolor_macro);
CUSTOM_ID(colors, defcolor_declaration);
CUSTOM_ID(colors, f0_compilation_backgroud);


//#include "4fda0_utilities.cpp"
#include "4fda0_commands.cpp"
#include "4fda0_definitions_lister.cpp"


function void
fda0_initialize_stuff(Application_Links *app)
{
    casey_switch_to_keybinding_0(app);

    time_t timestamp; time(&timestamp);
    tm *date = gmtime(&timestamp);
    
    
    Scratch_Block scratch(app);
    String_Const_u8 theme_name = def_get_config_string(scratch, vars_save_string_lit("default_theme_name"));
    
    if (date->tm_hour < 4 || date->tm_hour >= 23) {
        theme_name = S8Lit("theme-fleury");
    } else if (date->tm_hour < 7 || date->tm_hour > 21) {
        theme_name = S8Lit("theme-edge-night-sky");
    } else {
        theme_name = S8Lit("theme-edge-serene");
    }
    
    Color_Table *colors = get_color_table_by_name(theme_name);
    set_active_color(colors);
    
    String_Const_u8 theme_text = push_stringf(scratch, "Hour: %d, Theme: ", (int)date->tm_hour);
    print_message(app, theme_text);
    print_message(app, theme_name);
    print_message(app, S8Lit("\n"));
}


#define Fda0_Insert_At_Startup() fda0_initialize_stuff(app)
