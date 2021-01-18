#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <cjson/cJSON.h>

cJSON * root;
static char * root_directory;

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

//here comes the mkdir function in fuse
void mkdir_eq(const char * directory){
    printf("mkdir: Created this directory: %s\n", directory);
}

//here comes the touch function in fuse
void touch_eq(const char * filename, const char * data){
    printf("touch: Created this file %s including %s\n", filename, data);
}

//parses the json and makes distinction between directories and files
static void parse_directory_source(cJSON * root, char * parent_dir){
    if(root){
    //root shows the outest json structure. it does not have a name, so
    //we can't create a directory with its name. we should iterate through
    //its children.
        if(root->string){
            cJSON * child_dir = root->child;

            //check if it is a leaf
            if(!child_dir){
                //if it doesn't have any children, the json represents a file. 
                touch_eq(root->string, root->valuestring);
                return;
            }

            //file path includes the parent path and child->string and a / at the end
            char new_path[strlen(root->string) + strlen(parent_dir) + 1];
            strcpy(new_path, parent_dir);
            strcat(new_path, root->string);
            mkdir_eq(strcat(new_path, "/"));
    
            //iterate through the children of the current json
            while(child_dir){
                //recursive call to the function
                parse_directory_source(child_dir, new_path);
                child_dir = child_dir->next;
            }
        }
        else{
            cJSON * child_dir = root->child;
            while(child_dir){
                parse_directory_source(child_dir, parent_dir);
                child_dir = child_dir->next;
            }
        }
    }
}

int main(int argc, char ** argv){
    read_json_file(argv[1]);
    printf("\nFilesystem structure: \n\n%s\n", cJSON_Print(root));
    parse_directory_source(root, "/");
    return 0;
}

