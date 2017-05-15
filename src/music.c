
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>

#include "sqlite3.h"
#include "mp4utils.h"
#include "debugScreen.h"
#include "id3.h"

#define MUSIC_DB "ux0:mms/music/AVContent.db"

static const char *select_content_count_sql = "SELECT COUNT(*) FROM tbl_Music WHERE content_path=?";
static const char *select_content_sql = "SELECT mrid, content_path, title FROM tbl_Music";
static const char *delete_content_sql = "DELETE FROM tbl_Music WHERE mrid=?";
static const char *delete_all_sql = "DELETE FROM tbl_Music";
static const char *refresh_db_sql = "UPDATE tbl_config SET val=0;";

static const char *insert_music_sql = "INSERT INTO tbl_Music (\
codec_type, \
track_num, \
disc_num, \
size, \
container_type, \
status, \
analyzed, \
icon_data_type, \
artist, \
title, \
created_time, \
last_updated_time, \
imported_time, \
content_path, \
album_artist, \
album_name) \
VALUES (12,?,1,?,7,2,1,-1,?,?,datetime('now'),datetime('now'),datetime('now'),?,?,?)";

static int sql_get_count(sqlite3 *db, const char *sql, const char* fname) {
	int cnt = 0;
	int ret = 0;
	sqlite3_stmt *stmt;
	ret = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}
	ret = sqlite3_bind_text(stmt, 1, fname, strlen(fname), NULL);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}
	if (sqlite3_step(stmt) == SQLITE_ROW)
		cnt = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);

	return cnt;
fail:
	sqlite3_close(db);
	return 0;
}

static int sql_insert_music(sqlite3 *db, const char *path, size_t size)
{
	int ret = 0;
	sqlite3_stmt *stmt;
	struct ID3Tag tag;
	const char *sql = insert_music_sql;

	char *title = "Unknown";
	char *artist = "Unknown";
	char *album = "Unknown";

	if (!path) {
		goto fail;
	}

	memset(&tag, 0, sizeof(struct ID3Tag));
	ParseID3(path, &tag);

	if (strlen(tag.ID3Title) > 0) {
		title = tag.ID3Title;
	}
	if (strlen(tag.ID3Artist) > 0) {
		artist = tag.ID3Artist;
	}
	if (strlen(tag.ID3Album) > 0) {
		album = tag.ID3Album;
	}

	ret = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_int(stmt, 1, tag.ID3Track);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_int(stmt, 2, size);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_text(stmt, 3, artist, strlen(artist), NULL);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_text(stmt, 4, title, strlen(title), NULL);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_text(stmt, 5, path, strlen(path), NULL);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_text(stmt, 6, artist, strlen(artist), NULL);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_text(stmt, 7, album, strlen(album), NULL);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

fail:
	return ret;
}

static int sql_delete_music(sqlite3 *db, int64_t mrid)
{
	int ret = 0;
	sqlite3_stmt *stmt, *stmt2;
	const char *sql = delete_content_sql;

	ret = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_int64(stmt, 1, mrid);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

fail:
	return ret;
}

static char *concat_path(const char *parent, const char *child) {
	int len;
	char *new_path = NULL;

	len = strlen(parent) + strlen(child) + 2;
	new_path = malloc(len);
	if (new_path) {
		strcpy(new_path, parent);
		strcat(new_path, "/");
		strcat(new_path, child);
	}

	return new_path;
}

static int add_music_int(sqlite3 *db, const char *dir, int added)
{
	SceUID did;
	SceIoDirent dinfo;
	SceIoStat stat;
	int err = 0;
	char *new_path = NULL;

	sceIoGetstat(dir, &stat);

	did = sceIoDopen(dir);
	if (did < 0) {
		printf("dir %s not openable %x\n", dir, did);
		return 0;
	}

	err = sceIoDread(did, &dinfo);
	while (err > 0) {
		new_path = concat_path(dir, dinfo.d_name);
		if (SCE_S_ISDIR(dinfo.d_stat.st_mode)) {
			// recursion, ewww
			added = add_music_int(db, new_path, added);
		}
		else {
			int l = strlen(dinfo.d_name);
			if (l > 4 && dinfo.d_name[0] != '.' &&
				strcmp(dinfo.d_name + l - 4, ".mp3") == 0) {

				int c = sql_get_count(db, select_content_count_sql, new_path);
				if (c == 0) {
					sql_insert_music(db, new_path, dinfo.d_stat.st_size);
					added++;
					printf("Added %d tracks\r", added);
				}
			}
		}
		free(new_path);
		err = sceIoDread(did, &dinfo);
	} 

	sceIoDclose(did);

	return added;
}

int add_music(const char *dir)
{
	int added;
	sqlite3 *db;
	int ret = sqlite3_open(MUSIC_DB, &db);
	if (ret) {
		printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
		goto fail;
	}

	sqlite3_exec(db, "BEGIN", 0, 0, 0);
	added = add_music_int(db, dir, 0);
	printf("Added %d tracks\n", added);
fail:
	sqlite3_exec(db, "COMMIT", 0, 0, 0);
	sqlite3_close(db);
	return added;
}

int clean_music(void)
{
	int removed = 0;
	sqlite3 *db;
	int ret = sqlite3_open(MUSIC_DB, &db);
	if (ret) {
		printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
		goto fail;
	}

	sqlite3_exec(db, "BEGIN", 0, 0, 0);
	sqlite3_stmt *stmt;
	const char *sql = select_content_sql;
	ret = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int64_t mrid = sqlite3_column_int64(stmt, 0);
		const char *path = sqlite3_column_text(stmt, 1);
		const char *title = sqlite3_column_text(stmt, 2);

		FILE *testfile = fopen(path, "rb");
		if (!testfile) {
			sql_delete_music(db, mrid);
			removed++;
			printf("Removed %d tracks\r", removed);
		}
		else {
			fclose(testfile);
		}
	}
fail:
	if (stmt) {
		sqlite3_finalize(stmt);
	}
	sqlite3_exec(db, "COMMIT", 0, 0, 0);
	sqlite3_close(db);

	printf("Removed %d tracks\n", removed);
	return removed;
}

void empty_music(void)
{
	sqlite3 *db;
	int ret = sqlite3_open(MUSIC_DB, &db);
	if (ret) {
		printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
		goto fail;
	}

	sqlite3_stmt *stmt;
	const char *sql = delete_all_sql;
	ret = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}
	sqlite3_step(stmt);

fail:
	if (stmt) {
		sqlite3_finalize(stmt);
	}
	sqlite3_close(db);
}

void refresh_music_db(void)
{
	sqlite3 *db;
	int ret = sqlite3_open(MUSIC_DB, &db);
	if (ret) {
		printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
		goto fail;
	}

	sqlite3_stmt *stmt;
	const char *sql = refresh_db_sql;
	ret = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

fail:
	sqlite3_close(db);
}

