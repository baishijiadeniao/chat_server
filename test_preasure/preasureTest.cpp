#include<string>
#include<stdio.h>
#include<getopt.h>
#include<sys/socket.h>
#include<iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<string.h>
#include <unistd.h>
#include "../thirdparty/json.hpp"
#include<sys/time.h>
#include<signal.h>
#include"../include/public.h"
#include<errno.h>
using namespace std;
using  json=nlohmann::json;

string host="127.0.0.1";
int port=6001;
int clients=1;
int benchtime=30;
int mypipe[2];
int speed=0,failed=0,bytes=0;
volatile int timerexpired=0; //volatile 的意思是让编译器每次操作该变量时一定要从内存中真正取出，而不是使用已经存在寄存器中的值，

int bench();
void benchcore(const string,const int port);
string RegisRequest(pid_t pid);
int Socket(const string host,int clientPort);

void alarm_handler(int signal){
    timerexpired=1;
}

int main(int argc,char** argv){
    int opt=0;
    const char* str="p:a:t:c:";
    while((opt = getopt(argc,argv,str)) != -1){
        switch (opt)
        {
        case 'a':
            host=optarg;
            break;
        case 'p':
            port=atoi(optarg);
            break;
        case 't':
            benchtime=atoi(optarg);
            break;
        case 'c':
            clients=atoi(optarg);
            break;
        }
    }
    if(clients<1){
        clients=1;
    }
    if(benchtime<5){
        benchtime=5;
    }
    cout<<clients<<" clients,running "<<benchtime<<" sec"<<endl;
    bench();
}

int bench(){
    int i,j,k;
    pid_t pid=0;
    FILE* f;
     /* check avaibility of target server */
    i=Socket(host,port);
    if(i<0){
        fprintf(stderr,"\nConnect to server failed. Aborting benchmark.\n");
        return -1;
    }
    close(i);
    

    if(pipe(mypipe))
    {
        perror("pipe failed.");
        return 3;
    }
    for(i=0;i<clients;i++){
        pid=fork();
        if(pid <= 0){
            sleep(1);   //让父进程更快
            break;  // 要创建N个子进程这里就必须break跳出，否则子进程又会继续创建子进程，结果就会创建 2^N - 1个子进程
        }
    }
    if(pid <0){
        fprintf(stderr,"problems forking worker no. %d\n",i);
        perror("fork failed.");
        return 3;
    }
    if(pid==0){
        benchcore(host,port);
        f=fdopen(mypipe[1],"w");
        if(f==NULL){
            perror("open pipe for writing failed.");
            return 3;
        }
        fprintf(f,"%d %d %d\n",speed,failed,bytes);
        fclose(f);
        return 0;
    }
    if(pid >0){
        f=fdopen(mypipe[0],"r");
        if(f==NULL){
            perror("open pipe for writing failed.");
            return 3;
        }
        setvbuf(f,NULL,_IONBF,0); //设置为IO即时写入，无缓冲
        speed=0;
        failed=0;
        bytes=0;
        while(1){
            pid=fscanf(f,"%d %d %d",&i,&j,&k); //如果成功，该函数返回成功匹配和赋值的个数。如果到达文件末尾或发生读错误，则返回 EOF。
            if(pid<2){
                fprintf(stderr,"Some of our childrens died.\n");
                break;
            }
            speed +=i;
            failed +=j;
            bytes +=k;
            if(--clients==0) break;
        }
        fclose(f);
        printf("Speed=%d pages/min,%d bytes/sec.\nRequests: %d suceed, %d failed. \n",
        (int)((speed+failed)/(benchtime/60.0f)),(int)(bytes/(float)benchtime),
        speed,
        failed);
    }
    return i;
}

//每个子进程的循环
void benchcore(const string,const int port){
    int rlen;
    char buf[1500]={0};
    int s,i;
    string request;
    bool back_flag=false;
    struct sigaction sa;
    sa.sa_handler=alarm_handler;
    sa.sa_flags=0; //如果siginterrupt函数的flag参数设置为0时，如果系统调用被sig信号打断，则在处理完sig信号后，就会重新调用被打断的系统调用，这也是linux默认的行为。
    if(sigaction(SIGALRM,&sa,NULL)){
        exit(3);
    }
    alarm(benchtime);
    
    while(1)
    {   
        back_flag=false;
        if(timerexpired){
            if(failed>0){
                failed--;
            }
            break;
        }
        s=Socket(host,port);
        if(s<0) {
            failed++;
            return;
        }
        request=RegisRequest(getpid());
        rlen=request.length()+1;
        if(rlen != send(s,request.c_str(),strlen(request.c_str())+1,0)){    //提交注册请求
            failed++;
            close(s);
            continue;
        }
        while(1){
            if(timerexpired) 
                break;
            i=recv(s,buf,1500,0);   //接收应答信息
            if(i<0){
                cout<<"errno:"<<errno<<endl;
                failed++;
                cout<<"failed:"<<failed<<endl;
                close(s);
                back_flag=true;
                break;
            }else{
                json response=json::parse(buf);
                if(response["errno"].get<int>() !=0){ 
                    failed +=1;
                    close(s);
                    back_flag=true;
                    break;
                }
                bytes+=i;
                break;
            }
        }
        if(back_flag)
            continue;
        if(close(s)) {
            failed +=1;
            continue;
        }
        speed++;
    }
    return;
}

int Socket(const string host,int clientPort){
    int sock;
    unsigned long inaddr;
    struct sockaddr_in ad;
    memset(&ad,0,sizeof(ad));
    ad.sin_family=AF_INET;
    ad.sin_addr.s_addr=inet_addr(host.c_str());
    ad.sin_port=htons(clientPort);
    sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock<0){
        return sock;
    }
    if(connect(sock,(struct sockaddr*)&ad,sizeof(ad))<0)
        return -1;
    return sock;
}

//返回注册请求
string RegisRequest(pid_t pid){
    struct timeval curtime;
    gettimeofday(&curtime,NULL);
    long cursec=curtime.tv_sec;
    long curusec=curtime.tv_usec;
    json js;
    js["msgid"]=REG_MSG;
    string username="User"+to_string(pid)+"_"+to_string(cursec)+to_string(curusec);
    js["name"]=username;
    js["password"]="123";
    string request=js.dump();
    return request;
}