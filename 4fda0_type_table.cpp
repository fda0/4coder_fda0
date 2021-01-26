

//~ NOTE(mg): Holy cow! I would purge this place so hard but 4coder will get
// fast type coloring soon... So I will save myself the effort of refactoring this crap.

inline u64 
fda0_get_hash(u8 *str, u64 size)
{
    u64 hash = 0;
    
    for (u64 byte_index = 0;
         byte_index < size;
         ++byte_index)
    {
        u8 b = str[byte_index];
        
        // NOTE(mg): djb2 hash. Get better hash function?
        hash = ((hash << 5) + hash) + b;
    }
    
    return hash;
}

#define MZGI_GET_HASH_STR_LIT(STR) fda0_get_hash((u8 *)STR, sizeof(STR)-1)

inline u64 
fda0_get_hash(String_Const_u8 string)
{
    u64 hash = fda0_get_hash(string.str, string.size);
    
    return hash;
}


#define MZGI_GET_KEY(Hash) (Hash & (ArrayCount(fda0_token_hash_table) - 1))

internal void
fda0_insert_hash(u64 hash, Identifier_Type type)
{
    u64 key = MZGI_GET_KEY(hash);
    
    auto entry = fda0_token_hash_table + key;
    if (!entry->type)
    {
        entry->hash = hash;
        entry->type = type;
    }
    else
    {
        for (;
             ;
             entry = entry->next)
        {
            if (entry->hash == hash)
            {
                if (entry->type > type)
                {
                    entry->type = type;
                }
                break;
            }
            else if (!entry->next)
            {
                if (fda0_token_collision_count < ArrayCount(fda0_token_collision_table))
                {
                    entry->next = fda0_token_collision_table + fda0_token_collision_count++;
                    if (entry->next)
                    {
                        entry->next->hash = hash;
                        entry->next->type = type;
                        entry->next->next = NULL;
                        
                        ++fda0_token_collision_count;
                    }
                }
                break;
            }
        }
    }
}

internal void
fda0_insert_hash(String_Const_u8 string, Identifier_Type type)
{
    u64 hash = fda0_get_hash(string);
    fda0_insert_hash(hash, type);
}

static void
fda0_rebuild_hash_table(Application_Links *app, Text_Layout_ID text_layout_id)
{
    memset(&fda0_token_hash_table, 0, sizeof(fda0_token_hash_table));
    fda0_token_collision_count = 0;
    
    // NOTE(mg): Custom types
#define MZGI_ADD(StrLit, Type) fda0_insert_hash(MZGI_GET_HASH_STR_LIT(StrLit), Type)
    
    MZGI_ADD("auto", IdentifierType_Keyword);
    MZGI_ADD("NULL", IdentifierType_Const);
    
    
    
    // NOTE(mg): Scanning all buffers
    
    for (Buffer_ID buffer_it = get_buffer_next(app, 0, Access_Always);
         buffer_it != 0;
         buffer_it = get_buffer_next(app, buffer_it, Access_Always))
    {
        Code_Index_File* file = code_index_get_file(buffer_it);
        if (file != 0)
        {
            for (i32 i = 0; i < file->note_array.count; i += 1)
            {
                Code_Index_Note* note = file->note_array.ptrs[i];
                
                if (note)
                {
                    switch (note->note_kind)
                    {
                        case CodeIndexNote_Type: {
                            fda0_insert_hash(note->text, IdentifierType_Type);
                        } break;
                        
                        case CodeIndexNote_Function: {
                            fda0_insert_hash(note->text, IdentifierType_Function);
                        } break;
                        
                        case CodeIndexNote_Macro: {
                            fda0_insert_hash(note->text, IdentifierType_Macro);
                        } break;
                    }
                }
            }
        }
    }
}


internal Identifier_Type
fda0_get_type(u64 hash)
{
    Identifier_Type result = {};
    
    u64 key = MZGI_GET_KEY(hash);
    
    for (auto entry = fda0_token_hash_table + key;
         entry;
         entry = entry->next)
    {
        if (entry->hash == hash)
        {
            result = entry->type;
            break;
        }
    }
    
    return result;
}


internal Identifier_Type
fda0_get_type(String_Const_u8 string)
{
    u64 hash = fda0_get_hash(string);
    Identifier_Type result = fda0_get_type(hash);
    return result;
}
