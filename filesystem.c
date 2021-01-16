#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// ... //

char dir_list[ 256 ][ 256 ];
int curr_dir_idx = -1;

char files_list[ 256 ][ 256 ];
int curr_file_idx = -1;

char files_content[ 256 ][ 256 ];
int curr_file_content_idx = -1;

void add_dir( const char *dir_name )
{
	curr_dir_idx++;
	strcpy( dir_list[ curr_dir_idx ], dir_name );
}

int is_dir( const char *path )
{	
	int curr_idx;
	path++; // Eliminating "/" in the path
	
	for ( curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ ){
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 ){
			return 1;
		}
	}
	
	return 0;
}

void add_file( const char *filename )
{
	curr_file_idx++;
	strcpy( files_list[ curr_file_idx ], filename );
	
	curr_file_content_idx++;
	strcpy( files_content[ curr_file_content_idx ], "" );
}

int is_file( const char *path )
{
	int curr_idx;
	path++; // Eliminating "/" in the path
	
	for (curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ ){
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 ){
			return 1;
		}
	}
	
	return 0;
}

int get_file_index( const char *path )
{
	int curr_idx;
	path++; // Eliminating "/" in the path
	
	for ( curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ ){
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 ){
			return curr_idx;
			\
		}
	}
	
	return -1;
}

void write_to_file( const char *path, const char *new_content )
{
	int file_idx = get_file_index( path );
	
	if ( file_idx == -1 ){
		printf("no such file");
		return;
	}
		
	strcpy( files_content[ file_idx ], new_content ); 
}

// ... //

static int do_getattr( const char *path, struct stat *st )
{
	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
	
	if ( strcmp( path, "/" ) == 0 || is_dir( path ) == 1 )
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
	}
	else if ( is_file( path ) == 1 )
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
	}
	else
	{
		return -ENOENT;
	}
	
	return 0;
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	int curr_idx;
	filler( buffer, ".", NULL, 0 ); // Current Directory
	filler( buffer, "..", NULL, 0 ); // Parent Directory
	
	if ( strcmp( path, "/" ) == 0 ) // If the user is trying to show the files/directories of the root directory show the following
	{
		for ( curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ ){
			filler( buffer, dir_list[ curr_idx ], NULL, 0 );}
	
		for ( curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ ){
			filler( buffer, files_list[ curr_idx ], NULL, 0 );}
	}
	
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	int file_idx = get_file_index( path );
	
	if ( file_idx == -1 )
		return -1;
	
	char *content = files_content[ file_idx ];
	
	memcpy( buffer, content + offset, size );
		
	return strlen( content ) - offset;
}



void make_directory( const char *path)
{
	path++;
	add_dir( path );

}

void make_file( const char *path)
{
	path++;
	add_file( path );
	
}


static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .readdir	= do_readdir,
    .read		= do_read,
};

struct dir{
	char name[10];
	char path[100];
	struct dir* childs;
	int childCnt;
	int isFile; // 0 if is dir, 1 if is file
};

struct Queue { 
    int front, rear;
	int size; 
    unsigned capacity; 
    struct dir** array; 
}; 
   
struct Queue* createQueue(unsigned capacity) 
{ 
    struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue)); 
    queue->capacity = capacity;
    queue->front = 0;
	queue->size = 0;
    queue->rear = capacity - 1; 

    (*queue->array) = (struct dir*)malloc(queue->capacity * sizeof(struct dir));
	
    return queue; 
} 
  
// Queue is full when size becomes 
// equal to the capacity 
int isFull(struct Queue* queue) 
{ 
    return (queue->size == queue->capacity); 
} 
  
// Queue is empty when size is 0 
int isEmpty(struct Queue* queue) 
{ 
    return (queue->size == 0); 
} 
  
// Function to add an item to the queue. 
// It changes rear and size 
void enqueue(struct Queue* queue, struct dir* item) 
{ 
    if (isFull(queue)) 
        return; 
    queue->rear = (queue->rear + 1) % queue->capacity; 
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1; 
    printf("%s enqueued to queue\n", item->name); 
} 
  
// Function to remove an item from queue. 
// It changes front and size 
struct dir* dequeue(struct Queue* queue) 
{ 
    if (isEmpty(queue))
        return NULL; 
    struct dir* item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity; 
    queue->size = queue->size - 1; 
    return item; 
} 
  
// Function to get front of queue 
struct dir* front(struct Queue* queue) 
{ 
    if (isEmpty(queue)) 
        return NULL; 
    return queue->array[queue->front];
} 
  
// Function to get rear of queue 
struct dir* rear(struct Queue* queue) 
{ 
    if (isEmpty(queue)) 
        return NULL; 
    return queue->array[queue->rear]; 
} 

void make_dirs_DFS(struct dir* root){
}

int main( int argc, char *argv[] )
{
	make_directory("/dir1");
	make_directory("/dir2");
	make_file("/dir0_1");
	write_to_file("/dir0_1","rootdaki file");
	
	struct dir myDir; //root dir
	myDir.childs = (struct dir*)malloc(100 * sizeof(struct dir));
	strcat(myDir.name, "/dir0");
	myDir.childCnt = 0;

	struct dir myDir1;
	myDir1.childs = (struct dir*)malloc(100 * sizeof(struct dir));;
	strcat(myDir.name, "/dir1");
	myDir1.childCnt = 0;
	myDir.childs[myDir.childCnt] = myDir1;
	myDir.childCnt++;
	strcpy(myDir1.path, myDir.path);
	strcat(myDir1.path, myDir1.name);
	
	struct dir myDir2;
	myDir2.childs = (struct dir*)malloc(100 * sizeof(struct dir));;
	myDir2.childCnt = 0;
	myDir.childs[myDir.childCnt] = myDir2;
	myDir.childCnt++;
	strcat(myDir.path, myDir1.path); 

	struct dir myDir3;
	myDir3.childs = (struct dir*)malloc(100 * sizeof(struct dir));;
	myDir3.childCnt = 0;
	myDir.childs[myDir.childCnt] = myDir3;
	myDir.childCnt++;
	strcat(myDir.path, myDir1.path); 

	struct dir myDir1_1;
	myDir1_1.childs = (struct dir*)malloc(100 * sizeof(struct dir));;
	myDir1_1.childCnt = 0;
	myDir1.childs[myDir1.childCnt] = myDir1_1;
	myDir1.childCnt++;
	strcat(myDir.path, myDir1.path); 

	struct dir myDir1_2;
	myDir1_2.childs = (struct dir*)malloc(100 * sizeof(struct dir));;
	myDir1_2.childCnt = 0;
	myDir1.childs[myDir1.childCnt] = myDir1_2;
	myDir1.childCnt++;	
	strcat(myDir.path, myDir1.path); 
	
	struct dir myDir3_1;
	myDir3_1.childs = (struct dir*)malloc(100 * sizeof(struct dir));;
	myDir3_1.childCnt = 0;
	myDir3.childs[myDir3.childCnt] = myDir3_1;
	myDir3.childCnt++;
	strcat(myDir.path, myDir1.path); 

	return fuse_main( argc, argv, &operations, NULL );
	
}
