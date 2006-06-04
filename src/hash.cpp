static unsigned long hash_sdbm(unsigned char *str)
{
    unsigned long hash = 0;
    int c;

	while (c = *str++) {
        hash = c + (hash << 6) + (hash << 16) - hash;
	}

    return hash;
}
