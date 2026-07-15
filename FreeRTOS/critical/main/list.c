#include "list.h"
#include <stdlib.h>
#include <string.h>


//初始化
void list_init(list_t* t)
{
    memset(t,0,sizeof(list_t));
}

//添加一个元素
void list_push_back(list_t* t,int data)
{
    if(t->head == NULL)
    {
        t->head = (list_node*)malloc(sizeof(list_node));
        t->tail = t->head;
        t->head->data = data;
        t->head->next = NULL;
        t->head->pre = NULL;
        t->num++;
    }
    else
    {
        t->tail->next = (list_node*)malloc(sizeof(list_node));
        t->tail->next->pre = t->tail;
        t->tail = t->tail->next;
        t->tail->data = data;
        t->tail->next = NULL; 
        t->num++;
    }
}

//删除尾部一个元素
void list_pop_back(list_t* t)
{
    if(t->tail == NULL)
        return;
    list_node* node = t->tail;
    t->tail = t->tail->pre;
    if(t->tail)
    {
        t->tail->next = NULL;
    }
    else
    {
        t->head = NULL;
    }
    free(node);
    t->num--;
}
