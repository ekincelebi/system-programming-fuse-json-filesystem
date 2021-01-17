#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <cjson/cJSON.h>

//// cJSON methods

cJSON * root;

//reads the input file to a char pointer
static void read_json_file(const char * filename){
    char * myString = "0";
    long length;
    FILE * f = fopen(filename, "rb");

    if(f){
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        myString = malloc(length);

        if(myString){
            fread(myString, 1, length, f);
        }
        fclose(f);
    }
    root = cJSON_Parse(myString);
}

static cJSON* cJSON_by_key(cJSON *parent, const char *key)
{
    cJSON *dentry = parent;
    while (dentry) {
		printf("%s %s\n", dentry->string, key);
        if (strcmp(dentry->string, key) == 0){
            printf("condition holds\n");
			return dentry;
		}
        dentry = dentry->next;
    }
    return NULL;
}

static cJSON * cJSON_by_path(cJSON * parent, const char * path){
	char *next_parent_string;
	cJSON * next_parent, * parent_data = parent->child;
	if (strcmp("/", path) == 0)
        return root;
	else{
		int i = 1;
		///     Path: /club/basketball
		for(; path[i]; i++){
			if (path[i] == '/'){
				next_parent_string = calloc(i - 1, sizeof(char));
            	memcpy(next_parent_string, path + 1, i - 1);
				next_parent = cJSON_by_key(parent_data, next_parent_string);
				printf("by_path next parent str %s, new path %s\n", next_parent->string, path + i);
				return cJSON_by_path(next_parent, path+i);
			}
		}
		return cJSON_by_key(parent->child, path+1);
	}
}


///// json methods

static int do_getattr( const char *path, struct stat *stbuf, struct fuse_file_info *fi )
{
	int res = 0;

	// ls mnt
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }

	//ls mnt/class...
	else{

        memset(stbuf, 0, sizeof(struct stat));
        cJSON *dentry = cJSON_by_path(root, path);

        if (dentry == NULL) {
            stbuf->st_mode = S_IFREG | 0666;
            return -ENOENT;
        }

        cJSON *dentry_child = dentry->child;

        if (dentry->child != NULL) {
            stbuf->st_mode = S_IFDIR | 0755;
        }
		
		else {
            stbuf->st_mode = S_IFREG | 0666;
            if (dentry->valuestring)
                stbuf->st_size = strlen(dentry->valuestring);
            else
                stbuf->st_size = 0;
        }
    }
    return res;

}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	cJSON *dir, *dentry;

    dir = cJSON_by_path(root, path);

    if (dir == NULL)
        return -ENOENT;


    dentry = dir->child;

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);
    while (dentry) {
        filler(buffer, dentry->string , NULL, 0);
        dentry = dentry->next;
    }
    return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
    size_t len;
    cJSON *dentry_data, *dentry = cJSON_by_path(root, path);

    if (!dentry)
        return -ENOENT;

    if (dentry->valuestring)
        len = strlen(dentry->valuestring);
    else
        return 0;

    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buffer, dentry->valuestring + offset, size);
    } 
	else
        size = 0;

    return size;
}


static struct fuse_operations operations = {
    .getattr	= do_getattr, //cd
    .readdir	= do_readdir, //ls
    .read		= do_read, //cat
};

int main( int argc, char *argv[] )
{
	read_json_file("example.json");
	cJSON * deneme = cJSON_by_path(root, "/club/basketball");
	return fuse_main( argc, argv, &operations, NULL );
	
}
