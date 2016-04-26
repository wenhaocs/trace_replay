/*Queue - Linked List implementation*/
#include "replay.h"

void queue_push(struct trace_info *front,struct trace_info *rear,struct trace_info *req)
{
	struct trace_info* temp =(struct trace_info *)malloc(sizeof(struct trace_info));
	
	temp->time	= req->time;
	temp->dev	= req->dev;
	temp->lba	= req->lba;
	temp->size	= req->size;
	temp->type	= req->type;
	
	temp->next = NULL;
	if(front == NULL && rear == NULL)
	{
		front = rear = temp;
		return;
	}
	rear->next = temp;
	rear = temp;
}

void queue_pop(struct trace_info *front,struct trace_info *rear) 
{
	struct trace_info* temp = front;
	if(front == NULL) 
	{
		printf("Queue is Empty\n");
		return;
	}
	if(front == rear) 
	{
		front = rear = NULL;
	}
	else 
	{
		front = front->next;
	}
	free(temp);
}

void queue_front(struct trace_info *front,struct trace_info *rear,struct trace_info *req)
{
	if(front == NULL) 
	{
		printf("Queue is empty\n");
		return;
	}
	req->time = front->time;
	req->dev  = front->dev;
	req->lba  = front->lba;
	req->size = front->size;
	req->type = front->type;
}

void queue_print(struct trace_info *front,struct trace_info *rear)
{
	struct trace_info* temp = front;
	while(temp != NULL) 
	{
		printf("%d ",temp->time);
		printf("%d ",temp->dev);
		printf("%lld ",temp->lba);
		printf("%d ",temp->size);
		printf("%d ",temp->type);
		temp = temp->next;
	}
	printf("\n");
}