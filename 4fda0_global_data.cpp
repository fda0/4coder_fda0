
CUSTOM_ID(colors, defcolor_type);
CUSTOM_ID(colors, defcolor_function);
CUSTOM_ID(colors, defcolor_macro);
CUSTOM_ID(colors, defcolor_4coder_command);

CUSTOM_ID(colors, defcolor_declaration);
CUSTOM_ID(colors, defcolor_hover_background);
CUSTOM_ID(colors, defcolor_brace_highlight);
//CUSTOM_ID(colors, defcolor_username);


// NOTE(fda0): Global variables
static String_ID fda0_code_map_id;
static String_ID fda0_command_map_id;
Face_ID global_small_code_face;
static b32 fda0_command_mode;
static b32 fda0_function_helper;
static u64 fda0_current_hover_hash;