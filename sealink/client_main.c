#include "common.h"

uint8_t* key;
volatile int g_LoginReady = 0;
volatile uint32_t g_FileSz = 0;
pthread_t g_Receiver;

void* __attribute__((noreturn)) client_msg_handler(void* args) {
	int bindsock = (int)args;
	struct sockaddr_in sin;
	uint32_t sinlen = sizeof(sin);
	int clisock;
	uint8_t* buffer;

	g_LoginReady = 1;
	clisock = accept(bindsock, &sin, &sinlen);
	if(clisock < 0) {
		LogEvent(LOG_ERROR, "FATAL : Connect back failed (%s)", __ERRSTR__);
		pthread_exit(-1);
	}

	close(bindsock); 	// only one

	LogEvent(LOG_INFO, "Server connected");
	while(1) {
		uint8_t* packet = srv_ReadPacket(clisock, MAX_PACKET, key);
		if(!packet) {
			LogEvent(LOG_INFO, "Server Disconnected");
			close(clisock);
			pthread_exit(-1);
		}
		PUSER_MSG msg = (PUSER_MSG)packet;
		if(msg->magic == USERMSG_MAGIC) { 	// msg recvd
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}

			printf("Message from (%d) : %s\n", msg->uid, buffer);

			free(buffer);
		} else if(msg->magic == USERFILE_MAGIC) {
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}

			PUMSG_FILE mfile = (PUMSG_FILE)buffer;
			printf("Incoming file from (%d) : %s (%d)\n", msg->uid, mfile->filename, mfile->filesz);
			/* RACE CONDITION PREVENTION ? */
			g_FileSz = mfile->filesz;
			free(buffer);
		} else if(msg->magic == USERLOG_MAGIC) {
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}
			PUMSG_LOGIN mlog = (PUMSG_LOGIN)buffer;
			printf("User (%d) %s\n", mlog->uid, mlog->disconnect ? "disconnected" : "connected");
			free(buffer);
		} else if(msg->magic == USERNAME_MAGIC) {
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}
			printf("User (%d) name changed to %s\n", msg->uid, buffer);
			free(buffer);
		} else if(msg->magic == USERFRIEND_MAGIC) {
			printf("User (%d) wants to be friend with you\n", msg->uid);
		} else if(msg->magic == USERBEFRIEND_MAGIC) {
			printf("User (%d) has become a friend of yours\n", msg->uid);
		} else if(msg->magic == USERDELFRIEND_MAGIC) {
			printf("User (%d) has deleted you from his friend list\n", msg->uid);
		} else if(msg->magic == USERNEWGROUP_MAGIC) {
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}
			printf("User (%d) has invited you to group (%u)\n", msg->uid, *(uint32_t*)buffer);
			free(buffer);
		} else if(msg->magic == USERGROUPMSG_MAGIC) {
			buffer = srv_ReadPacket(clisock, msg->msglen, key);
			if(!buffer) {
				LogEvent(LOG_INFO, "Transmission error");
				close(clisock);
				pthread_exit(-1);
			}
			PUMSG_GROUPMSG gm = (PUMSG_GROUPMSG)buffer;
			printf("GroupMsg (%u) From user (%u) : %s\n", gm->gid, msg->uid, gm->msg);
			free(buffer);
		}
		free(packet);
	}
}

void __attribute__((noreturn)) do_client(int sockfd) {
	
	uint8_t buffer[200], buffer2[200];
	uint8_t command[10];
	uint32_t uid,v,logsess;
	PPACKET packet;
	int w;
	uint8_t* result;
	uint32_t fcount, i;

	key = srv_DoHandshake(sockfd, "dh.param");
	if(!key) {
		exit(-1);
	}
	LogEvent(LOG_INFO, "Handshake complete");
	while(1) {
		scanf("%10s",command);
		if(strcmp(command, "REG") == 0) {
			scanf("%200s%200s", buffer, buffer2);
			packet = protocol_BuildRegisterPacket(buffer, buffer2);
			if(!packet) {
				LogEvent(LOG_WARNING, "build packet failed");
				continue;
			}
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			free(packet);
			
			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			v = protocol_ParseRegResultPacket((PPACKET)result, &uid);
			free(result);
		} else if(strcmp(command, "LOGIN") == 0) {
			scanf("%u%200s",&uid,buffer);
			packet = protocol_BuildLoginPacket(uid, buffer);

			if(!packet) {
				LogEvent(LOG_WARNING, "build packet failed");
				continue;
			}

			int retry = 10;
			int clisock;
			while(retry--) {
				packet->u.logreq.port = (uint16_t)getRandom32();
				clisock = srv_TCPListenConnectBack(packet->u.logreq.port);
				if(clisock < 0) 	continue;
				break;
			} 
			pthread_create(&g_Receiver, NULL, client_msg_handler, (void*)clisock);

			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			free(packet);
			
			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			v = protocol_ParseLogResultPacket((PPACKET)result, &logsess);
			free(result);
		} else if(strcmp(command, "MSG") == 0) {
			scanf("%u%200s",&uid,buffer);
			packet = protocol_BuildMsgPacket(uid, strlen(buffer));
			if(!packet) {
				LogEvent(LOG_WARNING, "build packet failed");
				continue;
			}
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w >= 0) {
				w = srv_WritePacket(sockfd, buffer, strlen(buffer), key);
			}
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			v = protocol_ParseMsgResultPacket((PPACKET)result);
			free(result);
		} else if(strcmp(command, "SENDFILE") == 0) {
			scanf("%u%200s",&uid,buffer);
			uint32_t filesz;
			if(protocol_GetFileInfo(buffer,&filesz) < 0) {
				printf("FILE_ERROR\n");
				continue;
			}
			packet = protocol_BuildSendFilePacket(uid, pathToFileName(buffer), filesz);
			if(!packet) {
				LogEvent(LOG_WARNING, "build packet failed");
				continue;
			}
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			v = protocol_ParseSFileResultPacket((PPACKET)result);
			if(v == 0) { 	// opening port for file sending
				srv_TCPStartFileTransfer(FILET_PORT, buffer, filesz);
			}
			free(result);
		} else if(strcmp(command, "GETFILE") == 0) {
			scanf("%200s", buffer);
			packet = protocol_BuildGetFilePacket(0xdeadbeef);
			if(!packet) {
				LogEvent(LOG_WARNING, "build packet failed");
				continue;
			}
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			v = protocol_ParseGFileResultPacket((PPACKET)result);
			packet = (PPACKET)result;
			if(v == 0) {
				struct in_addr t;
				t.s_addr = packet->u.gfileres.ipaddr;
				printf("GetFile, connecting (%s:%d)\n", inet_ntoa(t), packet->u.gfileres.port);
				v = srv_TCPFileTransfer(packet->u.gfileres.ipaddr, packet->u.gfileres.port, g_FileSz, buffer);
				if(v == 0) {
					printf("GETFILE_SUCCESS\n");
					packet = protocol_BuildFopDonePacket(0);
					w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
					if(w < 0) {
						LogEvent(LOG_ERROR, "cannot send");
						close(sockfd);
						exit(-1);
					}
					free(packet);
				} else {
					printf("TRANS_FAILED\n");
					packet = protocol_BuildFopDonePacket(-1);
					w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
					if(w < 0) {
						LogEvent(LOG_ERROR, "cannot send");
						close(sockfd);
						exit(-1);
					}
					free(packet);
				}
			} else {
				printf("GETFILE_FAILED\n");
			}

			free(result);
		} else if (strcmp(command, "CNAME") == 0) {
			scanf("%200s", buffer); 
			/* TODO */
			packet = protocol_BuildChangeNamePacket(buffer);
			if(!packet) {
				LogEvent(LOG_WARNING, "build packet failed");
				continue;
			}
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			v = protocol_ParseCNResultPacket(result);
			free(result);
		} else if(strcmp(command, "EXIT") == 0) {
			LogEvent(LOG_INFO, "User shutdown program");
			close(sockfd);
			exit(-1);
		} else if(strcmp(command, "ADDF") == 0) {
			scanf("%u", &uid);
			packet = protocol_BuildAddFriendPacket(uid);
			if(!packet) {
				LogEvent(LOG_WARNING, "build packet failed");
				continue;
			}
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			v = protocol_ParseAFResultPacket(result);
			free(result);
		} else if(strcmp(command, "BEF") == 0) {
			scanf("%u", &uid);
			packet = protocol_BuildBeFriendPacket(uid);
			if(!packet) {
				LogEvent(LOG_WARNING, "build packet failed");
				continue;
			}
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			v = protocol_ParseBFResultPacket(result);
			free(result);
		} else if(strcmp(command, "QUERYF") == 0) {
			packet = protocol_BuildQueryFriendPacket(0xdeadbeef);
			if(!packet) {
				LogEvent(LOG_WARNING, "build packet failed");
				continue;
			}
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			
			v = protocol_ParseQFResultPacket(result, &fcount);
			if(v == 0) {
				printf("FRIEND_COUNT %u\n", fcount);
				uint32_t* friends = srv_ReadPacket(sockfd, fcount*sizeof(uint32_t), key);
				if(!friends) {
					LogEvent(LOG_ERROR, "WTF READ!!");
					close(sockfd);
					exit(-1);
				}
				for(i=0;i<fcount;i++) {
					printf("FRIEND %u\n", friends[i]);
				}
				free(friends);
			}
			free(result);
		} else if(strcmp(command, "DELF") == 0) {
			scanf("%u", &uid);
			packet = protocol_BuildDeleteFriendPacket(uid);
			if(!packet) {
				LogEvent(LOG_WARNING, "build packet failed");
				continue;
			}
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			v = protocol_ParseDFResultPacket(result);
		} else if (strcmp(command, "GETNAME") == 0) {
			scanf("%u", &uid);
			packet = protocol_BuildGetNamePacket(uid);
			if(!packet) {
				LogEvent(LOG_WARNING, "build packet failed");
				continue;
			}
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			v = protocol_ParseGNResultPacket(result);
			if(v == 0) {
				packet = (PPACKET)result;
			}
				printf("NICK_NAME %s\n", packet->u.gnres.username);
			
			free(result);
		} else if (strcmp(command, "CGROUP") == 0) 	 {
			uint32_t ucount;
			scanf("%u",&ucount);
			uint32_t* buffer = malloc(sizeof(uint32_t)*ucount);
			for(i=0;i<ucount;i++) {
				scanf("%u", &buffer[i]);
			}
			packet = protocol_BuildCreateGroupPacket(ucount);
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			w = srv_WritePacket(sockfd, buffer, sizeof(uint32_t)*ucount, key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(buffer);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			uint32_t gid;
			v = protocol_ParseCGResultPacket(result, &gid);
			if(v == 0) {
				printf("GROUP_ID %u\n", gid);
			}
			free(result);
		} else if (strcmp(command, "GGROUPS") == 0) {
			packet = protocol_BuildGetGroupsPacket(0xdeadbeef);
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			uint32_t groupcnt;
			v = protocol_ParseGGResultPacket(result, &groupcnt);
			if(v == 0) {
				printf("GROUP_COUNT %u\n", groupcnt);
				uint32_t* groups = srv_ReadPacket(sockfd, groupcnt*sizeof(uint32_t), key);
				if(!groups) {
					LogEvent(LOG_ERROR, "server disconnected");
					close(sockfd);
					exit(-1);
				}
				for(i=0;i<groupcnt;i++) {
					printf("IN_GROUP %u\n", groups[i]);
				}
				free(groups);
			}
			free(result);
		} else if (strcmp(command, "GGUSERS") == 0) {
			uint32_t tgid;
			scanf("%u", &tgid);
			packet = protocol_BuildGetGroupUsersPacket(tgid);
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			uint32_t usercount;
			v = protocol_ParseGGUResultPacket(result, &usercount);
			if(v == 0) {
				printf("GROUP_USER_COUNT %u\n", usercount);
				uint32_t* users = srv_ReadPacket(sockfd,usercount*sizeof(uint32_t), key);
				if(!users) {
					LogEvent(LOG_ERROR, "server disconnected");
					close(sockfd);
					exit(-1);
				}
				for(i=0;i<usercount;i++) {
					printf("USER_IN_GROUP %u\n", users[i]);
				}
				free(users);
			}
			free(result);
		} else if(strcmp(command, "GMSG") == 0 ) {
			uint32_t to_gid;
			scanf("%u%200s", &to_gid, buffer);
			packet = protocol_BuildGroupMsgPacket(to_gid, strlen(buffer));
			w = srv_WritePacket(sockfd, packet, sizeof(PACKET), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}
			free(packet);

			w = srv_WritePacket(sockfd, buffer, strlen(buffer), key);
			if(w < 0) {
				LogEvent(LOG_ERROR, "cannot send");
				close(sockfd);
				exit(-1);
			}

			result = srv_ReadPacket(sockfd, MAX_PACKET, key);
			if(!result) {
				LogEvent(LOG_ERROR, "server disconnected");
				close(sockfd);
				exit(-1);
			}
			v = protocol_ParseGMResultPacket(packet);
			free(result);
		}
		if(v == 0) {
			printf("SUCCESS\n");
		} else {
			printf("FAILED %d\n", v);
		}
	}
}

int main(int argc, char* argv[], char* envp[])
{
	LogInit(LOG_PATH);
	LogEvent(LOG_INFO, "Starting LinPop client");

	int sock = srv_TCPConnect("127.0.0.1", SERVER_PORT);
	if(sock < 0) {
		exit(-1);
	} else {
		LogEvent(LOG_INFO, "Connected to (%s:%d)", "127.0.0.1", SERVER_PORT);
	}
	do_client(sock);
	return 0;
}