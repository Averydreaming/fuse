
#define FUSE_USE_VERSION 31
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define NAME_LENTH 256
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <pthread.h>
/*
    cd      : getattr + readdir
    ls      : readdir
    mkdir   : mkdir
    touch   : getattr + create
    cat     : getattr + read
    echo    : getattr + write
    rmdir   : rmdir
    rm      : unlink
*/
 
struct fs_node {
    int type; //1:file; 0:dictionary
    char filename[NAME_LENTH];
    char *contents;
	struct fs_node* pre;
	struct fs_node* nxt;
    struct fs_node* sons;
    struct fs_node* father;
};

struct fs_node* root;
int chat_flag=1;

struct fs_node* init_node(const char* name, int type){
	struct fs_node* tmp=malloc(sizeof(struct fs_node));
    tmp->type=type;
	strcpy(tmp->filename,name);
    if (type==1) tmp->contents = ""; else tmp->contents = NULL;
	tmp->pre=tmp->nxt=tmp->sons=tmp->father=NULL;
	return tmp;
}

struct fs_node* search_node(struct fs_node* fa, const char *name){
    if (strcmp(name, "")==0) return fa;
    struct fs_node* tmp=fa->sons;
    while (tmp!=NULL) {
        if (strcmp(tmp->filename, name)==0) return tmp;
        if (strcmp(tmp->filename, name)<0) tmp=tmp->nxt; else break;
    }
    return NULL;
}


int insert_node(struct fs_node* fa, struct fs_node* x){
    struct fs_node *tmp=fa->sons, *last=NULL;
    x->father=fa;
    if (tmp==NULL) { fa->sons=x; return 1; }
    while (tmp != NULL) {
        if (strcmp(tmp->filename, x->filename) == 0 && tmp->type==x->type) {
            free(x);
            return 0;
        } 
        if (strcmp(tmp->filename, x->filename)<=0) {last=tmp;tmp=tmp->nxt;} else break;
    }
    if (last==NULL) {
        x->nxt=tmp;
        tmp->pre=x;
        fa->sons=x;
    } else {
        last->nxt=x;
        x->pre=last;
        x->nxt=tmp;
        if (tmp!=NULL) tmp->pre=x;
    }
    return 1;
}

int delete_node(struct fs_node* fa, const char* name, int type){
    struct fs_node *tmp=fa->sons;
    if (tmp == NULL) return 0;
    while (tmp != NULL) {
        if (strcmp(tmp->filename, name)<0) tmp=tmp->nxt;
        else if (strcmp(tmp->filename, name)==0 && tmp->type==type) break;
        else return 0;
    }
    if (tmp == NULL) return 0;
    struct fs_node *x=tmp->pre, *y=tmp->nxt;
    if (x!=NULL) x->nxt=y;
    if (y!=NULL) y->pre=x;
    if (tmp==fa->sons) fa->sons=y;
    free(tmp);
    return 1;
}


struct fs_node* search_file(const char *path){
	if (strcmp(path, "/") == 0) return root;
	struct node* tmp = root;
	int len=strlen(path);
	for (int l=0,r=0;l<len;l=r+1) {
		r=l;
		while(r<len && path[r]!='/') r++;
		char *tmp_name=malloc(sizeof(char)*(r-l+1));
		for (int i=l;i<r;i++) tmp_name[i-l] = path[i];
		tmp_name[r-l]='\0';
		tmp=search_node(tmp,tmp_name);
		free(tmp_name);
        if (!tmp) return NULL;
	}
    return tmp;
}

const char *getSubstringAfterLastSlash(const char *str) {
  const char *lastSlash = strrchr(str, '/');
  if (lastSlash == NULL) {
    return str;
  } else {
    return lastSlash + 1;
  }
}

static void *fs_init(struct fuse_conn_info *conn,struct fuse_config *cfg){
	(void) conn;
	cfg->kernel_cache = 1;
	root=malloc(sizeof(struct fs_node));
	root->type=0;
	strcpy(root->filename, "root");
	root->contents = NULL;
	root->pre = root->nxt = root->father = root->sons = NULL;
	return NULL;
}

/*
const char *path 是要获取属性的文件或目录的路径。
struct stat *stbuf 是指向 struct stat 结构的指针，用于存储获取到的文件属性信息。
struct fuse_file_info *fi 是指向 struct fuse_file_info 结构的指针，提供有关文件的一些附加信息，如文件打开模式等。在 getattr 函数中，一般情况下，你可以忽略该参数。
*/
static int fs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi){
    printf("%s: %s\n", __FUNCTION__, path);
    memset(stbuf, 0, sizeof(struct stat));

	struct fs_node *pf =search_file(path);
 
    if (!pf) { return -ENOENT;}  
    else if(pf->type == 0){ //directory
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else { //file
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        if(pf->contents) stbuf->st_size = strlen(pf->contents); else stbuf->st_size = 0;
    }
    return 0;
/*
stbuf->st_mode 用于设置文件或目录的访问权限和类型。在这里，S_IFDIR 表示该属性为目录类型，而 0755 表示目录的访问权限为 rwxr-xr-x（即所有者具有读、写和执行权限，而其他用户具有读和执行权限）。
stbuf->st_nlink 表示链接数目，用于设置文件或目录的硬链接数量。文件夹设置为 2 表示目录的硬链接数量为 2，通常包括指向自身的链接和一个父目录的链接。而文件设置为1*/   
}

/*
const char *path：要读取的目录的路径。
void *buf：用于存储目录条目的缓冲区。
fuse_fill_dir_t filler：目录填充器（directory filler）函数指针，用于将目录条目添加到缓冲区中。
off_t offset：目录读取的偏移量。
struct fuse_file_info *fi：文件信息结构体指针，提供有关文件的一些附加信息。
enum fuse_readdir_flags flag：读取目录时的标志，用于指定一些选项，例如是否包含隐藏文件等。
*/
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flag){
    (void)offset;
    (void)fi; 
    printf("operation readdir: %s\n",path);

    struct fs_node* tmp = search_file(path);
    if(tmp == NULL || tmp->type == 1) return -ENOENT;
    
    filler(buf, ".", NULL, 0, 0);//当前目录
    if (strcmp(path, "/") != 0) { filler(buf, "..", NULL, 0, 0); }//上级目录

    struct fs_node* head=tmp->sons;
    if (head==NULL) puts("Dictionary is empty!"); else puts("Dictionary include");
    while(head != NULL){
        printf("%s ",head->filename);
        filler(buf, head->filename, NULL, 0, 0);
        head = head->nxt;
    }
	printf("\n");
    return 0;
}


static int fs_mkdir(const char *path, mode_t mode){
    printf("operation mkdir: %s\n", path);
	char Parent_path[strlen(path)+1];
	strcpy(Parent_path, path);
	Parent_path[strlen(path)] = '\0';
	for (int i = strlen(path)-1; i > 0; i--) {
		if (path[i] == '/') break;
		Parent_path[i] = '\0';
	}
    struct fs_node *fa = search_file(Parent_path);
	if (!fa || fa->type != 0) return -ENOENT;
 
    char P[strlen(path) + 1];
    strcpy(P, path);
    if(P[strlen(P) - 1] == '/') P[strlen(P) - 1]='\0';
    struct node* tmp= init_node(getSubstringAfterLastSlash(P), 0);

    if(insert_node(fa,tmp) == 1) return 0;
    else return -EEXIST;
}

static int fs_rmdir(const char *path) {
	puts("operation rmdir");
    struct fs_node* tmp=search_file(path);
    if (tmp == NULL || tmp->type == 1) return -ENOENT;
    if (delete_node(tmp->father,tmp->filename, 0) == 1) return 0;  else return -ENOENT;
}

/*
在这个框架中，你需要根据给定的文件路径、缓冲区、读取大小和偏移量等参数，实现具体的文件读取逻辑。通过使用适当的系统调用和文件描述符，可以从文件中读取数据，并将其存储在提供的缓冲区 buf 中。
*/

static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    printf("Operation read: %s\n", path);
    struct fs_node* tmp=search_file(path);
    if (tmp== NULL || tmp->type == 0) return -ENOENT;
    if (tmp->contents == NULL) return 0;
    int len = strlen(tmp->contents);
    if (offset < len){
        if (offset + size > len) size = len - offset;
        memcpy(buf, tmp->contents + offset, size);
    } else { size = 0; }
    return size;
}


static void add_contents(struct fs_node *target, const char *buf, size_t size, off_t offset)
{
    int l = strlen(target->contents);
    char *tmp = malloc(sizeof(char) * (l+1));
    strcpy(tmp, target->contents);
    target->contents = malloc(sizeof(char) * (l + size + offset + 1));
    memcpy(target->contents, tmp, l);
    memcpy(target->contents + l + offset, buf, size);
}

static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    printf("operation write: %s\n", path);
    struct fs_node *tmp =search_file(path);
    if (tmp == NULL || tmp->type == 0) return -ENOENT;
    if (chat_flag==0) {
        tmp->contents = malloc(sizeof(char) * (size + offset + 1));
        memcpy(tmp->contents + offset, buf, size);
    } else  {
        tmp->contents = malloc(sizeof(char) * (size + offset + 1));
        memcpy(tmp->contents + offset, buf, size);
        /*add_contents(tmp, buf, size, offset);*/
        char *chat_path, *send_path, *receive, *send;
        chat_path = malloc(sizeof(char) * (strlen(path)+1));
        send_path = malloc(sizeof(char) * (strlen(path)+1));
        strcpy(chat_path, path);
        strcpy(send_path, path);
        chat_path[strlen(path)] = send_path[strlen(path)] = '\0';
        int flag = 0;
        for (int i = strlen(path); i > 0; --i) {
            if (flag == 0) chat_path[i] = send_path[i] = '\0';
            else if (flag == 1) chat_path[i] = '\0';
            else break;
            if (path[i] == '/') flag++;
        }
        char *p = malloc(sizeof(char) * (strlen(path) + 1));
        strcpy(p, path);
        receive = strrchr(p, '/') ;
        send = strrchr(send_path, '/');
        strcat(chat_path, receive);
        strcat(chat_path, send);
        struct fs_node *tmp1=search_file(chat_path);
        char *t_buf = malloc(sizeof(char) * (strlen(buf) + 2));
        t_buf[0] = '>';
        strcpy(t_buf + 1, buf);
        add_contents(tmp1, t_buf, size+1, offset);
    }
    return size;
}   


static int fs_create(const char *path, mode_t mode,struct fuse_file_info *fi){
    printf("operation create: %s\n", path);
	char Parent_path[strlen(path)+1];
	strcpy(Parent_path, path);
	Parent_path[strlen(path)] = '\0';
	for (int i = strlen(path)-1; i > 0; i--) {
		if (path[i] == '/') break;
	    Parent_path[i] = '\0';
	}
    struct fs_node *fa = search_file(Parent_path);
	if (!fa || fa->type != 0) return -ENOENT;
 
    char P[strlen(path) + 1];
    strcpy(P, path);
    if(P[strlen(P) - 1] == '/') P[strlen(P) - 1]='\0';
    if (strlen(P) >= NAME_LENTH-1) return -EFBIG;
    struct node* now= init_node(getSubstringAfterLastSlash(P), 1);

    if(insert_node(fa,now) == 1) return 0;
    else return -EEXIST;
}

static int fs_unlink(const char *path) {
    struct fs_node* tmp=search_file(path);
    if (tmp==NULL || tmp->type == 0) return -ENOENT;
    if (delete_node(tmp->father, tmp->filename, 1) == 1) return 0; else return -ENOENT;
}

static int fs_utimens(const char *path, const struct timespec tv[2],
    struct fuse_file_info *fi){
    return 0;
}

static const struct fuse_operations fs_oper = {
    .init = fs_init,
    .getattr=fs_getattr,//用于获取文件或目录的属性

    .readdir=fs_readdir,
    .mkdir = fs_mkdir,
    .rmdir = fs_rmdir,
    
    .read=fs_read,
    .write=fs_write,

    .create = fs_create,
    .unlink = fs_unlink,
    .utimens = fs_utimens,
};
int main(int argc, char *argv[])
{
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	/* Parse options */
	//if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
	//	return 1;
	ret = fuse_main(args.argc, args.argv, &fs_oper, NULL);
	//fuse_opt_free_args(&args);
	return ret;
}