/**
 * @file file.c
 *
 */
/* Copyright (C) 2017-2024 by Arjan van Vught mailto:info@orangepi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "../ff14b/source/ff.h"
#include <dirent.h>	/* DO NOT MOVE -> DIR is defined in ff.h */

static FIL file_object;
static FRESULT s_fresult;

static int fatfs_to_errno(const BYTE err) {
	switch (static_cast<FRESULT>(err)) {
	case FR_OK: 			/* FatFS (0) Succeeded */
		return (0); 		/* POSIX OK */
	case FR_DISK_ERR: 		/* FatFS (1) A hard error occurred in the low level disk I/O layer */
		return (EIO); 		/* POSIX Input/output error (POSIX.1) */
	case FR_INT_ERR: 		/* FatFS (2) Assertion failed */
		return (EPERM); 	/* POSIX Operation not permitted (POSIX.1) */
	case FR_NOT_READY: 		/* FatFS (3) The physical drive cannot work */
		return (EBUSY); 	/* POSIX Device or resource busy (POSIX.1) */
	case FR_NO_FILE: 		/* FatFS (4) Could not find the file */
		return (ENOENT); 	/* POSIX No such file or directory (POSIX.1) */
	case FR_NO_PATH: 		/* FatFS (5) Could not find the path */
		return (ENOENT); 	/* POSIX No such file or directory (POSIX.1) */
	case FR_INVALID_NAME: 	/* FatFS (6) The path name format is invalid */
		return (EINVAL); 	/* POSIX Invalid argument (POSIX.1) */
	case FR_DENIED: 		/* FatFS (7) Access denied due to prohibited access or directory full */
		return (EACCES); 	/* POSIX Permission denied (POSIX.1) */
	case FR_EXIST: 			/* FatFS (8) Access denied due to prohibited access */
		return (EACCES); 	/* POSIX Permission denied (POSIX.1) */
	case FR_INVALID_OBJECT: /* FatFS (9) The file/directory object is invalid */
		return (EINVAL); 	/* POSIX Invalid argument (POSIX.1) */
	case FR_WRITE_PROTECTED: /* FatFS (10) The physical drive is write protected */
		return (EROFS); 	/* POSIX Read-only filesystem (POSIX.1) */
	case FR_INVALID_DRIVE: 	/* FatFS (11) The logical drive number is invalid */
		return (ENXIO); 	/* POSIX No such device or address (POSIX.1) */
	case FR_NOT_ENABLED: 	/* FatFS (12) The volume has no work area */
		return (ENOSPC); 	/* POSIX No space left on device (POSIX.1) */
	case FR_NO_FILESYSTEM: 	/* FatFS (13) There is no valid FAT volume */
		return (ENXIO); 	/* POSIX No such device or address (POSIX.1) */
	case FR_MKFS_ABORTED: 	/* FatFS (14) The f_mkfs() aborted due to any parameter error */
		return (EINVAL); 	/* POSIX Invalid argument (POSIX.1) */
	case FR_TIMEOUT: 		/* FatFS (15) Could not get a grant to access the volume within defined period */
		return (EBUSY); 	/* POSIX Device or resource busy (POSIX.1) */
	case FR_LOCKED: 		/* FatFS (16) The operation is rejected according to the file sharing policy */
		return (EBUSY); 	/* POSIX Device or resource busy (POSIX.1) */
	case FR_NOT_ENOUGH_CORE: /* FatFS (17) LFN working buffer could not be allocated */
		return (ENOMEM); 	/* POSIX Not enough space (POSIX.1) */
	case FR_TOO_MANY_OPEN_FILES:/* FatFS (18) Number of open files > _FS_SHARE */
		return (EMFILE); 	/* POSIX Too many open files (POSIX.1) */
	case FR_INVALID_PARAMETER:/* FatFS (19) Given parameter is invalid */
		return (EINVAL); 	/* POSIX Invalid argument (POSIX.1) */
	}

	return (EBADMSG); 		/* POSIX Bad message (POSIX.1) */
}

extern "C" {
// http://elm-chan.org/fsw/ff/doc/open.html
FILE *fopen(const char *path, const char *mode) {
	assert(path != nullptr);
	assert(mode != nullptr);

	errno = 0;
	BYTE fm, fo;

	switch (mode[0]) {
	case 'r':
		fm = (BYTE) FA_READ;
		fo = 0;
		break;
	case 'w':
		fm = (BYTE) FA_WRITE;
		fo = (BYTE) FA_CREATE_ALWAYS;
		break;
	case 'a':
		fm = (BYTE) FA_WRITE;
		fo = (BYTE) FA_OPEN_APPEND;
		break;
	default:
		return nullptr;
		break;
	}

	while (*++mode != '\0') {
		switch (*mode) {
		case '+':
			fm = (BYTE) (FA_READ | FA_WRITE);
			break;
		case 'x':
			fo = (BYTE) FA_CREATE_NEW;
			break;
		default:
			return nullptr;
			break;
		}
	}

	s_fresult = f_open(&file_object, (TCHAR *) path, (BYTE) (fm | fo));
	errno = fatfs_to_errno(s_fresult);

	if (s_fresult == FR_OK) {
		return (FILE *) &file_object;
	} else {
		return nullptr;
	}
}

int fclose(FILE *stream) {
	errno = 0;

	if (stream == nullptr) {
		return 0;
	}

	s_fresult = f_close(&file_object);
	errno = fatfs_to_errno(s_fresult);

	if (s_fresult == FR_OK) {
		return 0;
	}

	return -1;
}

int fgetc(FILE *stream) {
	char c;
	UINT bytes_read;
	errno = 0;

	if (stream == nullptr) {
		return EOF;
	}

	if ((s_fresult = f_read(&file_object, &c, (UINT) 1, &bytes_read)) == FR_OK) {
		if (bytes_read > 0) {
			return c;
		}

		if (bytes_read < 1) {
			errno = fatfs_to_errno(s_fresult);
			return (EOF);
		}
	}

	errno = fatfs_to_errno(s_fresult);
	return (EOF);
}

size_t fread(void *ptr, size_t size, size_t nmemb, [[maybe_unused]] FILE *stream) {
	UINT bytes_read;

	s_fresult = f_read(&file_object, ptr, (size * nmemb), &bytes_read);
	errno = fatfs_to_errno(s_fresult);

	if (s_fresult == FR_OK) {
		return bytes_read;
	}

	return 0;
}

int fseek([[maybe_unused]] FILE *stream, long offset, int whence) {
	if (whence == SEEK_SET) {
		s_fresult = f_lseek(&file_object, (FSIZE_t) offset);
	} else if (whence == SEEK_END) {
		s_fresult = f_lseek(&file_object, f_size(&file_object));
	}

	errno = fatfs_to_errno(s_fresult);

	if (s_fresult == FR_OK) {
		return 0;
	}

	return -1;
}

long ftell([[maybe_unused]] FILE *stream) {
	return (long) f_tell(&file_object);
}

char *fgets(char *s, int size, FILE *stream) {
	assert(s != nullptr);
	errno = 0;

	if (stream == nullptr) {
		*s = '\0';
		return nullptr;
	}

	if (f_gets(s, size, &file_object) != s) {
		*s = '\0';
		errno = fatfs_to_errno(f_error(&file_object));
		return nullptr;
	}

	return s;
}

void clearerr([[maybe_unused]] FILE *stream) {
	s_fresult = FR_OK;
}

int ferror([[maybe_unused]] FILE *stream) {
	return s_fresult == FR_OK ? 0 : EOF;
}

/*
 *  The following API´s are implemented when CONFIG_FS_ENABLE_WRITE is defined
 */

int fputs([[maybe_unused]] const char *s, [[maybe_unused]] FILE *stream) {
#if !defined (CONFIG_FS_ENABLE_WRITE)
	errno = ENOSYS;
	return -1;
#else
	assert(s != nullptr);
	errno = 0;

	if (stream == nullptr) {
		return 0;
	}

	return f_puts(s, &file_object);
#endif
}

size_t fwrite([[maybe_unused]] const void *ptr, [[maybe_unused]] size_t size, [[maybe_unused]] size_t nmemb, [[maybe_unused]] FILE *stream) {
#if !defined (CONFIG_FS_ENABLE_WRITE)
	errno = ENOSYS;
	return 0;
#else
	UINT bytes_write;

	s_fresult = f_write(&file_object, ptr, (size * nmemb), &bytes_write);
	errno = fatfs_to_errno(s_fresult);

	if (s_fresult == FR_OK) {
		return bytes_write;
	}

	return 0;
#endif
}

int unlink([[maybe_unused]] const char *pathname) {
#if !defined (CONFIG_FS_ENABLE_WRITE)
	errno = ENOSYS;
	return -1;
#else
	s_fresult = f_unlink(pathname);
	errno = fatfs_to_errno(s_fresult);

	if (s_fresult == FR_OK) {
		return 0;
	}

	return -1;
#endif
}

#if !defined (CONFIG_FS_ENABLE_WRITE)
#else
static DIR s_dir;
static dirent_t s_dirent;
#endif

DIR *opendir([[maybe_unused]] const char *dirname) {
#if !defined (CONFIG_FS_ENABLE_WRITE)
	errno = ENOSYS;
	return 0;
#else
	const size_t len = strlen(dirname);

	if ((len > 0) && (dirname[len - 1] == '.')) {
		char *pathdir = (char *) dirname;
		pathdir[len - 1] = '\0';
		s_fresult = f_opendir(&s_dir, pathdir);
	} else {
		s_fresult = f_opendir(&s_dir, dirname);
	}

	errno = fatfs_to_errno(s_fresult);

	if (s_fresult != FR_OK) {
		return nullptr;
	}

	return &s_dir;
#endif
}

/**
 * On success, readdir() returns a pointer to a dirent structure.
 * If the end of the directory stream is reached, nullptr is returned and errno is not changed.
 * If an error occurs, nullptr is returned and errno is set appropriately.
 */

struct dirent *readdir([[maybe_unused]] DIR *dirp) {
#if !defined (CONFIG_FS_ENABLE_WRITE)
	errno = ENOSYS;
	return 0;
#else
	FILINFO fno;

	fno.fname[0] = '\0';

	s_fresult = f_readdir(&s_dir, &fno);

	if (s_fresult != FR_OK) {
		errno = fatfs_to_errno(s_fresult);
		return nullptr;
	}

	if (fno.fname[0] == '\0') {
		return nullptr;
	}

	errno = 0;

	strncpy(s_dirent.d_name, fno.fname, sizeof(s_dirent.d_name) - 1);

	if (fno.fattrib == AM_DIR) {
		s_dirent.d_type = DT_DIR;
	} else {
		s_dirent.d_type = DT_REG;
	}

	return &s_dirent;
#endif
}

int closedir([[maybe_unused]] DIR *dirp) {
#if !defined (CONFIG_FS_ENABLE_WRITE)
	errno = ENOSYS;
	return -1;
#else
	s_fresult = f_closedir(&s_dir);

	errno = fatfs_to_errno(s_fresult);

	if (s_fresult == FR_OK) {
		return 0;
	}

	return -1;
#endif
}
}
