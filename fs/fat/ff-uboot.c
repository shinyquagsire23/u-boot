/*
 * Copyright (C) 2015 Stephen Warren <swarren at wwwdotorg.org>
 * (GPL-2.0+)
 *
 * Small portions taken from Barebox v2015.07.0
 * Copyright (c) 2007 Sascha Hauer <s.hauer at pengutronix.de>, Pengutronix
 * (GPL-2.0)
 *
 * SPDX-License-Identifier:     GPL-2.0+ GPL-2.0
 */

#include <common.h>
#include <fat.h>
#include <linux/ctype.h>
#include <memalign.h>
#include <part.h>
#include "diskio.h"
#include "ff.h"

static struct blk_desc *fat_dev;
static disk_partition_t *fat_part;
static FATFS fat_ff_fs;

/* Functions called by ff.c */

DSTATUS disk_initialize(BYTE pdrv)
{
	return 0;
}

DSTATUS disk_status(BYTE pdrv)
{
	return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
	int ret;
	void *temp;

	if (!fat_dev) return -1;
	debug("%s(sector=%d, count=%u)\n", __func__, sector, count);

	temp = malloc_cache_aligned(count * fat_dev->blksz);

	ret = blk_dread(fat_dev, fat_part->start + sector, count, temp);
	if (ret != count) {
		free(temp);
		return RES_ERROR;
	}

	memcpy(buff, temp, count * fat_dev->blksz);

	free(temp);
	return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
	int ret;
	void *temp;

	if (!fat_dev) return -1;
	debug("%s(sector=%d, count=%u)\n", __func__, sector, count);

	temp = malloc_cache_aligned(count * fat_dev->blksz);
	memcpy(temp, buff, count * fat_dev->blksz);

	ret = blk_dwrite(fat_dev, fat_part->start + sector, count, temp);
	free(temp);

	if (ret != count)
		return RES_ERROR;

	return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
	debug("%s(cmd=%d)\n", __func__, (int)cmd);

	switch (cmd) {
	case CTRL_SYNC:
		return RES_OK;
	default:
		return RES_ERROR;
	}
}

/* From Barebox v2015.07.0 fs/fat/fat.c */
WCHAR ff_convert(WCHAR src, UINT dir)
{
	if (src <= 0x80)
		return src;
	else
		return '?';
}

/* Functions that call into ff.c */

int fat_set_blk_dev(struct blk_desc *dev, disk_partition_t *part)
{
	FRESULT ff_ret;

	debug("%s()\n", __func__);

	fat_dev = dev;
	fat_part = part;

	ff_ret = f_mount(&fat_ff_fs, "0:", 1);
	if (ff_ret != FR_OK) {
		debug("f_mount() failed: %d\n", ff_ret);
		fat_dev = NULL;
		fat_part = NULL;
		return -1;
	}

	debug("f_mount() succeeded\n");
	return 0;
}

int file_fat_detectfs(void)
{
	FRESULT ff_ret;
	TCHAR label[12];
	DWORD vsn;

	debug("%s()\n", __func__);

	if (!fat_dev) {
		printf("No current device\n");
		return 1;
	}

#ifdef HAVE_BLOCK_DEVICE
	printf("  Device %d: ", fat_dev->dev);
	dev_print(fat_dev);
#endif

	ff_ret = f_getlabel("0:", label, &vsn);
	if (ff_ret != FR_OK) {
		debug("f_getlabel() failed: %d\n", ff_ret);
		return -1;
	}

	printf("Filesystem:    ");
	switch (fat_ff_fs.fs_type) {
	case FS_FAT12:
		puts("FAT12\n");
		break;
	case FS_FAT16:
		puts("FAT16\n");
		break;
	case FS_FAT32:
		puts("FAT32\n");
		break;
	case FS_EXFAT:
		puts("EXFAT\n");
		break;
	default:
		puts("<<unknown>>\n");
		break;
	}

	printf("Volume label:  ");
	if (!label[0]) {
		puts("<<no label>>\n");
	} else {
		puts(label);
		puts("\n");
	}

	printf("Volume serial: %08x\n", vsn);

	return 0;
}

int fat_exists(const char *filename)
{
	loff_t size;
	int ret;

	debug("%s(filename=%s)\n", __func__, filename);

	ret = fat_size(filename, &size);
	if (ret)
		return 0;

	return 1;
}

int fat_size(const char *filename, loff_t *size)
{
	FRESULT ff_ret;
	FILINFO finfo;

	debug("%s(filename=%s)\n", __func__, filename);

	memset(&finfo, 0, sizeof(finfo));

	ff_ret = f_stat(filename, &finfo);
	if (ff_ret != FR_OK) {
		debug("f_stat() failed: %d\n", ff_ret);
		return -1;
	}

	*size = finfo.fsize;

	return 0;
}

int fat_read_file(const char *filename, void *buf, loff_t offset, loff_t len,
		  loff_t *actread)
{
	FRESULT ff_ret;
	FIL fil;
	UINT ff_actread;

	debug("%s(filename=%s, offset=%d, len=%d)\n", __func__, filename,
	      (int)offset, (int)len);

	ff_ret = f_open(&fil, filename, FA_READ);
	if (ff_ret != FR_OK) {
		debug("f_open() failed: %d\n", ff_ret);
		goto err;
	}

	if (!len)
		len = f_size(&fil) - offset;

	ff_ret = f_lseek(&fil, offset);
	if (ff_ret != FR_OK) {
		debug("f_lseek() failed: %d\n", ff_ret);
		goto err;
	}

	ff_ret = f_read(&fil, buf, len, &ff_actread);
	if (ff_ret != FR_OK) {
		debug("f_read() failed: %d\n", ff_ret);
		goto err;
	}
	debug("f_read() read %u bytes\n", ff_actread);
	*actread = ff_actread;

	ff_ret = f_close(&fil);
	if (ff_ret != FR_OK) {
		debug("f_close() failed: %d\n", ff_ret);
		goto err;
	}

	return 0;

err:
	printf("** Unable to read file %s **\n", filename);
	return -1;
}

#ifdef CONFIG_FAT_WRITE
int file_fat_write(const char *filename, void *buf, loff_t offset, loff_t len,
		   loff_t *actwrite)
{
	FRESULT ff_ret;
	FIL fil;
	UINT ff_actwrite;

	debug("%s(filename=%s, offset=%d, len=%d)\n", __func__, filename,
	      (int)offset, (int)len);

	ff_ret = f_open(&fil, filename, FA_WRITE | FA_OPEN_ALWAYS);
	if (ff_ret != FR_OK) {
		debug("f_open() failed: %d\n", ff_ret);
		goto err;
	}

	ff_ret = f_lseek(&fil, offset);
	if (ff_ret != FR_OK) {
		debug("f_lseek() failed: %d\n", ff_ret);
		goto err;
	}

	ff_ret = f_write(&fil, buf, len, &ff_actwrite);
	if (ff_ret != FR_OK) {
		debug("f_write() failed: %d\n", ff_ret);
		goto err;
	}
	debug("f_read() read %u bytes\n", ff_actwrite);
	*actwrite = ff_actwrite;

	ff_ret = f_close(&fil);
	if (ff_ret != FR_OK) {
		debug("f_close() failed: %d\n", ff_ret);
		goto err;
	}

	return 0;

err:
	printf("** Unable to write file %s **\n", filename);
	return -1;
}
#endif

typedef struct {
	struct fs_dir_stream parent;
	struct fs_dirent dirent;
	char fname[256];
	int iter;
} fat_dir;

int fat_opendir(const char *filename, struct fs_dir_stream **dirsp)
{
	FRESULT res;
	DIR ff_dir;
	fat_dir *dir;
	int ret;
	
	debug("%s(filename=%s)\n", __func__, filename);

	dir = malloc_cache_aligned(sizeof(*dir));
	if (!dir) {
		ret = -ENOMEM;
		goto fail_free_dir;
	}
	memset(dir, 0, sizeof(*dir));
	memset(&ff_dir, 0, sizeof(ff_dir));

	res = f_opendir(&ff_dir, filename);
	if (res != FR_OK) {
		ret = -ENOENT;
		goto fail_free_dir;
	}
	f_closedir(&ff_dir);
	
	strncpy(dir->fname, filename, 256);

	*dirsp = (struct fs_dir_stream *)dir;
	return 0;

fail_free_dir:
	printf("** Unable to open directory %s **\n", filename);

	free(dir);
	return ret;
}

int fat_readdir(struct fs_dir_stream *dirs, struct fs_dirent **dentp)
{
	FILINFO fno;
	DIR ff_dir;
	FRESULT res;
	fat_dir *dir = (fat_dir *)dirs;
	struct fs_dirent *dent = &dir->dirent;
	
	debug("%s()\n", __func__);

	memset(&fno, 0, sizeof(fno));
	memset(&ff_dir, 0, sizeof(ff_dir));
	
	res = f_opendir(&ff_dir, dir->fname);
	if (res != FR_OK) {
		return -ENOENT;
	}
	
	dir->iter++;
	for (int i = 0; i < dir->iter; i++) {
		res = f_readdir(&ff_dir, &fno);
		if (res != FR_OK || fno.fname[0] == 0) {
			return -ENOENT;
		}
	}
	
	f_closedir(&ff_dir);

	memset(dent, 0, sizeof(*dent));
	strcpy(dent->name, fno.fname);

	if (fno.fattrib & AM_DIR) {
		dent->type = FS_DT_DIR;
	} else {
		dent->type = FS_DT_REG;
		dent->size = fno.fsize;
	}

	*dentp = dent;

	return 0;
}

void fat_closedir(struct fs_dir_stream *dirs)
{
	fat_dir *dir = (fat_dir *)dirs;
	
	debug("%s()\n", __func__);
	
	free(dir);
}

void fat_close(void)
{
	debug("%s()\n", __func__);

	fat_dev = NULL;
	fat_part = NULL;
}
