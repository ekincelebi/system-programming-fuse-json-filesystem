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
	printf("%s\n", newJsonText);
	if(newJsonText) fprintf(f, "%s", newJsonText);
	else fprintf(stderr, "Cannot write to the file!");
	fclose(f);
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
	cJSON * parent = cJSON_by_path(root, cJSON_parent(path));
	char * parent_path = cJSON_parent(path);
	
	if(deleted_entry == NULL)
		return -ENOENT;

	if(parent && deleted_entry) cJSON_DetachItemViaPointer(parent, deleted_entry);
	
	FILE * f = fopen(filename, "w");

	char * newJsonText = cJSON_Print(root);
	printf("%s\n", newJsonText);
	if(newJsonText) fprintf(f, "%s", newJsonText);

	else fprintf(stderr, "Deleted but cannot write to the file!");
	fclose(f);
	return 0;
}

static int do_open(const char * path, struct fuse_file_info *fi){
    return 0;
}

static int do_create(const char * path, struct fuse_file_info *fi){
	return do_open(path, fi);	
}

static int do_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	
	cJSON * ptr = cJSON_by_path(root, path);
	
	if(ptr!=NULL) return 0;

    char *parent_path = cJSON_parent(path);
    
	char *basename = cJSON_basename(path);
	
	if(basename == NULL) return 0;
    
	cJSON * parent_json = cJSON_by_path(root, parent_path);
	cJSON * new_json = cJSON_CreateObject();
    
	cJSON_AddStringToObject(parent_json, basename, "");
	
	ptr->valuestring = realloc(ptr->valuestring, (offset+size+1) * sizeof(char));

    memcpy(ptr->valuestring + offset, buf, size);

	ptr->valuestring[size] = '\0';
	
	cJSON_edit_source();

	return offset;
}

static int do_truncate(const char *path, off_t size)
{
    (void)path;
    (void)size;
    return 0;
}

static int do_release(const char *path, struct fuse_file_info *finfo)
{
    (void) path;
    (void) finfo;
    return 0;
}

static struct fuse_operations operations = {
    .getattr	= do_getattr, //cd
    .readdir	= do_readdir, //ls
    .read		= do_read, //cat
	.unlink		= do_unlink,
	.open		= do_open,
	.create		= do_create,
	.write		= do_write,
	.truncate	= do_truncate,
	.release	= do_release,
};

int main( int argc, char *argv[] )
{
	read_json_file();
	printf("JSON structure ready\n");
	return fuse_main( argc, argv, &operations, NULL );
	
}
