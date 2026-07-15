#ifndef _LIST_H_
#define _LIST_H_

//定义链表节点结构体
typedef struct list_node
{
    int data;               // 4 字节 (offset 0)
    struct list_node* next; // 4 字节 (offset 4)
    struct list_node* pre;  // 4 字节 (offset 8)
}list_node;

//定义链表结构体
typedef struct
{
    list_node* head;  // 4 字节 (offset 0)
    list_node* tail;  // 4 字节 (offset 4)
    int num;          // 4 字节 (offset 8)
}list_t;

//初始化
void list_init(list_t* t);

//添加一个元素
void list_push_back(list_t* t,int data);

//删除尾部一个元素
void list_pop_back(list_t* t);

#endif