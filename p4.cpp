#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <string>
#include <netdb.h>
#include <vector>
#include <time.h>

#define FAIL 0
#define OK 1

using namespace std;

int count =0,local_time=1;
int ssocket,csocket;
int *heartbeat;

typedef struct endp{
	int index;
	string ip;
	int port;
	int heartbeat;
	int update_time;
	int fail_time;
	int state;
} endp;

int N,b,c,F,B,P,S,T;
int tempB;

vector<endp> docs,docsb;
vector<endp>::iterator r,rb;

struct sockaddr_in serverAddr, clientAddr, other1, other2;

pthread_t serverthread;

const char s1 ='\n';
const char s2 =':';
char *token1,*token2,*token3;

void loaddocs();
int basic_setup();
void *lis_server_thread(void *);
void choose_b_nodes();
int sending = 1;
int *timestamp;

int *dead_array;

struct hostent *hp,*mhp;

int main(int argc, char **argv){
	int i;

	N = atoi(argv[1]);
	b = atoi(argv[2]);
	c = atoi(argv[3]);
	F = atoi(argv[4]);
	B = atoi(argv[5]);
	P = atoi(argv[6]);
	S = atoi(argv[7]);
	T = atoi(argv[8]);

/* Call for function for initial setup which includes creation of endpoints file, inserting local host ip */
	if(basic_setup()==0) cout<<"Basic Setup success"<<endl;

	ssocket = socket(PF_INET,SOCK_DGRAM,0);
	bind(ssocket,(struct sockaddr*) &serverAddr, sizeof(serverAddr));

	pthread_create(&serverthread,NULL,lis_server_thread,(void*)&docsb);

	dead_array = (int*) malloc( sizeof(int)*N);
	for(int k=0;k<N;k++){
		dead_array[k]=0;
	}

	printf("I am Node: %d\n", count);
/* Send heart beats to randomly chosen nodes; # of random nodes = b */

	heartbeat = (int*)malloc(sizeof(int)*N);
	other1.sin_family = AF_INET;
	
	for (int i = 0; i < N; ++i){
		heartbeat[i] = 0;
	}
	
	srand(S+count);
	
	unsigned int fs = (unsigned int) S;
	int cm=c;
	int ran;
	unsigned int hard_coded_seed = 100;
	while(local_time<=T){
	while(cm > 0){

		if(sending){
			choose_b_nodes();		/* Create new list of randomly chosen b ndes */
		}

		die:
		if(local_time%P==0 && B!=0){
			ran = rand_r(&hard_coded_seed)%N;
			//printf("RAND GENERATED %d\n",ran );	
			if(ran==count){
				sending = FAIL;
				docs.at(count).fail_time=local_time;
				cout<<"Node: "<<count<<" has failed at: "<<local_time<<"sec"<<endl;
				B=0;
			}else{
				dead_array[ran]++;
				if(dead_array[ran]==1){
					B--;
				}
				else{
					goto die;
				}
			}
		}
	
		if(sending){
			docs.at(count).heartbeat++;
			docs.at(count).update_time = local_time;
		
			i=0;
			for(r=docs.begin();r!=docs.end();r++){
				heartbeat[r->index]=r->heartbeat;
				i++;
			}
		

			for(rb=docsb.begin();rb!=docsb.end();rb++){
				mhp = gethostbyname(rb->ip.c_str());
        		memcpy((void *)&other2.sin_addr, mhp->h_addr_list[0], mhp->h_length);
				other2.sin_port = htons(rb->port);
			
				sendto(ssocket,heartbeat, sizeof(int)*N,0,(struct sockaddr*)&other2, (socklen_t)sizeof(other2));
			}
		}

		cm--;
		sleep(1);
		//printf("Decremented c to %d\n",cm);
		/* Check if any node has failed */
		
		if(local_time>=F && sending){
			int j=0;
			for(;j<N;j++){
				if(j==count)
					continue;
				if(local_time-timestamp[j]>=F){
					docs.at(j).state=FAIL;
					cout<<"I found out that Node: "<<j<<" has failed. Last heartbeat received at "<<timestamp[j]<<endl;
				}
			}
		}

		local_time++;
		
	}
	

	local_time++;
	//printf("Incremented local_time to %d\n",local_time);
	sleep(1);
}
	
	FILE *filePtr;
	char filename[100];
	sprintf(filename,"list%d",count);
	cout<<"ListX for this node is: "<<filename<<endl;
	filePtr = fopen(filename,"w");
	char list[1000];
	if(sending==1){
		sprintf(list, "%s\n","OK");
		fprintf(filePtr, list);
	}
	else{
		sprintf(list, "%s\n","FAIL");
		fprintf(filePtr, list);
	}
	for(r=docs.begin();r!=docs.end();r++){
		sprintf(list,"%d %d\n",r->index,r->update_time);
		fprintf(filePtr, list);
	}
	fclose(filePtr);

	return(0);	
}



int basic_setup(){
	
/* Declare variables */
	int i=0;
	int iport;
	char buf1[1024],buf2[1024];
	char hostname[100],port[100];
	int locN = N;

/* Get the file endpoints in buf1 and count the number of entries in the endpoints file */    
    FILE *fp;
    fp = fopen("endpoints","a+");
    
    while(1){
        buf1[i] = fgetc(fp);
        if(buf1[i]=='\n'){
            count++;
        }
        if(buf1[i]==EOF){
            break;
        }
        i++;
    }

/* Create your entry to be added to endpoints file */
	gethostname(hostname,100);
    hp = gethostbyname(hostname);
    iport = 5000+count;
    sprintf(port,":%d:\n",iport);
    strcat(hostname,port);

/* Create a socket */
	ssocket = socket(PF_INET,SOCK_DGRAM,0);

/*Configure settings in address struct*/
  	serverAddr.sin_family = AF_INET;
  	serverAddr.sin_port = htons(iport);
    memcpy((void *)&serverAddr.sin_addr, hp->h_addr_list[0], hp->h_length);
  	
/*Bind socket with address struct*/
  	bind(ssocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
	
/*Configure settings in destination address struct*/
  	other1.sin_family = AF_INET;
  		
	
/* If this is the ultimate entry */
	if(count==locN-1){
	/* Add entry to the file */	
		fprintf(fp, hostname);
		fclose(fp);

		int countl = count;
		token1 = strtok(buf1,"\n");
		token3 = strtok(NULL,"\0");
	/* Send OK UDP messages to the N-1 entries */	
		while(token3!="\0"){
			token2 = strtok(token1,":");
            hp = gethostbyname(token2);
            memcpy((void *)&other1.sin_addr, hp->h_addr_list[0], hp->h_length);
			token2=strtok(NULL,":");
            other1.sin_port = htons(atoi(token2));
			/*cout<<"Return of sendto: "<<*/sendto(ssocket,"OK\n", sizeof(char)*3,0,(struct sockaddr*)&other1, (socklen_t)sizeof(other1))/*<<endl*/;
			token1 = strtok(token3,"\n");
			token3 = strtok (NULL,"\0");
			
			countl--;
			if(countl==0) break;
		}	
	}
/* If not the ultimate entry */
	else{
	  
	  fprintf(fp, hostname);
	  fclose(fp);	 
	  i = recv(ssocket, buf2, sizeof(char)*512, 0);
	  //cout<<"Return message: "<<buf2<<endl;
	}

/* Close the ssocket */
	close(ssocket);

/* Create a local list of endpoints file */
	loaddocs();


	return 0;
}

/* Read entries from endpoints file and push the entries to the list */	
void loaddocs(){
	
	int i=0;
	char buf[1024]={0};
	endp item;
	FILE *fp;
    fp = fopen("endpoints","r");
    int index = 0;
    
	while(1){
		buf[i] = fgetc(fp);
		while(buf[i]!=EOF){
			string ip, port;
			while(buf[i]!=':'){
				ip.push_back(buf[i]);
				buf[++i]=fgetc(fp);
			}
			item.ip = ip;
			buf[++i]=fgetc(fp);
			while(buf[i]!=':'){
				port.push_back(buf[i]);
				buf[++i]=fgetc(fp);
			}
			item.port=atoi(port.c_str());
			item.index = index++;
			item.heartbeat = 5000;
			item.update_time = 0;
			item.state = OK;
			item.fail_time = 0;
			docs.push_back(item);
			buf[++i]=fgetc(fp);

			if(buf[i]=='\n'){
				buf[++i]=fgetc(fp);	
			}
		}
		break;    
    }
    fclose(fp);
}

void *lis_server_thread(void * args){

	int recvd[N],i=0;
	timestamp = (int*)malloc(sizeof(int)*N);
	for(i=0;i<N;i++){
		//flag[i]=0;
		timestamp[i]=0;
		recvd[i]=5000;
	}
	i=0;
	

	vector<endp>::iterator it1;
	int useless= 1;
	
	while(local_time!=T){
		recv(ssocket, recvd, sizeof(char)*1000,0);
		i = 0;
		
		for(it1=docs.begin();it1!=docs.end();it1++){		
			
			/*Current node has failed so only absorb*/
			if(!sending){
				continue;
			}


			if(it1->heartbeat<recvd[i]){
				it1->heartbeat=recvd[i];
				timestamp[it1->index]=local_time;
				it1->update_time=local_time;
				it1->fail_time=F;
				i++;
			}
			else{
				i++;
			}
		}	
	}
	
	pthread_exit(NULL);
}

/* Select b nodes randomly for gossip */
void choose_b_nodes(){
	
	docsb.erase(docsb.begin(),docsb.end()); 	/* Erase previous list of nodes */

    int countb = b,random;
    vector<endp>::iterator it_cbn;
    int repeat_count = 0;
    for(r=docs.begin();r!=docs.end();r++){
    	if(r->state==OK && r->index!=count) repeat_count++;
    }
    if(repeat_count<=b){	
    	for(r=docs.begin();r!=docs.end();r++){
    		if(r->state==OK && r->index!=count) docsb.push_back(*r);
    	}
    	return;
    }
    while(countb !=0){
    	repeat_count = 0;
    	repeat:
    		repeat_count++;
    		random = rand()%(N);
    		if(random==count || docs.at(random).state==FAIL){
    			goto repeat;
    		}
    	r = docs.begin();
    	while(random!=0){
    		r++;
    		random--;
    	}
    	for(it_cbn=docsb.begin();it_cbn!=docsb.end();it_cbn++){
    		if(it_cbn->index == r->index) goto repeat;
    	}
    	docsb.push_back(*r);
 		countb--;
    }

    /* Nodes chosen for this iteration */
    /*cout<<"New set of random nodes picked by Node: "<<count<<" for gossiping"<<endl;
    for(it_cbn=docsb.begin();it_cbn!=docsb.end();it_cbn++){
    	cout<<"IP: "<<it_cbn->ip<<" Port: "<<it_cbn->port<<endl;
    }*/

}
