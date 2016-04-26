#include "replay.h"

void main()
{
	replay("11.ascii","config.txt");
}

void replay(char *traceName,char *configName)
{
	struct config_info *config;
	struct trace_info *req,*front=NULL,*rear=NULL;
	int fd;
	char *buf;
	int i,j;
	int nowTime,reqTime;
	
	confg=(struct config_info *)malloc(sizeof(struct config_info));
	req=(struct trace_info *)malloc(sizeof(struct trace_info));
	config_read(config,configName);
	trace_read(front,rear,traceName);

	fd = open(config->device, O_DIRECT | O_SYNC | O_RDWR); 
	if(fd < 0) 
	{
		fprintf(stderr, "Value of errno: %d\n", errno);
       	printf("Cannot open\n");
       	exit(1);
	}

	if (posix_memalign((void**)&buf, MEM_ALIGN, LARGEST_REQUEST_SIZE * BYTE_PER_BLOCK))
	{
		fprintf(stderr, "Error allocating buffer\n");
		return 1;
	}
	for(i=0;i<LARGEST_REQUEST_SIZE * BYTE_PER_BLOCK;i++)
	{
		//Generate random alphabets to write to file
		buf[i]=(char)(rand()%26+65);
	}

	init_aio();

	while(front!=NULL)
	{
		queue_pop(front,rear,req);
		nowTime=time_now();
		reqTime=req->time;
		while(nowTime < reqTime)
		{
			nowTime=time_now();
		}
		perform_aio(fd,buf,req);
	}

}

void config_read(struct config_info *config,const char *filename)
{
	FILE *configFile;
	int name,value;
	char line[BUFSIZE];
	char *ptr;

	configFile=fopen(filename,"r");
	if(configFile==NULL)
	{
		printf("error: opening config file\n");
		exit(-1);
	}
	//read config file
	memset(line,0,sizeof(char)*BUFSIZE);
	while(fgets(line,sizeof(line),configFile))
	{
		if(line[0]=='#'||line[0]==' ') continue;
        ptr=strchr(line,'=');
        if(!ptr) continue;
        name=ptr-buf;	//the end of name string+1
        value=name+1;	//the start of value string
        while(line[name-1]==' ') name--;
        line[name]=0;

		if(strcmp(line,"device")==0)
		{
			sscanf(buf+value,"%s",config->device);
			config->deviceNum++;
		}
		else if(strcmp(line,"trace")==0)
		{
			sscanf(buf+value,"%s",config->traceFileName);
		}
		else if(strcmp(line,"log")==0)
		{
			sscanf(buf+value,"%s",config->logFileName);
		}
		memset(line,0,sizeof(char)*BUFSIZE);
	}
	fclose(configFile);
}

void trace_read(struct trace_info *front,struct trace_info *rear,const char *filename)
{
	FILE *traceFile;
	char line[BUFSIZE];
	struct trace_info* req;

	traceFile=fopen(filename,"r");
	req=(struct trace_info *)malloc(sizeof(struct trace_info));
	if(traceFile==NULL)
	{
		printf("error: opening trace file\n");
		exit(-1);
	}
	while(fgets(line,sizeof(line),traceFile))
	{
		sscanf(line,"%lf %d %lld %d %d",&req->time,&req->dev,&req->lba,&req->size,&req->type);
		//push into request queue
		req->time=req->time*1000;	//ms-->us
		req->size=req->size*BYTE_PER_BLOCK;
		req->lba=(req->lba%BLOCK_PER_DRIVE)*BYTE_PER_BLOCK;
		queue_push(front,rear,req);
	}
	fclose(traceFile);
}

int time_now()
{
	struct timeval now;
	gettimeofday(&now,NULL);
	return 1000000*now.tv_sec+now.tv_usec;	//us
}

int time_elapsed(int begin)
{
	int now=time_now();
	return now-begin;	//us
}

static void IOCompleted(sigval_t sigval)
{
	struct aiocb_info *cb;
	struct trace_info *req;
	int latency;
	int error;
	int count;

	cb=(struct aiocb_info *)sigval.sival_ptr;
	latency=time_elapsed(cb->beginTime);
	error=aio_error(cb);
	if(error)
	{
		if(error!=ECANCELED)
		{
			fprintf(stderr,"Error completing i/o:%d\n",error);
		}
		return;
	}
	count=aio_return(cb);
	if(count<(int)cb->aio_nbytes)
	{
		fprintf(stderr, "Warning I/O completed:%db but requested:%ldb\n",
			count,cb->aio_nbytes);
	}
	req=cb->req;
	printf("%d,%lld,%d,%d ",req->time,req->lba,req->size,req->type);
	printf("latency=%d\n",latency);
}

static struct aiocb_info *perform_aio(int fd, void *buf, struct trace_info *req)
{
	struct aiocb_info *cb;
	char *buf_new;
	int error=0;

	cb->aio_fildes = fd;
	cb->aio_nbytes = req.size*512;
	cb->aio_offset = req.lba*512;

	cb->aio_sigevent.sigev_notify = SIGEV_THREAD;
	cb->aio_sigevent.sigev_notify_function = IOCompleted;
	cb->aio_sigevent.sigev_value.sival_ptr = cb;

	//write and read different buffer
	if(!USE_GLOBAL_BUFF)
	{
		if (posix_memalign((void**)&buf_new, MEM_ALIGN, req.size)) 
		{
			fprintf(stderr, "Error allocating buffer\n");
		}
		cb->aio_buf = buf_new;
	}
	else
	{
		cb->aio_buf = buf;
	}

	cb->req=req;
	cb->beginTime=time_now();

	if(req.type==1)
	{
		error=aio_write(cb);
	}
	else if(req.type==0)
	{
		error=aio_read(cb);
	}
	if(error)
	{
		fprintf(stderr, "Error performing i/o");
		return NULL;
	}
	return cb;
}

static void init_aio()
{
	aioinit aioParam = {0};
	//two thread for each device is better
	aioParam.aio_threads = AIO_THREAD_POOL_SIZE;
	aioParam.aio_num = 2048;
	aioParam.aio_idle_time = 1;	
	aio_init(&aioParam);
}



void queue_push(struct trace_info *front,struct trace_info *rear,struct trace_info *req)
{
	struct trace_info* temp = (struct trace_info *)malloc(sizeof(struct trace_info));
	
	temp->time = req->time;
	temp->dev = req->dev;
	temp->lba = req->lba;
	temp->size = req->size;
	temp->type = req->type;
	
	temp->next = NULL;
	if(front == NULL && rear == NULL)
	{
		front = rear = temp;
	}
	else
	{
		rear->next = temp;
		rear = temp;
	}
}

void queue_pop(struct trace_info *front,struct trace_info *rear,struct trace_info *req) 
{
	struct trace_info* temp = front;
	if(front == NULL) 
	{
		printf("Queue is Empty\n");
		return;
	}

	req->time = front->time;
	req->dev  = front->dev;
	req->lba  = front->lba;
	req->size = front->size;
	req->type = front->type;
	
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