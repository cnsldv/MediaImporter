
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>


inline uint32_t read_uint_be(const uint8_t *buf)
{
	return  buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
}

static int find_mp4_duration(const uint8_t *buf, size_t bufsz, uint32_t *dur, uint32_t *scale)
{
	size_t blklen = 32;
	size_t pos = 0;
	uint8_t ver = 0;

	for (pos = 0; pos < bufsz - blklen; pos++) {
		if (buf[pos] == 'm' && buf[pos + 1] == 'd' && buf[pos + 2] == 'h' && buf[pos + 3] == 'd') {
			ver = buf[5];
			pos += 16;
			if (ver == 1) {
				pos += 8;
			}

			*scale = read_uint_be(buf + pos);
			if (*scale) {
				*dur = read_uint_be(buf + pos + 4);
				return 1;
			}
			else {
				*scale = 1;
				return 0;
			}
		}
	}
	return 0;
}

uint32_t get_mp4_duration(const char *filename)
{
	uint32_t dur = 0;
	uint32_t scale = 0;
	char *buf = NULL;
	size_t bread;

	buf = (char *)malloc(4096);
	if (!buf) {
		goto out;
	}

	FILE *f = fopen(filename, "rb");
	if (!f) {
		goto out;
	}

	bread = fread(buf, 1, 4096, f);
	if (find_mp4_duration(buf, 4096, &dur, &scale)) {
		goto out;
	}

	fseek(f, -4096, SEEK_END);
	bread = fread(buf, 1, 4096, f);
	if (find_mp4_duration(buf, 4096, &dur, &scale)) {
		goto out;
	}

out:
	if (f) {
		fclose(f);
	}

	if (buf) {
		free(buf);
	}

	if (scale) {
		return (dur / scale) * 1000;
	}
	else {
		return 0;
	}
}

