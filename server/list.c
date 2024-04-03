#include <list.h>

void AppendNode(List *l, ListNode *node)
{
    ListNode *tmp = *l;

    while (tmp->next != NULL)
    {
        tmp = tmp->next;
    }

    tmp->next = node;
}

ListNode *CreateNode(ThreadData *data)
{
    ListNode *ret = (ListNode *)malloc(sizeof(ListNode));

    /* If NULL, error will be handled outside */
    if (ret != NULL)
    {
        ret->data = data;
        ret->next = NULL;
    }

    return ret;
}

void FreeList(List *head)
{
    while (*head != NULL)
    {
        ListNode *tmp = *head;
        *head = (*head)->next;
        free(tmp->data->tid);
        free(tmp->data);
        free(tmp);
    }
}

void PrintList(List l)
{
    ListNode *tmp = l;
    size_t i = 0;

    while (tmp != NULL)
    {
        ("Item [%lu]: TID: %lu - Complete: %u\n", i, (*tmp->data->tid), tmp->data->complete);
        ++i;
        tmp = tmp->next;
    }
}