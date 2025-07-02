void copy_block_to_file(uint32_t block_num, FILE *dest_file, unsigned int *bytes_remaining, char *block_buf) {
    if (*bytes_remaining == 0) return;
    read_block(block_num, block_buf);
    unsigned int bytes_to_write = (*bytes_remaining < block_size) ? *bytes_remaining : block_size;
    fwrite(block_buf, 1, bytes_to_write, dest_file);
    *bytes_remaining -= bytes_to_write;
}