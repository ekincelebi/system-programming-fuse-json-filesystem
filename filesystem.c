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

/*/////////////////////////// cJSON methods /////////////////////////////////////*/

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

static char * cJSON_parent(const char * path){
	
	if(strcmp(path, "/") == 0){
		return "/";
	}

	//cJSON * ptr = cJSON_by_path(root, path);
	//if(ptr == NULL) return NULL;

	////asd/adg
	char * parent_path = NULL;
	int i = strlen(path) -1;
	printf("path: %s\n", path);
	printf("length of path: %d\n", i);

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

//// /asd/def

static char * cJSON_basename(const char * path){
	char * basename, * parent_path;
	parent_path = cJSON_parent(path);
	if(strcmp(parent_path, "/")!=0){
		int parent_len = strlen(parent_path); 
		int path_len = strlen(path); 
		int diff = path_len - parent_len; 
		basename = calloc(diff, sizeof(char));
		memcpy(basename, path + parent_len +1, diff-1);
	}
	if(strcmp(parent_path, "/")==0){
		int parent_len = strlen(parent_path); 
		int path_len = strlen(path); 
		int diff = path_len - parent_len; 
		basename = calloc(diff, sizeof(char));
		memcpy(basename, path + parent_len, diff);
	}
	return basename;
}

static void cJSON_edit_source(){
	FILE * f = fopen(filename, "w");
	char * newJsonText = cJSON_Print(root);
	if(newJsonText) fprintf(f, "%s", newJsonText);
	else fprintf(stderr, "Cannot write to the file!");
	fclose(f);
}

/*/////////////////////////// cJSON methods /////////////////////////////////////*/

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
			stbuf->st_nlink = 2;
        }
		
		else {
            if (ptr->valuestring){
			    stbuf->st_mode = S_IFREG | 0666;
				stbuf->st_nlink = 1;
                stbuf->st_size = strlen(ptr->valuestring);
			}
            else
            	stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 2;
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


static int do_unlink( const char *path, struct fuse_file_info *fi ){
	
	cJSON * deleted_entry = cJSON_by_path(root, path);
	char * parent_path = cJSON_parent(path);
	cJSON * parent = cJSON_by_path(root, parent_path);
	
	if(deleted_entry == NULL)
		return -ENOENT;

	if(parent && deleted_entry) cJSON_DetachItemViaPointer(parent, deleted_entry);
	
	cJSON_edit_source();
	return 0;
}

static int do_open(const char * path, struct fuse_file_info *fi){
	
	return 0;
}

static int do_mknod( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi ){

	char * child_path = cJSON_basename(path);
	char *dir_path = cJSON_parent(path);
	printf("in mknod, path is %s\n", path);
	printf("in mknod, dir path is %s\n", dir_path);
	printf("in mknod, child path is %s\n", child_path);
	// set directory path that file is created in.
	/*char* last_slash_pos = strrchr(path, '/');
	strcpy(new_filename, last_slash_pos+1);
	/*if (last_slash_pos != NULL) { 
		*last_slash_pos = '\0';
	}*/

	// create cJSONs respectively
	cJSON * new_file_dir = cJSON_by_path(root, dir_path);
	cJSON * new_file = cJSON_CreateString(child_path);
	new_file->valuestring = calloc(1, sizeof(char));
	strcpy(new_file->valuestring, " ");
	cJSON_AddItemToObject(new_file_dir, child_path, new_file);
	cJSON_edit_source();
	return 0;
}

static int do_create(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi){
	return do_mknod(path, buffer, size, offset, fi);
}

static struct fuse_operations operations = {
    .getattr	= do_getattr, //cd
    .readdir	= do_readdir, //ls
    .read		= do_read, //cat
	.unlink		= do_unlink,
	.open		= do_mknod,
	//.mkdir		= do_mkdir,
	//.write		= do_write,
	.create		= do_create,
};

int main( int argc, char *argv[] )
{
	read_json_file();
	printf("JSON structure ready\n");
	return fuse_main( argc, argv, &operations, NULL );
	
}
