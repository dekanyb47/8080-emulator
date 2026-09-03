int read_file_to_buf(const char *filepath, uint8_t ** const buf, long *bufsize);
int disassemble_8080_op(const uint8_t *buf, const int pc);
int disassemble_8080_code(const char *filepath);