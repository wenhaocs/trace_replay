#include "replay.h"

void main()
{

}

void replay(char *traceName)
{
	struct config_info *config;
	struct trace_info *req,front=NULL,rear=NULL;
	int fd;
	char *buf;
	int i,j;
	int nowTime,reqTime;
	
	req=(struct trace_info *)malloc(sizeof(struct trace_info));
	config_read(config,"config.txt");
	while(trace_read(req,"hm_0.ascii")!=FAILURE)
	{
		req->time=req->time*1000;	//ms-->us
		req->size=req->size*BYTE_PER_BLOCK;
		req->lba=(req->lba%BLOCK_PER_DRIVE)*BYTE_PER_BLOCK;
		queue_push(front,rear,req);
	}

	fd = open(config->device, O_DIRECT | O_SYNC | O_RDWR); 
	if (fd < 0) 
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
	for(j=0;j<LARGEST_REQUEST_SIZE * BYTE_PER_BLOCK;j++)
	{
		//Generate random alphabets to write to file
		buf[j]=(char)(rand()%26+65);
	}

	/****************/
	replayer_aio_init();

	while(front)
	{
		queue_front(front,rear,req);
		queue_pop(front,rear);
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
	char line[SIZEBUF];
	char *ptr;

	configFile=fopen(filename,"r");
	if(configFile==NULL)
	{
		printf("error: opening config file\n");
		exit(-1);
	}
	
	//read config file
	memset(line,0,sizeof(char)*SIZEBUF);
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
			config->device_num++;
		}
		else if(strcmp(line,"trace")==0)
		{
			sscanf(buf+value,"%s",config->traceFileName);
		}
		else if(strcmp(line,"log")==0)
		{
			sscanf(buf+value,"%s",config->logFileName);
		}
		memset(line,0,sizeof(char)*SIZEBUF);
	}
	fclose(configFile);
}

int trace_read(struct trace_info *req,const char *filename)
{
	FILE *traceFile;
	char line[SIZEBUF];

	traceFile=fopen(filename,"r");
	if(traceFile==NULL)
	{
		printf("error: opening trace file\n");
		exit(-1);
	}
	while(fgets(line,sizeof(line),traceFile))
	{
		sscanf(line,"%lf %d %lld %d %d",&req->time,&req->dev,&req->lba,&req->size,&req->type);
		//push into request queue
	}
	fclose(traceFile);
	return SUCCESS;
}

int time_now()
{
	struct timeval now;
	gettimeofday(&now,NULL);
	return 1000000*now.tv_sec+now.tv_usec;
}
int time_elapsed(int begin)
{
	return time_now()-begin;
}

static void IOCompleted(sigval_t sigval)
{
	struct aiocb_info *request;
	int latency;
	int error;
	int count;

	request=(struct aiocb_info *)sigval.sival_ptr;
	latency=time_elapsed(request->beginTime);
	error=aio_error(request);
	if(error)
	{
		if(error!=ECANCELED)
		{
			fprintf(stderr,"Error completing i/o:%d\n",error);
		}
		return;
	}
	count=aio_return(request);
	if(count<(int)request->aio_nbytes)
	{
		fprintf(stderr, "Warning I/O completed:%db but requested:%ldb\n",
			count,request->aio_nbytes);
	}
	struct trace_info *io=request->req;
	fprintf(log,"%d\n",latency);

}

static struct aiocb_info *perform_aio(int fd, void *buf,struct trace_info *io)
{
	struct aiocb_info *cb;
	char *buf_new;
	int error=0;

	cb->aio_fildes = fd;
	cb->aio_nbytes = io.size*512;
	cb->aio_offset = io.lba*512;

	cb->aio_sigevent.sigev_notify = SIGEV_THREAD;
	cb->aio_sigevent.sigev_notify_function = IOCompleted;
	cb->aio_sigevent.sigev_value.sival_ptr = cb;

	//write and read different buffer
	if(!USE_GLOBAL_BUFF)
	{
		if (posix_memalign((void**)&buf_new, MEM_ALIGN, io.size)) 
		{
			fprintf(stderr, "Error allocating buffer\n");
		}
		cb->aio_buf = buf_new;
	}else
	{
		cb->aio_buf = buf;
	}

	cb->req=io;
	cb->beginTime=time_now();

	if(io.type==1)
	{
		error=aio_write(cb);
	}
	else if(io.type==0)
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

static void replayer_aio_init()
{
	aioinit aioParam = {0};
	//two thread for each device is better
	aioParam.aio_threads = AIO_THREAD_POOL_SIZE;
	aioParam.aio_num = 2048;
	aioParam.aio_idle_time = 1;	
	aio_init(&aioParam);
}
