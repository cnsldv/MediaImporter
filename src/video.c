
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>

#include "sqlite3.h"
#include "mp4utils.h"
#include "debugScreen.h"

#define VIDEO_DB "ux0:mms/video/AVContent.db"

static const char *select_content_count_sql = "SELECT COUNT(*) FROM tbl_VPContent WHERE content_path=?";
static const char *select_content_sql = "SELECT mrid, content_path, title FROM tbl_VPContent";
static const char *delete_content_sql = "DELETE FROM tbl_VPContent WHERE mrid=?";
static const char *delete_content2_sql = "DELETE FROM tbl_Video WHERE base_id=?";
static const char *delete_all_sql = "DELETE FROM tbl_VPContent";
static const char *delete_all2_sql = "DELETE FROM tbl_Video";
static const char *insert_video_sql = "INSERT INTO tbl_VPContent (\
content_type, \
duration, \
size, \
status, \
created_time, \
last_updated_time, \
title, \
container_type, \
content_path, \
content_path_ext, \
created_time_for_sort) \
VALUES (5,?,?,2,datetime('now'),datetime('now'),?,4,?,?,datetime('now'))";
static const char *insert_video2_sql = "INSERT INTO tbl_Video (base_id) VALUES (LAST_INSERT_ROWID())";
static const char *select_empty_icons_sql = "SELECT content_path,title FROM tbl_VPContent \
WHERE ifnull(icon_path, '') = ''";
static const char *update_icon_path_sql = "UPDATE tbl_VPContent SET icon_path=?,icon_codec_type=17 \
WHERE content_path=?";


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

static int sql_insert_video(sqlite3 *db, const char *path, size_t size, uint32_t duration)
{
	int ret = 0;
	sqlite3_stmt *stmt;
	const char *sql = insert_video_sql;

	const char *title, *ext;
	int path_len, title_len, ext_len, pos;

	pos = 0;
	title = ext = path;
	while (path[pos]) {
		switch(path[pos]) {
		case '/':
			title = ext = path + pos + 1;
			break;

		case '.':
			ext = path + pos + 1;
			break;

		default:
			break;
		}
		pos++;
	}

	path_len = pos;
	title_len = ext - title - 1;
	ext_len = path + path_len - ext;

	if (path_len < 0 || title_len < 0 || ext_len < 0) {
		goto fail;
	}

	ret = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_int(stmt, 1, duration);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_int64(stmt, 2, size);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_text(stmt, 3, title, title_len, NULL);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_text(stmt, 4, path, path_len, NULL);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_text(stmt, 5, ext, ext_len, NULL);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	sqlite3_stmt *stmt2;
	const char *sql2 = insert_video2_sql;
	ret = sqlite3_prepare_v2(db, sql2, -1, &stmt2, 0);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql2, sqlite3_errmsg(db));
		goto fail;
	}

	sqlite3_step(stmt2);
	sqlite3_finalize(stmt2);

fail:
	return ret;
}

static int sql_delete_video(sqlite3 *db, int64_t mrid)
{
	int ret = 0;
	sqlite3_stmt *stmt, *stmt2;
	const char *sql = delete_content_sql;
	const char *sql2 = delete_content2_sql;

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

	ret = sqlite3_prepare_v2(db, sql2, -1, &stmt2, 0);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	ret = sqlite3_bind_int64(stmt2, 1, mrid);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}

	sqlite3_step(stmt2);
	sqlite3_finalize(stmt2);

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

static int add_videos_int(sqlite3 *db, const char *dir, int added)
{
	SceUID did;
	SceIoDirent dinfo;
	int err = 0;
	char *new_path = NULL;

	did = sceIoDopen(dir);
	if (did < 0) {
		return 0;
	}

	err = sceIoDread(did, &dinfo);
	while (err > 0) {
		new_path = concat_path(dir, dinfo.d_name);
		if (SCE_S_ISDIR(dinfo.d_stat.st_mode)) {
			// recursion, ewww
			added = add_videos_int(db, new_path, added);
		}
		else {
			int l = strlen(dinfo.d_name);
			if (l > 4 && dinfo.d_name[0] != '.' &&
				strcmp(dinfo.d_name + l - 4, ".mp4") == 0) {

				int c = sql_get_count(db, select_content_count_sql, new_path);
				if (c == 0) {
					uint32_t dur = get_mp4_duration(new_path);
					sql_insert_video(db, new_path, dinfo.d_stat.st_size, dur);
					added++;
					printf("Added %d videos\r", added);
				}
			}
		}
		free(new_path);
		err = sceIoDread(did, &dinfo);
	} 

	sceIoDclose(did);

	return added;
}

void add_videos(const char *dir)
{
	int added;
	sqlite3 *db;
	int ret = sqlite3_open(VIDEO_DB, &db);
	if (ret) {
		printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
		goto fail;
	}

	sqlite3_exec(db, "BEGIN", 0, 0, 0);
	added = add_videos_int(db, dir, 0);
	sqlite3_exec(db, "COMMIT", 0, 0, 0);
	printf("Added %d videos\n", added);
fail:
	sqlite3_close(db);
}

static char *find_icon(const char* content_path)
{
	char *icon_path = NULL;
	char *ext = NULL;
	int len = 0;
	int e = 0;

	const char *exts[] = {"THM", "thm", "JPG", "jpg", NULL};

	icon_path = strdup(content_path);
	if (!icon_path) {
		goto fail;
	}
	len = strlen(icon_path);
	if (len < 4) {
		goto fail;
	}

	ext = icon_path + len - 3;

	e = 0;
	while(exts[e]) {
		strcpy(ext, exts[e]);

		FILE *testfile = fopen(icon_path, "rb");
		if (testfile) {
			fclose(testfile);
			return icon_path;
		}
		fclose(testfile);
		e++;
	}

fail:
	if (icon_path) {
		free(icon_path);
	}

	return NULL;
}

void update_video_icons(void)
{
	sqlite3 *db;
	int ret = sqlite3_open(VIDEO_DB, &db);
	if (ret) {
		printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
		goto fail;
	}

	sqlite3_exec(db, "BEGIN", 0, 0, 0);
	sqlite3_stmt *stmt;
	const char *sql = select_empty_icons_sql;
	ret = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
		goto fail;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const char *content_path = sqlite3_column_text(stmt, 0);
		const char *title = sqlite3_column_text(stmt, 1);
		char *icon_path = find_icon(content_path);
		if (icon_path) {
			sqlite3_stmt *stmt2;
			const char *sql = update_icon_path_sql;
			ret = sqlite3_prepare_v2(db, sql, -1, &stmt2, 0);
			if (ret != SQLITE_OK) {
				printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
				goto fail;
			}

			ret = sqlite3_bind_text(stmt2, 1, icon_path, strlen(icon_path), NULL);
			if (ret != SQLITE_OK) {
				printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
				goto fail;
			}

			ret = sqlite3_bind_text(stmt2, 2, content_path, strlen(content_path), NULL);
			if (ret != SQLITE_OK) {
				printf("Failed to execute %s, error %s\n", sql, sqlite3_errmsg(db));
				goto fail;
			}
			sqlite3_step(stmt2);
			sqlite3_finalize(stmt2);
		}
	}
fail:
	sqlite3_exec(db, "COMMIT", 0, 0, 0);
	sqlite3_close(db);
}

void clean_videos(void)
{
	int removed = 0;
	sqlite3 *db;
	int ret = sqlite3_open(VIDEO_DB, &db);
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
		int64_t mrid = sqlite3_column_int(stmt, 0);
		const char *path = sqlite3_column_text(stmt, 1);
		const char *title = sqlite3_column_text(stmt, 2);

		FILE *testfile = fopen(path, "rb");
		if (!testfile) {
			sql_delete_video(db, mrid);
			removed++;
			printf("Removed %d videos\r", removed);
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

	printf("Removed %d videos\n", removed);
}

void empty_videos(void)
{
	sqlite3 *db;
	int ret = sqlite3_open(VIDEO_DB, &db);
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

	sqlite3_stmt *stmt2;
	const char *sql2 = delete_all2_sql;
	ret = sqlite3_prepare_v2(db, sql2, -1, &stmt2, 0);
	if (ret != SQLITE_OK) {
		printf("Failed to execute %s, error %s\n", sql2, sqlite3_errmsg(db));
		goto fail;
	}
	sqlite3_step(stmt2);

fail:
	if (stmt) {
		sqlite3_finalize(stmt);
	}
	if (stmt2) {
		sqlite3_finalize(stmt2);
	}
	sqlite3_close(db);
}

