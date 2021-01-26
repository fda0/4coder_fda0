
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

inline u64 
fda0_get_hash(String_Const_u8 string)
{
    u64 hash = fda0_get_hash(string.str, string.size);
    
    return hash;
}
