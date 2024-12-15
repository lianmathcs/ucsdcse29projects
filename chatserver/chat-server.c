# include "http-server.h"
# include <string.h>
# include <stdlib.h>
# include <stdint.h>
# include <time.h>
# include <stdio.h>


#define MAX_CHATS 10000

struct Reaction{
	char* user;
	char* message;
};

struct Chat{
	uint32_t id;
	char *user;
	char *message;
	char *timestamp ;
	uint32_t num_reactions;
	struct Reaction* reactions; 
};

struct Post{
	char * user;
	char * message;
};


int num = 0;  // for the id 
char const *HTTP_404_NOT_FOUND = "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/plain\r\n\r\n";
char const *HTTP_200_OK  = "HTTP/1.1 200 OK\r\nContent-Type:text/plain\r\n\r\n";
char const *HTTP_500_INTERNAL_SERVER_ERROR = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\n";
struct  Chat *chats = NULL;     // store all the chats = posts + reactions


void crr_time(char *buffer, size_t buf_size){
	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);	
	strftime(buffer,buf_size,"%Y-%m-%d %H:%M:%S",tm_info);
}

uint8_t hex_to_byte(char c){
	if('0'<= c && c<= '9') return c -'0';
	if('a'<= c && c<= 'f') return c - 'a' + 10;
	if('A'<= c && c<= 'F') return c - 'A' + 10;
	return 0;
}

void url_decode(char *src,char *dest){
	int i= 0;
	char *p_dest = dest;
	char *p_src = src;
	if(src == NULL || dest  == NULL) return;
	while(*(p_src + i)!= 0){
		if(*(p_src + i)=='%'){ // %20 for space
			uint8_t a = hex_to_byte(*(p_src+i+1));
			uint8_t b = hex_to_byte(*(p_src+i+2));
			*p_dest = (unsigned char)(a << 4) | b;
			p_dest++;
			i+=2;
		}
		else{
			*p_dest = *(p_src + i);
		 	p_dest++;
		}
		i++;		
	}
	*p_dest = '\0';
}

// add chat 
uint8_t add_chat(char* username, char * message){
	chats = realloc(chats,sizeof(struct Chat)*(num+1));
	if(chats == NULL) return 1;

	struct Chat* chat = &chats[num];	
	char tm[20];					
	crr_time(tm, sizeof(tm));
	chat->num_reactions = 0;
	chat->user = NULL;
	chat->message= NULL;

	chat->id = num++;				
	chat->user = malloc(strlen(username) +1);
	if(chat->user == NULL) return 1;
	strcpy(chat->user,username);

	chat->message = malloc(strlen(message)+1);
	if(chat->message == NULL){
		free(chat->user); 
		return 1;
	}
	strcpy(chat->message, message);
	chat->timestamp = malloc(strlen(tm)+1);
	if(chat->timestamp == NULL){
		free(chat->user);
		free(chat->message);
		return 1;
	}
	strcpy(chat->timestamp,tm);
	
	chat->reactions = NULL;	
	chat->id++;
	return 0 ;
}


// add reaction 
uint8_t add_reaction(char* username,char*message, uint32_t id){

	struct Chat *chat = &chats[id]; // located to the currect reating chat
	struct Reaction *new_reactions = realloc(chat->reactions,sizeof(struct Reaction) *(chat->num_reactions+1));
	if(new_reactions == NULL) return 1;
	chat->reactions = new_reactions;		
	
	struct Reaction *reaction = &chat->reactions[chat->num_reactions];
	reaction->user = malloc(strlen(username)+1);
	if(reaction->user == NULL) return 1 ;	
	strcpy(reaction->user,username);

	reaction->message = malloc(strlen(message)+1);
	if(reaction->message==NULL){
		free(reaction->user);
		return 1; 
	}
	strcpy(reaction->message, message);
	chat->num_reactions++;
	return 0;
}

void respond_with_chats(int client_sock){
	char response_buff[BUFFER_SIZE];
	write(client_sock,HTTP_200_OK,strlen(HTTP_200_OK));
	int i,j ;
	for (i= 0; i < num; i++){	
		struct Chat *chat  = &chats[i];
		snprintf(response_buff,BUFFER_SIZE,"[#%d %s]\t%s: %s\n",i+1,chat->timestamp,chat->user,chat->message);
		write(client_sock,response_buff, strlen(response_buff));
		
		for(j = 0 ; j< chat->num_reactions;j++){
			snprintf(response_buff,BUFFER_SIZE,"\t\t\t\t(%s) %s\n",chat->reactions[j].user,chat->reactions[j].message);
			write(client_sock,response_buff, strlen(response_buff));
		}	
	}
	
}

void handle_post(int client_sock,char *path){
	char response_buff[BUFFER_SIZE];	
	struct Post *post = malloc(sizeof(struct Post));    		
	if(post==NULL) return;
	post->user = NULL;
	post->message= NULL;

	char * user_token_start = NULL;
	char * message_token_start = NULL;

	user_token_start = strstr(path,"user=");
	if(user_token_start!=NULL){
		user_token_start += 5;
		char *user_end  = strstr(user_token_start, "&");
		if(user_end == NULL){
			user_end = user_token_start + strlen(user_token_start);	
		}

		size_t username_length = (size_t)(user_end - user_token_start);
		if(username_length <= 15 ){
			post->user = malloc(username_length+1);			// **
			if(post->user == NULL){	
				free(post);	
				return;
			}
			strncpy(post->user,user_token_start,username_length);
			post->user[username_length] = '\0';
		}else{ // username is too long
			snprintf(response_buff,BUFFER_SIZE,"username is too long\n");	
			write(client_sock,HTTP_500_INTERNAL_SERVER_ERROR,strlen(HTTP_500_INTERNAL_SERVER_ERROR));
			write(client_sock,response_buff, strlen(response_buff));
			free(post);
			return;
		}
	}else{ // missing user name ?
		snprintf(response_buff,BUFFER_SIZE,"username parameter missing\n");	
		write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
		write(client_sock,response_buff,strlen(response_buff));
		free(post);
		return; 
	}
	
	message_token_start = strstr(path,"message=");
	if(message_token_start!=NULL){
		message_token_start += 8;			
		char* message_end = strchr( message_token_start,'&');
		if(message_end == NULL){
			message_end = message_token_start + strlen(message_token_start);
		}
		size_t message_length = (size_t)(message_end - message_token_start);
		if(message_length <= 255){ 
			post->message = malloc(message_length + 1);		//***
			if(post->message == NULL) {
				free(post->user);
				free(post);
				return;
			}
			strncpy(post->message,message_token_start,message_length);
			post->message[message_length ]= '\0';	
		
		}
		else{ // message is too long
			snprintf(response_buff,BUFFER_SIZE,"Message is too long\n");
			write(client_sock,HTTP_500_INTERNAL_SERVER_ERROR,strlen(HTTP_500_INTERNAL_SERVER_ERROR));
			write(client_sock,response_buff, strlen(response_buff));
			free(post->user);
			free(post);
			return;
		}
	}
	else{
		snprintf(response_buff,BUFFER_SIZE,"Message parameter missing\n");
		write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
		write(client_sock,response_buff,strlen(response_buff));
		free(post->user);	
		free(post);
		return;
	}
	
	if(num >= MAX_CHATS){	
		snprintf(response_buff,BUFFER_SIZE,"ChatsLimt\n");
		write(client_sock,HTTP_500_INTERNAL_SERVER_ERROR,strlen(HTTP_500_INTERNAL_SERVER_ERROR));
		write(client_sock,response_buff, strlen(response_buff));	
		free(post);
		return;
	}

	if(add_chat(post->user,post->message)!=0){
		write(client_sock,HTTP_500_INTERNAL_SERVER_ERROR,strlen(HTTP_500_INTERNAL_SERVER_ERROR));
		write(client_sock,response_buff, strlen(response_buff));	
		free(post->user);
		free(post->message);
		free(post);
		return; 
	}


	respond_with_chats(client_sock);
	free(post->user);
	free(post->message);
	free(post);
	write(client_sock,HTTP_200_OK,strlen(HTTP_200_OK));
}



void handle_reaction(int client_sock,char *path){
	char response_buff[BUFFER_SIZE];
	struct Reaction *reaction = malloc(sizeof(struct Reaction)); 		//*
	if(!reaction) return;
	reaction->user = NULL;
	reaction->message = NULL;
	
	char *user_token = NULL;
	char *react_token = NULL;
	char *id_token =NULL;
	char *id_num = NULL;
	uint32_t id;

	user_token = strstr(path,"user=");
	if(user_token!=NULL){
		user_token +=5;  // after = 
		char*user_end = strstr(user_token,"&");
		if(user_end== NULL){
			user_end = user_token + strlen(user_token);
		}
		size_t username_length = (size_t)(user_end-user_token);
		if(username_length <= 15){
			reaction->user = malloc(username_length+1);			// **
			if(reaction->user== NULL){
		      		free(reaction);	
		       		return;
			}
			strncpy(reaction->user,user_token,username_length);
			reaction->user[username_length] = '\0';
		}else{
			snprintf(response_buff,BUFFER_SIZE,"username is too long\n");
			write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
			write(client_sock,response_buff,strlen(response_buff));
			free(reaction);
			return;
		}	
	}else{
		snprintf(response_buff,BUFFER_SIZE,"Missing username\n");
		write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
		write(client_sock,response_buff,strlen(response_buff));
		free(reaction);
		return;
	}
		
	react_token = strstr(path,"message=");
	if(react_token!=NULL){
		react_token += 8;
		char *react_end = strstr(react_token,"&");
		if(react_end == NULL){
 			react_end = react_token + strlen(react_token);
		}
		size_t reaction_len = (size_t)(react_end - react_token);
	        if(reaction_len < 15){
				reaction->message = malloc(reaction_len +1);
				if(reaction->message == NULL){
					free(reaction->user);
					free(reaction);
					return;
				}
				strncpy(reaction->message,react_token,reaction_len);
				reaction->message[reaction_len] ='\0';						
			}else{
				snprintf(response_buff,BUFFER_SIZE,"Reaction Message too long");
				write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
				write(client_sock,response_buff,strlen(response_buff));		
				free(reaction->user);
				free(reaction);
				return;
			}
	}else{
		snprintf(response_buff,BUFFER_SIZE,"Message paramer missing\n");
		write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
		write(client_sock,response_buff,strlen(response_buff));
		free(reaction->user);
		free(reaction);
		return;	
	}		
		
	id_token = strstr(path, "id=");
	if(id_token!=NULL){
		id_token +=3;
		char * id_end = strchr(id_token,'\n');
		if(id_end == NULL) id_end = id_token + strlen(id_token);

		size_t id_len =(size_t)(id_end -id_token );
		id_num = malloc(id_len + 1);
		if(id_num!= NULL){
			strncpy(id_num,id_token,id_len);
			id_num[id_len] = '\0';
			id = atoi(id_num);
			id --; 	
			if(id < 0 || id >= num){
				snprintf(response_buff,BUFFER_SIZE,"ID invalid\n");		
				write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));		
				write(client_sock,response_buff,strlen(response_buff));		
				free(reaction->user);
				free(reaction->message);
				free(id_num);
				return;
			}		
		}else{
			snprintf(response_buff,BUFFER_SIZE,"ID invalid ID\n");
			write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
			write(client_sock,response_buff,strlen(response_buff));								
			free(reaction->user);
			free(reaction->message);
			free(id_num);
			free(reaction);
			return;	
		}
										
	}else{
		snprintf(response_buff,BUFFER_SIZE,"ID paremeber invalid\n");
  		write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
		write(client_sock,response_buff,strlen(response_buff));
		free(reaction->user);
		free(reaction->message);
		free(id_num);
		free(reaction);
		return; 	
	}
	struct Chat* chat  = &chats[id];
	if(chat->num_reactions>= 100){
		free(reaction->user);
		free(reaction->message);
		snprintf(response_buff,BUFFER_SIZE,"too many reactions\n");	
		write(client_sock,HTTP_500_INTERNAL_SERVER_ERROR,strlen(HTTP_500_INTERNAL_SERVER_ERROR));
		write(client_sock,response_buff, strlen(response_buff));
		return 1;		
	}
	
	if(add_reaction(reaction->user,reaction->message,id)!= 0){
		write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
		free(reaction->user);
		free(reaction->message);
		free(id_num);
		free(reaction);
		return;
	}	
	respond_with_chats(client_sock);
    free(reaction->user);
	free(reaction->message);
	free(id_num);
	free(reaction);
	write(client_sock,HTTP_200_OK,strlen(HTTP_200_OK));
}
void handle_edit(int client_sock,char *path){
	char response_buff[BUFFER_SIZE];
	char *id_token = NULL;
	char *message_token = NULL;

	int curr_id = 0;
	char *message = NULL;

	id_token = strstr(path,"id=");
	if(id_token!=NULL){
		id_token += 3;
		char *id_end = strstr(id_token,"&");
		if(id_end ==NULL){
			id_end = id_token + strlen(id_token);
		}
		char id_num[16];
		size_t id_length = (size_t)(id_end -id_token);
		if(id_length>=sizeof(id_num)){
			snprintf(response_buff,BUFFER_SIZE,"ID is too long\n");
			write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
			write(client_sock,response_buff,strlen(response_buff));	
			return;
		}
		strncpy(id_num, id_token,id_length);
		id_num[id_length]='\0';
		curr_id = atoi(id_num);
		if(curr_id <= 0 || curr_id > num){
			snprintf(response_buff,BUFFER_SIZE,"ID invalid\n");		
			write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));		
			write(client_sock,response_buff,strlen(response_buff));		
			return;
		}	
	}
	else{
		snprintf(response_buff,BUFFER_SIZE,"ID paremeber missing\n");
  		write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
		write(client_sock,response_buff,strlen(response_buff));
		return;
	}
	message_token = strstr(path,"message=");	
	if(message_token!=NULL){
		message_token += 8;
		char *message_end = strchr(message_token,'\n');
		if(message_end == NULL){
 			message_end = message_token + strlen(message_token);
		}
		size_t message_len = (size_t)(message_end - message_token);
	    
		if(message_len < 15){
				message = malloc(message_len +1);
				if(message == NULL) {
					snprintf(response_buff, BUFFER_SIZE, "Memory allocation failed\n");
                	write(client_sock, HTTP_404_NOT_FOUND, strlen(HTTP_404_NOT_FOUND));
                	write(client_sock, response_buff, strlen(response_buff));
					return;	
				}
				strncpy(message,message_token,message_len);
				message[message_len]= '\0';
		}else{
				snprintf(response_buff,BUFFER_SIZE,"message is too long\n");
				write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
				write(client_sock,response_buff,strlen(response_buff));
				return;
		}
	}else{
		snprintf(response_buff,BUFFER_SIZE,"message paremeber missing\n");
  		write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
		write(client_sock,response_buff,strlen(response_buff));
		return;
	}
	struct Chat* chat  = &chats[--curr_id];
	free(chat->message);
	chat->message = message;
	respond_with_chats(client_sock);
	write(client_sock,HTTP_200_OK,strlen(HTTP_200_OK));
}
void handle_404(int client_sock,char*path){
	printf("\nSERVER LOG: Got request for unrecognized path: \"%s\"\n", path);
	char response_buff[BUFFER_SIZE];
	snprintf(response_buff,BUFFER_SIZE,"Error 404:\r\nUnrecognized path:\%s\"\n",path);
	write(client_sock,HTTP_404_NOT_FOUND,strlen(HTTP_404_NOT_FOUND));
	write(client_sock,response_buff,strlen(response_buff));		
}
void handle_reset(int client_sock){
	char response_buff[BUFFER_SIZE];
	if(chats == NULL) return;
	int i, j;
	for( i = 0; i< num; i++){
		free(chats[i].user);
		free(chats[i].message);
		free(chats[i].timestamp);

		for(j=0; j < chats[i].num_reactions;j++){
			free(chats[i].reactions[j].user);	
			free(chats[i].reactions[j].message);
		}
		free(chats[i].reactions);
		chats->num_reactions = 0;
	}
	free(chats);
	num = 0;
	write(client_sock, HTTP_200_OK, strlen(HTTP_200_OK));
    write(client_sock, "", 0);	
}
void handle_response(char *request, int client_sock){

	char path[BUFFER_SIZE];	  //  http + request names + usernames + messages + id (including the constraints)
	printf("\nSERVER LOG :Got request: \"%s\"\n",request);
		
	if(sscanf(request,"GET %350s",path) != 1){ // sscanf return integers 
		printf("Invalid request line\n");
		return ;
	}
	
	char * decode_path = NULL;
	decode_path = malloc(strlen(path) + 1);
	if(decode_path == NULL) return ;
	
	url_decode(path,decode_path);

	if(strncmp(path,"/post",strlen("/post"))==0){			 
	  	handle_post(client_sock,decode_path);
		free(decode_path);
		return; 
	}	
	// handle reaction 
	else if(strncmp(path,"/react",strlen("/react"))==0){		 
		handle_reaction(client_sock,decode_path);
		free(decode_path);	
		return;	
	}

	// handle chats 
	else if(strncmp(path,"/chats",strlen("/chats"))== 0){			 
		respond_with_chats(client_sock);
		free(decode_path);
		return;
	}
	else if(strncmp(path,"/reset",strlen("/reset"))== 0){			 	      
		handle_reset(client_sock);
		free(decode_path);
		return;
	}
	else if(strncmp(path,"/edit",strlen("/edit"))==0){			 	      
		handle_edit(client_sock,decode_path);
		free(decode_path);
		return;
	}
	else{
		handle_404(client_sock,decode_path);	
		free(decode_path);
	}
}

int main(int argc,char *argv[]){		
	int port = 8000;
	if(argc>=2){
		port = atoi(argv[1]);
	}
	start_server(&handle_response,port);

}