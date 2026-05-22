int csv_insert_field(CSV_BUFFER *buffer, size_t row, size_t entry,
                char *field)
{
        /* If the field does not exist, simply set it */
        if (row > buffer->rows - 1
                || entry > buffer->width[row] - 1)
                csv_set_field(buffer, row, entry, field);

        /* Otherwise move everything over, then set it */
        else {
                append_field(buffer, row);
                int i = 0;
                for (i = buffer->width[row] - 1; i > entry; i--)
                        csv_copy_field(buffer, row, i,
                                        buffer, row, i-1);
                csv_set_field(buffer,row,entry,field);
        }

        return 0;
}