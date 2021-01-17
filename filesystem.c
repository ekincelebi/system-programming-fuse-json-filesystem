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
const char * filename = "example.json";

//reads the input file to a char pointer
static void read_json_file(){
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

static cJSON* cJSON_by_name(cJSON *parent, const char *name)
{
    cJSON * ptr = parent;
    while (ptr) {
		printf("%s %s\n", ptr->string, name);
        if (strcmp(ptr->string, name) == 0){
			return ptr;
		}
        ptr = ptr->next;
    }
    return NULL;
}

static cJSON * cJSON_by_path(cJSON * parent, const char * path){
	char *next_parent_name;
	cJSON * next_parent_1, * next_parent_2 = parent->child;
	if (strcmp("/", path) == 0)
        return root;
	else{
		int i = 0;
		///     Path: /club/basketball
		for(i = 1; path[i]; i++){
			if (path[i] == '/'){
				next_parent_name = calloc(i - 1, sizeof(char));
            	memcpy(next_parent_name, path + 1, i - 1);
				next_parent_1 = cJSON_by_name(next_parent_2, next_parent_name);
				return cJSON_by_path(next_parent_1, path+i);
			}
		}
		return cJSON_by_name(parent->child, path+1);
	}
}


///// json methods

static int do_getattr( const char *path, struct stat *stbuf, struct fuse_file_info *fi )
{
	int res = 0;

    memset(stbuf, 0, sizeof(struct stat));

	// ls mnt
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }

	//ls mnt/class...
	else{

        cJSON *ptr = cJSON_by_path(root, path);

        if (ptr == NULL) {
            stbuf->st_mode = S_IFREG | 0666;
            return -ENOENT;
        }

        cJSON *ptr_child = ptr->child;

        if (ptr->child != NULL) {
            stbuf->st_mode = S_IFDIR | 0755;
        }
		
		else {
            if (ptr->valuestring){
			    stbuf->st_mode = S_IFREG | 0666;
                stbuf->st_size = strlen(ptr->valuestring);
			}
            else
            	stbuf->st_mode = S_IFDIR | 0755;
        }
    }
    return res;

}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	cJSON *dir, *ptr;

    dir = cJSON_by_path(root, path);

    if (dir == NULL)
        return -ENOENT;


    ptr = dir->child;

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);
    while (ptr) {
        filler(buffer, ptr->string , NULL, 0);
        ptr = ptr->next;
    }
    return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
    size_t len;
    cJSON *ptr_data, *ptr = cJSON_by_path(root, path);

    if (!ptr)
        return -ENOENT;

    if (ptr->valuestring)
        len = strlen(ptr->valuestring);
    else
        return 0;

    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buffer, ptr->valuestring + offset, size);
    } 
	else
        size = 0;

    return size;
}

static char * cJSON_parent(const char * path){
	
	if(strcmp(path, "/") == 0){
		return "/";
	}

	cJSON * ptr = cJSON_by_path(root, path);
	if(ptr == NULL) return NULL;


	char * parent_path = NULL;
	int i = strlen(path) -1;

	for(; path[i] && i>=0; i--){
			if (path[i] == '/'){
				if(i==0){ //parent is root
					parent_path = calloc(i+1, sizeof(char));
					strcpy(parent_path, "/");
				}
				else{
					parent_path = calloc(i, sizeof(char));
					memcpy(parent_path, path, i);
				}
				break;
			}
		}

	return parent_path; 
}

static int do_unlink( const char *path, struct fuse_file_info *fi ){
	
	cJSON * deleted_entry = cJSON_by_path(root, path);
	cJSON * parent = cJSON_by_path(root, cJSON_parent(path));
	char * parent_path = cJSON_parent(path);
	
	if(deleted_entry == NULL)
		return -ENOENT;

	if(parent && deleted_entry) cJSON_DetachItemViaPointer(parent, deleted_entry);
	
	FILE * f = fopen(filename, "w");

	char * newJsonText = cJSON_Print(root);

	if(newJsonText) fprintf(f, "%s", newJsonText);
	else fprintf(stderr, "Deleted but cannot write to the file!");
	fclose(f);
	return 0;
}

static int do_open(const char * path, struct fuse_file_info *fi){
	return 0;
}

static struct fuse_operations operations = {
    .getattr	= do_getattr, //cd
    .readdir	= do_readdir, //ls
    .read		= do_read, //cat
	.unlink		= do_unlink,
	.open		= do_open,
	//.mkdir		= do_mkdir,
	//.mknod		= do_mknod,
};

int main( int argc, char *argv[] )
{
	read_json_file();
	printf("JSON structure ready\n");
	return fuse_main( argc, argv, &operations, NULL );
	
}
