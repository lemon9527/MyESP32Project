#ifndef __LIST_H__
#define __LIST_H__

//定义链表节点结构体
typedef struct list_node {
    int data;                 //数据指针
    struct list_node *next;     //下一个节点指针
    struct list_node *prev;     //上一个节点指针
} list_node_t;

#endif