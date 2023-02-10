#include<string>
#include<getopt.h>
#include<string.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../thirdparty/json.hpp"
#include<signal.h>
#include"../include/public.h"
#include<sys/time.h>
#include<iostream>
#include<queue>
using namespace std;
using  json=nlohmann::json;

string host="127.0.0.1";
int port=6001;
int mypipe[2];
int clients=1;
int benchtime=5;
int speed=0,failed=0,bytes=0;
int timerexpired=0;
queue<int> numble;

int sendRequest();
string Regrequest(string username);
string Loginrequest(int id);
string Logoutrequest(int id);
string getCurrentTime();
string Onechatrequest(int id,int friendid,string username);


void alarm_handler(int signal){
    timerexpired=1;
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

int bench();

int main(int argc,char** argv){
    for (int i = 2; i < 1000; i++)
    {
        numble.push(i);
    }
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
    return 0;
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

    
    struct sigaction sa;
    sa.sa_handler=alarm_handler;
    sa.sa_flags=0; //如果siginterrupt函数的flag参数设置为0时，如果系统调用被sig信号打断，则在处理完sig信号后，就会重新调用被打断的系统调用，这也是linux默认的行为。
    if(sigaction(SIGALRM,&sa,NULL)){
        exit(3);
    }
    if(pipe(mypipe))
    {
        perror("pipe failed.");
        return 3;
    }
    for(int i=0;i<clients;i++){
        pid=fork();
        sleep(1);
        if(pid <= 0){
            sleep(3);   //
            break;  // 要创建N个子进程这里就必须break跳出，否则子进程又会继续创建子进程，结果就会创建 2^N - 1个子进程
        }
    }
    if(pid<0){
        cout<<"error"<<endl;
        perror("fork failed.");
        return 3;
    }else if(pid==(pid_t)0){
        cout<<"I am child"<<getpid()<<endl;
        f=fdopen(mypipe[1],"w");
        if(f==NULL){
            perror("open pipe for writing failed.");
            return 3;
        }
        sendRequest();
        fprintf(f,"%d %d %d\n",speed,failed,bytes);
        fclose(f);    
    }else{
        cout<<"I am father"<<endl;
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
    // sendRequest();
    
    return 0;
}

int sendRequest(){
    alarm(benchtime);
    int s,i;

    while(!timerexpired){
        int s=Socket(host,port);
        
        if(s<0){
            cout<<"Socket error:"<<s<<endl;
            return -1;
        }
        //用户名
        struct timeval curtime;
        gettimeofday(&curtime,NULL);
        long cursec=curtime.tv_sec;
        long curusec=curtime.tv_usec;
        string username="User"+to_string(getpid())+"_"+to_string(cursec)+to_string(curusec);
        int sendSize;
        char buf[1500]={0};

        //注册请求
        // string regrequest=Regrequest(username);
        // sendSize=send(s,regrequest.c_str(),strlen(regrequest.c_str())+1,MSG_NOSIGNAL);
        // int recvSize=recv(s,buf,1500,0);
        // if(recvSize<0){
        //     failed++;
        //     cout<<errno<<endl;
        //     close(s);
        //     continue;
        // }else{
        //     json response=json::parse(buf);
        //     if(response["errno"].get<int>() !=0){ 
        //         failed++;
        //         close(s);
        //         continue;
        //     }
        //     bytes+=recvSize;
        //     string loginrequest=Loginrequest(response["id"].get<int>());
        //     sendSize=send(s,loginrequest.c_str(),strlen(loginrequest.c_str())+1,MSG_NOSIGNAL);
        //     int loginrecvSize=recv(s,buf,1500,0);
        //     if(loginrecvSize<0){
        //         failed++;
        //         close(s);
        //         continue;
        //     }else{
        //         json loginresponse=json::parse(buf);
        //         if(0 != loginresponse["errno"].get<int>()){
        //             failed++;
        //             close(s);
        //             continue;
        //         }
        //         bytes+=loginrecvSize;
        //         // string onechatrequest=Onechatrequest(response["id"].get<int>(),1,username);
        //         // sendSize=send(s,onechatrequest.c_str(),strlen(onechatrequest.c_str())+1,MSG_NOSIGNAL);

        //         string logoutrequest=Logoutrequest(response["id"].get<int>());
        //         sendSize=send(s,logoutrequest.c_str(),strlen(logoutrequest.c_str())+1,MSG_NOSIGNAL);
        //     }
        // }

        if(!numble.empty()){
            int ids=numble.front();
            numble.pop();
            string loginrequest=Loginrequest(ids);
            sendSize=send(s,loginrequest.c_str(),strlen(loginrequest.c_str())+1,MSG_NOSIGNAL);
            int loginrecvSize=recv(s,buf,1500,0);
            if(loginrecvSize<0){
                failed++;
                close(s);
                continue;
            }else{
                json loginresponse=json::parse(buf);
                if(0 != loginresponse["errno"].get<int>()){
                    failed++;
                    close(s);
                    continue;
                }
                bytes+=loginrecvSize;
                string onechatrequest=Onechatrequest(ids,1,username);
                sendSize=send(s,onechatrequest.c_str(),strlen(onechatrequest.c_str())+1,MSG_NOSIGNAL);

                string logoutrequest=Logoutrequest(ids);
                sendSize=send(s,logoutrequest.c_str(),strlen(logoutrequest.c_str())+1,MSG_NOSIGNAL);
            }
        }

        if(close(s)) {
            failed +=1;
            continue;
        }
        speed++;
        // usleep(10000);
    }
    return 0;
}

string Regrequest(string username){
    json js;
    string request;
    
    js["msgid"]=REG_MSG;
    js["password"]="123";
    // username="User"+to_string(getpid())+"_"+to_string(cursec)+to_string(curusec);
    js["name"]=username;
    request=js.dump();
    return request;
}

string Loginrequest(int id){
    json js;
    string request;
    js["msgid"]=LOGIN_MSG;
    js["id"]=id;
    js["password"]="123";
    request=js.dump();
    return request;
}

string Logoutrequest(int id){
    json js;
    js["msgid"]=LOGINOUT_MSG;
    js["id"]=id;
    string sendbuf=js.dump();
    return sendbuf;
}

string Onechatrequest(int id,int friendid,string username){
    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=id;
    js["name"]=username;
    js["toid"]=friendid;
    js["msg"]="chat message";
    js["time"]=getCurrentTime();
    string sendbuf=js.dump();
    return sendbuf;
}

//获取当前系统时间
string getCurrentTime(){
    time_t t=time(NULL);
    struct tm *ptm=localtime(&t);
    char date[60]={0};
    sprintf(date,"%d-%02d-%02d %02d:%02d:%02d",ptm->tm_year+1900,ptm->tm_mon+1,
            ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_min);
    return string(date);
}